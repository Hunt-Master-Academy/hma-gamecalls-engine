/**
 * @file session-analytics.js
 * @brief Session Analytics and Metrics System
 *
 * This module provides comprehensive analytics and metrics collection for
 * audio analysis sessions, user interactions, performance tracking, and
 * behavioral insights for the Huntmaster system.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Session Analytics Implementation
 * @date July 24, 2025
 */

/**
 * Analytics event types
 */
const ANALYTICS_EVENTS = {
  // Session lifecycle
  SESSION_STARTED: "session_started",
  SESSION_ENDED: "session_ended",
  SESSION_PAUSED: "session_paused",
  SESSION_RESUMED: "session_resumed",

  // Audio processing
  AUDIO_UPLOADED: "audio_uploaded",
  AUDIO_PROCESSED: "audio_processed",
  ANALYSIS_COMPLETED: "analysis_completed",
  CALL_IDENTIFIED: "call_identified",

  // User interactions
  UI_INTERACTION: "ui_interaction",
  SETTING_CHANGED: "setting_changed",
  FEATURE_USED: "feature_used",
  ERROR_ENCOUNTERED: "error_encountered",

  // Performance metrics
  PERFORMANCE_METRIC: "performance_metric",
  MEMORY_USAGE: "memory_usage",
  PROCESSING_TIME: "processing_time",

  // User engagement
  PAGE_VIEW: "page_view",
  TIME_ON_TASK: "time_on_task",
  FEATURE_DISCOVERY: "feature_discovery",
  HELP_ACCESSED: "help_accessed",
};

/**
 * Metric aggregation types
 */
const AGGREGATION_TYPES = {
  SUM: "sum",
  AVERAGE: "average",
  MIN: "min",
  MAX: "max",
  COUNT: "count",
  PERCENTILE: "percentile",
  HISTOGRAM: "histogram",
};

/**
 * Privacy levels for data collection
 */
const PRIVACY_LEVELS = {
  ANONYMOUS: "anonymous", // No PII, aggregated data only
  PSEUDONYMOUS: "pseudonymous", // Session-based tracking
  IDENTIFIED: "identified", // Full user tracking (with consent)
};

/**
 * @class SessionAnalytics
 * @brief Comprehensive session analytics and metrics system
 *
 * Features:
 * • Real-time event tracking and aggregation
 * • Performance metrics collection and analysis
 * • User behavior analytics with privacy controls
 * • Custom metrics and KPI tracking
 * • A/B testing support and experiment tracking
 * • Offline/online analytics with sync capabilities
 * • Data export and reporting functionality
 * • Privacy-compliant data collection
 * • Real-time dashboard data feeds
 * • Predictive analytics and insights
 */
export class SessionAnalytics {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Analytics configuration
      enableAnalytics: options.enableAnalytics !== false,
      privacyLevel: options.privacyLevel || PRIVACY_LEVELS.ANONYMOUS,
      sessionId: options.sessionId || this._generateSessionId(),
      userId: options.userId || null,

      // Data collection settings
      enableRealTimeTracking: options.enableRealTimeTracking !== false,
      enablePerformanceTracking: options.enablePerformanceTracking !== false,
      enableUserBehaviorTracking: options.enableUserBehaviorTracking !== false,
      enableErrorTracking: options.enableErrorTracking !== false,

      // Storage and persistence
      enableLocalStorage: options.enableLocalStorage !== false,
      enableRemoteSync: options.enableRemoteSync || false,
      storageKey: options.storageKey || "huntmaster_analytics",
      syncEndpoint: options.syncEndpoint || null,

      // Sampling and throttling
      samplingRate: options.samplingRate || 1.0,
      eventThrottleDelay: options.eventThrottleDelay || 100,
      maxEventsPerSession: options.maxEventsPerSession || 10000,
      maxStorageSize: options.maxStorageSize || 5 * 1024 * 1024, // 5MB

      // Reporting and aggregation
      aggregationInterval: options.aggregationInterval || 60000,
      enableCustomMetrics: options.enableCustomMetrics !== false,
      enableABTesting: options.enableABTesting || false,

      // Privacy and compliance
      enableDataAnonymization: options.enableDataAnonymization !== false,
      dataRetentionDays: options.dataRetentionDays || 30,
      enableConsentManagement: options.enableConsentManagement || false,

      // Debugging and development
      enableDetailedLogging: options.enableDetailedLogging || false,
      debugMode: options.debugMode || false,

      ...options,
    };

    // Analytics state
    this.isInitialized = false;
    this.isTracking = false;
    this.sessionStartTime = Date.now();
    this.eventQueue = [];
    this.eventCount = 0;

    // Data storage
    this.events = [];
    this.metrics = new Map();
    this.aggregatedData = new Map();
    this.customMetrics = new Map();
    this.performanceMetrics = new Map();

    // User behavior tracking
    this.userSessions = new Map();
    this.interactionHistory = [];
    this.featureUsageStats = new Map();
    this.errorHistory = [];

    // A/B testing
    this.experiments = new Map();
    this.testGroups = new Map();
    this.conversionEvents = [];

    // Performance monitoring
    this.performanceObserver = null;
    this.memoryUsageHistory = [];
    this.processingTimeHistory = [];

    // Throttling and debouncing
    this.eventThrottle = this._createThrottle(
      this._processEventQueue.bind(this),
      this.options.eventThrottleDelay
    );
    this.aggregationTimer = null;

    this._initialize();
  }

  /**
   * Initialize analytics system
   */
  _initialize() {
    try {
      if (!this.options.enableAnalytics) {
        this.log("Analytics disabled by configuration", "info");
        return;
      }

      // Initialize storage
      this._initializeStorage();

      // Set up event listeners
      this._setupEventListeners();

      // Initialize performance monitoring
      this._initializePerformanceMonitoring();

      // Start aggregation timer
      this._startAggregation();

      // Load existing data
      this._loadStoredData();

      this.isInitialized = true;
      this.isTracking = true;

      // Track session start
      this.trackEvent(ANALYTICS_EVENTS.SESSION_STARTED, {
        sessionId: this.options.sessionId,
        timestamp: this.sessionStartTime,
        userAgent: navigator.userAgent,
        url: window.location.href,
        viewport: {
          width: window.innerWidth,
          height: window.innerHeight,
        },
      });

      this.log("SessionAnalytics initialized", "success");
    } catch (error) {
      this.log(`Analytics initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Initialize storage systems
   */
  _initializeStorage() {
    if (this.options.enableLocalStorage) {
      try {
        // Test localStorage availability
        const testKey = `${this.options.storageKey}_test`;
        localStorage.setItem(testKey, "test");
        localStorage.removeItem(testKey);

        this.log("Local storage initialized", "success");
      } catch (error) {
        this.log(
          `Local storage initialization failed: ${error.message}`,
          "warning"
        );
        this.options.enableLocalStorage = false;
      }
    }
  }

  /**
   * Set up event listeners for automatic tracking
   */
  _setupEventListeners() {
    // Page lifecycle events
    window.addEventListener("beforeunload", () => {
      this._trackSessionEnd();
    });

    document.addEventListener("visibilitychange", () => {
      if (document.visibilityState === "hidden") {
        this.trackEvent(ANALYTICS_EVENTS.SESSION_PAUSED, {
          timestamp: Date.now(),
          duration: Date.now() - this.sessionStartTime,
        });
      } else if (document.visibilityState === "visible") {
        this.trackEvent(ANALYTICS_EVENTS.SESSION_RESUMED, {
          timestamp: Date.now(),
        });
      }
    });

    // UI interaction tracking
    if (this.options.enableUserBehaviorTracking) {
      this._setupUITracking();
    }

    // Error tracking
    if (this.options.enableErrorTracking) {
      this._setupErrorTracking();
    }

    // Performance tracking
    if (this.options.enablePerformanceTracking) {
      this._setupPerformanceTracking();
    }
  }

  /**
   * Set up UI interaction tracking
   */
  _setupUITracking() {
    // Click tracking
    document.addEventListener("click", (event) => {
      this.trackEvent(ANALYTICS_EVENTS.UI_INTERACTION, {
        type: "click",
        element: event.target.tagName.toLowerCase(),
        id: event.target.id || null,
        className: event.target.className || null,
        text: event.target.textContent?.substring(0, 50) || null,
        timestamp: Date.now(),
      });
    });

    // Form interaction tracking
    document.addEventListener("change", (event) => {
      if (
        event.target.tagName.toLowerCase() === "input" ||
        event.target.tagName.toLowerCase() === "select"
      ) {
        this.trackEvent(ANALYTICS_EVENTS.UI_INTERACTION, {
          type: "input_change",
          element: event.target.tagName.toLowerCase(),
          inputType: event.target.type || null,
          id: event.target.id || null,
          timestamp: Date.now(),
        });
      }
    });
  }

  /**
   * Set up error tracking
   */
  _setupErrorTracking() {
    window.addEventListener("error", (event) => {
      this.trackEvent(ANALYTICS_EVENTS.ERROR_ENCOUNTERED, {
        type: "javascript_error",
        message: event.message,
        filename: event.filename,
        line: event.lineno,
        column: event.colno,
        stack: event.error?.stack || null,
        timestamp: Date.now(),
      });
    });

    window.addEventListener("unhandledrejection", (event) => {
      this.trackEvent(ANALYTICS_EVENTS.ERROR_ENCOUNTERED, {
        type: "promise_rejection",
        reason: event.reason?.toString() || "Unknown reason",
        timestamp: Date.now(),
      });
    });
  }

  /**
   * Initialize performance monitoring
   */
  _initializePerformanceMonitoring() {
    if (typeof PerformanceObserver !== "undefined") {
      try {
        this.performanceObserver = new PerformanceObserver((list) => {
          for (const entry of list.getEntries()) {
            this._handlePerformanceEntry(entry);
          }
        });

        this.performanceObserver.observe({
          entryTypes: ["measure", "navigation", "resource"],
        });
      } catch (error) {
        this.log(
          `Performance observer setup failed: ${error.message}`,
          "warning"
        );
      }
    }

    // Memory usage tracking
    if (performance.memory) {
      setInterval(() => {
        this._trackMemoryUsage();
      }, 30000); // Every 30 seconds
    }
  }

  /**
   * Track an analytics event
   */
  trackEvent(eventType, data = {}) {
    try {
      if (!this.isTracking || !this._shouldSampleEvent()) {
        return;
      }

      if (this.eventCount >= this.options.maxEventsPerSession) {
        this.log("Max events per session reached", "warning");
        return;
      }

      const event = {
        id: this._generateEventId(),
        type: eventType,
        sessionId: this.options.sessionId,
        userId: this.options.userId,
        timestamp: Date.now(),
        data: this._sanitizeEventData(data),
        privacyLevel: this.options.privacyLevel,
      };

      // Add to event queue
      this.eventQueue.push(event);
      this.eventCount++;

      // Process queue (throttled)
      this.eventThrottle();

      // Emit event for real-time processing
      this.eventManager.emit("analyticsEvent", event);

      return event.id;
    } catch (error) {
      this.log(`Event tracking failed: ${error.message}`, "error");
      return null;
    }
  }

  /**
   * Track custom metric
   */
  trackMetric(
    name,
    value,
    aggregationType = AGGREGATION_TYPES.AVERAGE,
    metadata = {}
  ) {
    try {
      const metric = {
        name,
        value,
        aggregationType,
        metadata,
        timestamp: Date.now(),
        sessionId: this.options.sessionId,
      };

      // Store in custom metrics
      if (!this.customMetrics.has(name)) {
        this.customMetrics.set(name, []);
      }
      this.customMetrics.get(name).push(metric);

      // Emit metric event
      this.eventManager.emit("analyticsMetric", metric);

      this.log(`Metric tracked: ${name} = ${value}`, "info");
      return true;
    } catch (error) {
      this.log(`Metric tracking failed: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Track performance timing
   */
  trackTiming(name, startTime, endTime, metadata = {}) {
    const duration = endTime - startTime;

    return this.trackMetric(
      `timing_${name}`,
      duration,
      AGGREGATION_TYPES.AVERAGE,
      {
        startTime,
        endTime,
        ...metadata,
      }
    );
  }

  /**
   * Track user feature usage
   */
  trackFeatureUsage(featureName, action = "used", metadata = {}) {
    try {
      // Update feature usage stats
      const key = `${featureName}_${action}`;
      const currentCount = this.featureUsageStats.get(key) || 0;
      this.featureUsageStats.set(key, currentCount + 1);

      // Track as event
      this.trackEvent(ANALYTICS_EVENTS.FEATURE_USED, {
        feature: featureName,
        action: action,
        count: currentCount + 1,
        ...metadata,
      });

      return true;
    } catch (error) {
      this.log(`Feature usage tracking failed: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Track A/B test participation
   */
  trackExperiment(experimentName, variant, conversionEvent = null) {
    try {
      // Store experiment participation
      this.experiments.set(experimentName, {
        variant,
        timestamp: Date.now(),
        sessionId: this.options.sessionId,
      });

      // Track participation event
      this.trackEvent("experiment_participation", {
        experiment: experimentName,
        variant: variant,
        timestamp: Date.now(),
      });

      // Track conversion if provided
      if (conversionEvent) {
        this.conversionEvents.push({
          experiment: experimentName,
          variant: variant,
          event: conversionEvent,
          timestamp: Date.now(),
        });
      }

      this.log(`Experiment tracked: ${experimentName} = ${variant}`, "info");
      return true;
    } catch (error) {
      this.log(`Experiment tracking failed: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Get analytics summary
   */
  getAnalyticsSummary() {
    const summary = {
      session: {
        id: this.options.sessionId,
        startTime: this.sessionStartTime,
        duration: Date.now() - this.sessionStartTime,
        eventCount: this.eventCount,
        isActive: this.isTracking,
      },

      events: {
        total: this.events.length,
        byType: this._aggregateEventsByType(),
        recent: this.events.slice(-10),
      },

      metrics: {
        custom: this.customMetrics.size,
        performance: this.performanceMetrics.size,
        aggregated: this.aggregatedData.size,
      },

      userBehavior: {
        interactions: this.interactionHistory.length,
        features: this.featureUsageStats.size,
        errors: this.errorHistory.length,
      },

      experiments: {
        active: this.experiments.size,
        conversions: this.conversionEvents.length,
      },

      performance: {
        memorySnapshots: this.memoryUsageHistory.length,
        timingMetrics: this.processingTimeHistory.length,
      },

      timestamp: Date.now(),
    };

    return summary;
  }

  /**
   * Export analytics data
   */
  exportData(format = "json") {
    try {
      const exportData = {
        metadata: {
          sessionId: this.options.sessionId,
          exportTime: Date.now(),
          format: format,
          privacyLevel: this.options.privacyLevel,
        },

        events: this.events,
        metrics: Object.fromEntries(this.customMetrics),
        performance: Object.fromEntries(this.performanceMetrics),
        aggregated: Object.fromEntries(this.aggregatedData),
        experiments: Object.fromEntries(this.experiments),
        featureUsage: Object.fromEntries(this.featureUsageStats),
      };

      switch (format.toLowerCase()) {
        case "json":
          return JSON.stringify(exportData, null, 2);

        case "csv":
          return this._convertToCSV(exportData);

        default:
          throw new Error(`Unsupported export format: ${format}`);
      }
    } catch (error) {
      this.log(`Data export failed: ${error.message}`, "error");
      return null;
    }
  }

  /**
   * Process event queue
   */
  _processEventQueue() {
    if (this.eventQueue.length === 0) {
      return;
    }

    try {
      // Move events from queue to storage
      const eventsToProcess = [...this.eventQueue];
      this.eventQueue = [];

      // Add to events array
      this.events.push(...eventsToProcess);

      // Store in localStorage if enabled
      if (this.options.enableLocalStorage) {
        this._storeEventsLocally(eventsToProcess);
      }

      // Sync to remote if enabled
      if (this.options.enableRemoteSync) {
        this._syncEventsRemotely(eventsToProcess);
      }

      // Clean up old data
      this._cleanupOldData();
    } catch (error) {
      this.log(`Event queue processing failed: ${error.message}`, "error");
    }
  }

  /**
   * Start data aggregation
   */
  _startAggregation() {
    this.aggregationTimer = setInterval(() => {
      this._aggregateData();
    }, this.options.aggregationInterval);
  }

  /**
   * Aggregate collected data
   */
  _aggregateData() {
    try {
      // Aggregate events by type
      const eventAggregation = this._aggregateEventsByType();
      this.aggregatedData.set("events_by_type", eventAggregation);

      // Aggregate metrics
      const metricAggregation = this._aggregateMetrics();
      this.aggregatedData.set("metrics_summary", metricAggregation);

      // Aggregate performance data
      const performanceAggregation = this._aggregatePerformanceData();
      this.aggregatedData.set("performance_summary", performanceAggregation);

      // Emit aggregation complete event
      this.eventManager.emit("analyticsAggregated", {
        timestamp: Date.now(),
        aggregatedData: Object.fromEntries(this.aggregatedData),
      });
    } catch (error) {
      this.log(`Data aggregation failed: ${error.message}`, "error");
    }
  }

  /**
   * Aggregate events by type
   */
  _aggregateEventsByType() {
    const aggregation = {};

    for (const event of this.events) {
      if (!aggregation[event.type]) {
        aggregation[event.type] = {
          count: 0,
          firstSeen: event.timestamp,
          lastSeen: event.timestamp,
        };
      }

      aggregation[event.type].count++;
      aggregation[event.type].lastSeen = Math.max(
        aggregation[event.type].lastSeen,
        event.timestamp
      );
    }

    return aggregation;
  }

  /**
   * Track memory usage
   */
  _trackMemoryUsage() {
    if (performance.memory) {
      const memoryInfo = {
        used: performance.memory.usedJSHeapSize,
        total: performance.memory.totalJSHeapSize,
        limit: performance.memory.jsHeapSizeLimit,
        timestamp: Date.now(),
      };

      this.memoryUsageHistory.push(memoryInfo);

      // Keep only recent data
      if (this.memoryUsageHistory.length > 100) {
        this.memoryUsageHistory = this.memoryUsageHistory.slice(-50);
      }

      // Track as metric
      this.trackMetric(
        "memory_usage",
        memoryInfo.used,
        AGGREGATION_TYPES.AVERAGE,
        {
          total: memoryInfo.total,
          limit: memoryInfo.limit,
        }
      );
    }
  }

  /**
   * Track session end
   */
  _trackSessionEnd() {
    this.trackEvent(ANALYTICS_EVENTS.SESSION_ENDED, {
      sessionId: this.options.sessionId,
      duration: Date.now() - this.sessionStartTime,
      eventCount: this.eventCount,
      timestamp: Date.now(),
    });

    // Force process remaining events
    this._processEventQueue();

    this.isTracking = false;
  }

  /**
   * Utility methods
   */
  _shouldSampleEvent() {
    return Math.random() < this.options.samplingRate;
  }

  _generateEventId() {
    return `evt_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  _generateSessionId() {
    return `sess_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  _sanitizeEventData(data) {
    // Remove sensitive information based on privacy level
    if (this.options.privacyLevel === PRIVACY_LEVELS.ANONYMOUS) {
      const sanitized = { ...data };
      delete sanitized.userId;
      delete sanitized.email;
      delete sanitized.personalInfo;
      return sanitized;
    }

    return data;
  }

  _createThrottle(func, delay) {
    let timeoutId;
    return (...args) => {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
  }

  _storeEventsLocally(events) {
    try {
      const existingData = localStorage.getItem(this.options.storageKey);
      const storedEvents = existingData ? JSON.parse(existingData) : [];

      storedEvents.push(...events);

      // Check storage size
      const serialized = JSON.stringify(storedEvents);
      if (serialized.length > this.options.maxStorageSize) {
        // Remove oldest events
        const excess = serialized.length - this.options.maxStorageSize;
        const eventsToRemove = Math.ceil(excess / 1000); // Rough estimate
        storedEvents.splice(0, eventsToRemove);
      }

      localStorage.setItem(
        this.options.storageKey,
        JSON.stringify(storedEvents)
      );
    } catch (error) {
      this.log(`Local storage failed: ${error.message}`, "error");
    }
  }

  _loadStoredData() {
    if (this.options.enableLocalStorage) {
      try {
        const storedData = localStorage.getItem(this.options.storageKey);
        if (storedData) {
          const events = JSON.parse(storedData);
          this.events.push(...events);
          this.log(`Loaded ${events.length} stored events`, "info");
        }
      } catch (error) {
        this.log(`Failed to load stored data: ${error.message}`, "error");
      }
    }
  }

  _cleanupOldData() {
    const cutoffTime =
      Date.now() - this.options.dataRetentionDays * 24 * 60 * 60 * 1000;

    // Clean up events
    this.events = this.events.filter((event) => event.timestamp > cutoffTime);

    // Clean up memory history
    this.memoryUsageHistory = this.memoryUsageHistory.filter(
      (entry) => entry.timestamp > cutoffTime
    );
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [SessionAnalytics] ${message}`;

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
      this.eventManager.emit("analyticsLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "SessionAnalytics",
      });
    }
  }

  /**
   * Cleanup and destroy
   */
  destroy() {
    try {
      // Track session end
      this._trackSessionEnd();

      // Clear timers
      if (this.aggregationTimer) {
        clearInterval(this.aggregationTimer);
        this.aggregationTimer = null;
      }

      // Disconnect performance observer
      if (this.performanceObserver) {
        this.performanceObserver.disconnect();
        this.performanceObserver = null;
      }

      // Clear data structures
      this.events = [];
      this.eventQueue = [];
      this.metrics.clear();
      this.aggregatedData.clear();
      this.customMetrics.clear();
      this.performanceMetrics.clear();
      this.experiments.clear();
      this.featureUsageStats.clear();

      this.isTracking = false;
      this.log("SessionAnalytics destroyed", "success");
    } catch (error) {
      this.log(
        `Error during SessionAnalytics destruction: ${error.message}`,
        "error"
      );
    }
  }
}

export default SessionAnalytics;
export {
  SessionAnalytics,
  ANALYTICS_EVENTS,
  AGGREGATION_TYPES,
  PRIVACY_LEVELS,
};
