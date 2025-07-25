/**
 * @file audio-context-manager.js
 * @brief Comprehensive Web Audio API context and lifecycle management
 *
 * This module provides centralized management of Web Audio API contexts including:
 * - AudioContext creation and lifecycle management
 * - Cross-browser compatibility handling
 * - Audio routing and node management
 * - Performance optimization and resource cleanup
 * - State synchronization and event handling
 */

/**
 * Audio context states
 */
const CONTEXT_STATES = {
  SUSPENDED: "suspended",
  RUNNING: "running",
  CLOSED: "closed",
  INTERRUPTED: "interrupted",
};

/**
 * Audio context configuration presets
 */
const CONTEXT_PRESETS = {
  LOW_LATENCY: {
    latencyHint: "interactive",
    sampleRate: 44100,
  },
  BALANCED: {
    latencyHint: "balanced",
    sampleRate: 44100,
  },
  PLAYBACK: {
    latencyHint: "playback",
    sampleRate: 44100,
  },
  HIGH_QUALITY: {
    latencyHint: "playback",
    sampleRate: 48000,
  },
};

/**
 * Node types for automatic management
 */
const NODE_TYPES = {
  SOURCE: "source",
  EFFECT: "effect",
  DESTINATION: "destination",
  ANALYZER: "analyzer",
  PROCESSOR: "processor",
};

/**
 * Centralized Web Audio API context manager
 */
export class AudioContextManager {
  constructor(options = {}) {
    this.options = {
      // Context configuration
      preset: options.preset || "BALANCED",
      contextOptions: {
        ...CONTEXT_PRESETS[options.preset || "BALANCED"],
        ...options.contextOptions,
      },

      // Auto-resume behavior
      autoResume: options.autoResume !== false,
      resumeOnUserGesture: options.resumeOnUserGesture !== false,

      // Resource management
      enableNodeTracking: options.enableNodeTracking !== false,
      autoCleanup: options.autoCleanup !== false,
      cleanupInterval: options.cleanupInterval || 30000, // 30 seconds
      maxInactiveTime: options.maxInactiveTime || 300000, // 5 minutes

      // Performance monitoring
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      performanceUpdateInterval: options.performanceUpdateInterval || 1000,

      // Event handling
      eventManager: options.eventManager || null,

      // Debug options
      debugMode: options.debugMode || false,
      logStateChanges: options.logStateChanges || false,

      ...options,
    };

    // Context state
    this.context = null;
    this.state = CONTEXT_STATES.SUSPENDED;
    this.previousState = null;
    this.isInitialized = false;

    // Node tracking
    this.nodes = new Map();
    this.nodeConnections = new Map();
    this.nodeMetadata = new Map();

    // Performance monitoring
    this.performanceMetrics = {
      currentTime: 0,
      sampleRate: 0,
      baseLatency: 0,
      outputLatency: 0,
      state: CONTEXT_STATES.SUSPENDED,
      activeNodes: 0,
      audioWorkletSupported: false,
      memoryUsage: 0,
      cpuUsage: 0,
    };

    // Cleanup tracking
    this.cleanupTimer = null;
    this.performanceTimer = null;
    this.userGestureListeners = [];

    // Event tracking
    this.eventHandlers = new Map();

    // Browser compatibility
    this.audioContextClass = this._getAudioContextClass();
    this.isSupported = !!this.audioContextClass;

    if (this.options.autoResume && this.isSupported) {
      this._setupUserGestureListeners();
    }
  }

  /**
   * Initialize the audio context
   */
  async initialize() {
    if (this.isInitialized) {
      return this.context;
    }

    try {
      if (!this.isSupported) {
        throw new Error("Web Audio API is not supported in this browser");
      }

      // Create audio context with options
      this.context = new this.audioContextClass(this.options.contextOptions);

      // Set up event listeners
      this._setupContextEventListeners();

      // Initialize performance monitoring
      if (this.options.enablePerformanceMonitoring) {
        this._startPerformanceMonitoring();
      }

      // Initialize cleanup system
      if (this.options.autoCleanup) {
        this._startCleanupTimer();
      }

      // Update state
      this._updateState();
      this.isInitialized = true;

      // Emit initialization event
      this._emitEvent("contextInitialized", {
        sampleRate: this.context.sampleRate,
        baseLatency: this.context.baseLatency || 0,
        outputLatency: this.context.outputLatency || 0,
        state: this.context.state,
      });

      // Auto-resume if configured
      if (this.options.autoResume && this.context.state === "suspended") {
        await this.resume();
      }

      return this.context;
    } catch (error) {
      console.error("AudioContext initialization failed:", error);
      this._emitEvent("contextError", { error: error.message });
      throw error;
    }
  }

  /**
   * Resume the audio context
   */
  async resume() {
    if (!this.context) {
      throw new Error("AudioContext not initialized. Call initialize() first.");
    }

    if (this.context.state === "running") {
      return this.context;
    }

    try {
      await this.context.resume();
      this._updateState();

      this._emitEvent("contextResumed", {
        previousState: this.previousState,
        currentState: this.state,
      });

      return this.context;
    } catch (error) {
      console.error("Failed to resume AudioContext:", error);
      this._emitEvent("contextError", {
        error: error.message,
        operation: "resume",
      });
      throw error;
    }
  }

  /**
   * Suspend the audio context
   */
  async suspend() {
    if (!this.context) {
      throw new Error("AudioContext not initialized. Call initialize() first.");
    }

    if (this.context.state === "suspended") {
      return this.context;
    }

    try {
      await this.context.suspend();
      this._updateState();

      this._emitEvent("contextSuspended", {
        previousState: this.previousState,
        currentState: this.state,
      });

      return this.context;
    } catch (error) {
      console.error("Failed to suspend AudioContext:", error);
      this._emitEvent("contextError", {
        error: error.message,
        operation: "suspend",
      });
      throw error;
    }
  }

  /**
   * Close the audio context and clean up resources
   */
  async close() {
    if (!this.context) {
      return;
    }

    try {
      // Stop monitoring and cleanup timers
      this._stopPerformanceMonitoring();
      this._stopCleanupTimer();

      // Clean up all tracked nodes
      await this._cleanupAllNodes();

      // Remove event listeners
      this._removeUserGestureListeners();
      this._removeContextEventListeners();

      // Close the context
      if (this.context.state !== "closed") {
        await this.context.close();
      }

      this._updateState();

      this._emitEvent("contextClosed", {
        previousState: this.previousState,
        finalNodeCount: this.nodes.size,
      });

      // Reset state
      this.context = null;
      this.isInitialized = false;
      this.nodes.clear();
      this.nodeConnections.clear();
      this.nodeMetadata.clear();
    } catch (error) {
      console.error("Failed to close AudioContext:", error);
      this._emitEvent("contextError", {
        error: error.message,
        operation: "close",
      });
      throw error;
    }
  }

  /**
   * Create and register an audio node
   */
  createNode(nodeType, creationFunction, options = {}) {
    if (!this.context) {
      throw new Error("AudioContext not initialized. Call initialize() first.");
    }

    try {
      // Create the node
      const node = creationFunction(this.context);

      // Generate unique ID
      const nodeId = options.id || this._generateNodeId(nodeType);

      // Register node for tracking
      if (this.options.enableNodeTracking) {
        this._registerNode(nodeId, node, nodeType, options);
      }

      this._emitEvent("nodeCreated", {
        nodeId: nodeId,
        nodeType: nodeType,
        options: options,
      });

      return { node, nodeId };
    } catch (error) {
      console.error("Failed to create audio node:", error);
      this._emitEvent("nodeError", {
        error: error.message,
        operation: "create",
      });
      throw error;
    }
  }

  /**
   * Connect two audio nodes and track the connection
   */
  connectNodes(
    sourceNodeId,
    destinationNodeId,
    sourceOutput = 0,
    destinationInput = 0
  ) {
    const sourceNode = this.getNode(sourceNodeId);
    const destinationNode = this.getNode(destinationNodeId);

    if (!sourceNode || !destinationNode) {
      throw new Error("Both source and destination nodes must be registered");
    }

    try {
      // Make the connection
      if (
        typeof sourceOutput === "number" &&
        typeof destinationInput === "number"
      ) {
        sourceNode.connect(destinationNode, sourceOutput, destinationInput);
      } else {
        sourceNode.connect(destinationNode);
      }

      // Track the connection
      const connectionId = `${sourceNodeId}->${destinationNodeId}`;
      this.nodeConnections.set(connectionId, {
        source: sourceNodeId,
        destination: destinationNodeId,
        sourceOutput: sourceOutput,
        destinationInput: destinationInput,
        timestamp: Date.now(),
      });

      this._emitEvent("nodesConnected", {
        connectionId: connectionId,
        source: sourceNodeId,
        destination: destinationNodeId,
      });

      return connectionId;
    } catch (error) {
      console.error("Failed to connect audio nodes:", error);
      this._emitEvent("nodeError", {
        error: error.message,
        operation: "connect",
      });
      throw error;
    }
  }

  /**
   * Disconnect audio nodes
   */
  disconnectNodes(sourceNodeId, destinationNodeId = null) {
    const sourceNode = this.getNode(sourceNodeId);

    if (!sourceNode) {
      throw new Error("Source node not found");
    }

    try {
      if (destinationNodeId) {
        const destinationNode = this.getNode(destinationNodeId);
        if (destinationNode) {
          sourceNode.disconnect(destinationNode);
        }

        // Remove specific connection tracking
        const connectionId = `${sourceNodeId}->${destinationNodeId}`;
        this.nodeConnections.delete(connectionId);
      } else {
        // Disconnect all outputs
        sourceNode.disconnect();

        // Remove all connections from this source
        for (const [connectionId, connection] of this.nodeConnections) {
          if (connection.source === sourceNodeId) {
            this.nodeConnections.delete(connectionId);
          }
        }
      }

      this._emitEvent("nodesDisconnected", {
        source: sourceNodeId,
        destination: destinationNodeId,
      });
    } catch (error) {
      console.error("Failed to disconnect audio nodes:", error);
      this._emitEvent("nodeError", {
        error: error.message,
        operation: "disconnect",
      });
      throw error;
    }
  }

  /**
   * Get a registered node by ID
   */
  getNode(nodeId) {
    return this.nodes.get(nodeId);
  }

  /**
   * Get all nodes of a specific type
   */
  getNodesByType(nodeType) {
    const nodesOfType = [];
    for (const [nodeId, node] of this.nodes) {
      const metadata = this.nodeMetadata.get(nodeId);
      if (metadata && metadata.type === nodeType) {
        nodesOfType.push({ nodeId, node, metadata });
      }
    }
    return nodesOfType;
  }

  /**
   * Remove and clean up a node
   */
  removeNode(nodeId) {
    const node = this.nodes.get(nodeId);
    if (!node) {
      return false;
    }

    try {
      // Disconnect the node
      this.disconnectNodes(nodeId);

      // Clean up node-specific resources
      if (typeof node.disconnect === "function") {
        node.disconnect();
      }

      // Remove from tracking
      this.nodes.delete(nodeId);
      this.nodeMetadata.delete(nodeId);

      this._emitEvent("nodeRemoved", { nodeId: nodeId });

      return true;
    } catch (error) {
      console.error("Failed to remove audio node:", error);
      this._emitEvent("nodeError", {
        error: error.message,
        operation: "remove",
      });
      return false;
    }
  }

  /**
   * Get current performance metrics
   */
  getPerformanceMetrics() {
    return { ...this.performanceMetrics };
  }

  /**
   * Get context information
   */
  getContextInfo() {
    if (!this.context) {
      return null;
    }

    return {
      state: this.context.state,
      sampleRate: this.context.sampleRate,
      currentTime: this.context.currentTime,
      baseLatency: this.context.baseLatency || 0,
      outputLatency: this.context.outputLatency || 0,
      audioWorkletSupported: this._isAudioWorkletSupported(),
      nodeCount: this.nodes.size,
      connectionCount: this.nodeConnections.size,
    };
  }

  /**
   * Register event listener
   */
  on(eventName, callback) {
    if (!this.eventHandlers.has(eventName)) {
      this.eventHandlers.set(eventName, new Set());
    }
    this.eventHandlers.get(eventName).add(callback);
  }

  /**
   * Remove event listener
   */
  off(eventName, callback) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).delete(callback);
    }
  }

  /**
   * Get current context state
   */
  getState() {
    return this.state;
  }

  /**
   * Check if context is running
   */
  isRunning() {
    return this.state === CONTEXT_STATES.RUNNING;
  }

  /**
   * Check if context is suspended
   */
  isSuspended() {
    return this.state === CONTEXT_STATES.SUSPENDED;
  }

  /**
   * Check if context is closed
   */
  isClosed() {
    return this.state === CONTEXT_STATES.CLOSED;
  }

  // Private methods

  /**
   * Get the appropriate AudioContext class for the current browser
   */
  _getAudioContextClass() {
    if (typeof AudioContext !== "undefined") {
      return AudioContext;
    }
    if (typeof webkitAudioContext !== "undefined") {
      return webkitAudioContext;
    }
    return null;
  }

  /**
   * Check if AudioWorklet is supported
   */
  _isAudioWorkletSupported() {
    return this.context && typeof this.context.audioWorklet !== "undefined";
  }

  /**
   * Set up context event listeners
   */
  _setupContextEventListeners() {
    if (!this.context) return;

    this.context.addEventListener("statechange", () => {
      this._updateState();
      this._emitEvent("stateChanged", {
        previousState: this.previousState,
        currentState: this.state,
      });
    });
  }

  /**
   * Remove context event listeners
   */
  _removeContextEventListeners() {
    // Event listeners are automatically removed when context is closed
  }

  /**
   * Set up user gesture listeners for auto-resume
   */
  _setupUserGestureListeners() {
    if (!this.options.resumeOnUserGesture) return;

    const gestureEvents = ["click", "touchstart", "keydown"];
    const resumeHandler = async () => {
      if (this.context && this.context.state === "suspended") {
        try {
          await this.resume();
          this._removeUserGestureListeners();
        } catch (error) {
          console.warn("Failed to resume context on user gesture:", error);
        }
      }
    };

    gestureEvents.forEach((eventType) => {
      document.addEventListener(eventType, resumeHandler, { once: true });
      this.userGestureListeners.push({ eventType, handler: resumeHandler });
    });
  }

  /**
   * Remove user gesture listeners
   */
  _removeUserGestureListeners() {
    this.userGestureListeners.forEach(({ eventType, handler }) => {
      document.removeEventListener(eventType, handler);
    });
    this.userGestureListeners = [];
  }

  /**
   * Update internal state tracking
   */
  _updateState() {
    if (!this.context) return;

    this.previousState = this.state;
    this.state = this.context.state;

    if (this.options.logStateChanges) {
      console.log(
        `AudioContext state changed: ${this.previousState} -> ${this.state}`
      );
    }
  }

  /**
   * Register a node for tracking
   */
  _registerNode(nodeId, node, nodeType, options) {
    this.nodes.set(nodeId, node);
    this.nodeMetadata.set(nodeId, {
      type: nodeType,
      created: Date.now(),
      lastAccessed: Date.now(),
      options: options,
    });
  }

  /**
   * Generate unique node ID
   */
  _generateNodeId(nodeType) {
    const timestamp = Date.now();
    const random = Math.random().toString(36).substr(2, 9);
    return `${nodeType}-${timestamp}-${random}`;
  }

  /**
   * Start performance monitoring
   */
  _startPerformanceMonitoring() {
    this.performanceTimer = setInterval(() => {
      this._updatePerformanceMetrics();
    }, this.options.performanceUpdateInterval);
  }

  /**
   * Stop performance monitoring
   */
  _stopPerformanceMonitoring() {
    if (this.performanceTimer) {
      clearInterval(this.performanceTimer);
      this.performanceTimer = null;
    }
  }

  /**
   * Update performance metrics
   */
  _updatePerformanceMetrics() {
    if (!this.context) return;

    this.performanceMetrics = {
      currentTime: this.context.currentTime,
      sampleRate: this.context.sampleRate,
      baseLatency: this.context.baseLatency || 0,
      outputLatency: this.context.outputLatency || 0,
      state: this.context.state,
      activeNodes: this.nodes.size,
      audioWorkletSupported: this._isAudioWorkletSupported(),
      memoryUsage: this._estimateMemoryUsage(),
      cpuUsage: 0, // Would need specialized implementation
    };

    this._emitEvent("performanceUpdate", this.performanceMetrics);
  }

  /**
   * Estimate memory usage (rough approximation)
   */
  _estimateMemoryUsage() {
    // This is a rough estimate based on node count and connections
    const nodeMemory = this.nodes.size * 1024; // ~1KB per node
    const connectionMemory = this.nodeConnections.size * 256; // ~256B per connection
    return nodeMemory + connectionMemory;
  }

  /**
   * Start cleanup timer
   */
  _startCleanupTimer() {
    this.cleanupTimer = setInterval(() => {
      this._performCleanup();
    }, this.options.cleanupInterval);
  }

  /**
   * Stop cleanup timer
   */
  _stopCleanupTimer() {
    if (this.cleanupTimer) {
      clearInterval(this.cleanupTimer);
      this.cleanupTimer = null;
    }
  }

  /**
   * Perform automatic cleanup of inactive nodes
   */
  _performCleanup() {
    const now = Date.now();
    const nodesToRemove = [];

    for (const [nodeId, metadata] of this.nodeMetadata) {
      if (now - metadata.lastAccessed > this.options.maxInactiveTime) {
        nodesToRemove.push(nodeId);
      }
    }

    nodesToRemove.forEach((nodeId) => {
      this.removeNode(nodeId);
    });

    if (nodesToRemove.length > 0) {
      this._emitEvent("automaticCleanup", {
        removedNodes: nodesToRemove.length,
        totalNodes: this.nodes.size,
      });
    }
  }

  /**
   * Clean up all nodes
   */
  async _cleanupAllNodes() {
    const nodeIds = Array.from(this.nodes.keys());

    for (const nodeId of nodeIds) {
      try {
        this.removeNode(nodeId);
      } catch (error) {
        console.warn(`Failed to clean up node ${nodeId}:`, error);
      }
    }
  }

  /**
   * Emit events through event manager or direct handlers
   */
  _emitEvent(eventName, data = {}) {
    // Emit through event manager if available
    if (
      this.options.eventManager &&
      typeof this.options.eventManager.emit === "function"
    ) {
      this.options.eventManager.emit(eventName, {
        source: "AudioContextManager",
        timestamp: Date.now(),
        ...data,
      });
    }

    // Emit through internal handlers
    if (this.eventHandlers.has(eventName)) {
      for (const callback of this.eventHandlers.get(eventName)) {
        try {
          callback({
            type: eventName,
            timestamp: Date.now(),
            ...data,
          });
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      }
    }

    // Also dispatch as DOM event
    const event = new CustomEvent(`audiocontext:${eventName}`, {
      detail: data,
    });
    window.dispatchEvent(event);
  }
}

// Export constants
export { CONTEXT_STATES, CONTEXT_PRESETS, NODE_TYPES };

// Export default
export default AudioContextManager;
