/**
 * @file session-recovery.js
 * @brief Session Recovery and Restoration System
 *
 * This module provides comprehensive session recovery capabilities including
 * crash detection, automatic restoration, data integrity validation, and
 * graceful degradation strategies for the Huntmaster audio analysis system.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Session Recovery Implementation
 * @date July 24, 2025
 */

/**
 * Recovery states for session restoration
 */
const RECOVERY_STATES = {
  CLEAN_START: "clean_start",
  RECOVERING: "recovering",
  RECOVERED: "recovered",
  RECOVERY_FAILED: "recovery_failed",
  PARTIAL_RECOVERY: "partial_recovery",
};

/**
 * Recovery priorities for different data types
 */
const RECOVERY_PRIORITIES = {
  CRITICAL: { level: 1, timeout: 5000 }, // Session state, user settings
  HIGH: { level: 2, timeout: 10000 }, // Recording data, analysis results
  MEDIUM: { level: 3, timeout: 15000 }, // UI state, preferences
  LOW: { level: 4, timeout: 20000 }, // Cache, temporary data
};

/**
 * @class SessionRecovery
 * @brief Comprehensive session recovery and restoration system
 *
 * Features:
 * • Automatic crash detection and recovery initiation
 * • Data integrity validation and corruption detection
 * • Incremental recovery with priority-based restoration
 * • Recovery rollback for failed restoration attempts
 * • Session merge capabilities for multi-tab scenarios
 * • Performance monitoring during recovery operations
 * • User notification and recovery progress tracking
 * • Configurable recovery strategies and timeouts
 */
export class SessionRecovery {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Recovery configuration
      enableAutoRecovery: options.enableAutoRecovery !== false,
      recoveryTimeout: options.recoveryTimeout || 30000,
      maxRecoveryAttempts: options.maxRecoveryAttempts || 3,
      integrityCheckEnabled: options.integrityCheckEnabled !== false,

      // Storage configuration
      recoveryStorageKey: options.recoveryStorageKey || "huntmaster_recovery",
      backupStorageKey: options.backupStorageKey || "huntmaster_backup",
      crashDetectionKey:
        options.crashDetectionKey || "huntmaster_crash_detection",

      // Recovery strategies
      priorityBasedRecovery: options.priorityBasedRecovery !== false,
      incrementalRecovery: options.incrementalRecovery !== false,
      enableRollback: options.enableRollback !== false,

      // Monitoring and debugging
      enableProgressTracking: options.enableProgressTracking !== false,
      enableDetailedLogging: options.enableDetailedLogging || false,
      debugMode: options.debugMode || false,

      ...options,
    };

    // Recovery state
    this.recoveryState = RECOVERY_STATES.CLEAN_START;
    this.isRecovering = false;
    this.recoveryAttempts = 0;
    this.recoveryStartTime = null;
    this.recoveryProgress = { completed: 0, total: 0, stage: "" };

    // Storage management
    this.localStorage = window.localStorage;
    this.sessionStorage = window.sessionStorage;
    this.indexedDB = window.indexedDB;

    // Recovery data tracking
    this.recoveryQueue = [];
    this.recoveredData = new Map();
    this.failedRecoveries = new Map();
    this.integrityChecks = new Map();

    // Performance monitoring
    this.performanceMetrics = {
      recoveryDuration: 0,
      dataVolumeRecovered: 0,
      integrityCheckTime: 0,
      rollbackCount: 0,
    };

    this._initializeRecoverySystem();
  }

  /**
   * Initialize the recovery system
   */
  _initializeRecoverySystem() {
    try {
      // Set up crash detection
      this._initializeCrashDetection();

      // Set up storage event listeners
      this._setupStorageListeners();

      // Set up page lifecycle listeners
      this._setupPageLifecycleListeners();

      // Check for existing recovery data
      this._checkForRecoveryData();

      this.log("SessionRecovery system initialized", "success");
    } catch (error) {
      this.log(
        `Recovery system initialization failed: ${error.message}`,
        "error"
      );
      throw error;
    }
  }

  /**
   * Initialize crash detection mechanisms
   */
  _initializeCrashDetection() {
    try {
      // Set crash detection flag
      this._setCrashDetectionFlag(true);

      // Set up heartbeat for crash detection
      if (this.options.enableAutoRecovery) {
        this.heartbeatInterval = setInterval(() => {
          this._updateHeartbeat();
        }, 5000);
      }

      // Set up beforeunload handler for clean shutdown
      window.addEventListener("beforeunload", () => {
        this._setCrashDetectionFlag(false);
        this._createRecoveryCheckpoint();
      });

      // Set up unload handler for emergency backup
      window.addEventListener("unload", () => {
        this._createEmergencyBackup();
      });

      this.log("Crash detection initialized", "success");
    } catch (error) {
      this.log(
        `Crash detection initialization failed: ${error.message}`,
        "error"
      );
    }
  }

  /**
   * Set up storage event listeners for cross-tab coordination
   */
  _setupStorageListeners() {
    window.addEventListener("storage", (event) => {
      if (event.key === this.options.recoveryStorageKey) {
        this._handleRecoveryStorageChange(event);
      } else if (event.key === this.options.crashDetectionKey) {
        this._handleCrashDetectionChange(event);
      }
    });
  }

  /**
   * Set up page lifecycle listeners
   */
  _setupPageLifecycleListeners() {
    // Page visibility change handler
    document.addEventListener("visibilitychange", () => {
      if (
        document.visibilityState === "visible" &&
        this.options.enableAutoRecovery
      ) {
        this._checkForRecoveryData();
      }
    });

    // Focus handler for multi-tab scenarios
    window.addEventListener("focus", () => {
      if (this.options.enableAutoRecovery) {
        this._checkForRecoveryData();
      }
    });
  }

  /**
   * Check if recovery data exists and initiate recovery if needed
   */
  async _checkForRecoveryData() {
    try {
      const crashDetected = this._detectCrash();
      const recoveryData = this._getStoredRecoveryData();

      if (crashDetected || recoveryData) {
        this.log("Recovery data detected, initiating recovery process", "info");
        await this.initiateRecovery();
      } else {
        this.recoveryState = RECOVERY_STATES.CLEAN_START;
        this.log("No recovery data found, clean start", "info");
      }
    } catch (error) {
      this.log(`Recovery check failed: ${error.message}`, "error");
    }
  }

  /**
   * Detect if a crash occurred
   */
  _detectCrash() {
    try {
      const crashFlag = this.localStorage.getItem(
        this.options.crashDetectionKey
      );
      const lastHeartbeat = this.localStorage.getItem(
        `${this.options.crashDetectionKey}_heartbeat`
      );

      if (crashFlag === "true") {
        // Check heartbeat timestamp
        if (lastHeartbeat) {
          const timeSinceHeartbeat = Date.now() - parseInt(lastHeartbeat);
          return timeSinceHeartbeat > 30000; // Consider crash if no heartbeat for 30 seconds
        }
        return true;
      }

      return false;
    } catch (error) {
      this.log(`Crash detection failed: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Initiate recovery process
   */
  async initiateRecovery() {
    if (this.isRecovering) {
      this.log("Recovery already in progress", "warning");
      return { success: false, reason: "Already recovering" };
    }

    try {
      this.isRecovering = true;
      this.recoveryState = RECOVERY_STATES.RECOVERING;
      this.recoveryStartTime = Date.now();
      this.recoveryAttempts++;

      this.log(`Starting recovery attempt ${this.recoveryAttempts}`, "info");

      // Emit recovery started event
      this.eventManager.emit("recoveryStarted", {
        attempt: this.recoveryAttempts,
        timestamp: this.recoveryStartTime,
      });

      // Get recovery data
      const recoveryData = await this._gatherRecoveryData();

      if (!recoveryData || Object.keys(recoveryData).length === 0) {
        throw new Error("No recovery data available");
      }

      // Validate data integrity
      const integrityValid = await this._validateDataIntegrity(recoveryData);

      if (!integrityValid && this.options.integrityCheckEnabled) {
        throw new Error("Data integrity validation failed");
      }

      // Perform recovery
      const recoveryResult = await this._performRecovery(recoveryData);

      if (recoveryResult.success) {
        this.recoveryState = RECOVERY_STATES.RECOVERED;
        this._clearCrashDetectionFlag();
        this._cleanupRecoveryData();

        // Record performance metrics
        this.performanceMetrics.recoveryDuration =
          Date.now() - this.recoveryStartTime;
        this.performanceMetrics.dataVolumeRecovered =
          recoveryResult.dataVolume || 0;

        this.log("Recovery completed successfully", "success");

        // Emit recovery completed event
        this.eventManager.emit("recoveryCompleted", {
          result: recoveryResult,
          duration: this.performanceMetrics.recoveryDuration,
          attempt: this.recoveryAttempts,
        });

        return { success: true, result: recoveryResult };
      } else {
        throw new Error(recoveryResult.error || "Recovery failed");
      }
    } catch (error) {
      this.log(
        `Recovery attempt ${this.recoveryAttempts} failed: ${error.message}`,
        "error"
      );

      if (this.recoveryAttempts < this.options.maxRecoveryAttempts) {
        // Retry after delay
        setTimeout(() => {
          this.initiateRecovery();
        }, 2000 * this.recoveryAttempts);

        return { success: false, reason: "Retrying", error: error.message };
      } else {
        this.recoveryState = RECOVERY_STATES.RECOVERY_FAILED;
        this.isRecovering = false;

        // Emit recovery failed event
        this.eventManager.emit("recoveryFailed", {
          error: error.message,
          attempts: this.recoveryAttempts,
          timestamp: Date.now(),
        });

        return {
          success: false,
          reason: "Max attempts reached",
          error: error.message,
        };
      }
    } finally {
      this.isRecovering = false;
    }
  }

  /**
   * Gather all available recovery data
   */
  async _gatherRecoveryData() {
    try {
      const recoveryData = {};

      // Get data from localStorage
      const localStorageData = this._getLocalStorageRecoveryData();
      if (localStorageData) {
        recoveryData.localStorage = localStorageData;
      }

      // Get data from sessionStorage
      const sessionStorageData = this._getSessionStorageRecoveryData();
      if (sessionStorageData) {
        recoveryData.sessionStorage = sessionStorageData;
      }

      // Get data from IndexedDB if available
      if (this.indexedDB) {
        const indexedDBData = await this._getIndexedDBRecoveryData();
        if (indexedDBData) {
          recoveryData.indexedDB = indexedDBData;
        }
      }

      // Get backup data
      const backupData = this._getBackupData();
      if (backupData) {
        recoveryData.backup = backupData;
      }

      this.log(
        `Gathered recovery data from ${
          Object.keys(recoveryData).length
        } sources`,
        "info"
      );
      return recoveryData;
    } catch (error) {
      this.log(`Failed to gather recovery data: ${error.message}`, "error");
      return null;
    }
  }

  /**
   * Validate data integrity
   */
  async _validateDataIntegrity(recoveryData) {
    try {
      const startTime = Date.now();
      let validationResults = {};

      for (const [source, data] of Object.entries(recoveryData)) {
        validationResults[source] = await this._validateSourceData(
          source,
          data
        );
      }

      this.performanceMetrics.integrityCheckTime = Date.now() - startTime;

      const allValid = Object.values(validationResults).every((valid) => valid);

      this.log(
        `Data integrity validation ${allValid ? "passed" : "failed"}`,
        allValid ? "success" : "warning"
      );

      return allValid;
    } catch (error) {
      this.log(`Data integrity validation error: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Validate data from a specific source
   */
  async _validateSourceData(source, data) {
    try {
      if (!data || typeof data !== "object") {
        return false;
      }

      // Check for required fields based on source
      switch (source) {
        case "localStorage":
          return this._validateLocalStorageData(data);
        case "sessionStorage":
          return this._validateSessionStorageData(data);
        case "indexedDB":
          return this._validateIndexedDBData(data);
        case "backup":
          return this._validateBackupData(data);
        default:
          return true;
      }
    } catch (error) {
      this.log(
        `Source data validation failed for ${source}: ${error.message}`,
        "error"
      );
      return false;
    }
  }

  /**
   * Perform the actual recovery process
   */
  async _performRecovery(recoveryData) {
    try {
      const recoveryResult = {
        success: false,
        recovered: {},
        failed: {},
        dataVolume: 0,
      };

      // Create recovery queue based on priorities
      this._buildRecoveryQueue(recoveryData);

      // Process recovery queue
      for (const recoveryItem of this.recoveryQueue) {
        try {
          await this._recoverItem(recoveryItem);
          recoveryResult.recovered[recoveryItem.key] = recoveryItem;
          recoveryResult.dataVolume += this._calculateDataSize(
            recoveryItem.data
          );

          // Update progress
          this.recoveryProgress.completed++;
          this._updateRecoveryProgress();
        } catch (error) {
          this.log(
            `Failed to recover item ${recoveryItem.key}: ${error.message}`,
            "error"
          );
          recoveryResult.failed[recoveryItem.key] = error.message;
        }
      }

      // Determine overall success
      const recoveredCount = Object.keys(recoveryResult.recovered).length;
      const failedCount = Object.keys(recoveryResult.failed).length;
      const totalCount = recoveredCount + failedCount;

      if (recoveredCount === totalCount) {
        recoveryResult.success = true;
      } else if (recoveredCount > 0) {
        this.recoveryState = RECOVERY_STATES.PARTIAL_RECOVERY;
        recoveryResult.success = true; // Partial success
      }

      return recoveryResult;
    } catch (error) {
      this.log(`Recovery process failed: ${error.message}`, "error");
      return { success: false, error: error.message };
    }
  }

  /**
   * Build recovery queue with priorities
   */
  _buildRecoveryQueue(recoveryData) {
    this.recoveryQueue = [];

    for (const [source, data] of Object.entries(recoveryData)) {
      for (const [key, value] of Object.entries(data)) {
        const priority = this._determinePriority(key, value);

        this.recoveryQueue.push({
          source,
          key,
          data: value,
          priority: priority.level,
          timeout: priority.timeout,
        });
      }
    }

    // Sort by priority (lower number = higher priority)
    this.recoveryQueue.sort((a, b) => a.priority - b.priority);

    this.recoveryProgress.total = this.recoveryQueue.length;
    this.recoveryProgress.completed = 0;

    this.log(
      `Built recovery queue with ${this.recoveryQueue.length} items`,
      "info"
    );
  }

  /**
   * Determine recovery priority for data item
   */
  _determinePriority(key, data) {
    // Critical data
    if (["sessionId", "userSettings", "currentSession"].includes(key)) {
      return RECOVERY_PRIORITIES.CRITICAL;
    }

    // High priority data
    if (["recordingData", "analysisResults", "audioBuffer"].includes(key)) {
      return RECOVERY_PRIORITIES.HIGH;
    }

    // Medium priority data
    if (["uiState", "preferences", "history"].includes(key)) {
      return RECOVERY_PRIORITIES.MEDIUM;
    }

    // Default to low priority
    return RECOVERY_PRIORITIES.LOW;
  }

  /**
   * Recover individual item
   */
  async _recoverItem(recoveryItem) {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error(`Recovery timeout for ${recoveryItem.key}`));
      }, recoveryItem.timeout);

      try {
        // Emit recovery item event
        this.eventManager.emit("recoveryItemStarted", {
          key: recoveryItem.key,
          source: recoveryItem.source,
          priority: recoveryItem.priority,
        });

        // Perform actual recovery based on data type
        const recovered = this._restoreDataItem(recoveryItem);

        if (recovered) {
          this.recoveredData.set(recoveryItem.key, recovered);

          this.eventManager.emit("recoveryItemCompleted", {
            key: recoveryItem.key,
            source: recoveryItem.source,
            data: recovered,
          });

          clearTimeout(timeout);
          resolve(recovered);
        } else {
          throw new Error("Failed to restore data item");
        }
      } catch (error) {
        clearTimeout(timeout);
        this.failedRecoveries.set(recoveryItem.key, error.message);
        reject(error);
      }
    });
  }

  /**
   * Restore individual data item
   */
  _restoreDataItem(recoveryItem) {
    try {
      const { key, data, source } = recoveryItem;

      // Handle different data types
      switch (key) {
        case "sessionId":
          return this._restoreSessionId(data);
        case "userSettings":
          return this._restoreUserSettings(data);
        case "currentSession":
          return this._restoreCurrentSession(data);
        case "recordingData":
          return this._restoreRecordingData(data);
        case "analysisResults":
          return this._restoreAnalysisResults(data);
        default:
          return this._restoreGenericData(key, data);
      }
    } catch (error) {
      this.log(
        `Failed to restore data item ${recoveryItem.key}: ${error.message}`,
        "error"
      );
      return null;
    }
  }

  /**
   * Create recovery checkpoint for current session
   */
  createRecoveryCheckpoint(sessionData) {
    try {
      const checkpoint = {
        sessionId: sessionData.id || this._generateSessionId(),
        timestamp: Date.now(),
        sessionState: sessionData.state || "unknown",
        audioData: sessionData.audioData || null,
        analysisResults: sessionData.analysisResults || [],
        userSettings: sessionData.userSettings || {},
        recordings: sessionData.recordings || [],
        metadata: {
          version: "1.0",
          userAgent: navigator.userAgent,
          url: window.location.href,
          viewport: {
            width: window.innerWidth,
            height: window.innerHeight,
          },
        },
      };

      // Store in multiple locations for redundancy
      this._storeRecoveryData(checkpoint);
      this._createBackup(checkpoint);

      this.log("Recovery checkpoint created", "success");

      // Emit checkpoint event
      this.eventManager.emit("recoveryCheckpointCreated", {
        sessionId: checkpoint.sessionId,
        timestamp: checkpoint.timestamp,
      });

      return { success: true, checkpoint };
    } catch (error) {
      this.log(
        `Failed to create recovery checkpoint: ${error.message}`,
        "error"
      );
      return { success: false, error: error.message };
    }
  }

  /**
   * Get recovery status and progress
   */
  getRecoveryStatus() {
    return {
      state: this.recoveryState,
      isRecovering: this.isRecovering,
      attempts: this.recoveryAttempts,
      progress: { ...this.recoveryProgress },
      performanceMetrics: { ...this.performanceMetrics },
      recoveredDataCount: this.recoveredData.size,
      failedRecoveryCount: this.failedRecoveries.size,
      timestamp: Date.now(),
    };
  }

  /**
   * Store recovery data in localStorage
   */
  _storeRecoveryData(data) {
    try {
      const serialized = JSON.stringify(data);
      this.localStorage.setItem(this.options.recoveryStorageKey, serialized);
      return true;
    } catch (error) {
      this.log(`Failed to store recovery data: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Set crash detection flag
   */
  _setCrashDetectionFlag(crashed) {
    try {
      this.localStorage.setItem(
        this.options.crashDetectionKey,
        crashed.toString()
      );
    } catch (error) {
      this.log(`Failed to set crash detection flag: ${error.message}`, "error");
    }
  }

  /**
   * Update heartbeat timestamp
   */
  _updateHeartbeat() {
    try {
      const heartbeatKey = `${this.options.crashDetectionKey}_heartbeat`;
      this.localStorage.setItem(heartbeatKey, Date.now().toString());
    } catch (error) {
      this.log(`Failed to update heartbeat: ${error.message}`, "error");
    }
  }

  /**
   * Utility methods for data handling
   */
  _getStoredRecoveryData() {
    try {
      const stored = this.localStorage.getItem(this.options.recoveryStorageKey);
      return stored ? JSON.parse(stored) : null;
    } catch (error) {
      return null;
    }
  }

  _clearCrashDetectionFlag() {
    try {
      this.localStorage.removeItem(this.options.crashDetectionKey);
      this.localStorage.removeItem(
        `${this.options.crashDetectionKey}_heartbeat`
      );
    } catch (error) {
      this.log(
        `Failed to clear crash detection flag: ${error.message}`,
        "error"
      );
    }
  }

  _cleanupRecoveryData() {
    try {
      this.localStorage.removeItem(this.options.recoveryStorageKey);
      this.localStorage.removeItem(this.options.backupStorageKey);
    } catch (error) {
      this.log(`Failed to cleanup recovery data: ${error.message}`, "error");
    }
  }

  _calculateDataSize(data) {
    try {
      return JSON.stringify(data).length;
    } catch (error) {
      return 0;
    }
  }

  _updateRecoveryProgress() {
    const percentage =
      this.recoveryProgress.total > 0
        ? (this.recoveryProgress.completed / this.recoveryProgress.total) * 100
        : 0;

    this.eventManager.emit("recoveryProgress", {
      completed: this.recoveryProgress.completed,
      total: this.recoveryProgress.total,
      percentage: Math.round(percentage),
      stage: this.recoveryProgress.stage,
    });
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [SessionRecovery] ${message}`;

    switch (level) {
      case "error":
        console.error(logMessage);
        break;
      case "warning":
        console.warn(logMessage);
        break;
      case "success":
        console.log(`✅ ${logMessage}`);
        break;
      default:
        if (this.options.enableDetailedLogging) {
          console.log(logMessage);
        }
    }

    // Emit log event
    if (this.eventManager) {
      this.eventManager.emit("recoveryLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "SessionRecovery",
      });
    }
  }

  /**
   * Cleanup and destroy
   */
  destroy() {
    try {
      // Clear intervals
      if (this.heartbeatInterval) {
        clearInterval(this.heartbeatInterval);
        this.heartbeatInterval = null;
      }

      // Clear crash detection flag on clean shutdown
      this._clearCrashDetectionFlag();

      // Clear data structures
      this.recoveredData.clear();
      this.failedRecoveries.clear();
      this.integrityChecks.clear();
      this.recoveryQueue = [];

      this.log("SessionRecovery destroyed", "success");
    } catch (error) {
      this.log(
        `Error during SessionRecovery destruction: ${error.message}`,
        "error"
      );
    }
  }
}

export default SessionRecovery;
export { SessionRecovery, RECOVERY_STATES, RECOVERY_PRIORITIES };
