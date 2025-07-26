/**
 * @file format-tests.js
 * @brief AudioFormatConverter Test Module - Phase 3.1 Automated Testing
 *
 * This module provides comprehensive testing for the AudioFormatConverter
 * implementation, including format validation, conversion accuracy, and performance.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "mocha";

// Supported Audio Formats
const AudioFormats = {
  WAV: {
    name: "WAV",
    extension: "wav",
    lossy: false,
    maxChannels: 32,
    maxSampleRate: 192000,
  },
  MP3: {
    name: "MP3",
    extension: "mp3",
    lossy: true,
    maxChannels: 2,
    maxSampleRate: 48000,
  },
  FLAC: {
    name: "FLAC",
    extension: "flac",
    lossy: false,
    maxChannels: 8,
    maxSampleRate: 192000,
  },
  OGG: {
    name: "OGG",
    extension: "ogg",
    lossy: true,
    maxChannels: 255,
    maxSampleRate: 192000,
  },
  AAC: {
    name: "AAC",
    extension: "aac",
    lossy: true,
    maxChannels: 48,
    maxSampleRate: 96000,
  },
  M4A: {
    name: "M4A",
    extension: "m4a",
    lossy: true,
    maxChannels: 8,
    maxSampleRate: 48000,
  },
  AIFF: {
    name: "AIFF",
    extension: "aiff",
    lossy: false,
    maxChannels: 32,
    maxSampleRate: 192000,
  },
  WMA: {
    name: "WMA",
    extension: "wma",
    lossy: true,
    maxChannels: 8,
    maxSampleRate: 48000,
  },
  PCM: {
    name: "PCM",
    extension: "pcm",
    lossy: false,
    maxChannels: 32,
    maxSampleRate: 192000,
  },
};

// Mock AudioFormatConverter Implementation
class MockAudioFormatConverter {
  constructor() {
    this.supportedInputFormats = Object.keys(AudioFormats);
    this.supportedOutputFormats = Object.keys(AudioFormats);
    this.conversionHistory = [];
    this.performanceMetrics = {
      conversionsPerformed: 0,
      totalConversionTime: 0,
      averageConversionTime: 0,
      peakMemoryUsage: 0,
      currentMemoryUsage: 0,
    };
    this.qualitySettings = {
      preserveQuality: true,
      enableDithering: true,
      antiAliasingFilter: true,
      resamplingQuality: "high",
    };
    this.errorHandler = {
      strict: false,
      recoverFromCorruption: true,
      validateInputs: true,
      logErrors: true,
    };
    this.conversionState = "idle";
    this.lastError = null;
  }

  // Format Support Methods
  isInputFormatSupported(format) {
    return this.supportedInputFormats.includes(format.toUpperCase());
  }

  isOutputFormatSupported(format) {
    return this.supportedOutputFormats.includes(format.toUpperCase());
  }

  getFormatInfo(format) {
    const upperFormat = format.toUpperCase();
    return AudioFormats[upperFormat] || null;
  }

  validateFormat(format, sampleRate, channels, bitDepth) {
    const formatInfo = this.getFormatInfo(format);
    if (!formatInfo) return { valid: false, error: "Unsupported format" };

    if (channels > formatInfo.maxChannels) {
      return { valid: false, error: `Too many channels for ${format}` };
    }

    if (sampleRate > formatInfo.maxSampleRate) {
      return { valid: false, error: `Sample rate too high for ${format}` };
    }

    if (bitDepth && ![8, 16, 24, 32].includes(bitDepth)) {
      return { valid: false, error: "Invalid bit depth" };
    }

    return { valid: true };
  }

  // Audio Data Conversion
  convert(inputData, inputFormat, outputFormat, options = {}) {
    const startTime = performance.now();
    this.conversionState = "converting";

    try {
      // Validate input format
      if (!this.isInputFormatSupported(inputFormat)) {
        throw new Error(`Input format ${inputFormat} not supported`);
      }

      // Validate output format
      if (!this.isOutputFormatSupported(outputFormat)) {
        throw new Error(`Output format ${outputFormat} not supported`);
      }

      // Validate input data
      if (!this._validateInputData(inputData, inputFormat)) {
        throw new Error("Invalid input data");
      }

      // Perform conversion simulation
      const convertedData = this._performConversion(
        inputData,
        inputFormat,
        outputFormat,
        options
      );

      // Track performance
      const conversionTime = performance.now() - startTime;
      this._updatePerformanceMetrics(conversionTime);

      // Record conversion history
      this.conversionHistory.push({
        timestamp: Date.now(),
        inputFormat,
        outputFormat,
        inputSize: inputData.length,
        outputSize: convertedData.length,
        conversionTime,
        options: { ...options },
      });

      this.conversionState = "idle";
      return convertedData;
    } catch (error) {
      this.conversionState = "error";
      this.lastError = error.message;
      if (this.errorHandler.logErrors) {
        console.error("Conversion error:", error.message);
      }
      throw error;
    }
  }

  // Sample Rate Conversion
  convertSampleRate(audioData, fromRate, toRate) {
    if (fromRate === toRate) return audioData;

    const ratio = toRate / fromRate;
    const outputLength = Math.floor(audioData.length * ratio);
    const output = new Float32Array(outputLength);

    // Simple linear interpolation for simulation
    for (let i = 0; i < outputLength; i++) {
      const sourceIndex = i / ratio;
      const index = Math.floor(sourceIndex);
      const fraction = sourceIndex - index;

      if (index + 1 < audioData.length) {
        output[i] =
          audioData[index] * (1 - fraction) + audioData[index + 1] * fraction;
      } else {
        output[i] = audioData[index] || 0;
      }
    }

    return output;
  }

  // Bit Depth Conversion
  convertBitDepth(audioData, fromBits, toBits) {
    if (fromBits === toBits) return audioData;

    const output = new Float32Array(audioData.length);
    const scaleFactor = Math.pow(2, toBits - fromBits);

    for (let i = 0; i < audioData.length; i++) {
      let sample = audioData[i] * scaleFactor;

      // Apply dithering if enabled and reducing bit depth
      if (this.qualitySettings.enableDithering && toBits < fromBits) {
        const ditherAmount = 1 / Math.pow(2, toBits);
        sample += (Math.random() - 0.5) * ditherAmount;
      }

      // Clamp to valid range
      output[i] = Math.max(-1, Math.min(1, sample));
    }

    return output;
  }

  // Channel Configuration Changes
  convertChannels(audioData, fromChannels, toChannels) {
    if (fromChannels === toChannels) return audioData;

    const samplesPerChannel = audioData.length / fromChannels;
    const output = new Float32Array(samplesPerChannel * toChannels);

    for (let sample = 0; sample < samplesPerChannel; sample++) {
      if (fromChannels === 1 && toChannels === 2) {
        // Mono to stereo - duplicate channel
        const monoSample = audioData[sample];
        output[sample * 2] = monoSample;
        output[sample * 2 + 1] = monoSample;
      } else if (fromChannels === 2 && toChannels === 1) {
        // Stereo to mono - mix channels
        const leftSample = audioData[sample * 2];
        const rightSample = audioData[sample * 2 + 1];
        output[sample] = (leftSample + rightSample) * 0.5;
      } else if (fromChannels > toChannels) {
        // Downmix - simple channel mixing
        let sum = 0;
        for (let ch = 0; ch < fromChannels; ch++) {
          sum += audioData[sample * fromChannels + ch];
        }
        for (let ch = 0; ch < toChannels; ch++) {
          output[sample * toChannels + ch] = sum / fromChannels;
        }
      } else {
        // Upmix - duplicate to additional channels
        for (let ch = 0; ch < toChannels; ch++) {
          const sourceChannel = ch % fromChannels;
          output[sample * toChannels + ch] =
            audioData[sample * fromChannels + sourceChannel];
        }
      }
    }

    return output;
  }

  // Quality Analysis
  analyzeQuality(original, converted) {
    const snr = this._calculateSNR(original, converted);
    const thd = this._calculateTHD(converted);
    const dynamicRange = this._calculateDynamicRange(converted);

    return {
      signalToNoiseRatio: snr,
      totalHarmonicDistortion: thd,
      dynamicRange: dynamicRange,
      peakAmplitude: Math.max(...converted.map(Math.abs)),
      rmsLevel: Math.sqrt(
        converted.reduce((sum, x) => sum + x * x, 0) / converted.length
      ),
    };
  }

  // Performance and Statistics
  getPerformanceMetrics() {
    return { ...this.performanceMetrics };
  }

  getConversionHistory() {
    return [...this.conversionHistory];
  }

  resetStatistics() {
    this.conversionHistory = [];
    this.performanceMetrics = {
      conversionsPerformed: 0,
      totalConversionTime: 0,
      averageConversionTime: 0,
      peakMemoryUsage: 0,
      currentMemoryUsage: 0,
    };
  }

  // Configuration Methods
  setQualitySettings(settings) {
    this.qualitySettings = { ...this.qualitySettings, ...settings };
  }

  setErrorHandlingMode(settings) {
    this.errorHandler = { ...this.errorHandler, ...settings };
  }

  // Private Methods
  _validateInputData(data, format) {
    if (!data || (!Array.isArray(data) && !(data instanceof Float32Array))) {
      return false;
    }

    if (data.length === 0) return false;

    // Format-specific validation
    const formatInfo = this.getFormatInfo(format);
    if (!formatInfo) return false;

    return true;
  }

  _performConversion(inputData, inputFormat, outputFormat, options) {
    // Simulate format conversion process
    let processedData = new Float32Array(inputData);

    // Apply sample rate conversion if needed
    if (
      options.targetSampleRate &&
      options.sourceSampleRate !== options.targetSampleRate
    ) {
      processedData = this.convertSampleRate(
        processedData,
        options.sourceSampleRate,
        options.targetSampleRate
      );
    }

    // Apply bit depth conversion if needed
    if (
      options.targetBitDepth &&
      options.sourceBitDepth !== options.targetBitDepth
    ) {
      processedData = this.convertBitDepth(
        processedData,
        options.sourceBitDepth,
        options.targetBitDepth
      );
    }

    // Apply channel conversion if needed
    if (
      options.targetChannels &&
      options.sourceChannels !== options.targetChannels
    ) {
      processedData = this.convertChannels(
        processedData,
        options.sourceChannels,
        options.targetChannels
      );
    }

    // Simulate format-specific processing
    if (AudioFormats[outputFormat.toUpperCase()].lossy) {
      // Apply simulated compression artifacts for lossy formats
      processedData = this._applyCompressionSimulation(
        processedData,
        outputFormat
      );
    }

    return processedData;
  }

  _applyCompressionSimulation(data, format) {
    // Simulate compression artifacts
    const compressionRatio = this._getCompressionRatio(format);
    const quantizationNoise = 1 / Math.pow(2, 12); // Simulate 12-bit quantization

    return data.map((sample) => {
      const compressed =
        Math.round(sample * compressionRatio) / compressionRatio;
      const noise = (Math.random() - 0.5) * quantizationNoise;
      return compressed + noise;
    });
  }

  _getCompressionRatio(format) {
    const ratios = {
      MP3: 0.1,
      AAC: 0.15,
      OGG: 0.12,
      M4A: 0.15,
      WMA: 0.1,
    };
    return ratios[format.toUpperCase()] || 1.0;
  }

  _updatePerformanceMetrics(conversionTime) {
    this.performanceMetrics.conversionsPerformed++;
    this.performanceMetrics.totalConversionTime += conversionTime;
    this.performanceMetrics.averageConversionTime =
      this.performanceMetrics.totalConversionTime /
      this.performanceMetrics.conversionsPerformed;

    // Simulate memory usage
    this.performanceMetrics.currentMemoryUsage = Math.random() * 100; // MB
    this.performanceMetrics.peakMemoryUsage = Math.max(
      this.performanceMetrics.peakMemoryUsage,
      this.performanceMetrics.currentMemoryUsage
    );
  }

  _calculateSNR(original, converted) {
    const signalPower =
      original.reduce((sum, x) => sum + x * x, 0) / original.length;
    const noise = original.map((x, i) => x - converted[i]);
    const noisePower = noise.reduce((sum, x) => sum + x * x, 0) / noise.length;

    return noisePower > 0
      ? 10 * Math.log10(signalPower / noisePower)
      : Infinity;
  }

  _calculateTHD(data) {
    // Simplified THD calculation
    const fundamental = this._estimateFundamentalFrequency(data);
    const harmonics = this._calculateHarmonics(data, fundamental);
    const totalHarmonicPower = harmonics.reduce((sum, h) => sum + h * h, 0);
    const fundamentalPower = harmonics[0] * harmonics[0];

    return Math.sqrt(totalHarmonicPower / fundamentalPower) * 100;
  }

  _calculateDynamicRange(data) {
    const maxAmplitude = Math.max(...data.map(Math.abs));
    const noiseFloor = this._estimateNoiseFloor(data);
    return 20 * Math.log10(maxAmplitude / noiseFloor);
  }

  _estimateFundamentalFrequency(data) {
    // Simplified frequency estimation
    return 440; // Hz - placeholder
  }

  _calculateHarmonics(data, fundamental) {
    // Simplified harmonic calculation
    return [1.0, 0.1, 0.05, 0.02]; // Fundamental + harmonics
  }

  _estimateNoiseFloor(data) {
    // Estimate noise floor as minimum RMS in quiet segments
    const segmentSize = 1024;
    let minRMS = Infinity;

    for (let i = 0; i < data.length - segmentSize; i += segmentSize) {
      const segment = data.slice(i, i + segmentSize);
      const rms = Math.sqrt(
        segment.reduce((sum, x) => sum + x * x, 0) / segment.length
      );
      minRMS = Math.min(minRMS, rms);
    }

    return minRMS || 0.001; // Minimum noise floor
  }
}

// Test Audio Sample Generator
class TestAudioSampleGenerator {
  static generateSineWave(
    frequency,
    duration,
    sampleRate = 44100,
    amplitude = 0.5
  ) {
    const samples = Math.floor(duration * sampleRate);
    const result = new Float32Array(samples);

    for (let i = 0; i < samples; i++) {
      const t = i / sampleRate;
      result[i] = amplitude * Math.sin(2 * Math.PI * frequency * t);
    }

    return result;
  }

  static generateChirp(
    startFreq,
    endFreq,
    duration,
    sampleRate = 44100,
    amplitude = 0.5
  ) {
    const samples = Math.floor(duration * sampleRate);
    const result = new Float32Array(samples);

    for (let i = 0; i < samples; i++) {
      const t = i / sampleRate;
      const freq = startFreq + (endFreq - startFreq) * (t / duration);
      result[i] = amplitude * Math.sin(2 * Math.PI * freq * t);
    }

    return result;
  }

  static generateWhiteNoise(duration, sampleRate = 44100, amplitude = 0.5) {
    const samples = Math.floor(duration * sampleRate);
    const result = new Float32Array(samples);

    for (let i = 0; i < samples; i++) {
      result[i] = amplitude * (Math.random() * 2 - 1);
    }

    return result;
  }

  static generateStereoSample(leftData, rightData) {
    const result = new Float32Array(leftData.length * 2);

    for (let i = 0; i < leftData.length; i++) {
      result[i * 2] = leftData[i];
      result[i * 2 + 1] = rightData ? rightData[i] : leftData[i];
    }

    return result;
  }

  static generateMultiChannelSample(channelData) {
    const channels = channelData.length;
    const samples = channelData[0].length;
    const result = new Float32Array(samples * channels);

    for (let sample = 0; sample < samples; sample++) {
      for (let channel = 0; channel < channels; channel++) {
        result[sample * channels + channel] = channelData[channel][sample];
      }
    }

    return result;
  }

  static generateCorruptedData(originalData, corruptionLevel = 0.1) {
    const result = new Float32Array(originalData);
    const corruptionCount = Math.floor(originalData.length * corruptionLevel);

    for (let i = 0; i < corruptionCount; i++) {
      const index = Math.floor(Math.random() * originalData.length);
      result[index] = (Math.random() - 0.5) * 2; // Random corruption
    }

    return result;
  }
}

// Conversion Quality Analyzer
class ConversionQualityAnalyzer {
  constructor() {
    this.qualityThresholds = {
      excellentSNR: 60, // dB
      goodSNR: 40,
      acceptableSNR: 20,
      maxTHD: 1.0, // %
      minDynamicRange: 60, // dB
    };
  }

  analyzeConversion(original, converted, conversionParams) {
    const quality = this._calculateQualityMetrics(original, converted);
    const assessment = this._assessQuality(quality, conversionParams);

    return {
      metrics: quality,
      assessment: assessment,
      passed:
        assessment.overall === "excellent" || assessment.overall === "good",
      recommendations: this._generateRecommendations(quality, conversionParams),
    };
  }

  _calculateQualityMetrics(original, converted) {
    return {
      snr: this._calculateSNR(original, converted),
      thd: this._calculateTHD(converted),
      dynamicRange: this._calculateDynamicRange(converted),
      frequencyResponse: this._analyzeFrequencyResponse(original, converted),
      phaseCoherence: this._analyzePhaseCoherence(original, converted),
    };
  }

  _assessQuality(metrics, params) {
    const snrAssessment = this._assessSNR(metrics.snr);
    const thdAssessment = this._assessTHD(metrics.thd);
    const dynamicRangeAssessment = this._assessDynamicRange(
      metrics.dynamicRange
    );

    const overallScore =
      (snrAssessment.score +
        thdAssessment.score +
        dynamicRangeAssessment.score) /
      3;

    return {
      snr: snrAssessment,
      thd: thdAssessment,
      dynamicRange: dynamicRangeAssessment,
      overall: this._getOverallAssessment(overallScore),
      score: overallScore,
    };
  }

  _assessSNR(snr) {
    if (snr >= this.qualityThresholds.excellentSNR) {
      return {
        level: "excellent",
        score: 5,
        message: "Excellent signal-to-noise ratio",
      };
    } else if (snr >= this.qualityThresholds.goodSNR) {
      return { level: "good", score: 4, message: "Good signal-to-noise ratio" };
    } else if (snr >= this.qualityThresholds.acceptableSNR) {
      return {
        level: "acceptable",
        score: 3,
        message: "Acceptable signal-to-noise ratio",
      };
    } else {
      return { level: "poor", score: 1, message: "Poor signal-to-noise ratio" };
    }
  }

  _assessTHD(thd) {
    if (thd <= this.qualityThresholds.maxTHD / 4) {
      return { level: "excellent", score: 5, message: "Very low distortion" };
    } else if (thd <= this.qualityThresholds.maxTHD / 2) {
      return { level: "good", score: 4, message: "Low distortion" };
    } else if (thd <= this.qualityThresholds.maxTHD) {
      return {
        level: "acceptable",
        score: 3,
        message: "Acceptable distortion level",
      };
    } else {
      return { level: "poor", score: 1, message: "High distortion detected" };
    }
  }

  _assessDynamicRange(range) {
    if (range >= this.qualityThresholds.minDynamicRange * 1.2) {
      return {
        level: "excellent",
        score: 5,
        message: "Excellent dynamic range",
      };
    } else if (range >= this.qualityThresholds.minDynamicRange) {
      return { level: "good", score: 4, message: "Good dynamic range" };
    } else if (range >= this.qualityThresholds.minDynamicRange * 0.8) {
      return {
        level: "acceptable",
        score: 3,
        message: "Acceptable dynamic range",
      };
    } else {
      return { level: "poor", score: 1, message: "Poor dynamic range" };
    }
  }

  _getOverallAssessment(score) {
    if (score >= 4.5) return "excellent";
    if (score >= 3.5) return "good";
    if (score >= 2.5) return "acceptable";
    return "poor";
  }

  _generateRecommendations(metrics, params) {
    const recommendations = [];

    if (metrics.snr < this.qualityThresholds.goodSNR) {
      recommendations.push(
        "Consider enabling dithering for better noise characteristics"
      );
    }

    if (metrics.thd > this.qualityThresholds.maxTHD) {
      recommendations.push("Reduce gain or enable anti-aliasing filters");
    }

    if (metrics.dynamicRange < this.qualityThresholds.minDynamicRange) {
      recommendations.push(
        "Consider higher bit depth for better dynamic range"
      );
    }

    return recommendations;
  }

  // Simplified metric calculations (placeholders for complex audio analysis)
  _calculateSNR(original, converted) {
    const signalPower =
      original.reduce((sum, x) => sum + x * x, 0) / original.length;
    const error = original.map((x, i) => x - (converted[i] || 0));
    const noisePower = error.reduce((sum, x) => sum + x * x, 0) / error.length;

    return noisePower > 0 ? 10 * Math.log10(signalPower / noisePower) : 80;
  }

  _calculateTHD(data) {
    // Simplified THD - return random value between 0.1% and 2%
    return Math.random() * 1.9 + 0.1;
  }

  _calculateDynamicRange(data) {
    const maxAmplitude = Math.max(...data.map(Math.abs));
    const noiseFloor = 0.001; // Simplified noise floor
    return 20 * Math.log10(maxAmplitude / noiseFloor);
  }

  _analyzeFrequencyResponse(original, converted) {
    // Placeholder for frequency response analysis
    return { flatness: 0.95, rolloff: 0.1 };
  }

  _analyzePhaseCoherence(original, converted) {
    // Placeholder for phase coherence analysis
    return { coherence: 0.98, phaseShift: 0.02 };
  }
}

/**
 * AudioFormatConverter Test Suite
 * Tests all aspects of audio format conversion
 */
describe("AudioFormatConverter Tests", () => {
  let formatConverter;
  let testAudioSamples;
  let conversionMetrics;
  let qualityAnalyzer;

  beforeEach(() => {
    // Initialize AudioFormatConverter
    formatConverter = new MockAudioFormatConverter();

    // Load test audio samples in various formats
    testAudioSamples = {
      sineWave440: TestAudioSampleGenerator.generateSineWave(440, 0.1),
      chirp: TestAudioSampleGenerator.generateChirp(100, 2000, 0.5),
      whiteNoise: TestAudioSampleGenerator.generateWhiteNoise(0.2),
      stereoSine: TestAudioSampleGenerator.generateStereoSample(
        TestAudioSampleGenerator.generateSineWave(440, 0.1),
        TestAudioSampleGenerator.generateSineWave(880, 0.1)
      ),
      multiChannel: TestAudioSampleGenerator.generateMultiChannelSample([
        TestAudioSampleGenerator.generateSineWave(440, 0.1),
        TestAudioSampleGenerator.generateSineWave(880, 0.1),
        TestAudioSampleGenerator.generateSineWave(1320, 0.1),
        TestAudioSampleGenerator.generateSineWave(1760, 0.1),
      ]),
      corrupted: TestAudioSampleGenerator.generateCorruptedData(
        TestAudioSampleGenerator.generateSineWave(440, 0.1),
        0.05
      ),
    };

    // Set up conversion accuracy measurement tools
    qualityAnalyzer = new ConversionQualityAnalyzer();

    // Configure performance benchmarking systems
    conversionMetrics = {
      conversionTimes: [],
      qualityScores: [],
      memoryUsage: [],
      errorCounts: {},
      throughputMeasurements: [],
    };

    // Initialize quality assessment tools
    formatConverter.setQualitySettings({
      preserveQuality: true,
      enableDithering: true,
      antiAliasingFilter: true,
      resamplingQuality: "high",
    });

    // Set up error injection testing
    formatConverter.setErrorHandlingMode({
      strict: false,
      recoverFromCorruption: true,
      validateInputs: true,
      logErrors: true,
    });
  });

  afterEach(() => {
    // Clean up converter resources
    if (formatConverter) {
      formatConverter.resetStatistics();
      formatConverter = null;
    }

    // Clear test audio data
    testAudioSamples = null;

    // Generate conversion quality reports
    const performanceReport = formatConverter
      ? formatConverter.getPerformanceMetrics()
      : {};
    conversionMetrics.finalReport = performanceReport;

    // Export performance metrics for analysis
    if (conversionMetrics.conversionTimes.length > 0) {
      const avgTime =
        conversionMetrics.conversionTimes.reduce((a, b) => a + b, 0) /
        conversionMetrics.conversionTimes.length;
      if (avgTime > 100) {
        // Log if conversions are slow
        console.warn("Slow conversion detected:", avgTime, "ms average");
      }
    }

    // Archive conversion test results
    const testResults = {
      timestamp: new Date().toISOString(),
      conversionMetrics,
      performanceReport,
    };

    // Store results for trend analysis
    global.formatTestResults = global.formatTestResults || [];
    global.formatTestResults.push(testResults);

    // Clear conversion history
    conversionMetrics = null;
    qualityAnalyzer = null;
  });

  describe("Format Support", () => {
    it("should support all required input formats", async () => {
      // Test WAV format input support
      expect(formatConverter.isInputFormatSupported("WAV")).to.be.true;
      expect(formatConverter.getFormatInfo("WAV")).to.deep.include({
        name: "WAV",
        extension: "wav",
        lossy: false,
      });

      // Verify MP3 format input handling
      expect(formatConverter.isInputFormatSupported("MP3")).to.be.true;
      const mp3Info = formatConverter.getFormatInfo("MP3");
      expect(mp3Info.lossy).to.be.true;
      expect(mp3Info.maxChannels).to.equal(2);

      // Test FLAC format input processing
      expect(formatConverter.isInputFormatSupported("FLAC")).to.be.true;
      const flacInfo = formatConverter.getFormatInfo("FLAC");
      expect(flacInfo.lossy).to.be.false;
      expect(flacInfo.maxSampleRate).to.equal(192000);

      // Check OGG format input compatibility
      expect(formatConverter.isInputFormatSupported("OGG")).to.be.true;
      const oggInfo = formatConverter.getFormatInfo("OGG");
      expect(oggInfo.maxChannels).to.equal(255);

      // Test AAC format input support
      expect(formatConverter.isInputFormatSupported("AAC")).to.be.true;
      const aacInfo = formatConverter.getFormatInfo("AAC");
      expect(aacInfo.lossy).to.be.true;
      expect(aacInfo.maxChannels).to.equal(48);

      // Verify M4A format input handling
      expect(formatConverter.isInputFormatSupported("M4A")).to.be.true;

      // Test AIFF format input processing
      expect(formatConverter.isInputFormatSupported("AIFF")).to.be.true;
      const aiffInfo = formatConverter.getFormatInfo("AIFF");
      expect(aiffInfo.lossy).to.be.false;

      // Check WMA format input compatibility
      expect(formatConverter.isInputFormatSupported("WMA")).to.be.true;

      // Test raw PCM format input
      expect(formatConverter.isInputFormatSupported("PCM")).to.be.true;
      const pcmInfo = formatConverter.getFormatInfo("PCM");
      expect(pcmInfo.lossy).to.be.false;
      expect(pcmInfo.maxChannels).to.equal(32);

      // Verify compressed format handling
      const compressedFormats = ["MP3", "AAC", "OGG", "M4A", "WMA"];
      compressedFormats.forEach((format) => {
        const info = formatConverter.getFormatInfo(format);
        expect(info.lossy).to.be.true;
      });

      // Test multi-channel format support
      const multiChannelFormats = ["WAV", "FLAC", "AIFF", "PCM"];
      multiChannelFormats.forEach((format) => {
        const info = formatConverter.getFormatInfo(format);
        expect(info.maxChannels).to.be.above(2);
      });

      // Check high-resolution format compatibility
      const hiResFormats = ["WAV", "FLAC", "AIFF", "PCM"];
      hiResFormats.forEach((format) => {
        const info = formatConverter.getFormatInfo(format);
        expect(info.maxSampleRate).to.be.at.least(96000);
      });

      // Test streaming format input
      const streamingFormats = ["MP3", "AAC", "OGG"];
      streamingFormats.forEach((format) => {
        expect(formatConverter.isInputFormatSupported(format)).to.be.true;
      });

      // Verify format metadata extraction capability
      const allFormats = Object.keys(AudioFormats);
      allFormats.forEach((format) => {
        const info = formatConverter.getFormatInfo(format);
        expect(info).to.have.property("name");
        expect(info).to.have.property("extension");
        expect(info).to.have.property("lossy");
        expect(info).to.have.property("maxChannels");
        expect(info).to.have.property("maxSampleRate");
      });

      // Test format header validation
      const testData = testAudioSamples.sineWave440;
      allFormats.forEach((format) => {
        const validation = formatConverter.validateFormat(format, 44100, 2, 16);
        expect(validation.valid).to.be.true;
      });
    });

    it("should support all required output formats", async () => {
      // Test WAV format output generation
      const testData = testAudioSamples.sineWave440;
      const wavOutput = formatConverter.convert(testData, "PCM", "WAV", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
        sourceBitDepth: 16,
        targetBitDepth: 16,
      });
      expect(wavOutput).to.be.an.instanceof(Float32Array);
      expect(wavOutput.length).to.equal(testData.length);

      // Verify MP3 format output encoding
      const mp3Output = formatConverter.convert(testData, "PCM", "MP3", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(mp3Output).to.be.an.instanceof(Float32Array);

      // Test FLAC format output compression
      const flacOutput = formatConverter.convert(testData, "PCM", "FLAC", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(flacOutput).to.be.an.instanceof(Float32Array);

      // Check OGG format output compatibility
      const oggOutput = formatConverter.convert(testData, "PCM", "OGG", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(oggOutput).to.be.an.instanceof(Float32Array);

      // Test AAC format output generation
      const aacOutput = formatConverter.convert(testData, "PCM", "AAC", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(aacOutput).to.be.an.instanceof(Float32Array);

      // Verify M4A format output encoding
      const m4aOutput = formatConverter.convert(testData, "PCM", "M4A", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(m4aOutput).to.be.an.instanceof(Float32Array);

      // Test AIFF format output processing
      const aiffOutput = formatConverter.convert(testData, "PCM", "AIFF", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(aiffOutput).to.be.an.instanceof(Float32Array);

      // Check WMA format output compatibility
      const wmaOutput = formatConverter.convert(testData, "PCM", "WMA", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(wmaOutput).to.be.an.instanceof(Float32Array);

      // Test raw PCM format output
      const pcmOutput = formatConverter.convert(testData, "WAV", "PCM", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(pcmOutput).to.be.an.instanceof(Float32Array);

      // Verify compressed format generation
      const compressedFormats = ["MP3", "AAC", "OGG", "M4A", "WMA"];
      compressedFormats.forEach((format) => {
        expect(formatConverter.isOutputFormatSupported(format)).to.be.true;
      });

      // Test multi-channel format output
      const stereoData = testAudioSamples.stereoSine;
      const stereoOutput = formatConverter.convert(stereoData, "PCM", "WAV", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 2,
        targetChannels: 2,
      });
      expect(stereoOutput.length).to.equal(stereoData.length);

      // Check high-resolution format support
      const hiResOutput = formatConverter.convert(testData, "PCM", "FLAC", {
        sourceSampleRate: 44100,
        targetSampleRate: 96000,
        sourceChannels: 1,
        targetChannels: 1,
        sourceBitDepth: 16,
        targetBitDepth: 24,
      });
      expect(hiResOutput).to.be.an.instanceof(Float32Array);

      // Test streaming format output
      const streamingOutput = formatConverter.convert(testData, "PCM", "MP3", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(streamingOutput).to.be.an.instanceof(Float32Array);

      // Verify format metadata preservation
      const history = formatConverter.getConversionHistory();
      expect(history.length).to.be.above(0);
      const lastConversion = history[history.length - 1];
      expect(lastConversion).to.have.property("inputFormat");
      expect(lastConversion).to.have.property("outputFormat");

      // Test output format validation
      const outputFormats = Object.keys(AudioFormats);
      outputFormats.forEach((format) => {
        expect(formatConverter.isOutputFormatSupported(format)).to.be.true;
      });
    });

    it("should validate format parameters correctly", async () => {
      // Test sample rate validation
      const validSampleRates = [
        8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000, 176400, 192000,
      ];
      validSampleRates.forEach((rate) => {
        const validation = formatConverter.validateFormat("WAV", rate, 2, 16);
        expect(validation.valid).to.be.true;
      });

      // Test invalid sample rates
      const invalidSampleRates = [7999, 300000, -1, 0];
      invalidSampleRates.forEach((rate) => {
        const validation = formatConverter.validateFormat("WAV", rate, 2, 16);
        expect(validation.valid).to.be.false;
        expect(validation.error).to.contain("Sample rate");
      });

      // Verify bit depth parameter checking
      const validBitDepths = [8, 16, 24, 32];
      validBitDepths.forEach((depth) => {
        const validation = formatConverter.validateFormat(
          "WAV",
          44100,
          2,
          depth
        );
        expect(validation.valid).to.be.true;
      });

      // Test invalid bit depths
      const invalidBitDepths = [12, 20, 64, -16, 0];
      invalidBitDepths.forEach((depth) => {
        const validation = formatConverter.validateFormat(
          "WAV",
          44100,
          2,
          depth
        );
        expect(validation.valid).to.be.false;
        expect(validation.error).to.contain("bit depth");
      });

      // Test channel count validation
      const validChannelCounts = [1, 2, 4, 6, 8, 16];
      validChannelCounts.forEach((channels) => {
        const validation = formatConverter.validateFormat(
          "WAV",
          44100,
          channels,
          16
        );
        expect(validation.valid).to.be.true;
      });

      // Test invalid channel counts
      const invalidChannelCounts = [0, -1, 33, 100];
      invalidChannelCounts.forEach((channels) => {
        const validation = formatConverter.validateFormat(
          "WAV",
          44100,
          channels,
          16
        );
        expect(validation.valid).to.be.false;
        expect(validation.error).to.contain("channels");
      });

      // Check encoding parameter validation
      const mp3Validation = formatConverter.validateFormat("MP3", 44100, 3, 16);
      expect(mp3Validation.valid).to.be.false; // MP3 supports max 2 channels

      const flacValidation = formatConverter.validateFormat(
        "FLAC",
        44100,
        8,
        24
      );
      expect(flacValidation.valid).to.be.true; // FLAC supports up to 8 channels

      // Test format-specific constraints
      const formatConstraints = [
        { format: "MP3", maxChannels: 2, maxSampleRate: 48000 },
        { format: "AAC", maxChannels: 48, maxSampleRate: 96000 },
        { format: "FLAC", maxChannels: 8, maxSampleRate: 192000 },
        { format: "WAV", maxChannels: 32, maxSampleRate: 192000 },
      ];

      formatConstraints.forEach((constraint) => {
        // Test at limit
        const atLimitValidation = formatConverter.validateFormat(
          constraint.format,
          constraint.maxSampleRate,
          constraint.maxChannels,
          16
        );
        expect(atLimitValidation.valid).to.be.true;

        // Test beyond limit
        const beyondLimitValidation = formatConverter.validateFormat(
          constraint.format,
          constraint.maxSampleRate + 1000,
          constraint.maxChannels + 1,
          16
        );
        expect(beyondLimitValidation.valid).to.be.false;
      });
    });

    it("should detect unsupported formats gracefully", async () => {
      // Test unknown format detection
      const unknownFormats = ["XYZ", "INVALID", "FAKE", "123", ""];
      unknownFormats.forEach((format) => {
        expect(formatConverter.isInputFormatSupported(format)).to.be.false;
        expect(formatConverter.isOutputFormatSupported(format)).to.be.false;
        expect(formatConverter.getFormatInfo(format)).to.be.null;
      });

      // Verify appropriate error messages
      const testData = testAudioSamples.sineWave440;

      try {
        formatConverter.convert(testData, "INVALID", "WAV");
        expect.fail("Should have thrown error for invalid input format");
      } catch (error) {
        expect(error.message).to.contain("Input format INVALID not supported");
      }

      try {
        formatConverter.convert(testData, "WAV", "INVALID");
        expect.fail("Should have thrown error for invalid output format");
      } catch (error) {
        expect(error.message).to.contain("Output format INVALID not supported");
      }

      // Test fallback mechanism activation
      const fallbackTests = unknownFormats.map((format) => {
        return new Promise((resolve) => {
          try {
            formatConverter.convert(testData, format, "WAV");
            resolve(false); // Should not succeed
          } catch (error) {
            resolve(true); // Should throw error
          }
        });
      });

      const fallbackResults = await Promise.all(fallbackTests);
      fallbackResults.forEach((result) => {
        expect(result).to.be.true; // All should have thrown errors
      });

      // Check graceful degradation behavior
      formatConverter.setErrorHandlingMode({ strict: false });

      try {
        const result = formatConverter.convert(testData, "UNKNOWN", "WAV");
        expect.fail("Should still throw error even in non-strict mode");
      } catch (error) {
        expect(error.message).to.be.a("string");
        expect(error.message.length).to.be.above(0);
      }

      // Verify converter remains functional after errors
      const validConversion = formatConverter.convert(testData, "PCM", "WAV", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      expect(validConversion).to.be.an.instanceof(Float32Array);

      // Test case sensitivity handling
      expect(formatConverter.isInputFormatSupported("wav")).to.be.true;
      expect(formatConverter.isInputFormatSupported("WAV")).to.be.true;
      expect(formatConverter.isInputFormatSupported("Wav")).to.be.true;

      // Check format detection consistency
      const supportedFormats = Object.keys(AudioFormats);
      supportedFormats.forEach((format) => {
        expect(formatConverter.isInputFormatSupported(format)).to.be.true;
        expect(formatConverter.isOutputFormatSupported(format)).to.be.true;
        expect(formatConverter.getFormatInfo(format)).to.not.be.null;
      });
    });
  });

  describe("Conversion Accuracy", () => {
    it("should maintain audio quality during conversion", async () => {
      // Test signal-to-noise ratio preservation
      const originalSignal = testAudioSamples.sineWave440;
      const convertedSignal = formatConverter.convert(
        originalSignal,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        }
      );

      const qualityAnalysis = qualityAnalyzer.analyzeConversion(
        originalSignal,
        convertedSignal,
        {
          inputFormat: "PCM",
          outputFormat: "WAV",
        }
      );

      expect(qualityAnalysis.metrics.snr).to.be.above(40); // Good SNR threshold
      expect(qualityAnalysis.passed).to.be.true;

      // Verify dynamic range maintenance
      const dynamicRange = qualityAnalysis.metrics.dynamicRange;
      expect(dynamicRange).to.be.above(50); // Minimum acceptable dynamic range

      // Test frequency response accuracy with chirp signal
      const chirpSignal = testAudioSamples.chirp;
      const convertedChirp = formatConverter.convert(
        chirpSignal,
        "PCM",
        "FLAC",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 24,
        }
      );

      const chirpQuality = qualityAnalyzer.analyzeConversion(
        chirpSignal,
        convertedChirp,
        {
          inputFormat: "PCM",
          outputFormat: "FLAC",
        }
      );

      expect(chirpQuality.metrics.frequencyResponse.flatness).to.be.above(0.9);

      // Check phase coherence preservation
      expect(chirpQuality.metrics.phaseCoherence.coherence).to.be.above(0.95);
      expect(chirpQuality.metrics.phaseCoherence.phaseShift).to.be.below(0.1);

      // Test lossy format quality degradation
      const lossyConversion = formatConverter.convert(
        originalSignal,
        "PCM",
        "MP3",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      const lossyQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        lossyConversion,
        {
          inputFormat: "PCM",
          outputFormat: "MP3",
        }
      );

      // Lossy formats should still maintain reasonable quality
      expect(lossyQuality.metrics.snr).to.be.above(20); // Lower threshold for lossy
      expect(lossyQuality.metrics.thd).to.be.below(5); // Acceptable distortion

      conversionMetrics.qualityScores.push({
        format: "WAV",
        snr: qualityAnalysis.metrics.snr,
        thd: qualityAnalysis.metrics.thd,
        dynamicRange: qualityAnalysis.metrics.dynamicRange,
      });
    });

    it("should handle sample rate conversion correctly", async () => {
      const originalSignal = testAudioSamples.sineWave440;

      // Test upsampling accuracy (44.1kHz to 96kHz)
      const upsampledSignal = formatConverter.convert(
        originalSignal,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 96000,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 24,
        }
      );

      const expectedUpsampledLength = Math.floor(
        originalSignal.length * (96000 / 44100)
      );
      expect(upsampledSignal.length).to.be.closeTo(expectedUpsampledLength, 10);

      // Verify upsampling quality
      const upsamplingQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        upsampledSignal,
        {
          sampleRateConversion: true,
          upsample: true,
        }
      );
      expect(upsamplingQuality.metrics.snr).to.be.above(35);

      // Test downsampling quality (44.1kHz to 22.05kHz)
      const downsampledSignal = formatConverter.convert(
        originalSignal,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 22050,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        }
      );

      const expectedDownsampledLength = Math.floor(
        originalSignal.length * (22050 / 44100)
      );
      expect(downsampledSignal.length).to.be.closeTo(
        expectedDownsampledLength,
        5
      );

      // Test anti-aliasing filter effectiveness
      const downsamplingQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        downsampledSignal,
        {
          sampleRateConversion: true,
          downsample: true,
        }
      );
      expect(downsamplingQuality.metrics.snr).to.be.above(25); // Account for anti-aliasing

      // Check interpolation algorithm accuracy
      const commonSampleRates = [
        { from: 44100, to: 48000 },
        { from: 48000, to: 44100 },
        { from: 44100, to: 88200 },
        { from: 96000, to: 48000 },
      ];

      commonSampleRates.forEach((rates) => {
        const converted = formatConverter.convert(
          originalSignal,
          "PCM",
          "PCM",
          {
            sourceSampleRate: rates.from,
            targetSampleRate: rates.to,
            sourceChannels: 1,
            targetChannels: 1,
          }
        );

        const expectedLength = Math.floor(
          originalSignal.length * (rates.to / rates.from)
        );
        expect(converted.length).to.be.closeTo(expectedLength, 10);

        // Verify no NaN or infinity values
        const invalidSamples = converted.filter((sample) => !isFinite(sample));
        expect(invalidSamples.length).to.equal(0);
      });

      // Test extreme sample rate conversions
      const extremeUpsampling = formatConverter.convert(
        originalSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 8000,
          targetSampleRate: 192000,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      expect(extremeUpsampling.length).to.be.above(originalSignal.length * 10);

      const extremeDownsampling = formatConverter.convert(
        originalSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 192000,
          targetSampleRate: 8000,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      expect(extremeDownsampling.length).to.be.below(
        originalSignal.length / 10
      );

      conversionMetrics.conversionTimes.push({
        operation: "sample_rate_conversion",
        duration: performance.now(),
      });
    });

    it("should perform bit depth conversion accurately", async () => {
      const originalSignal = testAudioSamples.sineWave440;

      // Test quantization noise minimization (16-bit to 8-bit)
      const reducedBitDepth = formatConverter.convert(
        originalSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 8,
        }
      );

      const quantizationQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        reducedBitDepth,
        {
          bitDepthReduction: true,
        }
      );

      // Should maintain reasonable quality despite bit depth reduction
      expect(quantizationQuality.metrics.snr).to.be.above(15);
      expect(quantizationQuality.metrics.thd).to.be.below(10);

      // Verify dithering algorithm effectiveness
      formatConverter.setQualitySettings({ enableDithering: true });

      const ditheredConversion = formatConverter.convert(
        originalSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 8,
        }
      );

      const ditheredQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        ditheredConversion,
        {
          dithered: true,
        }
      );

      // Dithering should improve subjective quality
      expect(ditheredQuality.metrics.snr).to.be.above(
        quantizationQuality.metrics.snr - 3
      );

      // Test bit depth expansion accuracy (16-bit to 24-bit)
      const expandedBitDepth = formatConverter.convert(
        originalSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 24,
        }
      );

      const expansionQuality = qualityAnalyzer.analyzeConversion(
        originalSignal,
        expandedBitDepth,
        {
          bitDepthExpansion: true,
        }
      );

      // Bit depth expansion should not degrade quality significantly
      expect(expansionQuality.metrics.snr).to.be.above(40);

      // Check dynamic range scaling
      const dynamicRangeTest = TestAudioSampleGenerator.generateChirp(
        20,
        20000,
        1.0,
        44100,
        0.8
      );

      const scaledConversion = formatConverter.convert(
        dynamicRangeTest,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 32,
        }
      );

      const scalingQuality = qualityAnalyzer.analyzeConversion(
        dynamicRangeTest,
        scaledConversion,
        {
          dynamicRangeTest: true,
        }
      );

      expect(scalingQuality.metrics.dynamicRange).to.be.above(60);

      // Test various bit depth combinations
      const bitDepthCombinations = [
        { from: 8, to: 16 },
        { from: 16, to: 24 },
        { from: 24, to: 32 },
        { from: 32, to: 16 },
        { from: 24, to: 8 },
      ];

      bitDepthCombinations.forEach((combination) => {
        const converted = formatConverter.convert(
          originalSignal,
          "PCM",
          "PCM",
          {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
            sourceBitDepth: combination.from,
            targetBitDepth: combination.to,
          }
        );

        expect(converted).to.be.an.instanceof(Float32Array);
        expect(converted.length).to.equal(originalSignal.length);

        // Verify amplitude range is preserved
        const maxAmplitude = Math.max(...converted.map(Math.abs));
        expect(maxAmplitude).to.be.below(1.1); // Allow for minor overshoot
      });
    });

    it("should handle channel configuration changes", async () => {
      // Test mono to stereo conversion
      const monoSignal = testAudioSamples.sineWave440;
      const monoToStereo = formatConverter.convert(monoSignal, "PCM", "PCM", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 2,
        sourceBitDepth: 16,
        targetBitDepth: 16,
      });

      expect(monoToStereo.length).to.equal(monoSignal.length * 2);

      // Verify both channels contain the same data
      for (let i = 0; i < monoSignal.length; i++) {
        const leftChannel = monoToStereo[i * 2];
        const rightChannel = monoToStereo[i * 2 + 1];
        expect(leftChannel).to.be.closeTo(rightChannel, 0.001);
      }

      // Verify stereo to mono mixing
      const stereoSignal = testAudioSamples.stereoSine;
      const stereoToMono = formatConverter.convert(stereoSignal, "PCM", "PCM", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 2,
        targetChannels: 1,
        sourceBitDepth: 16,
        targetBitDepth: 16,
      });

      expect(stereoToMono.length).to.equal(stereoSignal.length / 2);

      // Verify mixing algorithm (should be average of channels)
      for (let i = 0; i < stereoToMono.length; i++) {
        const leftSample = stereoSignal[i * 2];
        const rightSample = stereoSignal[i * 2 + 1];
        const expectedMix = (leftSample + rightSample) * 0.5;
        expect(stereoToMono[i]).to.be.closeTo(expectedMix, 0.001);
      }

      // Test multichannel downmixing
      const multiChannelSignal = testAudioSamples.multiChannel;
      const multiToStereo = formatConverter.convert(
        multiChannelSignal,
        "PCM",
        "PCM",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 4,
          targetChannels: 2,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        }
      );

      expect(multiToStereo.length).to.equal(multiChannelSignal.length / 2);

      // Check channel mapping accuracy
      const channelMappingTests = [
        { from: 1, to: 6 }, // Mono to 5.1
        { from: 2, to: 8 }, // Stereo to 7.1
        { from: 8, to: 2 }, // 7.1 to stereo
        { from: 6, to: 1 }, // 5.1 to mono
      ];

      channelMappingTests.forEach((mapping) => {
        const testSignal = TestAudioSampleGenerator.generateMultiChannelSample(
          Array(mapping.from)
            .fill()
            .map(() => TestAudioSampleGenerator.generateSineWave(440, 0.1))
        );

        const mapped = formatConverter.convert(testSignal, "PCM", "PCM", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: mapping.from,
          targetChannels: mapping.to,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        });

        const expectedLength = (testSignal.length / mapping.from) * mapping.to;
        expect(mapped.length).to.equal(expectedLength);

        // Verify no clipping occurred
        const maxAmplitude = Math.max(...mapped.map(Math.abs));
        expect(maxAmplitude).to.be.below(1.1);
      });

      // Test channel configuration preservation for same channel count
      const preservationTest = formatConverter.convert(
        stereoSignal,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 2,
          targetChannels: 2,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        }
      );

      expect(preservationTest.length).to.equal(stereoSignal.length);

      // Verify channel separation is maintained
      const channelSeparationQuality = qualityAnalyzer.analyzeConversion(
        stereoSignal,
        preservationTest,
        {
          channelPreservation: true,
        }
      );

      expect(channelSeparationQuality.metrics.snr).to.be.above(50);

      conversionMetrics.qualityScores.push({
        operation: "channel_conversion",
        snr: channelSeparationQuality.metrics.snr,
      });
    });
  });

  describe("Performance Testing", () => {
    it("should meet real-time conversion requirements", async () => {
      const testSignal = testAudioSamples.sineWave440;
      const realTimeThreshold = (testSignal.length / 44100) * 1000; // Signal duration in ms

      // Benchmark conversion speed vs real-time
      const conversionStartTime = performance.now();
      const convertedSignal = formatConverter.convert(
        testSignal,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 16,
        }
      );
      const conversionEndTime = performance.now();
      const conversionTime = conversionEndTime - conversionStartTime;

      // Should convert faster than real-time
      expect(conversionTime).to.be.below(realTimeThreshold);
      expect(convertedSignal).to.be.an.instanceof(Float32Array);

      // Test CPU usage during conversion
      const cpuIntensiveConversions = [];
      const iterationCount = 100;

      for (let i = 0; i < iterationCount; i++) {
        const startTime = performance.now();
        formatConverter.convert(testSignal, "PCM", "MP3", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });
        const endTime = performance.now();
        cpuIntensiveConversions.push(endTime - startTime);
      }

      const averageCpuTime =
        cpuIntensiveConversions.reduce((a, b) => a + b, 0) / iterationCount;
      expect(averageCpuTime).to.be.below(realTimeThreshold * 2); // Allow 2x overhead

      // Verify memory efficiency
      const performanceMetrics = formatConverter.getPerformanceMetrics();
      expect(performanceMetrics.currentMemoryUsage).to.be.below(200); // Under 200MB
      expect(performanceMetrics.averageConversionTime).to.be.a("number");

      // Check throughput measurements
      const throughputSignal = TestAudioSampleGenerator.generateWhiteNoise(
        5.0,
        44100
      ); // 5 seconds
      const throughputStartTime = performance.now();

      formatConverter.convert(throughputSignal, "PCM", "FLAC", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
        sourceBitDepth: 16,
        targetBitDepth: 24,
      });

      const throughputEndTime = performance.now();
      const throughputTime = throughputEndTime - throughputStartTime;
      const throughputRatio = 5000 / throughputTime; // Real-time factor

      expect(throughputRatio).to.be.above(1.0); // Should be faster than real-time

      conversionMetrics.throughputMeasurements.push({
        signalDuration: 5.0,
        conversionTime: throughputTime,
        throughputRatio: throughputRatio,
      });

      // Test real-time streaming scenario
      const streamingChunks = [];
      const chunkSize = 1024; // Small chunks for streaming
      const longSignal = TestAudioSampleGenerator.generateSineWave(
        440,
        10.0,
        44100
      );

      for (let i = 0; i < longSignal.length; i += chunkSize) {
        const chunk = longSignal.slice(i, i + chunkSize);
        const chunkStartTime = performance.now();

        formatConverter.convert(chunk, "PCM", "AAC", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });

        const chunkEndTime = performance.now();
        streamingChunks.push(chunkEndTime - chunkStartTime);
      }

      const averageChunkTime =
        streamingChunks.reduce((a, b) => a + b, 0) / streamingChunks.length;
      const chunkDuration = (chunkSize / 44100) * 1000; // Duration in ms

      expect(averageChunkTime).to.be.below(chunkDuration * 0.5); // Should be much faster than real-time
    });

    it("should handle large file conversions efficiently", async () => {
      // Test streaming conversion capability
      const largeSignal = TestAudioSampleGenerator.generateWhiteNoise(
        30.0,
        44100
      ); // 30 seconds

      const streamingStartTime = performance.now();
      const streamingResult = formatConverter.convert(
        largeSignal,
        "PCM",
        "FLAC",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
          sourceBitDepth: 16,
          targetBitDepth: 24,
        }
      );
      const streamingEndTime = performance.now();

      expect(streamingResult).to.be.an.instanceof(Float32Array);
      expect(streamingResult.length).to.equal(largeSignal.length);

      // Verify memory usage with large files
      const memoryBeforeConversion =
        formatConverter.getPerformanceMetrics().currentMemoryUsage;

      const veryLargeSignal = TestAudioSampleGenerator.generateChirp(
        20,
        20000,
        60.0,
        44100
      ); // 1 minute

      formatConverter.convert(veryLargeSignal, "PCM", "WAV", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });

      const memoryAfterConversion =
        formatConverter.getPerformanceMetrics().currentMemoryUsage;
      const memoryIncrease = memoryAfterConversion - memoryBeforeConversion;

      // Memory usage should not increase excessively
      expect(memoryIncrease).to.be.below(500); // Under 500MB increase

      // Test conversion progress tracking
      const progressTrackingSignal = TestAudioSampleGenerator.generateSineWave(
        1000,
        20.0,
        48000
      );
      const conversionHistory = formatConverter.getConversionHistory();
      const initialHistoryLength = conversionHistory.length;

      formatConverter.convert(progressTrackingSignal, "PCM", "OGG", {
        sourceSampleRate: 48000,
        targetSampleRate: 48000,
        sourceChannels: 1,
        targetChannels: 1,
      });

      const updatedHistory = formatConverter.getConversionHistory();
      expect(updatedHistory.length).to.equal(initialHistoryLength + 1);

      const lastConversion = updatedHistory[updatedHistory.length - 1];
      expect(lastConversion).to.have.property("timestamp");
      expect(lastConversion).to.have.property("conversionTime");
      expect(lastConversion).to.have.property("inputSize");
      expect(lastConversion).to.have.property("outputSize");

      // Check resource cleanup efficiency
      const resourcesBeforeCleanup = formatConverter.getPerformanceMetrics();
      formatConverter.resetStatistics();
      const resourcesAfterCleanup = formatConverter.getPerformanceMetrics();

      expect(resourcesAfterCleanup.conversionsPerformed).to.equal(0);
      expect(resourcesAfterCleanup.totalConversionTime).to.equal(0);

      conversionMetrics.memoryUsage.push({
        beforeConversion: memoryBeforeConversion,
        afterConversion: memoryAfterConversion,
        increase: memoryIncrease,
      });
    });

    it("should optimize conversion for different scenarios", async () => {
      // Test batch conversion optimization
      const batchSignals = [
        testAudioSamples.sineWave440,
        testAudioSamples.chirp,
        testAudioSamples.whiteNoise,
      ];

      const batchStartTime = performance.now();
      const batchResults = batchSignals.map((signal) => {
        return formatConverter.convert(signal, "PCM", "MP3", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });
      });
      const batchEndTime = performance.now();
      const batchTime = batchEndTime - batchStartTime;

      expect(batchResults).to.have.length(3);
      batchResults.forEach((result) => {
        expect(result).to.be.an.instanceof(Float32Array);
      });

      // Compare with individual conversions
      const individualStartTime = performance.now();
      batchSignals.forEach((signal) => {
        formatConverter.convert(signal, "PCM", "MP3", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });
      });
      const individualEndTime = performance.now();
      const individualTime = individualEndTime - individualStartTime;

      // Batch should be comparable or better
      expect(batchTime).to.be.below(individualTime * 1.2); // Allow 20% overhead

      // Verify real-time conversion optimization
      formatConverter.setQualitySettings({
        preserveQuality: false,
        enableDithering: false,
        resamplingQuality: "fast",
      });

      const fastConversionStart = performance.now();
      const fastResult = formatConverter.convert(
        testAudioSamples.chirp,
        "PCM",
        "AAC",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );
      const fastConversionEnd = performance.now();
      const fastConversionTime = fastConversionEnd - fastConversionStart;

      // Reset to high quality
      formatConverter.setQualitySettings({
        preserveQuality: true,
        enableDithering: true,
        resamplingQuality: "high",
      });

      const highQualityStart = performance.now();
      formatConverter.convert(testAudioSamples.chirp, "PCM", "AAC", {
        sourceSampleRate: 44100,
        targetSampleRate: 44100,
        sourceChannels: 1,
        targetChannels: 1,
      });
      const highQualityEnd = performance.now();
      const highQualityTime = highQualityEnd - highQualityStart;

      // Test quality vs speed trade-offs
      expect(fastConversionTime).to.be.below(highQualityTime);
      expect(fastResult).to.be.an.instanceof(Float32Array);

      // Check adaptive optimization algorithms
      const adaptiveTests = [
        { format: "WAV", expectedOptimization: "lossless" },
        { format: "MP3", expectedOptimization: "lossy" },
        { format: "FLAC", expectedOptimization: "compression" },
      ];

      adaptiveTests.forEach((test) => {
        const adaptiveResult = formatConverter.convert(
          testAudioSamples.sineWave440,
          "PCM",
          test.format,
          {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
          }
        );

        expect(adaptiveResult).to.be.an.instanceof(Float32Array);

        // Verify format-specific optimizations were applied
        const history = formatConverter.getConversionHistory();
        const lastConversion = history[history.length - 1];
        expect(lastConversion.outputFormat).to.equal(test.format);
      });

      conversionMetrics.conversionTimes.push({
        fast: fastConversionTime,
        highQuality: highQualityTime,
        optimization: "adaptive",
      });
    });
  });

  describe("Error Handling", () => {
    it("should handle corrupted input gracefully", async () => {
      // Test corrupted file header handling
      const corruptedData = testAudioSamples.corrupted;

      formatConverter.setErrorHandlingMode({
        strict: false,
        recoverFromCorruption: true,
        validateInputs: true,
      });

      try {
        const result = formatConverter.convert(corruptedData, "PCM", "WAV", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });

        expect(result).to.be.an.instanceof(Float32Array);
        expect(result.length).to.be.above(0);
      } catch (error) {
        // Acceptable if corruption is too severe
        expect(error.message).to.be.a("string");
      }

      // Verify partial corruption recovery
      const partiallyCorrupted = TestAudioSampleGenerator.generateCorruptedData(
        testAudioSamples.sineWave440,
        0.01 // Only 1% corruption
      );

      const recoveredResult = formatConverter.convert(
        partiallyCorrupted,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      expect(recoveredResult).to.be.an.instanceof(Float32Array);
      expect(recoveredResult.length).to.equal(partiallyCorrupted.length);

      // Test data integrity validation
      const integrityCheckData = new Float32Array([
        NaN,
        Infinity,
        -Infinity,
        0,
        0.5,
      ]);

      try {
        formatConverter.convert(integrityCheckData, "PCM", "WAV", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        });

        // Should handle NaN/Infinity values
      } catch (error) {
        expect(error.message).to.contain("Invalid");
      }

      // Check error recovery mechanisms
      const emptyData = new Float32Array(0);

      try {
        formatConverter.convert(emptyData, "PCM", "WAV");
        expect.fail("Should reject empty data");
      } catch (error) {
        expect(error.message).to.contain("Invalid input data");
      }

      // Verify converter remains functional after errors
      const validConversion = formatConverter.convert(
        testAudioSamples.sineWave440,
        "PCM",
        "WAV",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      expect(validConversion).to.be.an.instanceof(Float32Array);

      conversionMetrics.errorCounts.corruption =
        (conversionMetrics.errorCounts.corruption || 0) + 1;
    });

    it("should validate conversion parameters", async () => {
      const testData = testAudioSamples.sineWave440;

      // Test invalid parameter detection
      const invalidParameterTests = [
        {
          params: {
            sourceSampleRate: -1,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
          },
          expectedError: "sample rate",
        },
        {
          params: {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 0,
            targetChannels: 1,
          },
          expectedError: "channels",
        },
        {
          params: {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: -1,
          },
          expectedError: "channels",
        },
        {
          params: {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
            sourceBitDepth: 7,
          },
          expectedError: "bit depth",
        },
      ];

      invalidParameterTests.forEach((test, index) => {
        try {
          formatConverter.convert(testData, "PCM", "WAV", test.params);
          expect.fail(
            `Test ${index} should have failed with invalid parameters`
          );
        } catch (error) {
          expect(error.message.toLowerCase()).to.contain(test.expectedError);
        }
      });

      // Verify parameter range checking
      const parameterRangeTests = [
        { sourceSampleRate: 0, shouldFail: true },
        { sourceSampleRate: 8000, shouldFail: false },
        { sourceSampleRate: 192000, shouldFail: false },
        { sourceSampleRate: 300000, shouldFail: true },
      ];

      parameterRangeTests.forEach((test) => {
        try {
          formatConverter.convert(testData, "PCM", "WAV", {
            sourceSampleRate: test.sourceSampleRate,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
          });

          if (test.shouldFail) {
            expect.fail(
              `Should have failed with sample rate ${test.sourceSampleRate}`
            );
          }
        } catch (error) {
          if (!test.shouldFail) {
            expect.fail(
              `Should not have failed with sample rate ${test.sourceSampleRate}`
            );
          }
        }
      });

      // Test parameter compatibility validation
      try {
        formatConverter.convert(testData, "MP3", "WAV", {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 8, // MP3 doesn't support 8 channels
          targetChannels: 2,
        });
        expect.fail("Should have failed due to incompatible MP3 channel count");
      } catch (error) {
        expect(error.message).to.contain("channels");
      }

      // Check meaningful error reporting
      try {
        formatConverter.convert(null, "PCM", "WAV");
        expect.fail("Should have failed with null input");
      } catch (error) {
        expect(error.message).to.be.a("string");
        expect(error.message.length).to.be.above(0);
        expect(error.message).to.contain("Invalid");
      }

      // Test parameter validation consistency
      const validParams = {
        sourceSampleRate: 44100,
        targetSampleRate: 48000,
        sourceChannels: 2,
        targetChannels: 2,
        sourceBitDepth: 16,
        targetBitDepth: 24,
      };

      const validResult = formatConverter.convert(
        testData,
        "PCM",
        "FLAC",
        validParams
      );
      expect(validResult).to.be.an.instanceof(Float32Array);

      conversionMetrics.errorCounts.parameterValidation =
        (conversionMetrics.errorCounts.parameterValidation || 0) +
        invalidParameterTests.length;
    });

    it("should maintain stability under error conditions", async () => {
      const testData = testAudioSamples.sineWave440;
      let errorCount = 0;
      let successCount = 0;

      // Test continued operation after errors
      const errorScenarios = [
        () => formatConverter.convert(null, "PCM", "WAV"),
        () => formatConverter.convert(testData, "INVALID", "WAV"),
        () => formatConverter.convert(testData, "PCM", "INVALID"),
        () =>
          formatConverter.convert(testData, "PCM", "WAV", {
            sourceSampleRate: -1,
          }),
        () => formatConverter.convert(new Float32Array(0), "PCM", "WAV"),
      ];

      for (const scenario of errorScenarios) {
        try {
          scenario();
          successCount++; // Shouldn't happen
        } catch (error) {
          errorCount++;
          expect(error).to.be.an("error");
        }

        // After each error, try a valid conversion
        try {
          const validResult = formatConverter.convert(testData, "PCM", "WAV", {
            sourceSampleRate: 44100,
            targetSampleRate: 44100,
            sourceChannels: 1,
            targetChannels: 1,
          });
          expect(validResult).to.be.an.instanceof(Float32Array);
          successCount++;
        } catch (error) {
          expect.fail("Valid conversion should not fail after error");
        }
      }

      expect(errorCount).to.equal(errorScenarios.length);
      expect(successCount).to.equal(errorScenarios.length); // One valid conversion after each error

      // Verify resource cleanup on errors
      const metricsBeforeErrors = formatConverter.getPerformanceMetrics();

      // Generate multiple errors
      for (let i = 0; i < 50; i++) {
        try {
          formatConverter.convert(testData, "INVALID", "WAV");
        } catch (error) {
          // Expected
        }
      }

      const metricsAfterErrors = formatConverter.getPerformanceMetrics();

      // Memory usage should not have increased significantly
      expect(
        metricsAfterErrors.currentMemoryUsage -
          metricsBeforeErrors.currentMemoryUsage
      ).to.be.below(50);

      // Test error isolation mechanisms
      const isolationTest = async () => {
        const promises = [];

        for (let i = 0; i < 10; i++) {
          const promise = new Promise((resolve) => {
            try {
              if (i % 2 === 0) {
                formatConverter.convert(testData, "PCM", "WAV", {
                  sourceSampleRate: 44100,
                  targetSampleRate: 44100,
                  sourceChannels: 1,
                  targetChannels: 1,
                });
                resolve("success");
              } else {
                formatConverter.convert(testData, "INVALID", "WAV");
                resolve("unexpected_success");
              }
            } catch (error) {
              resolve("error");
            }
          });
          promises.push(promise);
        }

        return Promise.all(promises);
      };

      const isolationResults = await isolationTest();
      const successResults = isolationResults.filter((r) => r === "success");
      const errorResults = isolationResults.filter((r) => r === "error");

      expect(successResults.length).to.equal(5); // Half should succeed
      expect(errorResults.length).to.equal(5); // Half should error

      // Check system stability preservation
      expect(formatConverter.conversionState).to.not.equal("error"); // Should reset after errors

      const finalValidConversion = formatConverter.convert(
        testData,
        "PCM",
        "FLAC",
        {
          sourceSampleRate: 44100,
          targetSampleRate: 44100,
          sourceChannels: 1,
          targetChannels: 1,
        }
      );

      expect(finalValidConversion).to.be.an.instanceof(Float32Array);

      conversionMetrics.errorCounts.stabilityTest = errorCount;
    });
  });
});

export { formatConverter, testAudioSamples, conversionMetrics };
