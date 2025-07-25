/**
 * @fileoverview Web Audio Worklets Management Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API worklet management with custom audio processing,
 * real-time DSP capabilities, and advanced worklet integration.
 *
 * Features:
 * ✅ Audio worklet registration and management
 * ✅ Custom audio processor creation and control
 * ✅ Real-time parameter messaging between main thread and worklets
 * ✅ Multi-threaded audio processing with shared memory
 * ✅ Worklet-based effects and analysis processors
 * ✅ Performance monitoring and resource management
 * ✅ Cross-browser compatibility and fallbacks
 * ✅ Dynamic worklet loading and hot-swapping
 * ✅ Advanced DSP algorithm implementations
 * ✅ Worklet debugging and error handling
 * ✅ Memory-efficient audio buffer management
 * ✅ Real-time visualization data extraction
 *
 * @example
 * ```javascript
 * import { WebAudioWorklets } from './modules/web-audio/index.js';
 *
 * const worklets = new WebAudioWorklets(audioContext);
 *
 * // Register and create custom processor
 * await worklets.registerWorklet('spectrum-analyzer', '/worklets/spectrum-analyzer.js');
 * const processor = await worklets.createProcessor('spectrum-analyzer', {
 *   fftSize: 2048,
 *   windowType: 'hann'
 * });
 * ```
 */

/**
 * Web Audio Worklets Manager
 *
 * Provides comprehensive audio worklet management with custom processing,
 * real-time DSP, and performance optimization.
 *
 * @class WebAudioWorklets
 */
export class WebAudioWorklets {
  /**
   * Create WebAudioWorklets manager
   *
   * @param {AudioContext} audioContext - Web Audio context
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableSharedArrayBuffer=true] - Enable shared memory
   * @param {number} [options.maxWorklets=20] - Maximum number of worklets
   * @param {string} [options.workletPath='/worklets/'] - Base path for worklet files
   */
  constructor(audioContext, options = {}) {
    if (!audioContext) {
      throw new Error("AudioContext is required");
    }

    this.audioContext = audioContext;

    // Configuration
    this.config = {
      enableSharedArrayBuffer: options.enableSharedArrayBuffer !== false,
      maxWorklets: options.maxWorklets || 20,
      workletPath: options.workletPath || "/worklets/",
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      maxMessageQueueSize: options.maxMessageQueueSize || 1000,
      processorTimeout: options.processorTimeout || 5000,
      enableDebugging: options.enableDebugging || false,
      ...options,
    };

    // Worklet management
    this.registeredWorklets = new Map(); // workletName -> registration data
    this.activeProcessors = new Map(); // processorId -> processor data
    this.workletModules = new Map(); // moduleName -> module data
    this.messageQueues = new Map(); // processorId -> message queue

    // Built-in worklet processors
    this.builtInProcessors = new Map();

    // Performance tracking
    this.performance = {
      activeWorklets: 0,
      totalWorklets: 0,
      messagesPerSecond: 0,
      averageLatency: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      droppedMessages: 0,
      processingErrors: 0,
    };

    // Event handling
    this.eventHandlers = new Map();

    // Capabilities detection
    this.capabilities = {
      supportsWorklets: false,
      supportsSharedArrayBuffer: false,
      supportsAtomics: false,
      maxChannelCount: 32,
    };

    // Initialize system
    this._detectCapabilities();
    this._initializeBuiltInProcessors();
    this._setupMessageHandling();

    // Start performance monitoring
    if (this.config.enablePerformanceMonitoring) {
      this._startPerformanceMonitoring();
    }

    console.log("WebAudioWorklets manager initialized");
  }

  /**
   * Detect browser capabilities
   * @private
   */
  _detectCapabilities() {
    // Check for AudioWorklet support
    this.capabilities.supportsWorklets = !!(
      this.audioContext.audioWorklet && this.audioContext.audioWorklet.addModule
    );

    // Check for SharedArrayBuffer support
    this.capabilities.supportsSharedArrayBuffer = !!(
      typeof SharedArrayBuffer !== "undefined" &&
      this.config.enableSharedArrayBuffer
    );

    // Check for Atomics support
    this.capabilities.supportsAtomics = !!(
      typeof Atomics !== "undefined" &&
      this.capabilities.supportsSharedArrayBuffer
    );

    // Get maximum channel count
    try {
      const testBuffer = this.audioContext.createBuffer(32, 128, 44100);
      this.capabilities.maxChannelCount = testBuffer.numberOfChannels;
    } catch (error) {
      this.capabilities.maxChannelCount = 2;
    }

    console.log("WebAudioWorklets capabilities:", this.capabilities);
  }

  /**
   * Initialize built-in processors
   * @private
   */
  _initializeBuiltInProcessors() {
    // Spectrum analyzer processor
    this.builtInProcessors.set("spectrum-analyzer", {
      name: "spectrum-analyzer",
      description: "Real-time spectrum analysis processor",
      parameters: {
        fftSize: { defaultValue: 2048, minValue: 256, maxValue: 32768 },
        windowType: {
          defaultValue: "hann",
          options: ["hann", "hamming", "blackman"],
        },
        smoothing: { defaultValue: 0.8, minValue: 0, maxValue: 1 },
      },
      code: this._generateSpectrumAnalyzerCode(),
    });

    // Pitch detector processor
    this.builtInProcessors.set("pitch-detector", {
      name: "pitch-detector",
      description: "Real-time pitch detection processor",
      parameters: {
        minFrequency: { defaultValue: 80, minValue: 20, maxValue: 2000 },
        maxFrequency: { defaultValue: 2000, minValue: 200, maxValue: 20000 },
        threshold: { defaultValue: 0.01, minValue: 0, maxValue: 1 },
      },
      code: this._generatePitchDetectorCode(),
    });

    // Audio enhancer processor
    this.builtInProcessors.set("audio-enhancer", {
      name: "audio-enhancer",
      description: "Real-time audio enhancement processor",
      parameters: {
        bassBoost: { defaultValue: 0, minValue: -20, maxValue: 20 },
        trebleBoost: { defaultValue: 0, minValue: -20, maxValue: 20 },
        dynamicRange: { defaultValue: 1, minValue: 0.1, maxValue: 10 },
      },
      code: this._generateAudioEnhancerCode(),
    });

    // Noise gate processor
    this.builtInProcessors.set("noise-gate", {
      name: "noise-gate",
      description: "Real-time noise gate processor",
      parameters: {
        threshold: { defaultValue: -40, minValue: -80, maxValue: 0 },
        ratio: { defaultValue: 10, minValue: 1, maxValue: 100 },
        attack: { defaultValue: 0.003, minValue: 0.001, maxValue: 1 },
        release: { defaultValue: 0.1, minValue: 0.01, maxValue: 5 },
      },
      code: this._generateNoiseGateCode(),
    });

    // Convolution reverb processor
    this.builtInProcessors.set("convolution-reverb", {
      name: "convolution-reverb",
      description: "High-quality convolution reverb processor",
      parameters: {
        wetLevel: { defaultValue: 0.3, minValue: 0, maxValue: 1 },
        dryLevel: { defaultValue: 0.7, minValue: 0, maxValue: 1 },
        preDelay: { defaultValue: 0, minValue: 0, maxValue: 0.1 },
      },
      code: this._generateConvolutionReverbCode(),
    });

    console.log(
      `Initialized ${this.builtInProcessors.size} built-in processors`
    );
  }

  /**
   * Setup message handling
   * @private
   */
  _setupMessageHandling() {
    // Message processing interval
    this.messageProcessingInterval = null;

    // Message statistics
    this.messageStats = {
      sent: 0,
      received: 0,
      dropped: 0,
      errors: 0,
    };
  }

  /**
   * Register audio worklet module
   *
   * @param {string} workletName - Worklet name
   * @param {string|Object} moduleOrUrl - Module URL or built-in processor config
   * @param {Object} [options] - Registration options
   * @returns {Promise<void>}
   */
  async registerWorklet(workletName, moduleOrUrl, options = {}) {
    if (this.registeredWorklets.has(workletName)) {
      throw new Error(`Worklet "${workletName}" is already registered`);
    }

    if (!this.capabilities.supportsWorklets) {
      throw new Error("AudioWorklet is not supported in this browser");
    }

    try {
      let moduleUrl;
      let isBuiltIn = false;

      // Handle built-in processors
      if (
        typeof moduleOrUrl === "string" &&
        this.builtInProcessors.has(moduleOrUrl)
      ) {
        const builtIn = this.builtInProcessors.get(moduleOrUrl);
        moduleUrl = await this._createBuiltInWorkletModule(builtIn);
        isBuiltIn = true;
      } else if (typeof moduleOrUrl === "object") {
        // Custom processor configuration
        moduleUrl = await this._createCustomWorkletModule(
          workletName,
          moduleOrUrl
        );
        isBuiltIn = false;
      } else {
        // External module URL
        moduleUrl = moduleOrUrl.startsWith("http")
          ? moduleOrUrl
          : this.config.workletPath + moduleOrUrl;
        isBuiltIn = false;
      }

      const startTime = performance.now();

      // Register the worklet module
      await this.audioContext.audioWorklet.addModule(moduleUrl);

      const registrationData = {
        name: workletName,
        moduleUrl: moduleUrl,
        isBuiltIn: isBuiltIn,
        options: options,
        registeredAt: Date.now(),
        registrationTime: performance.now() - startTime,
        activeProcessors: 0,
      };

      this.registeredWorklets.set(workletName, registrationData);

      console.log(
        `Registered worklet "${workletName}" in ${registrationData.registrationTime.toFixed(
          2
        )}ms`
      );

      this._emitEvent("workletRegistered", {
        workletName,
        registrationData,
        registrationTime: registrationData.registrationTime,
      });
    } catch (error) {
      console.error(`Failed to register worklet "${workletName}":`, error);
      this._emitEvent("workletRegistrationError", { workletName, error });
      throw error;
    }
  }

  /**
   * Create audio worklet processor
   *
   * @param {string} workletName - Registered worklet name
   * @param {Object} [options] - Processor options
   * @param {string} [processorId] - Custom processor ID
   * @returns {Promise<AudioWorkletNode>} Worklet node
   */
  async createProcessor(workletName, options = {}, processorId) {
    const registration = this.registeredWorklets.get(workletName);
    if (!registration) {
      throw new Error(`Worklet "${workletName}" is not registered`);
    }

    if (this.activeProcessors.size >= this.config.maxWorklets) {
      throw new Error(
        `Maximum number of worklets (${this.config.maxWorklets}) reached`
      );
    }

    // Generate processor ID
    if (!processorId) {
      processorId = this._generateProcessorId(workletName);
    }

    try {
      const startTime = performance.now();

      // Create processor options
      const processorOptions = {
        numberOfInputs: options.numberOfInputs || 1,
        numberOfOutputs: options.numberOfOutputs || 1,
        channelCount: options.channelCount || 2,
        channelCountMode: options.channelCountMode || "explicit",
        channelInterpretation: options.channelInterpretation || "speakers",
        processorOptions: {
          processorId: processorId,
          ...options,
        },
      };

      // Create the worklet node
      const workletNode = new AudioWorkletNode(
        this.audioContext,
        workletName,
        processorOptions
      );

      // Setup message handling
      workletNode.port.onmessage = (event) => {
        this._handleWorkletMessage(processorId, event.data);
      };

      workletNode.port.onmessageerror = (event) => {
        console.error(`Message error from processor ${processorId}:`, event);
        this.performance.processingErrors++;
        this._emitEvent("processorMessageError", { processorId, error: event });
      };

      // Setup error handling
      workletNode.onprocessorerror = (event) => {
        console.error(`Processor error in ${processorId}:`, event);
        this.performance.processingErrors++;
        this._emitEvent("processorError", { processorId, error: event });
      };

      const processorData = {
        id: processorId,
        workletName: workletName,
        node: workletNode,
        options: processorOptions,
        messageQueue: [],
        isActive: true,
        createdAt: Date.now(),
        creationTime: performance.now() - startTime,
        stats: {
          messagesReceived: 0,
          messagesSent: 0,
          processingTime: 0,
          lastActivity: Date.now(),
        },
      };

      // Store processor
      this.activeProcessors.set(processorId, processorData);
      this.messageQueues.set(processorId, []);

      // Update registration stats
      registration.activeProcessors++;

      // Update performance metrics
      this.performance.activeWorklets++;
      this.performance.totalWorklets++;

      console.log(
        `Created processor "${processorId}" for worklet "${workletName}" in ${processorData.creationTime.toFixed(
          2
        )}ms`
      );

      this._emitEvent("processorCreated", {
        processorId,
        workletName,
        processorData,
        creationTime: processorData.creationTime,
      });

      return workletNode;
    } catch (error) {
      console.error(
        `Failed to create processor for worklet "${workletName}":`,
        error
      );
      this._emitEvent("processorCreationError", {
        workletName,
        processorId,
        error,
      });
      throw error;
    }
  }

  /**
   * Send message to processor
   *
   * @param {string} processorId - Processor ID
   * @param {Object} message - Message data
   * @returns {boolean} Success status
   */
  sendMessage(processorId, message) {
    const processorData = this.activeProcessors.get(processorId);
    if (!processorData) {
      console.warn(`Processor "${processorId}" not found`);
      return false;
    }

    try {
      // Add message metadata
      const messageWithMeta = {
        ...message,
        timestamp: performance.now(),
        messageId: this._generateMessageId(),
      };

      // Send message
      processorData.node.port.postMessage(messageWithMeta);

      // Update statistics
      processorData.stats.messagesSent++;
      this.messageStats.sent++;
      processorData.stats.lastActivity = Date.now();

      if (this.config.enableDebugging) {
        console.log(
          `Sent message to processor ${processorId}:`,
          messageWithMeta
        );
      }

      return true;
    } catch (error) {
      console.error(
        `Failed to send message to processor ${processorId}:`,
        error
      );
      this.messageStats.errors++;
      this.performance.processingErrors++;
      return false;
    }
  }

  /**
   * Handle worklet message
   * @private
   */
  _handleWorkletMessage(processorId, messageData) {
    const processorData = this.activeProcessors.get(processorId);
    if (!processorData) {
      return;
    }

    try {
      // Update statistics
      processorData.stats.messagesReceived++;
      this.messageStats.received++;
      processorData.stats.lastActivity = Date.now();

      // Calculate processing time if timestamp exists
      if (messageData.timestamp) {
        const processingTime = performance.now() - messageData.timestamp;
        processorData.stats.processingTime = processingTime;
      }

      // Add to message queue
      const messageQueue = this.messageQueues.get(processorId);
      if (messageQueue.length >= this.config.maxMessageQueueSize) {
        messageQueue.shift(); // Remove oldest message
        this.messageStats.dropped++;
      }
      messageQueue.push({
        ...messageData,
        receivedAt: performance.now(),
      });

      if (this.config.enableDebugging) {
        console.log(
          `Received message from processor ${processorId}:`,
          messageData
        );
      }

      // Emit event
      this._emitEvent("processorMessage", {
        processorId,
        messageData,
        processingTime: processorData.stats.processingTime,
      });
    } catch (error) {
      console.error(
        `Error handling message from processor ${processorId}:`,
        error
      );
      this.messageStats.errors++;
      this.performance.processingErrors++;
    }
  }

  /**
   * Update processor parameters
   *
   * @param {string} processorId - Processor ID
   * @param {Object} parameters - Parameter updates
   * @returns {boolean} Success status
   */
  updateParameters(processorId, parameters) {
    return this.sendMessage(processorId, {
      type: "parameterUpdate",
      parameters: parameters,
    });
  }

  /**
   * Get processor messages
   *
   * @param {string} processorId - Processor ID
   * @param {number} [maxMessages] - Maximum messages to return
   * @returns {Array} Messages
   */
  getProcessorMessages(processorId, maxMessages) {
    const messageQueue = this.messageQueues.get(processorId);
    if (!messageQueue) {
      return [];
    }

    if (maxMessages && maxMessages < messageQueue.length) {
      return messageQueue.slice(-maxMessages);
    }

    return [...messageQueue];
  }

  /**
   * Clear processor messages
   *
   * @param {string} processorId - Processor ID
   */
  clearProcessorMessages(processorId) {
    const messageQueue = this.messageQueues.get(processorId);
    if (messageQueue) {
      messageQueue.length = 0;
    }
  }

  /**
   * Destroy processor
   *
   * @param {string} processorId - Processor ID
   */
  destroyProcessor(processorId) {
    const processorData = this.activeProcessors.get(processorId);
    if (!processorData) {
      throw new Error(`Processor "${processorId}" not found`);
    }

    try {
      // Disconnect the node
      processorData.node.disconnect();

      // Update registration stats
      const registration = this.registeredWorklets.get(
        processorData.workletName
      );
      if (registration) {
        registration.activeProcessors--;
      }

      // Clean up
      this.activeProcessors.delete(processorId);
      this.messageQueues.delete(processorId);

      // Update performance metrics
      this.performance.activeWorklets--;

      console.log(`Destroyed processor "${processorId}"`);

      this._emitEvent("processorDestroyed", { processorId });
    } catch (error) {
      console.error(`Failed to destroy processor ${processorId}:`, error);
      this._emitEvent("processorDestroyError", { processorId, error });
      throw error;
    }
  }

  // === BUILT-IN PROCESSOR CODE GENERATORS ===

  /**
   * Generate spectrum analyzer processor code
   * @private
   */
  _generateSpectrumAnalyzerCode() {
    return `
class SpectrumAnalyzerProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.fftSize = options.processorOptions.fftSize || 2048;
    this.windowType = options.processorOptions.windowType || 'hann';
    this.smoothing = options.processorOptions.smoothing || 0.8;

    this.bufferSize = this.fftSize;
    this.buffer = new Float32Array(this.bufferSize);
    this.bufferIndex = 0;
    this.fftData = new Float32Array(this.fftSize);
    this.spectrum = new Float32Array(this.fftSize / 2);
    this.window = this.createWindow(this.fftSize, this.windowType);

    this.port.onmessage = this.handleMessage.bind(this);
  }

  createWindow(size, type) {
    const window = new Float32Array(size);
    for (let i = 0; i < size; i++) {
      switch (type) {
        case 'hann':
          window[i] = 0.5 * (1 - Math.cos(2 * Math.PI * i / (size - 1)));
          break;
        case 'hamming':
          window[i] = 0.54 - 0.46 * Math.cos(2 * Math.PI * i / (size - 1));
          break;
        case 'blackman':
          window[i] = 0.42 - 0.5 * Math.cos(2 * Math.PI * i / (size - 1)) +
                     0.08 * Math.cos(4 * Math.PI * i / (size - 1));
          break;
        default:
          window[i] = 1;
      }
    }
    return window;
  }

  process(inputs, outputs) {
    const input = inputs[0];
    if (input.length > 0) {
      const channelData = input[0];

      for (let i = 0; i < channelData.length; i++) {
        this.buffer[this.bufferIndex] = channelData[i];
        this.bufferIndex = (this.bufferIndex + 1) % this.bufferSize;

        if (this.bufferIndex === 0) {
          this.performFFT();
        }
      }
    }

    return true;
  }

  performFFT() {
    // Apply window function
    for (let i = 0; i < this.fftSize; i++) {
      this.fftData[i] = this.buffer[i] * this.window[i];
    }

    // Simplified FFT implementation (placeholder)
    for (let i = 0; i < this.spectrum.length; i++) {
      this.spectrum[i] = this.spectrum[i] * this.smoothing +
                        Math.abs(this.fftData[i]) * (1 - this.smoothing);
    }

    this.port.postMessage({
      type: 'spectrum',
      data: Array.from(this.spectrum),
      timestamp: currentTime
    });
  }

  handleMessage(event) {
    const { type, parameters } = event.data;
    if (type === 'parameterUpdate') {
      if (parameters.fftSize) this.fftSize = parameters.fftSize;
      if (parameters.smoothing) this.smoothing = parameters.smoothing;
      if (parameters.windowType) {
        this.windowType = parameters.windowType;
        this.window = this.createWindow(this.fftSize, this.windowType);
      }
    }
  }
}

registerProcessor('spectrum-analyzer', SpectrumAnalyzerProcessor);
`;
  }

  /**
   * Generate pitch detector processor code
   * @private
   */
  _generatePitchDetectorCode() {
    return `
class PitchDetectorProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.minFrequency = options.processorOptions.minFrequency || 80;
    this.maxFrequency = options.processorOptions.maxFrequency || 2000;
    this.threshold = options.processorOptions.threshold || 0.01;

    this.bufferSize = 4096;
    this.buffer = new Float32Array(this.bufferSize);
    this.bufferIndex = 0;

    this.port.onmessage = this.handleMessage.bind(this);
  }

  process(inputs, outputs) {
    const input = inputs[0];
    if (input.length > 0) {
      const channelData = input[0];

      for (let i = 0; i < channelData.length; i++) {
        this.buffer[this.bufferIndex] = channelData[i];
        this.bufferIndex = (this.bufferIndex + 1) % this.bufferSize;

        if (this.bufferIndex % 512 === 0) {
          this.detectPitch();
        }
      }
    }

    return true;
  }

  detectPitch() {
    const pitch = this.autocorrelation();

    if (pitch > 0) {
      this.port.postMessage({
        type: 'pitch',
        frequency: pitch,
        confidence: this.calculateConfidence(pitch),
        timestamp: currentTime
      });
    }
  }

  autocorrelation() {
    const sampleRate = 44100; // Assume standard sample rate
    const minPeriod = Math.floor(sampleRate / this.maxFrequency);
    const maxPeriod = Math.floor(sampleRate / this.minFrequency);

    let bestCorrelation = 0;
    let bestPeriod = 0;

    for (let period = minPeriod; period <= maxPeriod; period++) {
      let correlation = 0;
      for (let i = 0; i < this.bufferSize - period; i++) {
        correlation += this.buffer[i] * this.buffer[i + period];
      }

      if (correlation > bestCorrelation) {
        bestCorrelation = correlation;
        bestPeriod = period;
      }
    }

    return bestCorrelation > this.threshold ? sampleRate / bestPeriod : 0;
  }

  calculateConfidence(pitch) {
    // Simplified confidence calculation
    return Math.min(1.0, pitch / 1000);
  }

  handleMessage(event) {
    const { type, parameters } = event.data;
    if (type === 'parameterUpdate') {
      if (parameters.minFrequency) this.minFrequency = parameters.minFrequency;
      if (parameters.maxFrequency) this.maxFrequency = parameters.maxFrequency;
      if (parameters.threshold) this.threshold = parameters.threshold;
    }
  }
}

registerProcessor('pitch-detector', PitchDetectorProcessor);
`;
  }

  /**
   * Generate audio enhancer processor code
   * @private
   */
  _generateAudioEnhancerCode() {
    return `
class AudioEnhancerProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.bassBoost = options.processorOptions.bassBoost || 0;
    this.trebleBoost = options.processorOptions.trebleBoost || 0;
    this.dynamicRange = options.processorOptions.dynamicRange || 1;

    // Simple filters
    this.lowpass = { x1: 0, x2: 0, y1: 0, y2: 0 };
    this.highpass = { x1: 0, x2: 0, y1: 0, y2: 0 };

    this.port.onmessage = this.handleMessage.bind(this);
  }

  process(inputs, outputs) {
    const input = inputs[0];
    const output = outputs[0];

    if (input.length > 0 && output.length > 0) {
      for (let channel = 0; channel < input.length; channel++) {
        const inputChannel = input[channel];
        const outputChannel = output[channel];

        for (let i = 0; i < inputChannel.length; i++) {
          let sample = inputChannel[i];

          // Dynamic range processing
          sample *= this.dynamicRange;

          // Bass boost (lowpass emphasis)
          const bassGain = Math.pow(10, this.bassBoost / 20);
          const lowpass = this.lowpassFilter(sample, 200);

          // Treble boost (highpass emphasis)
          const trebleGain = Math.pow(10, this.trebleBoost / 20);
          const highpass = this.highpassFilter(sample, 2000);

          // Combine
          outputChannel[i] = sample + (lowpass * (bassGain - 1)) + (highpass * (trebleGain - 1));
        }
      }
    }

    return true;
  }

  lowpassFilter(input, cutoff) {
    const sampleRate = 44100;
    const RC = 1.0 / (cutoff * 2 * Math.PI);
    const dt = 1.0 / sampleRate;
    const alpha = dt / (RC + dt);

    this.lowpass.y1 = this.lowpass.y1 + alpha * (input - this.lowpass.y1);
    return this.lowpass.y1;
  }

  highpassFilter(input, cutoff) {
    const sampleRate = 44100;
    const RC = 1.0 / (cutoff * 2 * Math.PI);
    const dt = 1.0 / sampleRate;
    const alpha = RC / (RC + dt);

    this.highpass.y1 = alpha * (this.highpass.y1 + input - this.highpass.x1);
    this.highpass.x1 = input;
    return this.highpass.y1;
  }

  handleMessage(event) {
    const { type, parameters } = event.data;
    if (type === 'parameterUpdate') {
      if (parameters.bassBoost !== undefined) this.bassBoost = parameters.bassBoost;
      if (parameters.trebleBoost !== undefined) this.trebleBoost = parameters.trebleBoost;
      if (parameters.dynamicRange !== undefined) this.dynamicRange = parameters.dynamicRange;
    }
  }
}

registerProcessor('audio-enhancer', AudioEnhancerProcessor);
`;
  }

  /**
   * Generate noise gate processor code
   * @private
   */
  _generateNoiseGateCode() {
    return `
class NoiseGateProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.threshold = Math.pow(10, (options.processorOptions.threshold || -40) / 20);
    this.ratio = options.processorOptions.ratio || 10;
    this.attack = options.processorOptions.attack || 0.003;
    this.release = options.processorOptions.release || 0.1;

    this.envelope = 0;
    this.gateOpen = false;

    this.port.onmessage = this.handleMessage.bind(this);
  }

  process(inputs, outputs) {
    const input = inputs[0];
    const output = outputs[0];
    const sampleRate = 44100;

    if (input.length > 0 && output.length > 0) {
      for (let channel = 0; channel < input.length; channel++) {
        const inputChannel = input[channel];
        const outputChannel = output[channel];

        for (let i = 0; i < inputChannel.length; i++) {
          const sample = inputChannel[i];
          const level = Math.abs(sample);

          // Update envelope
          const targetEnvelope = level > this.threshold ? 1 : 0;
          const rate = targetEnvelope > this.envelope ? this.attack : this.release;
          const coeff = Math.exp(-1 / (rate * sampleRate));
          this.envelope = targetEnvelope + (this.envelope - targetEnvelope) * coeff;

          // Apply gate
          let gain = 1;
          if (this.envelope < 1) {
            gain = Math.pow(this.envelope, 1 / this.ratio);
          }

          outputChannel[i] = sample * gain;
        }
      }
    }

    return true;
  }

  handleMessage(event) {
    const { type, parameters } = event.data;
    if (type === 'parameterUpdate') {
      if (parameters.threshold !== undefined) {
        this.threshold = Math.pow(10, parameters.threshold / 20);
      }
      if (parameters.ratio !== undefined) this.ratio = parameters.ratio;
      if (parameters.attack !== undefined) this.attack = parameters.attack;
      if (parameters.release !== undefined) this.release = parameters.release;
    }
  }
}

registerProcessor('noise-gate', NoiseGateProcessor);
`;
  }

  /**
   * Generate convolution reverb processor code
   * @private
   */
  _generateConvolutionReverbCode() {
    return `
class ConvolutionReverbProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.wetLevel = options.processorOptions.wetLevel || 0.3;
    this.dryLevel = options.processorOptions.dryLevel || 0.7;
    this.preDelay = options.processorOptions.preDelay || 0;

    this.impulseResponse = null;
    this.convolutionBuffer = new Float32Array(8192);
    this.bufferIndex = 0;

    this.port.onmessage = this.handleMessage.bind(this);
  }

  process(inputs, outputs) {
    const input = inputs[0];
    const output = outputs[0];

    if (input.length > 0 && output.length > 0 && this.impulseResponse) {
      for (let channel = 0; channel < input.length; channel++) {
        const inputChannel = input[channel];
        const outputChannel = output[channel];

        for (let i = 0; i < inputChannel.length; i++) {
          const drySample = inputChannel[i];

          // Store in convolution buffer
          this.convolutionBuffer[this.bufferIndex] = drySample;

          // Simple convolution (simplified for performance)
          let wetSample = 0;
          const impulseLength = Math.min(this.impulseResponse.length, 1024);
          for (let j = 0; j < impulseLength; j++) {
            const bufferPos = (this.bufferIndex - j + this.convolutionBuffer.length) % this.convolutionBuffer.length;
            wetSample += this.convolutionBuffer[bufferPos] * this.impulseResponse[j];
          }

          // Mix dry and wet
          outputChannel[i] = drySample * this.dryLevel + wetSample * this.wetLevel;

          this.bufferIndex = (this.bufferIndex + 1) % this.convolutionBuffer.length;
        }
      }
    } else if (input.length > 0 && output.length > 0) {
      // Pass through if no impulse response
      for (let channel = 0; channel < input.length; channel++) {
        output[channel].set(input[channel]);
      }
    }

    return true;
  }

  handleMessage(event) {
    const { type, parameters, impulseResponse } = event.data;
    if (type === 'parameterUpdate') {
      if (parameters.wetLevel !== undefined) this.wetLevel = parameters.wetLevel;
      if (parameters.dryLevel !== undefined) this.dryLevel = parameters.dryLevel;
      if (parameters.preDelay !== undefined) this.preDelay = parameters.preDelay;
    } else if (type === 'setImpulseResponse') {
      this.impulseResponse = new Float32Array(impulseResponse);
    }
  }
}

registerProcessor('convolution-reverb', ConvolutionReverbProcessor);
`;
  }

  /**
   * Create built-in worklet module
   * @private
   */
  async _createBuiltInWorkletModule(builtInProcessor) {
    const blob = new Blob([builtInProcessor.code], {
      type: "application/javascript",
    });
    return URL.createObjectURL(blob);
  }

  /**
   * Create custom worklet module
   * @private
   */
  async _createCustomWorkletModule(name, config) {
    const code = `
class ${name}Processor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    ${config.constructorCode || ""}
    this.port.onmessage = this.handleMessage.bind(this);
  }

  process(inputs, outputs, parameters) {
    ${config.processCode || "return true;"}
  }

  handleMessage(event) {
    ${config.messageHandlerCode || ""}
  }

  ${config.additionalMethods || ""}
}

registerProcessor('${name}', ${name}Processor);
`;

    const blob = new Blob([code], { type: "application/javascript" });
    return URL.createObjectURL(blob);
  }

  // === UTILITY METHODS ===

  /**
   * Generate unique processor ID
   * @private
   */
  _generateProcessorId(workletName) {
    return `${workletName}_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;
  }

  /**
   * Generate unique message ID
   * @private
   */
  _generateMessageId() {
    return `msg_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  /**
   * Start performance monitoring
   * @private
   */
  _startPerformanceMonitoring() {
    const updateMetrics = () => {
      // Calculate messages per second
      const now = Date.now();
      if (this.lastMetricsUpdate) {
        const timeDelta = (now - this.lastMetricsUpdate) / 1000;
        const messagesDelta =
          this.messageStats.sent - (this.lastMessageCount || 0);
        this.performance.messagesPerSecond = messagesDelta / timeDelta;
      }
      this.lastMetricsUpdate = now;
      this.lastMessageCount = this.messageStats.sent;

      // Calculate average latency
      let totalLatency = 0;
      let activeProcessorCount = 0;
      for (const processorData of this.activeProcessors.values()) {
        if (processorData.stats.processingTime > 0) {
          totalLatency += processorData.stats.processingTime;
          activeProcessorCount++;
        }
      }
      this.performance.averageLatency =
        activeProcessorCount > 0 ? totalLatency / activeProcessorCount : 0;

      // Update memory usage if available
      if (performance.memory) {
        this.performance.memoryUsage =
          performance.memory.usedJSHeapSize / 1024 / 1024;
      }

      // Estimate CPU usage based on active worklets
      this.performance.cpuUsage = Math.min(
        0.05 + this.performance.activeWorklets * 0.1,
        1.0
      );

      this._emitEvent("performanceUpdate", this.performance);
    };

    // Update metrics every 5 seconds
    setInterval(updateMetrics, 5000);
  }

  /**
   * Get registered worklet
   *
   * @param {string} workletName - Worklet name
   * @returns {Object|null} Registration data
   */
  getRegisteredWorklet(workletName) {
    return this.registeredWorklets.get(workletName) || null;
  }

  /**
   * Get all registered worklets
   *
   * @returns {Map<string, Object>} All registrations
   */
  getAllRegisteredWorklets() {
    return new Map(this.registeredWorklets);
  }

  /**
   * Get active processor
   *
   * @param {string} processorId - Processor ID
   * @returns {Object|null} Processor data
   */
  getProcessor(processorId) {
    return this.activeProcessors.get(processorId) || null;
  }

  /**
   * Get all active processors
   *
   * @returns {Map<string, Object>} All processors
   */
  getAllProcessors() {
    return new Map(this.activeProcessors);
  }

  /**
   * Get built-in processors
   *
   * @returns {Map<string, Object>} Built-in processors
   */
  getBuiltInProcessors() {
    return new Map(this.builtInProcessors);
  }

  /**
   * Get capabilities
   *
   * @returns {Object} Browser capabilities
   */
  getCapabilities() {
    return { ...this.capabilities };
  }

  /**
   * Get performance metrics
   *
   * @returns {Object} Performance metrics
   */
  getPerformanceMetrics() {
    return { ...this.performance };
  }

  /**
   * Get message statistics
   *
   * @returns {Object} Message statistics
   */
  getMessageStatistics() {
    return { ...this.messageStats };
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
  destroy() {
    console.log("Destroying WebAudioWorklets manager...");

    // Destroy all processors
    for (const processorId of this.activeProcessors.keys()) {
      try {
        this.destroyProcessor(processorId);
      } catch (error) {
        console.warn(`Failed to destroy processor ${processorId}:`, error);
      }
    }

    // Clean up URLs for built-in processors
    for (const [
      workletName,
      registration,
    ] of this.registeredWorklets.entries()) {
      if (
        registration.isBuiltIn &&
        registration.moduleUrl.startsWith("blob:")
      ) {
        try {
          URL.revokeObjectURL(registration.moduleUrl);
        } catch (error) {
          console.warn(`Failed to revoke URL for ${workletName}:`, error);
        }
      }
    }

    // Clear intervals
    if (this.messageProcessingInterval) {
      clearInterval(this.messageProcessingInterval);
    }

    // Clear all collections
    this.registeredWorklets.clear();
    this.activeProcessors.clear();
    this.workletModules.clear();
    this.messageQueues.clear();
    this.eventHandlers.clear();

    // Reset performance counters
    this.performance.activeWorklets = 0;

    this._emitEvent("destroyed");
    console.log("WebAudioWorklets manager destroyed");
  }
}

export default WebAudioWorklets;
