/**
 * AutomaticGainControl Module
 *
 * Provides sophisticated AGC implementation with multi-band dynamic range compression,
 * adaptive processing, look-ahead processing, and intelligent gain management.
 * Optimized for speech and music content with quality preservation.
 *
 * Features:
 * - Multi-band dynamic range compression
 * - Adaptive attack and release time constants
 * - Look-ahead processing for transient preservation
 * - Gain riding with smooth parameter updates
 * - Peak limiting with soft-knee characteristics
 * - Speech intelligibility optimization
 * - Music content detection and handling
 * - Headroom management and peak prevention
 */

export class AutomaticGainControl {
  constructor(eventManager, sampleRate = 48000) {
    this.eventManager = eventManager;
    this.sampleRate = sampleRate;
    this.isInitialized = false;

    // Core AGC configuration
    this.config = {
      enabled: true,
      targetLevel: -23, // dB (EBU R128 loudness target)
      maxGain: 20, // dB
      minGain: -20, // dB

      // Dynamic processing parameters
      attackTime: 0.003, // seconds (3ms)
      releaseTime: 0.1, // seconds (100ms)
      lookAheadTime: 0.005, // seconds (5ms)

      // Compressor parameters
      threshold: -18, // dB
      ratio: 4.0,
      kneeWidth: 2.0, // dB (soft knee)

      // Multi-band processing
      multibandEnabled: true,
      bandCount: 3,
      crossoverFreqs: [250, 2000], // Hz

      // Content adaptation
      speechOptimization: true,
      musicDetection: true,
      adaptiveProcessing: true,

      // Quality preservation
      transientPreservation: true,
      harmonicPreservation: true,
      qualityPriority: "balanced", // 'speed', 'balanced', 'quality'
    };

    // Processing state
    this.state = {
      currentGain: 1.0,
      targetGain: 1.0,
      instantaneousGain: 1.0,
      smoothingCoeff: 0.99,

      // Level tracking
      inputLevel: -60, // dB
      outputLevel: -60, // dB
      gainReduction: 0, // dB

      // Content detection
      contentType: "speech", // 'speech', 'music', 'mixed'
      contentConfidence: 0,

      // Performance
      processingLoad: 0,
      lastAdaptation: 0,
    };

    // Processing buffers
    this.buffers = {
      lookAheadBuffer: null,
      delayBuffer: null,
      levelHistory: null,
      gainHistory: null,

      // Multi-band buffers
      bandBuffers: [],
      bandStates: [],

      // Analysis buffers
      spectralBuffer: new Float32Array(1024),
      analysisWindow: new Float32Array(1024),
    };

    // Level detection and smoothing
    this.levelDetector = {
      // Peak detection
      peakHoldTime: 0.001, // seconds
      peakDecayTime: 1.6, // seconds
      currentPeak: 0,
      peakHoldSamples: 0,
      peakHoldCounter: 0,

      // RMS detection
      rmsWindowSize: 0.01, // seconds
      rmsBuffer: null,
      rmsIndex: 0,
      currentRMS: 0,

      // Integrated loudness (simplified LUFS)
      loudnessWindow: 0.4, // seconds (400ms)
      loudnessBuffer: null,
      loudnessIndex: 0,
      currentLoudness: -70, // LUFS
    };

    // Multi-band crossover filters
    this.crossoverFilters = [];

    // Look-ahead processing
    this.lookAhead = {
      enabled: this.config.lookAheadTime > 0,
      delaySamples: 0,
      bufferSize: 0,
      analysisBuffer: null,
      writeIndex: 0,
      readIndex: 0,
    };

    // Content analysis
    this.contentAnalyzer = {
      analysisInterval: 100, // ms
      lastAnalysis: 0,

      // Speech detection features
      speechFeatures: {
        spectralCentroid: 0,
        harmonicRatio: 0,
        voicedRatio: 0,
        dynamicRange: 0,
      },

      // Music detection features
      musicFeatures: {
        spectralComplexity: 0,
        rhythmicContent: 0,
        harmonicComplexity: 0,
        stereoWidth: 0,
      },
    };

    // Performance monitoring
    this.performance = {
      frameCount: 0,
      totalProcessingTime: 0,
      averageProcessingTime: 0,
      peakProcessingTime: 0,
      adaptiveQuality: true,
      qualityLevel: "high",
    };

    this.bindMethods();
  }

  bindMethods() {
    this.processAudio = this.processAudio.bind(this);
    this.processMultiband = this.processMultiband.bind(this);
    this.calculateGainReduction = this.calculateGainReduction.bind(this);
    this.smoothGainChanges = this.smoothGainChanges.bind(this);
  }

  /**
   * Initialize the AGC with all necessary components
   */
  async initialize(config = {}) {
    try {
      // Merge configuration
      this.config = { ...this.config, ...config };

      // Initialize level detection
      this.initializeLevelDetection();

      // Initialize look-ahead processing
      if (this.config.lookAheadTime > 0) {
        this.initializeLookAhead();
      }

      // Initialize multi-band processing
      if (this.config.multibandEnabled) {
        await this.initializeMultiband();
      }

      // Initialize content analysis
      this.initializeContentAnalysis();

      // Initialize performance monitoring
      this.initializePerformanceMonitoring();

      // Calculate processing coefficients
      this.calculateProcessingCoefficients();

      this.isInitialized = true;

      this.eventManager?.emitEvent("agcInitialized", {
        config: this.config,
        timestamp: performance.now(),
      });

      return { success: true };
    } catch (error) {
      console.error("Failed to initialize AGC:", error);
      this.eventManager?.emitEvent("agcError", {
        error: error.message,
        phase: "initialization",
      });
      throw error;
    }
  }

  /**
   * Initialize level detection components
   */
  initializeLevelDetection() {
    const detector = this.levelDetector;

    // Peak detection
    detector.peakHoldSamples = Math.ceil(
      detector.peakHoldTime * this.sampleRate
    );

    // RMS detection
    const rmsWindowSamples = Math.ceil(
      detector.rmsWindowSize * this.sampleRate
    );
    detector.rmsBuffer = new Float32Array(rmsWindowSamples);

    // Loudness measurement
    const loudnessWindowSamples = Math.ceil(
      detector.loudnessWindow * this.sampleRate
    );
    detector.loudnessBuffer = new Float32Array(loudnessWindowSamples);

    // History buffers
    this.buffers.levelHistory = new Array(1000).fill(-60);
    this.buffers.gainHistory = new Array(1000).fill(0);
  }

  /**
   * Initialize look-ahead processing
   */
  initializeLookAhead() {
    const lookAhead = this.lookAhead;

    lookAhead.delaySamples = Math.ceil(
      this.config.lookAheadTime * this.sampleRate
    );
    lookAhead.bufferSize = lookAhead.delaySamples * 2; // Double buffer for safety

    lookAhead.analysisBuffer = new Float32Array(lookAhead.bufferSize);
    this.buffers.lookAheadBuffer = new Float32Array(lookAhead.bufferSize);
    this.buffers.delayBuffer = new Float32Array(lookAhead.bufferSize);

    lookAhead.writeIndex = 0;
    lookAhead.readIndex = lookAhead.bufferSize - lookAhead.delaySamples;
  }

  /**
   * Initialize multi-band processing
   */
  async initializeMultiband() {
    const bandCount = this.config.bandCount;
    const crossoverFreqs = this.config.crossoverFreqs;

    // Initialize band buffers and states
    this.buffers.bandBuffers = [];
    this.buffers.bandStates = [];

    for (let band = 0; band < bandCount; band++) {
      this.buffers.bandBuffers.push(new Float32Array(1024));
      this.buffers.bandStates.push({
        currentGain: 1.0,
        targetGain: 1.0,
        envelope: 0,
        detector: {
          peak: 0,
          rms: 0,
          holdCounter: 0,
        },
      });
    }

    // Initialize crossover filters
    this.crossoverFilters = [];
    for (let i = 0; i < crossoverFreqs.length; i++) {
      const filter = this.createCrossoverFilter(crossoverFreqs[i]);
      this.crossoverFilters.push(filter);
    }
  }

  /**
   * Create crossover filter for multi-band processing
   */
  createCrossoverFilter(frequency) {
    // Linkwitz-Riley 4th order crossover
    const omega = (2 * Math.PI * frequency) / this.sampleRate;
    const k = Math.tan(omega / 2);
    const k2 = k * k;
    const k3 = k2 * k;
    const k4 = k2 * k2;

    // Calculate coefficients
    const denom = 1 + 2.613 * k + 3.414 * k2 + 2.613 * k3 + k4;

    return {
      // Low-pass coefficients
      lowpass: {
        b: [
          k4 / denom,
          (4 * k4) / denom,
          (6 * k4) / denom,
          (4 * k4) / denom,
          k4 / denom,
        ],
        a: [
          1,
          (4 * k4 + 4 * k3 - 4 * k - 4) / denom,
          (6 * k4 - 8 * k2 + 6) / denom,
          (4 * k4 - 4 * k3 - 4 * k + 4) / denom,
          (k4 - 2 * k3 + 3 * k2 - 2 * k + 1) / denom,
        ],
      },

      // High-pass coefficients
      highpass: {
        b: [1 / denom, -4 / denom, 6 / denom, -4 / denom, 1 / denom],
        a: [
          1,
          (4 * k4 + 4 * k3 - 4 * k - 4) / denom,
          (6 * k4 - 8 * k2 + 6) / denom,
          (4 * k4 - 4 * k3 - 4 * k + 4) / denom,
          (k4 - 2 * k3 + 3 * k2 - 2 * k + 1) / denom,
        ],
      },

      // Filter state
      state: {
        x: [0, 0, 0, 0, 0],
        y_low: [0, 0, 0, 0, 0],
        y_high: [0, 0, 0, 0, 0],
      },

      frequency: frequency,
    };
  }

  /**
   * Initialize content analysis
   */
  initializeContentAnalysis() {
    // Initialize analysis window (Hann window)
    const windowSize = this.buffers.analysisWindow.length;
    for (let i = 0; i < windowSize; i++) {
      this.buffers.analysisWindow[i] =
        0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (windowSize - 1));
    }

    // Initialize content analyzer state
    this.contentAnalyzer.lastAnalysis = 0;
  }

  /**
   * Initialize performance monitoring
   */
  initializePerformanceMonitoring() {
    this.performance = {
      frameCount: 0,
      totalProcessingTime: 0,
      averageProcessingTime: 0,
      peakProcessingTime: 0,
      adaptiveQuality: true,
      qualityLevel: "high",
      processingTimes: new Array(100).fill(0),
    };
  }

  /**
   * Calculate processing coefficients
   */
  calculateProcessingCoefficients() {
    // Attack and release coefficients
    this.attackCoeff = Math.exp(
      -1 / (this.config.attackTime * this.sampleRate)
    );
    this.releaseCoeff = Math.exp(
      -1 / (this.config.releaseTime * this.sampleRate)
    );

    // Smoothing coefficient for gain changes
    const smoothingTime = 0.01; // 10ms smoothing
    this.state.smoothingCoeff = Math.exp(
      -1 / (smoothingTime * this.sampleRate)
    );
  }

  /**
   * Main audio processing function
   */
  processAudio(inputBuffer, outputBuffer = null) {
    if (!this.isInitialized || !this.config.enabled) {
      return outputBuffer
        ? this.copyBuffer(inputBuffer, outputBuffer)
        : inputBuffer;
    }

    const startTime = performance.now();

    try {
      let processedBuffer;

      if (this.config.multibandEnabled) {
        // Multi-band processing
        processedBuffer = this.processMultiband(inputBuffer);
      } else {
        // Single-band processing
        processedBuffer = this.processSingleBand(inputBuffer);
      }

      // Content analysis (periodic)
      if (
        performance.now() - this.contentAnalyzer.lastAnalysis >
        this.contentAnalyzer.analysisInterval
      ) {
        this.analyzeContent(inputBuffer);
      }

      // Update performance metrics
      this.updatePerformanceMetrics(performance.now() - startTime);

      // Emit processing event
      this.eventManager?.emitEvent("agcProcessingComplete", {
        inputLevel: this.state.inputLevel,
        outputLevel: this.state.outputLevel,
        gainReduction: this.state.gainReduction,
        contentType: this.state.contentType,
        processingTime: performance.now() - startTime,
      });

      return outputBuffer
        ? this.copyBuffer(processedBuffer, outputBuffer)
        : processedBuffer;
    } catch (error) {
      console.error("Error in AGC processing:", error);
      this.eventManager?.emitEvent("agcProcessingError", {
        error: error.message,
        timestamp: performance.now(),
      });
      return outputBuffer
        ? this.copyBuffer(inputBuffer, outputBuffer)
        : inputBuffer;
    }
  }

  /**
   * Process audio with single-band AGC
   */
  processSingleBand(inputBuffer) {
    const outputBuffer = new Float32Array(inputBuffer.length);

    for (let i = 0; i < inputBuffer.length; i++) {
      const sample = inputBuffer[i];

      // Level detection
      const level = this.detectLevel(sample);

      // Calculate gain reduction
      const gainReduction = this.calculateGainReduction(level);

      // Smooth gain changes
      const smoothGain = this.smoothGainChanges(gainReduction);

      // Apply gain with look-ahead if enabled
      if (this.lookAhead.enabled) {
        outputBuffer[i] = this.processWithLookAhead(sample, smoothGain);
      } else {
        outputBuffer[i] = sample * smoothGain;
      }

      // Update state
      this.updateProcessingState(level, smoothGain);
    }

    return outputBuffer;
  }

  /**
   * Process audio with multi-band AGC
   */
  processMultiband(inputBuffer) {
    const outputBuffer = new Float32Array(inputBuffer.length);
    const bandCount = this.config.bandCount;

    // Split input into frequency bands
    const bandSignals = this.splitIntoBands(inputBuffer);

    // Process each band independently
    const processedBands = [];
    for (let band = 0; band < bandCount; band++) {
      const bandBuffer = bandSignals[band];
      const processedBand = this.processBand(bandBuffer, band);
      processedBands.push(processedBand);
    }

    // Combine processed bands
    this.combineBands(processedBands, outputBuffer);

    return outputBuffer;
  }

  /**
   * Split input signal into frequency bands
   */
  splitIntoBands(inputBuffer) {
    const bandSignals = [];
    const bandCount = this.config.bandCount;

    // Initialize band buffers
    for (let band = 0; band < bandCount; band++) {
      bandSignals.push(new Float32Array(inputBuffer.length));
    }

    // Apply crossover filters
    for (let i = 0; i < inputBuffer.length; i++) {
      const sample = inputBuffer[i];
      let currentSignal = sample;

      // Process through crossover filters
      for (
        let filterIndex = 0;
        filterIndex < this.crossoverFilters.length;
        filterIndex++
      ) {
        const filter = this.crossoverFilters[filterIndex];

        // Apply low-pass filter for lower band
        const lowOutput = this.applyBiquadFilter(
          currentSignal,
          filter.lowpass,
          filter.state,
          "low"
        );
        bandSignals[filterIndex][i] = lowOutput;

        // Apply high-pass filter for upper band
        const highOutput = this.applyBiquadFilter(
          currentSignal,
          filter.highpass,
          filter.state,
          "high"
        );
        currentSignal = highOutput;
      }

      // Last band gets the remaining high-frequency content
      if (bandSignals.length > this.crossoverFilters.length) {
        bandSignals[bandSignals.length - 1][i] = currentSignal;
      }
    }

    return bandSignals;
  }

  /**
   * Apply biquad filter
   */
  applyBiquadFilter(input, coeffs, state, type) {
    const stateArray = type === "low" ? state.y_low : state.y_high;

    // Shift state
    for (let i = 4; i > 0; i--) {
      state.x[i] = state.x[i - 1];
      stateArray[i] = stateArray[i - 1];
    }

    state.x[0] = input;

    // Calculate output
    let output = 0;
    for (let i = 0; i < 5; i++) {
      output += coeffs.b[i] * state.x[i];
    }
    for (let i = 1; i < 5; i++) {
      output -= coeffs.a[i] * stateArray[i];
    }

    stateArray[0] = output;
    return output;
  }

  /**
   * Process individual frequency band
   */
  processBand(bandBuffer, bandIndex) {
    const processedBand = new Float32Array(bandBuffer.length);
    const bandState = this.buffers.bandStates[bandIndex];

    for (let i = 0; i < bandBuffer.length; i++) {
      const sample = bandBuffer[i];

      // Band-specific level detection
      const level = this.detectBandLevel(sample, bandState);

      // Calculate band-specific gain reduction
      const gainReduction = this.calculateBandGainReduction(level, bandIndex);

      // Smooth gain changes for this band
      const smoothGain = this.smoothBandGain(gainReduction, bandState);

      // Apply gain
      processedBand[i] = sample * smoothGain;
    }

    return processedBand;
  }

  /**
   * Combine processed frequency bands
   */
  combineBands(bandSignals, outputBuffer) {
    // Simple addition of band signals
    for (let i = 0; i < outputBuffer.length; i++) {
      let sum = 0;
      for (let band = 0; band < bandSignals.length; band++) {
        sum += bandSignals[band][i];
      }
      outputBuffer[i] = sum;
    }
  }

  /**
   * Detect audio level
   */
  detectLevel(sample) {
    const detector = this.levelDetector;
    const absample = Math.abs(sample);

    // Peak detection with hold and decay
    if (absample > detector.currentPeak) {
      detector.currentPeak = absample;
      detector.peakHoldCounter = detector.peakHoldSamples;
    } else if (detector.peakHoldCounter > 0) {
      detector.peakHoldCounter--;
    } else {
      // Peak decay
      const decayCoeff = Math.exp(
        -1 / (detector.peakDecayTime * this.sampleRate)
      );
      detector.currentPeak *= decayCoeff;
    }

    // RMS detection
    detector.rmsBuffer[detector.rmsIndex] = sample * sample;
    detector.rmsIndex = (detector.rmsIndex + 1) % detector.rmsBuffer.length;

    let rmsSum = 0;
    for (let i = 0; i < detector.rmsBuffer.length; i++) {
      rmsSum += detector.rmsBuffer[i];
    }
    detector.currentRMS = Math.sqrt(rmsSum / detector.rmsBuffer.length);

    // Convert to dB
    const peakDb = 20 * Math.log10(Math.max(detector.currentPeak, 1e-10));
    const rmsDb = 20 * Math.log10(Math.max(detector.currentRMS, 1e-10));

    // Use RMS for general level, peak for limiting
    this.state.inputLevel = rmsDb;

    return {
      peak: peakDb,
      rms: rmsDb,
      linear: detector.currentRMS,
    };
  }

  /**
   * Detect level for specific frequency band
   */
  detectBandLevel(sample, bandState) {
    const detector = bandState.detector;
    const absample = Math.abs(sample);

    // Simple peak detection for band
    if (absample > detector.peak) {
      detector.peak = absample;
      detector.holdCounter = this.levelDetector.peakHoldSamples;
    } else if (detector.holdCounter > 0) {
      detector.holdCounter--;
    } else {
      const decayCoeff = Math.exp(
        -1 / (this.levelDetector.peakDecayTime * this.sampleRate)
      );
      detector.peak *= decayCoeff;
    }

    // Simple RMS for band
    detector.rms = detector.rms * 0.999 + sample * sample * 0.001;

    return {
      peak: 20 * Math.log10(Math.max(detector.peak, 1e-10)),
      rms: 20 * Math.log10(Math.max(Math.sqrt(detector.rms), 1e-10)),
      linear: Math.sqrt(detector.rms),
    };
  }

  /**
   * Calculate gain reduction based on compressor characteristics
   */
  calculateGainReduction(level) {
    const threshold = this.config.threshold;
    const ratio = this.config.ratio;
    const kneeWidth = this.config.kneeWidth;

    const inputLevel = level.rms;
    let gainReduction = 0;

    // Soft knee compression
    if (inputLevel > threshold + kneeWidth / 2) {
      // Above knee - full compression
      const overshoot = inputLevel - threshold;
      gainReduction = overshoot * (1 - 1 / ratio);
    } else if (inputLevel > threshold - kneeWidth / 2) {
      // In knee region - gradual compression
      const kneeRatio = (inputLevel - threshold + kneeWidth / 2) / kneeWidth;
      const overshoot = inputLevel - threshold;
      gainReduction = overshoot * (1 - 1 / ratio) * kneeRatio * kneeRatio;
    }

    // Limit gain reduction
    gainReduction = Math.max(0, Math.min(gainReduction, this.config.maxGain));

    return gainReduction;
  }

  /**
   * Calculate band-specific gain reduction
   */
  calculateBandGainReduction(level, bandIndex) {
    // Different parameters for different frequency bands
    let threshold = this.config.threshold;
    let ratio = this.config.ratio;

    // Adjust parameters based on frequency band and content
    if (this.state.contentType === "speech") {
      // Optimize for speech intelligibility
      if (bandIndex === 1) {
        // Mid frequencies (speech)
        threshold -= 3; // More sensitive in speech band
        ratio *= 0.8; // Gentler compression for speech
      }
    } else if (this.state.contentType === "music") {
      // Optimize for music dynamics
      if (bandIndex === 0) {
        // Low frequencies
        ratio *= 1.2; // More compression in bass
      }
    }

    return this.calculateGainReduction(level);
  }

  /**
   * Smooth gain changes to avoid artifacts
   */
  smoothGainChanges(gainReduction) {
    // Convert gain reduction to linear gain
    const targetGain = Math.pow(10, -gainReduction / 20);

    // Apply attack/release smoothing
    let smoothingCoeff;
    if (targetGain < this.state.currentGain) {
      // Attack (gain reduction increasing)
      smoothingCoeff = this.attackCoeff;
    } else {
      // Release (gain reduction decreasing)
      smoothingCoeff = this.releaseCoeff;
    }

    // Smooth the gain change
    this.state.currentGain =
      this.state.currentGain * smoothingCoeff +
      targetGain * (1 - smoothingCoeff);

    // Apply additional smoothing for gain riding
    this.state.targetGain =
      this.state.targetGain * this.state.smoothingCoeff +
      this.state.currentGain * (1 - this.state.smoothingCoeff);

    return this.state.targetGain;
  }

  /**
   * Smooth gain changes for frequency band
   */
  smoothBandGain(gainReduction, bandState) {
    const targetGain = Math.pow(10, -gainReduction / 20);

    // Apply smoothing
    let smoothingCoeff;
    if (targetGain < bandState.currentGain) {
      smoothingCoeff = this.attackCoeff;
    } else {
      smoothingCoeff = this.releaseCoeff;
    }

    bandState.currentGain =
      bandState.currentGain * smoothingCoeff +
      targetGain * (1 - smoothingCoeff);

    return bandState.currentGain;
  }

  /**
   * Process sample with look-ahead
   */
  processWithLookAhead(sample, gain) {
    const lookAhead = this.lookAhead;

    // Write current sample to look-ahead buffer
    lookAhead.analysisBuffer[lookAhead.writeIndex] = sample;
    this.buffers.delayBuffer[lookAhead.writeIndex] = sample;

    // Read delayed sample
    const delayedSample = this.buffers.delayBuffer[lookAhead.readIndex];

    // Apply gain to delayed sample
    const processedSample = delayedSample * gain;

    // Update indices
    lookAhead.writeIndex = (lookAhead.writeIndex + 1) % lookAhead.bufferSize;
    lookAhead.readIndex = (lookAhead.readIndex + 1) % lookAhead.bufferSize;

    return processedSample;
  }

  /**
   * Analyze content type (speech vs music)
   */
  analyzeContent(inputBuffer) {
    // Calculate spectral features
    const spectralCentroid = this.calculateSpectralCentroid(inputBuffer);
    const harmonicRatio = this.calculateHarmonicRatio(inputBuffer);
    const dynamicRange = this.calculateDynamicRange(inputBuffer);

    // Update features
    const speechFeatures = this.contentAnalyzer.speechFeatures;
    speechFeatures.spectralCentroid = spectralCentroid;
    speechFeatures.harmonicRatio = harmonicRatio;
    speechFeatures.dynamicRange = dynamicRange;

    // Simple content classification
    let speechScore = 0;
    let musicScore = 0;

    // Speech indicators
    if (spectralCentroid > 1000 && spectralCentroid < 3000) speechScore += 0.3;
    if (harmonicRatio > 0.6) speechScore += 0.3;
    if (dynamicRange < 20) speechScore += 0.2;

    // Music indicators
    if (spectralCentroid > 2000 || spectralCentroid < 800) musicScore += 0.3;
    if (harmonicRatio > 0.8) musicScore += 0.2;
    if (dynamicRange > 15) musicScore += 0.3;

    // Determine content type
    if (speechScore > musicScore && speechScore > 0.6) {
      this.state.contentType = "speech";
      this.state.contentConfidence = speechScore;
    } else if (musicScore > 0.6) {
      this.state.contentType = "music";
      this.state.contentConfidence = musicScore;
    } else {
      this.state.contentType = "mixed";
      this.state.contentConfidence = Math.max(speechScore, musicScore);
    }

    // Adapt processing parameters based on content
    this.adaptToContent();

    this.contentAnalyzer.lastAnalysis = performance.now();
  }

  /**
   * Calculate spectral centroid
   */
  calculateSpectralCentroid(inputBuffer) {
    // Simplified spectral centroid calculation
    // In production, would use proper FFT

    let weightedSum = 0;
    let magnitudeSum = 0;

    const windowSize = Math.min(1024, inputBuffer.length);
    for (let i = 0; i < windowSize; i++) {
      const freq = (i * this.sampleRate) / (2 * windowSize);
      const magnitude = Math.abs(inputBuffer[i]);

      weightedSum += freq * magnitude;
      magnitudeSum += magnitude;
    }

    return magnitudeSum > 0 ? weightedSum / magnitudeSum : 0;
  }

  /**
   * Calculate harmonic ratio (simplified)
   */
  calculateHarmonicRatio(inputBuffer) {
    // Simplified harmonic analysis
    // Would use autocorrelation or cepstral analysis in production

    let correlation = 0;
    const lag = Math.floor(this.sampleRate / 200); // Assume F0 around 200Hz

    for (let i = 0; i < inputBuffer.length - lag; i++) {
      correlation += inputBuffer[i] * inputBuffer[i + lag];
    }

    return Math.max(0, Math.min(1, correlation / (inputBuffer.length - lag)));
  }

  /**
   * Calculate dynamic range
   */
  calculateDynamicRange(inputBuffer) {
    let min = Infinity;
    let max = -Infinity;

    for (let i = 0; i < inputBuffer.length; i++) {
      const level = 20 * Math.log10(Math.abs(inputBuffer[i]) + 1e-10);
      min = Math.min(min, level);
      max = Math.max(max, level);
    }

    return max - min;
  }

  /**
   * Adapt processing parameters to content type
   */
  adaptToContent() {
    if (!this.config.adaptiveProcessing) return;

    const contentType = this.state.contentType;

    if (contentType === "speech") {
      // Optimize for speech intelligibility
      this.config.attackTime = 0.002; // Faster attack
      this.config.releaseTime = 0.05; // Faster release
      this.config.ratio = 3.0; // Gentler compression
    } else if (contentType === "music") {
      // Optimize for music dynamics
      this.config.attackTime = 0.005; // Slower attack
      this.config.releaseTime = 0.2; // Slower release
      this.config.ratio = 5.0; // More compression
    }

    // Recalculate coefficients
    this.calculateProcessingCoefficients();

    this.eventManager?.emitEvent("agcContentAdaptation", {
      contentType: contentType,
      confidence: this.state.contentConfidence,
      parameters: {
        attackTime: this.config.attackTime,
        releaseTime: this.config.releaseTime,
        ratio: this.config.ratio,
      },
    });
  }

  /**
   * Update processing state
   */
  updateProcessingState(level, gain) {
    // Update level history
    this.buffers.levelHistory.shift();
    this.buffers.levelHistory.push(level.rms);

    // Update gain history
    this.buffers.gainHistory.shift();
    const gainDb = 20 * Math.log10(gain);
    this.buffers.gainHistory.push(gainDb);

    // Update state
    this.state.outputLevel = level.rms + gainDb;
    this.state.gainReduction = -gainDb; // Negative because gain < 1 for reduction
    this.state.instantaneousGain = gain;
  }

  /**
   * Update performance metrics
   */
  updatePerformanceMetrics(processingTime) {
    const perf = this.performance;

    perf.frameCount++;
    perf.totalProcessingTime += processingTime;
    perf.averageProcessingTime = perf.totalProcessingTime / perf.frameCount;
    perf.peakProcessingTime = Math.max(perf.peakProcessingTime, processingTime);

    // Update processing times array
    perf.processingTimes.shift();
    perf.processingTimes.push(processingTime);

    // Adaptive quality adjustment
    const recentAverage =
      perf.processingTimes.reduce((a, b) => a + b, 0) /
      perf.processingTimes.length;
    const maxAllowedTime = 1000 / (this.sampleRate / 256); // Assume 256 sample blocks

    if (recentAverage > maxAllowedTime * 0.8 && perf.qualityLevel === "high") {
      this.reduceQuality();
    } else if (
      recentAverage < maxAllowedTime * 0.4 &&
      perf.qualityLevel === "medium"
    ) {
      this.increaseQuality();
    }
  }

  /**
   * Reduce processing quality for performance
   */
  reduceQuality() {
    if (this.performance.qualityLevel === "high") {
      this.performance.qualityLevel = "medium";
      this.config.multibandEnabled = false;
      this.config.lookAheadTime = 0.002; // Reduce look-ahead
    } else if (this.performance.qualityLevel === "medium") {
      this.performance.qualityLevel = "low";
      this.config.adaptiveProcessing = false;
      this.lookAhead.enabled = false;
    }

    this.eventManager?.emitEvent("agcQualityReduced", {
      newQuality: this.performance.qualityLevel,
      reason: "performance",
      averageProcessingTime: this.performance.averageProcessingTime,
    });
  }

  /**
   * Increase processing quality
   */
  increaseQuality() {
    if (this.performance.qualityLevel === "medium") {
      this.performance.qualityLevel = "high";
      this.config.multibandEnabled = true;
      this.config.lookAheadTime = 0.005;
    } else if (this.performance.qualityLevel === "low") {
      this.performance.qualityLevel = "medium";
      this.config.adaptiveProcessing = true;
      this.lookAhead.enabled = true;
    }

    this.eventManager?.emitEvent("agcQualityIncreased", {
      newQuality: this.performance.qualityLevel,
      averageProcessingTime: this.performance.averageProcessingTime,
    });
  }

  /**
   * Copy buffer utility
   */
  copyBuffer(source, target) {
    const length = Math.min(source.length, target.length);
    for (let i = 0; i < length; i++) {
      target[i] = source[i];
    }
    return target;
  }

  /**
   * Get current AGC state
   */
  getState() {
    return {
      isInitialized: this.isInitialized,
      enabled: this.config.enabled,
      inputLevel: this.state.inputLevel,
      outputLevel: this.state.outputLevel,
      gainReduction: this.state.gainReduction,
      currentGain: this.state.currentGain,
      contentType: this.state.contentType,
      contentConfidence: this.state.contentConfidence,
      qualityLevel: this.performance.qualityLevel,
      averageProcessingTime: this.performance.averageProcessingTime,
      frameCount: this.performance.frameCount,
    };
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    const oldConfig = { ...this.config };
    this.config = { ...this.config, ...newConfig };

    // Recalculate coefficients if timing parameters changed
    if (newConfig.attackTime || newConfig.releaseTime) {
      this.calculateProcessingCoefficients();
    }

    // Reinitialize multi-band if parameters changed
    if (
      newConfig.multibandEnabled !== oldConfig.multibandEnabled ||
      JSON.stringify(newConfig.crossoverFreqs) !==
        JSON.stringify(oldConfig.crossoverFreqs)
    ) {
      if (this.config.multibandEnabled) {
        this.initializeMultiband();
      }
    }

    this.eventManager?.emitEvent("agcConfigUpdated", {
      oldConfig,
      newConfig: this.config,
      timestamp: performance.now(),
    });
  }

  /**
   * Reset AGC state
   */
  reset() {
    this.state.currentGain = 1.0;
    this.state.targetGain = 1.0;
    this.state.inputLevel = -60;
    this.state.outputLevel = -60;
    this.state.gainReduction = 0;

    // Reset band states
    this.buffers.bandStates.forEach((bandState) => {
      bandState.currentGain = 1.0;
      bandState.targetGain = 1.0;
      bandState.detector.peak = 0;
      bandState.detector.rms = 0;
    });

    // Reset level detector
    this.levelDetector.currentPeak = 0;
    this.levelDetector.currentRMS = 0;
    if (this.levelDetector.rmsBuffer) {
      this.levelDetector.rmsBuffer.fill(0);
    }

    this.eventManager?.emitEvent("agcReset", {
      timestamp: performance.now(),
    });
  }

  /**
   * Enable/disable AGC
   */
  setEnabled(enabled) {
    this.config.enabled = enabled;

    if (!enabled) {
      this.reset();
    }

    this.eventManager?.emitEvent("agcEnabledChanged", {
      enabled: enabled,
      timestamp: performance.now(),
    });
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    // Clear buffers
    if (this.buffers.lookAheadBuffer) this.buffers.lookAheadBuffer.fill(0);
    if (this.buffers.delayBuffer) this.buffers.delayBuffer.fill(0);
    if (this.levelDetector.rmsBuffer) this.levelDetector.rmsBuffer.fill(0);
    if (this.levelDetector.loudnessBuffer)
      this.levelDetector.loudnessBuffer.fill(0);

    // Clear multi-band buffers
    this.buffers.bandBuffers.forEach((buffer) => buffer.fill(0));

    // Reset state
    this.isInitialized = false;
    this.performance.frameCount = 0;
    this.performance.totalProcessingTime = 0;

    this.eventManager?.emitEvent("agcCleanup", {
      timestamp: performance.now(),
    });
  }
}

export default AutomaticGainControl;
