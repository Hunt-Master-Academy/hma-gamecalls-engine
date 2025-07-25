/**
 * AudioWorkletManager Module
 *
 * Provides comprehensive Audio Worklet management with fallback support,
 * real-time processing coordination, message handling, and performance optimization.
 * Manages both modern AudioWorklet API and legacy ScriptProcessor fallbacks.
 *
 * Features:
 * - Audio Worklet setup and lifecycle management
 * - ScriptProcessor fallback for compatibility
 * - Real-time processing coordination
 * - Worklet message handling and communication
 * - Performance monitoring and optimization
 * - Dynamic worklet parameter updates
 * - Multi-worklet management and routing
 * - Error handling and recovery
 * - Memory management and garbage collection
 * - Cross-browser compatibility layer
 */

export class AudioWorkletManager {
  constructor(eventManager, audioContext) {
    this.eventManager = eventManager;
    this.audioContext = audioContext;
    this.isInitialized = false;

    // Core configuration
    this.config = {
      // Worklet settings
      enableAudioWorklets: true,
      fallbackToScriptProcessor: true,
      workletModulePath: "src/worklets/",
      maxConcurrentWorklets: 8,

      // Processing settings
      defaultBufferSize: 4096,
      preferredChannelCount: 1,
      enableParameterAutomation: true,

      // Performance settings
      lowLatencyMode: false,
      adaptiveBufferSize: true,
      memoryOptimization: true,

      // Compatibility settings
      checkBrowserSupport: true,
      enablePolyfills: true,
      gracefulDegradation: true,
    };

    // Worklet registry
    this.worklets = new Map();
    this.workletModules = new Map();

    // Active worklet nodes
    this.activeWorklets = new Map();
    this.workletConnections = new Map();

    // ScriptProcessor fallback
    this.scriptProcessors = new Map();
    this.useScriptProcessor = false;

    // Message handling
    this.messageHandlers = new Map();
    this.messageQueue = [];
    this.messageProcessingActive = false;

    // Performance monitoring
    this.performance = {
      workletsLoaded: 0,
      activeNodes: 0,
      messagesProcessed: 0,
      averageLatency: 0,
      memoryUsage: 0,
      cpuUsage: 0,
      underruns: 0,
      processingLoad: 0,
    };

    // Browser compatibility
    this.browserSupport = {
      audioWorklet: false,
      scriptProcessor: false,
      webAudio: false,
      sharedArrayBuffer: false,
      wasmSupport: false,
    };

    // Worklet definitions
    this.availableWorklets = {
      "audio-processor": {
        modulePath: "audio-worklet-processor.js",
        processorName: "audio-worklet-processor",
        description: "Main audio processing worklet",
        parameters: {
          bufferSize: { defaultValue: 4096, minValue: 256, maxValue: 16384 },
          enableAGC: { defaultValue: 1, minValue: 0, maxValue: 1 },
          enableNoiseReduction: { defaultValue: 1, minValue: 0, maxValue: 1 },
          gain: { defaultValue: 1.0, minValue: 0.0, maxValue: 2.0 },
        },
      },
      "level-monitor": {
        modulePath: "level-monitor-worklet.js",
        processorName: "level-monitor-processor",
        description: "Real-time level monitoring worklet",
        parameters: {
          updateRate: { defaultValue: 60, minValue: 1, maxValue: 120 },
          peakHoldTime: { defaultValue: 1000, minValue: 100, maxValue: 5000 },
        },
      },
      "noise-gate": {
        modulePath: "noise-gate-worklet.js",
        processorName: "noise-gate-processor",
        description: "Noise gate processing worklet",
        parameters: {
          threshold: { defaultValue: -40, minValue: -80, maxValue: 0 },
          ratio: { defaultValue: 10, minValue: 1, maxValue: 100 },
          attack: { defaultValue: 0.001, minValue: 0.0001, maxValue: 0.1 },
          release: { defaultValue: 0.1, minValue: 0.01, maxValue: 2.0 },
        },
      },
      "spectrum-analyzer": {
        modulePath: "spectrum-analyzer-worklet.js",
        processorName: "spectrum-analyzer-processor",
        description: "Real-time spectrum analysis worklet",
        parameters: {
          fftSize: { defaultValue: 2048, minValue: 256, maxValue: 32768 },
          smoothingTimeConstant: {
            defaultValue: 0.8,
            minValue: 0.0,
            maxValue: 0.95,
          },
        },
      },
    };

    // Connection routing
    this.routingGraph = {
      nodes: new Map(),
      connections: [],
      inputNode: null,
      outputNode: null,
    };

    this.bindMethods();
  }

  bindMethods() {
    this.setupAudioWorklets = this.setupAudioWorklets.bind(this);
    this.handleWorkletMessage = this.handleWorkletMessage.bind(this);
    this.setupScriptProcessorFallback =
      this.setupScriptProcessorFallback.bind(this);
    this.processMessageQueue = this.processMessageQueue.bind(this);
  }

  /**
   * Initialize the Audio Worklet Manager
   */
  async initialize(config = {}) {
    try {
      // Merge configuration
      this.config = { ...this.config, ...config };

      // Check browser support
      await this.checkBrowserSupport();

      // Initialize worklet system
      if (this.browserSupport.audioWorklet && this.config.enableAudioWorklets) {
        await this.initializeAudioWorkletSystem();
      } else if (this.config.fallbackToScriptProcessor) {
        await this.initializeScriptProcessorSystem();
        this.useScriptProcessor = true;
      } else {
        throw new Error("No supported audio processing method available");
      }

      // Initialize message handling
      this.initializeMessageHandling();

      // Initialize performance monitoring
      this.initializePerformanceMonitoring();

      // Initialize routing system
      this.initializeRoutingSystem();

      this.isInitialized = true;

      this.eventManager?.emitEvent("audioWorkletManagerInitialized", {
        useScriptProcessor: this.useScriptProcessor,
        browserSupport: this.browserSupport,
        availableWorklets: Object.keys(this.availableWorklets),
        timestamp: performance.now(),
      });

      return { success: true, useScriptProcessor: this.useScriptProcessor };
    } catch (error) {
      console.error("Failed to initialize AudioWorkletManager:", error);
      this.eventManager?.emitEvent("audioWorkletManagerError", {
        error: error.message,
        phase: "initialization",
      });
      throw error;
    }
  }

  /**
   * Check browser support for audio processing APIs
   */
  async checkBrowserSupport() {
    // Check AudioWorklet support
    this.browserSupport.audioWorklet = !!(
      this.audioContext &&
      this.audioContext.audioWorklet &&
      typeof this.audioContext.audioWorklet.addModule === "function"
    );

    // Check ScriptProcessor support (deprecated but widely supported)
    this.browserSupport.scriptProcessor = !!(
      this.audioContext &&
      typeof this.audioContext.createScriptProcessor === "function"
    );

    // Check Web Audio API support
    this.browserSupport.webAudio = !!(
      window.AudioContext || window.webkitAudioContext
    );

    // Check SharedArrayBuffer support (for advanced worklet features)
    this.browserSupport.sharedArrayBuffer =
      typeof SharedArrayBuffer !== "undefined";

    // Check WebAssembly support
    this.browserSupport.wasmSupport =
      typeof WebAssembly === "object" &&
      typeof WebAssembly.instantiate === "function";

    console.log("Browser support:", this.browserSupport);
  }

  /**
   * Initialize Audio Worklet system
   */
  async initializeAudioWorkletSystem() {
    try {
      // Load default worklet modules
      const defaultWorklets = ["audio-processor"];

      for (const workletName of defaultWorklets) {
        await this.loadWorkletModule(workletName);
      }

      console.log("AudioWorklet system initialized successfully");
    } catch (error) {
      console.error("Failed to initialize AudioWorklet system:", error);

      if (this.config.fallbackToScriptProcessor) {
        console.warn("Falling back to ScriptProcessor");
        await this.initializeScriptProcessorSystem();
        this.useScriptProcessor = true;
      } else {
        throw error;
      }
    }
  }

  /**
   * Initialize ScriptProcessor fallback system
   */
  async initializeScriptProcessorSystem() {
    // ScriptProcessor is immediately available, no async loading needed
    console.log("ScriptProcessor fallback system initialized");
  }

  /**
   * Initialize message handling system
   */
  initializeMessageHandling() {
    // Start message processing loop
    this.messageProcessingActive = true;
    this.processMessageQueue();

    // Setup periodic message processing
    this.messageInterval = setInterval(() => {
      if (this.messageQueue.length > 0) {
        this.processMessageQueue();
      }
    }, 16); // ~60 FPS
  }

  /**
   * Initialize performance monitoring
   */
  initializePerformanceMonitoring() {
    this.performanceInterval = setInterval(() => {
      this.updatePerformanceMetrics();
    }, 1000); // Every second
  }

  /**
   * Initialize routing system
   */
  initializeRoutingSystem() {
    // Create input and output nodes for the routing graph
    this.routingGraph.inputNode = this.audioContext.createGain();
    this.routingGraph.outputNode = this.audioContext.createGain();

    // Set default gain values
    this.routingGraph.inputNode.gain.value = 1.0;
    this.routingGraph.outputNode.gain.value = 1.0;
  }

  /**
   * Load a worklet module
   */
  async loadWorkletModule(workletName) {
    if (!this.availableWorklets[workletName]) {
      throw new Error(`Unknown worklet: ${workletName}`);
    }

    const workletDef = this.availableWorklets[workletName];
    const modulePath = `${this.config.workletModulePath}${workletDef.modulePath}`;

    try {
      // Load the worklet module
      await this.audioContext.audioWorklet.addModule(modulePath);

      // Register the worklet
      this.workletModules.set(workletName, {
        ...workletDef,
        loaded: true,
        loadTime: performance.now(),
      });

      this.performance.workletsLoaded++;

      this.eventManager?.emitEvent("workletModuleLoaded", {
        workletName,
        modulePath,
        timestamp: performance.now(),
      });

      console.log(`Worklet module loaded: ${workletName}`);
    } catch (error) {
      console.error(`Failed to load worklet module ${workletName}:`, error);

      // Mark as failed but keep definition for potential retry
      this.workletModules.set(workletName, {
        ...workletDef,
        loaded: false,
        error: error.message,
        errorTime: performance.now(),
      });

      throw error;
    }
  }

  /**
   * Setup audio worklets for processing
   */
  async setupAudioWorklets(workletName = "audio-processor", options = {}) {
    if (!this.isInitialized) {
      throw new Error("AudioWorkletManager not initialized");
    }

    if (this.useScriptProcessor) {
      return this.setupScriptProcessorFallback(workletName, options);
    }

    try {
      // Ensure worklet module is loaded
      if (
        !this.workletModules.has(workletName) ||
        !this.workletModules.get(workletName).loaded
      ) {
        await this.loadWorkletModule(workletName);
      }

      const workletDef = this.workletModules.get(workletName);

      // Create worklet node
      const workletOptions = {
        numberOfInputs: options.numberOfInputs || 1,
        numberOfOutputs: options.numberOfOutputs || 1,
        channelCount: options.channelCount || this.config.preferredChannelCount,
        processorOptions: {
          bufferSize: options.bufferSize || this.config.defaultBufferSize,
          ...options.processorOptions,
        },
      };

      const workletNode = new AudioWorkletNode(
        this.audioContext,
        workletDef.processorName,
        workletOptions
      );

      // Generate unique ID for this worklet instance
      const workletId = this.generateWorkletId(workletName);

      // Setup message handling for this worklet
      workletNode.port.onmessage = (event) => {
        this.handleWorkletMessage(event.data, workletId, workletName);
      };

      // Setup parameter automation if enabled
      if (this.config.enableParameterAutomation && workletDef.parameters) {
        this.setupParameterAutomation(workletNode, workletDef.parameters);
      }

      // Register the worklet instance
      this.activeWorklets.set(workletId, {
        node: workletNode,
        workletName,
        options: workletOptions,
        createdAt: performance.now(),
        messageCount: 0,
        lastActivity: performance.now(),
      });

      this.performance.activeNodes++;

      // Add to routing graph
      this.addToRoutingGraph(workletId, workletNode);

      this.eventManager?.emitEvent("workletCreated", {
        workletId,
        workletName,
        options: workletOptions,
        timestamp: performance.now(),
      });

      return {
        success: true,
        workletId,
        node: workletNode,
        useScriptProcessor: false,
      };
    } catch (error) {
      console.error("Failed to setup AudioWorklet:", error);

      if (this.config.fallbackToScriptProcessor) {
        console.warn("Falling back to ScriptProcessor for this instance");
        return this.setupScriptProcessorFallback(workletName, options);
      }

      throw error;
    }
  }

  /**
   * Setup ScriptProcessor fallback
   */
  setupScriptProcessorFallback(workletName = "audio-processor", options = {}) {
    try {
      const bufferSize = options.bufferSize || this.config.defaultBufferSize;
      const inputChannels =
        options.channelCount || this.config.preferredChannelCount;
      const outputChannels = inputChannels;

      // Create ScriptProcessor node
      const scriptProcessor = this.audioContext.createScriptProcessor(
        bufferSize,
        inputChannels,
        outputChannels
      );

      // Generate unique ID
      const processorId = this.generateScriptProcessorId(workletName);

      // Setup audio processing callback
      scriptProcessor.onaudioprocess = (event) => {
        this.handleScriptProcessorAudio(
          event,
          processorId,
          workletName,
          options
        );
      };

      // Register the script processor
      this.scriptProcessors.set(processorId, {
        node: scriptProcessor,
        workletName,
        options: { ...options, bufferSize, inputChannels, outputChannels },
        createdAt: performance.now(),
        processCount: 0,
        lastActivity: performance.now(),
      });

      this.performance.activeNodes++;

      // Add to routing graph
      this.addToRoutingGraph(processorId, scriptProcessor);

      this.eventManager?.emitEvent("scriptProcessorCreated", {
        processorId,
        workletName,
        bufferSize,
        channels: inputChannels,
        timestamp: performance.now(),
      });

      return {
        success: true,
        workletId: processorId,
        node: scriptProcessor,
        useScriptProcessor: true,
      };
    } catch (error) {
      console.error("Failed to setup ScriptProcessor fallback:", error);
      throw error;
    }
  }

  /**
   * Handle worklet messages
   */
  handleWorkletMessage(data, workletId, workletName) {
    try {
      // Update last activity
      const workletInfo = this.activeWorklets.get(workletId);
      if (workletInfo) {
        workletInfo.lastActivity = performance.now();
        workletInfo.messageCount++;
      }

      this.performance.messagesProcessed++;

      // Add to message queue for processing
      this.messageQueue.push({
        data,
        workletId,
        workletName,
        timestamp: performance.now(),
      });

      // Process immediately if queue is not too large
      if (this.messageQueue.length < 100) {
        this.processMessageQueue();
      }
    } catch (error) {
      console.error("Error handling worklet message:", error);

      this.eventManager?.emitEvent("workletMessageError", {
        error: error.message,
        workletId,
        workletName,
        timestamp: performance.now(),
      });
    }
  }

  /**
   * Process message queue
   */
  processMessageQueue() {
    if (!this.messageProcessingActive || this.messageQueue.length === 0) {
      return;
    }

    const startTime = performance.now();
    const maxProcessingTime = 5; // Max 5ms per batch

    while (
      this.messageQueue.length > 0 &&
      performance.now() - startTime < maxProcessingTime
    ) {
      const message = this.messageQueue.shift();
      this.processWorkletMessage(message);
    }
  }

  /**
   * Process individual worklet message
   */
  processWorkletMessage(message) {
    const { data, workletId, workletName, timestamp } = message;

    try {
      switch (data.type) {
        case "level-update":
          this.handleLevelUpdate(data, workletId);
          break;

        case "quality-metrics":
          this.handleQualityMetrics(data, workletId);
          break;

        case "processing-stats":
          this.handleProcessingStats(data, workletId);
          break;

        case "error":
          this.handleWorkletError(data, workletId, workletName);
          break;

        case "parameter-change":
          this.handleParameterChange(data, workletId);
          break;

        default:
          // Check for custom message handlers
          const handler = this.messageHandlers.get(data.type);
          if (handler) {
            handler(data, workletId, workletName);
          } else {
            console.warn(`Unknown worklet message type: ${data.type}`);
          }
      }
    } catch (error) {
      console.error("Error processing worklet message:", error);
    }
  }

  /**
   * Handle level update messages
   */
  handleLevelUpdate(data, workletId) {
    this.eventManager?.emitEvent("workletLevelUpdate", {
      workletId,
      level: data.level,
      peak: data.peak,
      rms: data.rms,
      timestamp: performance.now(),
    });
  }

  /**
   * Handle quality metrics messages
   */
  handleQualityMetrics(data, workletId) {
    this.eventManager?.emitEvent("workletQualityMetrics", {
      workletId,
      metrics: data.metrics,
      timestamp: performance.now(),
    });
  }

  /**
   * Handle processing statistics messages
   */
  handleProcessingStats(data, workletId) {
    // Update performance metrics
    if (data.latency !== undefined) {
      this.performance.averageLatency =
        this.performance.averageLatency * 0.9 + data.latency * 0.1;
    }

    if (data.cpuUsage !== undefined) {
      this.performance.cpuUsage = data.cpuUsage;
    }

    this.eventManager?.emitEvent("workletProcessingStats", {
      workletId,
      stats: data,
      timestamp: performance.now(),
    });
  }

  /**
   * Handle worklet errors
   */
  handleWorkletError(data, workletId, workletName) {
    console.error(
      `Worklet error in ${workletName} (${workletId}):`,
      data.error
    );

    this.eventManager?.emitEvent("workletError", {
      workletId,
      workletName,
      error: data.error,
      timestamp: performance.now(),
    });

    // Consider restarting the worklet if it's critical
    if (data.critical) {
      this.restartWorklet(workletId);
    }
  }

  /**
   * Handle parameter changes
   */
  handleParameterChange(data, workletId) {
    this.eventManager?.emitEvent("workletParameterChange", {
      workletId,
      parameter: data.parameter,
      value: data.value,
      timestamp: performance.now(),
    });
  }

  /**
   * Handle ScriptProcessor audio processing
   */
  handleScriptProcessorAudio(event, processorId, workletName, options) {
    const processorInfo = this.scriptProcessors.get(processorId);
    if (!processorInfo) return;

    try {
      processorInfo.lastActivity = performance.now();
      processorInfo.processCount++;

      const inputBuffer = event.inputBuffer;
      const outputBuffer = event.outputBuffer;

      // Apply basic processing based on worklet type
      switch (workletName) {
        case "audio-processor":
          this.processAudioWithScriptProcessor(
            inputBuffer,
            outputBuffer,
            options
          );
          break;

        case "level-monitor":
          this.processLevelMonitoringWithScriptProcessor(
            inputBuffer,
            outputBuffer,
            processorId
          );
          break;

        default:
          // Pass-through processing
          this.passthroughProcessing(inputBuffer, outputBuffer);
      }
    } catch (error) {
      console.error("ScriptProcessor audio processing error:", error);
    }
  }

  /**
   * Process audio with ScriptProcessor (basic implementation)
   */
  processAudioWithScriptProcessor(inputBuffer, outputBuffer, options) {
    const channels = Math.min(
      inputBuffer.numberOfChannels,
      outputBuffer.numberOfChannels
    );

    for (let channel = 0; channel < channels; channel++) {
      const inputData = inputBuffer.getChannelData(channel);
      const outputData = outputBuffer.getChannelData(channel);

      // Apply basic gain control
      const gain = options.processorOptions?.gain || 1.0;

      for (let i = 0; i < inputData.length; i++) {
        outputData[i] = inputData[i] * gain;
      }
    }
  }

  /**
   * Process level monitoring with ScriptProcessor
   */
  processLevelMonitoringWithScriptProcessor(
    inputBuffer,
    outputBuffer,
    processorId
  ) {
    const inputData = inputBuffer.getChannelData(0);
    const outputData = outputBuffer.getChannelData(0);

    // Calculate level metrics
    let peak = 0;
    let rmsSum = 0;

    for (let i = 0; i < inputData.length; i++) {
      const sample = Math.abs(inputData[i]);
      peak = Math.max(peak, sample);
      rmsSum += sample * sample;

      // Pass-through
      outputData[i] = inputData[i];
    }

    const rms = Math.sqrt(rmsSum / inputData.length);

    // Emit level update
    this.eventManager?.emitEvent("workletLevelUpdate", {
      workletId: processorId,
      level: 20 * Math.log10(Math.max(rms, 1e-10)),
      peak: 20 * Math.log10(Math.max(peak, 1e-10)),
      rms: 20 * Math.log10(Math.max(rms, 1e-10)),
      timestamp: performance.now(),
    });
  }

  /**
   * Pass-through processing (no modification)
   */
  passthroughProcessing(inputBuffer, outputBuffer) {
    const channels = Math.min(
      inputBuffer.numberOfChannels,
      outputBuffer.numberOfChannels
    );

    for (let channel = 0; channel < channels; channel++) {
      const inputData = inputBuffer.getChannelData(channel);
      const outputData = outputBuffer.getChannelData(channel);

      for (let i = 0; i < inputData.length; i++) {
        outputData[i] = inputData[i];
      }
    }
  }

  /**
   * Setup parameter automation for a worklet
   */
  setupParameterAutomation(workletNode, parameterDefs) {
    for (const [paramName, paramDef] of Object.entries(parameterDefs)) {
      const param = workletNode.parameters.get(paramName);
      if (param) {
        // Set default value
        param.setValueAtTime(
          paramDef.defaultValue,
          this.audioContext.currentTime
        );

        // Store parameter reference for later use
        if (!workletNode._parameterRefs) {
          workletNode._parameterRefs = new Map();
        }
        workletNode._parameterRefs.set(paramName, param);
      }
    }
  }

  /**
   * Update worklet parameter
   */
  updateWorkletParameter(workletId, parameterName, value, whenTime = null) {
    const workletInfo = this.activeWorklets.get(workletId);
    if (!workletInfo) {
      console.error(`Worklet not found: ${workletId}`);
      return false;
    }

    const param = workletInfo.node.parameters.get(parameterName);
    if (!param) {
      console.error(`Parameter not found: ${parameterName}`);
      return false;
    }

    const time = whenTime || this.audioContext.currentTime;

    try {
      param.setValueAtTime(value, time);

      this.eventManager?.emitEvent("workletParameterUpdated", {
        workletId,
        parameterName,
        value,
        time,
        timestamp: performance.now(),
      });

      return true;
    } catch (error) {
      console.error("Failed to update worklet parameter:", error);
      return false;
    }
  }

  /**
   * Send message to worklet
   */
  sendMessageToWorklet(workletId, message) {
    const workletInfo = this.activeWorklets.get(workletId);
    if (!workletInfo) {
      console.error(`Worklet not found: ${workletId}`);
      return false;
    }

    try {
      workletInfo.node.port.postMessage(message);
      return true;
    } catch (error) {
      console.error("Failed to send message to worklet:", error);
      return false;
    }
  }

  /**
   * Register custom message handler
   */
  registerMessageHandler(messageType, handler) {
    this.messageHandlers.set(messageType, handler);
  }

  /**
   * Unregister message handler
   */
  unregisterMessageHandler(messageType) {
    return this.messageHandlers.delete(messageType);
  }

  /**
   * Add worklet to routing graph
   */
  addToRoutingGraph(nodeId, audioNode) {
    this.routingGraph.nodes.set(nodeId, {
      audioNode,
      connections: {
        inputs: [],
        outputs: [],
      },
    });
  }

  /**
   * Connect worklets in routing graph
   */
  connectWorklets(sourceId, destinationId, outputIndex = 0, inputIndex = 0) {
    const sourceNode = this.routingGraph.nodes.get(sourceId);
    const destNode = this.routingGraph.nodes.get(destinationId);

    if (!sourceNode || !destNode) {
      console.error("Invalid worklet connection: nodes not found");
      return false;
    }

    try {
      sourceNode.audioNode.connect(destNode.audioNode, outputIndex, inputIndex);

      // Update routing graph
      sourceNode.connections.outputs.push({
        nodeId: destinationId,
        inputIndex,
      });
      destNode.connections.inputs.push({ nodeId: sourceId, outputIndex });

      this.routingGraph.connections.push({
        source: sourceId,
        destination: destinationId,
        outputIndex,
        inputIndex,
      });

      this.eventManager?.emitEvent("workletsConnected", {
        sourceId,
        destinationId,
        outputIndex,
        inputIndex,
        timestamp: performance.now(),
      });

      return true;
    } catch (error) {
      console.error("Failed to connect worklets:", error);
      return false;
    }
  }

  /**
   * Disconnect worklets
   */
  disconnectWorklets(sourceId, destinationId = null) {
    const sourceNode = this.routingGraph.nodes.get(sourceId);
    if (!sourceNode) {
      console.error("Source worklet not found");
      return false;
    }

    try {
      if (destinationId) {
        const destNode = this.routingGraph.nodes.get(destinationId);
        if (destNode) {
          sourceNode.audioNode.disconnect(destNode.audioNode);
        }
      } else {
        sourceNode.audioNode.disconnect();
      }

      // Update routing graph connections
      this.updateRoutingGraphConnections(sourceId, destinationId);

      this.eventManager?.emitEvent("workletsDisconnected", {
        sourceId,
        destinationId,
        timestamp: performance.now(),
      });

      return true;
    } catch (error) {
      console.error("Failed to disconnect worklets:", error);
      return false;
    }
  }

  /**
   * Update routing graph connections after disconnect
   */
  updateRoutingGraphConnections(sourceId, destinationId) {
    if (destinationId) {
      // Remove specific connection
      this.routingGraph.connections = this.routingGraph.connections.filter(
        (conn) =>
          !(conn.source === sourceId && conn.destination === destinationId)
      );
    } else {
      // Remove all connections from source
      this.routingGraph.connections = this.routingGraph.connections.filter(
        (conn) => conn.source !== sourceId
      );
    }
  }

  /**
   * Restart a worklet
   */
  async restartWorklet(workletId) {
    const workletInfo = this.activeWorklets.get(workletId);
    if (!workletInfo) {
      console.error(`Cannot restart: worklet not found: ${workletId}`);
      return false;
    }

    try {
      const { workletName, options } = workletInfo;

      // Disconnect and remove old worklet
      await this.removeWorklet(workletId);

      // Create new worklet with same options
      const result = await this.setupAudioWorklets(workletName, options);

      this.eventManager?.emitEvent("workletRestarted", {
        oldWorkletId: workletId,
        newWorkletId: result.workletId,
        workletName,
        timestamp: performance.now(),
      });

      return result;
    } catch (error) {
      console.error("Failed to restart worklet:", error);
      return false;
    }
  }

  /**
   * Remove a worklet
   */
  async removeWorklet(workletId) {
    // Check AudioWorklet
    const workletInfo = this.activeWorklets.get(workletId);
    if (workletInfo) {
      try {
        workletInfo.node.disconnect();
        this.activeWorklets.delete(workletId);
        this.routingGraph.nodes.delete(workletId);
        this.performance.activeNodes--;

        this.eventManager?.emitEvent("workletRemoved", {
          workletId,
          type: "AudioWorklet",
          timestamp: performance.now(),
        });

        return true;
      } catch (error) {
        console.error("Error removing AudioWorklet:", error);
      }
    }

    // Check ScriptProcessor
    const processorInfo = this.scriptProcessors.get(workletId);
    if (processorInfo) {
      try {
        processorInfo.node.disconnect();
        this.scriptProcessors.delete(workletId);
        this.routingGraph.nodes.delete(workletId);
        this.performance.activeNodes--;

        this.eventManager?.emitEvent("workletRemoved", {
          workletId,
          type: "ScriptProcessor",
          timestamp: performance.now(),
        });

        return true;
      } catch (error) {
        console.error("Error removing ScriptProcessor:", error);
      }
    }

    return false;
  }

  /**
   * Update performance metrics
   */
  updatePerformanceMetrics() {
    // Calculate processing load
    const totalNodes = this.performance.activeNodes;
    this.performance.processingLoad =
      totalNodes / this.config.maxConcurrentWorklets;

    // Estimate memory usage
    let memoryUsage = 0;
    memoryUsage += this.activeWorklets.size * 1024 * 1024; // ~1MB per worklet estimate
    memoryUsage += this.scriptProcessors.size * 512 * 1024; // ~512KB per script processor
    memoryUsage += this.messageQueue.length * 1024; // ~1KB per message

    this.performance.memoryUsage = memoryUsage;

    // Clean up inactive worklets
    this.cleanupInactiveWorklets();
  }

  /**
   * Clean up inactive worklets
   */
  cleanupInactiveWorklets() {
    const now = performance.now();
    const inactivityThreshold = 300000; // 5 minutes

    // Check AudioWorklets
    for (const [workletId, workletInfo] of this.activeWorklets) {
      if (now - workletInfo.lastActivity > inactivityThreshold) {
        console.log(`Cleaning up inactive worklet: ${workletId}`);
        this.removeWorklet(workletId);
      }
    }

    // Check ScriptProcessors
    for (const [processorId, processorInfo] of this.scriptProcessors) {
      if (now - processorInfo.lastActivity > inactivityThreshold) {
        console.log(`Cleaning up inactive script processor: ${processorId}`);
        this.removeWorklet(processorId);
      }
    }
  }

  /**
   * Generate unique worklet ID
   */
  generateWorkletId(workletName) {
    return `worklet_${workletName}_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;
  }

  /**
   * Generate unique script processor ID
   */
  generateScriptProcessorId(workletName) {
    return `script_${workletName}_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;
  }

  /**
   * Get worklet status
   */
  getWorkletStatus(workletId) {
    const workletInfo = this.activeWorklets.get(workletId);
    if (workletInfo) {
      return {
        id: workletId,
        type: "AudioWorklet",
        workletName: workletInfo.workletName,
        createdAt: workletInfo.createdAt,
        lastActivity: workletInfo.lastActivity,
        messageCount: workletInfo.messageCount,
        isActive: performance.now() - workletInfo.lastActivity < 10000, // 10 seconds
      };
    }

    const processorInfo = this.scriptProcessors.get(workletId);
    if (processorInfo) {
      return {
        id: workletId,
        type: "ScriptProcessor",
        workletName: processorInfo.workletName,
        createdAt: processorInfo.createdAt,
        lastActivity: processorInfo.lastActivity,
        processCount: processorInfo.processCount,
        isActive: performance.now() - processorInfo.lastActivity < 10000,
      };
    }

    return null;
  }

  /**
   * Get all active worklets
   */
  getActiveWorklets() {
    const worklets = [];

    // Add AudioWorklets
    for (const [workletId] of this.activeWorklets) {
      const status = this.getWorkletStatus(workletId);
      if (status) worklets.push(status);
    }

    // Add ScriptProcessors
    for (const [processorId] of this.scriptProcessors) {
      const status = this.getWorkletStatus(processorId);
      if (status) worklets.push(status);
    }

    return worklets;
  }

  /**
   * Get performance statistics
   */
  getPerformanceStats() {
    return { ...this.performance };
  }

  /**
   * Get routing graph
   */
  getRoutingGraph() {
    return {
      nodes: Array.from(this.routingGraph.nodes.keys()),
      connections: [...this.routingGraph.connections],
    };
  }

  /**
   * Get available worklet types
   */
  getAvailableWorklets() {
    return Object.keys(this.availableWorklets).map((key) => ({
      name: key,
      ...this.availableWorklets[key],
    }));
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    const oldConfig = { ...this.config };
    this.config = { ...this.config, ...newConfig };

    this.eventManager?.emitEvent("audioWorkletManagerConfigUpdated", {
      oldConfig,
      newConfig: this.config,
      timestamp: performance.now(),
    });
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    // Stop message processing
    this.messageProcessingActive = false;

    // Clear intervals
    if (this.messageInterval) {
      clearInterval(this.messageInterval);
    }

    if (this.performanceInterval) {
      clearInterval(this.performanceInterval);
    }

    // Remove all worklets
    for (const workletId of this.activeWorklets.keys()) {
      this.removeWorklet(workletId);
    }

    for (const processorId of this.scriptProcessors.keys()) {
      this.removeWorklet(processorId);
    }

    // Clear message queue
    this.messageQueue = [];

    // Clear handlers
    this.messageHandlers.clear();

    // Reset state
    this.isInitialized = false;

    this.eventManager?.emitEvent("audioWorkletManagerCleanup", {
      timestamp: performance.now(),
    });
  }
}

export default AudioWorkletManager;
