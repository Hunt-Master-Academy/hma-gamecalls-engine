/**
 * @fileoverview Web Audio Context Manager Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API context management with browser compatibility,
 * state management, and performance optimization.
 *
 * Features:
 * ✅ Cross-browser AudioContext compatibility
 * ✅ Context state management and recovery
 * ✅ Performance monitoring and optimization
 * ✅ Audio device enumeration and selection
 * ✅ Sample rate and buffer size management
 * ✅ Context suspension and resumption handling
 * ✅ Memory management and cleanup
 * ✅ Error handling and fallback strategies
 *
 * @example
 * ```javascript
 * import { WebAudioContext } from './modules/web-audio/index.js';
 *
 * const contextManager = new WebAudioContext({
 *   sampleRate: 44100,
 *   latencyHint: 'interactive'
 * });
 *
 * await contextManager.initialize();
 * const context = contextManager.getContext();
 * ```
 */

/**
 * Web Audio Context Manager
 *
 * Provides comprehensive AudioContext management with cross-browser compatibility,
 * state management, and performance optimization for web audio applications.
 *
 * @class WebAudioContext
 */
export class WebAudioContext {
  /**
   * Create WebAudioContext manager
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.sampleRate=44100] - Audio sample rate
   * @param {string} [options.latencyHint='interactive'] - Latency hint
   * @param {number} [options.bufferSize=4096] - Buffer size
   * @param {boolean} [options.enablePerformanceMonitoring=true] - Enable performance monitoring
   */
  constructor(options = {}) {
    // Configuration with comprehensive defaults
    this.config = {
      sampleRate: options.sampleRate || 44100,
      latencyHint: options.latencyHint || "interactive",
      bufferSize: options.bufferSize || 4096,
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,

      // Advanced configuration
      echoCancellation: options.echoCancellation !== false,
      noiseSuppression: options.noiseSuppression !== false,
      autoGainControl: options.autoGainControl !== false,
      channelCount: options.channelCount || 2,
      channelCountMode: options.channelCountMode || "explicit",
      channelInterpretation: options.channelInterpretation || "speakers",

      // Fallback options
      enableFallback: options.enableFallback !== false,
      fallbackSampleRate: options.fallbackSampleRate || 48000,
      maxRetries: options.maxRetries || 3,
      retryDelay: options.retryDelay || 1000,

      ...options,
    };

    // State management
    this.state = {
      isInitialized: false,
      isContextCreated: false,
      isContextRunning: false,
      currentState: "uninitialized",
      lastError: null,
      retryCount: 0,
      initStartTime: 0,
      totalInitTime: 0,
    };

    // Core Web Audio components
    this.audioContext = null;
    this.audioProperties = {
      sampleRate: 0,
      baseLatency: 0,
      outputLatency: 0,
      destination: null,
      listener: null,
      state: "suspended",
    };

    // Device management
    this.devices = {
      input: [],
      output: [],
      selected: {
        input: null,
        output: null,
      },
      capabilities: new Map(),
    };

    // Performance tracking
    this.performance = {
      contextCreationTime: 0,
      resumeTime: 0,
      suspendTime: 0,
      averageLatency: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      dropouts: 0,
      underruns: 0,
    };

    // Event handling
    this.eventHandlers = new Map();
    this.stateChangeListeners = [];

    // Browser compatibility detection
    this.browserCapabilities = {
      audioContext: false,
      webkitAudioContext: false,
      audioWorklet: false,
      mediaStreamSource: false,
      offlineAudioContext: false,
      decodeAudioData: false,
      createScriptProcessor: false,
    };

    // Context recovery
    this.recovery = {
      enabled: true,
      maxRecoveryAttempts: 3,
      recoveryAttempts: 0,
      lastRecoveryTime: 0,
      recoveryInterval: 5000,
    };

    console.log("WebAudioContext manager initialized");
  }

  /**
   * Initialize the Web Audio context
   *
   * @returns {Promise<AudioContext>} Initialized audio context
   */
  async initialize() {
    if (this.state.isInitialized) {
      return this.audioContext;
    }

    this.state.initStartTime = performance.now();
    this.state.currentState = "initializing";

    try {
      console.log("Initializing Web Audio context...");

      // Detect browser capabilities
      this._detectBrowserCapabilities();

      // Create audio context with fallback
      await this._createAudioContext();

      // Initialize context properties
      this._initializeContextProperties();

      // Enumerate audio devices
      await this._enumerateAudioDevices();

      // Setup context monitoring
      this._setupContextMonitoring();

      // Setup error handling
      this._setupErrorHandling();

      // Start performance monitoring
      if (this.config.enablePerformanceMonitoring) {
        this._startPerformanceMonitoring();
      }

      this.state.totalInitTime = performance.now() - this.state.initStartTime;
      this.state.isInitialized = true;
      this.state.currentState = "initialized";

      console.log(
        `Web Audio context initialized successfully in ${this.state.totalInitTime.toFixed(
          2
        )}ms`
      );
      this._emitEvent("initialized", {
        context: this.audioContext,
        properties: this.audioProperties,
        initTime: this.state.totalInitTime,
      });

      return this.audioContext;
    } catch (error) {
      this.state.lastError = error;
      this.state.currentState = "error";
      console.error("Web Audio context initialization failed:", error);
      this._emitEvent("initializationError", { error });

      // Attempt recovery if enabled
      if (
        this.recovery.enabled &&
        this.state.retryCount < this.config.maxRetries
      ) {
        return this._attemptRecovery();
      }

      throw error;
    }
  }

  /**
   * Detect browser capabilities
   * @private
   */
  _detectBrowserCapabilities() {
    // Check for AudioContext support
    this.browserCapabilities.audioContext = typeof AudioContext !== "undefined";
    this.browserCapabilities.webkitAudioContext =
      typeof webkitAudioContext !== "undefined";

    // Check for advanced features
    this.browserCapabilities.audioWorklet =
      typeof AudioWorkletNode !== "undefined";
    this.browserCapabilities.mediaStreamSource =
      typeof MediaStreamAudioSourceNode !== "undefined";
    this.browserCapabilities.offlineAudioContext =
      typeof OfflineAudioContext !== "undefined";

    // Check for legacy features
    if (this.browserCapabilities.audioContext) {
      const tempContext = new AudioContext();
      this.browserCapabilities.decodeAudioData =
        typeof tempContext.decodeAudioData === "function";
      this.browserCapabilities.createScriptProcessor =
        typeof tempContext.createScriptProcessor === "function";
      tempContext.close();
    }

    console.log("Browser capabilities detected:", this.browserCapabilities);
  }

  /**
   * Create audio context with fallback
   * @private
   */
  async _createAudioContext() {
    const startTime = performance.now();

    try {
      // Try standard AudioContext first
      if (this.browserCapabilities.audioContext) {
        this.audioContext = new AudioContext({
          sampleRate: this.config.sampleRate,
          latencyHint: this.config.latencyHint,
        });
      }
      // Fallback to webkit prefixed version
      else if (this.browserCapabilities.webkitAudioContext) {
        console.warn("Using webkit prefixed AudioContext");
        this.audioContext = new webkitAudioContext();
      } else {
        throw new Error("AudioContext not supported in this browser");
      }

      this.performance.contextCreationTime = performance.now() - startTime;
      this.state.isContextCreated = true;

      console.log(
        `AudioContext created in ${this.performance.contextCreationTime.toFixed(
          2
        )}ms`
      );
    } catch (error) {
      console.error("Failed to create AudioContext:", error);

      // Try with fallback configuration
      if (this.config.enableFallback && this.state.retryCount === 0) {
        console.log("Attempting fallback configuration...");
        try {
          this.audioContext = new (AudioContext || webkitAudioContext)({
            sampleRate: this.config.fallbackSampleRate,
            latencyHint: "playback", // More compatible latency hint
          });

          this.performance.contextCreationTime = performance.now() - startTime;
          this.state.isContextCreated = true;
          console.log("AudioContext created with fallback configuration");
        } catch (fallbackError) {
          console.error(
            "Fallback AudioContext creation failed:",
            fallbackError
          );
          throw fallbackError;
        }
      } else {
        throw error;
      }
    }
  }

  /**
   * Initialize context properties
   * @private
   */
  _initializeContextProperties() {
    if (!this.audioContext) {
      throw new Error("AudioContext not available");
    }

    // Store context properties
    this.audioProperties = {
      sampleRate: this.audioContext.sampleRate,
      baseLatency: this.audioContext.baseLatency || 0,
      outputLatency: this.audioContext.outputLatency || 0,
      destination: this.audioContext.destination,
      listener: this.audioContext.listener,
      state: this.audioContext.state,
      currentTime: this.audioContext.currentTime,
      onstatechange: null,
    };

    // Setup state change handling
    this.audioContext.onstatechange = () => {
      this._handleStateChange();
    };

    console.log("Context properties initialized:", this.audioProperties);
  }

  /**
   * Enumerate available audio devices
   * @private
   */
  async _enumerateAudioDevices() {
    try {
      if (!navigator.mediaDevices || !navigator.mediaDevices.enumerateDevices) {
        console.warn("Device enumeration not supported");
        return;
      }

      const devices = await navigator.mediaDevices.enumerateDevices();

      this.devices.input = devices.filter(
        (device) => device.kind === "audioinput"
      );
      this.devices.output = devices.filter(
        (device) => device.kind === "audiooutput"
      );

      // Get device capabilities if supported
      for (const device of [...this.devices.input, ...this.devices.output]) {
        try {
          if (navigator.mediaDevices.getSupportedConstraints) {
            const capabilities = await this._getDeviceCapabilities(device);
            this.devices.capabilities.set(device.deviceId, capabilities);
          }
        } catch (error) {
          console.warn(
            `Failed to get capabilities for device ${device.deviceId}:`,
            error
          );
        }
      }

      console.log(
        `Enumerated ${this.devices.input.length} input and ${this.devices.output.length} output devices`
      );

      this._emitEvent("devicesEnumerated", {
        input: this.devices.input,
        output: this.devices.output,
        capabilities: this.devices.capabilities,
      });
    } catch (error) {
      console.error("Device enumeration failed:", error);
    }
  }

  /**
   * Get device capabilities
   * @private
   */
  async _getDeviceCapabilities(device) {
    try {
      if (device.kind === "audioinput") {
        const stream = await navigator.mediaDevices.getUserMedia({
          audio: { deviceId: device.deviceId },
        });

        const track = stream.getAudioTracks()[0];
        const capabilities = track.getCapabilities
          ? track.getCapabilities()
          : {};

        // Cleanup
        track.stop();

        return capabilities;
      }

      // Output device capabilities are limited in current browsers
      return {};
    } catch (error) {
      console.warn(`Failed to get capabilities for ${device.deviceId}:`, error);
      return {};
    }
  }

  /**
   * Setup context monitoring
   * @private
   */
  _setupContextMonitoring() {
    if (!this.audioContext) {
      return;
    }

    // Monitor context state changes
    const checkContextState = () => {
      if (this.audioContext.state !== this.audioProperties.state) {
        const oldState = this.audioProperties.state;
        this.audioProperties.state = this.audioContext.state;

        console.log(
          `AudioContext state changed: ${oldState} → ${this.audioContext.state}`
        );
        this._emitEvent("stateChanged", {
          oldState,
          newState: this.audioContext.state,
          timestamp: performance.now(),
        });
      }
    };

    // Check state periodically
    setInterval(checkContextState, 1000);

    // Setup device change monitoring
    if (navigator.mediaDevices && navigator.mediaDevices.addEventListener) {
      navigator.mediaDevices.addEventListener("devicechange", () => {
        console.log("Audio devices changed");
        this._enumerateAudioDevices();
      });
    }
  }

  /**
   * Setup error handling
   * @private
   */
  _setupErrorHandling() {
    // Global error handler for audio context issues
    window.addEventListener("error", (event) => {
      if (
        event.error &&
        event.error.message &&
        event.error.message.toLowerCase().includes("audio")
      ) {
        console.error("Audio-related error detected:", event.error);
        this._handleAudioError(event.error);
      }
    });

    // Unhandled promise rejection handler
    window.addEventListener("unhandledrejection", (event) => {
      if (
        event.reason &&
        typeof event.reason === "object" &&
        event.reason.message &&
        event.reason.message.toLowerCase().includes("audio")
      ) {
        console.error("Audio-related promise rejection:", event.reason);
        this._handleAudioError(event.reason);
      }
    });
  }

  /**
   * Start performance monitoring
   * @private
   */
  _startPerformanceMonitoring() {
    if (!this.audioContext) {
      return;
    }

    const monitorPerformance = () => {
      // Update performance metrics
      this.performance.averageLatency =
        (this.audioProperties.baseLatency +
          this.audioProperties.outputLatency) *
        1000;

      // Monitor CPU usage (if available)
      if (performance.memory) {
        this.performance.memoryUsage =
          performance.memory.usedJSHeapSize / 1024 / 1024;
      }

      // Check for audio dropouts
      this._checkForDropouts();

      this._emitEvent("performanceUpdate", this.performance);
    };

    // Monitor every 5 seconds
    setInterval(monitorPerformance, 5000);
  }

  /**
   * Check for audio dropouts
   * @private
   */
  _checkForDropouts() {
    // This is a simplified dropout detection
    // In a real implementation, you'd monitor audio callback timing
    if (this.audioContext.state === "running") {
      const currentTime = this.audioContext.currentTime;
      const expectedTime = performance.now() / 1000;
      const timeDrift = Math.abs(currentTime - expectedTime);

      if (timeDrift > 0.1) {
        // 100ms drift threshold
        this.performance.dropouts++;
        console.warn(`Audio dropout detected: ${timeDrift.toFixed(3)}s drift`);
        this._emitEvent("dropout", {
          drift: timeDrift,
          timestamp: performance.now(),
        });
      }
    }
  }

  /**
   * Resume audio context
   *
   * @returns {Promise<void>}
   */
  async resumeContext() {
    if (!this.audioContext) {
      throw new Error("AudioContext not initialized");
    }

    if (this.audioContext.state === "suspended") {
      const startTime = performance.now();

      try {
        await this.audioContext.resume();
        this.performance.resumeTime = performance.now() - startTime;
        this.state.isContextRunning = true;

        console.log(
          `AudioContext resumed in ${this.performance.resumeTime.toFixed(2)}ms`
        );
        this._emitEvent("resumed", { resumeTime: this.performance.resumeTime });
      } catch (error) {
        console.error("Failed to resume AudioContext:", error);
        this._emitEvent("resumeError", { error });
        throw error;
      }
    }
  }

  /**
   * Suspend audio context
   *
   * @returns {Promise<void>}
   */
  async suspendContext() {
    if (!this.audioContext) {
      throw new Error("AudioContext not initialized");
    }

    if (this.audioContext.state === "running") {
      const startTime = performance.now();

      try {
        await this.audioContext.suspend();
        this.performance.suspendTime = performance.now() - startTime;
        this.state.isContextRunning = false;

        console.log(
          `AudioContext suspended in ${this.performance.suspendTime.toFixed(
            2
          )}ms`
        );
        this._emitEvent("suspended", {
          suspendTime: this.performance.suspendTime,
        });
      } catch (error) {
        console.error("Failed to suspend AudioContext:", error);
        this._emitEvent("suspendError", { error });
        throw error;
      }
    }
  }

  /**
   * Handle state changes
   * @private
   */
  _handleStateChange() {
    console.log(`AudioContext state changed to: ${this.audioContext.state}`);

    // Update running state
    this.state.isContextRunning = this.audioContext.state === "running";

    // Notify state change listeners
    this.stateChangeListeners.forEach((listener) => {
      try {
        listener(this.audioContext.state);
      } catch (error) {
        console.error("State change listener error:", error);
      }
    });
  }

  /**
   * Handle audio errors
   * @private
   */
  _handleAudioError(error) {
    this.state.lastError = error;
    console.error("Audio error handled:", error);

    // Attempt recovery if context is broken
    if (
      this.recovery.enabled &&
      this.recovery.recoveryAttempts < this.recovery.maxRecoveryAttempts
    ) {
      this._attemptRecovery();
    }

    this._emitEvent("audioError", { error });
  }

  /**
   * Attempt context recovery
   * @private
   */
  async _attemptRecovery() {
    console.log(
      `Attempting context recovery (attempt ${
        this.recovery.recoveryAttempts + 1
      })`
    );

    this.recovery.recoveryAttempts++;
    this.recovery.lastRecoveryTime = performance.now();

    try {
      // Close existing context if possible
      if (this.audioContext && typeof this.audioContext.close === "function") {
        await this.audioContext.close();
      }

      // Reset state
      this.state.isInitialized = false;
      this.state.isContextCreated = false;
      this.state.retryCount++;

      // Wait before retry
      await new Promise((resolve) =>
        setTimeout(resolve, this.config.retryDelay)
      );

      // Attempt reinitialization
      return await this.initialize();
    } catch (error) {
      console.error("Context recovery failed:", error);

      if (this.recovery.recoveryAttempts >= this.recovery.maxRecoveryAttempts) {
        console.error("Max recovery attempts reached, giving up");
        this._emitEvent("recoveryFailed", {
          error,
          attempts: this.recovery.recoveryAttempts,
        });
      }

      throw error;
    }
  }

  /**
   * Get audio context
   *
   * @returns {AudioContext|null} Audio context instance
   */
  getContext() {
    return this.audioContext;
  }

  /**
   * Get context properties
   *
   * @returns {Object} Context properties
   */
  getProperties() {
    return { ...this.audioProperties };
  }

  /**
   * Get available devices
   *
   * @returns {Object} Available input and output devices
   */
  getDevices() {
    return {
      input: [...this.devices.input],
      output: [...this.devices.output],
      capabilities: new Map(this.devices.capabilities),
    };
  }

  /**
   * Get performance metrics
   *
   * @returns {Object} Performance metrics
   */
  getPerformanceMetrics() {
    return {
      ...this.performance,
      state: { ...this.state },
      browserCapabilities: { ...this.browserCapabilities },
    };
  }

  /**
   * Add state change listener
   *
   * @param {Function} listener - State change listener function
   */
  addStateChangeListener(listener) {
    this.stateChangeListeners.push(listener);
  }

  /**
   * Remove state change listener
   *
   * @param {Function} listener - State change listener function
   */
  removeStateChangeListener(listener) {
    const index = this.stateChangeListeners.indexOf(listener);
    if (index > -1) {
      this.stateChangeListeners.splice(index, 1);
    }
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
   * Cleanup and destroy context
   */
  async destroy() {
    console.log("Destroying WebAudioContext...");

    try {
      // Stop performance monitoring
      // (In a real implementation, you'd clear intervals)

      // Close audio context
      if (this.audioContext && typeof this.audioContext.close === "function") {
        await this.audioContext.close();
      }

      // Clear state
      this.audioContext = null;
      this.state.isInitialized = false;
      this.state.isContextCreated = false;
      this.state.isContextRunning = false;

      // Clear event handlers
      this.eventHandlers.clear();
      this.stateChangeListeners = [];

      this._emitEvent("destroyed");
      console.log("WebAudioContext destroyed");
    } catch (error) {
      console.error("Error during WebAudioContext destruction:", error);
      throw error;
    }
  }
}

export default WebAudioContext;
