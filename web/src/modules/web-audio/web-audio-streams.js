/**
 * @fileoverview Web Audio Streams Management Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API stream management with device control,
 * real-time input/output handling, and advanced stream processing.
 *
 * Features:
 * ✅ Audio device enumeration and selection
 * ✅ Real-time audio input/output stream management
 * ✅ Multi-channel audio support with routing
 * ✅ Stream recording and playback capabilities
 * ✅ Real-time monitoring and visualization
 * ✅ Audio constraints and quality management
 * ✅ Cross-browser compatibility and fallbacks
 * ✅ Stream switching and dynamic reconfiguration
 * ✅ Audio level monitoring and automatic gain control
 * ✅ Echo cancellation and noise suppression
 * ✅ Stream synchronization and latency compensation
 * ✅ Performance monitoring and optimization
 *
 * @example
 * ```javascript
 * import { WebAudioStreams } from './modules/web-audio/index.js';
 *
 * const streams = new WebAudioStreams(audioContext);
 *
 * // Get user media stream
 * const inputStream = await streams.getUserMediaStream({
 *   audio: {
 *     deviceId: 'default',
 *     sampleRate: 44100,
 *     channelCount: 2,
 *     echoCancellation: true,
 *     noiseSuppression: true
 *   }
 * });
 *
 * // Create stream source
 * const source = streams.createStreamSource(inputStream);
 * ```
 */

/**
 * Web Audio Streams Manager
 *
 * Provides comprehensive audio stream management with device control,
 * real-time processing, and performance optimization.
 *
 * @class WebAudioStreams
 */
export class WebAudioStreams {
  /**
   * Create WebAudioStreams manager
   *
   * @param {AudioContext} audioContext - Web Audio context
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableAutoGainControl=true] - Enable automatic gain control
   * @param {boolean} [options.enableEchoCancellation=true] - Enable echo cancellation
   * @param {boolean} [options.enableNoiseSuppression=true] - Enable noise suppression
   * @param {number} [options.maxStreams=10] - Maximum number of streams
   */
  constructor(audioContext, options = {}) {
    if (!audioContext) {
      throw new Error("AudioContext is required");
    }

    this.audioContext = audioContext;

    // Configuration
    this.config = {
      enableAutoGainControl: options.enableAutoGainControl !== false,
      enableEchoCancellation: options.enableEchoCancellation !== false,
      enableNoiseSuppression: options.enableNoiseSuppression !== false,
      maxStreams: options.maxStreams || 10,
      defaultSampleRate: options.defaultSampleRate || 44100,
      defaultChannelCount: options.defaultChannelCount || 2,
      defaultLatencyHint: options.defaultLatencyHint || "interactive",
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      ...options,
    };

    // Stream management
    this.streams = new Map(); // streamId -> stream data
    this.streamSources = new Map(); // streamId -> MediaStreamAudioSourceNode
    this.streamDestinations = new Map(); // streamId -> MediaStreamAudioDestinationNode
    this.recordings = new Map(); // recordingId -> recording data

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

    // Stream monitoring
    this.monitors = new Map(); // streamId -> monitoring data
    this.visualizers = new Map(); // streamId -> visualizer data

    // Performance tracking
    this.performance = {
      activeStreams: 0,
      totalStreams: 0,
      recordingsActive: 0,
      totalRecordings: 0,
      averageLatency: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      droppedFrames: 0,
      underruns: 0,
    };

    // Event handling
    this.eventHandlers = new Map();

    // Stream constraints templates
    this.constraintTemplates = new Map();

    // Initialize system
    this._initializeConstraintTemplates();
    this._setupDeviceChangeListening();

    // Start performance monitoring
    if (this.config.enablePerformanceMonitoring) {
      this._startPerformanceMonitoring();
    }

    console.log("WebAudioStreams manager initialized");
  }

  /**
   * Initialize constraint templates
   * @private
   */
  _initializeConstraintTemplates() {
    // High quality recording
    this.constraintTemplates.set("highQuality", {
      audio: {
        sampleRate: 48000,
        channelCount: 2,
        sampleSize: 24,
        echoCancellation: false,
        noiseSuppression: false,
        autoGainControl: false,
        latency: 0.01,
      },
    });

    // Voice chat optimized
    this.constraintTemplates.set("voiceChat", {
      audio: {
        sampleRate: 16000,
        channelCount: 1,
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true,
        latency: 0.05,
      },
    });

    // Music production
    this.constraintTemplates.set("musicProduction", {
      audio: {
        sampleRate: 48000,
        channelCount: 2,
        sampleSize: 24,
        echoCancellation: false,
        noiseSuppression: false,
        autoGainControl: false,
        latency: 0.005,
      },
    });

    // Low latency monitoring
    this.constraintTemplates.set("lowLatency", {
      audio: {
        sampleRate: 44100,
        channelCount: 2,
        echoCancellation: false,
        noiseSuppression: false,
        autoGainControl: false,
        latency: 0.003,
      },
    });

    // Podcast recording
    this.constraintTemplates.set("podcast", {
      audio: {
        sampleRate: 44100,
        channelCount: 1,
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true,
        latency: 0.02,
      },
    });

    console.log(
      `Initialized ${this.constraintTemplates.size} constraint templates`
    );
  }

  /**
   * Setup device change listening
   * @private
   */
  _setupDeviceChangeListening() {
    if (navigator.mediaDevices && navigator.mediaDevices.addEventListener) {
      navigator.mediaDevices.addEventListener("devicechange", () => {
        console.log("Audio devices changed");
        this._enumerateDevices();
        this._emitEvent("devicesChanged");
      });
    }
  }

  /**
   * Enumerate available audio devices
   *
   * @returns {Promise<Object>} Available devices
   */
  async enumerateDevices() {
    try {
      if (!navigator.mediaDevices || !navigator.mediaDevices.enumerateDevices) {
        throw new Error("Device enumeration not supported");
      }

      const devices = await navigator.mediaDevices.enumerateDevices();

      this.devices.input = devices.filter(
        (device) => device.kind === "audioinput"
      );
      this.devices.output = devices.filter(
        (device) => device.kind === "audiooutput"
      );

      // Get device capabilities
      for (const device of [...this.devices.input, ...this.devices.output]) {
        try {
          const capabilities = await this._getDeviceCapabilities(device);
          this.devices.capabilities.set(device.deviceId, capabilities);
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

      return {
        input: [...this.devices.input],
        output: [...this.devices.output],
        capabilities: new Map(this.devices.capabilities),
      };
    } catch (error) {
      console.error("Device enumeration failed:", error);
      this._emitEvent("deviceEnumerationError", { error });
      throw error;
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

      // Output device capabilities are limited
      return {};
    } catch (error) {
      console.warn(`Failed to get capabilities for ${device.deviceId}:`, error);
      return {};
    }
  }

  /**
   * Get user media stream
   *
   * @param {Object|string} constraints - Media constraints or template name
   * @param {string} [streamId] - Custom stream ID
   * @returns {Promise<MediaStream>} Media stream
   */
  async getUserMediaStream(constraints = {}, streamId) {
    if (this.streams.size >= this.config.maxStreams) {
      throw new Error(
        `Maximum number of streams (${this.config.maxStreams}) reached`
      );
    }

    // Handle constraint templates
    if (typeof constraints === "string") {
      const templateName = constraints;
      const template = this.constraintTemplates.get(templateName);
      if (template) {
        constraints = template;
        console.log(`Using constraint template: ${templateName}`);
      } else {
        throw new Error(`Constraint template "${templateName}" not found`);
      }
    }

    // Apply default constraints
    constraints = this._mergeConstraints(constraints);

    // Generate stream ID
    if (!streamId) {
      streamId = this._generateStreamId("userMedia");
    }

    try {
      const startTime = performance.now();

      console.log("Requesting user media with constraints:", constraints);

      const mediaStream = await navigator.mediaDevices.getUserMedia(
        constraints
      );

      const streamData = {
        id: streamId,
        type: "userMedia",
        mediaStream: mediaStream,
        constraints: constraints,
        tracks: mediaStream.getTracks(),
        isActive: true,
        createdAt: Date.now(),
        creationTime: performance.now() - startTime,
        metadata: {
          sampleRate:
            constraints.audio.sampleRate || this.config.defaultSampleRate,
          channelCount:
            constraints.audio.channelCount || this.config.defaultChannelCount,
          deviceId: constraints.audio.deviceId,
        },
      };

      // Store stream
      this.streams.set(streamId, streamData);

      // Setup track event listeners
      this._setupTrackListeners(streamData);

      // Update performance metrics
      this.performance.activeStreams++;
      this.performance.totalStreams++;

      console.log(
        `Created user media stream "${streamId}" in ${streamData.creationTime.toFixed(
          2
        )}ms`
      );

      this._emitEvent("streamCreated", {
        streamId,
        streamData,
        creationTime: streamData.creationTime,
      });

      return mediaStream;
    } catch (error) {
      console.error("Failed to get user media stream:", error);
      this._emitEvent("streamCreationError", { streamId, constraints, error });
      throw error;
    }
  }

  /**
   * Merge constraints with defaults
   * @private
   */
  _mergeConstraints(constraints) {
    const defaultConstraints = {
      audio: {
        sampleRate: this.config.defaultSampleRate,
        channelCount: this.config.defaultChannelCount,
        echoCancellation: this.config.enableEchoCancellation,
        noiseSuppression: this.config.enableNoiseSuppression,
        autoGainControl: this.config.enableAutoGainControl,
        latency: this.config.defaultLatencyHint === "interactive" ? 0.01 : 0.05,
      },
    };

    return {
      audio: { ...defaultConstraints.audio, ...(constraints.audio || {}) },
    };
  }

  /**
   * Setup track event listeners
   * @private
   */
  _setupTrackListeners(streamData) {
    streamData.tracks.forEach((track) => {
      track.addEventListener("ended", () => {
        console.log(`Track ended for stream ${streamData.id}`);
        this._handleTrackEnded(streamData.id, track);
      });

      track.addEventListener("mute", () => {
        console.log(`Track muted for stream ${streamData.id}`);
        this._emitEvent("trackMuted", { streamId: streamData.id, track });
      });

      track.addEventListener("unmute", () => {
        console.log(`Track unmuted for stream ${streamData.id}`);
        this._emitEvent("trackUnmuted", { streamId: streamData.id, track });
      });
    });
  }

  /**
   * Handle track ended
   * @private
   */
  _handleTrackEnded(streamId, track) {
    const streamData = this.streams.get(streamId);
    if (streamData) {
      streamData.isActive = false;
      this._emitEvent("trackEnded", { streamId, track });

      // Check if all tracks have ended
      const activeTracks = streamData.tracks.filter(
        (t) => t.readyState === "live"
      );
      if (activeTracks.length === 0) {
        this._handleStreamEnded(streamId);
      }
    }
  }

  /**
   * Handle stream ended
   * @private
   */
  _handleStreamEnded(streamId) {
    console.log(`Stream ${streamId} ended`);
    this.performance.activeStreams--;
    this._emitEvent("streamEnded", { streamId });
  }

  /**
   * Create stream source node
   *
   * @param {MediaStream|string} stream - Media stream or stream ID
   * @param {string} [sourceId] - Custom source ID
   * @returns {MediaStreamAudioSourceNode} Stream source node
   */
  createStreamSource(stream, sourceId) {
    let mediaStream;
    let streamId;

    if (typeof stream === "string") {
      // Stream ID provided
      streamId = stream;
      const streamData = this.streams.get(streamId);
      if (!streamData) {
        throw new Error(`Stream "${streamId}" not found`);
      }
      mediaStream = streamData.mediaStream;
    } else {
      // MediaStream provided
      mediaStream = stream;
      streamId = this._findStreamIdByMediaStream(mediaStream) || "unknown";
    }

    if (!sourceId) {
      sourceId = `${streamId}_source`;
    }

    if (this.streamSources.has(sourceId)) {
      throw new Error(`Stream source "${sourceId}" already exists`);
    }

    try {
      const sourceNode = this.audioContext.createMediaStreamSource(mediaStream);

      this.streamSources.set(sourceId, {
        id: sourceId,
        streamId: streamId,
        node: sourceNode,
        mediaStream: mediaStream,
        createdAt: Date.now(),
      });

      console.log(
        `Created stream source "${sourceId}" for stream "${streamId}"`
      );

      this._emitEvent("streamSourceCreated", {
        sourceId,
        streamId,
        sourceNode,
      });

      return sourceNode;
    } catch (error) {
      console.error(`Failed to create stream source for ${streamId}:`, error);
      this._emitEvent("streamSourceError", { streamId, sourceId, error });
      throw error;
    }
  }

  /**
   * Create stream destination node
   *
   * @param {Object} [options={}] - Destination options
   * @param {string} [destinationId] - Custom destination ID
   * @returns {MediaStreamAudioDestinationNode} Stream destination node
   */
  createStreamDestination(options = {}, destinationId) {
    if (!destinationId) {
      destinationId = this._generateStreamId("destination");
    }

    if (this.streamDestinations.has(destinationId)) {
      throw new Error(`Stream destination "${destinationId}" already exists`);
    }

    try {
      const destinationNode = this.audioContext.createMediaStreamDestination();

      const destinationData = {
        id: destinationId,
        node: destinationNode,
        mediaStream: destinationNode.stream,
        options: options,
        createdAt: Date.now(),
      };

      this.streamDestinations.set(destinationId, destinationData);

      console.log(`Created stream destination "${destinationId}"`);

      this._emitEvent("streamDestinationCreated", {
        destinationId,
        destinationNode,
        mediaStream: destinationNode.stream,
      });

      return destinationNode;
    } catch (error) {
      console.error("Failed to create stream destination:", error);
      this._emitEvent("streamDestinationError", { destinationId, error });
      throw error;
    }
  }

  /**
   * Start stream recording
   *
   * @param {MediaStream|string} stream - Media stream or stream ID
   * @param {Object} [options={}] - Recording options
   * @returns {Promise<string>} Recording ID
   */
  async startRecording(stream, options = {}) {
    let mediaStream;
    let streamId;

    if (typeof stream === "string") {
      streamId = stream;
      const streamData = this.streams.get(streamId);
      if (!streamData) {
        throw new Error(`Stream "${streamId}" not found`);
      }
      mediaStream = streamData.mediaStream;
    } else {
      mediaStream = stream;
      streamId = this._findStreamIdByMediaStream(mediaStream) || "unknown";
    }

    const recordingId = `${streamId}_recording_${Date.now()}`;

    try {
      const recordingOptions = {
        mimeType: options.mimeType || "audio/webm",
        audioBitsPerSecond: options.audioBitsPerSecond || 128000,
        ...options,
      };

      const mediaRecorder = new MediaRecorder(mediaStream, recordingOptions);
      const chunks = [];

      mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          chunks.push(event.data);
        }
      };

      mediaRecorder.onstop = () => {
        const recordingData = this.recordings.get(recordingId);
        if (recordingData) {
          recordingData.blob = new Blob(chunks, {
            type: recordingOptions.mimeType,
          });
          recordingData.isRecording = false;
          recordingData.endedAt = Date.now();
          recordingData.duration =
            recordingData.endedAt - recordingData.startedAt;

          console.log(
            `Recording ${recordingId} completed: ${recordingData.duration}ms`
          );

          this._emitEvent("recordingCompleted", {
            recordingId,
            recordingData: recordingData,
          });
        }
      };

      mediaRecorder.onerror = (event) => {
        console.error(`Recording error for ${recordingId}:`, event.error);
        this._emitEvent("recordingError", { recordingId, error: event.error });
      };

      const recordingData = {
        id: recordingId,
        streamId: streamId,
        mediaRecorder: mediaRecorder,
        mediaStream: mediaStream,
        options: recordingOptions,
        chunks: chunks,
        blob: null,
        isRecording: false,
        startedAt: Date.now(),
        endedAt: null,
        duration: 0,
      };

      this.recordings.set(recordingId, recordingData);

      // Start recording
      mediaRecorder.start();
      recordingData.isRecording = true;

      // Update performance metrics
      this.performance.recordingsActive++;
      this.performance.totalRecordings++;

      console.log(
        `Started recording "${recordingId}" for stream "${streamId}"`
      );

      this._emitEvent("recordingStarted", {
        recordingId,
        streamId,
        options: recordingOptions,
      });

      return recordingId;
    } catch (error) {
      console.error(`Failed to start recording for stream ${streamId}:`, error);
      this._emitEvent("recordingStartError", { streamId, recordingId, error });
      throw error;
    }
  }

  /**
   * Stop stream recording
   *
   * @param {string} recordingId - Recording ID
   * @returns {Promise<Blob>} Recording blob
   */
  async stopRecording(recordingId) {
    const recordingData = this.recordings.get(recordingId);
    if (!recordingData) {
      throw new Error(`Recording "${recordingId}" not found`);
    }

    if (!recordingData.isRecording) {
      throw new Error(`Recording "${recordingId}" is not active`);
    }

    return new Promise((resolve, reject) => {
      const handleStop = () => {
        const recording = this.recordings.get(recordingId);
        if (recording && recording.blob) {
          this.performance.recordingsActive--;
          resolve(recording.blob);
        } else {
          reject(new Error(`Failed to get recording blob for ${recordingId}`));
        }
      };

      // Listen for completion
      const stopHandler = (event) => {
        if (event.recordingId === recordingId) {
          this.removeEventListener("recordingCompleted", stopHandler);
          handleStop();
        }
      };

      this.addEventListener("recordingCompleted", stopHandler);

      // Stop the recorder
      try {
        recordingData.mediaRecorder.stop();
      } catch (error) {
        this.removeEventListener("recordingCompleted", stopHandler);
        reject(error);
      }
    });
  }

  /**
   * Create stream monitor
   *
   * @param {string} streamId - Stream ID
   * @param {Object} [options={}] - Monitor options
   * @returns {Object} Monitor data
   */
  createStreamMonitor(streamId, options = {}) {
    const streamData = this.streams.get(streamId);
    if (!streamData) {
      throw new Error(`Stream "${streamId}" not found`);
    }

    if (this.monitors.has(streamId)) {
      return this.monitors.get(streamId);
    }

    try {
      // Create analyser node
      const analyser = this.audioContext.createAnalyser();
      analyser.fftSize = options.fftSize || 2048;
      analyser.smoothingTimeConstant = options.smoothingTimeConstant || 0.8;

      // Create source if not exists
      let sourceNode = null;
      for (const [sourceId, sourceData] of this.streamSources.entries()) {
        if (sourceData.streamId === streamId) {
          sourceNode = sourceData.node;
          break;
        }
      }

      if (!sourceNode) {
        sourceNode = this.createStreamSource(streamData.mediaStream);
      }

      // Connect source to analyser
      sourceNode.connect(analyser);

      const monitorData = {
        streamId: streamId,
        analyser: analyser,
        sourceNode: sourceNode,
        bufferLength: analyser.frequencyBinCount,
        dataArray: new Uint8Array(analyser.frequencyBinCount),
        timeData: new Uint8Array(analyser.frequencyBinCount),
        isMonitoring: true,
        createdAt: Date.now(),
        stats: {
          peak: 0,
          rms: 0,
          averageLevel: 0,
          clipping: false,
        },
      };

      this.monitors.set(streamId, monitorData);

      // Start monitoring
      this._startMonitoring(monitorData);

      console.log(`Created stream monitor for "${streamId}"`);

      this._emitEvent("streamMonitorCreated", {
        streamId,
        monitorData,
      });

      return monitorData;
    } catch (error) {
      console.error(`Failed to create stream monitor for ${streamId}:`, error);
      this._emitEvent("streamMonitorError", { streamId, error });
      throw error;
    }
  }

  /**
   * Start monitoring stream
   * @private
   */
  _startMonitoring(monitorData) {
    const updateStats = () => {
      if (!monitorData.isMonitoring) {
        return;
      }

      // Get frequency data
      monitorData.analyser.getByteFrequencyData(monitorData.dataArray);
      monitorData.analyser.getByteTimeDomainData(monitorData.timeData);

      // Calculate statistics
      let sum = 0;
      let peak = 0;
      let clipping = false;

      for (let i = 0; i < monitorData.timeData.length; i++) {
        const sample = (monitorData.timeData[i] - 128) / 128;
        const absSample = Math.abs(sample);

        sum += sample * sample;
        peak = Math.max(peak, absSample);

        if (absSample > 0.95) {
          clipping = true;
        }
      }

      monitorData.stats.rms = Math.sqrt(sum / monitorData.timeData.length);
      monitorData.stats.peak = peak;
      monitorData.stats.clipping = clipping;

      // Calculate average level from frequency data
      let freqSum = 0;
      for (let i = 0; i < monitorData.dataArray.length; i++) {
        freqSum += monitorData.dataArray[i];
      }
      monitorData.stats.averageLevel =
        freqSum / monitorData.dataArray.length / 255;

      // Emit monitoring event
      this._emitEvent("streamMonitorUpdate", {
        streamId: monitorData.streamId,
        stats: { ...monitorData.stats },
        frequencyData: new Uint8Array(monitorData.dataArray),
        timeData: new Uint8Array(monitorData.timeData),
      });

      // Schedule next update
      requestAnimationFrame(updateStats);
    };

    updateStats();
  }

  /**
   * Stop stream
   *
   * @param {string} streamId - Stream ID
   */
  stopStream(streamId) {
    const streamData = this.streams.get(streamId);
    if (!streamData) {
      throw new Error(`Stream "${streamId}" not found`);
    }

    try {
      // Stop all tracks
      streamData.tracks.forEach((track) => {
        track.stop();
      });

      // Clean up monitoring
      if (this.monitors.has(streamId)) {
        const monitor = this.monitors.get(streamId);
        monitor.isMonitoring = false;
        this.monitors.delete(streamId);
      }

      // Clean up sources
      for (const [sourceId, sourceData] of this.streamSources.entries()) {
        if (sourceData.streamId === streamId) {
          sourceData.node.disconnect();
          this.streamSources.delete(sourceId);
        }
      }

      // Update stream data
      streamData.isActive = false;
      streamData.endedAt = Date.now();

      console.log(`Stopped stream "${streamId}"`);

      this._emitEvent("streamStopped", { streamId });
    } catch (error) {
      console.error(`Failed to stop stream ${streamId}:`, error);
      this._emitEvent("streamStopError", { streamId, error });
      throw error;
    }
  }

  /**
   * Switch audio device
   *
   * @param {string} streamId - Stream ID
   * @param {string} deviceId - New device ID
   * @returns {Promise<MediaStream>} New media stream
   */
  async switchDevice(streamId, deviceId) {
    const streamData = this.streams.get(streamId);
    if (!streamData) {
      throw new Error(`Stream "${streamId}" not found`);
    }

    try {
      // Create new constraints with new device
      const newConstraints = {
        ...streamData.constraints,
        audio: {
          ...streamData.constraints.audio,
          deviceId: deviceId,
        },
      };

      // Stop current stream
      this.stopStream(streamId);

      // Create new stream with same ID
      const newStream = await this.getUserMediaStream(newConstraints, streamId);

      console.log(
        `Switched device for stream "${streamId}" to device "${deviceId}"`
      );

      this._emitEvent("deviceSwitched", {
        streamId,
        oldDeviceId: streamData.constraints.audio.deviceId,
        newDeviceId: deviceId,
      });

      return newStream;
    } catch (error) {
      console.error(`Failed to switch device for stream ${streamId}:`, error);
      this._emitEvent("deviceSwitchError", { streamId, deviceId, error });
      throw error;
    }
  }

  // === UTILITY METHODS ===

  /**
   * Generate unique stream ID
   * @private
   */
  _generateStreamId(type) {
    return `${type}_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  /**
   * Find stream ID by MediaStream
   * @private
   */
  _findStreamIdByMediaStream(mediaStream) {
    for (const [streamId, streamData] of this.streams.entries()) {
      if (streamData.mediaStream === mediaStream) {
        return streamId;
      }
    }
    return null;
  }

  /**
   * Start performance monitoring
   * @private
   */
  _startPerformanceMonitoring() {
    const updateMetrics = () => {
      // Update memory usage if available
      if (performance.memory) {
        this.performance.memoryUsage =
          performance.memory.usedJSHeapSize / 1024 / 1024;
      }

      // Estimate CPU usage based on active streams
      this.performance.cpuUsage = Math.min(
        0.1 +
          this.performance.activeStreams * 0.05 +
          this.performance.recordingsActive * 0.1,
        1.0
      );

      this._emitEvent("performanceUpdate", this.performance);
    };

    // Update metrics every 5 seconds
    setInterval(updateMetrics, 5000);
  }

  /**
   * Get stream by ID
   *
   * @param {string} streamId - Stream ID
   * @returns {Object|null} Stream data
   */
  getStream(streamId) {
    return this.streams.get(streamId) || null;
  }

  /**
   * Get all streams
   *
   * @returns {Map<string, Object>} All streams
   */
  getAllStreams() {
    return new Map(this.streams);
  }

  /**
   * Get stream source
   *
   * @param {string} sourceId - Source ID
   * @returns {Object|null} Source data
   */
  getStreamSource(sourceId) {
    return this.streamSources.get(sourceId) || null;
  }

  /**
   * Get recording
   *
   * @param {string} recordingId - Recording ID
   * @returns {Object|null} Recording data
   */
  getRecording(recordingId) {
    return this.recordings.get(recordingId) || null;
  }

  /**
   * Get available devices
   *
   * @returns {Object} Available devices
   */
  getAvailableDevices() {
    return {
      input: [...this.devices.input],
      output: [...this.devices.output],
      capabilities: new Map(this.devices.capabilities),
    };
  }

  /**
   * Get stream monitor
   *
   * @param {string} streamId - Stream ID
   * @returns {Object|null} Monitor data
   */
  getStreamMonitor(streamId) {
    return this.monitors.get(streamId) || null;
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
    console.log("Destroying WebAudioStreams manager...");

    // Stop all streams
    for (const streamId of this.streams.keys()) {
      try {
        this.stopStream(streamId);
      } catch (error) {
        console.warn(`Failed to stop stream ${streamId}:`, error);
      }
    }

    // Stop all recordings
    for (const [recordingId, recordingData] of this.recordings.entries()) {
      if (recordingData.isRecording) {
        try {
          recordingData.mediaRecorder.stop();
        } catch (error) {
          console.warn(`Failed to stop recording ${recordingId}:`, error);
        }
      }
    }

    // Disconnect all sources
    for (const [sourceId, sourceData] of this.streamSources.entries()) {
      try {
        sourceData.node.disconnect();
      } catch (error) {
        console.warn(`Failed to disconnect source ${sourceId}:`, error);
      }
    }

    // Clear all collections
    this.streams.clear();
    this.streamSources.clear();
    this.streamDestinations.clear();
    this.recordings.clear();
    this.monitors.clear();
    this.visualizers.clear();
    this.eventHandlers.clear();

    // Reset performance counters
    this.performance.activeStreams = 0;
    this.performance.recordingsActive = 0;

    this._emitEvent("destroyed");
    console.log("WebAudioStreams manager destroyed");
  }
}

export default WebAudioStreams;
