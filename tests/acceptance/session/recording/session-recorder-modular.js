/**
 * Modular Session Recorder - Main Coordination Module
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This is the main session recording orchestrator that coordinates all
 * recording modules including event capture, audio recording, user interactions,
 * performance tracking, and privacy compliance.
 *
 * @fileoverview Main session recording coordination and management
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires EventCapture - For comprehensive event recording
 * @requires UserInteractions - For user interaction tracking
 * @requires AudioCapture - For hunt call session recording
 * @requires PerformanceTracking - For performance monitoring
 * @requires PrivacyCompliance - For privacy protection
 * @requires SessionStorage - For session data management
 */

import { EventCapture } from "./event-capture.js";
import { UserInteractions } from "./user-interactions.js";
import { AudioCapture } from "./audio-capture.js";
import { PerformanceTracking } from "./performance-tracking.js";
import { PrivacyCompliance } from "./privacy-compliance.js";
import { SessionStorage } from "./session-storage.js";
import { DataValidator } from "../validation/data-validator.js";

/**
 * SessionRecorderModular class for comprehensive session recording
 * Orchestrates all recording modules with privacy compliance and performance optimization
 */
class SessionRecorderModular {
  constructor(options = {}) {
    this.config = {
      recordEvents: options.recordEvents !== false,
      recordUserInteractions: options.recordUserInteractions !== false,
      recordAudio: options.recordAudio !== false,
      recordPerformance: options.recordPerformance !== false,
      enablePrivacyCompliance: options.enablePrivacyCompliance !== false,
      autoStart: options.autoStart !== false,
      maxSessionDuration: options.maxSessionDuration || 3600000, // 1 hour
      maxRecordingSize: options.maxRecordingSize || 100 * 1024 * 1024, // 100MB
      compressionEnabled: options.compressionEnabled !== false,
      realTimeProcessing: options.realTimeProcessing !== false,
      bufferSize: options.bufferSize || 1000,
      flushInterval: options.flushInterval || 5000, // 5 seconds
      debugMode: options.debugMode || false,
      userId: options.userId || null,
      sessionId: options.sessionId || this.generateSessionId(),
      ...options,
    };

    this.state = {
      isRecording: false,
      isPaused: false,
      startTime: null,
      lastFlushTime: null,
      totalEvents: 0,
      totalSize: 0,
      errors: [],
      warnings: [],
    };

    this.modules = {
      eventCapture: null,
      userInteractions: null,
      audioCapture: null,
      performanceTracking: null,
      privacyCompliance: null,
      sessionStorage: null,
    };

    this.dataBuffer = [];
    this.validator = new DataValidator();
    this.recordingMetadata = {};

    this.eventHandlers = new Map();

    this.initializeSessionRecorder();
  }

  /**
   * Initialize the session recording system
   * Set up all recording modules and coordination logic
   */
  async initializeSessionRecorder() {
    try {
      if (this.config.enablePrivacyCompliance) {
        await this.initializePrivacyCompliance();
      }

      await this.initializeSessionStorage();

      await this.initializeRecordingModules();

      this.setupModuleCommunication();

      this.setupDataFlushing();

      this.setupSessionLifecycle();

      this.setupErrorHandling();

      if (this.config.autoStart) {
        await this.startRecording();
      }

      console.log("SessionRecorderModular: Initialized successfully");
    } catch (error) {
      console.error("SessionRecorderModular: Initialization failed:", error);
      this.handleError("initialization_failed", error);
    }
  }

  /**
   * Initialize privacy compliance module
   * Set up privacy protection and consent management
   */
  async initializePrivacyCompliance() {
    try {
      this.modules.privacyCompliance = new PrivacyCompliance({
        gdprEnabled: true,
        ccpaEnabled: true,
        anonymizationEnabled: true,
        consentRequired: this.config.userId ? true : false,
        logComplianceEvents: true,
      });

      if (
        this.config.userId &&
        !this.modules.privacyCompliance.hasValidConsent(this.config.userId)
      ) {
        throw new Error("Valid consent required for session recording");
      }

      console.log("SessionRecorderModular: Privacy compliance initialized");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Privacy compliance initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Initialize session storage module
   * Set up session data persistence and management
   */
  async initializeSessionStorage() {
    try {
      this.modules.sessionStorage = new SessionStorage({
        sessionId: this.config.sessionId,
        userId: this.config.userId,
        maxSize: this.config.maxRecordingSize,
        compressionEnabled: this.config.compressionEnabled,
        autoFlush: true,
        flushInterval: this.config.flushInterval,
      });

      this.recordingMetadata = {
        sessionId: this.config.sessionId,
        userId: this.config.userId,
        startTime: null,
        endTime: null,
        userAgent: navigator.userAgent,
        url: window.location.href,
        viewport: {
          width: window.innerWidth,
          height: window.innerHeight,
        },
        modules: Object.keys(this.config).filter(
          (key) => key.startsWith("record") && this.config[key]
        ),
      };

      console.log("SessionRecorderModular: Session storage initialized");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Session storage initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Initialize recording modules based on configuration
   * Set up event, interaction, audio, and performance recording
   */
  async initializeRecordingModules() {
    try {
      if (this.config.recordEvents) {
        this.modules.eventCapture = new EventCapture({
          targetElements: document,
          captureMouseEvents: true,
          captureKeyboardEvents: true,
          captureTouchEvents: true,
          captureFormEvents: true,
          captureNavigationEvents: true,
          bufferSize: this.config.bufferSize,
        });
      }

      if (this.config.recordUserInteractions) {
        this.modules.userInteractions = new UserInteractions({
          trackMouseMovements: true,
          trackScrolling: true,
          trackClicks: true,
          trackFormInteractions: true,
          trackKeyboardShortcuts: true,
          sampling: { mousemove: 100 }, // Sample every 100ms
        });
      }

      if (this.config.recordAudio) {
        this.modules.audioCapture = new AudioCapture({
          recordSystemAudio: true,
          recordMicrophone: false, // Privacy default
          recordHuntCallInteractions: true,
          audioQuality: "medium",
          bufferDuration: 5000, // 5 seconds
        });
      }

      if (this.config.recordPerformance) {
        this.modules.performanceTracking = new PerformanceTracking({
          trackResourceTiming: true,
          trackNavigationTiming: true,
          trackPaintMetrics: true,
          trackLayoutShifts: true,
          trackLongTasks: true,
          trackMemoryUsage: true,
          sampleInterval: 1000, // 1 second
        });
      }

      console.log("SessionRecorderModular: Recording modules initialized");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Recording modules initialization failed:",
        error
      );
      throw error;
    }
  }

  /**
   * Set up communication between recording modules
   * TODO: Establish event-driven communication and data sharing
   */
  setupModuleCommunication() {
    try {
      Object.entries(this.modules).forEach(([moduleName, module]) => {
        if (module && typeof module.on === "function") {
          module.on("data", (data) => {
            this.handleModuleData(moduleName, data);
          });

          module.on("error", (error) => {
            this.handleModuleError(moduleName, error);
          });

          module.on("status", (status) => {
            this.handleModuleStatus(moduleName, status);
          });
        }
      });

      this.setupCrossModuleDataSharing();

      console.log("SessionRecorderModular: Module communication established");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Module communication setup failed:",
        error
      );
    }
  }

  /**
   * Set up cross-module data sharing
   * TODO: Enable modules to share relevant data with each other
   */
  setupCrossModuleDataSharing() {
    if (this.modules.userInteractions && this.modules.eventCapture) {
      this.modules.userInteractions.on("interaction", (interaction) => {
        this.modules.eventCapture.correlateWithInteraction(interaction);
      });
    }

    if (this.modules.performanceTracking) {
      this.modules.performanceTracking.on("performance", (metrics) => {
        Object.values(this.modules).forEach((module) => {
          if (module && typeof module.updatePerformanceContext === "function") {
            module.updatePerformanceContext(metrics);
          }
        });
      });
    }

    if (this.modules.privacyCompliance) {
      const privacyFilter = (data) => {
        return this.modules.privacyCompliance.filterData(data);
      };

      Object.values(this.modules).forEach((module) => {
        if (module && typeof module.setPrivacyFilter === "function") {
          module.setPrivacyFilter(privacyFilter);
        }
      });
    }
  }

  /**
   * Set up automatic data flushing to storage
   * Implement periodic data flushing to prevent memory buildup
   */
  setupDataFlushing() {
    try {
      this.flushTimer = setInterval(() => {
        this.flushDataToStorage();
      }, this.config.flushInterval);

      this.checkBufferSize = () => {
        const bufferSize = this.getBufferSize();
        if (bufferSize > this.config.bufferSize * 0.8) {
          this.flushDataToStorage();
        }
      };

      console.log("SessionRecorderModular: Data flushing configured");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Data flushing setup failed:",
        error
      );
    }
  }

  /**
   * Set up session lifecycle management
   * Handle session start, pause, resume, and end events
   */
  setupSessionLifecycle() {
    try {
      if (this.config.maxSessionDuration) {
        this.sessionTimeoutTimer = setTimeout(() => {
          this.stopRecording("session_timeout");
        }, this.config.maxSessionDuration);
      }

      document.addEventListener("visibilitychange", () => {
        if (document.hidden) {
          this.pauseRecording("tab_hidden");
        } else {
          this.resumeRecording("tab_visible");
        }
      });

      window.addEventListener("beforeunload", () => {
        this.stopRecording("page_unload");
      });

      this.setupStorageQuotaMonitoring();

      console.log("SessionRecorderModular: Session lifecycle configured");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Session lifecycle setup failed:",
        error
      );
    }
  }

  /**
   * Set up error handling for the recording system
   * Implement comprehensive error handling and recovery
   */
  setupErrorHandling() {
    try {
      window.addEventListener("error", (event) => {
        this.handleError("global_error", {
          message: event.message,
          filename: event.filename,
          lineno: event.lineno,
          colno: event.colno,
          error: event.error,
        });
      });

      window.addEventListener("unhandledrejection", (event) => {
        this.handleError("unhandled_promise_rejection", {
          reason: event.reason,
          promise: event.promise,
        });
      });

      console.log("SessionRecorderModular: Error handling configured");
    } catch (error) {
      console.error(
        "SessionRecorderModular: Error handling setup failed:",
        error
      );
    }
  }

  /**
   * Start session recording
   * TODO: Begin recording across all enabled modules
   */
  async startRecording() {
    try {
      if (this.state.isRecording) {
        console.warn("SessionRecorderModular: Recording already in progress");
        return false;
      }

      if (this.config.enablePrivacyCompliance && this.config.userId) {
        if (
          !this.modules.privacyCompliance.hasValidConsent(this.config.userId)
        ) {
          throw new Error("Valid consent required to start recording");
        }
      }

      this.state.isRecording = true;
      this.state.isPaused = false;
      this.state.startTime = Date.now();
      this.state.lastFlushTime = this.state.startTime;

      this.recordingMetadata.startTime = this.state.startTime;

      const startPromises = Object.entries(this.modules).map(
        async ([moduleName, module]) => {
          if (module && typeof module.startRecording === "function") {
            try {
              await module.startRecording();
              console.log(`SessionRecorderModular: Started ${moduleName}`);
            } catch (error) {
              console.error(
                `SessionRecorderModular: Failed to start ${moduleName}:`,
                error
              );
              this.handleModuleError(moduleName, error);
            }
          }
        }
      );

      await Promise.allSettled(startPromises);

      this.logEvent("recording_started", {
        sessionId: this.config.sessionId,
        userId: this.config.userId,
        modules: Object.keys(this.modules).filter((key) => this.modules[key]),
      });

      console.log("SessionRecorderModular: Recording started successfully");
      return true;
    } catch (error) {
      console.error(
        "SessionRecorderModular: Failed to start recording:",
        error
      );
      this.handleError("start_recording_failed", error);
      return false;
    }
  }

  /**
   * Stop session recording
   * TODO: End recording and finalize session data
   */
  async stopRecording(reason = "manual_stop") {
    try {
      if (!this.state.isRecording) {
        console.warn("SessionRecorderModular: No active recording to stop");
        return false;
      }

      this.state.isRecording = false;
      this.state.endTime = Date.now();

      this.recordingMetadata.endTime = this.state.endTime;
      this.recordingMetadata.duration =
        this.state.endTime - this.state.startTime;
      this.recordingMetadata.stopReason = reason;

      const stopPromises = Object.entries(this.modules).map(
        async ([moduleName, module]) => {
          if (module && typeof module.stopRecording === "function") {
            try {
              await module.stopRecording();
              console.log(`SessionRecorderModular: Stopped ${moduleName}`);
            } catch (error) {
              console.error(
                `SessionRecorderModular: Failed to stop ${moduleName}:`,
                error
              );
              this.handleModuleError(moduleName, error);
            }
          }
        }
      );

      await Promise.allSettled(stopPromises);

      await this.flushDataToStorage();

      await this.finalizeSession();

      if (this.flushTimer) {
        clearInterval(this.flushTimer);
        this.flushTimer = null;
      }

      if (this.sessionTimeoutTimer) {
        clearTimeout(this.sessionTimeoutTimer);
        this.sessionTimeoutTimer = null;
      }

      this.logEvent("recording_stopped", {
        sessionId: this.config.sessionId,
        duration: this.recordingMetadata.duration,
        reason,
        totalEvents: this.state.totalEvents,
        totalSize: this.state.totalSize,
      });

      console.log("SessionRecorderModular: Recording stopped successfully");
      return true;
    } catch (error) {
      console.error("SessionRecorderModular: Failed to stop recording:", error);
      this.handleError("stop_recording_failed", error);
      return false;
    }
  }

  /**
   * Pause session recording
   * TODO: Temporarily pause recording while maintaining state
   */
  async pauseRecording(reason = "manual_pause") {
    try {
      if (!this.state.isRecording || this.state.isPaused) {
        return false;
      }

      this.state.isPaused = true;

      Object.entries(this.modules).forEach(([moduleName, module]) => {
        if (module && typeof module.pauseRecording === "function") {
          module.pauseRecording();
        }
      });

      this.logEvent("recording_paused", { reason });

      console.log(`SessionRecorderModular: Recording paused (${reason})`);
      return true;
    } catch (error) {
      console.error(
        "SessionRecorderModular: Failed to pause recording:",
        error
      );
      return false;
    }
  }

  /**
   * Resume session recording
   * TODO: Resume recording from paused state
   */
  async resumeRecording(reason = "manual_resume") {
    try {
      if (!this.state.isRecording || !this.state.isPaused) {
        return false;
      }

      this.state.isPaused = false;

      Object.entries(this.modules).forEach(([moduleName, module]) => {
        if (module && typeof module.resumeRecording === "function") {
          module.resumeRecording();
        }
      });

      this.logEvent("recording_resumed", { reason });

      console.log(`SessionRecorderModular: Recording resumed (${reason})`);
      return true;
    } catch (error) {
      console.error(
        "SessionRecorderModular: Failed to resume recording:",
        error
      );
      return false;
    }
  }

  /**
   * Handle data from recording modules
   * Process and buffer data from individual modules
   */
  handleModuleData(moduleName, data) {
    try {
      if (!this.state.isRecording || this.state.isPaused) {
        return;
      }

      let filteredData = data;
      if (this.modules.privacyCompliance) {
        filteredData = this.modules.privacyCompliance.filterData(data);
      }

      if (!this.validator.validate(filteredData)) {
        console.warn(`SessionRecorderModular: Invalid data from ${moduleName}`);
        return;
      }

      const enrichedData = {
        ...filteredData,
        timestamp: Date.now(),
        sessionId: this.config.sessionId,
        module: moduleName,
        sequence: this.state.totalEvents++,
      };

      this.dataBuffer.push(enrichedData);

      this.state.totalSize += JSON.stringify(enrichedData).length;

      this.checkBufferSize();

      this.emit("data", enrichedData);

      if (this.config.debugMode) {
        console.log(`SessionRecorderModular: Received data from ${moduleName}`);
      }
    } catch (error) {
      console.error(
        `SessionRecorderModular: Failed to handle data from ${moduleName}:`,
        error
      );
      this.handleError("module_data_handling_failed", error);
    }
  }

  /**
   * Flush buffered data to storage
   * TODO: Transfer buffered data to persistent storage
   */
  async flushDataToStorage() {
    try {
      if (this.dataBuffer.length === 0) {
        return;
      }

      const dataToFlush = [...this.dataBuffer];
      this.dataBuffer = [];

      if (this.modules.sessionStorage) {
        await this.modules.sessionStorage.storeData(dataToFlush);
      }

      this.state.lastFlushTime = Date.now();

      this.emit("flush", {
        count: dataToFlush.length,
        size: JSON.stringify(dataToFlush).length,
        timestamp: this.state.lastFlushTime,
      });

      if (this.config.debugMode) {
        console.log(
          `SessionRecorderModular: Flushed ${dataToFlush.length} items to storage`
        );
      }
    } catch (error) {
      console.error(
        "SessionRecorderModular: Failed to flush data to storage:",
        error
      );
      this.handleError("data_flush_failed", error);
    }
  }

  /**
   * Finalize recording session
   * TODO: Complete session processing and generate final artifacts
   */
  async finalizeSession() {
    try {
      const sessionSummary = {
        ...this.recordingMetadata,
        stats: {
          totalEvents: this.state.totalEvents,
          totalSize: this.state.totalSize,
          errors: this.state.errors.length,
          warnings: this.state.warnings.length,
        },
        modules: Object.fromEntries(
          Object.entries(this.modules).map(([name, module]) => [
            name,
            module
              ? module.getStats?.() || { enabled: true }
              : { enabled: false },
          ])
        ),
      };

      if (this.modules.sessionStorage) {
        await this.modules.sessionStorage.storeSessionSummary(sessionSummary);
      }

      this.emit("sessionComplete", sessionSummary);

      console.log("SessionRecorderModular: Session finalized successfully");
      return sessionSummary;
    } catch (error) {
      console.error(
        "SessionRecorderModular: Failed to finalize session:",
        error
      );
      this.handleError("session_finalization_failed", error);
      return null;
    }
  }

  /**
   * Get current recording status
   * Return comprehensive status information
   */
  getStatus() {
    return {
      isRecording: this.state.isRecording,
      isPaused: this.state.isPaused,
      sessionId: this.config.sessionId,
      startTime: this.state.startTime,
      duration: this.state.startTime ? Date.now() - this.state.startTime : 0,
      totalEvents: this.state.totalEvents,
      totalSize: this.state.totalSize,
      bufferSize: this.dataBuffer.length,
      modules: Object.fromEntries(
        Object.entries(this.modules).map(([name, module]) => [
          name,
          module
            ? module.getStatus?.() || { enabled: true }
            : { enabled: false },
        ])
      ),
      errors: this.state.errors.length,
      warnings: this.state.warnings.length,
    };
  }

  /**
   * Generate unique session ID
   * Create cryptographically secure session identifier
   */
  generateSessionId() {
    const timestamp = Date.now().toString(36);
    const randomPart = Math.random().toString(36).substr(2, 9);
    return `session_${timestamp}_${randomPart}`;
  }

  /**
   * Clean up and destroy the session recorder
   * Clean up resources and finalize recording
   */
  async destroy() {
    try {
      if (this.state.isRecording) {
        await this.stopRecording("recorder_destroyed");
      }

      Object.entries(this.modules).forEach(([moduleName, module]) => {
        if (module && typeof module.destroy === "function") {
          module.destroy();
        }
      });

      this.dataBuffer = [];
      this.eventHandlers.clear();

      console.log("SessionRecorderModular: Destroyed successfully");
    } catch (error) {
      console.error("SessionRecorderModular: Destruction failed:", error);
    }
  }
}

export { SessionRecorderModular };

export const createSessionRecorder = (options) =>
  new SessionRecorderModular(options);

export const SessionRecorderUtils = {
  generateSessionId: () => {
    const timestamp = Date.now().toString(36);
    const randomPart = Math.random().toString(36).substr(2, 9);
    return `session_${timestamp}_${randomPart}`;
  },

  validateConfig: (config) => {
    return (
      config &&
      typeof config === "object" &&
      (config.recordEvents ||
        config.recordUserInteractions ||
        config.recordAudio ||
        config.recordPerformance)
    );
  },

  getDefaultConfig: () => ({
    recordEvents: true,
    recordUserInteractions: true,
    recordAudio: false,
    recordPerformance: true,
    enablePrivacyCompliance: true,
    autoStart: false,
    maxSessionDuration: 3600000,
    compressionEnabled: true,
  }),

  formatSessionSummary: (summary) => {
    return {
      id: summary.sessionId,
      duration: summary.duration,
      events: summary.stats?.totalEvents || 0,
      size: summary.stats?.totalSize || 0,
      modules: Object.keys(summary.modules || {}).filter(
        (m) => summary.modules[m].enabled
      ),
    };
  },
};

console.log("SessionRecorderModular module loaded successfully");
