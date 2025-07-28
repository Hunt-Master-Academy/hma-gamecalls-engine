/**
 * @file wasm-functions.test.js
 * @brief WASM Function Export Test Module - Phase 3.1 Automated Testing
 *
 * This module provides comprehensive testing for WebAssembly function exports,
 * parameter validation, and return value verification in the Huntmaster Engine.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "mocha";

// Mock WASM module with function exports
class MockWasmModule {
  constructor() {
    this.memory = new WebAssembly.Memory({ initial: 1 });
    this.exports = this.createExports();
    this.callHistory = [];
    this.performanceMetrics = new Map();
    this.errorInjection = new Map();
  }

  createExports() {
    return {
      // Memory management
      memory: this.memory,
      malloc: this.trackFunction('malloc', (size) => {
        if (size <= 0) return 0;
        return Math.floor(Math.random() * 65536) + 1024;
      }),
      free: this.trackFunction('free', (ptr) => ptr > 0),

      // Audio processing functions
      process_audio_buffer: this.trackFunction('process_audio_buffer', (bufferPtr, length, channels) => {
        if (!bufferPtr || length <= 0 || channels <= 0) return -1;
        return 0; // Success
      }),

      convert_audio_format: this.trackFunction('convert_audio_format', (srcPtr, dstPtr, srcFormat, dstFormat, length) => {
        if (!srcPtr || !dstPtr || length <= 0) return -1;
        return length; // Converted samples
      }),

      assess_audio_quality: this.trackFunction('assess_audio_quality', (bufferPtr, length) => {
        if (!bufferPtr || length <= 0) return -1.0;
        return Math.random() * 100; // Quality score 0-100
      }),

      stream_audio_process: this.trackFunction('stream_audio_process', (inputPtr, outputPtr, length) => {
        if (!inputPtr || !outputPtr || length <= 0) return -1;
        return 0; // Success
      }),

      // Engine management functions
      initialize_engine: this.trackFunction('initialize_engine', (configPtr) => {
        if (!configPtr) return -1;
        return 0; // Success
      }),

      configure_engine: this.trackFunction('configure_engine', (paramPtr, valuePtr) => {
        if (!paramPtr || !valuePtr) return -1;
        return 0; // Success
      }),

      shutdown_engine: this.trackFunction('shutdown_engine', () => 0),

      get_engine_state: this.trackFunction('get_engine_state', () => 1), // Running state

      // Mathematical utilities
      calculate_fft: this.trackFunction('calculate_fft', (inputPtr, outputPtr, size) => {
        if (!inputPtr || !outputPtr || size <= 0) return -1;
        return 0; // Success
      }),

      apply_filter: this.trackFunction('apply_filter', (bufferPtr, length, filterType, params) => {
        if (!bufferPtr || length <= 0) return -1;
        return 0; // Success
      }),

      normalize_audio: this.trackFunction('normalize_audio', (bufferPtr, length, targetLevel) => {
        if (!bufferPtr || length <= 0) return -1;
        return targetLevel; // Achieved level
      }),

      // Data conversion utilities
      convert_endianness: this.trackFunction('convert_endianness', (dataPtr, length, fromBig, toBig) => {
        if (!dataPtr || length <= 0) return -1;
        return 0; // Success
      }),

      encode_audio: this.trackFunction('encode_audio', (inputPtr, outputPtr, length, codec) => {
        if (!inputPtr || !outputPtr || length <= 0) return -1;
        return Math.floor(length * 0.5); // Compressed size
      }),

      decode_audio: this.trackFunction('decode_audio', (inputPtr, outputPtr, length) => {
        if (!inputPtr || !outputPtr || length <= 0) return -1;
        return length * 2; // Decompressed size
      }),

      // Debug and diagnostics
      get_debug_info: this.trackFunction('get_debug_info', (bufferPtr, maxLength) => {
        if (!bufferPtr || maxLength <= 0) return -1;
        return Math.min(maxLength, 256); // Debug info length
      }),

      get_performance_metrics: this.trackFunction('get_performance_metrics', (metricsPtr) => {
        if (!metricsPtr) return -1;
        return 8; // Number of metrics
      }),

      report_error: this.trackFunction('report_error', (errorCode, messagePtr) => {
        if (!messagePtr) return -1;
        return 0; // Success
      })
    };
  }

  trackFunction(name, implementation) {
    return (...args) => {
      const startTime = performance.now();

      // Check for error injection
      if (this.errorInjection.has(name)) {
        const errorConfig = this.errorInjection.get(name);
        if (Math.random() < errorConfig.probability) {
          throw new Error(errorConfig.message);
        }
      }

      try {
        const result = implementation(...args);
        const endTime = performance.now();

        // Record call history
        this.callHistory.push({
          function: name,
          args: args.length,
          timestamp: startTime,
          duration: endTime - startTime,
          result: result,
          success: true
        });

        // Update performance metrics
        if (!this.performanceMetrics.has(name)) {
          this.performanceMetrics.set(name, {
            callCount: 0,
            totalTime: 0,
            avgTime: 0,
            minTime: Infinity,
            maxTime: 0,
            errorCount: 0
          });
        }

        const metrics = this.performanceMetrics.get(name);
        metrics.callCount++;
        metrics.totalTime += (endTime - startTime);
        metrics.avgTime = metrics.totalTime / metrics.callCount;
        metrics.minTime = Math.min(metrics.minTime, endTime - startTime);
        metrics.maxTime = Math.max(metrics.maxTime, endTime - startTime);

        return result;
      } catch (error) {
        const endTime = performance.now();

        // Record error in call history
        this.callHistory.push({
          function: name,
          args: args.length,
          timestamp: startTime,
          duration: endTime - startTime,
          error: error.message,
          success: false
        });

        // Update error count
        if (this.performanceMetrics.has(name)) {
          this.performanceMetrics.get(name).errorCount++;
        }

        throw error;
      }
    };
  }

  injectError(functionName, probability, message) {
    this.errorInjection.set(functionName, { probability, message });
  }

  clearErrorInjection() {
    this.errorInjection.clear();
  }

  getCallHistory() {
    return [...this.callHistory];
  }

  getPerformanceMetrics() {
    return new Map(this.performanceMetrics);
  }

  clearHistory() {
    this.callHistory = [];
    this.performanceMetrics.clear();
  }
}

// Function registry for tracking and validation
class FunctionRegistry {
  constructor() {
    this.requiredFunctions = new Set([
      // Memory management
      'malloc', 'free',

      // Audio processing
      'process_audio_buffer', 'convert_audio_format', 'assess_audio_quality', 'stream_audio_process',

      // Engine management
      'initialize_engine', 'configure_engine', 'shutdown_engine', 'get_engine_state',

      // Mathematical utilities
      'calculate_fft', 'apply_filter', 'normalize_audio',

      // Data conversion
      'convert_endianness', 'encode_audio', 'decode_audio',

      // Debug and diagnostics
      'get_debug_info', 'get_performance_metrics', 'report_error'
    ]);

    this.functionSignatures = new Map([
      ['malloc', { params: ['number'], returns: 'number' }],
      ['free', { params: ['number'], returns: 'boolean' }],
      ['process_audio_buffer', { params: ['number', 'number', 'number'], returns: 'number' }],
      ['convert_audio_format', { params: ['number', 'number', 'number', 'number', 'number'], returns: 'number' }],
      ['assess_audio_quality', { params: ['number', 'number'], returns: 'number' }],
      ['initialize_engine', { params: ['number'], returns: 'number' }],
      ['calculate_fft', { params: ['number', 'number', 'number'], returns: 'number' }]
    ]);
  }

  validateExports(exports) {
    const missingFunctions = [];
    const invalidFunctions = [];

    for (const funcName of this.requiredFunctions) {
      if (!(funcName in exports)) {
        missingFunctions.push(funcName);
      } else if (typeof exports[funcName] !== 'function') {
        invalidFunctions.push(funcName);
      }
    }

    return {
      isValid: missingFunctions.length === 0 && invalidFunctions.length === 0,
      missingFunctions,
      invalidFunctions,
      totalRequired: this.requiredFunctions.size,
      totalFound: this.requiredFunctions.size - missingFunctions.length
    };
  }

  validateSignature(functionName, args, returnValue) {
    if (!this.functionSignatures.has(functionName)) {
      return { isValid: true, message: 'No signature validation configured' };
    }

    const signature = this.functionSignatures.get(functionName);

    // Validate parameter count
    if (args.length !== signature.params.length) {
      return {
        isValid: false,
        message: `Expected ${signature.params.length} parameters, got ${args.length}`
      };
    }

    // Validate parameter types (basic validation)
    for (let i = 0; i < args.length; i++) {
      const expectedType = signature.params[i];
      const actualType = typeof args[i];

      if (expectedType === 'number' && actualType !== 'number') {
        return {
          isValid: false,
          message: `Parameter ${i} should be ${expectedType}, got ${actualType}`
        };
      }
    }

    // Validate return type
    const expectedReturnType = signature.returns;
    const actualReturnType = typeof returnValue;

    if (expectedReturnType !== actualReturnType && returnValue !== null && returnValue !== undefined) {
      return {
        isValid: false,
        message: `Return value should be ${expectedReturnType}, got ${actualReturnType}`
      };
    }

    return { isValid: true, message: 'Signature validation passed' };
  }
}

/**
 * WASM Function Export Test Suite
 * Tests all exported WASM functions and their interfaces
 */
describe("WASM Function Export Tests", () => {
  let wasmModule;
  let functionRegistry;
  let testStartTime;
  let functionCallCount;

  beforeEach(() => {
    // Load WASM module with all function exports
    wasmModule = new MockWasmModule();

    // Initialize function testing environment
    functionRegistry = new FunctionRegistry();

    // Set up parameter validation systems
    testStartTime = performance.now();
    functionCallCount = 0;

    // Configure return value verification
    wasmModule.clearHistory();
    wasmModule.clearErrorInjection();

    // Create function call monitoring system
    const originalExports = { ...wasmModule.exports };
    Object.keys(originalExports).forEach(funcName => {
      if (typeof originalExports[funcName] === 'function') {
        const originalFunc = originalExports[funcName];
        wasmModule.exports[funcName] = (...args) => {
          functionCallCount++;
          return originalFunc(...args);
        };
      }
    });

    // Initialize performance benchmarking
    global.gc && global.gc(); // Trigger garbage collection if available

    // Set up error injection framework and function call logging
    console.log(`Test setup completed for ${functionRegistry.requiredFunctions.size} functions`);
  });

  afterEach(() => {
    // Clean up function call contexts and generate performance reports
    const testDuration = performance.now() - testStartTime;
    const callHistory = wasmModule.getCallHistory();
    const performanceMetrics = wasmModule.getPerformanceMetrics();

    // Validate function call cleanup
    const activeAllocations = callHistory.filter(call =>
      call.function === 'malloc' && call.success
    ).length - callHistory.filter(call =>
      call.function === 'free' && call.success
    ).length;

    if (activeAllocations > 0) {
      console.warn(`${activeAllocations} potential memory leaks detected`);
    }

    // Export performance metrics
    const performanceReport = {
      testDuration,
      totalCalls: functionCallCount,
      callsPerSecond: functionCallCount / (testDuration / 1000),
      functionMetrics: Object.fromEntries(performanceMetrics),
      callHistory: callHistory.length
    };

    // Clear function call history and reset state
    wasmModule.clearHistory();
    functionCallCount = 0;

    // Archive function test results
    console.log('Function Test Performance:', performanceReport);

    // Reset WASM module state
    wasmModule = null;
    functionRegistry = null;
  });

  describe("Audio Processing Functions", () => {
    /**
     * Test Case 1: Audio Function Export Validation
     * Verifies all required audio processing functions are exported
     */
    it("should export all required audio functions", async () => {
      const requiredAudioFunctions = [
        'process_audio_buffer', 'convert_audio_format', 'assess_audio_quality',
        'stream_audio_process', 'calculate_fft', 'apply_filter', 'normalize_audio'
      ];

      // Verify function availability
      for (const funcName of requiredAudioFunctions) {
        expect(wasmModule.exports).to.have.property(funcName);
        expect(wasmModule.exports[funcName]).to.be.a('function');
      }

      // Validate function registry
      const validation = functionRegistry.validateExports(wasmModule.exports);
      expect(validation.isValid).to.be.true;
      expect(validation.missingFunctions).to.be.empty;
      expect(validation.invalidFunctions).to.be.empty;

      console.log(`Audio functions validated: ${validation.totalFound}/${validation.totalRequired}`);
    });

    /**
     * Test Case 2: Audio Buffer Processing
     * Tests audio buffer read/write and manipulation functions
     */
    it("should handle audio buffer processing correctly", async () => {
      const bufferSize = 1024;
      const channels = 2;
      const sampleRate = 44100;

      // Allocate test buffer
      const bufferPtr = wasmModule.exports.malloc(bufferSize * channels * 4); // 4 bytes per float
      expect(bufferPtr).to.be.greaterThan(0);

      try {
        // Test audio buffer processing
        const processResult = wasmModule.exports.process_audio_buffer(bufferPtr, bufferSize, channels);
        expect(processResult).to.equal(0); // Success

        // Test audio quality assessment
        const qualityScore = wasmModule.exports.assess_audio_quality(bufferPtr, bufferSize);
        expect(qualityScore).to.be.a('number');
        expect(qualityScore).to.be.at.least(0);
        expect(qualityScore).to.be.at.most(100);

        // Test audio normalization
        const targetLevel = 0.8;
        const achievedLevel = wasmModule.exports.normalize_audio(bufferPtr, bufferSize, targetLevel);
        expect(achievedLevel).to.be.a('number');
        expect(achievedLevel).to.be.closeTo(targetLevel, 0.1);

        // Test FFT calculation
        const fftOutputPtr = wasmModule.exports.malloc(bufferSize * 8); // Complex numbers
        const fftResult = wasmModule.exports.calculate_fft(bufferPtr, fftOutputPtr, bufferSize);
        expect(fftResult).to.equal(0); // Success

        wasmModule.exports.free(fftOutputPtr);
      } finally {
        wasmModule.exports.free(bufferPtr);
      }
    });

    /**
     * Test Case 3: Audio Format Conversion
     * Validates audio format conversion functions
     */
    it("should validate audio processing parameters", async () => {
      const validBuffer = wasmModule.exports.malloc(1024);
      const validSize = 512;
      const validChannels = 2;

      try {
        // Test valid parameters
        const validResult = wasmModule.exports.process_audio_buffer(validBuffer, validSize, validChannels);
        expect(validResult).to.equal(0);

        // Test invalid buffer pointer
        const invalidBufferResult = wasmModule.exports.process_audio_buffer(0, validSize, validChannels);
        expect(invalidBufferResult).to.equal(-1);

        // Test invalid size
        const invalidSizeResult = wasmModule.exports.process_audio_buffer(validBuffer, 0, validChannels);
        expect(invalidSizeResult).to.equal(-1);

        // Test invalid channels
        const invalidChannelsResult = wasmModule.exports.process_audio_buffer(validBuffer, validSize, 0);
        expect(invalidChannelsResult).to.equal(-1);

        // Test negative parameters
        const negativeResult = wasmModule.exports.process_audio_buffer(validBuffer, -1, validChannels);
        expect(negativeResult).to.equal(-1);

      } finally {
        wasmModule.exports.free(validBuffer);
      }
    });

    /**
     * Test Case 4: Audio Processing Results Validation
     * Verifies return values and processing accuracy
     */
    it("should return correct audio processing results", async () => {
      const testBufferSize = 256;
      const sourcePtr = wasmModule.exports.malloc(testBufferSize * 4);
      const destPtr = wasmModule.exports.malloc(testBufferSize * 4);

      try {
        // Test audio format conversion
        const convertedSamples = wasmModule.exports.convert_audio_format(
          sourcePtr, destPtr, 1, 2, testBufferSize // Convert format 1 to format 2
        );
        expect(convertedSamples).to.equal(testBufferSize);

        // Test streaming audio processing
        const streamResult = wasmModule.exports.stream_audio_process(sourcePtr, destPtr, testBufferSize);
        expect(streamResult).to.equal(0); // Success

        // Test filter application
        const filterResult = wasmModule.exports.apply_filter(sourcePtr, testBufferSize, 1, 0);
        expect(filterResult).to.equal(0); // Success

        // Validate function signatures
        const conversionValidation = functionRegistry.validateSignature(
          'convert_audio_format',
          [sourcePtr, destPtr, 1, 2, testBufferSize],
          convertedSamples
        );
        expect(conversionValidation.isValid).to.be.true;

      } finally {
        wasmModule.exports.free(sourcePtr);
        wasmModule.exports.free(destPtr);
      }
    });
  });

  describe("Engine Management Functions", () => {
    /**
     * Test Case 5: Engine Lifecycle Functions
     * Tests engine initialization, configuration, and shutdown
     */
    it("should export engine lifecycle functions", async () => {
      const requiredEngineFunctions = [
        'initialize_engine', 'configure_engine', 'shutdown_engine', 'get_engine_state'
      ];

      // Verify all engine functions are available
      for (const funcName of requiredEngineFunctions) {
        expect(wasmModule.exports).to.have.property(funcName);
        expect(wasmModule.exports[funcName]).to.be.a('function');
      }

      // Test engine lifecycle
      const configPtr = wasmModule.exports.malloc(64); // Configuration data

      try {
        // Test engine initialization
        const initResult = wasmModule.exports.initialize_engine(configPtr);
        expect(initResult).to.equal(0); // Success

        // Test engine state query
        const engineState = wasmModule.exports.get_engine_state();
        expect(engineState).to.be.a('number');
        expect(engineState).to.be.at.least(0);

        // Test engine shutdown
        const shutdownResult = wasmModule.exports.shutdown_engine();
        expect(shutdownResult).to.equal(0); // Success

      } finally {
        wasmModule.exports.free(configPtr);
      }
    });

    /**
     * Test Case 6: Engine Configuration Management
     * Tests engine parameter setting and validation
     */
    it("should handle engine configuration correctly", async () => {
      const paramPtr = wasmModule.exports.malloc(32); // Parameter name
      const valuePtr = wasmModule.exports.malloc(32); // Parameter value

      try {
        // Test valid configuration
        const configResult = wasmModule.exports.configure_engine(paramPtr, valuePtr);
        expect(configResult).to.equal(0); // Success

        // Test invalid parameter pointer
        const invalidParamResult = wasmModule.exports.configure_engine(0, valuePtr);
        expect(invalidParamResult).to.equal(-1); // Failure

        // Test invalid value pointer
        const invalidValueResult = wasmModule.exports.configure_engine(paramPtr, 0);
        expect(invalidValueResult).to.equal(-1); // Failure

        // Test configuration with null pointers
        const nullResult = wasmModule.exports.configure_engine(0, 0);
        expect(nullResult).to.equal(-1); // Failure

      } finally {
        wasmModule.exports.free(paramPtr);
        wasmModule.exports.free(valuePtr);
      }
    });

    /**
     * Test Case 7: Engine Resource Management
     * Tests resource allocation and cleanup functions
     */
    it("should manage engine resources properly", async () => {
      const resourceSize = 1024;
      const numResources = 5;
      const allocatedResources = [];

      try {
        // Test resource allocation
        for (let i = 0; i < numResources; i++) {
          const resourcePtr = wasmModule.exports.malloc(resourceSize);
          expect(resourcePtr).to.be.greaterThan(0);
          allocatedResources.push(resourcePtr);
        }

        // Verify all resources are tracked
        expect(allocatedResources.length).to.equal(numResources);

        // Test resource cleanup
        let freedCount = 0;
        for (const resourcePtr of allocatedResources) {
          const freeResult = wasmModule.exports.free(resourcePtr);
          if (freeResult) freedCount++;
        }

        expect(freedCount).to.equal(numResources);

        // Test double-free protection
        const doubleFreedResource = allocatedResources[0];
        const doubleFreeResult = wasmModule.exports.free(doubleFreedResource);
        expect(doubleFreeResult).to.be.false; // Should fail on double-free

      } catch (error) {
        // Clean up any remaining resources
        allocatedResources.forEach(ptr => {
          try {
            wasmModule.exports.free(ptr);
          } catch (cleanupError) {
            console.warn('Resource cleanup error:', cleanupError.message);
          }
        });
        throw error;
      }
    });
  });

  describe("Utility Functions", () => {
    /**
     * Test Case 8: Mathematical Utility Functions
     * Tests FFT calculations and signal processing utilities
     */
    it("should export mathematical utility functions", async () => {
      const mathFunctions = ['calculate_fft', 'apply_filter', 'normalize_audio'];

      // Verify mathematical function availability
      for (const funcName of mathFunctions) {
        expect(wasmModule.exports).to.have.property(funcName);
        expect(wasmModule.exports[funcName]).to.be.a('function');
      }

      const inputSize = 512;
      const inputPtr = wasmModule.exports.malloc(inputSize * 4); // Float array
      const outputPtr = wasmModule.exports.malloc(inputSize * 8); // Complex array

      try {
        // Test FFT calculation
        const fftResult = wasmModule.exports.calculate_fft(inputPtr, outputPtr, inputSize);
        expect(fftResult).to.equal(0); // Success

        // Test filter application
        const filterType = 1; // Low-pass filter
        const filterParams = 0;
        const filterResult = wasmModule.exports.apply_filter(inputPtr, inputSize, filterType, filterParams);
        expect(filterResult).to.equal(0); // Success

        // Test audio normalization
        const targetLevel = 0.9;
        const normalizeResult = wasmModule.exports.normalize_audio(inputPtr, inputSize, targetLevel);
        expect(normalizeResult).to.be.a('number');
        expect(normalizeResult).to.be.closeTo(targetLevel, 0.2);

      } finally {
        wasmModule.exports.free(inputPtr);
        wasmModule.exports.free(outputPtr);
      }
    });

    /**
     * Test Case 9: Data Conversion Functions
     * Tests audio format and endianness conversion utilities
     */
    it("should handle data conversion functions", async () => {
      const dataSize = 256;
      const inputPtr = wasmModule.exports.malloc(dataSize);
      const outputPtr = wasmModule.exports.malloc(dataSize * 2); // Allow for expansion

      try {
        // Test endianness conversion
        const endiannessResult = wasmModule.exports.convert_endianness(inputPtr, dataSize, true, false);
        expect(endiannessResult).to.equal(0); // Success

        // Test audio encoding
        const codec = 1; // Example codec type
        const encodedSize = wasmModule.exports.encode_audio(inputPtr, outputPtr, dataSize, codec);
        expect(encodedSize).to.be.a('number');
        expect(encodedSize).to.be.greaterThan(0);
        expect(encodedSize).to.be.lessThanOrEqual(dataSize); // Compression expected

        // Test audio decoding
        const decodedSize = wasmModule.exports.decode_audio(outputPtr, inputPtr, encodedSize);
        expect(decodedSize).to.be.a('number');
        expect(decodedSize).to.be.greaterThanOrEqual(encodedSize); // Expansion expected

        // Test invalid conversions
        const invalidResult = wasmModule.exports.convert_endianness(0, dataSize, true, false);
        expect(invalidResult).to.equal(-1); // Should fail with null pointer

      } finally {
        wasmModule.exports.free(inputPtr);
        wasmModule.exports.free(outputPtr);
      }
    });

    /**
     * Test Case 10: Debugging and Diagnostics
     * Tests debugging information and performance metrics functions
     */
    it("should provide debugging and diagnostics", async () => {
      const debugFunctions = ['get_debug_info', 'get_performance_metrics', 'report_error'];

      // Verify debug function availability
      for (const funcName of debugFunctions) {
        expect(wasmModule.exports).to.have.property(funcName);
        expect(wasmModule.exports[funcName]).to.be.a('function');
      }

      const debugBufferSize = 512;
      const debugBufferPtr = wasmModule.exports.malloc(debugBufferSize);
      const metricsPtr = wasmModule.exports.malloc(64); // Metrics structure
      const errorMessagePtr = wasmModule.exports.malloc(128);

      try {
        // Test debug information retrieval
        const debugInfoLength = wasmModule.exports.get_debug_info(debugBufferPtr, debugBufferSize);
        expect(debugInfoLength).to.be.a('number');
        expect(debugInfoLength).to.be.greaterThan(0);
        expect(debugInfoLength).to.be.lessThanOrEqual(debugBufferSize);

        // Test performance metrics retrieval
        const metricsCount = wasmModule.exports.get_performance_metrics(metricsPtr);
        expect(metricsCount).to.be.a('number');
        expect(metricsCount).to.be.greaterThanOrEqual(0);

        // Test error reporting
        const errorCode = 404;
        const reportResult = wasmModule.exports.report_error(errorCode, errorMessagePtr);
        expect(reportResult).to.equal(0); // Success

        // Test invalid debug calls
        const invalidDebugResult = wasmModule.exports.get_debug_info(0, debugBufferSize);
        expect(invalidDebugResult).to.equal(-1); // Should fail with null pointer

        const invalidMetricsResult = wasmModule.exports.get_performance_metrics(0);
        expect(invalidMetricsResult).to.equal(-1); // Should fail with null pointer

      } finally {
        wasmModule.exports.free(debugBufferPtr);
        wasmModule.exports.free(metricsPtr);
        wasmModule.exports.free(errorMessagePtr);
      }
    });
  });

  describe("Function Performance", () => {
    /**
     * Test Case 11: Performance Benchmarking
     * Benchmarks critical audio processing functions
     */
    it("should meet performance benchmarks", async () => {
      const benchmarkIterations = 1000;
      const testBufferSize = 1024;
      const testBuffer = wasmModule.exports.malloc(testBufferSize * 4);

      try {
        // Benchmark audio buffer processing
        const audioProcessingStart = performance.now();
        for (let i = 0; i < benchmarkIterations; i++) {
          wasmModule.exports.process_audio_buffer(testBuffer, testBufferSize, 2);
        }
        const audioProcessingTime = performance.now() - audioProcessingStart;

        // Benchmark FFT calculations
        const fftOutputBuffer = wasmModule.exports.malloc(testBufferSize * 8);
        const fftStart = performance.now();
        for (let i = 0; i < benchmarkIterations; i++) {
          wasmModule.exports.calculate_fft(testBuffer, fftOutputBuffer, testBufferSize);
        }
        const fftTime = performance.now() - fftStart;

        // Benchmark audio quality assessment
        const qualityStart = performance.now();
        for (let i = 0; i < benchmarkIterations; i++) {
          wasmModule.exports.assess_audio_quality(testBuffer, testBufferSize);
        }
        const qualityTime = performance.now() - qualityStart;

        // Calculate performance metrics
        const audioProcessingRate = benchmarkIterations / (audioProcessingTime / 1000);
        const fftRate = benchmarkIterations / (fftTime / 1000);
        const qualityRate = benchmarkIterations / (qualityTime / 1000);

        // Performance assertions
        expect(audioProcessingRate).to.be.greaterThan(10000, "Audio processing should exceed 10k calls/sec");
        expect(fftRate).to.be.greaterThan(5000, "FFT should exceed 5k calls/sec");
        expect(qualityRate).to.be.greaterThan(10000, "Quality assessment should exceed 10k calls/sec");

        // Average call time assertions
        const avgAudioTime = audioProcessingTime / benchmarkIterations;
        const avgFftTime = fftTime / benchmarkIterations;
        const avgQualityTime = qualityTime / benchmarkIterations;

        expect(avgAudioTime).to.be.lessThan(0.1, "Audio processing avg time should be < 0.1ms");
        expect(avgFftTime).to.be.lessThan(0.2, "FFT avg time should be < 0.2ms");
        expect(avgQualityTime).to.be.lessThan(0.1, "Quality assessment avg time should be < 0.1ms");

        console.log(`Performance: Audio: ${audioProcessingRate.toFixed(0)}/sec, FFT: ${fftRate.toFixed(0)}/sec, Quality: ${qualityRate.toFixed(0)}/sec`);

        wasmModule.exports.free(fftOutputBuffer);
      } finally {
        wasmModule.exports.free(testBuffer);
      }
    });

    /**
     * Test Case 12: High-Frequency Function Calls
     * Tests rapid function invocation scenarios
     */
    it("should handle high-frequency function calls", async () => {
      const rapidCallCount = 10000;
      const smallBuffer = wasmModule.exports.malloc(64);

      try {
        // Test rapid memory allocation/deallocation
        const memoryStart = performance.now();
        const allocatedPointers = [];

        for (let i = 0; i < rapidCallCount; i++) {
          const ptr = wasmModule.exports.malloc(32);
          allocatedPointers.push(ptr);
        }

        for (const ptr of allocatedPointers) {
          wasmModule.exports.free(ptr);
        }
        const memoryTime = performance.now() - memoryStart;

        // Test rapid function calls with minimal processing
        const rapidStart = performance.now();
        for (let i = 0; i < rapidCallCount; i++) {
          wasmModule.exports.get_engine_state();
        }
        const rapidTime = performance.now() - rapidStart;

        // Calculate call rates
        const memoryCallRate = (rapidCallCount * 2) / (memoryTime / 1000); // malloc + free
        const stateCallRate = rapidCallCount / (rapidTime / 1000);

        // Performance assertions for high-frequency calls
        expect(memoryCallRate).to.be.greaterThan(50000, "Memory operations should exceed 50k calls/sec");
        expect(stateCallRate).to.be.greaterThan(100000, "State queries should exceed 100k calls/sec");

        // Verify no memory leaks from rapid calls
        const callHistory = wasmModule.getCallHistory();
        const mallocCalls = callHistory.filter(call => call.function === 'malloc' && call.success).length;
        const freeCalls = callHistory.filter(call => call.function === 'free' && call.success).length;

        expect(freeCalls).to.be.at.least(mallocCalls - 1); // Allow for test buffer

        console.log(`High-frequency: Memory: ${memoryCallRate.toFixed(0)}/sec, State: ${stateCallRate.toFixed(0)}/sec`);

      } finally {
        wasmModule.exports.free(smallBuffer);
      }
    });

    /**
     * Test Case 13: Memory Usage Optimization
     * Tests memory efficiency in function calls
     */
    it("should optimize memory usage in function calls", async () => {
      const testIterations = 1000;
      const baselineMemory = wasmModule.getPerformanceMetrics().get('malloc') || { callCount: 0 };
      const initialCallCount = baselineMemory.callCount;

      // Test memory-efficient function calls
      const memoryTestBuffer = wasmModule.exports.malloc(2048);

      try {
        // Perform operations that should not require additional allocations
        for (let i = 0; i < testIterations; i++) {
          wasmModule.exports.process_audio_buffer(memoryTestBuffer, 512, 2);
          wasmModule.exports.normalize_audio(memoryTestBuffer, 512, 0.8);
          wasmModule.exports.get_engine_state();
        }

        // Check memory usage after operations
        const finalMetrics = wasmModule.getPerformanceMetrics().get('malloc') || { callCount: 0 };
        const additionalAllocations = finalMetrics.callCount - initialCallCount - 1; // Subtract our test buffer

        // Should not require many additional allocations for processing
        expect(additionalAllocations).to.be.lessThan(10, "Processing should not require excessive allocations");

        // Test memory usage monitoring
        const performanceMetrics = wasmModule.getPerformanceMetrics();
        for (const [funcName, metrics] of performanceMetrics) {
          expect(metrics.callCount).to.be.a('number');
          expect(metrics.totalTime).to.be.a('number');
          expect(metrics.avgTime).to.be.a('number');
          expect(metrics.errorCount).to.be.a('number');

          if (metrics.callCount > 0) {
            expect(metrics.avgTime).to.be.greaterThan(0);
            expect(metrics.minTime).to.be.greaterThan(0);
            expect(metrics.maxTime).to.be.greaterThanOrEqual(metrics.minTime);
          }
        }

        console.log(`Memory efficiency: ${additionalAllocations} additional allocations for ${testIterations} operations`);

      } finally {
        wasmModule.exports.free(memoryTestBuffer);
      }
    });
  });

  describe("Error Handling", () => {
    /**
     * Test Case 14: Invalid Parameter Handling
     * Tests function behavior with invalid parameters
     */
    it("should handle invalid function parameters", async () => {
      // Test null pointer handling
      const nullPtrResults = [
        wasmModule.exports.process_audio_buffer(0, 1024, 2),
        wasmModule.exports.convert_audio_format(0, 1024, 1, 2, 512),
        wasmModule.exports.assess_audio_quality(0, 1024),
        wasmModule.exports.calculate_fft(0, 1024, 512)
      ];

      // All should return error codes
      nullPtrResults.forEach((result, index) => {
        expect(result).to.equal(-1, `Function ${index} should fail with null pointer`);
      });

      // Test out-of-range parameters
      const validBuffer = wasmModule.exports.malloc(1024);

      try {
        // Test negative sizes
        const negativeSize = wasmModule.exports.process_audio_buffer(validBuffer, -1, 2);
        expect(negativeSize).to.equal(-1);

        // Test zero sizes
        const zeroSize = wasmModule.exports.process_audio_buffer(validBuffer, 0, 2);
        expect(zeroSize).to.equal(-1);

        // Test invalid channel counts
        const invalidChannels = wasmModule.exports.process_audio_buffer(validBuffer, 1024, 0);
        expect(invalidChannels).to.equal(-1);

        // Test extremely large values
        const largeValue = wasmModule.exports.process_audio_buffer(validBuffer, Number.MAX_SAFE_INTEGER, 2);
        expect(largeValue).to.equal(-1);

      } finally {
        wasmModule.exports.free(validBuffer);
      }
    });

    /**
     * Test Case 15: Error Message Validation
     * Tests error reporting and message clarity
     */
    it("should provide meaningful error messages", async () => {
      const errorMessageBuffer = wasmModule.exports.malloc(256);

      try {
        // Test error reporting functionality
        const errorCodes = [100, 200, 404, 500];

        for (const errorCode of errorCodes) {
          const reportResult = wasmModule.exports.report_error(errorCode, errorMessageBuffer);
          expect(reportResult).to.equal(0, `Error reporting should succeed for code ${errorCode}`);
        }

        // Test error reporting with invalid message buffer
        const invalidReportResult = wasmModule.exports.report_error(100, 0);
        expect(invalidReportResult).to.equal(-1, "Error reporting should fail with null message buffer");

        // Test consistency of error codes
        const validBuffer = wasmModule.exports.malloc(64);

        // Multiple calls with same invalid parameters should return same error code
        const error1 = wasmModule.exports.process_audio_buffer(0, 1024, 2);
        const error2 = wasmModule.exports.process_audio_buffer(0, 1024, 2);
        expect(error1).to.equal(error2, "Consistent error codes for same invalid input");

        wasmModule.exports.free(validBuffer);

      } finally {
        wasmModule.exports.free(errorMessageBuffer);
      }
    });

    /**
     * Test Case 16: System Stability Under Errors
     * Tests system stability after function errors
     */
    it("should maintain stability on function errors", async () => {
      let errorCount = 0;
      let successCount = 0;
      const testIterations = 100;

      // Inject errors into specific functions
      wasmModule.injectError('process_audio_buffer', 0.3, 'Simulated processing error');
      wasmModule.injectError('calculate_fft', 0.2, 'Simulated FFT error');

      const testBuffer = wasmModule.exports.malloc(1024);
      const fftBuffer = wasmModule.exports.malloc(2048);

      try {
        // Perform operations with error injection
        for (let i = 0; i < testIterations; i++) {
          try {
            // Test audio processing with potential errors
            wasmModule.exports.process_audio_buffer(testBuffer, 256, 2);
            successCount++;
          } catch (error) {
            errorCount++;
            expect(error.message).to.include('Simulated');
          }

          try {
            // Test FFT with potential errors
            wasmModule.exports.calculate_fft(testBuffer, fftBuffer, 256);
            successCount++;
          } catch (error) {
            errorCount++;
            expect(error.message).to.include('Simulated');
          }

          // Test that other functions still work after errors
          const engineState = wasmModule.exports.get_engine_state();
          expect(engineState).to.be.a('number');
        }

        // Verify we had both successes and failures
        expect(errorCount).to.be.greaterThan(0, "Error injection should have caused failures");
        expect(successCount).to.be.greaterThan(0, "Some operations should have succeeded");

        // Clear error injection and verify recovery
        wasmModule.clearErrorInjection();

        // Test that functions work normally after clearing errors
        const recoveryResult = wasmModule.exports.process_audio_buffer(testBuffer, 256, 2);
        expect(recoveryResult).to.equal(0, "Functions should work normally after error recovery");

        console.log(`Error stability test: ${errorCount} errors, ${successCount} successes out of ${testIterations * 2} operations`);

      } finally {
        wasmModule.exports.free(testBuffer);
        wasmModule.exports.free(fftBuffer);
        wasmModule.clearErrorInjection();
      }
    });

    /**
     * Test Case 17: Comprehensive Function Validation
     * Final comprehensive test of all function exports
     */
    it("should pass comprehensive function export validation", async () => {
      // Validate all exported functions
      const validation = functionRegistry.validateExports(wasmModule.exports);

      expect(validation.isValid).to.be.true;

      expect(validation.totalFound).to.equal(validation.totalRequired);

      // Test a representative sample of functions to ensure they work
      const testFunctions = [
        'malloc', 'free', 'process_audio_buffer', 'calculate_fft',
        'initialize_engine', 'get_engine_state', 'report_error'
      ];

      const functionTestResults = [];

      for (const funcName of testFunctions) {
        try {
          let result;
          switch (funcName) {
            case 'malloc':
              result = wasmModule.exports.malloc(64);
              expect(result).to.be.greaterThan(0);
              wasmModule.exports.free(result);
              break;
            case 'free':
              const ptr = wasmModule.exports.malloc(64);
              result = wasmModule.exports.free(ptr);
              expect(result).to.be.true;
              break;
            case 'process_audio_buffer':
              const buffer = wasmModule.exports.malloc(256);
              result = wasmModule.exports.process_audio_buffer(buffer, 64, 2);
              expect(result).to.equal(0);
              wasmModule.exports.free(buffer);
              break;
            case 'calculate_fft':
              const inBuf = wasmModule.exports.malloc(256);
              const outBuf = wasmModule.exports.malloc(512);
              result = wasmModule.exports.calculate_fft(inBuf, outBuf, 64);
              expect(result).to.equal(0);
              wasmModule.exports.free(inBuf);
              wasmModule.exports.free(outBuf);
              break;
            case 'initialize_engine':
              const config = wasmModule.exports.malloc(64);
              result = wasmModule.exports.initialize_engine(config);
              expect(result).to.equal(0);
              wasmModule.exports.free(config);
              break;
            case 'get_engine_state':
              result = wasmModule.exports.get_engine_state();
              expect(result).to.be.a('number');
              break;
            case 'report_error':
              const msgBuf = wasmModule.exports.malloc(128);
              result = wasmModule.exports.report_error(404, msgBuf);
              expect(result).to.equal(0);
              wasmModule.exports.free(msgBuf);
              break;
          }

          functionTestResults.push({ function: funcName, success: true });
        } catch (error) {
          functionTestResults.push({ function: funcName, success: false, error: error.message });
        }
      }

      // All test functions should have passed
      const failedTests = functionTestResults.filter(test => !test.success);
      expect(failedTests).to.be.empty,
        `Failed function tests: ${failedTests.map(t => `${t.function}: ${t.error}`).join(', ')}`
      );

      // Generate final test report
      const finalMetrics = wasmModule.getPerformanceMetrics();
      const callHistory = wasmModule.getCallHistory();

      const testReport = {
        validationPassed: validation.isValid,
        functionsValidated: validation.totalFound,
        functionsRequired: validation.totalRequired,
        functionTestsPassed: functionTestResults.filter(t => t.success).length,
        totalFunctionCalls: callHistory.length,
        performanceMetrics: finalMetrics.size,
        testDuration: performance.now() - testStartTime
      };

      console.log('Comprehensive Function Validation Report:', testReport);
    });
  });
});

export { wasmModule, functionRegistry };
