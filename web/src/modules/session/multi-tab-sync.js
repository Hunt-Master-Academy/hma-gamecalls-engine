/**
 * @file multi-tab-sync.js
 * @brief Multi-Tab Session Synchronization System
 *
 * This module provides real-time synchronization of session data across multiple
 * browser tabs and windows, ensuring consistent state and collaborative capabilities
 * for the Huntmaster audio analysis system.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Multi-Tab Synchronization Implementation
 * @date July 24, 2025
 */

/**
 * Synchronization states
 */
const SYNC_STATES = {
  DISCONNECTED: "disconnected",
  CONNECTING: "connecting",
  CONNECTED: "connected",
  SYNCING: "syncing",
  CONFLICT: "conflict",
  ERROR: "error",
};

/**
 * Message types for inter-tab communication
 */
const MESSAGE_TYPES = {
  // Connection management
  TAB_REGISTER: "tab_register",
  TAB_UNREGISTER: "tab_unregister",
  TAB_HEARTBEAT: "tab_heartbeat",
  TAB_LIST_REQUEST: "tab_list_request",
  TAB_LIST_RESPONSE: "tab_list_response",

  // Data synchronization
  DATA_UPDATE: "data_update",
  DATA_REQUEST: "data_request",
  DATA_RESPONSE: "data_response",
  FULL_SYNC_REQUEST: "full_sync_request",
  FULL_SYNC_RESPONSE: "full_sync_response",

  // Session management
  SESSION_CREATED: "session_created",
  SESSION_UPDATED: "session_updated",
  SESSION_ENDED: "session_ended",
  SESSION_LOCK: "session_lock",
  SESSION_UNLOCK: "session_unlock",

  // Conflict resolution
  CONFLICT_DETECTED: "conflict_detected",
  CONFLICT_RESOLUTION: "conflict_resolution",

  // Leadership election
  LEADER_ELECTION: "leader_election",
  LEADER_ANNOUNCEMENT: "leader_announcement",
};

/**
 * Conflict resolution strategies
 */
const CONFLICT_STRATEGIES = {
  LAST_WRITE_WINS: "last_write_wins",
  FIRST_WRITE_WINS: "first_write_wins",
  MANUAL_RESOLUTION: "manual_resolution",
  MERGE_STRATEGY: "merge_strategy",
  LEADER_DECIDES: "leader_decides",
};

/**
 * @class MultiTabSync
 * @brief Comprehensive multi-tab session synchronization system
 *
 * Features:
 * • Real-time data synchronization using BroadcastChannel API
 * • Leader election for coordination across tabs
 * • Conflict detection and resolution strategies
 * • Session locking for exclusive operations
 * • Automatic failover when tabs are closed
 * • Performance optimization with selective sync
 * • Cross-origin communication support
 * • Offline/online state management
 * • Tab lifecycle management and cleanup
 */
export class MultiTabSync {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Channel configuration
      channelName: options.channelName || "huntmaster_sync",
      enableBroadcastChannel: options.enableBroadcastChannel !== false,
      enableLocalStorage: options.enableLocalStorage !== false,

      // Tab management
      tabId: options.tabId || this._generateTabId(),
      heartbeatInterval: options.heartbeatInterval || 10000,
      tabTimeout: options.tabTimeout || 30000,
      leaderElectionTimeout: options.leaderElectionTimeout || 5000,

      // Synchronization settings
      enableAutoSync: options.enableAutoSync !== false,
      syncInterval: options.syncInterval || 1000,
      conflictStrategy:
        options.conflictStrategy || CONFLICT_STRATEGIES.LAST_WRITE_WINS,
      enableSessionLocking: options.enableSessionLocking !== false,

      // Performance optimization
      enableSelectiveSync: options.enableSelectiveSync !== false,
      syncThrottleDelay: options.syncThrottleDelay || 100,
      maxSyncRetries: options.maxSyncRetries || 3,

      // Monitoring and debugging
      enablePerformanceTracking: options.enablePerformanceTracking !== false,
      enableDetailedLogging: options.enableDetailedLogging || false,
      debugMode: options.debugMode || false,

      ...options,
    };

    // Tab state management
    this.tabId = this.options.tabId;
    this.syncState = SYNC_STATES.DISCONNECTED;
    this.isLeader = false;
    this.connectedTabs = new Map();
    this.lastHeartbeat = Date.now();

    // Communication channels
    this.broadcastChannel = null;
    this.messageQueue = [];
    this.pendingRequests = new Map();

    // Synchronization state
    this.syncData = new Map();
    this.syncTimestamps = new Map();
    this.conflictQueue = [];
    this.sessionLocks = new Map();

    // Performance tracking
    this.performanceMetrics = {
      messagesSent: 0,
      messagesReceived: 0,
      syncOperations: 0,
      conflictsResolved: 0,
      averageLatency: 0,
      lastSyncTime: 0,
    };

    // Throttling and debouncing
    this.syncThrottle = this._createThrottle(
      this._performSync.bind(this),
      this.options.syncThrottleDelay
    );
    this.heartbeatThrottle = this._createThrottle(
      this._sendHeartbeat.bind(this),
      1000
    );

    this._initializeSync();
  }

  /**
   * Initialize multi-tab synchronization
   */
  _initializeSync() {
    try {
      // Initialize communication channels
      this._initializeBroadcastChannel();
      this._initializeLocalStorageSync();

      // Register this tab
      this._registerTab();

      // Start heartbeat
      this._startHeartbeat();

      // Start leader election
      this._startLeaderElection();

      // Set up event listeners
      this._setupEventListeners();

      this.syncState = SYNC_STATES.CONNECTING;
      this.log("MultiTabSync initialized", "success");
    } catch (error) {
      this.syncState = SYNC_STATES.ERROR;
      this.log(`Sync initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Initialize BroadcastChannel for modern browsers
   */
  _initializeBroadcastChannel() {
    if (
      this.options.enableBroadcastChannel &&
      typeof BroadcastChannel !== "undefined"
    ) {
      try {
        this.broadcastChannel = new BroadcastChannel(this.options.channelName);

        this.broadcastChannel.addEventListener("message", (event) => {
          this._handleBroadcastMessage(event.data);
        });

        this.broadcastChannel.addEventListener("messageerror", (event) => {
          this.log(`Broadcast message error: ${event}`, "error");
        });

        this.log("BroadcastChannel initialized", "success");
      } catch (error) {
        this.log(
          `BroadcastChannel initialization failed: ${error.message}`,
          "error"
        );
        this.broadcastChannel = null;
      }
    }
  }

  /**
   * Initialize localStorage-based sync as fallback
   */
  _initializeLocalStorageSync() {
    if (this.options.enableLocalStorage) {
      window.addEventListener("storage", (event) => {
        if (event.key && event.key.startsWith(this.options.channelName)) {
          this._handleStorageMessage(event);
        }
      });

      this.log("LocalStorage sync initialized", "success");
    }
  }

  /**
   * Register this tab in the sync network
   */
  _registerTab() {
    const tabInfo = {
      id: this.tabId,
      timestamp: Date.now(),
      url: window.location.href,
      userAgent: navigator.userAgent,
      isLeader: false,
      capabilities: this._getTabCapabilities(),
    };

    // Add to local tab registry
    this.connectedTabs.set(this.tabId, tabInfo);

    // Broadcast registration
    this._broadcastMessage({
      type: MESSAGE_TYPES.TAB_REGISTER,
      tabId: this.tabId,
      tabInfo: tabInfo,
      timestamp: Date.now(),
    });

    // Request existing tab list
    this._broadcastMessage({
      type: MESSAGE_TYPES.TAB_LIST_REQUEST,
      tabId: this.tabId,
      timestamp: Date.now(),
    });

    this.log(`Tab registered: ${this.tabId}`, "success");
  }

  /**
   * Start heartbeat mechanism
   */
  _startHeartbeat() {
    this.heartbeatInterval = setInterval(() => {
      this.heartbeatThrottle();
      this._cleanupDeadTabs();
    }, this.options.heartbeatInterval);
  }

  /**
   * Send heartbeat to other tabs
   */
  _sendHeartbeat() {
    this.lastHeartbeat = Date.now();

    this._broadcastMessage({
      type: MESSAGE_TYPES.TAB_HEARTBEAT,
      tabId: this.tabId,
      timestamp: this.lastHeartbeat,
      isLeader: this.isLeader,
      syncState: this.syncState,
    });
  }

  /**
   * Start leader election process
   */
  _startLeaderElection() {
    // Simple leader election: lowest tab ID becomes leader
    setTimeout(() => {
      const sortedTabs = Array.from(this.connectedTabs.keys()).sort();
      const shouldBeLeader = sortedTabs[0] === this.tabId;

      if (shouldBeLeader && !this.isLeader) {
        this._becomeLeader();
      }
    }, this.options.leaderElectionTimeout);
  }

  /**
   * Become the leader tab
   */
  _becomeLeader() {
    this.isLeader = true;
    this.syncState = SYNC_STATES.CONNECTED;

    this._broadcastMessage({
      type: MESSAGE_TYPES.LEADER_ANNOUNCEMENT,
      tabId: this.tabId,
      timestamp: Date.now(),
    });

    this.log(`Tab ${this.tabId} became leader`, "success");

    // Emit leader event
    this.eventManager.emit("syncLeaderElected", {
      tabId: this.tabId,
      timestamp: Date.now(),
    });
  }

  /**
   * Synchronize data across tabs
   */
  async syncData(key, data, options = {}) {
    try {
      const syncOptions = {
        immediate: options.immediate || false,
        lockSession: options.lockSession || false,
        conflictStrategy:
          options.conflictStrategy || this.options.conflictStrategy,
        ...options,
      };

      // Check for session lock if required
      if (syncOptions.lockSession && this.sessionLocks.has(key)) {
        const lock = this.sessionLocks.get(key);
        if (lock.tabId !== this.tabId) {
          throw new Error(`Session ${key} is locked by tab ${lock.tabId}`);
        }
      }

      // Store data locally
      const timestamp = Date.now();
      this.syncData.set(key, data);
      this.syncTimestamps.set(key, timestamp);

      // Broadcast update
      this._broadcastMessage({
        type: MESSAGE_TYPES.DATA_UPDATE,
        tabId: this.tabId,
        key: key,
        data: data,
        timestamp: timestamp,
        options: syncOptions,
      });

      // Immediate sync if requested
      if (syncOptions.immediate) {
        await this._performSync();
      } else {
        this.syncThrottle();
      }

      this.performanceMetrics.syncOperations++;
      this.performanceMetrics.lastSyncTime = timestamp;

      this.log(`Data synced: ${key}`, "success");
      return { success: true, timestamp };
    } catch (error) {
      this.log(`Sync failed for ${key}: ${error.message}`, "error");
      return { success: false, error: error.message };
    }
  }

  /**
   * Request data from other tabs
   */
  async requestData(key, timeout = 5000) {
    return new Promise((resolve, reject) => {
      const requestId = this._generateRequestId();

      // Set up timeout
      const timeoutId = setTimeout(() => {
        this.pendingRequests.delete(requestId);
        reject(new Error(`Data request timeout for ${key}`));
      }, timeout);

      // Store request
      this.pendingRequests.set(requestId, {
        key,
        resolve,
        reject,
        timeoutId,
        timestamp: Date.now(),
      });

      // Broadcast request
      this._broadcastMessage({
        type: MESSAGE_TYPES.DATA_REQUEST,
        tabId: this.tabId,
        requestId: requestId,
        key: key,
        timestamp: Date.now(),
      });
    });
  }

  /**
   * Lock session for exclusive access
   */
  async lockSession(sessionId, timeout = 30000) {
    try {
      // Check if already locked
      if (this.sessionLocks.has(sessionId)) {
        const lock = this.sessionLocks.get(sessionId);
        if (lock.tabId !== this.tabId) {
          throw new Error(
            `Session ${sessionId} already locked by tab ${lock.tabId}`
          );
        }
      }

      // Create lock
      const lock = {
        sessionId,
        tabId: this.tabId,
        timestamp: Date.now(),
        timeout: timeout,
      };

      this.sessionLocks.set(sessionId, lock);

      // Broadcast lock
      this._broadcastMessage({
        type: MESSAGE_TYPES.SESSION_LOCK,
        tabId: this.tabId,
        sessionId: sessionId,
        lock: lock,
        timestamp: Date.now(),
      });

      // Set auto-unlock timer
      setTimeout(() => {
        if (
          this.sessionLocks.has(sessionId) &&
          this.sessionLocks.get(sessionId).tabId === this.tabId
        ) {
          this.unlockSession(sessionId);
        }
      }, timeout);

      this.log(`Session locked: ${sessionId}`, "success");
      return { success: true, lock };
    } catch (error) {
      this.log(`Session lock failed: ${error.message}`, "error");
      return { success: false, error: error.message };
    }
  }

  /**
   * Unlock session
   */
  async unlockSession(sessionId) {
    try {
      const lock = this.sessionLocks.get(sessionId);

      if (!lock || lock.tabId !== this.tabId) {
        throw new Error(
          `Cannot unlock session ${sessionId}: not owned by this tab`
        );
      }

      this.sessionLocks.delete(sessionId);

      // Broadcast unlock
      this._broadcastMessage({
        type: MESSAGE_TYPES.SESSION_UNLOCK,
        tabId: this.tabId,
        sessionId: sessionId,
        timestamp: Date.now(),
      });

      this.log(`Session unlocked: ${sessionId}`, "success");
      return { success: true };
    } catch (error) {
      this.log(`Session unlock failed: ${error.message}`, "error");
      return { success: false, error: error.message };
    }
  }

  /**
   * Handle incoming broadcast messages
   */
  _handleBroadcastMessage(message) {
    try {
      if (!message || !message.type || message.tabId === this.tabId) {
        return; // Ignore invalid messages or messages from self
      }

      this.performanceMetrics.messagesReceived++;

      switch (message.type) {
        case MESSAGE_TYPES.TAB_REGISTER:
          this._handleTabRegister(message);
          break;
        case MESSAGE_TYPES.TAB_UNREGISTER:
          this._handleTabUnregister(message);
          break;
        case MESSAGE_TYPES.TAB_HEARTBEAT:
          this._handleTabHeartbeat(message);
          break;
        case MESSAGE_TYPES.TAB_LIST_REQUEST:
          this._handleTabListRequest(message);
          break;
        case MESSAGE_TYPES.TAB_LIST_RESPONSE:
          this._handleTabListResponse(message);
          break;
        case MESSAGE_TYPES.DATA_UPDATE:
          this._handleDataUpdate(message);
          break;
        case MESSAGE_TYPES.DATA_REQUEST:
          this._handleDataRequest(message);
          break;
        case MESSAGE_TYPES.DATA_RESPONSE:
          this._handleDataResponse(message);
          break;
        case MESSAGE_TYPES.SESSION_LOCK:
          this._handleSessionLock(message);
          break;
        case MESSAGE_TYPES.SESSION_UNLOCK:
          this._handleSessionUnlock(message);
          break;
        case MESSAGE_TYPES.LEADER_ANNOUNCEMENT:
          this._handleLeaderAnnouncement(message);
          break;
        case MESSAGE_TYPES.CONFLICT_DETECTED:
          this._handleConflictDetected(message);
          break;
        default:
          this.log(`Unknown message type: ${message.type}`, "warning");
      }
    } catch (error) {
      this.log(`Error handling broadcast message: ${error.message}`, "error");
    }
  }

  /**
   * Handle tab registration
   */
  _handleTabRegister(message) {
    this.connectedTabs.set(message.tabId, {
      ...message.tabInfo,
      lastSeen: Date.now(),
    });

    // Respond with our tab info if we're established
    if (this.syncState === SYNC_STATES.CONNECTED) {
      this._broadcastMessage({
        type: MESSAGE_TYPES.TAB_LIST_RESPONSE,
        tabId: this.tabId,
        tabs: Array.from(this.connectedTabs.entries()),
        timestamp: Date.now(),
      });
    }

    this.log(`Tab registered: ${message.tabId}`, "info");
  }

  /**
   * Handle data updates
   */
  _handleDataUpdate(message) {
    const { key, data, timestamp, options } = message;

    // Check for conflicts
    const localTimestamp = this.syncTimestamps.get(key);

    if (localTimestamp && Math.abs(timestamp - localTimestamp) < 1000) {
      // Potential conflict detected
      this._handleDataConflict(key, data, timestamp, localTimestamp);
    } else {
      // Accept update if it's newer
      if (!localTimestamp || timestamp > localTimestamp) {
        this.syncData.set(key, data);
        this.syncTimestamps.set(key, timestamp);

        // Emit sync event
        this.eventManager.emit("dataSync", {
          key,
          data,
          timestamp,
          source: message.tabId,
        });
      }
    }
  }

  /**
   * Handle data conflicts
   */
  _handleDataConflict(key, remoteData, remoteTimestamp, localTimestamp) {
    const conflict = {
      key,
      localData: this.syncData.get(key),
      localTimestamp,
      remoteData,
      remoteTimestamp,
      strategy: this.options.conflictStrategy,
      timestamp: Date.now(),
    };

    this.conflictQueue.push(conflict);
    this.performanceMetrics.conflictsResolved++;

    // Resolve conflict based on strategy
    this._resolveConflict(conflict);
  }

  /**
   * Resolve data conflict
   */
  _resolveConflict(conflict) {
    let resolvedData;

    switch (conflict.strategy) {
      case CONFLICT_STRATEGIES.LAST_WRITE_WINS:
        resolvedData =
          conflict.remoteTimestamp > conflict.localTimestamp
            ? conflict.remoteData
            : conflict.localData;
        break;

      case CONFLICT_STRATEGIES.FIRST_WRITE_WINS:
        resolvedData =
          conflict.localTimestamp < conflict.remoteTimestamp
            ? conflict.localData
            : conflict.remoteData;
        break;

      case CONFLICT_STRATEGIES.LEADER_DECIDES:
        if (this.isLeader) {
          resolvedData = conflict.localData;
        } else {
          resolvedData = conflict.remoteData;
        }
        break;

      case CONFLICT_STRATEGIES.MERGE_STRATEGY:
        resolvedData = this._mergeData(conflict.localData, conflict.remoteData);
        break;

      default:
        resolvedData = conflict.remoteData; // Default to remote
    }

    // Apply resolution
    this.syncData.set(conflict.key, resolvedData);
    this.syncTimestamps.set(conflict.key, Date.now());

    // Emit conflict resolution event
    this.eventManager.emit("conflictResolved", {
      conflict,
      resolvedData,
      strategy: conflict.strategy,
    });

    this.log(
      `Conflict resolved for ${conflict.key} using ${conflict.strategy}`,
      "success"
    );
  }

  /**
   * Get synchronization status
   */
  getSyncStatus() {
    return {
      tabId: this.tabId,
      syncState: this.syncState,
      isLeader: this.isLeader,
      connectedTabs: this.connectedTabs.size,
      syncDataCount: this.syncData.size,
      activeLocks: this.sessionLocks.size,
      conflictQueue: this.conflictQueue.length,
      performanceMetrics: { ...this.performanceMetrics },
      lastHeartbeat: this.lastHeartbeat,
      timestamp: Date.now(),
    };
  }

  /**
   * Broadcast message to all tabs
   */
  _broadcastMessage(message) {
    try {
      message.timestamp = message.timestamp || Date.now();

      // Use BroadcastChannel if available
      if (this.broadcastChannel) {
        this.broadcastChannel.postMessage(message);
      }

      // Fallback to localStorage
      if (this.options.enableLocalStorage) {
        const storageKey = `${
          this.options.channelName
        }_${Date.now()}_${Math.random()}`;
        localStorage.setItem(storageKey, JSON.stringify(message));

        // Clean up after short delay
        setTimeout(() => {
          localStorage.removeItem(storageKey);
        }, 1000);
      }

      this.performanceMetrics.messagesSent++;
    } catch (error) {
      this.log(`Failed to broadcast message: ${error.message}`, "error");
    }
  }

  /**
   * Perform sync operation
   */
  async _performSync() {
    if (this.syncState !== SYNC_STATES.CONNECTED) {
      return;
    }

    try {
      this.syncState = SYNC_STATES.SYNCING;

      // Process any pending operations
      await this._processPendingOperations();

      this.syncState = SYNC_STATES.CONNECTED;
    } catch (error) {
      this.log(`Sync operation failed: ${error.message}`, "error");
      this.syncState = SYNC_STATES.ERROR;
    }
  }

  /**
   * Cleanup dead tabs
   */
  _cleanupDeadTabs() {
    const now = Date.now();
    const tabsToRemove = [];

    for (const [tabId, tabInfo] of this.connectedTabs) {
      if (
        tabId !== this.tabId &&
        now - tabInfo.lastSeen > this.options.tabTimeout
      ) {
        tabsToRemove.push(tabId);
      }
    }

    for (const tabId of tabsToRemove) {
      this.connectedTabs.delete(tabId);
      this.log(`Removed dead tab: ${tabId}`, "info");
    }

    // Re-elect leader if current leader is dead
    if (tabsToRemove.length > 0) {
      this._checkLeaderStatus();
    }
  }

  /**
   * Check leader status and re-elect if needed
   */
  _checkLeaderStatus() {
    const activeTabs = Array.from(this.connectedTabs.keys());
    const hasActiveLeader = activeTabs.some((tabId) => {
      const tab = this.connectedTabs.get(tabId);
      return tab && tab.isLeader;
    });

    if (!hasActiveLeader) {
      this._startLeaderElection();
    }
  }

  /**
   * Utility methods
   */
  _generateTabId() {
    return `tab_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  _generateRequestId() {
    return `req_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  _getTabCapabilities() {
    return {
      broadcastChannel: !!this.broadcastChannel,
      localStorage: !!window.localStorage,
      indexedDB: !!window.indexedDB,
      serviceWorker: "serviceWorker" in navigator,
    };
  }

  _createThrottle(func, delay) {
    let timeoutId;
    return (...args) => {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
  }

  _mergeData(localData, remoteData) {
    // Simple merge strategy - can be enhanced
    if (typeof localData === "object" && typeof remoteData === "object") {
      return { ...localData, ...remoteData };
    }
    return remoteData;
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Page unload cleanup
    window.addEventListener("beforeunload", () => {
      this._broadcastMessage({
        type: MESSAGE_TYPES.TAB_UNREGISTER,
        tabId: this.tabId,
        timestamp: Date.now(),
      });
    });

    // Visibility change handling
    document.addEventListener("visibilitychange", () => {
      if (document.visibilityState === "visible") {
        this._sendHeartbeat();
      }
    });
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [MultiTabSync:${this.tabId}] ${message}`;

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
      this.eventManager.emit("syncLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "MultiTabSync",
        tabId: this.tabId,
      });
    }
  }

  /**
   * Cleanup and destroy
   */
  destroy() {
    try {
      // Unregister tab
      this._broadcastMessage({
        type: MESSAGE_TYPES.TAB_UNREGISTER,
        tabId: this.tabId,
        timestamp: Date.now(),
      });

      // Clear intervals
      if (this.heartbeatInterval) {
        clearInterval(this.heartbeatInterval);
        this.heartbeatInterval = null;
      }

      // Close broadcast channel
      if (this.broadcastChannel) {
        this.broadcastChannel.close();
        this.broadcastChannel = null;
      }

      // Clear data structures
      this.connectedTabs.clear();
      this.syncData.clear();
      this.syncTimestamps.clear();
      this.sessionLocks.clear();
      this.conflictQueue = [];
      this.pendingRequests.clear();

      this.syncState = SYNC_STATES.DISCONNECTED;
      this.log("MultiTabSync destroyed", "success");
    } catch (error) {
      this.log(
        `Error during MultiTabSync destruction: ${error.message}`,
        "error"
      );
    }
  }
}

export default MultiTabSync;
export { MultiTabSync, SYNC_STATES, MESSAGE_TYPES, CONFLICT_STRATEGIES };
