/**
 * NoiseDetector Module
 *
 * Provides comprehensive background noise detection, filtering, and voice activity detection.
 * Implements advanced spectral analysis, adaptive noise reduction, and real-time processing
 * optimization for high-quality audio processing.
 *
 * Features:
 * - Spectral noise floor estimation and tracking
 * - Adaptive noise gating with hysteresis
 * - Noise profile learning and adaptation
 * - Real-time noise reduction processing
 * - Voice Activity Detection (VAD) integration
 * - Environmental noise classification
 */

export class NoiseDetector {
  constructor(eventManager, sampleRate = 48000) {
    this.eventManager = eventManager;
    this.sampleRate = sampleRate;
    this.isInitialized = false;

    // Core noise detection parameters
    this.config = {
      noiseFloor: -60, // dB
      noiseLearningRate: 0.01,
      gateThreshold: -50, // dB
      gateHysteresis: 3, // dB

      // Spectral analysis parameters
      fftSize: 1024,
      hopSize: 512,
      windowType: "hann",

      // VAD parameters
      vadEnabled: true,
      speechThreshold: 0.6,
      minSpeechDuration: 0.1, // seconds
      minSilenceDuration: 0.05, // seconds

      // Adaptive filtering
      filterEnabled: true,
      filterStrength: 0.5,
      preserveSpeech: true,
      adaptationRate: 0.1,

      // Environmental classification
      environmentalAnalysis: true,
      noiseClassificationEnabled: true,
    };

    // Processing state
    this.state = {
      currentNoiseFloor: this.config.noiseFloor,
      speechProbability: 0,
      voiceActivityDetected: false,
      environmentType: "unknown",
      adaptationActive: false,
      lastVADUpdate: 0,
      consecutiveSpeechFrames: 0,
      consecutiveSilenceFrames: 0,
    };

    // Analysis buffers and processing
    this.buffers = {
      fftBuffer: null,
      windowBuffer: null,
      noiseProfile: null,
      speechProfile: null,
      spectralHistory: [],
      powerHistory: [],
      vadHistory: [],
    };

    // Performance optimization
    this.performance = {
      frameCount: 0,
      processingTime: 0,
      lastOptimization: 0,
      adaptiveProcessing: true,
      qualityLevel: "high",
    };

    // Noise classification
    this.classification = {
      environments: [
        "quiet",
        "indoor",
        "outdoor",
        "vehicular",
        "crowd",
        "machinery",
      ],
      currentEnvironment: "unknown",
      confidence: 0,
      features: new Map(),
      classifier: null,
    };

    this.bindMethods();
  }

  bindMethods() {
    this.processAudio = this.processAudio.bind(this);
    this.updateNoiseProfile = this.updateNoiseProfile.bind(this);
    this.detectVoiceActivity = this.detectVoiceActivity.bind(this);
    this.classifyEnvironment = this.classifyEnvironment.bind(this);
  }

  /**
   * Initialize the noise detector with all necessary components
   */
  async initialize(config = {}) {
    try {
      // Merge configuration
      this.config = { ...this.config, ...config };

      // Initialize FFT analyzer
      await this.initializeSpectralAnalyzer();

      // Initialize noise profiling
      this.initializeNoiseProfiler();

      // Initialize VAD system
      this.initializeVAD();

      // Initialize environmental classifier
      if (this.config.environmentalAnalysis) {
        await this.initializeEnvironmentalClassifier();
      }

      // Initialize performance monitoring
      this.initializePerformanceMonitoring();

      this.isInitialized = true;

      this.eventManager?.emitEvent("noiseDetectorInitialized", {
        config: this.config,
        timestamp: performance.now(),
      });

      return { success: true };
    } catch (error) {
      console.error("Failed to initialize NoiseDetector:", error);
      this.eventManager?.emitEvent("noiseDetectorError", {
        error: error.message,
        phase: "initialization",
      });
      throw error;
    }
  }

  /**
   * Initialize spectral analyzer for noise detection
   */
  async initializeSpectralAnalyzer() {
    const fftSize = this.config.fftSize;
    const hopSize = this.config.hopSize;

    // Initialize buffers
    this.buffers.fftBuffer = new Float32Array(fftSize);
    this.buffers.windowBuffer = new Float32Array(fftSize);
    this.buffers.noiseProfile = new Float32Array(fftSize / 2);
    this.buffers.speechProfile = new Float32Array(fftSize / 2);

    // Initialize spectral history
    this.buffers.spectralHistory = new Array(50)
      .fill(null)
      .map(() => new Float32Array(fftSize / 2));

    // Create window function (Hann window)
    this.createWindowFunction();

    // Initialize FFT processor (using Web Audio API's AnalyserNode concept)
    this.initializeFFTProcessor();
  }

  /**
   * Create window function for spectral analysis
   */
  createWindowFunction() {
    const size = this.config.fftSize;
    const window = this.buffers.windowBuffer;

    for (let i = 0; i < size; i++) {
      // Hann window
      window[i] = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (size - 1));
    }
  }

  /**
   * Initialize FFT processor
   */
  initializeFFTProcessor() {
    // Simplified FFT implementation for noise analysis
    // In production, would use optimized FFT library
    this.fftProcessor = {
      size: this.config.fftSize,
      halfSize: this.config.fftSize / 2,
      real: new Float32Array(this.config.fftSize),
      imag: new Float32Array(this.config.fftSize),
      magnitude: new Float32Array(this.config.fftSize / 2),
      phase: new Float32Array(this.config.fftSize / 2),
    };
  }

  /**
   * Initialize noise profiling system
   */
  initializeNoiseProfiler() {
    // Initialize noise profile with default values
    this.buffers.noiseProfile.fill(-80); // Start with very low noise floor

    // Initialize adaptation parameters
    this.noiseProfiler = {
      adaptationFrames: 0,
      requiredFrames: Math.ceil((this.sampleRate / this.config.hopSize) * 2), // 2 seconds
      isLearning: true,
      profileStability: 0,
      lastUpdate: 0,
    };
  }

  /**
   * Initialize Voice Activity Detection system
   */
  initializeVAD() {
    this.vad = {
      energyThreshold: -40, // dB
      spectralCentroidThreshold: 2000, // Hz
      zeroCrossingRateThreshold: 0.1,

      // Temporal smoothing
      smoothingFactor: 0.8,
      minSpeechFrames: Math.ceil(
        (this.config.minSpeechDuration * this.sampleRate) / this.config.hopSize
      ),
      minSilenceFrames: Math.ceil(
        (this.config.minSilenceDuration * this.sampleRate) / this.config.hopSize
      ),

      // Feature buffers
      energyHistory: new Array(20).fill(-80),
      spectralCentroidHistory: new Array(20).fill(0),
      zcrHistory: new Array(20).fill(0),

      // Current features
      currentEnergy: -80,
      currentSpectralCentroid: 0,
      currentZCR: 0,

      // Decision logic
      speechScore: 0,
      vadDecision: false,
      confidence: 0,
    };
  }

  /**
   * Initialize environmental noise classifier
   */
  async initializeEnvironmentalClassifier() {
    this.environmentalClassifier = {
      features: {
        spectralCentroid: 0,
        spectralRolloff: 0,
        spectralFlux: 0,
        mfcc: new Array(13).fill(0),
        zcr: 0,
        energy: 0,
        harmonicity: 0,
      },

      // Environment templates (simplified)
      templates: {
        quiet: { centroid: 1000, rolloff: 2000, energy: -50 },
        indoor: { centroid: 1500, rolloff: 3000, energy: -40 },
        outdoor: { centroid: 2000, rolloff: 4000, energy: -35 },
        vehicular: { centroid: 800, rolloff: 1500, energy: -25 },
        crowd: { centroid: 1800, rolloff: 3500, energy: -30 },
        machinery: { centroid: 1200, rolloff: 2500, energy: -20 },
      },

      classificationHistory: new Array(100).fill("unknown"),
      confidence: 0,
      updateInterval: 1000, // ms
    };
  }

  /**
   * Initialize performance monitoring
   */
  initializePerformanceMonitoring() {
    this.performanceMonitor = {
      frameProcessingTimes: new Array(100).fill(0),
      averageProcessingTime: 0,
      peakProcessingTime: 0,
      adaptiveQuality: true,
      qualityAdjustments: 0,
      lastQualityCheck: 0,
    };
  }

  /**
   * Main audio processing function
   */
  processAudio(inputBuffer, outputBuffer = null) {
    if (!this.isInitialized) {
      return inputBuffer;
    }

    const startTime = performance.now();

    try {
      // Perform spectral analysis
      this.performSpectralAnalysis(inputBuffer);

      // Update noise profile if in learning phase
      if (this.noiseProfiler.isLearning || this.state.adaptationActive) {
        this.updateNoiseProfile();
      }

      // Detect voice activity
      const vadResult = this.detectVoiceActivity();

      // Classify environment periodically
      if (
        this.config.environmentalAnalysis &&
        performance.now() - this.environmentalClassifier.lastUpdate >
          this.environmentalClassifier.updateInterval
      ) {
        this.classifyEnvironment();
      }

      // Apply noise reduction if enabled and not speech
      let processedBuffer = inputBuffer;
      if (this.config.filterEnabled) {
        processedBuffer = this.applyNoiseReduction(inputBuffer, vadResult);
      }

      // Update performance metrics
      this.updatePerformanceMetrics(performance.now() - startTime);

      // Emit processing event
      this.eventManager?.emitEvent("noiseProcessingComplete", {
        vadResult,
        noiseLevel: this.state.currentNoiseFloor,
        environment: this.classification.currentEnvironment,
        processingTime: performance.now() - startTime,
      });

      return outputBuffer
        ? this.copyToBuffer(processedBuffer, outputBuffer)
        : processedBuffer;
    } catch (error) {
      console.error("Error in noise processing:", error);
      this.eventManager?.emitEvent("noiseProcessingError", {
        error: error.message,
        timestamp: performance.now(),
      });
      return inputBuffer; // Return original on error
    }
  }

  /**
   * Perform spectral analysis on input buffer
   */
  performSpectralAnalysis(inputBuffer) {
    const fftSize = this.config.fftSize;
    const hopSize = this.config.hopSize;

    // Process overlapping windows
    for (
      let offset = 0;
      offset < inputBuffer.length - fftSize;
      offset += hopSize
    ) {
      // Copy windowed data to FFT buffer
      for (let i = 0; i < fftSize; i++) {
        this.buffers.fftBuffer[i] =
          inputBuffer[offset + i] * this.buffers.windowBuffer[i];
      }

      // Perform FFT (simplified implementation)
      this.performFFT(this.buffers.fftBuffer);

      // Calculate magnitude spectrum
      this.calculateMagnitudeSpectrum();

      // Update spectral history
      this.updateSpectralHistory();
    }
  }

  /**
   * Simplified FFT implementation
   */
  performFFT(buffer) {
    const N = buffer.length;
    const real = this.fftProcessor.real;
    const imag = this.fftProcessor.imag;

    // Copy input
    for (let i = 0; i < N; i++) {
      real[i] = buffer[i];
      imag[i] = 0;
    }

    // Simplified FFT (bit-reversal and butterfly operations)
    // In production, use optimized FFT library like Kiss FFT or FFTW
    this.simplifiedFFT(real, imag, N);
  }

  /**
   * Simplified FFT computation
   */
  simplifiedFFT(real, imag, N) {
    // Bit-reversal permutation
    let j = 0;
    for (let i = 1; i < N; i++) {
      let bit = N >> 1;
      while (j & bit) {
        j ^= bit;
        bit >>= 1;
      }
      j ^= bit;

      if (i < j) {
        [real[i], real[j]] = [real[j], real[i]];
        [imag[i], imag[j]] = [imag[j], imag[i]];
      }
    }

    // Butterfly operations
    for (let len = 2; len <= N; len <<= 1) {
      const wlen = (2 * Math.PI) / len;
      const wlen_cos = Math.cos(wlen);
      const wlen_sin = Math.sin(wlen);

      for (let i = 0; i < N; i += len) {
        let w_cos = 1;
        let w_sin = 0;

        for (let j = 0; j < len / 2; j++) {
          const u_real = real[i + j];
          const u_imag = imag[i + j];
          const v_real =
            real[i + j + len / 2] * w_cos - imag[i + j + len / 2] * w_sin;
          const v_imag =
            real[i + j + len / 2] * w_sin + imag[i + j + len / 2] * w_cos;

          real[i + j] = u_real + v_real;
          imag[i + j] = u_imag + v_imag;
          real[i + j + len / 2] = u_real - v_real;
          imag[i + j + len / 2] = u_imag - v_imag;

          const temp_cos = w_cos * wlen_cos - w_sin * wlen_sin;
          w_sin = w_cos * wlen_sin + w_sin * wlen_cos;
          w_cos = temp_cos;
        }
      }
    }
  }

  /**
   * Calculate magnitude spectrum from FFT results
   */
  calculateMagnitudeSpectrum() {
    const halfSize = this.fftProcessor.halfSize;
    const real = this.fftProcessor.real;
    const imag = this.fftProcessor.imag;
    const magnitude = this.fftProcessor.magnitude;

    for (let i = 0; i < halfSize; i++) {
      magnitude[i] = Math.sqrt(real[i] * real[i] + imag[i] * imag[i]);

      // Convert to dB
      magnitude[i] = 20 * Math.log10(Math.max(magnitude[i], 1e-10));
    }
  }

  /**
   * Update spectral history for analysis
   */
  updateSpectralHistory() {
    const history = this.buffers.spectralHistory;

    // Shift history
    for (let i = history.length - 1; i > 0; i--) {
      const temp = history[i];
      history[i] = history[i - 1];
      history[i - 1] = temp;
    }

    // Copy current spectrum
    const current = history[0];
    const magnitude = this.fftProcessor.magnitude;
    for (let i = 0; i < magnitude.length; i++) {
      current[i] = magnitude[i];
    }
  }

  /**
   * Update noise profile based on current spectral analysis
   */
  updateNoiseProfile() {
    const noiseProfile = this.buffers.noiseProfile;
    const magnitude = this.fftProcessor.magnitude;
    const learningRate = this.config.noiseLearningRate;

    // Update only during non-speech segments
    if (!this.state.voiceActivityDetected || this.noiseProfiler.isLearning) {
      for (let i = 0; i < noiseProfile.length; i++) {
        // Adaptive learning with minimum noise floor constraint
        const currentLevel = magnitude[i];
        const profileLevel = noiseProfile[i];

        if (currentLevel < profileLevel || this.noiseProfiler.isLearning) {
          noiseProfile[i] =
            profileLevel + learningRate * (currentLevel - profileLevel);
        }
      }

      this.noiseProfiler.adaptationFrames++;

      // Stop initial learning phase
      if (
        this.noiseProfiler.isLearning &&
        this.noiseProfiler.adaptationFrames >= this.noiseProfiler.requiredFrames
      ) {
        this.noiseProfiler.isLearning = false;
        this.eventManager?.emitEvent("noiseProfileLearned", {
          adaptationFrames: this.noiseProfiler.adaptationFrames,
          timestamp: performance.now(),
        });
      }
    }

    // Calculate overall noise floor
    this.calculateOverallNoiseFloor();
  }

  /**
   * Calculate overall noise floor from spectral profile
   */
  calculateOverallNoiseFloor() {
    const noiseProfile = this.buffers.noiseProfile;
    let sum = 0;
    let count = 0;

    // Calculate weighted average (emphasize speech frequencies)
    for (let i = 0; i < noiseProfile.length; i++) {
      const freq = (i * this.sampleRate) / (2 * noiseProfile.length);
      const weight = this.getFrequencyWeight(freq);

      sum += noiseProfile[i] * weight;
      count += weight;
    }

    this.state.currentNoiseFloor =
      count > 0 ? sum / count : this.config.noiseFloor;
  }

  /**
   * Get frequency weighting for noise floor calculation
   */
  getFrequencyWeight(frequency) {
    // Emphasize speech frequencies (300-3400 Hz)
    if (frequency >= 300 && frequency <= 3400) {
      return 1.0;
    } else if (frequency >= 100 && frequency <= 8000) {
      return 0.5;
    } else {
      return 0.1;
    }
  }

  /**
   * Detect voice activity using multiple features
   */
  detectVoiceActivity() {
    const vad = this.vad;
    const magnitude = this.fftProcessor.magnitude;

    // Calculate energy
    const energy = this.calculateSpectralEnergy(magnitude);
    vad.currentEnergy = energy;

    // Calculate spectral centroid
    const centroid = this.calculateSpectralCentroid(magnitude);
    vad.currentSpectralCentroid = centroid;

    // Calculate zero crossing rate (approximate from spectral data)
    const zcr = this.calculateZeroCrossingRate();
    vad.currentZCR = zcr;

    // Update feature histories
    this.updateVADHistory(energy, centroid, zcr);

    // Calculate speech probability
    const speechScore = this.calculateSpeechScore();
    vad.speechScore = speechScore;

    // Make VAD decision with temporal smoothing
    const vadDecision = this.makeVADDecision(speechScore);

    // Update state
    this.updateVADState(vadDecision);

    return {
      voiceActivityDetected: vadDecision,
      speechProbability: speechScore,
      confidence: vad.confidence,
      features: {
        energy: energy,
        spectralCentroid: centroid,
        zeroCrossingRate: zcr,
      },
    };
  }

  /**
   * Calculate spectral energy
   */
  calculateSpectralEnergy(magnitude) {
    let sum = 0;
    for (let i = 0; i < magnitude.length; i++) {
      sum += Math.pow(10, magnitude[i] / 10); // Convert from dB
    }
    return 10 * Math.log10(sum / magnitude.length); // Convert back to dB
  }

  /**
   * Calculate spectral centroid
   */
  calculateSpectralCentroid(magnitude) {
    let weightedSum = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < magnitude.length; i++) {
      const freq = (i * this.sampleRate) / (2 * magnitude.length);
      const mag = Math.pow(10, magnitude[i] / 20); // Convert from dB

      weightedSum += freq * mag;
      magnitudeSum += mag;
    }

    return magnitudeSum > 0 ? weightedSum / magnitudeSum : 0;
  }

  /**
   * Calculate zero crossing rate (approximate)
   */
  calculateZeroCrossingRate() {
    // Simplified ZCR calculation from high-frequency content
    const magnitude = this.fftProcessor.magnitude;
    const highFreqStart = Math.floor(magnitude.length * 0.7);

    let highFreqEnergy = 0;
    let totalEnergy = 0;

    for (let i = 0; i < magnitude.length; i++) {
      const energy = Math.pow(10, magnitude[i] / 10);
      totalEnergy += energy;

      if (i >= highFreqStart) {
        highFreqEnergy += energy;
      }
    }

    return totalEnergy > 0 ? highFreqEnergy / totalEnergy : 0;
  }

  /**
   * Update VAD feature histories
   */
  updateVADHistory(energy, centroid, zcr) {
    const vad = this.vad;

    // Shift histories
    vad.energyHistory.shift();
    vad.energyHistory.push(energy);

    vad.spectralCentroidHistory.shift();
    vad.spectralCentroidHistory.push(centroid);

    vad.zcrHistory.shift();
    vad.zcrHistory.push(zcr);
  }

  /**
   * Calculate overall speech score from features
   */
  calculateSpeechScore() {
    const vad = this.vad;

    // Energy-based score
    const avgEnergy =
      vad.energyHistory.reduce((a, b) => a + b, 0) / vad.energyHistory.length;
    const energyScore = Math.max(
      0,
      Math.min(1, (avgEnergy - this.state.currentNoiseFloor) / 20)
    );

    // Spectral centroid score
    const avgCentroid =
      vad.spectralCentroidHistory.reduce((a, b) => a + b, 0) /
      vad.spectralCentroidHistory.length;
    const centroidScore = Math.max(
      0,
      Math.min(1, avgCentroid / vad.spectralCentroidThreshold)
    );

    // Zero crossing rate score
    const avgZCR =
      vad.zcrHistory.reduce((a, b) => a + b, 0) / vad.zcrHistory.length;
    const zcrScore = Math.max(
      0,
      Math.min(1, avgZCR / vad.zeroCrossingRateThreshold)
    );

    // Weighted combination
    const speechScore =
      0.5 * energyScore + 0.3 * centroidScore + 0.2 * zcrScore;

    return speechScore;
  }

  /**
   * Make VAD decision with temporal smoothing
   */
  makeVADDecision(speechScore) {
    const vad = this.vad;

    // Apply smoothing
    const smoothedScore =
      vad.smoothingFactor * vad.speechScore +
      (1 - vad.smoothingFactor) * speechScore;

    // Threshold with hysteresis
    let decision = false;
    if (smoothedScore > this.config.speechThreshold) {
      decision = true;
    } else if (
      smoothedScore > this.config.speechThreshold * 0.7 &&
      vad.vadDecision
    ) {
      decision = true; // Hysteresis
    }

    // Temporal consistency check
    if (decision) {
      this.state.consecutiveSpeechFrames++;
      this.state.consecutiveSilenceFrames = 0;
    } else {
      this.state.consecutiveSilenceFrames++;
      this.state.consecutiveSpeechFrames = 0;
    }

    // Apply minimum duration constraints
    if (decision && this.state.consecutiveSpeechFrames < vad.minSpeechFrames) {
      decision = false;
    }
    if (
      !decision &&
      this.state.consecutiveSilenceFrames < vad.minSilenceFrames &&
      vad.vadDecision
    ) {
      decision = true;
    }

    return decision;
  }

  /**
   * Update VAD state
   */
  updateVADState(vadDecision) {
    const previousState = this.state.voiceActivityDetected;
    this.state.voiceActivityDetected = vadDecision;
    this.state.speechProbability = this.vad.speechScore;
    this.state.lastVADUpdate = performance.now();

    // Calculate confidence
    this.vad.confidence = Math.abs(
      this.vad.speechScore - this.config.speechThreshold
    );

    // Emit state change events
    if (previousState !== vadDecision) {
      this.eventManager?.emitEvent("vadStateChanged", {
        voiceActivity: vadDecision,
        speechProbability: this.state.speechProbability,
        confidence: this.vad.confidence,
        timestamp: performance.now(),
      });
    }
  }

  /**
   * Apply noise reduction to audio buffer
   */
  applyNoiseReduction(inputBuffer, vadResult) {
    const outputBuffer = new Float32Array(inputBuffer.length);
    const filterStrength = this.config.filterStrength;
    const preserveSpeech = this.config.preserveSpeech;

    // Apply spectral subtraction-based noise reduction
    // This is a simplified implementation

    // During speech, reduce noise reduction strength
    const adaptiveStrength =
      vadResult.voiceActivityDetected && preserveSpeech
        ? filterStrength * 0.3
        : filterStrength;

    // Apply adaptive filtering
    for (let i = 0; i < inputBuffer.length; i++) {
      const sample = inputBuffer[i];

      // Simple noise gate
      const sampleLevel = 20 * Math.log10(Math.abs(sample) + 1e-10);
      const gateThreshold =
        this.state.currentNoiseFloor + this.config.gateThreshold;

      if (sampleLevel < gateThreshold) {
        // Apply noise reduction
        outputBuffer[i] = sample * (1 - adaptiveStrength);
      } else {
        // Preserve signal above noise floor
        outputBuffer[i] = sample;
      }
    }

    return outputBuffer;
  }

  /**
   * Classify environmental noise
   */
  classifyEnvironment() {
    const features = this.environmentalClassifier.features;
    const magnitude = this.fftProcessor.magnitude;

    // Calculate environmental features
    features.spectralCentroid = this.calculateSpectralCentroid(magnitude);
    features.spectralRolloff = this.calculateSpectralRolloff(magnitude);
    features.energy = this.calculateSpectralEnergy(magnitude);
    features.zcr = this.calculateZeroCrossingRate();

    // Classify based on templates (simplified)
    let bestMatch = "unknown";
    let bestScore = -Infinity;

    const templates = this.environmentalClassifier.templates;
    for (const [environment, template] of Object.entries(templates)) {
      const score = this.calculateEnvironmentScore(features, template);
      if (score > bestScore) {
        bestScore = score;
        bestMatch = environment;
      }
    }

    // Update classification
    const confidence = Math.max(0, Math.min(1, bestScore / 100));
    this.classification.currentEnvironment = bestMatch;
    this.classification.confidence = confidence;

    // Update history
    this.environmentalClassifier.classificationHistory.shift();
    this.environmentalClassifier.classificationHistory.push(bestMatch);

    this.environmentalClassifier.lastUpdate = performance.now();

    // Emit classification event
    this.eventManager?.emitEvent("environmentClassified", {
      environment: bestMatch,
      confidence: confidence,
      features: { ...features },
      timestamp: performance.now(),
    });
  }

  /**
   * Calculate spectral rolloff
   */
  calculateSpectralRolloff(magnitude) {
    const totalEnergy = magnitude.reduce(
      (sum, mag) => sum + Math.pow(10, mag / 10),
      0
    );
    const threshold = totalEnergy * 0.85; // 85% rolloff

    let cumulativeEnergy = 0;
    for (let i = 0; i < magnitude.length; i++) {
      cumulativeEnergy += Math.pow(10, magnitude[i] / 10);
      if (cumulativeEnergy >= threshold) {
        return (i * this.sampleRate) / (2 * magnitude.length);
      }
    }

    return this.sampleRate / 2; // Nyquist frequency
  }

  /**
   * Calculate environment classification score
   */
  calculateEnvironmentScore(features, template) {
    // Simple distance-based scoring
    const centroidDiff =
      Math.abs(features.spectralCentroid - template.centroid) /
      template.centroid;
    const rolloffDiff =
      Math.abs(features.spectralRolloff - template.rolloff) / template.rolloff;
    const energyDiff =
      Math.abs(features.energy - template.energy) / Math.abs(template.energy);

    const score = 100 - (centroidDiff + rolloffDiff + energyDiff) * 33.33;
    return score;
  }

  /**
   * Update performance metrics
   */
  updatePerformanceMetrics(processingTime) {
    const monitor = this.performanceMonitor;

    // Update processing time history
    monitor.frameProcessingTimes.shift();
    monitor.frameProcessingTimes.push(processingTime);

    // Calculate averages
    monitor.averageProcessingTime =
      monitor.frameProcessingTimes.reduce((a, b) => a + b, 0) /
      monitor.frameProcessingTimes.length;
    monitor.peakProcessingTime = Math.max(...monitor.frameProcessingTimes);

    // Adaptive quality adjustment
    if (
      monitor.adaptiveQuality &&
      performance.now() - monitor.lastQualityCheck > 5000
    ) {
      // Check every 5 seconds
      this.adjustProcessingQuality();
      monitor.lastQualityCheck = performance.now();
    }

    this.performance.frameCount++;
    this.performance.processingTime += processingTime;
  }

  /**
   * Adjust processing quality based on performance
   */
  adjustProcessingQuality() {
    const monitor = this.performanceMonitor;
    const maxProcessingTime =
      (1000 / this.sampleRate) * this.config.hopSize * 0.8; // 80% of available time

    if (monitor.averageProcessingTime > maxProcessingTime) {
      // Reduce quality
      if (this.performance.qualityLevel === "high") {
        this.performance.qualityLevel = "medium";
        this.config.fftSize = Math.max(512, this.config.fftSize / 2);
        monitor.qualityAdjustments++;
      } else if (this.performance.qualityLevel === "medium") {
        this.performance.qualityLevel = "low";
        this.config.environmentalAnalysis = false;
        monitor.qualityAdjustments++;
      }

      this.eventManager?.emitEvent("qualityAdjusted", {
        newQuality: this.performance.qualityLevel,
        reason: "performance",
        averageProcessingTime: monitor.averageProcessingTime,
      });
    }
  }

  /**
   * Copy processed audio to output buffer
   */
  copyToBuffer(sourceBuffer, targetBuffer) {
    const length = Math.min(sourceBuffer.length, targetBuffer.length);
    for (let i = 0; i < length; i++) {
      targetBuffer[i] = sourceBuffer[i];
    }
    return targetBuffer;
  }

  /**
   * Get current noise detection state
   */
  getState() {
    return {
      isInitialized: this.isInitialized,
      noiseFloor: this.state.currentNoiseFloor,
      voiceActivity: this.state.voiceActivityDetected,
      speechProbability: this.state.speechProbability,
      environment: this.classification.currentEnvironment,
      environmentConfidence: this.classification.confidence,
      vadConfidence: this.vad.confidence,
      adaptationActive: this.state.adaptationActive,
      qualityLevel: this.performance.qualityLevel,
      frameCount: this.performance.frameCount,
      averageProcessingTime: this.performanceMonitor.averageProcessingTime,
    };
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    const oldConfig = { ...this.config };
    this.config = { ...this.config, ...newConfig };

    // Reinitialize if necessary
    if (newConfig.fftSize && newConfig.fftSize !== oldConfig.fftSize) {
      this.initializeSpectralAnalyzer();
    }

    this.eventManager?.emitEvent("noiseDetectorConfigUpdated", {
      oldConfig,
      newConfig: this.config,
      timestamp: performance.now(),
    });
  }

  /**
   * Reset noise profile and learning
   */
  resetNoiseProfile() {
    this.buffers.noiseProfile.fill(-80);
    this.noiseProfiler.isLearning = true;
    this.noiseProfiler.adaptationFrames = 0;
    this.state.adaptationActive = true;

    this.eventManager?.emitEvent("noiseProfileReset", {
      timestamp: performance.now(),
    });
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    // Clear buffers
    if (this.buffers.fftBuffer) this.buffers.fftBuffer.fill(0);
    if (this.buffers.noiseProfile) this.buffers.noiseProfile.fill(0);
    if (this.buffers.spectralHistory) {
      this.buffers.spectralHistory.forEach((buffer) => buffer.fill(0));
    }

    // Reset state
    this.isInitialized = false;
    this.performance.frameCount = 0;
    this.performance.processingTime = 0;

    this.eventManager?.emitEvent("noiseDetectorCleanup", {
      timestamp: performance.now(),
    });
  }
}

export default NoiseDetector;
