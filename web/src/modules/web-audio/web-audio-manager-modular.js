/**
 * @fileoverview Web Audio Manager - Modular Integration System
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API management system integrating all specialized modules
 * for context management, node routing, effects processing, streams, worklets, and performance.
 *
 * This replaces the original web-audio-manager.js with a fully modular architecture
 * providing enterprise-grade Web Audio capabilities through specialized subsystems.
 *
 * Integrated Modules:
 * ✅ WebAudioContext - AudioContext management with cross-browser compatibility
 * ✅ WebAudioNodes - Advanced node management with dynamic routing and automation
 * ✅ WebAudioEffects - Comprehensive effects processing with 15+ effect types
 * ✅ WebAudioStreams - Audio stream management with device control
 * ✅ WebAudioWorklets - Custom audio processing with worklet integration
 * ✅ WebAudioPerformance - Real-time performance monitoring and optimization
 *
 * Features:
 * ✅ Integrated Web Audio API management across all domains
 * ✅ Cross-module communication and event coordination
 * ✅ Unified configuration and state management
 * ✅ Advanced error handling and recovery
 * ✅ Performance optimization and resource management
 * ✅ Enterprise-grade monitoring and analytics
 * ✅ Modular architecture with clean separation of concerns
 * ✅ Event-driven architecture with comprehensive lifecycle management
 *
 * @example
 * ```javascript
 * import { WebAudioManagerModular } from './modules/web-audio/index.js';
 *
 * const audioManager = new WebAudioManagerModular({
 *   sampleRate: 44100,
 *   latencyHint: 'interactive',
 *   enablePerformanceMonitoring: true
 * });
 *
 * await audioManager.initialize();
 *
 * // Access specialized modules
 * const context = audioManager.getContext();
 * const effects = audioManager.getEffects();
 * const streams = audioManager.getStreams();
 * ```
 */

import WebAudioContext from "./web-audio-context.js";
import WebAudioNodes from "./web-audio-nodes.js";
import WebAudioEffects from "./web-audio-effects.js";
import WebAudioStreams from "./web-audio-streams.js";
import WebAudioWorklets from "./web-audio-worklets.js";
import WebAudioPerformance from "./web-audio-performance.js";

/**
 * Web Audio Manager - Modular Integration System
 *
 * Provides comprehensive Web Audio API management through integrated specialized modules
 * with enterprise-grade features, performance monitoring, and advanced capabilities.
 *
 * @class WebAudioManagerModular
 */
export class WebAudioManagerModular {
  /**
   * Create WebAudioManagerModular instance
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.sampleRate=44100] - Audio sample rate
   * @param {string} [options.latencyHint='interactive'] - Latency hint
   * @param {boolean} [options.enablePerformanceMonitoring=true] - Enable performance monitoring
   * @param {boolean} [options.enableWorklets=true] - Enable audio worklets
   * @param {boolean} [options.enableStreams=true] - Enable stream management
   * @param {boolean} [options.enableEffects=true] - Enable effects processing
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      sampleRate: options.sampleRate || 44100,
      latencyHint: options.latencyHint || "interactive",
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      enableWorklets: options.enableWorklets !== false,
      enableStreams: options.enableStreams !== false,
      enableEffects: options.enableEffects !== false,
      enableNodeManagement: options.enableNodeManagement !== false,
      enableAutoRecovery: options.enableAutoRecovery !== false,
      maxRetries: options.maxRetries || 3,
      retryDelay: options.retryDelay || 1000,
      ...options,
    };

    // Initialization state
    this.isInitialized = false;
    this.isDestroyed = false;
    this.initializationAttempts = 0;

    // Module instances
    this.modules = {
      context: null,
      nodes: null,
      effects: null,
      streams: null,
      worklets: null,
      performance: null,
    };

    // Event handling
    this.eventHandlers = new Map();
    this.moduleEventHandlers = new Map();

    // State management
    this.state = {
      contextState: "created",
      modulesReady: false,
      hasActiveStreams: false,
      hasActiveEffects: false,
      hasActiveWorklets: false,
      performanceMonitoring: false,
    };

    // Error tracking
    this.errors = {
      initialization: [],
      runtime: [],
      recovery: [],
    };

    // Performance metrics aggregation
    this.aggregatedMetrics = {
      lastUpdate: null,
      totalNodes: 0,
      activeStreams: 0,
      activeEffects: 0,
      activeWorklets: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      latency: 0,
    };

    console.log("WebAudioManagerModular instance created");
  }

  /**
   * Initialize the Web Audio Manager and all modules
   *
   * @returns {Promise<boolean>} Initialization success
   */
  async initialize() {
    if (this.isInitialized) {
      console.warn("WebAudioManagerModular is already initialized");
      return true;
    }

    if (this.isDestroyed) {
      throw new Error("Cannot initialize destroyed WebAudioManagerModular");
    }

    this.initializationAttempts++;
    const startTime = performance.now();

    try {
      console.log("Initializing WebAudioManagerModular...");

      // Step 1: Initialize AudioContext module
      await this._initializeContextModule();

      // Step 2: Initialize core modules
      await this._initializeCoreModules();

      // Step 3: Initialize optional modules
      await this._initializeOptionalModules();

      // Step 4: Setup inter-module communication
      this._setupInterModuleCommunication();

      // Step 5: Setup error handling and recovery
      this._setupErrorHandling();

      // Step 6: Start monitoring if enabled
      if (this.config.enablePerformanceMonitoring) {
        this._startPerformanceMonitoring();
      }

      // Mark as initialized
      this.isInitialized = true;
      this.state.modulesReady = true;

      const initTime = performance.now() - startTime;
      console.log(
        `WebAudioManagerModular initialized successfully in ${initTime.toFixed(
          2
        )}ms`
      );

      this._emitEvent("initialized", {
        initializationTime: initTime,
        modules: Object.keys(this.modules).filter(
          (key) => this.modules[key] !== null
        ),
        config: { ...this.config },
      });

      return true;
    } catch (error) {
      console.error("Failed to initialize WebAudioManagerModular:", error);
      this.errors.initialization.push({
        error,
        attempt: this.initializationAttempts,
        timestamp: Date.now(),
      });

      this._emitEvent("initializationError", {
        error,
        attempt: this.initializationAttempts,
      });

      // Attempt recovery if enabled
      if (
        this.config.enableAutoRecovery &&
        this.initializationAttempts < this.config.maxRetries
      ) {
        console.log(
          `Attempting recovery (${this.initializationAttempts}/${this.config.maxRetries})...`
        );
        await this._delay(this.config.retryDelay);
        return this.initialize();
      }

      throw error;
    }
  }

  /**
   * Initialize AudioContext module
   * @private
   */
  async _initializeContextModule() {
    console.log("Initializing WebAudioContext module...");

    this.modules.context = new WebAudioContext({
      sampleRate: this.config.sampleRate,
      latencyHint: this.config.latencyHint,
    });

    await this.modules.context.initialize();

    // Setup context event handlers
    this.modules.context.addEventListener("stateChanged", (data) => {
      this.state.contextState = data.state;
      this._emitEvent("contextStateChanged", data);
    });

    this.modules.context.addEventListener("error", (error) => {
      this._handleModuleError("context", error);
    });

    console.log("WebAudioContext module initialized");
  }

  /**
   * Initialize core modules
   * @private
   */
  async _initializeCoreModules() {
    const audioContext = this.modules.context.getAudioContext();

    // Initialize Nodes module
    if (this.config.enableNodeManagement) {
      console.log("Initializing WebAudioNodes module...");
      this.modules.nodes = new WebAudioNodes(audioContext);
      this._setupModuleEventHandlers("nodes", this.modules.nodes);
      console.log("WebAudioNodes module initialized");
    }
  }

  /**
   * Initialize optional modules
   * @private
   */
  async _initializeOptionalModules() {
    const audioContext = this.modules.context.getAudioContext();

    // Initialize Effects module
    if (this.config.enableEffects) {
      console.log("Initializing WebAudioEffects module...");
      this.modules.effects = new WebAudioEffects(audioContext);
      this._setupModuleEventHandlers("effects", this.modules.effects);
      console.log("WebAudioEffects module initialized");
    }

    // Initialize Streams module
    if (this.config.enableStreams) {
      console.log("Initializing WebAudioStreams module...");
      this.modules.streams = new WebAudioStreams(audioContext);
      this._setupModuleEventHandlers("streams", this.modules.streams);
      console.log("WebAudioStreams module initialized");
    }

    // Initialize Worklets module
    if (this.config.enableWorklets) {
      console.log("Initializing WebAudioWorklets module...");
      this.modules.worklets = new WebAudioWorklets(audioContext);
      this._setupModuleEventHandlers("worklets", this.modules.worklets);
      console.log("WebAudioWorklets module initialized");
    }

    // Initialize Performance module
    if (this.config.enablePerformanceMonitoring) {
      console.log("Initializing WebAudioPerformance module...");
      this.modules.performance = new WebAudioPerformance(audioContext, {
        updateInterval: 1000,
        enableAlerting: true,
      });
      this._setupModuleEventHandlers("performance", this.modules.performance);
      console.log("WebAudioPerformance module initialized");
    }
  }

  /**
   * Setup module event handlers
   * @private
   */
  _setupModuleEventHandlers(moduleName, moduleInstance) {
    const handlers = new Map();

    // Generic error handling
    const errorHandler = (error) => {
      this._handleModuleError(moduleName, error);
    };
    moduleInstance.addEventListener("error", errorHandler);
    handlers.set("error", errorHandler);

    // Module-specific event handlers
    switch (moduleName) {
      case "streams":
        const streamHandler = (data) => {
          this.state.hasActiveStreams = data.streamCount > 0;
          this._updateAggregatedMetrics();
        };
        moduleInstance.addEventListener("streamCreated", streamHandler);
        moduleInstance.addEventListener("streamDestroyed", streamHandler);
        handlers.set("streamEvents", streamHandler);
        break;

      case "effects":
        const effectHandler = (data) => {
          this.state.hasActiveEffects = data.effectCount > 0;
          this._updateAggregatedMetrics();
        };
        moduleInstance.addEventListener("effectCreated", effectHandler);
        moduleInstance.addEventListener("effectDestroyed", effectHandler);
        handlers.set("effectEvents", effectHandler);
        break;

      case "worklets":
        const workletHandler = (data) => {
          this.state.hasActiveWorklets = data.workletCount > 0;
          this._updateAggregatedMetrics();
        };
        moduleInstance.addEventListener("processorCreated", workletHandler);
        moduleInstance.addEventListener("processorDestroyed", workletHandler);
        handlers.set("workletEvents", workletHandler);
        break;

      case "performance":
        const metricsHandler = (metrics) => {
          this._handlePerformanceMetrics(metrics);
        };
        moduleInstance.addEventListener("metricsUpdated", metricsHandler);
        handlers.set("metricsEvents", metricsHandler);
        break;
    }

    this.moduleEventHandlers.set(moduleName, handlers);
  }

  /**
   * Setup inter-module communication
   * @private
   */
  _setupInterModuleCommunication() {
    // Enable cross-module node sharing
    if (this.modules.nodes && this.modules.effects) {
      this.modules.effects.setNodeManager(this.modules.nodes);
    }

    if (this.modules.nodes && this.modules.streams) {
      this.modules.streams.setNodeManager(this.modules.nodes);
    }

    // Enable performance monitoring for all modules
    if (this.modules.performance) {
      Object.entries(this.modules).forEach(([name, module]) => {
        if (name !== "performance" && module) {
          this.modules.performance.addEventListener(
            "performanceAlert",
            (alert) => {
              this._emitEvent("performanceAlert", { module: name, alert });
            }
          );
        }
      });
    }

    console.log("Inter-module communication established");
  }

  /**
   * Setup error handling and recovery
   * @private
   */
  _setupErrorHandling() {
    // Global error recovery strategies
    this.errorRecoveryStrategies = new Map([
      ["context", this._recoverAudioContext.bind(this)],
      ["nodes", this._recoverNodesModule.bind(this)],
      ["effects", this._recoverEffectsModule.bind(this)],
      ["streams", this._recoverStreamsModule.bind(this)],
      ["worklets", this._recoverWorkletsModule.bind(this)],
      ["performance", this._recoverPerformanceModule.bind(this)],
    ]);

    console.log("Error handling and recovery strategies configured");
  }

  /**
   * Start performance monitoring
   * @private
   */
  _startPerformanceMonitoring() {
    if (this.modules.performance) {
      this.modules.performance.startMonitoring();
      this.state.performanceMonitoring = true;

      // Start metrics aggregation
      this.metricsAggregationInterval = setInterval(() => {
        this._updateAggregatedMetrics();
      }, 5000);

      console.log("Performance monitoring started");
    }
  }

  /**
   * Handle module error
   * @private
   */
  _handleModuleError(moduleName, error) {
    console.error(`Error in ${moduleName} module:`, error);

    this.errors.runtime.push({
      module: moduleName,
      error,
      timestamp: Date.now(),
    });

    this._emitEvent("moduleError", { module: moduleName, error });

    // Attempt recovery if strategy exists
    const recoveryStrategy = this.errorRecoveryStrategies.get(moduleName);
    if (recoveryStrategy && this.config.enableAutoRecovery) {
      recoveryStrategy(error).catch((recoveryError) => {
        console.error(`Recovery failed for ${moduleName}:`, recoveryError);
        this.errors.recovery.push({
          module: moduleName,
          originalError: error,
          recoveryError,
          timestamp: Date.now(),
        });
      });
    }
  }

  /**
   * Handle performance metrics
   * @private
   */
  _handlePerformanceMetrics(metrics) {
    // Update aggregated metrics
    this.aggregatedMetrics.lastUpdate = Date.now();
    this.aggregatedMetrics.cpuUsage = metrics.performance?.cpuUsage || 0;
    this.aggregatedMetrics.memoryUsage = metrics.performance?.memoryUsage || 0;
    this.aggregatedMetrics.latency = metrics.performance?.averageLatency || 0;

    this._emitEvent("metricsUpdated", this.aggregatedMetrics);
  }

  /**
   * Update aggregated metrics
   * @private
   */
  _updateAggregatedMetrics() {
    // Collect metrics from all modules
    this.aggregatedMetrics.totalNodes = this.modules.nodes?.getNodeCount() || 0;
    this.aggregatedMetrics.activeStreams =
      this.modules.streams?.getActiveStreamCount() || 0;
    this.aggregatedMetrics.activeEffects =
      this.modules.effects?.getActiveEffectCount() || 0;
    this.aggregatedMetrics.activeWorklets =
      this.modules.worklets?.getActiveProcessorCount() || 0;
    this.aggregatedMetrics.lastUpdate = Date.now();
  }

  // === RECOVERY STRATEGIES ===

  /**
   * Recover AudioContext module
   * @private
   */
  async _recoverAudioContext(error) {
    console.log("Attempting AudioContext recovery...");

    try {
      if (this.modules.context) {
        await this.modules.context.recover();
      } else {
        await this._initializeContextModule();
      }
      console.log("AudioContext recovery successful");
    } catch (recoveryError) {
      console.error("AudioContext recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  /**
   * Recover Nodes module
   * @private
   */
  async _recoverNodesModule(error) {
    console.log("Attempting Nodes module recovery...");

    try {
      if (this.modules.nodes) {
        // Attempt to reinitialize the module
        const audioContext = this.modules.context.getAudioContext();
        this.modules.nodes = new WebAudioNodes(audioContext);
        this._setupModuleEventHandlers("nodes", this.modules.nodes);
      }
      console.log("Nodes module recovery successful");
    } catch (recoveryError) {
      console.error("Nodes module recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  /**
   * Recover Effects module
   * @private
   */
  async _recoverEffectsModule(error) {
    console.log("Attempting Effects module recovery...");

    try {
      if (this.modules.effects) {
        const audioContext = this.modules.context.getAudioContext();
        this.modules.effects = new WebAudioEffects(audioContext);
        this._setupModuleEventHandlers("effects", this.modules.effects);
      }
      console.log("Effects module recovery successful");
    } catch (recoveryError) {
      console.error("Effects module recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  /**
   * Recover Streams module
   * @private
   */
  async _recoverStreamsModule(error) {
    console.log("Attempting Streams module recovery...");

    try {
      if (this.modules.streams) {
        const audioContext = this.modules.context.getAudioContext();
        this.modules.streams = new WebAudioStreams(audioContext);
        this._setupModuleEventHandlers("streams", this.modules.streams);
      }
      console.log("Streams module recovery successful");
    } catch (recoveryError) {
      console.error("Streams module recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  /**
   * Recover Worklets module
   * @private
   */
  async _recoverWorkletsModule(error) {
    console.log("Attempting Worklets module recovery...");

    try {
      if (this.modules.worklets) {
        const audioContext = this.modules.context.getAudioContext();
        this.modules.worklets = new WebAudioWorklets(audioContext);
        this._setupModuleEventHandlers("worklets", this.modules.worklets);
      }
      console.log("Worklets module recovery successful");
    } catch (recoveryError) {
      console.error("Worklets module recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  /**
   * Recover Performance module
   * @private
   */
  async _recoverPerformanceModule(error) {
    console.log("Attempting Performance module recovery...");

    try {
      if (this.modules.performance) {
        const audioContext = this.modules.context.getAudioContext();
        this.modules.performance = new WebAudioPerformance(audioContext);
        this._setupModuleEventHandlers("performance", this.modules.performance);
        this.modules.performance.startMonitoring();
      }
      console.log("Performance module recovery successful");
    } catch (recoveryError) {
      console.error("Performance module recovery failed:", recoveryError);
      throw recoveryError;
    }
  }

  // === PUBLIC API METHODS ===

  /**
   * Get AudioContext module
   *
   * @returns {WebAudioContext|null} Context module
   */
  getContext() {
    return this.modules.context;
  }

  /**
   * Get Nodes module
   *
   * @returns {WebAudioNodes|null} Nodes module
   */
  getNodes() {
    return this.modules.nodes;
  }

  /**
   * Get Effects module
   *
   * @returns {WebAudioEffects|null} Effects module
   */
  getEffects() {
    return this.modules.effects;
  }

  /**
   * Get Streams module
   *
   * @returns {WebAudioStreams|null} Streams module
   */
  getStreams() {
    return this.modules.streams;
  }

  /**
   * Get Worklets module
   *
   * @returns {WebAudioWorklets|null} Worklets module
   */
  getWorklets() {
    return this.modules.worklets;
  }

  /**
   * Get Performance module
   *
   * @returns {WebAudioPerformance|null} Performance module
   */
  getPerformance() {
    return this.modules.performance;
  }

  /**
   * Get underlying AudioContext
   *
   * @returns {AudioContext|null} Audio context
   */
  getAudioContext() {
    return this.modules.context?.getAudioContext() || null;
  }

  /**
   * Get current state
   *
   * @returns {Object} Current state
   */
  getState() {
    return {
      ...this.state,
      isInitialized: this.isInitialized,
      isDestroyed: this.isDestroyed,
      initializationAttempts: this.initializationAttempts,
    };
  }

  /**
   * Get aggregated metrics
   *
   * @returns {Object} Aggregated metrics
   */
  getMetrics() {
    return { ...this.aggregatedMetrics };
  }

  /**
   * Get error history
   *
   * @returns {Object} Error history
   */
  getErrors() {
    return {
      initialization: [...this.errors.initialization],
      runtime: [...this.errors.runtime],
      recovery: [...this.errors.recovery],
    };
  }

  /**
   * Get system health status
   *
   * @returns {Object} Health status
   */
  getHealthStatus() {
    const errors = this.getErrors();
    const recentErrors = [...errors.runtime, ...errors.recovery].filter(
      (e) => Date.now() - e.timestamp < 300000
    ); // Last 5 minutes

    return {
      overall:
        this.isInitialized && !this.isDestroyed && recentErrors.length === 0
          ? "healthy"
          : "degraded",
      modules: Object.entries(this.modules).reduce((status, [name, module]) => {
        status[name] = module ? "active" : "inactive";
        return status;
      }, {}),
      recentErrors: recentErrors.length,
      uptime: this.isInitialized
        ? Date.now() - (this.aggregatedMetrics.lastUpdate || 0)
        : 0,
      performance: this.modules.performance?.getCurrentMetrics() || null,
    };
  }

  /**
   * Suspend audio context
   *
   * @returns {Promise<void>}
   */
  async suspend() {
    if (this.modules.context) {
      await this.modules.context.suspend();
      this._emitEvent("suspended");
    }
  }

  /**
   * Resume audio context
   *
   * @returns {Promise<void>}
   */
  async resume() {
    if (this.modules.context) {
      await this.modules.context.resume();
      this._emitEvent("resumed");
    }
  }

  /**
   * Utility methods
   */
  _delay(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  /**
   * Event handling
   */
  addEventListener(event, handler) {
    if (!this.eventHandlers.has(event)) {
      this.eventHandlers.set(event, []);
    }
    this.eventHandlers.get(event).push(handler);
  }

  removeEventListener(event, handler) {
    if (this.eventHandlers.has(event)) {
      const handlers = this.eventHandlers.get(event);
      const index = handlers.indexOf(handler);
      if (index > -1) {
        handlers.splice(index, 1);
      }
    }
  }

  /**
   * Emit event
   * @private
   */
  _emitEvent(eventName, data) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).forEach((handler) => {
        try {
          handler(data);
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      });
    }
  }

  /**
   * Cleanup and destroy manager
   */
  async destroy() {
    if (this.isDestroyed) {
      console.warn("WebAudioManagerModular is already destroyed");
      return;
    }

    console.log("Destroying WebAudioManagerModular...");

    this.isDestroyed = true;

    // Stop performance monitoring
    if (this.metricsAggregationInterval) {
      clearInterval(this.metricsAggregationInterval);
    }

    // Destroy all modules in reverse order
    const moduleOrder = [
      "performance",
      "worklets",
      "streams",
      "effects",
      "nodes",
      "context",
    ];

    for (const moduleName of moduleOrder) {
      const module = this.modules[moduleName];
      if (module && typeof module.destroy === "function") {
        try {
          await module.destroy();
          console.log(`${moduleName} module destroyed`);
        } catch (error) {
          console.warn(`Failed to destroy ${moduleName} module:`, error);
        }
      }
      this.modules[moduleName] = null;
    }

    // Clean up event handlers
    for (const handlers of this.moduleEventHandlers.values()) {
      handlers.clear();
    }
    this.moduleEventHandlers.clear();
    this.eventHandlers.clear();

    // Clear error recovery strategies
    this.errorRecoveryStrategies.clear();

    // Reset state
    this.isInitialized = false;
    this.state.modulesReady = false;
    this.state.performanceMonitoring = false;

    this._emitEvent("destroyed");
    console.log("WebAudioManagerModular destroyed");
  }
}

export default WebAudioManagerModular;
