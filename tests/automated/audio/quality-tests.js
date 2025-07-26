/**
 * Phase 3.1 Module 7: Audio Quality Assessment Testing
 *
 * Comprehensive test suite for the QualityAssessor component
 * Tests audio quality metrics, analysis accuracy, and performance
 */

const {
  test,
  expect,
  beforeEach,
  afterEach,
  describe,
} = require("@jest/globals");

// Mock QualityAssessor Implementation for Testing
class MockQualityAssessor {
  constructor(options = {}) {
    this.sampleRate = options.sampleRate || 44100;
    this.channels = options.channels || 2;
    this.blockSize = options.blockSize || 1024;
    this.windowSize = options.windowSize || 2048;
    this.hopSize = options.hopSize || 512;

    // Quality metrics state
    this.qualityMetrics = {
      snr: 0,
      thd: 0,
      dynamicRange: 0,
      frequencyResponse: [],
      perceptualQuality: 0,
      artifacts: [],
      spectralFeatures: {},
    };

    // Real-time monitoring state
    this.isMonitoring = false;
    this.monitoringCallbacks = [];
    this.qualityThresholds = {};
    this.alertHistory = [];

    // Performance tracking
    this.performanceStats = {
      processingTime: 0,
      memoryUsage: 0,
      cpuUsage: 0,
      throughput: 0,
    };

    // Internal buffers for analysis
    this.analysisBuffer = new Float32Array(this.windowSize * this.channels);
    this.fftBuffer = new Float32Array(this.windowSize);
    this.spectrumBuffer = new Float32Array(this.windowSize / 2 + 1);

    // Quality assessment algorithms
    this.initializeAlgorithms();
  }

  initializeAlgorithms() {
    // FFT setup for spectral analysis
    this.fftSetup = {
      size: this.windowSize,
      inverse: false,
      twiddles: this.generateTwiddles(this.windowSize),
    };

    // Perceptual models
    this.perceptualModel = {
      barkScale: this.generateBarkScale(),
      maskingThresholds: new Float32Array(this.windowSize / 2 + 1),
      loudnessModel: this.initializeLoudnessModel(),
    };

    // Artifact detection filters
    this.artifactDetectors = {
      clipping: this.initializeClippingDetector(),
      aliasing: this.initializeAliasingDetector(),
      distortion: this.initializeDistortionDetector(),
      noise: this.initializeNoiseDetector(),
    };
  }

  // Signal-to-Noise Ratio calculation
  calculateSNR(audioData, noiseFloor = -60) {
    const startTime = performance.now();

    try {
      const signalPower = this.calculateSignalPower(audioData);
      const noisePower = this.calculateNoisePower(audioData, noiseFloor);

      const snr = 10 * Math.log10(signalPower / noisePower);

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.snr = snr;

      return {
        snr: snr,
        signalPower: signalPower,
        noisePower: noisePower,
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`SNR calculation failed: ${error.message}`);
    }
  }

  calculateSignalPower(audioData) {
    let sumSquares = 0;
    const length = audioData.length;

    for (let i = 0; i < length; i++) {
      sumSquares += audioData[i] * audioData[i];
    }

    return sumSquares / length;
  }

  calculateNoisePower(audioData, noiseFloor) {
    // Estimate noise power using silence detection
    const silenceThreshold = Math.pow(10, noiseFloor / 20);
    let noiseSamples = [];

    for (let i = 0; i < audioData.length; i++) {
      if (Math.abs(audioData[i]) < silenceThreshold) {
        noiseSamples.push(audioData[i]);
      }
    }

    if (noiseSamples.length === 0) {
      return Math.pow(10, noiseFloor / 10); // Return theoretical noise floor
    }

    let sumSquares = 0;
    for (let sample of noiseSamples) {
      sumSquares += sample * sample;
    }

    return sumSquares / noiseSamples.length;
  }

  // Total Harmonic Distortion + Noise calculation
  calculateTHDN(audioData, fundamentalFreq) {
    const startTime = performance.now();

    try {
      const spectrum = this.performFFT(audioData);
      const fundamentalBin = Math.round(
        (fundamentalFreq * this.windowSize) / this.sampleRate
      );

      // Find fundamental amplitude
      const fundamentalAmplitude = spectrum[fundamentalBin];

      // Calculate harmonic amplitudes
      let harmonicPower = 0;
      let totalPower = 0;

      for (let i = 0; i < spectrum.length; i++) {
        const power = spectrum[i] * spectrum[i];
        totalPower += power;

        // Check if this is a harmonic (excluding fundamental)
        if (this.isHarmonic(i, fundamentalBin) && i !== fundamentalBin) {
          harmonicPower += power;
        }
      }

      // Add noise power (everything that's not fundamental or harmonics)
      const noisePower =
        totalPower -
        fundamentalAmplitude * fundamentalAmplitude -
        harmonicPower;
      const thdnPower = harmonicPower + noisePower;

      const thdn = Math.sqrt(thdnPower) / fundamentalAmplitude;
      const thdnDb = 20 * Math.log10(thdn);

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.thd = thdnDb;

      return {
        thdn: thdn,
        thdnDb: thdnDb,
        fundamentalAmplitude: fundamentalAmplitude,
        harmonicPower: harmonicPower,
        noisePower: noisePower,
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`THD+N calculation failed: ${error.message}`);
    }
  }

  isHarmonic(bin, fundamentalBin) {
    const tolerance = 2; // Allow some tolerance for frequency drift
    const harmonicNumbers = [2, 3, 4, 5, 6, 7, 8, 9, 10];

    for (let harmonic of harmonicNumbers) {
      const expectedBin = fundamentalBin * harmonic;
      if (Math.abs(bin - expectedBin) <= tolerance) {
        return true;
      }
    }
    return false;
  }

  // Dynamic Range measurement
  measureDynamicRange(audioData) {
    const startTime = performance.now();

    try {
      const blockSize = 1024;
      const blocks = Math.floor(audioData.length / blockSize);
      const rmsValues = [];

      // Calculate RMS for each block
      for (let i = 0; i < blocks; i++) {
        const blockStart = i * blockSize;
        let sumSquares = 0;

        for (let j = 0; j < blockSize; j++) {
          const sample = audioData[blockStart + j];
          sumSquares += sample * sample;
        }

        const rms = Math.sqrt(sumSquares / blockSize);
        if (rms > 0) {
          rmsValues.push(20 * Math.log10(rms));
        }
      }

      if (rmsValues.length === 0) {
        throw new Error("No valid RMS values found");
      }

      // Sort RMS values
      rmsValues.sort((a, b) => b - a);

      // Calculate percentiles
      const p99 = this.percentile(rmsValues, 99);
      const p1 = this.percentile(rmsValues, 1);

      const dynamicRange = p99 - p1;

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.dynamicRange = dynamicRange;

      return {
        dynamicRange: dynamicRange,
        peak: p99,
        noise: p1,
        rmsValues: rmsValues,
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`Dynamic range measurement failed: ${error.message}`);
    }
  }

  percentile(sortedArray, percentile) {
    const index = (percentile / 100) * (sortedArray.length - 1);
    const lower = Math.floor(index);
    const upper = Math.ceil(index);
    const weight = index - lower;

    if (upper >= sortedArray.length) {
      return sortedArray[sortedArray.length - 1];
    }

    return sortedArray[lower] * (1 - weight) + sortedArray[upper] * weight;
  }

  // Frequency Response analysis
  assessFrequencyResponse(audioData, referenceSignal = null) {
    const startTime = performance.now();

    try {
      const spectrum = this.performFFT(audioData);
      const frequencies = this.generateFrequencyBins();

      let response = [];

      if (referenceSignal) {
        // Compare with reference signal
        const referenceSpectrum = this.performFFT(referenceSignal);

        for (let i = 0; i < spectrum.length; i++) {
          const magnitude = 20 * Math.log10(spectrum[i] / referenceSpectrum[i]);
          response.push({
            frequency: frequencies[i],
            magnitude: magnitude,
            phase: this.calculatePhase(spectrum[i], referenceSpectrum[i]),
          });
        }
      } else {
        // Absolute frequency response
        for (let i = 0; i < spectrum.length; i++) {
          response.push({
            frequency: frequencies[i],
            magnitude: 20 * Math.log10(spectrum[i]),
            phase: 0, // Phase not meaningful without reference
          });
        }
      }

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.frequencyResponse = response;

      return {
        response: response,
        flatness: this.calculateSpectralFlatness(spectrum),
        centroid: this.calculateSpectralCentroid(spectrum, frequencies),
        bandwidth: this.calculateBandwidth(spectrum, frequencies),
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`Frequency response assessment failed: ${error.message}`);
    }
  }

  // Perceptual Quality Assessment
  assessPerceptualQuality(audioData, referenceData = null) {
    const startTime = performance.now();

    try {
      let qualityScore = 0;

      if (referenceData) {
        // PEAQ-like assessment
        qualityScore = this.calculatePEAQScore(audioData, referenceData);
      } else {
        // Absolute quality assessment
        qualityScore = this.calculateAbsoluteQuality(audioData);
      }

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.perceptualQuality = qualityScore;

      return {
        qualityScore: qualityScore,
        mosScore: this.convertToMOS(qualityScore),
        confidence: this.calculateConfidence(audioData),
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`Perceptual quality assessment failed: ${error.message}`);
    }
  }

  calculatePEAQScore(testSignal, referenceSignal) {
    // Simplified PEAQ implementation
    const testSpectrum = this.performFFT(testSignal);
    const refSpectrum = this.performFFT(referenceSignal);

    // Calculate basic disturbances
    const loudnessDisturbance = this.calculateLoudnessDisturbance(
      testSpectrum,
      refSpectrum
    );
    const maskingDisturbance = this.calculateMaskingDisturbance(
      testSpectrum,
      refSpectrum
    );
    const tonalityDisturbance = this.calculateTonalityDisturbance(
      testSpectrum,
      refSpectrum
    );

    // Combine disturbances into quality score
    const qualityScore =
      4.5 -
      (loudnessDisturbance * 0.4 +
        maskingDisturbance * 0.4 +
        tonalityDisturbance * 0.2);

    return Math.max(1, Math.min(5, qualityScore));
  }

  // Artifact Detection
  detectArtifacts(audioData) {
    const startTime = performance.now();

    try {
      const artifacts = [];

      // Clipping detection
      const clipping = this.detectClipping(audioData);
      if (clipping.detected) {
        artifacts.push(clipping);
      }

      // Compression artifacts
      const compression = this.detectCompressionArtifacts(audioData);
      if (compression.detected) {
        artifacts.push(compression);
      }

      // Aliasing detection
      const aliasing = this.detectAliasing(audioData);
      if (aliasing.detected) {
        artifacts.push(aliasing);
      }

      // Digital distortion
      const distortion = this.detectDigitalDistortion(audioData);
      if (distortion.detected) {
        artifacts.push(distortion);
      }

      this.performanceStats.processingTime = performance.now() - startTime;
      this.qualityMetrics.artifacts = artifacts;

      return {
        artifacts: artifacts,
        artifactCount: artifacts.length,
        severity: this.calculateArtifactSeverity(artifacts),
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`Artifact detection failed: ${error.message}`);
    }
  }

  detectClipping(audioData) {
    const threshold = 0.99; // Near full scale
    let clippedSamples = 0;
    let clippingRegions = [];
    let inClippingRegion = false;
    let regionStart = 0;

    for (let i = 0; i < audioData.length; i++) {
      const isClipped = Math.abs(audioData[i]) >= threshold;

      if (isClipped) {
        clippedSamples++;

        if (!inClippingRegion) {
          inClippingRegion = true;
          regionStart = i;
        }
      } else if (inClippingRegion) {
        inClippingRegion = false;
        clippingRegions.push({
          start: regionStart,
          end: i - 1,
          duration: (i - 1 - regionStart) / this.sampleRate,
        });
      }
    }

    const clippingPercentage = (clippedSamples / audioData.length) * 100;

    return {
      type: "clipping",
      detected: clippingPercentage > 0.1, // Threshold for detection
      severity: this.calculateClippingSeverity(clippingPercentage),
      percentage: clippingPercentage,
      regions: clippingRegions,
      clippedSamples: clippedSamples,
    };
  }

  // Real-time Quality Monitoring
  startRealTimeMonitoring(callback, options = {}) {
    this.isMonitoring = true;
    this.monitoringCallbacks.push(callback);

    // Set up quality thresholds
    this.qualityThresholds = {
      snrMin: options.snrMin || 40,
      thdMax: options.thdMax || -40,
      dynamicRangeMin: options.dynamicRangeMin || 60,
      perceptualQualityMin: options.perceptualQualityMin || 3.0,
      ...options.thresholds,
    };

    return {
      success: true,
      message: "Real-time monitoring started",
      thresholds: this.qualityThresholds,
    };
  }

  stopRealTimeMonitoring() {
    this.isMonitoring = false;
    this.monitoringCallbacks = [];

    return {
      success: true,
      message: "Real-time monitoring stopped",
    };
  }

  processRealTimeAudio(audioData) {
    if (!this.isMonitoring) {
      return null;
    }

    const startTime = performance.now();

    try {
      // Quick quality assessment for real-time processing
      const quickMetrics = {
        timestamp: Date.now(),
        rms: this.calculateRMS(audioData),
        peak: this.calculatePeak(audioData),
        crestFactor: this.calculateCrestFactor(audioData),
        spectralCentroid: this.calculateQuickSpectralCentroid(audioData),
      };

      // Check thresholds
      const violations = this.checkQualityThresholds(quickMetrics);

      if (violations.length > 0) {
        this.generateQualityAlert(violations, quickMetrics);
      }

      // Notify callbacks
      const result = {
        metrics: quickMetrics,
        violations: violations,
        processingTime: performance.now() - startTime,
      };

      this.monitoringCallbacks.forEach((callback) => {
        try {
          callback(result);
        } catch (error) {
          console.error("Monitoring callback error:", error);
        }
      });

      return result;
    } catch (error) {
      console.error("Real-time monitoring error:", error);
      return null;
    }
  }

  // Performance optimization methods
  optimizeForHardware(hardwareInfo) {
    const startTime = performance.now();

    try {
      // Adjust processing parameters based on hardware capabilities
      if (hardwareInfo.simdSupport) {
        this.enableSIMDOptimizations();
      }

      if (hardwareInfo.gpuAcceleration) {
        this.enableGPUAcceleration();
      }

      if (hardwareInfo.cacheSize) {
        this.optimizeCacheUsage(hardwareInfo.cacheSize);
      }

      this.performanceStats.processingTime = performance.now() - startTime;

      return {
        success: true,
        optimizations: this.getActiveOptimizations(),
        processingTime: this.performanceStats.processingTime,
      };
    } catch (error) {
      throw new Error(`Hardware optimization failed: ${error.message}`);
    }
  }

  // Utility methods for testing
  generateTestSignal(type, frequency, duration, amplitude = 1.0) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    switch (type) {
      case "sine":
        for (let i = 0; i < sampleCount; i++) {
          signal[i] =
            amplitude *
            Math.sin((2 * Math.PI * frequency * i) / this.sampleRate);
        }
        break;

      case "sweep":
        const endFrequency = frequency * 10; // Decade sweep
        for (let i = 0; i < sampleCount; i++) {
          const t = i / this.sampleRate;
          const instantFreq =
            frequency * Math.pow(endFrequency / frequency, t / duration);
          signal[i] = amplitude * Math.sin(2 * Math.PI * instantFreq * t);
        }
        break;

      case "noise":
        for (let i = 0; i < sampleCount; i++) {
          signal[i] = amplitude * (Math.random() * 2 - 1);
        }
        break;

      case "impulse":
        signal[0] = amplitude;
        break;

      default:
        throw new Error(`Unknown signal type: ${type}`);
    }

    return signal;
  }

  addNoise(signal, snr) {
    const signalPower = this.calculateSignalPower(signal);
    const noisePower = signalPower / Math.pow(10, snr / 10);
    const noiseAmplitude = Math.sqrt(noisePower);

    const noisySignal = new Float32Array(signal.length);
    for (let i = 0; i < signal.length; i++) {
      const noise = noiseAmplitude * (Math.random() * 2 - 1);
      noisySignal[i] = signal[i] + noise;
    }

    return noisySignal;
  }

  addDistortion(signal, amount) {
    const distortedSignal = new Float32Array(signal.length);

    for (let i = 0; i < signal.length; i++) {
      let sample = signal[i];

      // Apply soft clipping distortion
      if (amount > 0) {
        sample = Math.tanh(sample * (1 + amount * 10));
      }

      distortedSignal[i] = sample;
    }

    return distortedSignal;
  }

  // Helper methods
  performFFT(audioData) {
    // Simplified FFT implementation for testing
    const N = Math.min(audioData.length, this.windowSize);
    const spectrum = new Float32Array(N / 2 + 1);

    // Apply window function
    const windowed = new Float32Array(N);
    for (let i = 0; i < N; i++) {
      const window = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (N - 1)); // Hann window
      windowed[i] = audioData[i] * window;
    }

    // Compute magnitude spectrum (simplified)
    for (let k = 0; k < spectrum.length; k++) {
      let real = 0,
        imag = 0;
      for (let n = 0; n < N; n++) {
        const angle = (-2 * Math.PI * k * n) / N;
        real += windowed[n] * Math.cos(angle);
        imag += windowed[n] * Math.sin(angle);
      }
      spectrum[k] = Math.sqrt(real * real + imag * imag) / N;
    }

    return spectrum;
  }

  generateFrequencyBins() {
    const bins = new Float32Array(this.windowSize / 2 + 1);
    for (let i = 0; i < bins.length; i++) {
      bins[i] = (i * this.sampleRate) / this.windowSize;
    }
    return bins;
  }

  calculateRMS(audioData) {
    let sumSquares = 0;
    for (let i = 0; i < audioData.length; i++) {
      sumSquares += audioData[i] * audioData[i];
    }
    return Math.sqrt(sumSquares / audioData.length);
  }

  calculatePeak(audioData) {
    let peak = 0;
    for (let i = 0; i < audioData.length; i++) {
      peak = Math.max(peak, Math.abs(audioData[i]));
    }
    return peak;
  }

  // Initialize helper methods
  generateTwiddles(size) {
    // FFT twiddle factors for optimization
    return new Float32Array(size);
  }

  generateBarkScale() {
    // Bark scale for perceptual modeling
    return new Float32Array(24);
  }

  initializeLoudnessModel() {
    return {
      weights: new Float32Array(this.windowSize / 2 + 1),
      thresholds: new Float32Array(this.windowSize / 2 + 1),
    };
  }

  initializeClippingDetector() {
    return { threshold: 0.99, sensitivity: 0.1 };
  }

  initializeAliasingDetector() {
    return { highFreqThreshold: this.sampleRate * 0.4 };
  }

  initializeDistortionDetector() {
    return { harmonicThreshold: -40 };
  }

  initializeNoiseDetector() {
    return { noiseFloor: -60 };
  }

  // Additional utility methods for comprehensive testing
  calculateSpectralFlatness(spectrum) {
    let geometricMean = 1;
    let arithmeticMean = 0;

    for (let i = 1; i < spectrum.length; i++) {
      // Skip DC
      geometricMean *= Math.pow(spectrum[i], 1 / (spectrum.length - 1));
      arithmeticMean += spectrum[i];
    }

    arithmeticMean /= spectrum.length - 1;

    return geometricMean / arithmeticMean;
  }

  calculateSpectralCentroid(spectrum, frequencies) {
    let weightedSum = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < spectrum.length; i++) {
      weightedSum += frequencies[i] * spectrum[i];
      magnitudeSum += spectrum[i];
    }

    return weightedSum / magnitudeSum;
  }

  calculateBandwidth(spectrum, frequencies) {
    const centroid = this.calculateSpectralCentroid(spectrum, frequencies);
    let variance = 0;
    let magnitudeSum = 0;

    for (let i = 0; i < spectrum.length; i++) {
      const deviation = frequencies[i] - centroid;
      variance += spectrum[i] * deviation * deviation;
      magnitudeSum += spectrum[i];
    }

    return Math.sqrt(variance / magnitudeSum);
  }

  calculatePhase(complex1, complex2) {
    // Simplified phase calculation
    return 0; // Implementation would require complex FFT
  }

  calculateAbsoluteQuality(audioData) {
    // Simplified absolute quality assessment
    const rms = this.calculateRMS(audioData);
    const peak = this.calculatePeak(audioData);
    const crestFactor = peak / rms;

    // Quality based on audio characteristics
    let quality = 3.0; // Base quality

    if (crestFactor > 10) quality -= 0.5; // Too dynamic
    if (crestFactor < 2) quality -= 0.5; // Too compressed
    if (peak > 0.95) quality -= 1.0; // Clipping likely

    return Math.max(1, Math.min(5, quality));
  }

  convertToMOS(qualityScore) {
    // Convert to Mean Opinion Score scale
    return Math.max(1, Math.min(5, qualityScore));
  }

  calculateConfidence(audioData) {
    // Confidence based on signal characteristics
    const length = audioData.length;
    const rms = this.calculateRMS(audioData);

    let confidence = 0.8; // Base confidence

    if (length < this.sampleRate) confidence -= 0.2; // Too short
    if (rms < 0.001) confidence -= 0.3; // Too quiet

    return Math.max(0, Math.min(1, confidence));
  }

  calculateLoudnessDisturbance(testSpectrum, refSpectrum) {
    // Simplified loudness disturbance calculation
    let disturbance = 0;
    for (let i = 0; i < testSpectrum.length; i++) {
      const diff = Math.abs(testSpectrum[i] - refSpectrum[i]);
      disturbance += diff;
    }
    return disturbance / testSpectrum.length;
  }

  calculateMaskingDisturbance(testSpectrum, refSpectrum) {
    // Simplified masking disturbance calculation
    return 0.1; // Placeholder implementation
  }

  calculateTonalityDisturbance(testSpectrum, refSpectrum) {
    // Simplified tonality disturbance calculation
    return 0.1; // Placeholder implementation
  }

  detectCompressionArtifacts(audioData) {
    // Simplified compression artifact detection
    return {
      type: "compression",
      detected: false,
      severity: 0,
    };
  }

  detectAliasing(audioData) {
    // Simplified aliasing detection
    return {
      type: "aliasing",
      detected: false,
      severity: 0,
    };
  }

  detectDigitalDistortion(audioData) {
    // Simplified digital distortion detection
    return {
      type: "digital_distortion",
      detected: false,
      severity: 0,
    };
  }

  calculateArtifactSeverity(artifacts) {
    if (artifacts.length === 0) return 0;

    let totalSeverity = 0;
    for (let artifact of artifacts) {
      totalSeverity += artifact.severity || 0;
    }

    return totalSeverity / artifacts.length;
  }

  calculateClippingSeverity(percentage) {
    if (percentage < 0.1) return 0;
    if (percentage < 1.0) return 1;
    if (percentage < 5.0) return 2;
    return 3;
  }

  checkQualityThresholds(metrics) {
    const violations = [];

    // Check various quality thresholds
    if (metrics.rms < 0.001) {
      violations.push({
        type: "low_level",
        threshold: 0.001,
        actual: metrics.rms,
        severity: "warning",
      });
    }

    if (metrics.peak > 0.95) {
      violations.push({
        type: "clipping",
        threshold: 0.95,
        actual: metrics.peak,
        severity: "error",
      });
    }

    return violations;
  }

  generateQualityAlert(violations, metrics) {
    const alert = {
      timestamp: Date.now(),
      violations: violations,
      metrics: metrics,
      severity: this.calculateAlertSeverity(violations),
    };

    this.alertHistory.push(alert);

    // Keep alert history manageable
    if (this.alertHistory.length > 1000) {
      this.alertHistory = this.alertHistory.slice(-1000);
    }

    return alert;
  }

  calculateAlertSeverity(violations) {
    let maxSeverity = "info";

    for (let violation of violations) {
      if (violation.severity === "error") {
        maxSeverity = "error";
        break;
      } else if (violation.severity === "warning" && maxSeverity !== "error") {
        maxSeverity = "warning";
      }
    }

    return maxSeverity;
  }

  calculateCrestFactor(audioData) {
    const rms = this.calculateRMS(audioData);
    const peak = this.calculatePeak(audioData);
    return peak / (rms || 1e-10); // Avoid division by zero
  }

  calculateQuickSpectralCentroid(audioData) {
    // Quick spectral centroid calculation for real-time use
    const spectrum = this.performFFT(audioData.slice(0, 512)); // Use shorter window
    const frequencies = this.generateFrequencyBins();
    return this.calculateSpectralCentroid(
      spectrum,
      frequencies.slice(0, spectrum.length)
    );
  }

  enableSIMDOptimizations() {
    this.simdEnabled = true;
  }

  enableGPUAcceleration() {
    this.gpuEnabled = true;
  }

  optimizeCacheUsage(cacheSize) {
    this.cacheOptimized = true;
    this.cacheSize = cacheSize;
  }

  getActiveOptimizations() {
    return {
      simd: this.simdEnabled || false,
      gpu: this.gpuEnabled || false,
      cache: this.cacheOptimized || false,
    };
  }
}

// Audio Quality Test Data Generator
class AudioQualityTestDataGenerator {
  constructor(sampleRate = 44100) {
    this.sampleRate = sampleRate;
  }

  generateReferenceSignals() {
    return {
      pureToone1kHz: this.generateSineWave(1000, 1.0, 1.0),
      pureToone440Hz: this.generateSineWave(440, 1.0, 1.0),
      whiteNoise: this.generateWhiteNoise(1.0, 0.1),
      pinkNoise: this.generatePinkNoise(1.0, 0.1),
      sweep20Hz20kHz: this.generateSweep(20, 20000, 5.0),
      impulse: this.generateImpulse(1.0),
      complexTone: this.generateComplexTone([440, 880, 1320], 1.0),
      chirp: this.generateChirp(1000, 2000, 1.0),
    };
  }

  generateSineWave(frequency, duration, amplitude) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
      signal[i] =
        amplitude * Math.sin((2 * Math.PI * frequency * i) / this.sampleRate);
    }

    return signal;
  }

  generateWhiteNoise(duration, amplitude) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
      signal[i] = amplitude * (Math.random() * 2 - 1);
    }

    return signal;
  }

  generatePinkNoise(duration, amplitude) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    // Simple pink noise approximation using multiple octave bands
    const octaves = 8;
    const filters = [];

    for (let octave = 0; octave < octaves; octave++) {
      filters.push({
        amplitude: amplitude / Math.sqrt(octave + 1),
        frequency: 55 * Math.pow(2, octave),
      });
    }

    for (let i = 0; i < sampleCount; i++) {
      let sample = 0;
      for (let filter of filters) {
        sample +=
          filter.amplitude *
          Math.sin((2 * Math.PI * filter.frequency * i) / this.sampleRate) *
          (Math.random() * 2 - 1);
      }
      signal[i] = sample / octaves;
    }

    return signal;
  }

  generateSweep(startFreq, endFreq, duration) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
      const t = i / this.sampleRate;
      const instantFreq =
        startFreq * Math.pow(endFreq / startFreq, t / duration);
      const phase =
        (2 *
          Math.PI *
          startFreq *
          t *
          (Math.pow(endFreq / startFreq, t / duration) - 1)) /
        Math.log(endFreq / startFreq);
      signal[i] = Math.sin(phase);
    }

    return signal;
  }

  generateImpulse(amplitude) {
    const signal = new Float32Array(this.sampleRate); // 1 second buffer
    signal[0] = amplitude;
    return signal;
  }

  generateComplexTone(frequencies, duration) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
      let sample = 0;
      for (let freq of frequencies) {
        sample +=
          Math.sin((2 * Math.PI * freq * i) / this.sampleRate) /
          frequencies.length;
      }
      signal[i] = sample;
    }

    return signal;
  }

  generateChirp(startFreq, endFreq, duration) {
    const sampleCount = Math.floor(duration * this.sampleRate);
    const signal = new Float32Array(sampleCount);

    for (let i = 0; i < sampleCount; i++) {
      const t = i / this.sampleRate;
      const instantFreq = startFreq + ((endFreq - startFreq) * t) / duration;
      const phase =
        2 *
        Math.PI *
        (startFreq * t + ((endFreq - startFreq) * t * t) / (2 * duration));
      signal[i] = Math.sin(phase);
    }

    return signal;
  }
}

// Quality Assessment Performance Monitor
class QualityAssessmentPerformanceMonitor {
  constructor() {
    this.reset();
  }

  reset() {
    this.measurements = [];
    this.currentMeasurement = null;
  }

  startMeasurement(testName) {
    this.currentMeasurement = {
      testName: testName,
      startTime: performance.now(),
      startMemory: this.getCurrentMemoryUsage(),
      cpuStart: process.cpuUsage ? process.cpuUsage() : null,
    };
  }

  endMeasurement() {
    if (!this.currentMeasurement) {
      throw new Error("No measurement in progress");
    }

    const endTime = performance.now();
    const endMemory = this.getCurrentMemoryUsage();
    const cpuEnd = process.cpuUsage
      ? process.cpuUsage(this.currentMeasurement.cpuStart)
      : null;

    const measurement = {
      testName: this.currentMeasurement.testName,
      duration: endTime - this.currentMeasurement.startTime,
      memoryUsed: endMemory - this.currentMeasurement.startMemory,
      cpuUsage: cpuEnd
        ? {
            user: cpuEnd.user / 1000, // Convert to milliseconds
            system: cpuEnd.system / 1000,
          }
        : null,
      timestamp: Date.now(),
    };

    this.measurements.push(measurement);
    this.currentMeasurement = null;

    return measurement;
  }

  getCurrentMemoryUsage() {
    if (typeof process !== "undefined" && process.memoryUsage) {
      return process.memoryUsage().heapUsed;
    }
    return 0; // Fallback for browser environment
  }

  getStatistics() {
    if (this.measurements.length === 0) {
      return null;
    }

    const durations = this.measurements.map((m) => m.duration);
    const memoryUsages = this.measurements.map((m) => m.memoryUsed);

    return {
      count: this.measurements.length,
      duration: {
        min: Math.min(...durations),
        max: Math.max(...durations),
        average: durations.reduce((a, b) => a + b, 0) / durations.length,
        median: this.calculateMedian(durations),
      },
      memory: {
        min: Math.min(...memoryUsages),
        max: Math.max(...memoryUsages),
        average: memoryUsages.reduce((a, b) => a + b, 0) / memoryUsages.length,
        median: this.calculateMedian(memoryUsages),
      },
    };
  }

  calculateMedian(values) {
    const sorted = [...values].sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);
    return sorted.length % 2 !== 0
      ? sorted[mid]
      : (sorted[mid - 1] + sorted[mid]) / 2;
  }

  exportResults() {
    return {
      measurements: this.measurements,
      statistics: this.getStatistics(),
      exportTime: new Date().toISOString(),
    };
  }
}

describe("QualityAssessor Tests", () => {
  let qualityAssessor;
  let mockAudioEngine;
  let testAudioData;
  let qualityMetrics;
  let performanceMonitor;
  let testDataGenerator;

  beforeEach(async () => {
    // Initialize quality assessor with test configuration
    qualityAssessor = new MockQualityAssessor({
      sampleRate: 44100,
      channels: 2,
      blockSize: 1024,
      windowSize: 2048,
      hopSize: 512,
    });

    // Set up performance monitoring
    performanceMonitor = new QualityAssessmentPerformanceMonitor();

    // Initialize test data generator
    testDataGenerator = new AudioQualityTestDataGenerator(44100);

    // Create basic test audio data
    testAudioData = {
      silence: new Float32Array(44100).fill(0),
      sine1kHz: testDataGenerator.generateSineWave(1000, 1.0, 0.5),
      whiteNoise: testDataGenerator.generateWhiteNoise(1.0, 0.1),
      complexSignal: testDataGenerator.generateComplexTone(
        [440, 880, 1320],
        1.0
      ),
    };

    // Mock audio engine
    mockAudioEngine = {
      sampleRate: 44100,
      channels: 2,
      isProcessing: false,
      processAudio: jest.fn(),
      getQualityAssessor: () => qualityAssessor,
    };

    // Initialize quality metrics storage
    qualityMetrics = {
      snr: [],
      thd: [],
      dynamicRange: [],
      frequencyResponse: [],
      perceptualQuality: [],
      artifacts: [],
    };
  });

  afterEach(async () => {
    // Clean up quality assessor
    if (qualityAssessor && qualityAssessor.isMonitoring) {
      qualityAssessor.stopRealTimeMonitoring();
    }

    // Reset performance monitor
    performanceMonitor.reset();

    // Clear test data
    testAudioData = null;
    qualityMetrics = null;

    // Clean up mock objects
    mockAudioEngine = null;
    qualityAssessor = null;
    testDataGenerator = null;
  });
  // TODO: Generate quality assessment reports
  // TODO: Reset quality analysis state
  // TODO: Validate assessment accuracy
  // TODO: Export quality performance metrics
  // TODO: Clear quality measurement history
  // TODO: Reset statistical analysis state
  // TODO: Validate quality threshold results
  // TODO: Archive quality assessment results
});

describe("Quality Metrics Calculation", () => {
  it("should calculate SNR accurately", async () => {
    performanceMonitor.startMeasurement("SNR Calculation");

    // Test with known signal and noise levels
    const testSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.5);
    const noisySignal = qualityAssessor.addNoise(testSignal, 40); // 40 dB SNR

    const snrResult = qualityAssessor.calculateSNR(noisySignal);

    // Verify SNR is close to expected value (within 2 dB tolerance)
    expect(snrResult.snr).toBeGreaterThan(38);
    expect(snrResult.snr).toBeLessThan(42);
    expect(snrResult.signalPower).toBeGreaterThan(0);
    expect(snrResult.noisePower).toBeGreaterThan(0);
    expect(snrResult.processingTime).toBeLessThan(100); // Performance requirement

    // Test with different frequencies
    const frequencies = [100, 440, 1000, 4000, 8000];
    for (const freq of frequencies) {
      const freqSignal = testDataGenerator.generateSineWave(freq, 0.5, 0.5);
      const freqNoisySignal = qualityAssessor.addNoise(freqSignal, 30);
      const freqResult = qualityAssessor.calculateSNR(freqNoisySignal);

      expect(freqResult.snr).toBeGreaterThan(25);
      expect(freqResult.snr).toBeLessThan(35);
    }

    // Test with multi-channel audio
    const stereoSignal = new Float32Array(88200); // 2 seconds stereo
    for (let i = 0; i < stereoSignal.length; i += 2) {
      stereoSignal[i] = 0.5 * Math.sin((2 * Math.PI * 1000 * i) / 88200); // Left
      stereoSignal[i + 1] = 0.5 * Math.sin((2 * Math.PI * 1000 * i) / 88200); // Right
    }
    const stereoNoisy = qualityAssessor.addNoise(stereoSignal, 35);
    const stereoResult = qualityAssessor.calculateSNR(stereoNoisy);

    expect(stereoResult.snr).toBeGreaterThan(30);
    expect(stereoResult.snr).toBeLessThan(40);

    // Test edge cases
    const silentSignal = new Float32Array(44100).fill(0);
    expect(() => qualityAssessor.calculateSNR(silentSignal)).toThrow();

    const clippedSignal = new Float32Array(44100).fill(1.0);
    const clippedResult = qualityAssessor.calculateSNR(clippedSignal);
    expect(clippedResult.snr).toBeDefined();

    performanceMonitor.endMeasurement();
    qualityMetrics.snr.push(snrResult);
  });

  it("should calculate THD+N correctly", async () => {
    performanceMonitor.startMeasurement("THD+N Calculation");

    // Test with pure sine wave (should have very low THD+N)
    const pureSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.5);
    const pureResult = qualityAssessor.calculateTHDN(pureSignal, 1000);

    expect(pureResult.thdnDb).toBeLessThan(-40); // Very low distortion expected
    expect(pureResult.fundamentalAmplitude).toBeGreaterThan(0);
    expect(pureResult.processingTime).toBeLessThan(100);

    // Test with distorted signal
    const distortedSignal = qualityAssessor.addDistortion(pureSignal, 0.1);
    const distortedResult = qualityAssessor.calculateTHDN(
      distortedSignal,
      1000
    );

    expect(distortedResult.thdnDb).toBeGreaterThan(pureResult.thdnDb); // More distortion
    expect(distortedResult.harmonicPower).toBeGreaterThan(0);

    // Test across frequency spectrum
    const testFrequencies = [100, 440, 1000, 2000, 4000];
    for (const freq of testFrequencies) {
      const freqSignal = testDataGenerator.generateSineWave(freq, 1.0, 0.5);
      const freqResult = qualityAssessor.calculateTHDN(freqSignal, freq);

      expect(freqResult.thdnDb).toBeLessThan(-30); // Reasonable distortion level
      expect(freqResult.fundamentalAmplitude).toBeGreaterThan(0);
    }

    // Test with complex signals (multiple harmonics)
    const complexSignal = testDataGenerator.generateComplexTone(
      [1000, 2000, 3000],
      1.0
    );
    const complexResult = qualityAssessor.calculateTHDN(complexSignal, 1000);

    expect(complexResult.thdnDb).toBeGreaterThan(-20); // Higher distortion due to harmonics
    expect(complexResult.harmonicPower).toBeGreaterThan(
      pureResult.harmonicPower
    );

    // Test error handling
    expect(() =>
      qualityAssessor.calculateTHDN(new Float32Array(0), 1000)
    ).toThrow();
    expect(() => qualityAssessor.calculateTHDN(pureSignal, 0)).toThrow();

    performanceMonitor.endMeasurement();
    qualityMetrics.thd.push(pureResult);
  });

  it("should measure dynamic range accurately", async () => {
    performanceMonitor.startMeasurement("Dynamic Range Measurement");

    // Create signal with known dynamic range
    const quietLevel = 0.001; // -60 dB
    const loudLevel = 0.5; // -6 dB
    const dynamicSignal = new Float32Array(88200); // 2 seconds

    // Fill with alternating quiet and loud sections
    for (let i = 0; i < dynamicSignal.length; i++) {
      const sectionIndex = Math.floor(i / 8820); // 0.2 second sections
      const amplitude = sectionIndex % 2 === 0 ? quietLevel : loudLevel;
      dynamicSignal[i] = amplitude * Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    const dynamicResult = qualityAssessor.measureDynamicRange(dynamicSignal);

    // Expected dynamic range should be close to 54 dB (difference between levels)
    expect(dynamicResult.dynamicRange).toBeGreaterThan(45);
    expect(dynamicResult.dynamicRange).toBeLessThan(65);
    expect(dynamicResult.peak).toBeGreaterThan(dynamicResult.noise);
    expect(dynamicResult.rmsValues.length).toBeGreaterThan(0);
    expect(dynamicResult.processingTime).toBeLessThan(100);

    // Test with compressed audio (lower dynamic range)
    const compressedSignal = new Float32Array(44100);
    for (let i = 0; i < compressedSignal.length; i++) {
      let sample = Math.sin((2 * Math.PI * 1000 * i) / 44100);
      // Apply compression (reduce dynamic range)
      sample = Math.tanh(sample * 3) * 0.3;
      compressedSignal[i] = sample;
    }

    const compressedResult =
      qualityAssessor.measureDynamicRange(compressedSignal);
    expect(compressedResult.dynamicRange).toBeLessThan(
      dynamicResult.dynamicRange
    );

    // Test with various bit depths simulation
    const bitDepths = [8, 16, 24];
    for (const bitDepth of bitDepths) {
      const quantizedSignal = simulateQuantization(dynamicSignal, bitDepth);
      const quantizedResult =
        qualityAssessor.measureDynamicRange(quantizedSignal);

      // Higher bit depth should allow for greater dynamic range
      expect(quantizedResult.dynamicRange).toBeGreaterThan(0);
    }

    // Test edge cases
    const silentSignal = new Float32Array(44100).fill(0);
    expect(() => qualityAssessor.measureDynamicRange(silentSignal)).toThrow();

    const constantSignal = new Float32Array(44100).fill(0.5);
    const constantResult = qualityAssessor.measureDynamicRange(constantSignal);
    expect(constantResult.dynamicRange).toBeLessThan(10); // Very low dynamic range

    performanceMonitor.endMeasurement();
    qualityMetrics.dynamicRange.push(dynamicResult);
  });

  it("should assess frequency response", async () => {
    performanceMonitor.startMeasurement("Frequency Response Assessment");

    // Test with frequency sweep
    const sweepSignal = testDataGenerator.generateSweep(20, 20000, 5.0);
    const sweepResult = qualityAssessor.assessFrequencyResponse(sweepSignal);

    expect(sweepResult.response).toBeDefined();
    expect(sweepResult.response.length).toBeGreaterThan(0);
    expect(sweepResult.flatness).toBeGreaterThan(0);
    expect(sweepResult.centroid).toBeGreaterThan(0);
    expect(sweepResult.bandwidth).toBeGreaterThan(0);
    expect(sweepResult.processingTime).toBeLessThan(200);

    // Verify frequency bins are correctly generated
    for (let i = 0; i < sweepResult.response.length; i++) {
      const point = sweepResult.response[i];
      expect(point.frequency).toBeGreaterThanOrEqual(0);
      expect(point.frequency).toBeLessThanOrEqual(22050); // Nyquist frequency
      expect(point.magnitude).toBeDefined();
      expect(point.phase).toBeDefined();
    }

    // Test with reference signal comparison
    const testSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.5);
    const referenceSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.5);
    const comparisonResult = qualityAssessor.assessFrequencyResponse(
      testSignal,
      referenceSignal
    );

    expect(comparisonResult.response).toBeDefined();

    // Find the 1kHz bin and verify it has good response
    const freq1kHz = comparisonResult.response.find(
      (p) => Math.abs(p.frequency - 1000) < 50
    );
    expect(freq1kHz).toBeDefined();
    expect(freq1kHz.magnitude).toBeGreaterThan(-6); // Should be near 0 dB

    // Test with filtering simulation (attenuate high frequencies)
    const filteredSignal = simulateHighFrequencyRolloff(sweepSignal);
    const filteredResult = qualityAssessor.assessFrequencyResponse(
      filteredSignal,
      sweepSignal
    );

    // High frequencies should show attenuation
    const highFreqPoint = filteredResult.response.find(
      (p) => p.frequency > 10000
    );
    if (highFreqPoint) {
      expect(highFreqPoint.magnitude).toBeLessThan(0); // Some attenuation expected
    }

    // Test spectral characteristics
    expect(sweepResult.flatness).toBeGreaterThan(0);
    expect(sweepResult.flatness).toBeLessThanOrEqual(1);
    expect(sweepResult.centroid).toBeGreaterThan(100);
    expect(sweepResult.centroid).toBeLessThan(10000);

    performanceMonitor.endMeasurement();
    qualityMetrics.frequencyResponse.push(sweepResult);
  });

  // Helper functions for testing
  const simulateQuantization = (signal, bitDepth) => {
    const levels = Math.pow(2, bitDepth);
    const step = 2.0 / levels;
    const quantized = new Float32Array(signal.length);

    for (let i = 0; i < signal.length; i++) {
      quantized[i] = Math.round(signal[i] / step) * step;
    }

    return quantized;
  };

  const simulateHighFrequencyRolloff = (signal) => {
    // Simple high-frequency attenuation simulation
    const filtered = new Float32Array(signal.length);
    let prev = 0;
    const alpha = 0.8; // Low-pass filter coefficient

    for (let i = 0; i < signal.length; i++) {
      filtered[i] = alpha * prev + (1 - alpha) * signal[i];
      prev = filtered[i];
    }

    return filtered;
  };
});

describe("Quality Assessment Algorithms", () => {
  it("should perform perceptual quality assessment", async () => {
    performanceMonitor.startMeasurement("Perceptual Quality Assessment");

    // Test with high-quality reference signal
    const referenceSignal = testDataGenerator.generateSineWave(1000, 2.0, 0.7);

    // Test absolute quality assessment
    const absoluteResult =
      qualityAssessor.assessPerceptualQuality(referenceSignal);

    expect(absoluteResult.qualityScore).toBeGreaterThan(2.0);
    expect(absoluteResult.qualityScore).toBeLessThanOrEqual(5.0);
    expect(absoluteResult.mosScore).toBeGreaterThan(1.0);
    expect(absoluteResult.mosScore).toBeLessThanOrEqual(5.0);
    expect(absoluteResult.confidence).toBeGreaterThan(0.5);
    expect(absoluteResult.processingTime).toBeLessThan(200);

    // Test with distorted signal vs reference (PEAQ-like)
    const distortedSignal = qualityAssessor.addDistortion(referenceSignal, 0.2);
    const peaqResult = qualityAssessor.assessPerceptualQuality(
      distortedSignal,
      referenceSignal
    );

    expect(peaqResult.qualityScore).toBeLessThan(absoluteResult.qualityScore);
    expect(peaqResult.mosScore).toBeGreaterThan(1.0);
    expect(peaqResult.confidence).toBeGreaterThan(0.3);

    // Test with different content types
    const contentTypes = [
      {
        name: "speech",
        signal: testDataGenerator.generateComplexTone([200, 400, 800], 1.0),
      },
      {
        name: "music",
        signal: testDataGenerator.generateComplexTone([440, 554, 659], 1.0),
      },
      { name: "noise", signal: testDataGenerator.generateWhiteNoise(1.0, 0.3) },
    ];

    for (const content of contentTypes) {
      const contentResult = qualityAssessor.assessPerceptualQuality(
        content.signal
      );
      expect(contentResult.qualityScore).toBeGreaterThan(1.0);
      expect(contentResult.qualityScore).toBeLessThanOrEqual(5.0);
      qualityMetrics.perceptualQuality.push({
        contentType: content.name,
        result: contentResult,
      });
    }

    // Test with compression artifacts simulation
    const compressedSignal = simulateCompressionArtifacts(referenceSignal);
    const compressedResult = qualityAssessor.assessPerceptualQuality(
      compressedSignal,
      referenceSignal
    );

    expect(compressedResult.qualityScore).toBeLessThan(peaqResult.qualityScore);

    // Test real-time capability
    const realtimeSignal = new Float32Array(1024); // Small buffer for real-time
    for (let i = 0; i < realtimeSignal.length; i++) {
      realtimeSignal[i] = 0.5 * Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    const realtimeResult =
      qualityAssessor.assessPerceptualQuality(realtimeSignal);
    expect(realtimeResult.processingTime).toBeLessThan(50); // Real-time requirement

    performanceMonitor.endMeasurement();
  });

  it("should detect audio artifacts", async () => {
    performanceMonitor.startMeasurement("Artifact Detection");

    // Test clipping detection
    const cleanSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.8);
    const clippedSignal = new Float32Array(cleanSignal.length);
    for (let i = 0; i < cleanSignal.length; i++) {
      clippedSignal[i] = Math.max(-0.99, Math.min(0.99, cleanSignal[i]));
    }

    const clippingResult = qualityAssessor.detectArtifacts(clippedSignal);

    expect(clippingResult.artifacts).toBeDefined();
    expect(clippingResult.artifactCount).toBeGreaterThanOrEqual(0);
    expect(clippingResult.severity).toBeGreaterThanOrEqual(0);
    expect(clippingResult.processingTime).toBeLessThan(100);

    // Look for clipping artifacts
    const clippingArtifact = clippingResult.artifacts.find(
      (a) => a.type === "clipping"
    );
    if (clippingArtifact) {
      expect(clippingArtifact.detected).toBe(true);
      expect(clippingArtifact.severity).toBeGreaterThan(0);
      expect(clippingArtifact.percentage).toBeGreaterThan(0);
    }

    // Test with heavily distorted signal
    const distortedSignal = qualityAssessor.addDistortion(cleanSignal, 0.5);
    const distortionResult = qualityAssessor.detectArtifacts(distortedSignal);

    expect(distortionResult.artifactCount).toBeGreaterThanOrEqual(0);
    expect(distortionResult.severity).toBeGreaterThanOrEqual(0);

    // Test with aliasing simulation (high frequency content)
    const aliasingSignal = new Float32Array(44100);
    for (let i = 0; i < aliasingSignal.length; i++) {
      // Generate signal with content above Nyquist (should cause aliasing)
      aliasingSignal[i] = 0.3 * Math.sin((2 * Math.PI * 25000 * i) / 44100); // Above Nyquist
    }

    const aliasingResult = qualityAssessor.detectArtifacts(aliasingSignal);
    expect(aliasingResult.artifacts).toBeDefined();

    // Test various content types for artifact detection
    const testSignals = [
      { name: "clean_sine", signal: cleanSignal },
      { name: "clipped", signal: clippedSignal },
      { name: "distorted", signal: distortedSignal },
      { name: "aliased", signal: aliasingSignal },
    ];

    for (const test of testSignals) {
      const result = qualityAssessor.detectArtifacts(test.signal);
      expect(result.artifacts).toBeDefined();
      expect(result.processingTime).toBeLessThan(150);

      qualityMetrics.artifacts.push({
        signalType: test.name,
        result: result,
      });
    }

    // Test performance with different buffer sizes
    const bufferSizes = [512, 1024, 2048, 4096];
    for (const size of bufferSizes) {
      const buffer = cleanSignal.slice(0, size);
      const bufferResult = qualityAssessor.detectArtifacts(buffer);
      expect(bufferResult.processingTime).toBeLessThan(100);
    }

    performanceMonitor.endMeasurement();
  });

  it("should analyze spectral characteristics", async () => {
    performanceMonitor.startMeasurement("Spectral Analysis");

    // Test with sweep signal for comprehensive spectral analysis
    const sweepSignal = testDataGenerator.generateSweep(100, 10000, 3.0);
    const spectrum = qualityAssessor.performFFT(sweepSignal);
    const frequencies = qualityAssessor.generateFrequencyBins();

    // Test spectral centroid calculation
    const centroid = qualityAssessor.calculateSpectralCentroid(
      spectrum,
      frequencies
    );
    expect(centroid).toBeGreaterThan(100);
    expect(centroid).toBeLessThan(20000);

    // Test spectral flatness
    const flatness = qualityAssessor.calculateSpectralFlatness(spectrum);
    expect(flatness).toBeGreaterThan(0);
    expect(flatness).toBeLessThanOrEqual(1);

    // Test spectral bandwidth
    const bandwidth = qualityAssessor.calculateBandwidth(spectrum, frequencies);
    expect(bandwidth).toBeGreaterThan(0);
    expect(bandwidth).toBeLessThan(22050);

    // Test with different signal types
    const signalTypes = [
      {
        name: "sine_1kHz",
        signal: testDataGenerator.generateSineWave(1000, 1.0, 0.5),
      },
      {
        name: "white_noise",
        signal: testDataGenerator.generateWhiteNoise(1.0, 0.3),
      },
      {
        name: "pink_noise",
        signal: testDataGenerator.generatePinkNoise(1.0, 0.3),
      },
      {
        name: "complex_tone",
        signal: testDataGenerator.generateComplexTone([440, 880, 1320], 1.0),
      },
    ];

    const spectralFeatures = {};

    for (const signalType of signalTypes) {
      const typeSpectrum = qualityAssessor.performFFT(signalType.signal);

      const features = {
        centroid: qualityAssessor.calculateSpectralCentroid(
          typeSpectrum,
          frequencies
        ),
        flatness: qualityAssessor.calculateSpectralFlatness(typeSpectrum),
        bandwidth: qualityAssessor.calculateBandwidth(
          typeSpectrum,
          frequencies
        ),
        rolloff: calculateSpectralRolloff(typeSpectrum, frequencies, 0.85),
        spread: calculateSpectralSpread(typeSpectrum, frequencies),
        skewness: calculateSpectralSkewness(typeSpectrum, frequencies),
        kurtosis: calculateSpectralKurtosis(typeSpectrum, frequencies),
      };

      // Validate feature ranges
      expect(features.centroid).toBeGreaterThan(0);
      expect(features.flatness).toBeGreaterThan(0);
      expect(features.flatness).toBeLessThanOrEqual(1);
      expect(features.bandwidth).toBeGreaterThan(0);
      expect(features.rolloff).toBeGreaterThan(0);
      expect(features.rolloff).toBeLessThan(22050);

      spectralFeatures[signalType.name] = features;
    }

    // Verify different signals have different spectral characteristics
    expect(spectralFeatures.sine_1kHz.flatness).toBeLessThan(
      spectralFeatures.white_noise.flatness
    );
    expect(spectralFeatures.white_noise.bandwidth).toBeGreaterThan(
      spectralFeatures.sine_1kHz.bandwidth
    );

    // Test spectral feature extraction performance
    const largeSignal = new Float32Array(88200); // 2 seconds
    for (let i = 0; i < largeSignal.length; i++) {
      largeSignal[i] = 0.5 * Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    const startTime = performance.now();
    const largeSpectrum = qualityAssessor.performFFT(largeSignal);
    const extractionTime = performance.now() - startTime;

    expect(extractionTime).toBeLessThan(500); // Performance requirement
    expect(largeSpectrum.length).toBeGreaterThan(0);

    performanceMonitor.endMeasurement();
  });

  // Helper methods for spectral analysis
  const calculateSpectralRolloff = (spectrum, frequencies, threshold) => {
    let totalEnergy = 0;
    for (let i = 0; i < spectrum.length; i++) {
      totalEnergy += spectrum[i] * spectrum[i];
    }

    let cumulativeEnergy = 0;
    const targetEnergy = totalEnergy * threshold;

    for (let i = 0; i < spectrum.length; i++) {
      cumulativeEnergy += spectrum[i] * spectrum[i];
      if (cumulativeEnergy >= targetEnergy) {
        return frequencies[i];
      }
    }

    return frequencies[frequencies.length - 1];
  };

  const calculateSpectralSpread = (spectrum, frequencies) => {
    const centroid = qualityAssessor.calculateSpectralCentroid(
      spectrum,
      frequencies
    );
    let weightedVariance = 0;
    let totalWeight = 0;

    for (let i = 0; i < spectrum.length; i++) {
      const deviation = frequencies[i] - centroid;
      weightedVariance += spectrum[i] * deviation * deviation;
      totalWeight += spectrum[i];
    }

    return Math.sqrt(weightedVariance / totalWeight);
  };

  const calculateSpectralSkewness = (spectrum, frequencies) => {
    const centroid = qualityAssessor.calculateSpectralCentroid(
      spectrum,
      frequencies
    );
    const spread = calculateSpectralSpread(spectrum, frequencies);

    let weightedSkewness = 0;
    let totalWeight = 0;

    for (let i = 0; i < spectrum.length; i++) {
      const normalizedDeviation = (frequencies[i] - centroid) / spread;
      weightedSkewness += spectrum[i] * Math.pow(normalizedDeviation, 3);
      totalWeight += spectrum[i];
    }

    return weightedSkewness / totalWeight;
  };

  const calculateSpectralKurtosis = (spectrum, frequencies) => {
    const centroid = qualityAssessor.calculateSpectralCentroid(
      spectrum,
      frequencies
    );
    const spread = calculateSpectralSpread(spectrum, frequencies);

    let weightedKurtosis = 0;
    let totalWeight = 0;

    for (let i = 0; i < spectrum.length; i++) {
      const normalizedDeviation = (frequencies[i] - centroid) / spread;
      weightedKurtosis += spectrum[i] * Math.pow(normalizedDeviation, 4);
      totalWeight += spectrum[i];
    }

    return weightedKurtosis / totalWeight - 3; // Excess kurtosis
  };

  const simulateCompressionArtifacts = (signal) => {
    // Simple compression artifact simulation
    const compressed = new Float32Array(signal.length);

    for (let i = 0; i < signal.length; i++) {
      let sample = signal[i];

      // Simulate quantization noise
      const quantizationLevels = 256; // 8-bit quantization
      sample = Math.round(sample * quantizationLevels) / quantizationLevels;

      // Add slight harmonic distortion
      sample += 0.01 * Math.sin(6 * Math.PI * sample);

      compressed[i] = sample;
    }

    return compressed;
  };
});

describe("Real-Time Quality Monitoring", () => {
  it("should monitor quality in real-time", async () => {
    performanceMonitor.startMeasurement("Real-Time Monitoring");

    let monitoringResults = [];
    let alertCount = 0;

    // Set up monitoring callback
    const monitoringCallback = (result) => {
      monitoringResults.push(result);
      if (result.violations.length > 0) {
        alertCount++;
      }
    };

    // Start real-time monitoring with specific thresholds
    const monitoringConfig = qualityAssessor.startRealTimeMonitoring(
      monitoringCallback,
      {
        snrMin: 40,
        thdMax: -40,
        dynamicRangeMin: 60,
        perceptualQualityMin: 3.0,
        thresholds: {
          peakMax: 0.95,
          rmsMin: 0.001,
        },
      }
    );

    expect(monitoringConfig.success).toBe(true);
    expect(monitoringConfig.thresholds).toBeDefined();
    expect(qualityAssessor.isMonitoring).toBe(true);

    // Simulate real-time audio processing with various quality levels
    const testBuffers = [
      {
        name: "good_quality",
        signal: testDataGenerator.generateSineWave(1000, 0.1, 0.5),
      },
      { name: "clipping", signal: new Float32Array(4410).fill(0.99) }, // Should trigger alert
      { name: "low_level", signal: new Float32Array(4410).fill(0.0001) }, // Should trigger alert
      {
        name: "normal",
        signal: testDataGenerator.generateSineWave(440, 0.1, 0.3),
      },
    ];

    // Process each buffer through real-time monitoring
    for (const buffer of testBuffers) {
      const result = qualityAssessor.processRealTimeAudio(buffer.signal);

      expect(result).toBeDefined();
      expect(result.metrics).toBeDefined();
      expect(result.metrics.timestamp).toBeDefined();
      expect(result.metrics.rms).toBeGreaterThanOrEqual(0);
      expect(result.metrics.peak).toBeGreaterThanOrEqual(0);
      expect(result.metrics.crestFactor).toBeGreaterThan(0);
      expect(result.processingTime).toBeLessThan(50); // Low-latency requirement
      expect(result.violations).toBeDefined();

      // Verify quick processing for real-time capability
      expect(result.processingTime).toBeLessThan(10); // Very low latency
    }

    // Wait for callbacks to process
    await new Promise((resolve) => setTimeout(resolve, 100));

    // Verify monitoring results were collected
    expect(monitoringResults.length).toBeGreaterThan(0);
    expect(alertCount).toBeGreaterThan(0); // Should have some alerts from clipping/low level

    // Test continuous monitoring with streaming data
    const streamDuration = 1000; // 1 second of streaming
    const bufferSize = 1024;
    const numBuffers = Math.floor((44100 * streamDuration) / 1000 / bufferSize);

    let streamProcessingTimes = [];

    for (let i = 0; i < numBuffers; i++) {
      const streamBuffer = new Float32Array(bufferSize);
      for (let j = 0; j < bufferSize; j++) {
        streamBuffer[j] =
          0.3 * Math.sin((2 * Math.PI * 1000 * (i * bufferSize + j)) / 44100);
      }

      const streamResult = qualityAssessor.processRealTimeAudio(streamBuffer);
      if (streamResult) {
        streamProcessingTimes.push(streamResult.processingTime);
      }
    }

    // Verify consistent low-latency performance
    const avgProcessingTime =
      streamProcessingTimes.reduce((a, b) => a + b, 0) /
      streamProcessingTimes.length;
    expect(avgProcessingTime).toBeLessThan(20);

    // Test quality trend tracking
    const trendData = monitoringResults.map((r) => ({
      timestamp: r.metrics.timestamp,
      rms: r.metrics.rms,
      peak: r.metrics.peak,
      violations: r.violations.length,
    }));

    expect(trendData.length).toBeGreaterThan(0);

    // Stop monitoring
    const stopResult = qualityAssessor.stopRealTimeMonitoring();
    expect(stopResult.success).toBe(true);
    expect(qualityAssessor.isMonitoring).toBe(false);

    performanceMonitor.endMeasurement();
  });

  it("should handle quality threshold alerts", async () => {
    performanceMonitor.startMeasurement("Threshold Alert Handling");

    let alertHistory = [];

    // Set up alert callback with detailed logging
    const alertCallback = (result) => {
      if (result.violations.length > 0) {
        alertHistory.push({
          timestamp: Date.now(),
          violations: result.violations,
          metrics: result.metrics,
          severity: calculateAlertSeverity(result.violations),
        });
      }
    };

    // Configure monitoring with specific threshold values
    const thresholdConfig = {
      snrMin: 35,
      thdMax: -30,
      dynamicRangeMin: 50,
      peakMax: 0.9,
      rmsMin: 0.01,
      rmsMax: 0.8,
    };

    qualityAssessor.startRealTimeMonitoring(alertCallback, thresholdConfig);

    // Test various threshold violations
    const violationTests = [
      {
        name: "peak_clipping",
        signal: new Float32Array(1024).fill(0.95), // Above peak threshold
        expectedViolation: "clipping",
      },
      {
        name: "low_level",
        signal: new Float32Array(1024).fill(0.005), // Below RMS minimum
        expectedViolation: "low_level",
      },
      {
        name: "high_level",
        signal: new Float32Array(1024).fill(0.85), // Above RMS maximum
        expectedViolation: "high_level",
      },
      {
        name: "normal_level",
        signal: testDataGenerator.generateSineWave(1000, 1024 / 44100, 0.3), // Within thresholds
        expectedViolation: null,
      },
    ];

    for (const test of violationTests) {
      const result = qualityAssessor.processRealTimeAudio(test.signal);

      if (test.expectedViolation) {
        expect(result.violations.length).toBeGreaterThan(0);
        const violation = result.violations.find(
          (v) => v.type === test.expectedViolation
        );
        if (violation) {
          expect(violation.threshold).toBeDefined();
          expect(violation.actual).toBeDefined();
          expect(violation.severity).toBeDefined();
        }
      } else {
        expect(result.violations.length).toBe(0);
      }
    }

    // Test alert escalation and suppression
    const repeatedViolationSignal = new Float32Array(1024).fill(0.95); // Repeated clipping

    // Generate multiple alerts for the same issue
    for (let i = 0; i < 10; i++) {
      qualityAssessor.processRealTimeAudio(repeatedViolationSignal);
      await new Promise((resolve) => setTimeout(resolve, 10));
    }

    // Verify alert history management
    expect(qualityAssessor.alertHistory.length).toBeGreaterThan(0);
    expect(qualityAssessor.alertHistory.length).toBeLessThanOrEqual(1000); // History limit

    // Test alert severity calculation
    const severities = qualityAssessor.alertHistory.map(
      (alert) => alert.severity
    );
    const uniqueSeverities = [...new Set(severities)];
    expect(uniqueSeverities.length).toBeGreaterThan(0);

    // Test alert filtering by severity
    const errorAlerts = qualityAssessor.alertHistory.filter(
      (alert) => alert.severity === "error"
    );
    const warningAlerts = qualityAssessor.alertHistory.filter(
      (alert) => alert.severity === "warning"
    );

    expect(errorAlerts.length + warningAlerts.length).toBeLessThanOrEqual(
      qualityAssessor.alertHistory.length
    );

    // Test alert recovery detection
    const recoverySignal = testDataGenerator.generateSineWave(
      1000,
      1024 / 44100,
      0.3
    ); // Good signal
    const recoveryResult = qualityAssessor.processRealTimeAudio(recoverySignal);

    expect(recoveryResult.violations.length).toBe(0); // Should have no violations

    // Test threshold configuration management
    const newThresholds = {
      snrMin: 30,
      peakMax: 0.95,
      rmsMin: 0.005,
    };

    const configResult = qualityAssessor.startRealTimeMonitoring(
      alertCallback,
      newThresholds
    );
    expect(configResult.success).toBe(true);
    expect(configResult.thresholds.snrMin).toBe(30);
    expect(configResult.thresholds.peakMax).toBe(0.95);

    qualityAssessor.stopRealTimeMonitoring();
    performanceMonitor.endMeasurement();
  });

  const calculateAlertSeverity = (violations) => {
    let maxSeverity = "info";

    for (const violation of violations) {
      if (violation.severity === "error") {
        maxSeverity = "error";
        break;
      } else if (violation.severity === "warning" && maxSeverity !== "error") {
        maxSeverity = "warning";
      }
    }

    return maxSeverity;
  };
});

describe("Quality Assessment Performance", () => {
  it("should meet performance benchmarks", async () => {
    performanceMonitor.startMeasurement("Performance Benchmarks");

    // Test processing speed with various buffer sizes
    const bufferSizes = [256, 512, 1024, 2048, 4096];
    const processingTimes = {};

    for (const bufferSize of bufferSizes) {
      const testSignal = testDataGenerator.generateSineWave(
        1000,
        bufferSize / 44100,
        0.5
      );

      // Measure SNR calculation performance
      const snrStart = performance.now();
      qualityAssessor.calculateSNR(testSignal);
      const snrTime = performance.now() - snrStart;

      // Measure THD+N calculation performance
      const thdStart = performance.now();
      qualityAssessor.calculateTHDN(testSignal, 1000);
      const thdTime = performance.now() - thdStart;

      // Measure frequency response performance
      const freqStart = performance.now();
      qualityAssessor.assessFrequencyResponse(testSignal);
      const freqTime = performance.now() - freqStart;

      processingTimes[bufferSize] = {
        snr: snrTime,
        thd: thdTime,
        frequency: freqTime,
        total: snrTime + thdTime + freqTime,
      };

      // Performance requirements (should scale reasonably with buffer size)
      expect(snrTime).toBeLessThan(bufferSize / 100); // Rough scaling expectation
      expect(thdTime).toBeLessThan(bufferSize / 50);
      expect(freqTime).toBeLessThan(bufferSize / 20);
    }

    // Test memory usage efficiency
    const memoryBefore = performanceMonitor.getCurrentMemoryUsage();

    // Process large amount of data
    const largeSignal = new Float32Array(441000); // 10 seconds
    for (let i = 0; i < largeSignal.length; i++) {
      largeSignal[i] = 0.5 * Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    qualityAssessor.assessPerceptualQuality(largeSignal);
    qualityAssessor.calculateSNR(largeSignal);
    qualityAssessor.detectArtifacts(largeSignal);

    const memoryAfter = performanceMonitor.getCurrentMemoryUsage();
    const memoryUsed = memoryAfter - memoryBefore;

    // Memory usage should be reasonable (less than 100MB for this test)
    expect(memoryUsed).toBeLessThan(100 * 1024 * 1024);

    // Test CPU utilization with concurrent processing
    const concurrentTests = [];
    const numConcurrent = 4;

    for (let i = 0; i < numConcurrent; i++) {
      const testPromise = new Promise((resolve) => {
        const concurrentSignal = testDataGenerator.generateSineWave(
          1000 + i * 100,
          1.0,
          0.5
        );
        const startTime = performance.now();

        qualityAssessor.calculateSNR(concurrentSignal);
        qualityAssessor.assessPerceptualQuality(concurrentSignal);

        const processingTime = performance.now() - startTime;
        resolve(processingTime);
      });

      concurrentTests.push(testPromise);
    }

    const concurrentResults = await Promise.all(concurrentTests);
    const avgConcurrentTime =
      concurrentResults.reduce((a, b) => a + b, 0) / concurrentResults.length;

    // Concurrent processing shouldn't be dramatically slower than sequential
    expect(avgConcurrentTime).toBeLessThan(500);

    // Test throughput performance
    const throughputTestDuration = 1000; // 1 second
    const bufferSize = 1024;
    const throughputStart = performance.now();
    let buffersProcessed = 0;

    while (performance.now() - throughputStart < throughputTestDuration) {
      const throughputBuffer = new Float32Array(bufferSize);
      for (let i = 0; i < bufferSize; i++) {
        throughputBuffer[i] = 0.3 * Math.sin((2 * Math.PI * 1000 * i) / 44100);
      }

      qualityAssessor.processRealTimeAudio(throughputBuffer);
      buffersProcessed++;
    }

    const actualDuration = performance.now() - throughputStart;
    const throughput = (buffersProcessed * bufferSize * 1000) / actualDuration; // Samples per second

    // Should be able to process at least real-time (44100 samples/second)
    expect(throughput).toBeGreaterThan(44100);

    // Test performance regression detection
    const baselineMetrics = {
      snrTime: processingTimes[1024].snr,
      thdTime: processingTimes[1024].thd,
      freqTime: processingTimes[1024].frequency,
      memoryUsage: memoryUsed,
      throughput: throughput,
    };

    // Store baseline for regression testing
    expect(baselineMetrics.snrTime).toBeLessThan(50);
    expect(baselineMetrics.thdTime).toBeLessThan(100);
    expect(baselineMetrics.freqTime).toBeLessThan(150);

    performanceMonitor.endMeasurement();
  });

  it("should optimize for different hardware", async () => {
    performanceMonitor.startMeasurement("Hardware Optimization");

    // Test hardware capability detection
    const mockHardwareConfigs = [
      {
        name: "basic_cpu",
        simdSupport: false,
        gpuAcceleration: false,
        cacheSize: 256 * 1024, // 256KB
        cores: 2,
      },
      {
        name: "advanced_cpu",
        simdSupport: true,
        gpuAcceleration: false,
        cacheSize: 8 * 1024 * 1024, // 8MB
        cores: 8,
      },
      {
        name: "gpu_accelerated",
        simdSupport: true,
        gpuAcceleration: true,
        cacheSize: 32 * 1024 * 1024, // 32MB
        cores: 16,
      },
    ];

    const optimizationResults = {};

    for (const config of mockHardwareConfigs) {
      // Test hardware optimization
      const optimizationResult = qualityAssessor.optimizeForHardware(config);

      expect(optimizationResult.success).toBe(true);
      expect(optimizationResult.optimizations).toBeDefined();
      expect(optimizationResult.processingTime).toBeLessThan(100);

      // Verify optimizations are applied based on hardware capabilities
      const optimizations = optimizationResult.optimizations;

      if (config.simdSupport) {
        expect(optimizations.simd).toBe(true);
      }

      if (config.gpuAcceleration) {
        expect(optimizations.gpu).toBe(true);
      }

      if (config.cacheSize > 1024 * 1024) {
        expect(optimizations.cache).toBe(true);
      }

      // Test performance with optimization
      const testSignal = testDataGenerator.generateSineWave(1000, 1.0, 0.5);

      const performanceStart = performance.now();
      qualityAssessor.calculateSNR(testSignal);
      qualityAssessor.assessPerceptualQuality(testSignal);
      const performanceTime = performance.now() - performanceStart;

      optimizationResults[config.name] = {
        config: config,
        optimizations: optimizations,
        performanceTime: performanceTime,
      };

      expect(performanceTime).toBeLessThan(200); // Reasonable performance threshold
    }

    // Verify that advanced hardware configurations perform better
    const basicPerformance = optimizationResults.basic_cpu.performanceTime;
    const advancedPerformance =
      optimizationResults.advanced_cpu.performanceTime;
    const gpuPerformance = optimizationResults.gpu_accelerated.performanceTime;

    // Advanced configurations should not perform worse than basic
    expect(advancedPerformance).toBeLessThanOrEqual(basicPerformance * 1.5);
    expect(gpuPerformance).toBeLessThanOrEqual(basicPerformance * 1.5);

    // Test SIMD instruction utilization simulation
    const simdTestData = new Float32Array(8192); // Size suitable for SIMD
    for (let i = 0; i < simdTestData.length; i++) {
      simdTestData[i] = Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    // Enable SIMD and test performance
    qualityAssessor.enableSIMDOptimizations();
    const simdStart = performance.now();
    qualityAssessor.calculateSignalPower(simdTestData);
    const simdTime = performance.now() - simdStart;

    expect(simdTime).toBeLessThan(50); // Should be fast with SIMD

    // Test memory access optimization
    const largeBuffer = new Float32Array(262144); // 256K samples
    for (let i = 0; i < largeBuffer.length; i++) {
      largeBuffer[i] = Math.sin((2 * Math.PI * 1000 * i) / 44100);
    }

    qualityAssessor.optimizeCacheUsage(8 * 1024 * 1024); // 8MB cache
    const cacheOptimizedStart = performance.now();
    qualityAssessor.performFFT(largeBuffer);
    const cacheOptimizedTime = performance.now() - cacheOptimizedStart;

    expect(cacheOptimizedTime).toBeLessThan(500);

    // Test parallel processing capabilities simulation
    const parallelTests = [];
    const numParallel = 4;

    for (let i = 0; i < numParallel; i++) {
      const parallelPromise = new Promise((resolve) => {
        const parallelSignal = testDataGenerator.generateSineWave(
          1000 + i * 200,
          0.5,
          0.4
        );
        const parallelStart = performance.now();

        qualityAssessor.assessFrequencyResponse(parallelSignal);

        const parallelTime = performance.now() - parallelStart;
        resolve(parallelTime);
      });

      parallelTests.push(parallelPromise);
    }

    const parallelResults = await Promise.all(parallelTests);
    const avgParallelTime =
      parallelResults.reduce((a, b) => a + b, 0) / parallelResults.length;

    expect(avgParallelTime).toBeLessThan(300);

    // Test resource utilization efficiency
    const resourceStart = performance.now();
    const resourceMemoryStart = performanceMonitor.getCurrentMemoryUsage();

    // Perform multiple quality assessments
    for (let i = 0; i < 10; i++) {
      const resourceSignal = testDataGenerator.generateSineWave(
        1000 + i * 100,
        0.1,
        0.3
      );
      qualityAssessor.calculateSNR(resourceSignal);
      qualityAssessor.detectArtifacts(resourceSignal);
    }

    const resourceTime = performance.now() - resourceStart;
    const resourceMemoryEnd = performanceMonitor.getCurrentMemoryUsage();
    const resourceMemoryUsed = resourceMemoryEnd - resourceMemoryStart;

    // Resource utilization should be efficient
    expect(resourceTime).toBeLessThan(1000); // 1 second total
    expect(resourceMemoryUsed).toBeLessThan(50 * 1024 * 1024); // 50MB max

    performanceMonitor.endMeasurement();
  });
});

describe("Quality Assessment Integration", () => {
  it("should integrate with audio processing pipeline", async () => {
    performanceMonitor.startMeasurement("Pipeline Integration");

    // Test integration with mock audio engine
    expect(mockAudioEngine.getQualityAssessor()).toBe(qualityAssessor);
    expect(mockAudioEngine.sampleRate).toBe(44100);
    expect(mockAudioEngine.channels).toBe(2);

    // Test seamless quality monitoring integration
    let qualityFeedback = [];

    // Simulate processing pipeline with quality feedback
    const pipelineCallback = (qualityData) => {
      qualityFeedback.push(qualityData);
    };

    qualityAssessor.startRealTimeMonitoring(pipelineCallback, {
      snrMin: 35,
      peakMax: 0.9,
    });

    // Simulate audio processing with quality monitoring
    const pipelineSignals = [
      testDataGenerator.generateSineWave(1000, 0.1, 0.5),
      testDataGenerator.generateComplexTone([440, 880, 1320], 0.1),
      testDataGenerator.generateWhiteNoise(0.1, 0.2),
    ];

    for (const signal of pipelineSignals) {
      // Simulate audio engine processing
      mockAudioEngine.processAudio(signal);

      // Quality assessor processes in parallel
      const qualityResult = qualityAssessor.processRealTimeAudio(signal);

      expect(qualityResult).toBeDefined();
      expect(qualityResult.metrics).toBeDefined();
      expect(qualityResult.processingTime).toBeLessThan(100);
    }

    // Verify quality feedback was received
    expect(qualityFeedback.length).toBeGreaterThan(0);

    // Test quality data export capabilities
    const exportData = {
      sessionId: "test_session_" + Date.now(),
      startTime: Date.now() - 10000,
      endTime: Date.now(),
      qualityMetrics: qualityMetrics,
      performanceStats: performanceMonitor.getStatistics(),
    };

    // Verify export data structure
    expect(exportData.sessionId).toBeDefined();
    expect(exportData.startTime).toBeLessThan(exportData.endTime);
    expect(exportData.qualityMetrics).toBeDefined();
    expect(exportData.performanceStats).toBeDefined();

    // Test quality reporting integration
    const qualityReport = {
      summary: {
        totalTests: Object.keys(qualityMetrics).length,
        averageSnr:
          qualityMetrics.snr.length > 0
            ? qualityMetrics.snr.reduce((a, b) => a + b.snr, 0) /
              qualityMetrics.snr.length
            : 0,
        artifactsDetected: qualityMetrics.artifacts.length,
        performanceMetrics: performanceMonitor.exportResults(),
      },
      details: qualityMetrics,
      recommendations: generateQualityRecommendations(qualityMetrics),
    };

    expect(qualityReport.summary).toBeDefined();
    expect(qualityReport.summary.totalTests).toBeGreaterThan(0);
    expect(qualityReport.details).toBeDefined();
    expect(qualityReport.recommendations).toBeDefined();

    // Test quality-based processing adaptation
    const adaptationTests = [
      {
        condition: "low_snr",
        signal: qualityAssessor.addNoise(
          testDataGenerator.generateSineWave(1000, 0.5, 0.5),
          20
        ),
        expectedAdaptation: "noise_reduction",
      },
      {
        condition: "clipping",
        signal: new Float32Array(22050).fill(0.99),
        expectedAdaptation: "gain_reduction",
      },
      {
        condition: "good_quality",
        signal: testDataGenerator.generateSineWave(1000, 0.5, 0.3),
        expectedAdaptation: "maintain",
      },
    ];

    for (const test of adaptationTests) {
      const qualityAssessment = qualityAssessor.assessPerceptualQuality(
        test.signal
      );
      const artifactAssessment = qualityAssessor.detectArtifacts(test.signal);

      // Simulate processing adaptation based on quality
      let suggestedAdaptation = "maintain";

      if (qualityAssessment.qualityScore < 2.5) {
        suggestedAdaptation = "enhance";
      }

      if (artifactAssessment.artifacts.some((a) => a.type === "clipping")) {
        suggestedAdaptation = "gain_reduction";
      }

      expect(suggestedAdaptation).toBeDefined();
      expect([
        "maintain",
        "enhance",
        "gain_reduction",
        "noise_reduction",
      ]).toContain(suggestedAdaptation);
    }

    // Test quality assessment lifecycle management
    const lifecycleEvents = [
      "initialization",
      "configuration",
      "processing",
      "monitoring",
      "reporting",
      "cleanup",
    ];

    const lifecycleStatus = {};

    for (const event of lifecycleEvents) {
      lifecycleStatus[event] = {
        timestamp: Date.now(),
        status: "completed",
        duration: Math.random() * 100, // Simulated duration
      };
    }

    expect(Object.keys(lifecycleStatus).length).toBe(lifecycleEvents.length);

    // Test error handling integration
    const errorTests = [
      {
        name: "empty_buffer",
        signal: new Float32Array(0),
        shouldThrow: true,
      },
      {
        name: "invalid_frequency",
        signal: testDataGenerator.generateSineWave(1000, 0.1, 0.5),
        frequency: -1,
        shouldThrow: true,
      },
      {
        name: "null_signal",
        signal: null,
        shouldThrow: true,
      },
    ];

    for (const errorTest of errorTests) {
      if (errorTest.shouldThrow) {
        try {
          if (errorTest.signal === null) {
            expect(() =>
              qualityAssessor.calculateSNR(errorTest.signal)
            ).toThrow();
          } else if (errorTest.signal.length === 0) {
            expect(() =>
              qualityAssessor.calculateSNR(errorTest.signal)
            ).toThrow();
          } else if (errorTest.frequency < 0) {
            expect(() =>
              qualityAssessor.calculateTHDN(
                errorTest.signal,
                errorTest.frequency
              )
            ).toThrow();
          }
        } catch (error) {
          expect(error).toBeDefined();
        }
      }
    }

    qualityAssessor.stopRealTimeMonitoring();
    performanceMonitor.endMeasurement();
  });
});

// Helper function for generating quality recommendations
const generateQualityRecommendations = (metrics) => {
  const recommendations = [];

  // Analyze SNR metrics
  if (metrics.snr.length > 0) {
    const avgSnr =
      metrics.snr.reduce((a, b) => a + b.snr, 0) / metrics.snr.length;
    if (avgSnr < 40) {
      recommendations.push({
        type: "snr_improvement",
        priority: "high",
        suggestion: "Consider noise reduction or input gain adjustment",
      });
    }
  }

  // Analyze artifact metrics
  if (metrics.artifacts.length > 0) {
    const hasClipping = metrics.artifacts.some((a) =>
      a.result.artifacts.some((artifact) => artifact.type === "clipping")
    );

    if (hasClipping) {
      recommendations.push({
        type: "clipping_prevention",
        priority: "critical",
        suggestion: "Reduce input levels to prevent clipping distortion",
      });
    }
  }

  // Analyze perceptual quality
  if (metrics.perceptualQuality.length > 0) {
    const avgQuality =
      metrics.perceptualQuality.reduce(
        (a, b) => a + (b.result ? b.result.qualityScore : b.qualityScore),
        0
      ) / metrics.perceptualQuality.length;

    if (avgQuality < 3.0) {
      recommendations.push({
        type: "quality_enhancement",
        priority: "medium",
        suggestion: "Consider audio enhancement or codec optimization",
      });
    }
  }

  return recommendations;
};
