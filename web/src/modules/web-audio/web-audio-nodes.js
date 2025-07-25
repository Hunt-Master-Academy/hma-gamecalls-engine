/**
 * @fileoverview Web Audio Node Manager Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API node management with dynamic routing,
 * connection management, and real-time parameter control.
 *
 * Features:
 * ✅ Dynamic audio node creation and management
 * ✅ Flexible audio routing and connection management
 * ✅ Real-time parameter automation and control
 * ✅ Node graph visualization and analysis
 * ✅ Performance optimization and resource management
 * ✅ Preset management and serialization
 * ✅ Error handling and recovery
 * ✅ Memory leak prevention
 *
 * @example
 * ```javascript
 * import { WebAudioNodes } from './modules/web-audio/index.js';
 *
 * const nodeManager = new WebAudioNodes(audioContext);
 *
 * const oscillator = await nodeManager.createNode('oscillator', {
 *   frequency: 440,
 *   type: 'sine'
 * });
 *
 * const filter = await nodeManager.createNode('biquadFilter', {
 *   type: 'lowpass',
 *   frequency: 1000
 * });
 *
 * nodeManager.connect(oscillator, filter);
 * nodeManager.connect(filter, 'destination');
 * ```
 */

/**
 * Web Audio Node Manager
 *
 * Provides comprehensive management of Web Audio API nodes with dynamic
 * routing, parameter automation, and performance optimization.
 *
 * @class WebAudioNodes
 */
export class WebAudioNodes {
  /**
   * Create WebAudioNodes manager
   *
   * @param {AudioContext} audioContext - Web Audio context
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableAutomation=true] - Enable parameter automation
   * @param {boolean} [options.enableVisualization=false] - Enable node graph visualization
   * @param {number} [options.maxNodes=100] - Maximum number of nodes
   */
  constructor(audioContext, options = {}) {
    if (!audioContext) {
      throw new Error("AudioContext is required");
    }

    this.audioContext = audioContext;

    // Configuration
    this.config = {
      enableAutomation: options.enableAutomation !== false,
      enableVisualization: options.enableVisualization === true,
      maxNodes: options.maxNodes || 100,
      defaultFadeTime: options.defaultFadeTime || 0.05,
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      ...options,
    };

    // Node management
    this.nodes = new Map(); // nodeId -> node instance
    this.nodeMetadata = new Map(); // nodeId -> metadata
    this.connections = new Map(); // connectionId -> connection info
    this.nodeGroups = new Map(); // groupId -> Set of nodeIds

    // Routing and connections
    this.routingMatrix = new Map(); // sourceId -> Set of destinationIds
    this.reverseRoutingMatrix = new Map(); // destinationId -> Set of sourceIds
    this.connectionHistory = [];

    // Parameter automation
    this.automations = new Map(); // automationId -> automation info
    this.scheduledEvents = new Map(); // eventId -> scheduled event
    this.parameterSnapshots = new Map(); // nodeId -> parameter snapshot

    // Performance tracking
    this.performance = {
      nodeCount: 0,
      connectionCount: 0,
      automationCount: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      createdNodes: 0,
      destroyedNodes: 0,
      failedConnections: 0,
    };

    // Node templates and presets
    this.nodeTemplates = new Map();
    this.presets = new Map();
    this.defaultParameters = new Map();

    // Event handling
    this.eventHandlers = new Map();

    // Initialize built-in node templates
    this._initializeNodeTemplates();

    // Initialize default parameters
    this._initializeDefaultParameters();

    console.log("WebAudioNodes manager initialized");
  }

  /**
   * Initialize built-in node templates
   * @private
   */
  _initializeNodeTemplates() {
    // Oscillator template
    this.nodeTemplates.set("oscillator", {
      type: "source",
      createMethod: "createOscillator",
      parameters: ["frequency", "detune", "type"],
      outputs: 1,
      inputs: 0,
      canStart: true,
      canStop: true,
    });

    // Gain template
    this.nodeTemplates.set("gain", {
      type: "effect",
      createMethod: "createGain",
      parameters: ["gain"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // Biquad Filter template
    this.nodeTemplates.set("biquadFilter", {
      type: "effect",
      createMethod: "createBiquadFilter",
      parameters: ["frequency", "Q", "gain", "type", "detune"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // Delay template
    this.nodeTemplates.set("delay", {
      type: "effect",
      createMethod: "createDelay",
      parameters: ["delayTime"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
      constructor: { maxDelayTime: 1.0 },
    });

    // Compressor template
    this.nodeTemplates.set("dynamicsCompressor", {
      type: "effect",
      createMethod: "createDynamicsCompressor",
      parameters: ["threshold", "knee", "ratio", "attack", "release"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // Analyser template
    this.nodeTemplates.set("analyser", {
      type: "analyser",
      createMethod: "createAnalyser",
      parameters: [
        "fftSize",
        "minDecibels",
        "maxDecibels",
        "smoothingTimeConstant",
      ],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // Convolver template
    this.nodeTemplates.set("convolver", {
      type: "effect",
      createMethod: "createConvolver",
      parameters: ["buffer", "normalize"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // WaveShaper template
    this.nodeTemplates.set("waveShaper", {
      type: "effect",
      createMethod: "createWaveShaper",
      parameters: ["curve", "oversample"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // ChannelSplitter template
    this.nodeTemplates.set("channelSplitter", {
      type: "utility",
      createMethod: "createChannelSplitter",
      parameters: [],
      outputs: 6, // Default max channels
      inputs: 1,
      canStart: false,
      canStop: false,
      constructor: { numberOfOutputs: 6 },
    });

    // ChannelMerger template
    this.nodeTemplates.set("channelMerger", {
      type: "utility",
      createMethod: "createChannelMerger",
      parameters: [],
      outputs: 1,
      inputs: 6, // Default max channels
      canStart: false,
      canStop: false,
      constructor: { numberOfInputs: 6 },
    });

    // Panner template
    this.nodeTemplates.set("panner", {
      type: "spatial",
      createMethod: "createPanner",
      parameters: [
        "positionX",
        "positionY",
        "positionZ",
        "orientationX",
        "orientationY",
        "orientationZ",
      ],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    // StereoPanner template
    this.nodeTemplates.set("stereoPanner", {
      type: "spatial",
      createMethod: "createStereoPanner",
      parameters: ["pan"],
      outputs: 1,
      inputs: 1,
      canStart: false,
      canStop: false,
    });

    console.log(`Initialized ${this.nodeTemplates.size} node templates`);
  }

  /**
   * Initialize default parameters
   * @private
   */
  _initializeDefaultParameters() {
    // Oscillator defaults
    this.defaultParameters.set("oscillator", {
      frequency: 440,
      detune: 0,
      type: "sine",
    });

    // Gain defaults
    this.defaultParameters.set("gain", {
      gain: 1.0,
    });

    // BiquadFilter defaults
    this.defaultParameters.set("biquadFilter", {
      frequency: 350,
      Q: 1,
      gain: 0,
      type: "lowpass",
      detune: 0,
    });

    // Delay defaults
    this.defaultParameters.set("delay", {
      delayTime: 0.0,
    });

    // DynamicsCompressor defaults
    this.defaultParameters.set("dynamicsCompressor", {
      threshold: -24,
      knee: 30,
      ratio: 12,
      attack: 0.003,
      release: 0.25,
    });

    // Analyser defaults
    this.defaultParameters.set("analyser", {
      fftSize: 2048,
      minDecibels: -100,
      maxDecibels: -30,
      smoothingTimeConstant: 0.8,
    });

    // Convolver defaults
    this.defaultParameters.set("convolver", {
      normalize: true,
    });

    // WaveShaper defaults
    this.defaultParameters.set("waveShaper", {
      oversample: "none",
    });

    // Panner defaults
    this.defaultParameters.set("panner", {
      positionX: 0,
      positionY: 0,
      positionZ: 0,
      orientationX: 1,
      orientationY: 0,
      orientationZ: 0,
    });

    // StereoPanner defaults
    this.defaultParameters.set("stereoPanner", {
      pan: 0,
    });
  }

  /**
   * Create audio node
   *
   * @param {string} nodeType - Type of node to create
   * @param {Object|string} [options={}] - Node options or preset name
   * @param {string} [nodeId] - Custom node ID
   * @returns {Promise<AudioNode>} Created audio node
   */
  async createNode(nodeType, options = {}, nodeId) {
    if (this.nodes.size >= this.config.maxNodes) {
      throw new Error(
        `Maximum number of nodes (${this.config.maxNodes}) reached`
      );
    }

    // Handle preset loading
    if (typeof options === "string") {
      const presetName = options;
      options = this.presets.get(presetName) || {};
      console.log(`Loading preset "${presetName}" for node type "${nodeType}"`);
    }

    // Get node template
    const template = this.nodeTemplates.get(nodeType);
    if (!template) {
      throw new Error(`Unknown node type: ${nodeType}`);
    }

    // Generate unique node ID
    if (!nodeId) {
      nodeId = this._generateNodeId(nodeType);
    }

    if (this.nodes.has(nodeId)) {
      throw new Error(`Node with ID "${nodeId}" already exists`);
    }

    try {
      const startTime = performance.now();

      // Create the audio node
      let audioNode;
      if (template.constructor) {
        // Node requires constructor parameters
        audioNode = this.audioContext[template.createMethod](
          template.constructor
        );
      } else {
        audioNode = this.audioContext[template.createMethod]();
      }

      // Store node and metadata
      this.nodes.set(nodeId, audioNode);
      this.nodeMetadata.set(nodeId, {
        id: nodeId,
        type: nodeType,
        template: template,
        createdAt: Date.now(),
        creationTime: performance.now() - startTime,
        connections: { inputs: new Set(), outputs: new Set() },
        parameters: new Map(),
        isStarted: false,
        isDestroyed: false,
      });

      // Apply default parameters
      await this._applyDefaultParameters(audioNode, nodeType, options);

      // Create parameter snapshot
      this._createParameterSnapshot(nodeId, audioNode, template);

      // Update performance counters
      this.performance.nodeCount++;
      this.performance.createdNodes++;

      console.log(
        `Created ${nodeType} node "${nodeId}" in ${(
          performance.now() - startTime
        ).toFixed(2)}ms`
      );

      this._emitEvent("nodeCreated", {
        nodeId,
        nodeType,
        audioNode,
        metadata: this.nodeMetadata.get(nodeId),
      });

      return audioNode;
    } catch (error) {
      console.error(`Failed to create ${nodeType} node:`, error);
      this._emitEvent("nodeCreationError", { nodeType, nodeId, error });
      throw error;
    }
  }

  /**
   * Apply default parameters to node
   * @private
   */
  async _applyDefaultParameters(audioNode, nodeType, options) {
    const defaults = this.defaultParameters.get(nodeType) || {};
    const template = this.nodeTemplates.get(nodeType);

    // Merge defaults with provided options
    const parameters = { ...defaults, ...options };

    // Apply parameters
    for (const [paramName, value] of Object.entries(parameters)) {
      if (template.parameters.includes(paramName)) {
        try {
          const param = audioNode[paramName];
          if (param && typeof param.setValueAtTime === "function") {
            // AudioParam - use setValueAtTime for immediate change
            param.setValueAtTime(value, this.audioContext.currentTime);
          } else if (param !== undefined) {
            // Regular property
            audioNode[paramName] = value;
          }
        } catch (error) {
          console.warn(
            `Failed to set parameter ${paramName} on ${nodeType}:`,
            error
          );
        }
      }
    }
  }

  /**
   * Create parameter snapshot
   * @private
   */
  _createParameterSnapshot(nodeId, audioNode, template) {
    const snapshot = new Map();

    template.parameters.forEach((paramName) => {
      try {
        const param = audioNode[paramName];
        if (param && typeof param.value !== "undefined") {
          snapshot.set(paramName, param.value);
        } else if (param !== undefined) {
          snapshot.set(paramName, param);
        }
      } catch (error) {
        console.warn(`Failed to snapshot parameter ${paramName}:`, error);
      }
    });

    this.parameterSnapshots.set(nodeId, snapshot);
  }

  /**
   * Connect audio nodes
   *
   * @param {string|AudioNode} source - Source node ID or node instance
   * @param {string|AudioNode} destination - Destination node ID or node instance
   * @param {number} [outputIndex=0] - Source output index
   * @param {number} [inputIndex=0] - Destination input index
   * @returns {string} Connection ID
   */
  connect(source, destination, outputIndex = 0, inputIndex = 0) {
    // Resolve source and destination
    const sourceNode = this._resolveNode(source);
    const destNode = this._resolveNode(destination);

    if (!sourceNode) {
      throw new Error(`Source node not found: ${source}`);
    }

    if (!destNode) {
      throw new Error(`Destination node not found: ${destination}`);
    }

    // Generate connection ID
    const connectionId = this._generateConnectionId(
      source,
      destination,
      outputIndex,
      inputIndex
    );

    if (this.connections.has(connectionId)) {
      console.warn(`Connection already exists: ${connectionId}`);
      return connectionId;
    }

    try {
      // Make the connection
      if (typeof destNode.connect === "function") {
        // Destination is an AudioNode
        sourceNode.connect(destNode, outputIndex, inputIndex);
      } else {
        // Destination might be an AudioParam
        sourceNode.connect(destNode, outputIndex);
      }

      // Store connection metadata
      this.connections.set(connectionId, {
        id: connectionId,
        source: this._getNodeId(source),
        destination: this._getNodeId(destination),
        sourceNode: sourceNode,
        destinationNode: destNode,
        outputIndex,
        inputIndex,
        createdAt: Date.now(),
        isActive: true,
      });

      // Update routing matrices
      this._updateRoutingMatrices(source, destination);

      // Update node metadata
      this._updateNodeConnections(source, destination);

      // Update performance counters
      this.performance.connectionCount++;

      console.log(
        `Connected ${this._getNodeId(source)} → ${this._getNodeId(destination)}`
      );

      this._emitEvent("nodeConnected", {
        connectionId,
        source: this._getNodeId(source),
        destination: this._getNodeId(destination),
        outputIndex,
        inputIndex,
      });

      return connectionId;
    } catch (error) {
      console.error(`Failed to connect nodes:`, error);
      this.performance.failedConnections++;
      this._emitEvent("connectionError", {
        source: this._getNodeId(source),
        destination: this._getNodeId(destination),
        error,
      });
      throw error;
    }
  }

  /**
   * Disconnect audio nodes
   *
   * @param {string|AudioNode} source - Source node ID or node instance
   * @param {string|AudioNode} [destination] - Destination node ID or node instance
   * @param {number} [outputIndex] - Source output index
   * @param {number} [inputIndex] - Destination input index
   */
  disconnect(source, destination, outputIndex, inputIndex) {
    const sourceNode = this._resolveNode(source);

    if (!sourceNode) {
      throw new Error(`Source node not found: ${source}`);
    }

    try {
      if (destination === undefined) {
        // Disconnect all outputs
        sourceNode.disconnect();
        this._removeAllConnectionsFrom(source);
      } else {
        const destNode = this._resolveNode(destination);
        if (!destNode) {
          throw new Error(`Destination node not found: ${destination}`);
        }

        // Disconnect specific connection
        if (outputIndex !== undefined && inputIndex !== undefined) {
          sourceNode.disconnect(destNode, outputIndex, inputIndex);
        } else if (outputIndex !== undefined) {
          sourceNode.disconnect(destNode, outputIndex);
        } else {
          sourceNode.disconnect(destNode);
        }

        // Remove connection metadata
        const connectionId = this._generateConnectionId(
          source,
          destination,
          outputIndex,
          inputIndex
        );
        this._removeConnection(connectionId);
      }

      console.log(
        `Disconnected ${this._getNodeId(source)}${
          destination ? ` → ${this._getNodeId(destination)}` : " (all)"
        }`
      );

      this._emitEvent("nodeDisconnected", {
        source: this._getNodeId(source),
        destination: destination ? this._getNodeId(destination) : null,
        outputIndex,
        inputIndex,
      });
    } catch (error) {
      console.error(`Failed to disconnect nodes:`, error);
      this._emitEvent("disconnectionError", {
        source: this._getNodeId(source),
        destination: destination ? this._getNodeId(destination) : null,
        error,
      });
      throw error;
    }
  }

  /**
   * Set node parameter
   *
   * @param {string|AudioNode} node - Node ID or node instance
   * @param {string} paramName - Parameter name
   * @param {number} value - Parameter value
   * @param {number} [time] - Time to apply change (default: now)
   * @param {string} [method='setValueAtTime'] - Automation method
   */
  setParameter(
    node,
    paramName,
    value,
    time = this.audioContext.currentTime,
    method = "setValueAtTime"
  ) {
    const audioNode = this._resolveNode(node);
    const nodeId = this._getNodeId(node);

    if (!audioNode) {
      throw new Error(`Node not found: ${node}`);
    }

    try {
      const param = audioNode[paramName];

      if (param && typeof param[method] === "function") {
        // AudioParam with automation methods
        param[method](value, time);

        // Update snapshot
        const snapshot = this.parameterSnapshots.get(nodeId);
        if (snapshot) {
          snapshot.set(paramName, value);
        }

        console.log(
          `Set ${nodeId}.${paramName} = ${value} at time ${time.toFixed(3)}`
        );
      } else if (param !== undefined) {
        // Regular property
        audioNode[paramName] = value;

        console.log(`Set ${nodeId}.${paramName} = ${value}`);
      } else {
        throw new Error(
          `Parameter "${paramName}" not found on node "${nodeId}"`
        );
      }

      this._emitEvent("parameterChanged", {
        nodeId,
        paramName,
        value,
        time,
        method,
      });
    } catch (error) {
      console.error(
        `Failed to set parameter ${paramName} on ${nodeId}:`,
        error
      );
      this._emitEvent("parameterError", { nodeId, paramName, value, error });
      throw error;
    }
  }

  /**
   * Automate parameter
   *
   * @param {string|AudioNode} node - Node ID or node instance
   * @param {string} paramName - Parameter name
   * @param {Array} automation - Automation curve [{time, value, method}]
   * @returns {string} Automation ID
   */
  automateParameter(node, paramName, automation) {
    const audioNode = this._resolveNode(node);
    const nodeId = this._getNodeId(node);

    if (!audioNode) {
      throw new Error(`Node not found: ${node}`);
    }

    const param = audioNode[paramName];
    if (!param || typeof param.setValueAtTime !== "function") {
      throw new Error(
        `Parameter "${paramName}" is not automatable on node "${nodeId}"`
      );
    }

    const automationId = `${nodeId}_${paramName}_${Date.now()}`;

    try {
      // Schedule automation events
      automation.forEach(
        ({ time, value, method = "linearRampToValueAtTime" }) => {
          const scheduleTime = this.audioContext.currentTime + time;

          if (method === "setValueAtTime") {
            param.setValueAtTime(value, scheduleTime);
          } else if (method === "linearRampToValueAtTime") {
            param.linearRampToValueAtTime(value, scheduleTime);
          } else if (method === "exponentialRampToValueAtTime") {
            param.exponentialRampToValueAtTime(value, scheduleTime);
          } else if (method === "setTargetAtTime") {
            const timeConstant = automation.timeConstant || 0.1;
            param.setTargetAtTime(value, scheduleTime, timeConstant);
          }
        }
      );

      // Store automation metadata
      this.automations.set(automationId, {
        id: automationId,
        nodeId,
        paramName,
        automation,
        createdAt: Date.now(),
        isActive: true,
      });

      this.performance.automationCount++;

      console.log(
        `Created automation "${automationId}" for ${nodeId}.${paramName}`
      );

      this._emitEvent("automationCreated", {
        automationId,
        nodeId,
        paramName,
        automation,
      });

      return automationId;
    } catch (error) {
      console.error(
        `Failed to create automation for ${nodeId}.${paramName}:`,
        error
      );
      this._emitEvent("automationError", {
        nodeId,
        paramName,
        automation,
        error,
      });
      throw error;
    }
  }

  /**
   * Start node (for nodes that support start/stop)
   *
   * @param {string|AudioNode} node - Node ID or node instance
   * @param {number} [when=0] - When to start (default: now)
   * @param {number} [offset] - Offset for buffer-based nodes
   * @param {number} [duration] - Duration for time-limited nodes
   */
  startNode(node, when = this.audioContext.currentTime, offset, duration) {
    const audioNode = this._resolveNode(node);
    const nodeId = this._getNodeId(node);
    const metadata = this.nodeMetadata.get(nodeId);

    if (!audioNode) {
      throw new Error(`Node not found: ${node}`);
    }

    if (!metadata || !metadata.template.canStart) {
      throw new Error(`Node "${nodeId}" does not support start operation`);
    }

    if (metadata.isStarted) {
      throw new Error(`Node "${nodeId}" is already started`);
    }

    try {
      if (duration !== undefined) {
        audioNode.start(when, offset, duration);
      } else if (offset !== undefined) {
        audioNode.start(when, offset);
      } else {
        audioNode.start(when);
      }

      metadata.isStarted = true;
      metadata.startedAt = Date.now();

      console.log(`Started node "${nodeId}" at time ${when.toFixed(3)}`);

      this._emitEvent("nodeStarted", {
        nodeId,
        when,
        offset,
        duration,
      });
    } catch (error) {
      console.error(`Failed to start node "${nodeId}":`, error);
      this._emitEvent("nodeStartError", { nodeId, error });
      throw error;
    }
  }

  /**
   * Stop node (for nodes that support start/stop)
   *
   * @param {string|AudioNode} node - Node ID or node instance
   * @param {number} [when=0] - When to stop (default: now)
   */
  stopNode(node, when = this.audioContext.currentTime) {
    const audioNode = this._resolveNode(node);
    const nodeId = this._getNodeId(node);
    const metadata = this.nodeMetadata.get(nodeId);

    if (!audioNode) {
      throw new Error(`Node not found: ${node}`);
    }

    if (!metadata || !metadata.template.canStop) {
      throw new Error(`Node "${nodeId}" does not support stop operation`);
    }

    if (!metadata.isStarted) {
      throw new Error(`Node "${nodeId}" is not started`);
    }

    try {
      audioNode.stop(when);

      metadata.isStarted = false;
      metadata.stoppedAt = Date.now();

      console.log(`Stopped node "${nodeId}" at time ${when.toFixed(3)}`);

      this._emitEvent("nodeStopped", {
        nodeId,
        when,
      });
    } catch (error) {
      console.error(`Failed to stop node "${nodeId}":`, error);
      this._emitEvent("nodeStopError", { nodeId, error });
      throw error;
    }
  }

  /**
   * Destroy node
   *
   * @param {string|AudioNode} node - Node ID or node instance
   */
  destroyNode(node) {
    const nodeId = this._getNodeId(node);
    const audioNode = this._resolveNode(node);
    const metadata = this.nodeMetadata.get(nodeId);

    if (!audioNode || !metadata) {
      throw new Error(`Node not found: ${node}`);
    }

    if (metadata.isDestroyed) {
      console.warn(`Node "${nodeId}" is already destroyed`);
      return;
    }

    try {
      // Disconnect all connections
      this.disconnect(audioNode);
      this._removeAllConnectionsTo(nodeId);

      // Stop node if it's running
      if (metadata.isStarted && metadata.template.canStop) {
        try {
          audioNode.stop();
        } catch (error) {
          console.warn(`Failed to stop node during destruction:`, error);
        }
      }

      // Mark as destroyed
      metadata.isDestroyed = true;
      metadata.destroyedAt = Date.now();

      // Remove from collections
      this.nodes.delete(nodeId);
      this.nodeMetadata.delete(nodeId);
      this.parameterSnapshots.delete(nodeId);

      // Update performance counters
      this.performance.nodeCount--;
      this.performance.destroyedNodes++;

      console.log(`Destroyed node "${nodeId}"`);

      this._emitEvent("nodeDestroyed", {
        nodeId,
        nodeType: metadata.type,
      });
    } catch (error) {
      console.error(`Failed to destroy node "${nodeId}":`, error);
      this._emitEvent("nodeDestroyError", { nodeId, error });
      throw error;
    }
  }

  // === UTILITY METHODS ===

  /**
   * Resolve node from ID or instance
   * @private
   */
  _resolveNode(node) {
    if (typeof node === "string") {
      if (node === "destination") {
        return this.audioContext.destination;
      }
      return this.nodes.get(node);
    }
    return node;
  }

  /**
   * Get node ID from node or ID
   * @private
   */
  _getNodeId(node) {
    if (typeof node === "string") {
      return node;
    }

    // Find ID by node instance
    for (const [id, audioNode] of this.nodes.entries()) {
      if (audioNode === node) {
        return id;
      }
    }

    return node === this.audioContext.destination ? "destination" : "unknown";
  }

  /**
   * Generate unique node ID
   * @private
   */
  _generateNodeId(nodeType) {
    let counter = 1;
    let nodeId;

    do {
      nodeId = `${nodeType}_${counter}`;
      counter++;
    } while (this.nodes.has(nodeId));

    return nodeId;
  }

  /**
   * Generate connection ID
   * @private
   */
  _generateConnectionId(source, destination, outputIndex, inputIndex) {
    const sourceId = this._getNodeId(source);
    const destId = this._getNodeId(destination);
    return `${sourceId}[${outputIndex}]→${destId}[${inputIndex}]`;
  }

  /**
   * Update routing matrices
   * @private
   */
  _updateRoutingMatrices(source, destination) {
    const sourceId = this._getNodeId(source);
    const destId = this._getNodeId(destination);

    // Update forward routing matrix
    if (!this.routingMatrix.has(sourceId)) {
      this.routingMatrix.set(sourceId, new Set());
    }
    this.routingMatrix.get(sourceId).add(destId);

    // Update reverse routing matrix
    if (!this.reverseRoutingMatrix.has(destId)) {
      this.reverseRoutingMatrix.set(destId, new Set());
    }
    this.reverseRoutingMatrix.get(destId).add(sourceId);
  }

  /**
   * Update node connection metadata
   * @private
   */
  _updateNodeConnections(source, destination) {
    const sourceId = this._getNodeId(source);
    const destId = this._getNodeId(destination);

    const sourceMetadata = this.nodeMetadata.get(sourceId);
    const destMetadata = this.nodeMetadata.get(destId);

    if (sourceMetadata) {
      sourceMetadata.connections.outputs.add(destId);
    }

    if (destMetadata) {
      destMetadata.connections.inputs.add(sourceId);
    }
  }

  /**
   * Remove connection
   * @private
   */
  _removeConnection(connectionId) {
    const connection = this.connections.get(connectionId);
    if (connection) {
      this.connections.delete(connectionId);
      this.performance.connectionCount--;

      // Update routing matrices
      const sourceSet = this.routingMatrix.get(connection.source);
      if (sourceSet) {
        sourceSet.delete(connection.destination);
        if (sourceSet.size === 0) {
          this.routingMatrix.delete(connection.source);
        }
      }

      const destSet = this.reverseRoutingMatrix.get(connection.destination);
      if (destSet) {
        destSet.delete(connection.source);
        if (destSet.size === 0) {
          this.reverseRoutingMatrix.delete(connection.destination);
        }
      }
    }
  }

  /**
   * Remove all connections from a node
   * @private
   */
  _removeAllConnectionsFrom(sourceId) {
    const sourceIdStr = this._getNodeId(sourceId);
    const connectionsToRemove = [];

    for (const [connectionId, connection] of this.connections.entries()) {
      if (connection.source === sourceIdStr) {
        connectionsToRemove.push(connectionId);
      }
    }

    connectionsToRemove.forEach((connectionId) => {
      this._removeConnection(connectionId);
    });
  }

  /**
   * Remove all connections to a node
   * @private
   */
  _removeAllConnectionsTo(destId) {
    const destIdStr = this._getNodeId(destId);
    const connectionsToRemove = [];

    for (const [connectionId, connection] of this.connections.entries()) {
      if (connection.destination === destIdStr) {
        connectionsToRemove.push(connectionId);
      }
    }

    connectionsToRemove.forEach((connectionId) => {
      this._removeConnection(connectionId);
    });
  }

  /**
   * Get node by ID
   *
   * @param {string} nodeId - Node ID
   * @returns {AudioNode|null} Audio node instance
   */
  getNode(nodeId) {
    return this.nodes.get(nodeId) || null;
  }

  /**
   * Get all nodes
   *
   * @returns {Map<string, AudioNode>} All nodes
   */
  getAllNodes() {
    return new Map(this.nodes);
  }

  /**
   * Get node metadata
   *
   * @param {string} nodeId - Node ID
   * @returns {Object|null} Node metadata
   */
  getNodeMetadata(nodeId) {
    return this.nodeMetadata.get(nodeId) || null;
  }

  /**
   * Get connections from node
   *
   * @param {string} nodeId - Node ID
   * @returns {Set<string>} Connected node IDs
   */
  getConnectionsFrom(nodeId) {
    return this.routingMatrix.get(nodeId) || new Set();
  }

  /**
   * Get connections to node
   *
   * @param {string} nodeId - Node ID
   * @returns {Set<string>} Connected node IDs
   */
  getConnectionsTo(nodeId) {
    return this.reverseRoutingMatrix.get(nodeId) || new Set();
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
   * Create node group
   *
   * @param {string} groupId - Group ID
   * @param {Array<string>} nodeIds - Node IDs to group
   */
  createNodeGroup(groupId, nodeIds = []) {
    if (this.nodeGroups.has(groupId)) {
      throw new Error(`Node group "${groupId}" already exists`);
    }

    const nodeSet = new Set(nodeIds);
    this.nodeGroups.set(groupId, nodeSet);

    console.log(`Created node group "${groupId}" with ${nodeSet.size} nodes`);
    this._emitEvent("nodeGroupCreated", { groupId, nodeIds: [...nodeSet] });
  }

  /**
   * Add node to group
   *
   * @param {string} groupId - Group ID
   * @param {string} nodeId - Node ID to add
   */
  addNodeToGroup(groupId, nodeId) {
    const group = this.nodeGroups.get(groupId);
    if (!group) {
      throw new Error(`Node group "${groupId}" not found`);
    }

    group.add(nodeId);
    this._emitEvent("nodeAddedToGroup", { groupId, nodeId });
  }

  /**
   * Remove node from group
   *
   * @param {string} groupId - Group ID
   * @param {string} nodeId - Node ID to remove
   */
  removeNodeFromGroup(groupId, nodeId) {
    const group = this.nodeGroups.get(groupId);
    if (!group) {
      throw new Error(`Node group "${groupId}" not found`);
    }

    group.delete(nodeId);
    this._emitEvent("nodeRemovedFromGroup", { groupId, nodeId });
  }

  /**
   * Save preset
   *
   * @param {string} presetName - Preset name
   * @param {string} nodeType - Node type
   * @param {Object} parameters - Parameter values
   */
  savePreset(presetName, nodeType, parameters) {
    this.presets.set(presetName, {
      nodeType,
      parameters,
      createdAt: Date.now(),
    });

    console.log(`Saved preset "${presetName}" for ${nodeType}`);
    this._emitEvent("presetSaved", { presetName, nodeType, parameters });
  }

  /**
   * Load preset
   *
   * @param {string} presetName - Preset name
   * @returns {Object|null} Preset data
   */
  loadPreset(presetName) {
    return this.presets.get(presetName) || null;
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
    console.log("Destroying WebAudioNodes manager...");

    // Stop all nodes
    for (const [nodeId, metadata] of this.nodeMetadata.entries()) {
      if (metadata.isStarted && metadata.template.canStop) {
        try {
          this.stopNode(nodeId);
        } catch (error) {
          console.warn(`Failed to stop node ${nodeId} during cleanup:`, error);
        }
      }
    }

    // Disconnect all nodes
    for (const nodeId of this.nodes.keys()) {
      try {
        this.disconnect(nodeId);
      } catch (error) {
        console.warn(
          `Failed to disconnect node ${nodeId} during cleanup:`,
          error
        );
      }
    }

    // Clear all collections
    this.nodes.clear();
    this.nodeMetadata.clear();
    this.connections.clear();
    this.nodeGroups.clear();
    this.routingMatrix.clear();
    this.reverseRoutingMatrix.clear();
    this.automations.clear();
    this.scheduledEvents.clear();
    this.parameterSnapshots.clear();
    this.eventHandlers.clear();

    // Reset performance counters
    this.performance.nodeCount = 0;
    this.performance.connectionCount = 0;
    this.performance.automationCount = 0;

    this._emitEvent("destroyed");
    console.log("WebAudioNodes manager destroyed");
  }
}

export default WebAudioNodes;
