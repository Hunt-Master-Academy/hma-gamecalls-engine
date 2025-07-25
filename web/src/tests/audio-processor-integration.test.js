/**
 * @file audio-processor-integration.test.js
 * @brief Comprehensive integration test for the Phase 2A modular audio-processor
 *
 * This test validates the complete integration of all Phase 2A modules with the
 * new modular audio-processor system, ensuring enterprise-grade functionality.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Phase 2A Integration Testing
 * @date July 24, 2025
 */

import {
  AudioProcessor,
  PROCESSING_STATES,
  QUALITY_LEVELS,
} from "../src/audio-processor-modular.js";

/**
 * Mock HTML elements for UI testing
 */
function createMockUIElements() {
  // Create mock canvas for waveform renderer
  const mockCanvas = {
    width: 800,
    height: 200,
    getContext: () => ({
      clearRect: () => {},
      fillRect: () => {},
      beginPath: () => {},
      moveTo: () => {},
      lineTo: () => {},
      stroke: () => {},
      fillStyle: "",
      strokeStyle: "",
      lineWidth: 1,
    }),
    getBoundingClientRect: () => ({ left: 0, top: 0, width: 800, height: 200 }),
  };

  // Create mock UI container
  const mockContainer = {
    appendChild: () => {},
    querySelector: () => null,
    style: {},
  };

  return { mockCanvas, mockContainer };
}

/**
 * Mock audio context for testing
 */
function createMockAudioContext() {
  return {
    sampleRate: 44100,
    state: "running",
    currentTime: 0,
    createGain: () => ({
      gain: { setValueAtTime: () => {} },
      connect: () => {},
      disconnect: () => {},
    }),
    createAnalyser: () => ({
      fftSize: 2048,
      smoothingTimeConstant: 0.8,
      connect: () => {},
      disconnect: () => {},
    }),
    resume: () => Promise.resolve(),
    close: () => Promise.resolve(),
  };
}

/**
 * Test Suite: Phase 2A Audio Processor Integration
 */
describe("AudioProcessor - Phase 2A Integration Tests", () => {
  let audioProcessor;
  let mockElements;

  beforeEach(() => {
    // Set up mock environment
    mockElements = createMockUIElements();

    // Mock navigator.mediaDevices
    global.navigator = {
      mediaDevices: {
        getUserMedia: () =>
          Promise.resolve({
            getAudioTracks: () => [{ stop: () => {} }],
          }),
      },
    };

    // Mock AudioContext
    global.AudioContext = function () {
      return createMockAudioContext();
    };

    // Mock performance API
    global.performance = {
      memory: { usedJSHeapSize: 10000000 },
      now: () => Date.now(),
    };
  });

  afterEach(async () => {
    if (audioProcessor) {
      await audioProcessor.destroy();
      audioProcessor = null;
    }
  });

  /**
   * TEST GROUP 1: Initialization and Module Loading
   */
  describe("Initialization and Module Loading", () => {
    test("should create AudioProcessor with default Phase 2A configuration", () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
      });

      expect(audioProcessor).toBeInstanceOf(AudioProcessor);
      expect(audioProcessor.state).toBe(PROCESSING_STATES.IDLE);
      expect(audioProcessor.isInitialized).toBe(false);
      expect(audioProcessor.options.enableSessionPersistence).toBe(true);
      expect(audioProcessor.options.enableUIComponents).toBe(true);
      expect(audioProcessor.options.enableVisualization).toBe(true);
    });

    test("should load all Phase 2A modules during construction", () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        debugMode: true,
      });

      // Verify core modules
      expect(audioProcessor.eventManager).toBeDefined();
      expect(audioProcessor.performanceMonitor).toBeDefined();
      expect(audioProcessor.wasmEngineManager).toBeDefined();
      expect(audioProcessor.audioContextManager).toBeDefined();

      // Verify Phase 2A modules
      expect(audioProcessor.sessionStorage).toBeDefined();
      expect(audioProcessor.sessionState).toBeDefined();
      expect(audioProcessor.uiComponents).toBeDefined();
      expect(audioProcessor.uiLayout).toBeDefined();
      expect(audioProcessor.waveformRenderer).toBeDefined();

      // Verify audio processing modules
      expect(audioProcessor.audioLevelMonitor).toBeDefined();
      expect(audioProcessor.noiseDetector).toBeDefined();
      expect(audioProcessor.automaticGainControl).toBeDefined();
      expect(audioProcessor.qualityAssessor).toBeDefined();
      expect(audioProcessor.masterCallManager).toBeDefined();
      expect(audioProcessor.recordingEnhancer).toBeDefined();
    });

    test("should handle different quality levels", () => {
      const highQualityProcessor = new AudioProcessor({
        qualityLevel: "HIGH",
        waveformCanvas: mockElements.mockCanvas,
      });

      expect(highQualityProcessor.options.sampleRate).toBe(48000);
      expect(highQualityProcessor.options.bufferSize).toBe(1024);
      expect(highQualityProcessor.options.bitDepth).toBe(24);
    });

    test("should initialize with custom Phase 2A options", () => {
      audioProcessor = new AudioProcessor({
        enableSessionPersistence: false,
        enableUIComponents: false,
        enableVisualization: false,
        sessionStorageOptions: { maxSize: 50 },
        visualizationStyle: "OSCILLOSCOPE",
        colorScheme: "GREEN",
      });

      expect(audioProcessor.sessionStorage).toBeUndefined();
      expect(audioProcessor.uiComponents).toBeUndefined();
      expect(audioProcessor.waveformRenderer).toBeUndefined();
    });
  });

  /**
   * TEST GROUP 2: Session Management (Phase 2A)
   */
  describe("Session Management (Phase 2A)", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        debugMode: true,
      });
    });

    test("should create and manage audio processing sessions", async () => {
      const sessionResult = await audioProcessor.createSession({
        customOption: "test-value",
      });

      expect(sessionResult.success).toBe(true);
      expect(sessionResult.session).toBeDefined();
      expect(sessionResult.session.id).toMatch(/^session_\d+_/);
      expect(sessionResult.session.startTime).toBeGreaterThan(0);
      expect(sessionResult.session.options.customOption).toBe("test-value");
    });

    test("should persist session data using SessionStorage module", async () => {
      await audioProcessor.initialize();

      const sessionResult = await audioProcessor.createSession({
        testData: "persistence-test",
      });

      expect(sessionResult.success).toBe(true);
      expect(audioProcessor.currentSession).toBeDefined();
      expect(audioProcessor.currentSession.options.testData).toBe(
        "persistence-test"
      );
    });

    test("should manage session state transitions", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();

      // Track state changes
      const stateChanges = [];
      audioProcessor.eventManager.on("processingStateChanged", (data) => {
        stateChanges.push(data.newState);
      });

      await audioProcessor.startProcessing();
      await audioProcessor.stopProcessing();

      expect(stateChanges).toContain(PROCESSING_STATES.PROCESSING);
      expect(stateChanges).toContain(PROCESSING_STATES.READY);
    });
  });

  /**
   * TEST GROUP 3: UI Integration (Phase 2A)
   */
  describe("UI Integration (Phase 2A)", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        enableUIComponents: true,
        enableResponsiveLayout: true,
        debugMode: true,
      });
    });

    test("should create UI controls during initialization", async () => {
      await audioProcessor.initialize();

      expect(audioProcessor.volumeSlider).toBeDefined();
      expect(audioProcessor.processingToggle).toBeDefined();
      expect(audioProcessor.qualityProgress).toBeDefined();
    });

    test("should handle volume control changes", async () => {
      await audioProcessor.initialize();

      // Mock volume change event
      const volumeEvents = [];
      audioProcessor.eventManager.on("volumeChanged", (data) => {
        volumeEvents.push(data.level);
      });

      // Simulate volume slider change
      audioProcessor._setVolume(0.75);

      expect(volumeEvents).toContain(0.75);
    });

    test("should update UI based on processing state", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();

      const uiStateChanges = [];
      audioProcessor.eventManager.on("processingStateChanged", (data) => {
        uiStateChanges.push(data.newState);
      });

      await audioProcessor.startProcessing();
      expect(uiStateChanges).toContain(PROCESSING_STATES.PROCESSING);

      await audioProcessor.stopProcessing();
      expect(uiStateChanges).toContain(PROCESSING_STATES.READY);
    });
  });

  /**
   * TEST GROUP 4: Visualization (Phase 2A)
   */
  describe("Visualization (Phase 2A)", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        enableVisualization: true,
        debugMode: true,
      });
    });

    test("should initialize waveform renderer with canvas", async () => {
      await audioProcessor.initialize();

      expect(audioProcessor.waveformRenderer).toBeDefined();
      expect(audioProcessor.waveformRenderer.canvas).toBe(
        mockElements.mockCanvas
      );
    });

    test("should update visualization during audio processing", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();
      await audioProcessor.startProcessing();

      // Mock audio buffer data
      const mockAudioBuffer = new Float32Array(1024);
      for (let i = 0; i < mockAudioBuffer.length; i++) {
        mockAudioBuffer[i] = Math.sin((2 * Math.PI * i) / 100) * 0.5;
      }

      // Process buffer should trigger visualization update
      const visualizationEvents = [];
      audioProcessor.eventManager.on("audioLevelUpdate", (data) => {
        visualizationEvents.push(data);
      });

      await audioProcessor.processAudioBuffer(mockAudioBuffer);

      expect(visualizationEvents.length).toBeGreaterThan(0);
    });
  });

  /**
   * TEST GROUP 5: Audio Processing Pipeline
   */
  describe("Audio Processing Pipeline", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        qualityLevel: "MEDIUM",
        debugMode: true,
      });
    });

    test("should process audio buffers through complete pipeline", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();
      await audioProcessor.startProcessing();

      // Create test audio buffer
      const testBuffer = new Float32Array(2048);
      for (let i = 0; i < testBuffer.length; i++) {
        testBuffer[i] = Math.sin((2 * Math.PI * 440 * i) / 44100) * 0.5;
      }

      const result = await audioProcessor.processAudioBuffer(testBuffer);

      expect(result).toBeDefined();
      expect(result.levels).toBeDefined();
      expect(result.noiseAnalysis).toBeDefined();
      expect(result.quality).toBeDefined();
      expect(result.callAnalysis).toBeDefined();
      expect(result.wasmResult).toBeDefined();
      expect(result.timestamp).toBeGreaterThan(0);
    });

    test("should handle AGC and enhancement processing", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();
      await audioProcessor.startProcessing();

      // Create low-level audio buffer
      const lowLevelBuffer = new Float32Array(1024);
      for (let i = 0; i < lowLevelBuffer.length; i++) {
        lowLevelBuffer[i] = Math.sin((2 * Math.PI * 440 * i) / 44100) * 0.1; // Low amplitude
      }

      const result = await audioProcessor.processAudioBuffer(lowLevelBuffer);

      expect(result.levels).toBeDefined();
      expect(result.quality.score).toBeGreaterThanOrEqual(0);
      expect(result.quality.score).toBeLessThanOrEqual(1);
    });
  });

  /**
   * TEST GROUP 6: Performance and Monitoring
   */
  describe("Performance and Monitoring", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        enablePerformanceMonitoring: true,
        debugMode: true,
      });
    });

    test("should provide comprehensive system status", async () => {
      await audioProcessor.initialize();
      const status = audioProcessor.getStatus();

      expect(status.state).toBe(PROCESSING_STATES.READY);
      expect(status.isInitialized).toBe(true);
      expect(status.audioContext).toBeDefined();
      expect(status.modules).toBeDefined();
      expect(status.performance).toBeDefined();
      expect(status.timestamp).toBeGreaterThan(0);

      // Verify Phase 2A status information
      expect(status.sessionStorage).toBeDefined();
      expect(status.sessionState).toBeDefined();
      expect(status.uiLayout).toBeDefined();
      expect(status.waveformRenderer).toBeDefined();
    });

    test("should track performance metrics", async () => {
      await audioProcessor.initialize();

      const metrics = audioProcessor.getPerformanceMetrics();

      expect(metrics.core).toBeDefined();
      expect(metrics.audioContext).toBeDefined();
      expect(metrics.waveform).toBeDefined();
      expect(metrics.session).toBeDefined();
      expect(metrics.overall).toBeDefined();
      expect(metrics.overall.memoryUsage).toBeGreaterThanOrEqual(0);
    });

    test("should handle performance monitoring events", async () => {
      await audioProcessor.initialize();

      const performanceEvents = [];
      audioProcessor.eventManager.on("performanceUpdate", (data) => {
        performanceEvents.push(data);
      });

      // Simulate some processing to trigger performance updates
      await audioProcessor.createSession();
      await audioProcessor.startProcessing();

      // Performance monitoring should be active
      expect(audioProcessor.performanceMonitor).toBeDefined();
    });
  });

  /**
   * TEST GROUP 7: Error Handling and Recovery
   */
  describe("Error Handling and Recovery", () => {
    beforeEach(() => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        debugMode: true,
      });
    });

    test("should handle initialization errors gracefully", async () => {
      // Force an initialization error by mocking a failing module
      const originalWASM = audioProcessor.wasmEngineManager;
      audioProcessor.wasmEngineManager = {
        initialize: () =>
          Promise.reject(new Error("WASM initialization failed")),
      };

      const result = await audioProcessor.initialize();

      expect(result.success).toBe(false);
      expect(result.error).toContain("WASM initialization failed");
      expect(audioProcessor.state).toBe(PROCESSING_STATES.ERROR);

      // Restore original for cleanup
      audioProcessor.wasmEngineManager = originalWASM;
    });

    test("should handle module errors during processing", async () => {
      await audioProcessor.initialize();
      await audioProcessor.createSession();

      const errorEvents = [];
      audioProcessor.eventManager.on("systemError", (data) => {
        errorEvents.push(data);
      });

      // Simulate module error
      audioProcessor.eventManager.emit("error", {
        error: "Test module error",
        source: "TestModule",
        critical: false,
      });

      expect(errorEvents.length).toBeGreaterThan(0);
      expect(errorEvents[0].error).toBe("Test module error");
      expect(errorEvents[0].source).toBe("TestModule");
    });

    test("should handle critical errors appropriately", async () => {
      await audioProcessor.initialize();

      // Simulate critical error
      audioProcessor.eventManager.emit("error", {
        error: "Critical system failure",
        source: "CoreModule",
        critical: true,
      });

      expect(audioProcessor.state).toBe(PROCESSING_STATES.ERROR);
    });
  });

  /**
   * TEST GROUP 8: Cleanup and Destruction
   */
  describe("Cleanup and Destruction", () => {
    test("should properly destroy all modules and clean up resources", async () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        debugMode: true,
      });

      await audioProcessor.initialize();
      await audioProcessor.createSession();
      await audioProcessor.startProcessing();

      // Verify system is running
      expect(audioProcessor.isInitialized).toBe(true);
      expect(audioProcessor.isProcessing).toBe(true);

      // Destroy the processor
      await audioProcessor.destroy();

      // Verify cleanup
      expect(audioProcessor.state).toBe(PROCESSING_STATES.DESTROYED);
      expect(audioProcessor.isInitialized).toBe(false);
      expect(audioProcessor.eventManager).toBeNull();
      expect(audioProcessor.audioContextManager).toBeNull();
      expect(audioProcessor.sessionStorage).toBeNull();
      expect(audioProcessor.waveformRenderer).toBeNull();
    });

    test("should handle destruction errors gracefully", async () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
      });

      // Mock a module that throws during destruction
      audioProcessor.mockFailingModule = {
        destroy: () => {
          throw new Error("Destruction failed");
        },
      };

      await audioProcessor.initialize();

      // Destruction should handle errors but still complete
      await expect(audioProcessor.destroy()).resolves.not.toThrow();
      expect(audioProcessor.state).toBe(PROCESSING_STATES.DESTROYED);
    });
  });

  /**
   * TEST GROUP 9: Cross-browser Compatibility
   */
  describe("Cross-browser Compatibility", () => {
    test("should handle missing Web Audio API gracefully", () => {
      // Remove AudioContext support
      const originalAudioContext = global.AudioContext;
      delete global.AudioContext;

      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
      });

      expect(audioProcessor).toBeDefined();

      // Restore for other tests
      global.AudioContext = originalAudioContext;
    });

    test("should work without canvas support", () => {
      audioProcessor = new AudioProcessor({
        enableVisualization: false,
        uiContainer: mockElements.mockContainer,
      });

      expect(audioProcessor.waveformRenderer).toBeUndefined();
      expect(audioProcessor.options.enableVisualization).toBe(false);
    });
  });

  /**
   * TEST GROUP 10: Integration Validation
   */
  describe("Integration Validation", () => {
    test("should validate complete Phase 2A module integration", async () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        enablePerformanceMonitoring: true,
        debugMode: true,
      });

      await audioProcessor.initialize();

      // Verify all Phase 2A modules are integrated
      const initializedModules = audioProcessor._getInitializedModules();

      expect(initializedModules.eventManager).toBe(true);
      expect(initializedModules.performanceMonitor).toBe(true);
      expect(initializedModules.wasmEngineManager).toBe(true);
      expect(initializedModules.audioContextManager).toBe(true);

      // Phase 2A modules
      expect(initializedModules.sessionStorage).toBe(true);
      expect(initializedModules.sessionState).toBe(true);
      expect(initializedModules.uiComponents).toBe(true);
      expect(initializedModules.uiLayout).toBe(true);
      expect(initializedModules.waveformRenderer).toBe(true);

      // Audio processing modules
      expect(initializedModules.audioLevelMonitor).toBe(true);
      expect(initializedModules.noiseDetector).toBe(true);
      expect(initializedModules.automaticGainControl).toBe(true);
      expect(initializedModules.qualityAssessor).toBe(true);
      expect(initializedModules.masterCallManager).toBe(true);
      expect(initializedModules.recordingEnhancer).toBe(true);
      expect(initializedModules.formatConverter).toBe(true);
      expect(initializedModules.audioWorkletManager).toBe(true);
    });

    test("should demonstrate end-to-end audio processing workflow", async () => {
      audioProcessor = new AudioProcessor({
        waveformCanvas: mockElements.mockCanvas,
        uiContainer: mockElements.mockContainer,
        qualityLevel: "HIGH",
        debugMode: true,
      });

      // Complete workflow test
      await audioProcessor.initialize();
      expect(audioProcessor.isInitialized).toBe(true);

      const session = await audioProcessor.createSession({
        testWorkflow: true,
      });
      expect(session.success).toBe(true);

      await audioProcessor.startProcessing();
      expect(audioProcessor.isProcessing).toBe(true);

      // Create realistic audio data
      const audioData = new Float32Array(4096);
      for (let i = 0; i < audioData.length; i++) {
        audioData[i] =
          Math.sin((2 * Math.PI * 440 * i) / 48000) * 0.3 +
          Math.sin((2 * Math.PI * 880 * i) / 48000) * 0.2;
      }

      const result = await audioProcessor.processAudioBuffer(audioData);
      expect(result).toBeDefined();
      expect(result.levels).toBeDefined();
      expect(result.quality).toBeDefined();
      expect(result.timestamp).toBeGreaterThan(0);

      await audioProcessor.stopProcessing();
      expect(audioProcessor.isProcessing).toBe(false);

      const status = audioProcessor.getStatus();
      expect(status.state).toBe(PROCESSING_STATES.READY);
      expect(status.currentSession).toBeDefined();
    });
  });
});

// Export test utilities for other test files
export {
  createMockUIElements,
  createMockAudioContext,
  PROCESSING_STATES,
  QUALITY_LEVELS,
};

console.log("✅ Phase 2A Audio Processor Integration Tests Ready");
console.log("🎯 Testing complete modular architecture with:");
console.log("   • Session Management (SessionStorage, SessionState)");
console.log("   • UI Integration (UIComponents, UILayout)");
console.log("   • Visualization (WaveformRenderer)");
console.log("   • Audio Context Management");
console.log("   • Complete Audio Processing Pipeline");
console.log("   • Performance Monitoring");
console.log("   • Error Handling & Recovery");
console.log("   • Cross-browser Compatibility");
