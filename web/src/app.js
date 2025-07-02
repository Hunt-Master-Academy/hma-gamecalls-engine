// Huntmaster Web Application - Improved Version
// This code review focuses on improving the interface with the WASM engine,
// memory management, and overall architecture.

/**
 * @class AudioProcessor
 * @brief Manages audio recording, processing, and interaction with the WASM engine
 */
class AudioProcessor {
  constructor() {
    this.audioContext = null;
    this.mediaRecorder = null;
    this.recordedChunks = [];
    this.isRecording = false;
    this.analyser = null;
    this.animationId = null;
    this.masterCallAudioData = null;
    this.masterCallAudioDuration = 0;

    // Engine-related properties
    this.engine = null;
    this.engineId = null;
    this.engineReady = false;

    // Master call data
    this.masterCallAudioData = null;
    this.masterCallWaveformData = null;
    this.masterCallDuration = 0;

    // Constants
    this.CHUNK_SIZE = 4096;
    this.WAVEFORM_UPDATE_INTERVAL = 50; // ms
    this.MAX_WAVEFORM_SAMPLES = 44100 * 3; // 3 seconds at 44.1kHz
  }

  /**
   * Initialize the WASM engine
   * @returns {Promise<boolean>} Success status
   */
  async initializeEngine() {
    try {
      if (typeof HuntmasterEngine === "undefined") {
        throw new Error("HuntmasterEngine module not found");
      }

      const Module = await HuntmasterEngine();

      // Wait for module to be fully ready
      if (Module.ready) {
        await Module.ready;
      }

      this.engine = Module;

      // Validate required functions
      const requiredFunctions = [
        "_createEngine",
        "_destroyEngine",
        "_startSession",
        "_endSession",
        "_processAudioChunk",
        "_loadMasterCall",
        "_getSimilarityScore",
        "_malloc",
        "_free",
      ];

      for (const func of requiredFunctions) {
        if (typeof this.engine[func] !== "function") {
          throw new Error(`Required function ${func} not found in engine`);
        }
      }

      // Create engine instance
      this.engineId = this.engine._createEngine();

      if (!this.engineId || this.engineId <= 0) {
        throw new Error(`Invalid engine ID: ${this.engineId}`);
      }

      this.engineReady = true;
      Logger.log("Engine initialized successfully", "success");
      return true;
    } catch (error) {
      Logger.log(`Engine initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Allocate memory in WASM heap and copy data
   * @param {Float32Array|Array} data - Data to copy
   * @returns {{ptr: number, size: number}} Pointer and size information
   */
  allocateWASMMemory(data) {
    const size = data.length * 4; // 4 bytes per float
    const ptr = this.audioProcessor.engine._malloc(size);

    if (!ptr) {
      throw new Error("Failed to allocate WASM memory");
    }

    try {
      this.engine.HEAPF32.set(data, ptr / 4);
      return { ptr, size };
    } catch (error) {
      this.engine._free(ptr);
      throw error;
    }
  }

  /**
   * Process audio chunk with proper memory management
   * @param {Float32Array} audioData - Audio samples
   * @param {number} sampleRate - Sample rate
   * @returns {number} Processing result code
   */
  processAudioChunk(audioData, sampleRate) {
    const { ptr, size } = this.allocateWASMMemory(audioData);

    try {
      const result = this.engine._processAudioChunk(
        this.engineId,
        ptr,
        audioData.length,
        sampleRate
      );

      return result;
    } finally {
      this.engine._free(ptr);
    }
  }

  /**
   * Load master call with validation
   * @param {Object} masterCallData - Master call MFCC data
   * @returns {boolean} Success status
   */
  loadMasterCall(masterCallData) {
    if (
      !masterCallData.mfcc_features ||
      !Array.isArray(masterCallData.mfcc_features)
    ) {
      throw new Error("Invalid master call data format");
    }

    const flatFeatures = masterCallData.mfcc_features.flat();
    const numFrames = masterCallData.mfcc_features.length;
    const numCoefficients = masterCallData.mfcc_features[0].length;

    const { ptr } = this.allocateWASMMemory(flatFeatures);

    try {
      const result = this.engine._loadMasterCall(
        this.engineId,
        ptr,
        numFrames,
        numCoefficients
      );

      return result === 1;
    } finally {
      this.engine._free(ptr);
    }
  }

  /**
   * Clean up resources
   */
  destroy() {
    if (this.engine && this.engineId) {
      this.engine._destroyEngine(this.engineId);
      this.engineId = null;
    }

    if (this.audioContext) {
      this.audioContext.close();
      this.audioContext = null;
    }

    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
      this.animationId = null;
    }
  }
}

/**
 * @class WaveformVisualizer
 * @brief Handles waveform visualization for both master calls and recordings
 */
class WaveformVisualizer {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.masterWaveform = null;
    this.liveWaveform = null;

    this.setupCanvas();
  }

  setupCanvas() {
    const rect = this.canvas.getBoundingClientRect();
    this.canvas.width = rect.width;
    this.canvas.height = rect.height;

    this.clear();
  }

  clear() {
    this.ctx.fillStyle = "#f0f0f0";
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
  }

  /**
   * Downsample audio data for visualization
   * @param {Float32Array} audioData - Raw audio data
   * @param {number} targetLength - Target number of points
   * @returns {Float32Array} Downsampled data
   */
  downsample(audioData, targetLength) {
    const sourceLength = audioData.length;
    const ratio = sourceLength / targetLength;
    const result = new Float32Array(targetLength);

    for (let i = 0; i < targetLength; i++) {
      const start = Math.floor(i * ratio);
      const end = Math.floor((i + 1) * ratio);

      // RMS value for better visualization
      let sum = 0;
      let count = 0;

      for (let j = start; j < end && j < sourceLength; j++) {
        sum += audioData[j] * audioData[j];
        count++;
      }

      result[i] = count > 0 ? Math.sqrt(sum / count) : 0;

      // Preserve sign from original
      if (audioData[start] < 0) {
        result[i] = -result[i];
      }
    }

    return result;
  }

  // /**
  //  * Draw waveform with specified style
  //  * @param {Float32Array} waveformData - Waveform data
  //  * @param {Object} style - Drawing style options
  //  */
  // drawWaveform(waveformData, style = {}) {
  //   const {
  //     color = '#00aa00',
  //     lineWidth = 2,
  //     opacity = 1.0,
  //     offset = 0
  //   } = style;

  //   const width = this.canvas.width;
  //   const height = this.canvas.height;
  //   const centerY = height / 2;

  //   this.ctx.strokeStyle = opacity < 1 ?
  //     `rgba(${this.hexToRgb(color).join(',')}, ${opacity})` : color;
  //   this.ctx.lineWidth = lineWidth;
  //   this.ctx.beginPath();

  //   const barWidth = width / waveformData.length;

  //   for (let i = 0; i < waveformData.length; i++) {
  //     const x = i * barWidth + offset;
  //     const normalizedValue = waveformData[i];
  //     const y = centerY - normalizedValue * centerY * 0.8;

  //     if (i === 0) {
  //       this.ctx.moveTo(x, y);
  //     } else {
  //       this.ctx.lineTo(x, y);
  //     }
  //   }

  //   this.ctx.stroke();
  // }

  drawMasterCallWaveform() {
    if (!this.masterCallAudioData || !this.waveformVisualizer) return;

    const width = this.waveformVisualizer.canvas.width;

    // Downsample the audio data for visualization
    const downsampled = this.waveformVisualizer.downsample(
      this.masterCallAudioData,
      width
    );

    // Store it in the visualizer
    this.waveformVisualizer.masterWaveform = downsampled;

    // Update the display
    this.waveformVisualizer.update();
  }

  hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result
      ? [
          parseInt(result[1], 16),
          parseInt(result[2], 16),
          parseInt(result[3], 16),
        ]
      : [0, 0, 0];
  }

  /**
   * Update display with both master and live waveforms
   */
  update(analyserNode = null) {
    this.clear();

    // Draw center line
    this.ctx.strokeStyle = "#ccc";
    this.ctx.lineWidth = 1;
    this.ctx.beginPath();
    this.ctx.moveTo(0, this.canvas.height / 2);
    this.ctx.lineTo(this.canvas.width, this.canvas.height / 2);
    this.ctx.stroke();

    // Draw master waveform if available
    if (this.masterWaveform) {
      this.drawWaveform(this.masterWaveform, {
        color: "#6464c8",
        opacity: 0.5,
      });
    }

    // Draw live waveform if recording
    if (analyserNode) {
      const bufferLength = analyserNode.fftSize;
      const dataArray = new Float32Array(bufferLength);
      analyserNode.getFloatTimeDomainData(dataArray);

      const downsampled = this.downsample(dataArray, this.canvas.width);
      this.drawWaveform(downsampled, {
        color: "#00aa00",
        opacity: 1.0,
      });
    }

    // Draw legend
    this.drawLegend();
  }

  drawLegend() {
    this.ctx.font = "12px Arial";

    if (this.masterWaveform) {
      this.ctx.fillStyle = "rgba(100, 100, 200, 0.8)";
      this.ctx.fillText("Master Call", 10, 20);
    }

    this.ctx.fillStyle = "#00aa00";
    this.ctx.fillText("Your Recording", 10, 35);
  }
}

/**
 * @class Logger
 * @brief Centralized logging utility
 */
class Logger {
  static log(message, level = "info") {
    const timestamp = new Date().toLocaleTimeString();
    const formattedMessage = `[${timestamp}] ${message}`;

    // Console logging
    switch (level) {
      case "error":
        console.error(formattedMessage);
        break;
      case "warn":
        console.warn(formattedMessage);
        break;
      case "success":
        console.log(`✅ ${formattedMessage}`);
        break;
      default:
        console.log(formattedMessage);
    }

    // Page logging
    const logElement = document.getElementById("debugLog");
    if (logElement) {
      logElement.textContent += `${formattedMessage}\n`;
      logElement.scrollTop = logElement.scrollHeight;
    }
  }
}
// Create script processor for real-time processing
const scriptProcessor = this.audioProcessor.audioContext.createScriptProcessor(
  4096,
  1,
  1
);

//
// scriptProcessor.onaudioprocess = (event) => {
//   if (this.audioProcessor.isRecording) {
//     const inputData = event.inputBuffer.getChannelData(0);

//     // Send to WASM for processing
//     const audioPtr = this.audioProcessor.engine._malloc(inputData.length * 4);
//     this.audioProcessor.engine.HEAPF32.set(inputData, audioPtr / 4);
//     this.audioProcessor.engine._processAudioChunk(
//       this.audioProcessor.engineId,
//       audioPtr,
//       inputData.length
//     );
//     this.audioProcessor.engine._free(audioPtr);
//   }
// };

// Connect nodes
source.connect(scriptProcessor);
scriptProcessor.connect(this.audioProcessor.audioContext.destination);

// Store reference for cleanup
this.scriptProcessor = scriptProcessor;
/**
 * @class UIController
 * @brief Manages UI state and interactions
 */
class UIController {
  constructor() {
    this.elements = {
      startBtn: document.getElementById("startBtn"),
      stopBtn: document.getElementById("stopBtn"),
      playbackBtn: document.getElementById("playbackBtn"),
      playMasterBtn: document.getElementById("playMasterCallBtn"),
      loadMasterBtn: document.getElementById("loadMasterBtn"),
      resetBtn: document.getElementById("resetBtn"),
      masterCallSelect: document.getElementById("masterCallSelect"),
      status: document.getElementById("status"),
      engineStatus: document.getElementById("engineStatus"),
      similarityScore: document.getElementById("similarityScore"),
    };

    this.states = {
      IDLE: "idle",
      RECORDING: "recording",
      PROCESSING: "processing",
      READY: "ready",
    };

    this.currentState = this.states.IDLE;
  }

  setState(state) {
    this.currentState = state;
    this.updateUI();
  }

  updateUI() {
    switch (this.currentState) {
      case this.states.IDLE:
        this.elements.startBtn.disabled = true;
        this.elements.stopBtn.disabled = true;
        this.elements.playbackBtn.disabled = true;
        break;

      case this.states.READY:
        this.elements.startBtn.disabled = false;
        this.elements.stopBtn.disabled = true;
        break;

      case this.states.RECORDING:
        this.elements.startBtn.disabled = true;
        this.elements.stopBtn.disabled = false;
        break;

      case this.states.PROCESSING:
        this.elements.startBtn.disabled = true;
        this.elements.stopBtn.disabled = true;
        break;
    }
  }

  showStatus(message, type = "info") {
    this.elements.status.textContent = message;

    const colors = {
      success: "#4CAF50",
      error: "#f44336",
      info: "#2196F3",
    };

    this.elements.status.style.color = colors[type] || colors.info;
  }

  updateSimilarityScore(percentage) {
    const element = this.elements.similarityScore;
    element.textContent = `${percentage.toFixed(1)}%`;
    element.style.display = "block";

    // Color coding
    if (percentage >= 80) {
      element.style.color = "#00aa00";
      element.style.backgroundColor = "#e8f5e9";
    } else if (percentage >= 60) {
      element.style.color = "#ff9800";
      element.style.backgroundColor = "#fff3e0";
    } else {
      element.style.color = "#f44336";
      element.style.backgroundColor = "#ffebee";
    }
  }
}

/**
 * @class HuntmasterApplication
 * @brief Main application controller that orchestrates all components
 */
class HuntmasterApplication {
  constructor() {
    this.audioProcessor = new AudioProcessor();
    this.waveformVisualizer = new WaveformVisualizer("waveform");
    this.uiController = new UIController();

    // Audio playback
    this.recordedBuffer = null;
    this.masterCallBuffer = null;

    // Recording state
    this.mediaRecorder = null;
    this.recordedChunks = [];
    this.audioStream = null;

    // Animation state
    this.isAnimating = false;
    this.animationId = null;

    // Configuration
    this.config = {
      fftSize: 2048,
      smoothingTimeConstant: 0.8,
      minDecibels: -90,
      maxDecibels: -10,
    };
  }

  /**
   * Initialize the application
   */
  async initialize() {
    try {
      Logger.log("Initializing Huntmaster Application...");

      // Initialize audio context
      this.audioProcessor.audioContext = new (window.AudioContext ||
        window.webkitAudioContext)();

      // Initialize WASM engine
      await this.audioProcessor.initializeEngine();

      // Update UI
      this.uiController.elements.engineStatus.textContent = "Engine Ready!";
      this.uiController.setState(this.uiController.states.READY);

      // Setup event listeners
      this.setupEventListeners();

      // Setup cleanup on page unload
      window.addEventListener("beforeunload", () => this.cleanup());

      Logger.log("Application initialized successfully", "success");
    } catch (error) {
      Logger.log(`Failed to initialize application: ${error.message}`, "error");
      this.uiController.elements.engineStatus.textContent =
        "Initialization Error";
      this.uiController.showStatus(
        "Failed to initialize. Please refresh the page.",
        "error"
      );
      throw error;
    }
  }

  /**
   * Setup all event listeners
   */
  setupEventListeners() {
    // Recording controls
    this.uiController.elements.startBtn.addEventListener("click", () =>
      this.startRecording()
    );
    this.uiController.elements.stopBtn.addEventListener("click", () =>
      this.stopRecording()
    );
    this.uiController.elements.playbackBtn.addEventListener("click", () =>
      this.playRecording()
    );
    this.uiController.elements.resetBtn.addEventListener("click", () =>
      this.reset()
    );

    // Master call controls
    this.uiController.elements.loadMasterBtn.addEventListener("click", () =>
      this.loadMasterCall()
    );
    this.uiController.elements.playMasterBtn.addEventListener("click", () =>
      this.playMasterCall()
    );

    // Master call selection
    this.uiController.elements.masterCallSelect.addEventListener(
      "change",
      (e) => {
        this.uiController.elements.loadMasterBtn.disabled = !e.target.value;
      }
    );
  }

  /**
   * Start audio recording
   */
  async startRecording() {
    try {
      Logger.log("Starting recording...");
      this.uiController.showStatus("Requesting microphone access...", "info");

      // Get audio stream
      this.audioStream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: true,
          noiseSuppression: true,
          autoGainControl: true,
        },
      });

      // Create audio nodes
      const source = this.audioProcessor.audioContext.createMediaStreamSource(
        this.audioStream
      );

      // Setup analyser for visualization
      this.audioProcessor.analyser =
        this.audioProcessor.audioContext.createAnalyser();
      this.audioProcessor.analyser.fftSize = this.config.fftSize;
      this.audioProcessor.analyser.smoothingTimeConstant =
        this.config.smoothingTimeConstant;
      this.audioProcessor.analyser.minDecibels = this.config.minDecibels;
      this.audioProcessor.analyser.maxDecibels = this.config.maxDecibels;

      source.connect(this.audioProcessor.analyser);

      // Setup media recorder
      this.recordedChunks = [];
      this.mediaRecorder = new MediaRecorder(this.audioStream, {
        mimeType: "audio/webm",
      });

      this.mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          this.recordedChunks.push(event.data);
        }
      };

      this.mediaRecorder.onstop = async () => {
        await this.processRecording();
      };

      // Start engine session
      // const sessionResult = this.audioProcessor.engine._startSession(
      //   this.audioProcessor.engineId
      // );
      const sessionResult = this.audioProcessor.engine._startSession(
        this.audioProcessor.engineId,
        44100, // sampleRate
        2048, // frameSize
        512 // hopSize
      );
      if (sessionResult !== 1) {
        throw new Error(`Failed to start engine session: ${sessionResult}`);
      }

      // Start recording
      this.mediaRecorder.start();
      this.audioProcessor.isRecording = true;

      // Update UI
      this.uiController.setState(this.uiController.states.RECORDING);
      this.uiController.showStatus("Recording...", "info");

      // Start waveform animation
      this.startWaveformAnimation();

      Logger.log("Recording started successfully", "success");
    } catch (error) {
      Logger.log(`Failed to start recording: ${error.message}`, "error");
      this.uiController.showStatus("Failed to access microphone", "error");
      this.uiController.setState(this.uiController.states.READY);

      // Cleanup on error
      if (this.audioStream) {
        this.audioStream.getTracks().forEach((track) => track.stop());
        this.audioStream = null;
      }
    }
  }

  /**
   * Stop audio recording
   */
  stopRecording() {
    Logger.log("Stopping recording...");

    if (this.mediaRecorder && this.audioProcessor.isRecording) {
      this.mediaRecorder.stop();
      this.audioProcessor.isRecording = false;

      // Stop audio stream
      if (this.audioStream) {
        this.audioStream.getTracks().forEach((track) => track.stop());
        this.audioStream = null;
      }

      // Stop animation
      this.stopWaveformAnimation();

      // Update UI
      this.uiController.setState(this.uiController.states.PROCESSING);
      this.uiController.showStatus("Processing...", "info");

      // End engine session
      this.audioProcessor.engine._endSession(this.audioProcessor.engineId);
      if (this.scriptProcessor) {
        this.scriptProcessor.disconnect();
        this.scriptProcessor = null;
      }
      Logger.log("Recording stopped", "success");
    }
  }

  /**
   * Process recorded audio
   */
  async processRecording() {
    try {
      const audioBlob = new Blob(this.recordedChunks, { type: "audio/webm" });

      // Convert to array buffer
      const arrayBuffer = await audioBlob.arrayBuffer();

      // Decode audio data
      this.recordedBuffer =
        await this.audioProcessor.audioContext.decodeAudioData(
          arrayBuffer.slice(0)
        );

      // Enable playback
      this.uiController.elements.playbackBtn.disabled = false;

      // Calculate similarity score
      await this.calculateSimilarity(audioBlob);

      // Update UI
      this.uiController.setState(this.uiController.states.READY);
    } catch (error) {
      Logger.log(`Failed to process recording: ${error.message}`, "error");
      this.uiController.showStatus("Failed to process recording", "error");
      this.uiController.setState(this.uiController.states.READY);
    }
  }

  /**
   * Calculate similarity score between recording and master call
   */
  async calculateSimilarity(audioBlob) {
    try {
      Logger.log("Calculating similarity score...");
      this.uiController.showStatus("Analyzing similarity...", "info");

      const arrayBuffer = await audioBlob.arrayBuffer();
      const audioBuffer =
        await this.audioProcessor.audioContext.decodeAudioData(arrayBuffer);

      const audioData = audioBuffer.getChannelData(0);
      const sampleRate = audioBuffer.sampleRate;

      // Normalize audio
      const maxValue = Math.max(...audioData.map(Math.abs));
      const normalizedData = new Float32Array(audioData.length);

      if (maxValue > 0) {
        for (let i = 0; i < audioData.length; i++) {
          normalizedData[i] = audioData[i] / maxValue;
        }
      }

      // Start new session for similarity calculation
      const sessionResult = this.audioProcessor.engine._startSession(
        this.audioProcessor.engineId
      );
      if (sessionResult !== 1) {
        throw new Error("Failed to start similarity session");
      }

      // Process audio in chunks
      const chunkSize = this.audioProcessor.CHUNK_SIZE;
      let processedSamples = 0;

      while (processedSamples < normalizedData.length) {
        const remainingSamples = normalizedData.length - processedSamples;
        const currentChunkSize = Math.min(chunkSize, remainingSamples);

        const chunk = normalizedData.slice(
          processedSamples,
          processedSamples + currentChunkSize
        );

        const result = this.audioProcessor.processAudioChunk(chunk, sampleRate);

        if (result !== 1) {
          Logger.log(`Warning: Chunk processing returned ${result}`, "warn");
        }

        processedSamples += currentChunkSize;
      }

      // Get similarity score
      const rawScore = this.audioProcessor.engine._getSimilarityScore(
        this.audioProcessor.engineId
      );

      // Get feature count for debugging
      const featureCount = this.audioProcessor.engine._getSessionFeatureCount(
        this.audioProcessor.engineId
      );
      Logger.log(`Processed ${featureCount} feature frames`);

      // End session
      this.audioProcessor.engine._endSession(this.audioProcessor.engineId);

      // Convert DTW score to percentage
      const similarityPercentage = this.convertDTWToPercentage(rawScore);

      Logger.log(
        `Raw DTW score: ${rawScore}, Similarity: ${similarityPercentage}%`
      );

      // Update UI
      this.uiController.updateSimilarityScore(similarityPercentage);
      this.uiController.showStatus(
        `Similarity Score: ${similarityPercentage.toFixed(1)}%`,
        similarityPercentage > 70 ? "success" : "info"
      );
    } catch (error) {
      Logger.log(`Failed to calculate similarity: ${error.message}`, "error");
      this.uiController.showStatus("Failed to calculate similarity", "error");
    }
  }

  /**
   * Convert DTW score to percentage
   * @param {number} dtwScore - Raw DTW score (lower is better)
   * @returns {number} Similarity percentage (0-100)
   */
  convertDTWToPercentage(dtwScore) {
    if (dtwScore < 0) return 0;

    // Empirically determined thresholds - adjust based on testing
    const perfectScore = 0;
    const goodScore = 30;
    const poorScore = 100;

    if (dtwScore <= perfectScore) {
      return 100;
    } else if (dtwScore >= poorScore) {
      return 0;
    } else if (dtwScore <= goodScore) {
      // Linear interpolation between perfect and good
      const range = goodScore - perfectScore;
      const position = dtwScore - perfectScore;
      return 100 - (position / range) * 30; // 100% to 70%
    } else {
      // Linear interpolation between good and poor
      const range = poorScore - goodScore;
      const position = dtwScore - goodScore;
      return 70 - (position / range) * 70; // 70% to 0%
    }
  }

  /**
   * Load master call audio and data
   */
  // async loadMasterCall() {
  //   const selectedCall = document.getElementById("masterCallSelect").value;

  //   if (!selectedCall) {
  //     this.uiController.showStatus("Please select a master call", "error");
  //     return;
  //   }

  //   try {
  //     Logger.log(`Loading master call: ${selectedCall}`);
  //     this.uiController.showStatus("Loading master call...", "info");

  //     // Load both WAV (for playback/visualization) and JSON (for MFCC features)

  //     // 1. Load WAV file for playback and waveform visualization
  //     const audioResponse = await fetch(
  //       `test_data/master_calls/${selectedCall}.wav`
  //     );
  //     if (!audioResponse.ok) {
  //       throw new Error(
  //         `Failed to load audio file: ${audioResponse.statusText}`
  //       );
  //     }

  //     const audioBlob = await audioResponse.blob();
  //     const arrayBuffer = await audioBlob.arrayBuffer();

  //     // Decode audio for playback and waveform
  //     const audioBuffer =
  //       await this.audioProcessor.audioContext.decodeAudioData(
  //         arrayBuffer.slice(0)
  //       );
  //     this.masterCallBuffer = audioBuffer;
  //     this.masterCallDuration = audioBuffer.duration;

  //     // Extract audio data for waveform visualization
  //     this.masterCallAudioData = audioBuffer.getChannelData(0);

  //     // Prepare waveform visualization
  //     this.drawMasterCallWaveform();

  //     // Enable playback button
  //     document.getElementById("playMasterCallBtn").disabled = false;

  //     // 2. Load JSON file with MFCC features for comparison
  //     const jsonResponse = await fetch(
  //       `test_data/master_calls/${selectedCall}.json`
  //     );
  //     if (!jsonResponse.ok) {
  //       throw new Error(`Failed to load MFCC data: ${jsonResponse.statusText}`);
  //     }

  //     const masterCallData = await jsonResponse.json();

  //     if (
  //       !masterCallData.mfcc_features ||
  //       !Array.isArray(masterCallData.mfcc_features)
  //     ) {
  //       throw new Error("Invalid master call data format");
  //     }

  //     // Flatten the MFCC features for passing to WASM
  //     const flatFeatures = masterCallData.mfcc_features.flat();
  //     const numFrames = masterCallData.mfcc_features.length;
  //     const numCoefficients = masterCallData.mfcc_features[0].length;

  //     // Allocate memory in WASM heap
  //     // const featuresPtr = this.engine._malloc(flatFeatures.length * 4);
  //     const featuresPtr = this.audioProcessor.engine._malloc(
  //       flatFeatures.length * 4
  //     );

  //     // Copy data to WASM heap
  //     this.engine.HEAPF32.set(flatFeatures, featuresPtr / 4);

  //     // Load into engine
  //     const result = this.engine._loadMasterCall(
  //       this.engineId,
  //       featuresPtr,
  //       numFrames,
  //       numCoefficients
  //     );

  //     // Free allocated memory
  //     this.engine._free(featuresPtr);

  //     if (result === 1) {
  //       this.uiController.showStatus(
  //         `Master call "${selectedCall}" loaded successfully!`,
  //         "success"
  //       );
  //       Logger.log(
  //         `Master call loaded: ${numFrames} frames, ${numCoefficients} coefficients`,
  //         "success"
  //       );
  //       Logger.log(
  //         `Master call loaded: ${numFrames} frames, ${numCoefficients} coefficients`,
  //         "success"
  //       );
  //     } else {
  //       throw new Error("Failed to load master call into engine");
  //     }
  //   } catch (error) {
  //     Logger.log(`Failed to load master call: ${error.message}`, "error");
  //     this.uiController.showStatus(`Error: ${error.message}`, "error");
  //   }
  // }
  async loadMasterCall() {
    const selectedCall = this.uiController.elements.masterCallSelect.value;

    if (!selectedCall) {
      this.uiController.showStatus("Please select a master call", "error");
      return;
    }

    try {
      Logger.log(`Loading master call: ${selectedCall}`);
      this.uiController.showStatus("Loading master call...", "info");

      // 1. Load WAV file for playback and waveform visualization
      const audioResponse = await fetch(
        `test_data/master_calls/${selectedCall}.wav`
      );
      if (!audioResponse.ok) {
        throw new Error(
          `Failed to load audio file: ${audioResponse.statusText}`
        );
      }

      const audioBlob = await audioResponse.blob();
      const arrayBuffer = await audioBlob.arrayBuffer();

      // Decode audio for playback and waveform
      const audioBuffer =
        await this.audioProcessor.audioContext.decodeAudioData(
          arrayBuffer.slice(0)
        );
      this.masterCallBuffer = audioBuffer;
      this.masterCallDuration = audioBuffer.duration;

      // Extract audio data for waveform visualization
      this.masterCallAudioData = audioBuffer.getChannelData(0);

      // Prepare waveform visualization
      this.drawMasterCallWaveform();

      // Enable playback button
      this.uiController.elements.playMasterBtn.disabled = false;

      // 2. Load JSON file with MFCC features for comparison
      const jsonResponse = await fetch(
        `test_data/master_calls/${selectedCall}.json`
      );
      if (!jsonResponse.ok) {
        throw new Error(`Failed to load MFCC data: ${jsonResponse.statusText}`);
      }

      const masterCallData = await jsonResponse.json();

      // Use the AudioProcessor's loadMasterCall method
      const success = this.audioProcessor.loadMasterCall(masterCallData);

      if (success) {
        this.uiController.showStatus(
          `Master call "${selectedCall}" loaded successfully!`,
          "success"
        );
        Logger.log(
          `Master call loaded: ${masterCallData.mfcc_features.length} frames`,
          "success"
        );
      } else {
        throw new Error("Failed to load master call into engine");
      }
    } catch (error) {
      Logger.log(`Failed to load master call: ${error.message}`, "error");
      this.uiController.showStatus(`Error: ${error.message}`, "error");
    }
  }

  /**
   * Play master call audio
   */
  async playMasterCall() {
    if (!this.masterCallBuffer) {
      this.uiController.showStatus("No master call loaded", "error");
      return;
    }

    try {
      const source = this.audioProcessor.audioContext.createBufferSource();
      source.buffer = this.masterCallBuffer;
      source.connect(this.audioProcessor.audioContext.destination);

      source.onended = () => {
        this.uiController.showStatus(
          "Master call playback finished",
          "success"
        );
      };

      source.start();
      this.uiController.showStatus("Playing master call...", "info");
    } catch (error) {
      Logger.log(`Failed to play master call: ${error.message}`, "error");
      this.uiController.showStatus("Error playing master call", "error");
    }
  }

  /**
   * Play recorded audio
   */
  playRecording() {
    if (!this.recordedBuffer) {
      this.uiController.showStatus("No recording available", "error");
      return;
    }

    try {
      const source = this.audioProcessor.audioContext.createBufferSource();
      source.buffer = this.recordedBuffer;
      source.connect(this.audioProcessor.audioContext.destination);

      source.onended = () => {
        this.uiController.showStatus("Playback finished", "success");
      };

      source.start();
      this.uiController.showStatus("Playing recording...", "info");
    } catch (error) {
      Logger.log(`Failed to play recording: ${error.message}`, "error");
      this.uiController.showStatus("Error playing recording", "error");
    }
  }

  /**
   * Start waveform animation
   */
  startWaveformAnimation() {
    this.isAnimating = true;

    const animate = () => {
      if (!this.isAnimating) return;

      this.waveformVisualizer.update(this.audioProcessor.analyser);
      this.animationId = requestAnimationFrame(animate);
    };

    animate();
  }

  /**
   * Stop waveform animation
   */
  stopWaveformAnimation() {
    this.isAnimating = false;

    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
      this.animationId = null;
    }

    // Final update to show static waveform
    this.waveformVisualizer.update();
  }

  /**
   * Reset application state
   */
  reset() {
    Logger.log("Resetting application state");

    // Stop any ongoing recording
    if (this.audioProcessor.isRecording) {
      this.stopRecording();
    }

    // Clear recorded data
    this.recordedBuffer = null;
    this.recordedChunks = [];

    // Reset UI
    this.uiController.elements.playbackBtn.disabled = true;
    this.uiController.elements.similarityScore.style.display = "none";
    this.uiController.showStatus("Ready to record", "info");
    this.uiController.setState(this.uiController.states.READY);

    // Clear waveform (keep master if loaded)
    this.waveformVisualizer.update();
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    Logger.log("Cleaning up application resources");

    // Stop recording if active
    if (this.audioProcessor.isRecording) {
      this.stopRecording();
    }

    // Stop animation
    this.stopWaveformAnimation();

    // Cleanup audio processor
    this.audioProcessor.destroy();

    // Stop any audio streams
    if (this.audioStream) {
      this.audioStream.getTracks().forEach((track) => track.stop());
    }
  }
}

// ============================================================================
// Application Entry Point
// ============================================================================

// Global application instance
let app = null;

/**
 * Initialize the application when the page loads
 */
document.addEventListener("DOMContentLoaded", () => {
  Logger.log("DOM loaded, checking for WASM module...");

  // Check if WASM module is available
  if (typeof HuntmasterEngine === "undefined") {
    Logger.log(
      "ERROR: HuntmasterEngine is not defined. Check if huntmaster_engine.js loaded correctly.",
      "error"
    );
    document.getElementById("engineStatus").textContent =
      "Error: WASM module not found";
    return;
  }

  Logger.log("HuntmasterEngine found. Creating application instance...");

  // Create and initialize application
  app = new HuntmasterApplication();

  // Initialize with error handling
  app.initialize().catch((error) => {
    Logger.log(`Fatal error during initialization: ${error.message}`, "error");
    console.error(error);
  });
});

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Format time duration
 * @param {number} seconds - Duration in seconds
 * @returns {string} Formatted time string
 */
function formatDuration(seconds) {
  const minutes = Math.floor(seconds / 60);
  const remainingSeconds = Math.floor(seconds % 60);
  return `${minutes}:${remainingSeconds.toString().padStart(2, "0")}`;
}

/**
 * Calculate RMS of audio buffer
 * @param {Float32Array} buffer - Audio samples
 * @returns {number} RMS value
 */
function calculateRMS(buffer) {
  let sum = 0;
  for (let i = 0; i < buffer.length; i++) {
    sum += buffer[i] * buffer[i];
  }
  return Math.sqrt(sum / buffer.length);
}

/**
 * Normalize audio buffer
 * @param {Float32Array} buffer - Audio samples
 * @returns {Float32Array} Normalized buffer
 */
function normalizeAudio(buffer) {
  const maxValue = Math.max(...buffer.map(Math.abs));
  const normalized = new Float32Array(buffer.length);

  if (maxValue > 0) {
    for (let i = 0; i < buffer.length; i++) {
      normalized[i] = buffer[i] / maxValue;
    }
  }

  return normalized;
}

// ============================================================================
// Export for testing (if using modules)
// ============================================================================

// If using ES6 modules, uncomment the following:
// export { HuntmasterApplication, AudioProcessor, WaveformVisualizer, UIController, Logger };
