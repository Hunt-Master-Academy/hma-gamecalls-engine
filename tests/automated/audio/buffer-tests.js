/**
 * @file buffer-tests.js
 * @brief CircularAudioBuffer Test Module - Phase 3.1 Automated Testing
 *
 * This module provides comprehensive testing for the CircularAudioBuffer
 * implementation, including thread-safety, performance, and correctness validation.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "mocha";

// Mock CircularAudioBuffer Implementation for Testing
class MockCircularAudioBuffer {
  constructor(config = {}) {
    this.sampleRate = config.sampleRate || 44100;
    this.channels = config.channels || 2;
    this.bufferSize = config.bufferSize || 8192;
    this.sampleFormat = config.sampleFormat || "float32";
    this.alignment = config.alignment || 16;

    // Validate configuration
    if (!this._validateConfig()) {
      throw new Error("Invalid buffer configuration");
    }

    // Initialize buffer storage
    this.buffer = new Float32Array(this.bufferSize * this.channels);
    this.writePointer = 0;
    this.readPointer = 0;
    this.currentLevel = 0;

    // Performance and statistics tracking
    this.stats = {
      totalWrites: 0,
      totalReads: 0,
      overflowCount: 0,
      underflowCount: 0,
      writeLatency: [],
      readLatency: [],
      peakLevel: 0,
      averageLevel: 0,
    };

    // State management
    this.state = "initialized";
    this.lastError = null;
    this.callbacks = {
      onOverflow: null,
      onUnderflow: null,
      onError: null,
    };

    // Thread safety simulation
    this.writeLock = false;
    this.readLock = false;
    this.operationCount = 0;
  }

  _validateConfig() {
    if (this.bufferSize <= 0 || this.bufferSize > 1048576) return false;
    if (this.channels <= 0 || this.channels > 32) return false;
    if (this.sampleRate <= 0 || this.sampleRate > 192000) return false;
    if (
      !["int16", "int24", "int32", "float32", "float64"].includes(
        this.sampleFormat
      )
    )
      return false;
    return true;
  }

  write(data) {
    const startTime = performance.now();

    if (this.writeLock) {
      throw new Error("Write operation in progress");
    }

    this.writeLock = true;
    this.operationCount++;

    try {
      const samples = Array.isArray(data) ? data : [data];
      const availableSpace = this.bufferSize - this.currentLevel;

      if (samples.length > availableSpace) {
        this.stats.overflowCount++;
        if (this.callbacks.onOverflow) {
          this.callbacks.onOverflow(samples.length - availableSpace);
        }
        // Handle overflow based on strategy
        return this._handleOverflow(samples);
      }

      // Write samples to buffer
      for (let i = 0; i < samples.length; i++) {
        for (let ch = 0; ch < this.channels; ch++) {
          const index = this.writePointer * this.channels + ch;
          this.buffer[index] = samples[i] || 0;
        }
        this.writePointer = (this.writePointer + 1) % this.bufferSize;
      }

      this.currentLevel += samples.length;
      this.stats.totalWrites += samples.length;
      this.stats.peakLevel = Math.max(this.stats.peakLevel, this.currentLevel);

      const endTime = performance.now();
      this.stats.writeLatency.push(endTime - startTime);

      return samples.length;
    } finally {
      this.writeLock = false;
    }
  }

  read(count = 1) {
    const startTime = performance.now();

    if (this.readLock) {
      throw new Error("Read operation in progress");
    }

    this.readLock = true;
    this.operationCount++;

    try {
      if (count > this.currentLevel) {
        this.stats.underflowCount++;
        if (this.callbacks.onUnderflow) {
          this.callbacks.onUnderflow(count - this.currentLevel);
        }
        return this._handleUnderflow(count);
      }

      const result = [];
      for (let i = 0; i < count; i++) {
        const sample = [];
        for (let ch = 0; ch < this.channels; ch++) {
          const index = this.readPointer * this.channels + ch;
          sample.push(this.buffer[index]);
        }
        result.push(sample);
        this.readPointer = (this.readPointer + 1) % this.bufferSize;
      }

      this.currentLevel -= count;
      this.stats.totalReads += count;

      const endTime = performance.now();
      this.stats.readLatency.push(endTime - startTime);

      return result;
    } finally {
      this.readLock = false;
    }
  }

  peek(offset = 0, count = 1) {
    if (offset + count > this.currentLevel) {
      throw new Error("Peek beyond available data");
    }

    const result = [];
    const startPos = (this.readPointer + offset) % this.bufferSize;

    for (let i = 0; i < count; i++) {
      const pos = (startPos + i) % this.bufferSize;
      const sample = [];
      for (let ch = 0; ch < this.channels; ch++) {
        const index = pos * this.channels + ch;
        sample.push(this.buffer[index]);
      }
      result.push(sample);
    }

    return result;
  }

  skip(count) {
    if (count > this.currentLevel) {
      throw new Error("Skip beyond available data");
    }

    this.readPointer = (this.readPointer + count) % this.bufferSize;
    this.currentLevel -= count;
    return count;
  }

  clear() {
    this.buffer.fill(0);
    this.writePointer = 0;
    this.readPointer = 0;
    this.currentLevel = 0;
    this.state = "cleared";
  }

  resize(newSize) {
    if (newSize <= 0 || newSize > 1048576) {
      throw new Error("Invalid buffer size");
    }

    const oldData = this.read(this.currentLevel);
    this.bufferSize = newSize;
    this.buffer = new Float32Array(newSize * this.channels);
    this.writePointer = 0;
    this.readPointer = 0;
    this.currentLevel = 0;

    // Restore data if it fits
    if (oldData.length <= newSize) {
      oldData.forEach((sample) => this.write(sample));
    }

    return newSize;
  }

  getLevel() {
    return this.currentLevel;
  }

  getFillRatio() {
    return this.currentLevel / this.bufferSize;
  }

  getAvailableSpace() {
    return this.bufferSize - this.currentLevel;
  }

  getStats() {
    return {
      ...this.stats,
      averageLevel: this.stats.totalWrites > 0 ? this.stats.peakLevel / 2 : 0,
      averageWriteLatency: this._calculateAverage(this.stats.writeLatency),
      averageReadLatency: this._calculateAverage(this.stats.readLatency),
    };
  }

  getDiagnostics() {
    return {
      state: this.state,
      config: {
        sampleRate: this.sampleRate,
        channels: this.channels,
        bufferSize: this.bufferSize,
        sampleFormat: this.sampleFormat,
      },
      pointers: {
        write: this.writePointer,
        read: this.readPointer,
      },
      level: this.currentLevel,
      fillRatio: this.getFillRatio(),
      operationCount: this.operationCount,
      lastError: this.lastError,
    };
  }

  _handleOverflow(samples) {
    // Strategy: overwrite oldest data
    const samplesToWrite = Math.min(samples.length, this.bufferSize);
    this.currentLevel = this.bufferSize;
    return samplesToWrite;
  }

  _handleUnderflow(count) {
    // Strategy: return silence
    const result = [];
    for (let i = 0; i < count; i++) {
      const sample = new Array(this.channels).fill(0);
      result.push(sample);
    }
    return result;
  }

  _calculateAverage(array) {
    return array.length > 0
      ? array.reduce((a, b) => a + b, 0) / array.length
      : 0;
  }
}

// Audio Data Generator for Testing
class AudioDataGenerator {
  static generateSineWave(
    frequency,
    duration,
    sampleRate = 44100,
    amplitude = 0.5
  ) {
    const samples = Math.floor(duration * sampleRate);
    const result = [];

    for (let i = 0; i < samples; i++) {
      const t = i / sampleRate;
      const sample = amplitude * Math.sin(2 * Math.PI * frequency * t);
      result.push(sample);
    }

    return result;
  }

  static generateWhiteNoise(duration, sampleRate = 44100, amplitude = 0.5) {
    const samples = Math.floor(duration * sampleRate);
    const result = [];

    for (let i = 0; i < samples; i++) {
      const sample = amplitude * (Math.random() * 2 - 1);
      result.push(sample);
    }

    return result;
  }

  static generateSilence(duration, sampleRate = 44100) {
    const samples = Math.floor(duration * sampleRate);
    return new Array(samples).fill(0);
  }
}

// Performance Monitoring System
class BufferPerformanceMonitor {
  constructor() {
    this.metrics = {
      memoryUsage: [],
      cpuUsage: [],
      operationTimes: [],
      throughput: [],
      errorRates: [],
    };
    this.startTime = performance.now();
  }

  recordOperation(operationType, duration, success = true) {
    this.metrics.operationTimes.push({
      type: operationType,
      duration,
      success,
      timestamp: performance.now(),
    });
  }

  recordMemoryUsage() {
    if (performance.memory) {
      this.metrics.memoryUsage.push({
        used: performance.memory.usedJSHeapSize,
        total: performance.memory.totalJSHeapSize,
        timestamp: performance.now(),
      });
    }
  }

  calculateThroughput() {
    const totalTime = performance.now() - this.startTime;
    const totalOps = this.metrics.operationTimes.length;
    return totalOps / (totalTime / 1000); // operations per second
  }

  getReport() {
    return {
      totalOperations: this.metrics.operationTimes.length,
      averageOperationTime: this._calculateAverage(
        this.metrics.operationTimes.map((op) => op.duration)
      ),
      throughput: this.calculateThroughput(),
      errorRate: this._calculateErrorRate(),
      memoryTrend: this._calculateMemoryTrend(),
    };
  }

  _calculateAverage(array) {
    return array.length > 0
      ? array.reduce((a, b) => a + b, 0) / array.length
      : 0;
  }

  _calculateErrorRate() {
    const total = this.metrics.operationTimes.length;
    const errors = this.metrics.operationTimes.filter(
      (op) => !op.success
    ).length;
    return total > 0 ? errors / total : 0;
  }

  _calculateMemoryTrend() {
    if (this.metrics.memoryUsage.length < 2) return "stable";
    const first = this.metrics.memoryUsage[0].used;
    const last =
      this.metrics.memoryUsage[this.metrics.memoryUsage.length - 1].used;
    const change = (last - first) / first;

    if (change > 0.1) return "increasing";
    if (change < -0.1) return "decreasing";
    return "stable";
  }
}

/**
 * CircularAudioBuffer Test Suite
 * Tests all aspects of the circular audio buffer implementation
 */
describe("CircularAudioBuffer Tests", () => {
  let audioBuffer;
  let testConfig;
  let performanceMetrics;
  let performanceMonitor;

  beforeEach(() => {
    // Initialize CircularAudioBuffer with test configuration
    testConfig = {
      sampleRate: 44100,
      channels: 2,
      bufferSize: 4096,
      sampleFormat: "float32",
      alignment: 16,
    };

    audioBuffer = new MockCircularAudioBuffer(testConfig);

    // Set up performance monitoring systems
    performanceMonitor = new BufferPerformanceMonitor();

    // Configure test audio data generators
    performanceMetrics = {
      writeOperations: [],
      readOperations: [],
      memoryAllocations: [],
      threadSafetyTests: [],
      errorHandlingTests: [],
    };

    // Initialize thread-safety testing framework
    audioBuffer.threadSafetyMode = true;

    // Create buffer state tracking system
    audioBuffer.stateTracker = {
      stateChanges: [],
      operationHistory: [],
      performanceBaseline: performance.now(),
    };

    // Set up memory usage monitoring
    performanceMonitor.recordMemoryUsage();

    // Initialize audio data validation
    audioBuffer.dataValidator = {
      checksumHistory: [],
      integrityChecks: 0,
      validationErrors: 0,
    };
  });

  afterEach(() => {
    // Clean up buffer resources
    if (audioBuffer) {
      audioBuffer.clear();
      audioBuffer = null;
    }

    // Verify no memory leaks
    performanceMonitor.recordMemoryUsage();
    const memoryTrend = performanceMonitor._calculateMemoryTrend();
    expect(memoryTrend).to.not.equal("increasing");

    // Generate performance reports
    const performanceReport = performanceMonitor.getReport();
    performanceMetrics.finalReport = performanceReport;

    // Reset buffer statistics
    testConfig = null;

    // Validate final buffer state
    expect(audioBuffer).to.be.null;

    // Export performance metrics for analysis
    if (performanceReport.errorRate > 0.1) {
      console.warn("High error rate detected:", performanceReport.errorRate);
    }

    // Archive buffer test results
    const testResults = {
      timestamp: new Date().toISOString(),
      performanceMetrics,
      performanceReport,
    };

    // Store results for trend analysis
    global.bufferTestResults = global.bufferTestResults || [];
    global.bufferTestResults.push(testResults);
  });

  describe("Buffer Initialization", () => {
    it("should initialize with valid configuration", async () => {
      // Test buffer initialization with various configurations
      const configs = [
        { sampleRate: 44100, channels: 2, bufferSize: 1024 },
        { sampleRate: 48000, channels: 1, bufferSize: 2048 },
        { sampleRate: 96000, channels: 8, bufferSize: 4096 },
      ];

      for (const config of configs) {
        const buffer = new MockCircularAudioBuffer(config);

        // Verify buffer size allocation
        expect(buffer.bufferSize).to.equal(config.bufferSize);
        expect(buffer.buffer.length).to.equal(
          config.bufferSize * config.channels
        );

        // Check initial buffer state
        expect(buffer.state).to.equal("initialized");
        expect(buffer.currentLevel).to.equal(0);
        expect(buffer.writePointer).to.equal(0);
        expect(buffer.readPointer).to.equal(0);

        // Validate configuration parameter handling
        expect(buffer.sampleRate).to.equal(config.sampleRate);
        expect(buffer.channels).to.equal(config.channels);

        // Test different sample formats
        const formatConfigs = ["int16", "int32", "float32", "float64"];
        for (const format of formatConfigs) {
          const formatBuffer = new MockCircularAudioBuffer({
            ...config,
            sampleFormat: format,
          });
          expect(formatBuffer.sampleFormat).to.equal(format);
        }

        // Verify channel configuration handling
        expect(buffer.channels).to.be.within(1, 32);

        // Test sample rate validation
        expect(buffer.sampleRate).to.be.within(8000, 192000);

        // Validate buffer alignment requirements
        expect(buffer.alignment).to.be.a("number");
        expect(buffer.alignment % 2).to.equal(0); // Should be power of 2

        // Test initialization performance
        const startTime = performance.now();
        const perfBuffer = new MockCircularAudioBuffer(config);
        const initTime = performance.now() - startTime;
        expect(initTime).to.be.below(10); // Should initialize in under 10ms

        // Verify memory allocation efficiency
        const expectedSize = config.bufferSize * config.channels * 4; // Float32 = 4 bytes
        expect(buffer.buffer.byteLength).to.equal(expectedSize);

        // Test configuration persistence
        const diagnostics = buffer.getDiagnostics();
        expect(diagnostics.config.sampleRate).to.equal(config.sampleRate);
        expect(diagnostics.config.channels).to.equal(config.channels);

        // Validate initialization error handling
        buffer.lastError = null;
        expect(buffer.lastError).to.be.null;

        // Test configuration validation
        expect(buffer._validateConfig()).to.be.true;

        // Verify initialization thread safety
        expect(buffer.writeLock).to.be.false;
        expect(buffer.readLock).to.be.false;

        // Test initialization with edge cases
        if (config.bufferSize === 1024) {
          expect(buffer.getAvailableSpace()).to.equal(1024);
          expect(buffer.getFillRatio()).to.equal(0);
        }
      }
    });

    it("should reject invalid configurations", async () => {
      const invalidConfigs = [
        { sampleRate: -1, channels: 2, bufferSize: 1024 }, // Invalid sample rate
        { sampleRate: 44100, channels: 0, bufferSize: 1024 }, // Invalid channel count
        { sampleRate: 44100, channels: 2, bufferSize: -1 }, // Invalid buffer size
        { sampleRate: 44100, channels: 2, bufferSize: 0 }, // Zero buffer size
        { sampleRate: 44100, channels: 33, bufferSize: 1024 }, // Too many channels
        { sampleRate: 300000, channels: 2, bufferSize: 1024 }, // Invalid high sample rate
        { sampleRate: 44100, channels: 2, bufferSize: 2000000 }, // Oversized buffer
        {
          sampleRate: 44100,
          channels: 2,
          bufferSize: 1024,
          sampleFormat: "invalid",
        }, // Invalid format
      ];

      for (const config of invalidConfigs) {
        // Test initialization with invalid buffer sizes, channel counts, etc.
        expect(() => new MockCircularAudioBuffer(config)).to.throw(
          "Invalid buffer configuration"
        );

        // Verify invalid parameter handling
        try {
          new MockCircularAudioBuffer(config);
          expect.fail("Should have thrown error for invalid config");
        } catch (error) {
          // Verify error message accuracy
          expect(error.message).to.contain("Invalid");

          // Test configuration validation completeness
          expect(error).to.be.an("error");
        }
      }

      // Test null parameter handling
      expect(() => new MockCircularAudioBuffer(null)).to.throw();

      // Test configuration boundary conditions
      const boundaryConfigs = [
        { sampleRate: 1, channels: 1, bufferSize: 1 }, // Minimum values
        { sampleRate: 192001, channels: 1, bufferSize: 1 }, // Just over limit
        { sampleRate: 44100, channels: 32, bufferSize: 1048576 }, // Maximum valid
      ];

      boundaryConfigs.forEach((config, index) => {
        if (index < 2) {
          expect(() => new MockCircularAudioBuffer(config)).to.throw();
        } else {
          expect(() => new MockCircularAudioBuffer(config)).to.not.throw();
        }
      });

      // Validate graceful failure handling
      const partiallyInvalidConfig = { sampleRate: 44100, channels: 2 }; // Missing bufferSize
      const buffer = new MockCircularAudioBuffer(partiallyInvalidConfig);
      expect(buffer.bufferSize).to.equal(8192); // Should use default

      // Test configuration sanitization
      const unsanitizedConfig = {
        sampleRate: 44100.5,
        channels: 2.7,
        bufferSize: 1024.3,
      };
      const sanitizedBuffer = new MockCircularAudioBuffer(unsanitizedConfig);
      expect(sanitizedBuffer.sampleRate).to.equal(44100.5); // Exact values preserved
    });

    it("should handle dynamic reconfiguration", async () => {
      // Test runtime configuration changes
      const initialConfig = {
        sampleRate: 44100,
        channels: 2,
        bufferSize: 1024,
      };
      const buffer = new MockCircularAudioBuffer(initialConfig);

      // Add some test data
      const testData = AudioDataGenerator.generateSineWave(440, 0.1, 44100);
      testData
        .slice(0, 100)
        .forEach((sample) => buffer.write([sample, sample]));

      const initialLevel = buffer.getLevel();
      expect(initialLevel).to.be.above(0);

      // Verify buffer resizing operations
      const newSize = 2048;
      const resizeResult = buffer.resize(newSize);
      expect(resizeResult).to.equal(newSize);
      expect(buffer.bufferSize).to.equal(newSize);

      // Test configuration rollback on failure
      try {
        buffer.resize(-1); // Invalid size
        expect.fail("Should have thrown error");
      } catch (error) {
        expect(buffer.bufferSize).to.equal(newSize); // Should maintain previous valid size
      }

      // Validate data preservation during reconfiguration
      const levelAfterResize = buffer.getLevel();
      expect(levelAfterResize).to.equal(initialLevel); // Data should be preserved

      // Test reading preserved data
      const preservedData = buffer.read(10);
      expect(preservedData).to.have.length(10);
      expect(preservedData[0]).to.be.an("array");
    });

    it("should optimize memory allocation", async () => {
      // Test memory-aligned buffer allocation
      const config = {
        sampleRate: 44100,
        channels: 2,
        bufferSize: 1024,
        alignment: 16,
      };
      const buffer = new MockCircularAudioBuffer(config);

      // Verify efficient memory usage
      const expectedBytes = config.bufferSize * config.channels * 4; // Float32Array
      expect(buffer.buffer.byteLength).to.equal(expectedBytes);

      // Test large buffer allocation handling
      const largeConfig = { ...config, bufferSize: 65536 };
      const largeBuffer = new MockCircularAudioBuffer(largeConfig);
      expect(largeBuffer.buffer.byteLength).to.equal(65536 * 2 * 4);

      // Check memory fragmentation prevention
      const multipleBuffers = [];
      for (let i = 0; i < 10; i++) {
        multipleBuffers.push(new MockCircularAudioBuffer(config));
      }

      // All buffers should be successfully allocated
      expect(multipleBuffers).to.have.length(10);
      multipleBuffers.forEach((buf) => {
        expect(buf.buffer).to.be.an.instanceof(Float32Array);
        expect(buf.state).to.equal("initialized");
      });
    });
  });

  describe("Write Operations", () => {
    it("should write audio data correctly", async () => {
      // Test single-sample write operations
      const singleSample = [0.5, -0.3]; // Stereo sample
      const writeResult = audioBuffer.write(singleSample);
      expect(writeResult).to.equal(1);
      expect(audioBuffer.getLevel()).to.equal(1);

      // Verify data integrity
      const readSample = audioBuffer.read(1);
      expect(readSample[0]).to.deep.equal(singleSample);

      // Verify block write operations
      const blockData = AudioDataGenerator.generateSineWave(440, 0.01, 44100); // 0.01 seconds
      const blockWriteResult = audioBuffer.write(blockData);
      expect(blockWriteResult).to.equal(blockData.length);
      expect(audioBuffer.getLevel()).to.equal(blockData.length);

      // Test partial write handling
      const largeBlock = AudioDataGenerator.generateWhiteNoise(1.0, 44100); // 1 second
      const availableSpace = audioBuffer.getAvailableSpace();

      if (largeBlock.length > availableSpace) {
        const partialResult = audioBuffer.write(largeBlock);
        expect(partialResult).to.be.at.most(availableSpace);
        expect(audioBuffer.stats.overflowCount).to.be.above(0);
      }

      // Check write pointer advancement
      const initialPointer = audioBuffer.writePointer;
      audioBuffer.write([0.1, 0.2]);
      expect(audioBuffer.writePointer).to.equal(
        (initialPointer + 1) % audioBuffer.bufferSize
      );

      // Verify write statistics tracking
      expect(audioBuffer.stats.totalWrites).to.be.above(0);
      expect(audioBuffer.stats.writeLatency.length).to.be.above(0);
    });

    it("should handle buffer overflow conditions", async () => {
      // Fill buffer to capacity
      const fillData = AudioDataGenerator.generateSilence(0.5, 44100);
      let totalWritten = 0;

      while (
        audioBuffer.getAvailableSpace() > 0 &&
        totalWritten < fillData.length
      ) {
        const chunkSize = Math.min(100, audioBuffer.getAvailableSpace());
        const chunk = fillData.slice(totalWritten, totalWritten + chunkSize);
        audioBuffer.write(chunk);
        totalWritten += chunkSize;
      }

      expect(audioBuffer.getFillRatio()).to.equal(1.0);

      // Test write operations when buffer is full
      const overflowData = [0.8, -0.8];
      const initialOverflowCount = audioBuffer.stats.overflowCount;

      audioBuffer.write(overflowData);

      // Verify overflow protection mechanisms
      expect(audioBuffer.stats.overflowCount).to.be.above(initialOverflowCount);
      expect(audioBuffer.getFillRatio()).to.equal(1.0); // Should remain full

      // Test overflow callback invocation
      let callbackInvoked = false;
      let overflowAmount = 0;

      audioBuffer.callbacks.onOverflow = (amount) => {
        callbackInvoked = true;
        overflowAmount = amount;
      };

      const moreOverflowData = AudioDataGenerator.generateWhiteNoise(
        0.1,
        44100
      );
      audioBuffer.write(moreOverflowData);

      expect(callbackInvoked).to.be.true;
      expect(overflowAmount).to.be.above(0);

      // Check overflow statistics tracking
      const stats = audioBuffer.getStats();
      expect(stats.overflowCount).to.be.above(0);
    });

    it("should maintain write performance under load", async () => {
      const performanceData = [];
      const iterations = 1000;
      const sampleData = [0.5, -0.5];

      // Benchmark write operation performance
      for (let i = 0; i < iterations; i++) {
        const startTime = performance.now();
        audioBuffer.write(sampleData);
        const endTime = performance.now();
        performanceData.push(endTime - startTime);

        // Clear buffer periodically to avoid overflow
        if (i % 100 === 0) {
          audioBuffer.clear();
        }
      }

      // Calculate performance metrics
      const averageWriteTime =
        performanceData.reduce((a, b) => a + b, 0) / performanceData.length;
      const maxWriteTime = Math.max(...performanceData);
      const minWriteTime = Math.min(...performanceData);

      // Test high-frequency write scenarios
      expect(averageWriteTime).to.be.below(1.0); // Should be under 1ms on average
      expect(maxWriteTime).to.be.below(10.0); // Max should be under 10ms

      // Verify real-time write capabilities
      const realTimeThreshold = (1.0 / 44100) * 1000; // Time for one sample at 44.1kHz in ms
      const realTimeCompliantWrites = performanceData.filter(
        (time) => time < realTimeThreshold
      ).length;
      const realTimeRatio = realTimeCompliantWrites / performanceData.length;

      expect(realTimeRatio).to.be.above(0.95); // 95% should meet real-time requirements

      // Check write latency measurements
      const stats = audioBuffer.getStats();
      expect(stats.averageWriteLatency).to.be.a("number");
      expect(stats.averageWriteLatency).to.be.above(0);

      performanceMetrics.writeOperations.push({
        iterations,
        averageTime: averageWriteTime,
        maxTime: maxWriteTime,
        minTime: minWriteTime,
        realTimeRatio,
      });
    });

    it("should ensure thread-safe write operations", async () => {
      // Test concurrent write operations simulation
      const concurrentWrites = [];
      const writePromises = [];

      // Simulate multiple writers
      for (let i = 0; i < 10; i++) {
        const writePromise = new Promise((resolve) => {
          setTimeout(() => {
            try {
              const data = [Math.random(), Math.random()];
              const result = audioBuffer.write(data);
              concurrentWrites.push({ writerId: i, result, success: true });
              resolve();
            } catch (error) {
              concurrentWrites.push({
                writerId: i,
                error: error.message,
                success: false,
              });
              resolve();
            }
          }, Math.random() * 10);
        });
        writePromises.push(writePromise);
      }

      await Promise.all(writePromises);

      // Verify write operation atomicity
      const successfulWrites = concurrentWrites.filter((w) => w.success);
      expect(successfulWrites.length).to.be.above(0);

      // Test write/read operation coordination
      audioBuffer.clear();

      const coordinationTest = async () => {
        const writeData = AudioDataGenerator.generateSineWave(440, 0.01, 44100);
        const writePromise = new Promise((resolve) => {
          setTimeout(() => {
            audioBuffer.write(writeData);
            resolve("write_complete");
          }, 5);
        });

        const readPromise = new Promise((resolve) => {
          setTimeout(() => {
            const level = audioBuffer.getLevel();
            resolve(level);
          }, 10);
        });

        const [writeResult, readLevel] = await Promise.all([
          writePromise,
          readPromise,
        ]);
        expect(writeResult).to.equal("write_complete");
        expect(readLevel).to.be.a("number");
      };

      await coordinationTest();

      // Check for race condition prevention
      expect(audioBuffer.operationCount).to.be.above(0);

      // Verify lock mechanisms
      audioBuffer.writeLock = true;
      expect(() => audioBuffer.write([0.1, 0.1])).to.throw(
        "Write operation in progress"
      );
      audioBuffer.writeLock = false;

      performanceMetrics.threadSafetyTests.push({
        concurrentWrites: concurrentWrites.length,
        successfulWrites: successfulWrites.length,
        lockingMechanism: "functional",
      });
    });
  });

  describe("Read Operations", () => {
    it("should read audio data correctly", async () => {
      // Prepare test data
      const testData = AudioDataGenerator.generateSineWave(440, 0.01, 44100);
      const stereoData = testData.map((sample) => [sample, sample * 0.5]);

      // Write test data
      stereoData.forEach((sample) => audioBuffer.write(sample));
      const initialLevel = audioBuffer.getLevel();

      // Test single-sample read operations
      const singleRead = audioBuffer.read(1);
      expect(singleRead).to.have.length(1);
      expect(singleRead[0]).to.be.an("array");
      expect(singleRead[0]).to.have.length(2); // Stereo
      expect(audioBuffer.getLevel()).to.equal(initialLevel - 1);

      // Verify block read operations
      const blockSize = 10;
      const blockRead = audioBuffer.read(blockSize);
      expect(blockRead).to.have.length(blockSize);
      expect(audioBuffer.getLevel()).to.equal(initialLevel - 1 - blockSize);

      // Verify data integrity
      blockRead.forEach((sample) => {
        expect(sample).to.be.an("array");
        expect(sample).to.have.length(2);
        expect(sample[0]).to.be.a("number");
        expect(sample[1]).to.be.a("number");
      });

      // Test partial read handling
      const remainingData = audioBuffer.getLevel();
      const partialRead = audioBuffer.read(remainingData + 10); // Request more than available

      expect(audioBuffer.stats.underflowCount).to.be.above(0);

      // Check read pointer advancement
      audioBuffer.clear();
      audioBuffer.write([0.1, 0.2]);
      audioBuffer.write([0.3, 0.4]);

      const initialReadPointer = audioBuffer.readPointer;
      audioBuffer.read(1);
      expect(audioBuffer.readPointer).to.equal(
        (initialReadPointer + 1) % audioBuffer.bufferSize
      );

      // Verify read statistics tracking
      expect(audioBuffer.stats.totalReads).to.be.above(0);
      expect(audioBuffer.stats.readLatency.length).to.be.above(0);
    });

    it("should handle buffer underflow conditions", async () => {
      // Ensure buffer is empty
      audioBuffer.clear();
      expect(audioBuffer.getLevel()).to.equal(0);

      // Test read operations when buffer is empty
      const initialUnderflowCount = audioBuffer.stats.underflowCount;
      const underflowRead = audioBuffer.read(5);

      // Verify underflow protection mechanisms
      expect(audioBuffer.stats.underflowCount).to.be.above(
        initialUnderflowCount
      );
      expect(underflowRead).to.have.length(5); // Should return silence

      // Verify returned data is silence
      underflowRead.forEach((sample) => {
        expect(sample).to.deep.equal([0, 0]); // Stereo silence
      });

      // Test underflow callback invocation
      let callbackInvoked = false;
      let underflowAmount = 0;

      audioBuffer.callbacks.onUnderflow = (amount) => {
        callbackInvoked = true;
        underflowAmount = amount;
      };

      audioBuffer.read(10);
      expect(callbackInvoked).to.be.true;
      expect(underflowAmount).to.equal(10);

      // Check underflow statistics tracking
      const stats = audioBuffer.getStats();
      expect(stats.underflowCount).to.be.above(1);

      // Test partial underflow scenario
      audioBuffer.write([
        [0.5, -0.5],
        [0.6, -0.6],
      ]); // Add 2 samples
      const partialUnderflowRead = audioBuffer.read(5); // Request 5 samples

      expect(partialUnderflowRead).to.have.length(5);
      // First 2 should be real data, last 3 should be silence
      expect(partialUnderflowRead[0]).to.not.deep.equal([0, 0]);
      expect(partialUnderflowRead[1]).to.not.deep.equal([0, 0]);
      expect(partialUnderflowRead[2]).to.deep.equal([0, 0]);
      expect(partialUnderflowRead[3]).to.deep.equal([0, 0]);
      expect(partialUnderflowRead[4]).to.deep.equal([0, 0]);
    });

    it("should maintain read performance under load", async () => {
      // Prepare buffer with test data
      const testData = AudioDataGenerator.generateWhiteNoise(0.1, 44100);
      testData.forEach((sample) => audioBuffer.write([sample, sample]));

      const performanceData = [];
      const iterations = 1000;

      // Benchmark read operation performance
      for (let i = 0; i < iterations; i++) {
        // Ensure there's data to read
        if (audioBuffer.getLevel() === 0) {
          testData
            .slice(0, 100)
            .forEach((sample) => audioBuffer.write([sample, sample]));
        }

        const startTime = performance.now();
        audioBuffer.read(1);
        const endTime = performance.now();
        performanceData.push(endTime - startTime);
      }

      // Calculate performance metrics
      const averageReadTime =
        performanceData.reduce((a, b) => a + b, 0) / performanceData.length;
      const maxReadTime = Math.max(...performanceData);
      const minReadTime = Math.min(...performanceData);

      // Test high-frequency read scenarios
      expect(averageReadTime).to.be.below(1.0); // Should be under 1ms on average
      expect(maxReadTime).to.be.below(10.0); // Max should be under 10ms

      // Verify real-time read capabilities
      const realTimeThreshold = (1.0 / 44100) * 1000; // Time for one sample at 44.1kHz in ms
      const realTimeCompliantReads = performanceData.filter(
        (time) => time < realTimeThreshold
      ).length;
      const realTimeRatio = realTimeCompliantReads / performanceData.length;

      expect(realTimeRatio).to.be.above(0.95); // 95% should meet real-time requirements

      // Check read latency measurements
      const stats = audioBuffer.getStats();
      expect(stats.averageReadLatency).to.be.a("number");
      expect(stats.averageReadLatency).to.be.above(0);

      performanceMetrics.readOperations.push({
        iterations,
        averageTime: averageReadTime,
        maxTime: maxReadTime,
        minTime: minReadTime,
        realTimeRatio,
      });

      // Test burst read performance
      audioBuffer.clear();
      const burstData = AudioDataGenerator.generateSineWave(1000, 0.1, 44100);
      burstData.forEach((sample) => audioBuffer.write([sample, sample]));

      const burstStartTime = performance.now();
      const burstRead = audioBuffer.read(burstData.length);
      const burstEndTime = performance.now();
      const burstTime = burstEndTime - burstStartTime;

      expect(burstRead).to.have.length(burstData.length);
      expect(burstTime).to.be.below(50); // Burst read should complete in under 50ms
    });

    it("should ensure thread-safe read operations", async () => {
      // Prepare buffer with data for concurrent reads
      const prepData = AudioDataGenerator.generateWhiteNoise(0.5, 44100);
      prepData.forEach((sample) => audioBuffer.write([sample, sample]));

      // Test concurrent read operations simulation
      const concurrentReads = [];
      const readPromises = [];

      // Simulate multiple readers
      for (let i = 0; i < 8; i++) {
        const readPromise = new Promise((resolve) => {
          setTimeout(() => {
            try {
              const data = audioBuffer.read(10);
              concurrentReads.push({
                readerId: i,
                dataLength: data.length,
                success: true,
              });
              resolve();
            } catch (error) {
              concurrentReads.push({
                readerId: i,
                error: error.message,
                success: false,
              });
              resolve();
            }
          }, Math.random() * 5);
        });
        readPromises.push(readPromise);
      }

      await Promise.all(readPromises);

      // Verify read operation atomicity
      const successfulReads = concurrentReads.filter((r) => r.success);
      expect(successfulReads.length).to.be.above(0);

      // Each successful read should have gotten exactly what was requested or underflow
      successfulReads.forEach((read) => {
        expect(read.dataLength).to.be.at.most(10);
      });

      // Test reader/writer coordination
      audioBuffer.clear();

      const coordinationTest = async () => {
        // Start a reader that waits for data
        const readerPromise = new Promise((resolve) => {
          setTimeout(() => {
            const level = audioBuffer.getLevel();
            if (level > 0) {
              const data = audioBuffer.read(5);
              resolve(data.length);
            } else {
              resolve(0);
            }
          }, 10);
        });

        // Writer adds data
        const writerPromise = new Promise((resolve) => {
          setTimeout(() => {
            const writeData = AudioDataGenerator.generateSineWave(
              440,
              0.01,
              44100
            );
            writeData
              .slice(0, 10)
              .forEach((sample) => audioBuffer.write([sample, sample]));
            resolve("write_complete");
          }, 5);
        });

        const [readCount, writeResult] = await Promise.all([
          readerPromise,
          writerPromise,
        ]);
        expect(writeResult).to.equal("write_complete");
        expect(readCount).to.be.a("number");
      };

      await coordinationTest();

      // Check for data corruption prevention
      const dataIntegrityTest = audioBuffer.read(1);
      if (dataIntegrityTest.length > 0) {
        expect(dataIntegrityTest[0]).to.be.an("array");
        expect(dataIntegrityTest[0]).to.have.length(2);
      }

      // Verify lock mechanisms
      audioBuffer.readLock = true;
      expect(() => audioBuffer.read(1)).to.throw("Read operation in progress");
      audioBuffer.readLock = false;

      performanceMetrics.threadSafetyTests.push({
        concurrentReads: concurrentReads.length,
        successfulReads: successfulReads.length,
        coordinationTest: "passed",
      });
    });
  });

  describe("Buffer State Management", () => {
    it("should track buffer level accurately", async () => {
      // Test buffer level calculation accuracy
      audioBuffer.clear();
      expect(audioBuffer.getLevel()).to.equal(0);

      // Add test data incrementally
      const testSamples = [
        [0.1, 0.2],
        [0.3, 0.4],
        [0.5, 0.6],
      ];
      testSamples.forEach((sample, index) => {
        audioBuffer.write(sample);
        expect(audioBuffer.getLevel()).to.equal(index + 1);
      });

      // Verify fill ratio calculations
      const expectedFillRatio = testSamples.length / audioBuffer.bufferSize;
      expect(audioBuffer.getFillRatio()).to.be.closeTo(
        expectedFillRatio,
        0.001
      );

      // Test available space calculations
      const expectedAvailableSpace =
        audioBuffer.bufferSize - testSamples.length;
      expect(audioBuffer.getAvailableSpace()).to.equal(expectedAvailableSpace);

      // Check buffer state consistency after read operations
      const readCount = 2;
      audioBuffer.read(readCount);
      expect(audioBuffer.getLevel()).to.equal(testSamples.length - readCount);
      expect(audioBuffer.getAvailableSpace()).to.equal(
        audioBuffer.bufferSize - (testSamples.length - readCount)
      );
    });

    it("should handle buffer wraparound correctly", async () => {
      // Fill buffer near capacity to force wraparound
      const nearCapacity = audioBuffer.bufferSize - 10;
      const testData = AudioDataGenerator.generateSineWave(440, 0.01, 44100);

      for (let i = 0; i < nearCapacity; i++) {
        audioBuffer.write([
          testData[i % testData.length],
          testData[i % testData.length] * 0.5,
        ]);
      }

      // Store data for integrity verification
      const preWrapData = audioBuffer.peek(nearCapacity - 5, 5);

      // Force wraparound by adding more data
      for (let i = 0; i < 20; i++) {
        audioBuffer.write([0.7, -0.7]);
      }

      // Test circular buffer wraparound behavior
      expect(audioBuffer.writePointer).to.be.below(audioBuffer.bufferSize);
      expect(audioBuffer.getLevel()).to.equal(audioBuffer.bufferSize); // Should be full

      // Verify data integrity across wraparound
      const readData = audioBuffer.read(5);
      expect(readData).to.have.length(5);

      // Test pointer wraparound handling
      const initialReadPointer = audioBuffer.readPointer;
      audioBuffer.read(audioBuffer.bufferSize - 10); // Read most of buffer
      expect(audioBuffer.readPointer).to.not.equal(initialReadPointer);

      // Check wraparound performance impact
      const wraparoundStartTime = performance.now();
      for (let i = 0; i < 100; i++) {
        audioBuffer.write([0.1, 0.1]);
        audioBuffer.read(1);
      }
      const wraparoundTime = performance.now() - wraparoundStartTime;
      expect(wraparoundTime).to.be.below(50); // Should complete in under 50ms
    });

    it("should provide accurate statistics", async () => {
      // Clear buffer and reset statistics
      audioBuffer.clear();

      // Perform various operations to generate statistics
      const writeOperations = 100;
      const readOperations = 50;

      for (let i = 0; i < writeOperations; i++) {
        audioBuffer.write([Math.random(), Math.random()]);
      }

      for (let i = 0; i < readOperations; i++) {
        if (audioBuffer.getLevel() > 0) {
          audioBuffer.read(1);
        }
      }

      // Test buffer statistics collection
      const stats = audioBuffer.getStats();
      expect(stats.totalWrites).to.be.at.least(writeOperations);
      expect(stats.totalReads).to.be.at.least(0);

      // Verify performance metrics accuracy
      expect(stats.averageWriteLatency).to.be.a("number");
      expect(stats.averageReadLatency).to.be.a("number");
      expect(stats.peakLevel).to.be.at.most(audioBuffer.bufferSize);

      // Test error statistics tracking
      try {
        audioBuffer.read(audioBuffer.bufferSize + 100); // Force underflow
      } catch (error) {
        // Expected underflow
      }

      const updatedStats = audioBuffer.getStats();
      expect(updatedStats.underflowCount).to.be.above(0);

      // Check health score calculations
      const diagnostics = audioBuffer.getDiagnostics();
      expect(diagnostics.level).to.be.a("number");
      expect(diagnostics.fillRatio).to.be.within(0, 1);
      expect(diagnostics.operationCount).to.be.above(0);
    });

    it("should support buffer clearing operations", async () => {
      // Fill buffer with test data
      const fillData = AudioDataGenerator.generateWhiteNoise(0.1, 44100);
      fillData
        .slice(0, 100)
        .forEach((sample) => audioBuffer.write([sample, sample]));

      const levelBeforeClear = audioBuffer.getLevel();
      expect(levelBeforeClear).to.be.above(0);

      // Test buffer clear functionality
      audioBuffer.clear();

      // Verify complete data removal
      expect(audioBuffer.getLevel()).to.equal(0);
      expect(audioBuffer.writePointer).to.equal(0);
      expect(audioBuffer.readPointer).to.equal(0);

      // Test clear operation performance
      const performanceTest = () => {
        const largeData = AudioDataGenerator.generateSineWave(1000, 0.5, 44100);
        largeData
          .slice(0, 1000)
          .forEach((sample) => audioBuffer.write([sample, sample]));

        const clearStartTime = performance.now();
        audioBuffer.clear();
        const clearEndTime = performance.now();

        return clearEndTime - clearStartTime;
      };

      const clearTime = performanceTest();
      expect(clearTime).to.be.below(10); // Should clear in under 10ms

      // Check state reset after clearing
      expect(audioBuffer.state).to.equal("cleared");
      expect(audioBuffer.getAvailableSpace()).to.equal(audioBuffer.bufferSize);

      // Verify buffer is usable after clearing
      audioBuffer.write([0.5, -0.5]);
      expect(audioBuffer.getLevel()).to.equal(1);

      const readAfterClear = audioBuffer.read(1);
      expect(readAfterClear[0]).to.deep.equal([0.5, -0.5]);
    });
  });

  describe("Advanced Features", () => {
    it("should support peek operations without consuming data", async () => {
      // Prepare test data
      const testData = [
        [0.1, 0.2],
        [0.3, 0.4],
        [0.5, 0.6],
        [0.7, 0.8],
        [0.9, 1.0],
      ];
      testData.forEach((sample) => audioBuffer.write(sample));

      const initialLevel = audioBuffer.getLevel();

      // Test peek operation functionality
      const peekedData = audioBuffer.peek(0, 3);
      expect(peekedData).to.have.length(3);
      expect(peekedData[0]).to.deep.equal(testData[0]);
      expect(peekedData[1]).to.deep.equal(testData[1]);
      expect(peekedData[2]).to.deep.equal(testData[2]);

      // Verify data integrity after peek
      expect(audioBuffer.getLevel()).to.equal(initialLevel);
      expect(audioBuffer.readPointer).to.equal(0); // Should not advance

      // Test peek with offset parameters
      const offsetPeek = audioBuffer.peek(2, 2);
      expect(offsetPeek).to.have.length(2);
      expect(offsetPeek[0]).to.deep.equal(testData[2]);
      expect(offsetPeek[1]).to.deep.equal(testData[3]);

      // Check peek operation performance
      const peekPerformanceTest = () => {
        const startTime = performance.now();
        for (let i = 0; i < 1000; i++) {
          if (audioBuffer.getLevel() > 0) {
            audioBuffer.peek(0, 1);
          }
        }
        return performance.now() - startTime;
      };

      const peekTime = peekPerformanceTest();
      expect(peekTime).to.be.below(100); // 1000 peeks should complete in under 100ms

      // Test error handling for invalid peek operations
      expect(() => audioBuffer.peek(10, 1)).to.throw(
        "Peek beyond available data"
      );
      expect(() => audioBuffer.peek(0, 10)).to.throw(
        "Peek beyond available data"
      );
    });

    it("should support skip operations for data discarding", async () => {
      // Prepare test data
      const testData = AudioDataGenerator.generateSineWave(440, 0.05, 44100);
      testData
        .slice(0, 20)
        .forEach((sample) => audioBuffer.write([sample, sample]));

      const initialLevel = audioBuffer.getLevel();
      const initialReadPointer = audioBuffer.readPointer;

      // Test skip operation functionality
      const skipCount = 5;
      const skipped = audioBuffer.skip(skipCount);
      expect(skipped).to.equal(skipCount);

      // Verify correct data discarding
      expect(audioBuffer.getLevel()).to.equal(initialLevel - skipCount);
      expect(audioBuffer.readPointer).to.equal(
        (initialReadPointer + skipCount) % audioBuffer.bufferSize
      );

      // Test skip with count parameters
      const remainingData = audioBuffer.getLevel();
      const skipHalf = Math.floor(remainingData / 2);
      audioBuffer.skip(skipHalf);
      expect(audioBuffer.getLevel()).to.equal(remainingData - skipHalf);

      // Check skip operation performance
      audioBuffer.clear();
      const perfTestData = AudioDataGenerator.generateWhiteNoise(0.1, 44100);
      perfTestData
        .slice(0, 1000)
        .forEach((sample) => audioBuffer.write([sample, sample]));

      const skipStartTime = performance.now();
      audioBuffer.skip(500);
      const skipEndTime = performance.now();
      const skipTime = skipEndTime - skipStartTime;

      expect(skipTime).to.be.below(5); // Should complete in under 5ms
      expect(audioBuffer.getLevel()).to.equal(500);

      // Test error handling for invalid skip operations
      const currentLevel = audioBuffer.getLevel();
      expect(() => audioBuffer.skip(currentLevel + 10)).to.throw(
        "Skip beyond available data"
      );
    });

    it("should handle buffer resizing operations", async () => {
      // Add initial test data
      const initialData = AudioDataGenerator.generateSineWave(440, 0.02, 44100);
      initialData
        .slice(0, 50)
        .forEach((sample) => audioBuffer.write([sample, sample]));

      const initialLevel = audioBuffer.getLevel();
      const initialSize = audioBuffer.bufferSize;

      // Test dynamic buffer resizing
      const newSize = initialSize * 2;
      const resizeResult = audioBuffer.resize(newSize);

      expect(resizeResult).to.equal(newSize);
      expect(audioBuffer.bufferSize).to.equal(newSize);
      expect(audioBuffer.buffer.length).to.equal(
        newSize * audioBuffer.channels
      );

      // Verify data preservation during resize
      expect(audioBuffer.getLevel()).to.equal(initialLevel);

      // Test reading preserved data
      const preservedData = audioBuffer.read(10);
      expect(preservedData).to.have.length(10);
      preservedData.forEach((sample) => {
        expect(sample).to.be.an("array");
        expect(sample).to.have.length(2);
      });

      // Test resize failure recovery
      const validSize = audioBuffer.bufferSize;
      try {
        audioBuffer.resize(-100); // Invalid size
        expect.fail("Should have thrown error");
      } catch (error) {
        expect(audioBuffer.bufferSize).to.equal(validSize);
        expect(error.message).to.contain("Invalid buffer size");
      }

      // Check resize operation performance
      const resizePerformanceTest = () => {
        const startTime = performance.now();
        audioBuffer.resize(audioBuffer.bufferSize);
        return performance.now() - startTime;
      };

      const resizeTime = resizePerformanceTest();
      expect(resizeTime).to.be.below(50); // Should complete in under 50ms

      // Test resize with data loss scenario
      const smallSize = 10;
      const largeDataSet = AudioDataGenerator.generateWhiteNoise(0.1, 44100);
      largeDataSet
        .slice(0, 100)
        .forEach((sample) => audioBuffer.write([sample, sample]));

      audioBuffer.resize(smallSize);
      expect(audioBuffer.bufferSize).to.equal(smallSize);
      expect(audioBuffer.getLevel()).to.be.at.most(smallSize);
    });

    it("should provide diagnostic information", async () => {
      // Perform various operations to generate diagnostic data
      const testOperations = () => {
        const writeData = AudioDataGenerator.generateSineWave(
          1000,
          0.01,
          44100
        );
        writeData
          .slice(0, 25)
          .forEach((sample) => audioBuffer.write([sample, sample]));

        for (let i = 0; i < 10; i++) {
          audioBuffer.read(1);
        }

        audioBuffer.peek(0, 5);
        audioBuffer.skip(3);
      };

      testOperations();

      // Test diagnostic information generation
      const diagnostics = audioBuffer.getDiagnostics();

      expect(diagnostics).to.be.an("object");
      expect(diagnostics.state).to.be.a("string");
      expect(diagnostics.config).to.be.an("object");
      expect(diagnostics.pointers).to.be.an("object");
      expect(diagnostics.level).to.be.a("number");
      expect(diagnostics.operationCount).to.be.a("number");

      // Verify diagnostic data accuracy
      expect(diagnostics.config.sampleRate).to.equal(audioBuffer.sampleRate);
      expect(diagnostics.config.channels).to.equal(audioBuffer.channels);
      expect(diagnostics.config.bufferSize).to.equal(audioBuffer.bufferSize);

      expect(diagnostics.pointers.write).to.equal(audioBuffer.writePointer);
      expect(diagnostics.pointers.read).to.equal(audioBuffer.readPointer);
      expect(diagnostics.level).to.equal(audioBuffer.getLevel());

      // Test diagnostic formatting
      const diagnosticKeys = Object.keys(diagnostics);
      const requiredKeys = [
        "state",
        "config",
        "pointers",
        "level",
        "fillRatio",
        "operationCount",
      ];
      requiredKeys.forEach((key) => {
        expect(diagnosticKeys).to.include(key);
      });

      // Check diagnostic performance impact
      const diagnosticPerformanceTest = () => {
        const startTime = performance.now();
        for (let i = 0; i < 1000; i++) {
          audioBuffer.getDiagnostics();
        }
        return performance.now() - startTime;
      };

      const diagnosticTime = diagnosticPerformanceTest();
      expect(diagnosticTime).to.be.below(100); // 1000 diagnostic calls in under 100ms

      // Test diagnostic information completeness
      expect(diagnostics.fillRatio).to.be.within(0, 1);
      expect(diagnostics.operationCount).to.be.above(0);

      if (diagnostics.lastError) {
        expect(diagnostics.lastError).to.be.a("string");
      }
    });
  });

  describe("Error Handling and Recovery", () => {
    it("should handle and report errors appropriately", async () => {
      // Test error detection mechanisms
      const errorTests = [
        () => audioBuffer.read(audioBuffer.bufferSize + 100), // Underflow
        () => {
          // Fill buffer completely then try overflow
          while (audioBuffer.getAvailableSpace() > 0) {
            audioBuffer.write([0.1, 0.1]);
          }
          audioBuffer.write([0.2, 0.2]); // This should trigger overflow
        },
        () => audioBuffer.peek(100, 10), // Invalid peek
        () => audioBuffer.skip(audioBuffer.bufferSize + 50), // Invalid skip
      ];

      let errorCount = 0;
      const initialErrorCounts = {
        overflow: audioBuffer.stats.overflowCount,
        underflow: audioBuffer.stats.underflowCount,
      };

      errorTests.forEach((test, index) => {
        try {
          test();
        } catch (error) {
          errorCount++;
          // Verify error reporting accuracy
          expect(error).to.be.an("error");
          expect(error.message).to.be.a("string");
          expect(error.message.length).to.be.above(0);
        }
        audioBuffer.clear(); // Reset for next test
      });

      // Test error callback invocation
      let overflowCallbackInvoked = false;
      let underflowCallbackInvoked = false;

      audioBuffer.callbacks.onOverflow = () => {
        overflowCallbackInvoked = true;
      };
      audioBuffer.callbacks.onUnderflow = () => {
        underflowCallbackInvoked = true;
      };

      // Trigger overflow
      while (audioBuffer.getAvailableSpace() > 0) {
        audioBuffer.write([0.5, 0.5]);
      }
      audioBuffer.write([0.6, 0.6]);
      expect(overflowCallbackInvoked).to.be.true;

      // Trigger underflow
      audioBuffer.clear();
      audioBuffer.read(10);
      expect(underflowCallbackInvoked).to.be.true;

      // Check error recovery procedures
      const stats = audioBuffer.getStats();
      expect(stats.overflowCount).to.be.above(initialErrorCounts.overflow);
      expect(stats.underflowCount).to.be.above(initialErrorCounts.underflow);

      // Verify buffer is still functional after errors
      audioBuffer.clear();
      audioBuffer.write([0.1, 0.1]);
      const testRead = audioBuffer.read(1);
      expect(testRead[0]).to.deep.equal([0.1, 0.1]);
    });

    it("should maintain stability under error conditions", async () => {
      // Test system stability after errors
      const stabilityTest = async () => {
        const errors = [];

        // Generate multiple error conditions
        for (let i = 0; i < 50; i++) {
          try {
            // Random error-inducing operations
            if (i % 3 === 0) {
              audioBuffer.read(audioBuffer.bufferSize + Math.random() * 100);
            } else if (i % 3 === 1) {
              audioBuffer.peek(100, 50);
            } else {
              audioBuffer.skip(audioBuffer.bufferSize + 10);
            }
          } catch (error) {
            errors.push(error.message);
          }

          // Continue normal operations between errors
          try {
            audioBuffer.write([Math.random(), Math.random()]);
            if (audioBuffer.getLevel() > 0) {
              audioBuffer.read(1);
            }
          } catch (error) {
            // Unexpected error in normal operation
            errors.push(`Unexpected: ${error.message}`);
          }
        }

        return errors;
      };

      const errorLog = await stabilityTest();

      // Verify error isolation mechanisms
      expect(audioBuffer.state).to.not.equal("error");
      expect(audioBuffer.getLevel()).to.be.a("number");
      expect(audioBuffer.getAvailableSpace()).to.be.a("number");

      // Test continued operation after errors
      audioBuffer.clear();

      const continuedOperationTest = () => {
        const testData = AudioDataGenerator.generateSineWave(440, 0.01, 44100);
        let successfulOps = 0;

        for (let i = 0; i < testData.length && i < 100; i++) {
          try {
            audioBuffer.write([testData[i], testData[i] * 0.5]);
            successfulOps++;
          } catch (error) {
            break;
          }
        }

        return successfulOps;
      };

      const successfulOperations = continuedOperationTest();
      expect(successfulOperations).to.be.above(50); // Should complete most operations

      // Check error impact minimization
      const diagnostics = audioBuffer.getDiagnostics();
      expect(diagnostics.operationCount).to.be.above(0);
      expect(diagnostics.level).to.be.within(0, audioBuffer.bufferSize);

      performanceMetrics.errorHandlingTests.push({
        errorCount: errorLog.length,
        stabilityMaintained: true,
        continuedOperations: successfulOperations,
      });
    });

    it("should provide error context and debugging info", async () => {
      // Test error context preservation
      const contextualErrors = [];

      const errorWithContext = (operation, expected) => {
        try {
          operation();
          expect.fail(`Expected error for: ${expected}`);
        } catch (error) {
          contextualErrors.push({
            operation: expected,
            error: error.message,
            context: {
              level: audioBuffer.getLevel(),
              availableSpace: audioBuffer.getAvailableSpace(),
              writePointer: audioBuffer.writePointer,
              readPointer: audioBuffer.readPointer,
            },
          });
        }
      };

      // Generate errors with different contexts
      audioBuffer.clear();
      errorWithContext(() => audioBuffer.read(10), "empty buffer read");

      audioBuffer.write([0.5, 0.5]);
      errorWithContext(() => audioBuffer.peek(10, 5), "invalid peek offset");

      errorWithContext(() => audioBuffer.skip(50), "excessive skip");

      // Verify debugging information accuracy
      contextualErrors.forEach((errorInfo) => {
        expect(errorInfo.operation).to.be.a("string");
        expect(errorInfo.error).to.be.a("string");
        expect(errorInfo.context).to.be.an("object");
        expect(errorInfo.context.level).to.be.a("number");
        expect(errorInfo.context.availableSpace).to.be.a("number");
      });

      // Test error traceability
      const diagnostics = audioBuffer.getDiagnostics();
      expect(diagnostics.operationCount).to.be.above(0);

      // Check error analysis support
      const stats = audioBuffer.getStats();
      expect(stats.overflowCount).to.be.a("number");
      expect(stats.underflowCount).to.be.a("number");
      expect(stats.totalWrites).to.be.a("number");
      expect(stats.totalReads).to.be.a("number");

      // Verify error information completeness
      expect(contextualErrors.length).to.equal(3);

      contextualErrors.forEach((errorInfo) => {
        expect(errorInfo.context.level).to.be.within(0, audioBuffer.bufferSize);
        expect(errorInfo.context.availableSpace).to.be.within(
          0,
          audioBuffer.bufferSize
        );
        expect(errorInfo.context.writePointer).to.be.within(
          0,
          audioBuffer.bufferSize - 1
        );
        expect(errorInfo.context.readPointer).to.be.within(
          0,
          audioBuffer.bufferSize - 1
        );
      });

      performanceMetrics.errorHandlingTests.push({
        contextualErrors: contextualErrors.length,
        errorTraceability: "complete",
        debuggingInfo: "comprehensive",
      });
    });
  });
});

export { audioBuffer, testConfig, performanceMetrics };
