/**
 * QualityAssessor Module
 *
 * Provides comprehensive audio quality assessment with multi-domain quality metrics,
 * real-time monitoring, artifact detection, and perceptual quality modeling.
 * Optimized for both objective measurements and subjective quality correlation.
 *
 * Features:
 * - Multi-domain quality metrics calculation
 * - Perceptual quality modeling and scoring
 * - Real-time quality monitoring and feedback
 * - Artifact detection (distortion, clipping, dropouts)
 * - Frequency response analysis and validation
 * - Dynamic range measurement and optimization
 * - Signal-to-noise ratio analysis and improvement
 * - Quality prediction for processing decisions
 */

export class QualityAssessor {
  constructor(eventManager, sampleRate = 48000) {
    this.eventManager = eventManager;
    this.sampleRate = sampleRate;
    this.isInitialized = false;

    // Core quality assessment configuration
    this.config = {
      // Assessment intervals
      updateInterval: 100, // ms
      longTermInterval: 1000, // ms for long-term metrics

      // Quality thresholds
      qualityThreshold: 0.7, // Overall quality threshold (0-1)
      snrThreshold: 20, // dB
      thdThreshold: 0.01, // 1%
      clippingThreshold: -0.1, // dB from full scale

      // Analysis parameters
      analysisWindowSize: 2048,
      overlapRatio: 0.5,
      windowType: "hann",

      // Perceptual modeling
      perceptualWeighting: true,
      psychoacousticModel: "basic", // 'basic', 'advanced'

      // Artifact detection sensitivity
      artifactSensitivity: "medium", // 'low', 'medium', 'high'

      // Quality prediction
      predictiveModel: true,
      learningEnabled: true,
    };

    // Quality metrics state
    this.metrics = {
      // Overall quality
      overallQuality: 0, // 0-1 scale
      qualityGrade: "unknown", // 'excellent', 'good', 'fair', 'poor'

      // Signal quality metrics
      snrRatio: 0, // dB
      thd: 0, // Total Harmonic Distortion (0-1)
      thdPlus: 0, // THD+N
      sinad: 0, // Signal-to-Noise and Distortion ratio

      // Dynamic range metrics
      dynamicRange: 0, // dB
      crestFactor: 0, // dB
      peakToAverage: 0, // dB

      // Frequency domain metrics
      spectralFlatness: 0, // Spectral flatness measure
      spectralCentroid: 0, // Hz
      spectralRolloff: 0, // Hz
      spectralBandwidth: 0, // Hz

      // Artifact metrics
      clippingLevel: 0, // % of samples clipped
      dropoutCount: 0,
      dropoutDuration: 0, // ms
      noiseFloor: -80, // dB

      // Perceptual metrics
      loudness: 0, // LUFS
      sharpness: 0, // acum
      roughness: 0, // asper
      fluctuationStrength: 0, // vacil
    };

    // Quality history for trending
    this.history = {
      qualityHistory: new Array(1000).fill(0),
      snrHistory: new Array(1000).fill(0),
      thdHistory: new Array(1000).fill(0),
      clippingHistory: new Array(1000).fill(0),

      // Long-term trends
      qualityTrend: "stable", // 'improving', 'degrading', 'stable'
      trendConfidence: 0,
      trendDuration: 0, // ms
    };

    // Analysis buffers and processing
    this.buffers = {
      analysisBuffer: null,
      windowBuffer: null,
      fftBuffer: null,
      magnitudeSpectrum: null,
      phaseSpectrum: null,

      // Artifact detection buffers
      clipDetectionBuffer: null,
      dropoutDetectionBuffer: null,
      noiseEstimationBuffer: null,

      // Perceptual analysis buffers
      barkSpectrum: null,
      maskingThreshold: null,
      loudnessBuffer: null,
    };

    // Artifact detectors
    this.artifactDetectors = {
      clippingDetector: {
        threshold: 0.99, // Linear threshold for clipping
        consecutiveThreshold: 3, // Consecutive samples
        currentClipLength: 0,
        totalClippedSamples: 0,
        clipEvents: [],
      },

      dropoutDetector: {
        silenceThreshold: -60, // dB
        minDropoutDuration: 0.01, // seconds
        currentSilenceLength: 0,
        dropoutEvents: [],
        falsePositiveReduction: true,
      },

      distortionAnalyzer: {
        harmonicAnalysis: true,
        intermodulationAnalysis: false,
        fundamentalFreq: 1000, // Hz for test tones
        harmonicBuffer: new Array(10).fill(0),
      },
    };

    // Perceptual model components
    this.perceptualModel = {
      // Bark scale filterbank
      barkFilters: [],
      barkBands: 24,

      // Masking model
      tonalMasking: true,
      noiseMasking: true,
      spreadingFunction: null,

      // Loudness model
      loudnessModel: "iso532b", // ISO 532-B standard
      calibrationLevel: 94, // dB SPL

      // Quality prediction model
      qualityModel: {
        weights: {
          snr: 0.3,
          thd: 0.2,
          artifacts: 0.2,
          perceptual: 0.3,
        },
        bias: 0,
        trained: false,
      },
    };

    // Performance monitoring
    this.performance = {
      frameCount: 0,
      totalProcessingTime: 0,
      averageProcessingTime: 0,
      peakProcessingTime: 0,
      qualityLevel: "high",
      adaptiveProcessing: true,
    };

    // Real-time feedback
    this.feedback = {
      lastUpdate: 0,
      qualityAlerts: [],
      recommendations: [],
      adaptiveThresholds: new Map(),
    };

    this.bindMethods();
  }

  bindMethods() {
    this.assessQuality = this.assessQuality.bind(this);
    this.calculateSNR = this.calculateSNR.bind(this);
    this.detectArtifacts = this.detectArtifacts.bind(this);
    this.updateQualityMetrics = this.updateQualityMetrics.bind(this);
  }

  /**
   * Initialize the quality assessor with all necessary components
   */
  async initialize(config = {}) {
    try {
      // Merge configuration
      this.config = { ...this.config, ...config };

      // Initialize analysis buffers
      this.initializeBuffers();

      // Initialize artifact detectors
      this.initializeArtifactDetectors();

      // Initialize perceptual model
      if (this.config.perceptualWeighting) {
        await this.initializePerceptualModel();
      }

      // Initialize quality prediction model
      if (this.config.predictiveModel) {
        this.initializeQualityModel();
      }

      // Initialize performance monitoring
      this.initializePerformanceMonitoring();

      this.isInitialized = true;

      this.eventManager?.emitEvent("qualityAssessorInitialized", {
        config: this.config,
        timestamp: performance.now(),
      });

      return { success: true };
    } catch (error) {
      console.error("Failed to initialize QualityAssessor:", error);
      this.eventManager?.emitEvent("qualityAssessorError", {
        error: error.message,
        phase: "initialization",
      });
      throw error;
    }
  }

  /**
   * Initialize analysis buffers
   */
  initializeBuffers() {
    const windowSize = this.config.analysisWindowSize;

    // Main analysis buffers
    this.buffers.analysisBuffer = new Float32Array(windowSize);
    this.buffers.windowBuffer = new Float32Array(windowSize);
    this.buffers.fftBuffer = new Float32Array(windowSize * 2); // Complex FFT
    this.buffers.magnitudeSpectrum = new Float32Array(windowSize / 2);
    this.buffers.phaseSpectrum = new Float32Array(windowSize / 2);

    // Create analysis window (Hann window)
    for (let i = 0; i < windowSize; i++) {
      this.buffers.windowBuffer[i] =
        0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (windowSize - 1));
    }

    // Artifact detection buffers
    this.buffers.clipDetectionBuffer = new Float32Array(1024);
    this.buffers.dropoutDetectionBuffer = new Float32Array(this.sampleRate); // 1 second buffer
    this.buffers.noiseEstimationBuffer = new Float32Array(this.sampleRate * 2); // 2 second buffer

    // Perceptual analysis buffers
    if (this.config.perceptualWeighting) {
      this.buffers.barkSpectrum = new Float32Array(24); // 24 Bark bands
      this.buffers.maskingThreshold = new Float32Array(windowSize / 2);
      this.buffers.loudnessBuffer = new Float32Array(
        Math.ceil(this.sampleRate * 0.4)
      ); // 400ms for loudness
    }
  }

  /**
   * Initialize artifact detectors
   */
  initializeArtifactDetectors() {
    // Clipping detector
    const clipping = this.artifactDetectors.clippingDetector;
    clipping.clipEvents = [];
    clipping.totalClippedSamples = 0;

    // Dropout detector
    const dropout = this.artifactDetectors.dropoutDetector;
    dropout.dropoutEvents = [];
    dropout.minDropoutSamples = Math.ceil(
      dropout.minDropoutDuration * this.sampleRate
    );

    // Distortion analyzer
    const distortion = this.artifactDetectors.distortionAnalyzer;
    distortion.harmonicBuffer = new Array(10).fill(0);
  }

  /**
   * Initialize perceptual model
   */
  async initializePerceptualModel() {
    const model = this.perceptualModel;

    // Initialize Bark scale filterbank
    model.barkFilters = this.createBarkFilterbank();

    // Initialize masking spreading function
    model.spreadingFunction = this.createSpreadingFunction();

    // Initialize loudness model components
    this.initializeLoudnessModel();
  }

  /**
   * Create Bark scale filterbank
   */
  createBarkFilterbank() {
    const filters = [];
    const numBands = this.perceptualModel.barkBands;
    const nyquist = this.sampleRate / 2;
    const fftSize = this.config.analysisWindowSize;

    for (let band = 0; band < numBands; band++) {
      // Bark frequency scale conversion
      const barkFreq = band;
      const centerFreq = 600 * Math.sinh(barkFreq / 4);
      const bandwidth =
        25 + 75 * Math.pow(1 + 1.4 * Math.pow(centerFreq / 1000, 0.69), 0.69);

      // Create triangular filter
      const filter = {
        centerFreq: centerFreq,
        bandwidth: bandwidth,
        weights: new Float32Array(fftSize / 2),
      };

      const centerBin = Math.floor((centerFreq * fftSize) / this.sampleRate);
      const bandwidthBins = Math.floor((bandwidth * fftSize) / this.sampleRate);

      // Triangular filter weights
      for (let bin = 0; bin < fftSize / 2; bin++) {
        const distance = Math.abs(bin - centerBin);
        if (distance <= bandwidthBins) {
          filter.weights[bin] = Math.max(0, 1 - distance / bandwidthBins);
        }
      }

      filters.push(filter);
    }

    return filters;
  }

  /**
   * Create masking spreading function
   */
  createSpreadingFunction() {
    const numBands = this.perceptualModel.barkBands;
    const spreadingFunction = new Array(numBands);

    for (let i = 0; i < numBands; i++) {
      spreadingFunction[i] = new Float32Array(numBands);

      for (let j = 0; j < numBands; j++) {
        const distance = Math.abs(i - j);

        // Simplified spreading function
        if (distance === 0) {
          spreadingFunction[i][j] = 1.0;
        } else if (distance <= 3) {
          spreadingFunction[i][j] = Math.pow(10, -0.25 * distance);
        } else {
          spreadingFunction[i][j] = Math.pow(10, -0.5 * distance);
        }
      }
    }

    return spreadingFunction;
  }

  /**
   * Initialize loudness model
   */
  initializeLoudnessModel() {
    // Simplified loudness model initialization
    // In production, would implement full ISO 532-B model
    this.loudnessModel = {
      calibrated: false,
      referenceLevel: this.perceptualModel.calibrationLevel,
      loudnessCoeffs: new Float32Array(this.perceptualModel.barkBands),
      temporalIntegration: 0.4, // seconds
    };

    // Initialize loudness coefficients (simplified)
    for (let i = 0; i < this.perceptualModel.barkBands; i++) {
      this.loudnessModel.loudnessCoeffs[i] = 1.0; // Uniform weighting for now
    }
  }

  /**
   * Initialize quality prediction model
   */
  initializeQualityModel() {
    const model = this.perceptualModel.qualityModel;

    // Default weights for quality prediction
    model.weights = {
      snr: 0.3,
      thd: 0.2,
      artifacts: 0.2,
      perceptual: 0.3,
    };

    model.bias = 0;
    model.trained = false;

    // Learning parameters
    model.learningRate = 0.01;
    model.trainingData = [];
    model.maxTrainingData = 1000;
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
      qualityLevel: "high",
      adaptiveProcessing: true,
      processingTimes: new Array(100).fill(0),
    };
  }

  /**
   * Main quality assessment function
   */
  assessQuality(inputBuffer, outputBuffer = null) {
    if (!this.isInitialized) {
      return {
        quality: 0,
        metrics: this.metrics,
        alerts: [],
      };
    }

    const startTime = performance.now();

    try {
      // Perform spectral analysis
      this.performSpectralAnalysis(inputBuffer);

      // Calculate signal quality metrics
      this.calculateSignalMetrics(inputBuffer);

      // Detect artifacts
      this.detectArtifacts(inputBuffer);

      // Calculate perceptual metrics
      if (this.config.perceptualWeighting) {
        this.calculatePerceptualMetrics();
      }

      // Update overall quality score
      this.updateOverallQuality();

      // Update quality history and trends
      this.updateQualityHistory();

      // Generate feedback and recommendations
      const feedback = this.generateFeedback();

      // Update performance metrics
      this.updatePerformanceMetrics(performance.now() - startTime);

      // Emit quality assessment event
      this.eventManager?.emitEvent("qualityAssessmentComplete", {
        quality: this.metrics.overallQuality,
        grade: this.metrics.qualityGrade,
        metrics: { ...this.metrics },
        feedback: feedback,
        processingTime: performance.now() - startTime,
      });

      return {
        quality: this.metrics.overallQuality,
        grade: this.metrics.qualityGrade,
        metrics: { ...this.metrics },
        feedback: feedback,
        alerts: this.feedback.qualityAlerts,
      };
    } catch (error) {
      console.error("Error in quality assessment:", error);
      this.eventManager?.emitEvent("qualityAssessmentError", {
        error: error.message,
        timestamp: performance.now(),
      });

      return {
        quality: 0,
        metrics: this.metrics,
        alerts: [{ type: "error", message: "Quality assessment failed" }],
      };
    }
  }

  /**
   * Perform spectral analysis
   */
  performSpectralAnalysis(inputBuffer) {
    const windowSize = this.config.analysisWindowSize;
    const hopSize = Math.floor(windowSize * (1 - this.config.overlapRatio));

    // Process overlapping windows
    for (
      let offset = 0;
      offset < inputBuffer.length - windowSize;
      offset += hopSize
    ) {
      // Copy windowed data to analysis buffer
      for (let i = 0; i < windowSize; i++) {
        this.buffers.analysisBuffer[i] =
          inputBuffer[offset + i] * this.buffers.windowBuffer[i];
      }

      // Perform FFT
      this.performFFT(this.buffers.analysisBuffer);

      // Calculate magnitude and phase spectra
      this.calculateSpectra();

      // Process current spectrum
      this.processSpectrum();
    }
  }

  /**
   * Perform FFT analysis
   */
  performFFT(buffer) {
    const N = buffer.length;
    const fftBuffer = this.buffers.fftBuffer;

    // Copy real part, zero imaginary part
    for (let i = 0; i < N; i++) {
      fftBuffer[i * 2] = buffer[i];
      fftBuffer[i * 2 + 1] = 0;
    }

    // Simplified FFT implementation
    // In production, use optimized FFT library
    this.fft(fftBuffer, N);
  }

  /**
   * Simplified FFT implementation
   */
  fft(buffer, N) {
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
        [buffer[i * 2], buffer[j * 2]] = [buffer[j * 2], buffer[i * 2]];
        [buffer[i * 2 + 1], buffer[j * 2 + 1]] = [
          buffer[j * 2 + 1],
          buffer[i * 2 + 1],
        ];
      }
    }

    // Butterfly operations
    for (let len = 2; len <= N; len <<= 1) {
      const wlen = (2 * Math.PI) / len;
      for (let i = 0; i < N; i += len) {
        let w_real = 1;
        let w_imag = 0;

        for (let j = 0; j < len / 2; j++) {
          const u_real = buffer[(i + j) * 2];
          const u_imag = buffer[(i + j) * 2 + 1];
          const v_real =
            buffer[(i + j + len / 2) * 2] * w_real -
            buffer[(i + j + len / 2) * 2 + 1] * w_imag;
          const v_imag =
            buffer[(i + j + len / 2) * 2] * w_imag +
            buffer[(i + j + len / 2) * 2 + 1] * w_real;

          buffer[(i + j) * 2] = u_real + v_real;
          buffer[(i + j) * 2 + 1] = u_imag + v_imag;
          buffer[(i + j + len / 2) * 2] = u_real - v_real;
          buffer[(i + j + len / 2) * 2 + 1] = u_imag - v_imag;

          const temp_real = w_real * Math.cos(wlen) - w_imag * Math.sin(wlen);
          w_imag = w_real * Math.sin(wlen) + w_imag * Math.cos(wlen);
          w_real = temp_real;
        }
      }
    }
  }

  /**
   * Calculate magnitude and phase spectra
   */
  calculateSpectra() {
    const fftBuffer = this.buffers.fftBuffer;
    const magnitude = this.buffers.magnitudeSpectrum;
    const phase = this.buffers.phaseSpectrum;
    const halfSize = magnitude.length;

    for (let i = 0; i < halfSize; i++) {
      const real = fftBuffer[i * 2];
      const imag = fftBuffer[i * 2 + 1];

      magnitude[i] = Math.sqrt(real * real + imag * imag);
      phase[i] = Math.atan2(imag, real);
    }
  }

  /**
   * Process current spectrum for quality metrics
   */
  processSpectrum() {
    const magnitude = this.buffers.magnitudeSpectrum;

    // Calculate spectral features
    this.metrics.spectralCentroid = this.calculateSpectralCentroid(magnitude);
    this.metrics.spectralRolloff = this.calculateSpectralRolloff(magnitude);
    this.metrics.spectralBandwidth = this.calculateSpectralBandwidth(magnitude);
    this.metrics.spectralFlatness = this.calculateSpectralFlatness(magnitude);
  }

  /**
   * Calculate signal quality metrics
   */
  calculateSignalMetrics(inputBuffer) {
    // Calculate SNR
    this.metrics.snrRatio = this.calculateSNR(inputBuffer);

    // Calculate THD
    this.metrics.thd = this.calculateTHD(inputBuffer);

    // Calculate dynamic range metrics
    this.calculateDynamicRangeMetrics(inputBuffer);

    // Update noise floor estimate
    this.updateNoiseFloorEstimate(inputBuffer);
  }

  /**
   * Calculate Signal-to-Noise Ratio
   */
  calculateSNR(inputBuffer) {
    // Simplified SNR calculation
    // In production, would use more sophisticated signal/noise separation

    let signalPower = 0;
    let noisePower = 0;
    const windowSize = 1024;

    // Calculate signal power (high-energy portions)
    for (let i = 0; i < inputBuffer.length; i++) {
      const sample = inputBuffer[i];
      const power = sample * sample;
      signalPower += power;
    }
    signalPower /= inputBuffer.length;

    // Estimate noise power from quiet portions
    // This is a simplified approach
    const sortedSamples = Array.from(inputBuffer).map(Math.abs).sort();
    const quietSamples = sortedSamples.slice(
      0,
      Math.floor(sortedSamples.length * 0.1)
    );

    for (let i = 0; i < quietSamples.length; i++) {
      noisePower += quietSamples[i] * quietSamples[i];
    }
    noisePower /= quietSamples.length;

    // Convert to dB
    const snr =
      10 *
      Math.log10(Math.max(signalPower / Math.max(noisePower, 1e-10), 1e-10));

    return Math.max(-60, Math.min(60, snr)); // Clamp to reasonable range
  }

  /**
   * Calculate Total Harmonic Distortion
   */
  calculateTHD(inputBuffer) {
    const magnitude = this.buffers.magnitudeSpectrum;
    const fundamentalBin = this.findFundamentalFrequency(magnitude);

    if (fundamentalBin === -1) {
      return 0; // No clear fundamental found
    }

    // Calculate harmonic powers
    const fundamentalPower =
      magnitude[fundamentalBin] * magnitude[fundamentalBin];
    let harmonicPower = 0;

    // Sum powers of first 5 harmonics
    for (let harmonic = 2; harmonic <= 6; harmonic++) {
      const harmonicBin = Math.floor(fundamentalBin * harmonic);
      if (harmonicBin < magnitude.length) {
        harmonicPower += magnitude[harmonicBin] * magnitude[harmonicBin];
      }
    }

    // Calculate THD as ratio
    const thd = Math.sqrt(harmonicPower / Math.max(fundamentalPower, 1e-10));

    return Math.min(thd, 1.0); // Clamp to 100%
  }

  /**
   * Find fundamental frequency in spectrum
   */
  findFundamentalFrequency(magnitude) {
    let maxMagnitude = 0;
    let fundamentalBin = -1;

    // Search in typical fundamental frequency range (80-800 Hz)
    const minBin = Math.floor((80 * magnitude.length * 2) / this.sampleRate);
    const maxBin = Math.floor((800 * magnitude.length * 2) / this.sampleRate);

    for (let i = minBin; i < Math.min(maxBin, magnitude.length); i++) {
      if (magnitude[i] > maxMagnitude) {
        maxMagnitude = magnitude[i];
        fundamentalBin = i;
      }
    }

    return fundamentalBin;
  }

  /**
   * Calculate dynamic range metrics
   */
  calculateDynamicRangeMetrics(inputBuffer) {
    let min = Infinity;
    let max = -Infinity;
    let rms = 0;
    let peak = 0;

    for (let i = 0; i < inputBuffer.length; i++) {
      const sample = inputBuffer[i];
      const absample = Math.abs(sample);

      min = Math.min(min, absample);
      max = Math.max(max, absample);
      rms += sample * sample;
      peak = Math.max(peak, absample);
    }

    rms = Math.sqrt(rms / inputBuffer.length);

    // Convert to dB
    const peakDb = 20 * Math.log10(Math.max(peak, 1e-10));
    const rmsDb = 20 * Math.log10(Math.max(rms, 1e-10));
    const minDb = 20 * Math.log10(Math.max(min, 1e-10));

    this.metrics.dynamicRange = peakDb - minDb;
    this.metrics.crestFactor = peakDb - rmsDb;
    this.metrics.peakToAverage = peakDb - rmsDb;
  }

  /**
   * Update noise floor estimate
   */
  updateNoiseFloorEstimate(inputBuffer) {
    const buffer = this.buffers.noiseEstimationBuffer;
    const bufferSize = buffer.length;

    // Shift buffer and add new samples
    const samplesToAdd = Math.min(inputBuffer.length, bufferSize);
    for (let i = 0; i < bufferSize - samplesToAdd; i++) {
      buffer[i] = buffer[i + samplesToAdd];
    }
    for (let i = 0; i < samplesToAdd; i++) {
      buffer[bufferSize - samplesToAdd + i] = inputBuffer[i];
    }

    // Estimate noise floor from quietest 10% of samples
    const sortedSamples = Array.from(buffer).map(Math.abs).sort();
    const quietSamples = sortedSamples.slice(
      0,
      Math.floor(sortedSamples.length * 0.1)
    );

    let quietPower = 0;
    for (let i = 0; i < quietSamples.length; i++) {
      quietPower += quietSamples[i] * quietSamples[i];
    }
    quietPower /= quietSamples.length;

    this.metrics.noiseFloor =
      20 * Math.log10(Math.max(Math.sqrt(quietPower), 1e-10));
  }

  /**
   * Detect audio artifacts
   */
  detectArtifacts(inputBuffer) {
    // Detect clipping
    this.detectClipping(inputBuffer);

    // Detect dropouts
    this.detectDropouts(inputBuffer);

    // Detect other distortions
    this.detectDistortion(inputBuffer);
  }

  /**
   * Detect clipping artifacts
   */
  detectClipping(inputBuffer) {
    const detector = this.artifactDetectors.clippingDetector;
    let clippedSamples = 0;

    for (let i = 0; i < inputBuffer.length; i++) {
      const absample = Math.abs(inputBuffer[i]);

      if (absample >= detector.threshold) {
        detector.currentClipLength++;
        clippedSamples++;

        if (detector.currentClipLength >= detector.consecutiveThreshold) {
          // Record clipping event
          detector.clipEvents.push({
            timestamp: performance.now(),
            duration: detector.currentClipLength,
            level: absample,
          });
        }
      } else {
        detector.currentClipLength = 0;
      }
    }

    detector.totalClippedSamples += clippedSamples;
    this.metrics.clippingLevel = (clippedSamples / inputBuffer.length) * 100;

    // Cleanup old events (keep last 100)
    if (detector.clipEvents.length > 100) {
      detector.clipEvents = detector.clipEvents.slice(-100);
    }
  }

  /**
   * Detect dropout artifacts
   */
  detectDropouts(inputBuffer) {
    const detector = this.artifactDetectors.dropoutDetector;
    const silenceThresholdLinear = Math.pow(10, detector.silenceThreshold / 20);

    for (let i = 0; i < inputBuffer.length; i++) {
      const absample = Math.abs(inputBuffer[i]);

      if (absample < silenceThresholdLinear) {
        detector.currentSilenceLength++;
      } else {
        if (detector.currentSilenceLength >= detector.minDropoutSamples) {
          // Record dropout event
          const dropoutDuration =
            (detector.currentSilenceLength / this.sampleRate) * 1000; // ms

          detector.dropoutEvents.push({
            timestamp: performance.now(),
            duration: dropoutDuration,
            samples: detector.currentSilenceLength,
          });

          this.metrics.dropoutDuration += dropoutDuration;
          this.metrics.dropoutCount++;
        }

        detector.currentSilenceLength = 0;
      }
    }

    // Cleanup old events
    if (detector.dropoutEvents.length > 100) {
      const removedEvents = detector.dropoutEvents.splice(
        0,
        detector.dropoutEvents.length - 100
      );
      // Adjust total duration
      removedEvents.forEach((event) => {
        this.metrics.dropoutDuration = Math.max(
          0,
          this.metrics.dropoutDuration - event.duration
        );
      });
    }
  }

  /**
   * Detect distortion artifacts
   */
  detectDistortion(inputBuffer) {
    // Simple distortion detection based on harmonic analysis
    const magnitude = this.buffers.magnitudeSpectrum;
    const fundamentalBin = this.findFundamentalFrequency(magnitude);

    if (fundamentalBin !== -1) {
      // Calculate harmonic distortion products
      let totalHarmonicEnergy = 0;
      let fundamentalEnergy =
        magnitude[fundamentalBin] * magnitude[fundamentalBin];

      for (let harmonic = 2; harmonic <= 5; harmonic++) {
        const harmonicBin = Math.floor(fundamentalBin * harmonic);
        if (harmonicBin < magnitude.length) {
          totalHarmonicEnergy +=
            magnitude[harmonicBin] * magnitude[harmonicBin];
        }
      }

      // Update THD+N
      this.metrics.thdPlus = Math.sqrt(
        totalHarmonicEnergy / Math.max(fundamentalEnergy, 1e-10)
      );

      // Calculate SINAD
      const noiseEnergy = this.estimateNoiseEnergy(magnitude, fundamentalBin);
      const totalDistortionNoise = totalHarmonicEnergy + noiseEnergy;
      this.metrics.sinad =
        10 *
        Math.log10(fundamentalEnergy / Math.max(totalDistortionNoise, 1e-10));
    }
  }

  /**
   * Estimate noise energy from spectrum
   */
  estimateNoiseEnergy(magnitude, fundamentalBin) {
    let noiseEnergy = 0;
    let noiseBins = 0;

    // Sum energy in non-harmonic bins
    for (let i = 1; i < magnitude.length; i++) {
      let isHarmonic = false;

      // Check if this bin is near a harmonic
      for (let harmonic = 1; harmonic <= 5; harmonic++) {
        const harmonicBin = Math.floor(fundamentalBin * harmonic);
        if (Math.abs(i - harmonicBin) <= 2) {
          // Within 2 bins of harmonic
          isHarmonic = true;
          break;
        }
      }

      if (!isHarmonic) {
        noiseEnergy += magnitude[i] * magnitude[i];
        noiseBins++;
      }
    }

    return noiseBins > 0 ? noiseEnergy / noiseBins : 0;
  }

  /**
   * Calculate perceptual quality metrics
   */
  calculatePerceptualMetrics() {
    if (!this.config.perceptualWeighting) return;

    // Calculate Bark spectrum
    this.calculateBarkSpectrum();

    // Calculate loudness
    this.calculateLoudness();

    // Calculate sharpness, roughness, etc.
    this.calculatePsychoacousticMetrics();
  }

  /**
   * Calculate Bark spectrum
   */
  calculateBarkSpectrum() {
    const magnitude = this.buffers.magnitudeSpectrum;
    const barkSpectrum = this.buffers.barkSpectrum;
    const filters = this.perceptualModel.barkFilters;

    for (let band = 0; band < filters.length; band++) {
      let bandEnergy = 0;
      const filter = filters[band];

      for (let bin = 0; bin < magnitude.length; bin++) {
        bandEnergy += magnitude[bin] * magnitude[bin] * filter.weights[bin];
      }

      barkSpectrum[band] = Math.sqrt(bandEnergy);
    }
  }

  /**
   * Calculate loudness (simplified LUFS)
   */
  calculateLoudness() {
    const barkSpectrum = this.buffers.barkSpectrum;
    const coeffs = this.loudnessModel.loudnessCoeffs;

    let totalLoudness = 0;
    for (let i = 0; i < barkSpectrum.length; i++) {
      // Simplified loudness calculation
      const bandLoudness = barkSpectrum[i] * coeffs[i];
      totalLoudness += bandLoudness * bandLoudness;
    }

    // Convert to LUFS (simplified)
    this.metrics.loudness =
      -23 + 10 * Math.log10(Math.max(totalLoudness, 1e-10));
  }

  /**
   * Calculate psychoacoustic metrics
   */
  calculatePsychoacousticMetrics() {
    const barkSpectrum = this.buffers.barkSpectrum;

    // Simplified sharpness calculation (Aures model)
    let weightedSum = 0;
    let totalSum = 0;

    for (let i = 0; i < barkSpectrum.length; i++) {
      const weight = i / barkSpectrum.length; // Simplified weighting
      weightedSum += barkSpectrum[i] * weight;
      totalSum += barkSpectrum[i];
    }

    this.metrics.sharpness = totalSum > 0 ? weightedSum / totalSum : 0;

    // Simplified roughness (placeholder)
    this.metrics.roughness = 0; // Would require more complex modulation analysis

    // Simplified fluctuation strength (placeholder)
    this.metrics.fluctuationStrength = 0; // Would require temporal analysis
  }

  /**
   * Update overall quality score
   */
  updateOverallQuality() {
    const weights = this.perceptualModel.qualityModel.weights;

    // Normalize individual metrics to 0-1 scale
    const snrScore = Math.max(
      0,
      Math.min(1, (this.metrics.snrRatio + 20) / 40)
    ); // -20 to +20 dB range
    const thdScore = Math.max(0, 1 - this.metrics.thd * 10); // Lower THD is better
    const artifactScore = Math.max(
      0,
      1 - this.metrics.clippingLevel / 10 - this.metrics.dropoutCount / 10
    );
    const perceptualScore = Math.max(
      0,
      Math.min(1, (this.metrics.loudness + 50) / 50)
    ); // Simplified

    // Weighted combination
    this.metrics.overallQuality =
      weights.snr * snrScore +
      weights.thd * thdScore +
      weights.artifacts * artifactScore +
      weights.perceptual * perceptualScore;

    // Determine quality grade
    if (this.metrics.overallQuality >= 0.9) {
      this.metrics.qualityGrade = "excellent";
    } else if (this.metrics.overallQuality >= 0.7) {
      this.metrics.qualityGrade = "good";
    } else if (this.metrics.overallQuality >= 0.5) {
      this.metrics.qualityGrade = "fair";
    } else {
      this.metrics.qualityGrade = "poor";
    }
  }

  /**
   * Update quality history and trends
   */
  updateQualityHistory() {
    const history = this.history;

    // Update quality history
    history.qualityHistory.shift();
    history.qualityHistory.push(this.metrics.overallQuality);

    history.snrHistory.shift();
    history.snrHistory.push(this.metrics.snrRatio);

    history.thdHistory.shift();
    history.thdHistory.push(this.metrics.thd);

    history.clippingHistory.shift();
    history.clippingHistory.push(this.metrics.clippingLevel);

    // Analyze trends
    this.analyzeTrends();
  }

  /**
   * Analyze quality trends
   */
  analyzeTrends() {
    const history = this.history;
    const recentSamples = 100; // Analyze last 100 samples
    const recentQuality = history.qualityHistory.slice(-recentSamples);

    if (recentQuality.length < 10) return; // Need minimum samples

    // Simple linear regression for trend
    const n = recentQuality.length;
    let sumX = 0,
      sumY = 0,
      sumXY = 0,
      sumX2 = 0;

    for (let i = 0; i < n; i++) {
      sumX += i;
      sumY += recentQuality[i];
      sumXY += i * recentQuality[i];
      sumX2 += i * i;
    }

    const slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);

    // Determine trend
    if (slope > 0.001) {
      history.qualityTrend = "improving";
      history.trendConfidence = Math.min(1, Math.abs(slope) * 100);
    } else if (slope < -0.001) {
      history.qualityTrend = "degrading";
      history.trendConfidence = Math.min(1, Math.abs(slope) * 100);
    } else {
      history.qualityTrend = "stable";
      history.trendConfidence = 1 - Math.abs(slope) * 100;
    }
  }

  /**
   * Generate feedback and recommendations
   */
  generateFeedback() {
    const feedback = {
      alerts: [],
      recommendations: [],
      summary: "",
    };

    // Check quality thresholds
    if (this.metrics.overallQuality < this.config.qualityThreshold) {
      feedback.alerts.push({
        type: "warning",
        message: `Overall quality (${(
          this.metrics.overallQuality * 100
        ).toFixed(1)}%) is below threshold`,
        severity: "medium",
      });
    }

    // SNR alerts
    if (this.metrics.snrRatio < this.config.snrThreshold) {
      feedback.alerts.push({
        type: "warning",
        message: `Low SNR detected: ${this.metrics.snrRatio.toFixed(1)} dB`,
        severity: "medium",
      });

      feedback.recommendations.push({
        type: "noise_reduction",
        message:
          "Consider enabling noise reduction or improving recording environment",
        priority: "medium",
      });
    }

    // THD alerts
    if (this.metrics.thd > this.config.thdThreshold) {
      feedback.alerts.push({
        type: "warning",
        message: `High distortion detected: ${(this.metrics.thd * 100).toFixed(
          2
        )}%`,
        severity: "high",
      });

      feedback.recommendations.push({
        type: "gain_adjustment",
        message: "Reduce input gain to minimize distortion",
        priority: "high",
      });
    }

    // Clipping alerts
    if (this.metrics.clippingLevel > 0.1) {
      feedback.alerts.push({
        type: "error",
        message: `Clipping detected in ${this.metrics.clippingLevel.toFixed(
          2
        )}% of samples`,
        severity: "high",
      });

      feedback.recommendations.push({
        type: "gain_adjustment",
        message: "Immediately reduce input gain to prevent clipping",
        priority: "critical",
      });
    }

    // Dropout alerts
    if (this.metrics.dropoutCount > 0) {
      feedback.alerts.push({
        type: "warning",
        message: `${this.metrics.dropoutCount} audio dropouts detected`,
        severity: "medium",
      });

      feedback.recommendations.push({
        type: "connection_check",
        message: "Check audio connection and buffer settings",
        priority: "medium",
      });
    }

    // Generate summary
    feedback.summary = this.generateQualitySummary();

    return feedback;
  }

  /**
   * Generate quality summary
   */
  generateQualitySummary() {
    const grade = this.metrics.qualityGrade;
    const quality = (this.metrics.overallQuality * 100).toFixed(1);
    const trend = this.history.qualityTrend;

    let summary = `Audio quality is ${grade} (${quality}%)`;

    if (trend !== "stable") {
      summary += ` and ${trend}`;
    }

    // Add key metrics
    const keyIssues = [];
    if (this.metrics.snrRatio < 15) keyIssues.push("low SNR");
    if (this.metrics.thd > 0.02) keyIssues.push("high distortion");
    if (this.metrics.clippingLevel > 0) keyIssues.push("clipping");

    if (keyIssues.length > 0) {
      summary += `. Issues: ${keyIssues.join(", ")}`;
    }

    return summary;
  }

  /**
   * Calculate spectral features
   */
  calculateSpectralCentroid(magnitude) {
    let weightedSum = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < magnitude.length; i++) {
      const freq = (i * this.sampleRate) / (2 * magnitude.length);
      const mag = magnitude[i];

      weightedSum += freq * mag;
      magnitudeSum += mag;
    }

    return magnitudeSum > 0 ? weightedSum / magnitudeSum : 0;
  }

  calculateSpectralRolloff(magnitude) {
    const totalEnergy = magnitude.reduce((sum, mag) => sum + mag * mag, 0);
    const threshold = totalEnergy * 0.85; // 85% rolloff

    let cumulativeEnergy = 0;
    for (let i = 0; i < magnitude.length; i++) {
      cumulativeEnergy += magnitude[i] * magnitude[i];
      if (cumulativeEnergy >= threshold) {
        return (i * this.sampleRate) / (2 * magnitude.length);
      }
    }

    return this.sampleRate / 2;
  }

  calculateSpectralBandwidth(magnitude) {
    const centroid = this.calculateSpectralCentroid(magnitude);
    let weightedSum = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < magnitude.length; i++) {
      const freq = (i * this.sampleRate) / (2 * magnitude.length);
      const mag = magnitude[i];
      const deviation = freq - centroid;

      weightedSum += deviation * deviation * mag;
      magnitudeSum += mag;
    }

    return magnitudeSum > 0 ? Math.sqrt(weightedSum / magnitudeSum) : 0;
  }

  calculateSpectralFlatness(magnitude) {
    let geometricMean = 1;
    let arithmeticMean = 0;
    let validBins = 0;

    for (let i = 1; i < magnitude.length; i++) {
      // Skip DC
      if (magnitude[i] > 0) {
        geometricMean *= Math.pow(magnitude[i], 1 / (magnitude.length - 1));
        arithmeticMean += magnitude[i];
        validBins++;
      }
    }

    if (validBins === 0) return 0;

    arithmeticMean /= validBins;

    return arithmeticMean > 0 ? geometricMean / arithmeticMean : 0;
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
    const maxAllowedTime = 1000 / (this.sampleRate / 1024); // Assume 1024 sample blocks

    if (recentAverage > maxAllowedTime * 0.8 && perf.qualityLevel === "high") {
      this.reduceProcessingQuality();
    }
  }

  /**
   * Reduce processing quality for performance
   */
  reduceProcessingQuality() {
    if (this.performance.qualityLevel === "high") {
      this.performance.qualityLevel = "medium";
      this.config.analysisWindowSize = Math.max(
        1024,
        this.config.analysisWindowSize / 2
      );
      this.config.perceptualWeighting = false;
    } else if (this.performance.qualityLevel === "medium") {
      this.performance.qualityLevel = "low";
      this.config.updateInterval = Math.max(
        200,
        this.config.updateInterval * 2
      );
    }

    this.eventManager?.emitEvent("qualityAssessorQualityReduced", {
      newQuality: this.performance.qualityLevel,
      reason: "performance",
    });
  }

  /**
   * Get current quality state
   */
  getState() {
    return {
      isInitialized: this.isInitialized,
      overallQuality: this.metrics.overallQuality,
      qualityGrade: this.metrics.qualityGrade,
      snrRatio: this.metrics.snrRatio,
      thd: this.metrics.thd,
      clippingLevel: this.metrics.clippingLevel,
      dropoutCount: this.metrics.dropoutCount,
      noiseFloor: this.metrics.noiseFloor,
      qualityTrend: this.history.qualityTrend,
      trendConfidence: this.history.trendConfidence,
      processingQuality: this.performance.qualityLevel,
      frameCount: this.performance.frameCount,
      averageProcessingTime: this.performance.averageProcessingTime,
    };
  }

  /**
   * Update configuration
   */
  updateConfig(newConfig) {
    const oldConfig = { ...this.config };
    this.config = { ...this.config, ...newConfig };

    // Reinitialize buffers if window size changed
    if (
      newConfig.analysisWindowSize &&
      newConfig.analysisWindowSize !== oldConfig.analysisWindowSize
    ) {
      this.initializeBuffers();
    }

    // Reinitialize perceptual model if needed
    if (newConfig.perceptualWeighting !== oldConfig.perceptualWeighting) {
      if (this.config.perceptualWeighting) {
        this.initializePerceptualModel();
      }
    }

    this.eventManager?.emitEvent("qualityAssessorConfigUpdated", {
      oldConfig,
      newConfig: this.config,
      timestamp: performance.now(),
    });
  }

  /**
   * Reset quality metrics
   */
  reset() {
    // Reset metrics
    Object.keys(this.metrics).forEach((key) => {
      if (typeof this.metrics[key] === "number") {
        this.metrics[key] = 0;
      } else if (typeof this.metrics[key] === "string") {
        this.metrics[key] = key === "qualityGrade" ? "unknown" : "";
      }
    });

    // Reset history
    this.history.qualityHistory.fill(0);
    this.history.snrHistory.fill(0);
    this.history.thdHistory.fill(0);
    this.history.clippingHistory.fill(0);

    // Reset artifact detectors
    this.artifactDetectors.clippingDetector.clipEvents = [];
    this.artifactDetectors.clippingDetector.totalClippedSamples = 0;
    this.artifactDetectors.dropoutDetector.dropoutEvents = [];

    this.eventManager?.emitEvent("qualityAssessorReset", {
      timestamp: performance.now(),
    });
  }

  /**
   * Cleanup resources
   */
  cleanup() {
    // Clear buffers
    Object.values(this.buffers).forEach((buffer) => {
      if (buffer && buffer.fill) {
        buffer.fill(0);
      }
    });

    // Reset state
    this.isInitialized = false;
    this.performance.frameCount = 0;
    this.performance.totalProcessingTime = 0;

    this.eventManager?.emitEvent("qualityAssessorCleanup", {
      timestamp: performance.now(),
    });
  }
}

export default QualityAssessor;
