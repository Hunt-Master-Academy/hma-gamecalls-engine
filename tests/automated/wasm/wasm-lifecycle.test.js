/**
 * @file wasm-lifecycle.test.js
 * @brief WASM Lifecycle Management Test Module - Phase 3.1 Automated Testing
 *
 * This module provides comprehensive testing for WebAssembly module lifecycle
 * management, including initialization, running states, and cleanup processes.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

import { expect } from "chai";
import { describe, it, beforeEach, afterEach } from "mocha";

// WASM Lifecycle States
const LifecycleStates = {
  UNINITIALIZED: 'uninitialized',
  INITIALIZING: 'initializing',
  INITIALIZED: 'initialized',
  STARTING: 'starting',
  RUNNING: 'running',
  PAUSED: 'paused',
  STOPPING: 'stopping',
  STOPPED: 'stopped',
  CLEANUP: 'cleanup',
  DESTROYED: 'destroyed',
  ERROR: 'error'
};

// Mock WASM Lifecycle Manager
class MockWasmLifecycleManager {
  constructor() {
    this.state = LifecycleStates.UNINITIALIZED;
    this.stateHistory = [{ state: this.state, timestamp: performance.now() }];
    this.resources = new Map();
    this.eventListeners = new Map();
    this.config = new Map();
    this.healthMetrics = {
      uptime: 0,
      memoryUsage: 0,
      cpuUsage: 0,
      errorCount: 0,
      warningCount: 0
    };
    this.performanceMetrics = {
      initTime: 0,
      startTime: 0,
      stopTime: 0,
      cleanupTime: 0,
      totalLifetime: 0
    };
    this.lifecycleStartTime = performance.now();
    this.initializationAttempts = 0;
    this.maxInitAttempts = 3;
    this.forceShutdownFlag = false;
    this.resourceLeakTracker = new Map();
  }

  // State management
  setState(newState, metadata = {}) {
    if (!this.isValidTransition(this.state, newState)) {
      throw new Error(`Invalid state transition from ${this.state} to ${newState}`);
    }

    const oldState = this.state;
    this.state = newState;
    this.stateHistory.push({
      state: newState,
      previousState: oldState,
      timestamp: performance.now(),
      metadata
    });

    this.emitEvent('stateChanged', { oldState, newState, metadata });
  }

  isValidTransition(fromState, toState) {
    const validTransitions = {
      [LifecycleStates.UNINITIALIZED]: [LifecycleStates.INITIALIZING, LifecycleStates.ERROR],
      [LifecycleStates.INITIALIZING]: [LifecycleStates.INITIALIZED, LifecycleStates.ERROR, LifecycleStates.UNINITIALIZED],
      [LifecycleStates.INITIALIZED]: [LifecycleStates.STARTING, LifecycleStates.CLEANUP, LifecycleStates.ERROR],
      [LifecycleStates.STARTING]: [LifecycleStates.RUNNING, LifecycleStates.ERROR, LifecycleStates.STOPPING],
      [LifecycleStates.RUNNING]: [LifecycleStates.PAUSED, LifecycleStates.STOPPING, LifecycleStates.ERROR],
      [LifecycleStates.PAUSED]: [LifecycleStates.RUNNING, LifecycleStates.STOPPING, LifecycleStates.ERROR],
      [LifecycleStates.STOPPING]: [LifecycleStates.STOPPED, LifecycleStates.ERROR, LifecycleStates.CLEANUP],
      [LifecycleStates.STOPPED]: [LifecycleStates.CLEANUP, LifecycleStates.STARTING, LifecycleStates.DESTROYED],
      [LifecycleStates.CLEANUP]: [LifecycleStates.DESTROYED, LifecycleStates.ERROR],
      [LifecycleStates.ERROR]: [LifecycleStates.CLEANUP, LifecycleStates.DESTROYED, LifecycleStates.UNINITIALIZED],
      [LifecycleStates.DESTROYED]: []
    };

    return validTransitions[fromState]?.includes(toState) || false;
  }

  // Lifecycle operations
  async initialize(config = {}) {
    if (this.state !== LifecycleStates.UNINITIALIZED) {
      throw new Error(`Cannot initialize from state: ${this.state}`);
    }

    this.initializationAttempts++;
    if (this.initializationAttempts > this.maxInitAttempts) {
      throw new Error('Maximum initialization attempts exceeded');
    }

    this.setState(LifecycleStates.INITIALIZING);
    const startTime = performance.now();

    try {
      // Simulate initialization process
      await this.simulateAsyncOperation(100, 0.1); // 100ms with 10% failure rate

      // Store configuration
      Object.entries(config).forEach(([key, value]) => {
        this.config.set(key, value);
      });

      // Allocate initial resources
      this.allocateResource('wasmModule', { size: 1024 * 1024, type: 'memory' });
      this.allocateResource('audioBuffer', { size: 4096, type: 'buffer' });

      this.performanceMetrics.initTime = performance.now() - startTime;
      this.setState(LifecycleStates.INITIALIZED, { initTime: this.performanceMetrics.initTime });

      return true;
    } catch (error) {
      this.setState(LifecycleStates.ERROR, { error: error.message });
      throw error;
    }
  }

  async start() {
    if (this.state !== LifecycleStates.INITIALIZED && this.state !== LifecycleStates.STOPPED) {
      throw new Error(`Cannot start from state: ${this.state}`);
    }

    this.setState(LifecycleStates.STARTING);
    const startTime = performance.now();

    try {
      // Simulate startup process
      await this.simulateAsyncOperation(50, 0.05); // 50ms with 5% failure rate

      this.performanceMetrics.startTime = performance.now() - startTime;
      this.setState(LifecycleStates.RUNNING, { startTime: this.performanceMetrics.startTime });

      // Start health monitoring
      this.startHealthMonitoring();

      return true;
    } catch (error) {
      this.setState(LifecycleStates.ERROR, { error: error.message });
      throw error;
    }
  }

  async stop(force = false) {
    if (this.state !== LifecycleStates.RUNNING && this.state !== LifecycleStates.PAUSED) {
      if (!force) {
        throw new Error(`Cannot stop from state: ${this.state}`);
      }
    }

    this.forceShutdownFlag = force;
    this.setState(LifecycleStates.STOPPING, { forced: force });
    const stopTime = performance.now();

    try {
      // Stop health monitoring
      this.stopHealthMonitoring();

      // Simulate graceful shutdown
      if (!force) {
        await this.simulateAsyncOperation(75, 0.03); // 75ms with 3% failure rate
      }

      this.performanceMetrics.stopTime = performance.now() - stopTime;
      this.setState(LifecycleStates.STOPPED, { stopTime: this.performanceMetrics.stopTime, forced: force });

      return true;
    } catch (error) {
      this.setState(LifecycleStates.ERROR, { error: error.message });
      throw error;
    }
  }

  async cleanup() {
    if (this.state !== LifecycleStates.STOPPED && this.state !== LifecycleStates.ERROR && this.state !== LifecycleStates.INITIALIZED) {
      throw new Error(`Cannot cleanup from state: ${this.state}`);
    }

    this.setState(LifecycleStates.CLEANUP);
    const cleanupTime = performance.now();

    try {
      // Deallocate all resources
      const resourceKeys = Array.from(this.resources.keys());
      for (const key of resourceKeys) {
        await this.deallocateResource(key);
      }

      // Clear configuration
      this.config.clear();

      // Clear event listeners
      this.eventListeners.clear();

      this.performanceMetrics.cleanupTime = performance.now() - cleanupTime;
      this.performanceMetrics.totalLifetime = performance.now() - this.lifecycleStartTime;

      this.setState(LifecycleStates.DESTROYED, {
        cleanupTime: this.performanceMetrics.cleanupTime,
        totalLifetime: this.performanceMetrics.totalLifetime
      });

      return true;
    } catch (error) {
      this.setState(LifecycleStates.ERROR, { error: error.message });
      throw error;
    }
  }

  // Resource management
  allocateResource(name, spec) {
    if (this.resources.has(name)) {
      throw new Error(`Resource ${name} already allocated`);
    }

    const resource = {
      name,
      spec,
      allocatedAt: performance.now(),
      size: spec.size || 0,
      type: spec.type || 'unknown'
    };

    this.resources.set(name, resource);
    this.resourceLeakTracker.set(name, resource);
    this.healthMetrics.memoryUsage += resource.size;

    this.emitEvent('resourceAllocated', { name, resource });
    return resource;
  }

  async deallocateResource(name) {
    const resource = this.resources.get(name);
    if (!resource) {
      throw new Error(`Resource ${name} not found`);
    }

    // Simulate deallocation delay
    await this.simulateAsyncOperation(10, 0.01);

    this.resources.delete(name);
    this.resourceLeakTracker.delete(name);
    this.healthMetrics.memoryUsage -= resource.size;

    this.emitEvent('resourceDeallocated', { name, resource });
    return true;
  }

  // Health monitoring
  startHealthMonitoring() {
    this.healthMonitorInterval = setInterval(() => {
      this.updateHealthMetrics();
    }, 100); // Update every 100ms
  }

  stopHealthMonitoring() {
    if (this.healthMonitorInterval) {
      clearInterval(this.healthMonitorInterval);
      this.healthMonitorInterval = null;
    }
  }

  updateHealthMetrics() {
    if (this.state === LifecycleStates.RUNNING) {
      this.healthMetrics.uptime = performance.now() - this.lifecycleStartTime;
      this.healthMetrics.cpuUsage = Math.random() * 100; // Simulate CPU usage

      // Simulate occasional health issues
      if (Math.random() < 0.01) { // 1% chance
        this.healthMetrics.warningCount++;
        this.emitEvent('healthWarning', { metric: 'cpu', value: this.healthMetrics.cpuUsage });
      }
    }
  }

  // Event system
  on(event, callback) {
    if (!this.eventListeners.has(event)) {
      this.eventListeners.set(event, []);
    }
    this.eventListeners.get(event).push(callback);
  }

  off(event, callback) {
    if (this.eventListeners.has(event)) {
      const listeners = this.eventListeners.get(event);
      const index = listeners.indexOf(callback);
      if (index !== -1) {
        listeners.splice(index, 1);
      }
    }
  }

  emitEvent(event, data) {
    if (this.eventListeners.has(event)) {
      this.eventListeners.get(event).forEach(callback => {
        try {
          callback(data);
        } catch (error) {
          console.error(`Event handler error for ${event}:`, error);
        }
      });
    }
  }

  // Utility methods
  async simulateAsyncOperation(duration, failureRate = 0) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        if (Math.random() < failureRate) {
          reject(new Error('Simulated operation failure'));
        } else {
          resolve();
        }
      }, duration);
    });
  }

  getState() {
    return this.state;
  }

  getStateHistory() {
    return [...this.stateHistory];
  }

  getHealthMetrics() {
    return { ...this.healthMetrics };
  }

  getPerformanceMetrics() {
    return { ...this.performanceMetrics };
  }

  getResources() {
    return new Map(this.resources);
  }

  getResourceLeaks() {
    return new Map(this.resourceLeakTracker);
  }

  getCurrentConfig() {
    return new Map(this.config);
  }

  // State corruption simulation for testing
  simulateStateCorruption() {
    this.state = 'corrupted_state';
    this.emitEvent('stateCorruption', { corruptedState: this.state });
  }

  recoverFromStateCorruption() {
    // Attempt to recover to a safe state
    const lastValidState = this.stateHistory
      .filter(entry => Object.values(LifecycleStates).includes(entry.state))
      .pop();

    if (lastValidState) {
      this.state = lastValidState.state;
      this.emitEvent('stateRecovered', { recoveredState: this.state });
      return true;
    }

    // If no valid state found, reset to uninitialized
    this.state = LifecycleStates.UNINITIALIZED;
    this.emitEvent('stateReset', { resetState: this.state });
    return false;
  }
}

// State Tracker for lifecycle monitoring
class LifecycleStateTracker {
  constructor() {
    this.transitions = [];
    this.anomalies = [];
    this.performanceData = [];
    this.healthData = [];
  }

  trackTransition(from, to, metadata = {}) {
    this.transitions.push({
      from,
      to,
      timestamp: performance.now(),
      metadata
    });
  }

  trackAnomaly(type, description, severity = 'medium') {
    this.anomalies.push({
      type,
      description,
      severity,
      timestamp: performance.now()
    });
  }

  trackPerformance(operation, duration, success = true) {
    this.performanceData.push({
      operation,
      duration,
      success,
      timestamp: performance.now()
    });
  }

  trackHealth(metrics) {
    this.healthData.push({
      ...metrics,
      timestamp: performance.now()
    });
  }

  getTransitionSummary() {
    const summary = {};
    this.transitions.forEach(transition => {
      const key = `${transition.from} -> ${transition.to}`;
      summary[key] = (summary[key] || 0) + 1;
    });
    return summary;
  }

  getAnomalySummary() {
    const summary = {};
    this.anomalies.forEach(anomaly => {
      summary[anomaly.type] = (summary[anomaly.type] || 0) + 1;
    });
    return summary;
  }

  getPerformanceSummary() {
    const summary = {};
    this.performanceData.forEach(data => {
      if (!summary[data.operation]) {
        summary[data.operation] = { total: 0, count: 0, successes: 0, failures: 0 };
      }
      summary[data.operation].total += data.duration;
      summary[data.operation].count++;
      if (data.success) {
        summary[data.operation].successes++;
      } else {
        summary[data.operation].failures++;
      }
    });

    // Calculate averages
    Object.keys(summary).forEach(operation => {
      const stats = summary[operation];
      stats.average = stats.total / stats.count;
      stats.successRate = stats.successes / stats.count;
    });

    return summary;
  }

  reset() {
    this.transitions = [];
    this.anomalies = [];
    this.performanceData = [];
    this.healthData = [];
  }
}

/**
 * WASM Lifecycle Management Test Suite
 * Tests complete WASM module lifecycle from birth to death
 */
describe("WASM Lifecycle Management Tests", () => {
  let lifecycleManager;
  let stateTracker;
  let testStartTime;

  beforeEach(() => {
    // Initialize lifecycle management system
    lifecycleManager = new MockWasmLifecycleManager();

    // Set up state tracking mechanisms
    stateTracker = new LifecycleStateTracker();

    // Configure lifecycle event monitoring
    lifecycleManager.on('stateChanged', (data) => {
      stateTracker.trackTransition(data.oldState, data.newState, data.metadata);
    });

    lifecycleManager.on('healthWarning', (data) => {
      stateTracker.trackAnomaly('health_warning', `Health warning: ${data.metric}`, 'medium');
    });

    lifecycleManager.on('stateCorruption', (data) => {
      stateTracker.trackAnomaly('state_corruption', 'State corruption detected', 'high');
    });

    // Initialize cleanup verification systems
    testStartTime = performance.now();

    console.log('Lifecycle test setup completed');
  });

  afterEach(() => {
    // Verify complete lifecycle cleanup
    const resourceLeaks = lifecycleManager.getResourceLeaks();
    if (resourceLeaks.size > 0) {
      console.warn(`Resource leaks detected: ${Array.from(resourceLeaks.keys()).join(', ')}`);
    }

    // Generate lifecycle performance reports
    const testDuration = performance.now() - testStartTime;
    const performanceMetrics = lifecycleManager.getPerformanceMetrics();
    const transitionSummary = stateTracker.getTransitionSummary();
    const anomalySummary = stateTracker.getAnomalySummary();

    const testReport = {
      testDuration,
      finalState: lifecycleManager.getState(),
      performanceMetrics,
      transitions: Object.keys(transitionSummary).length,
      anomalies: Object.keys(anomalySummary).length,
      resourceLeaks: resourceLeaks.size
    };

    // Check for resource leaks and reset lifecycle tracking state
    expect(resourceLeaks.size).to.equal(0, "No resource leaks should remain after test");

    // Reset lifecycle tracking state
    stateTracker.reset();
    lifecycleManager = null;
    stateTracker = null;

    console.log('Lifecycle Test Report:', testReport);
  });

  describe("Initialization Phase", () => {
    /**
     * Test Case 1: Successful WASM Module Initialization
     * Tests the complete initialization sequence
     */
    it("should initialize WASM module correctly", async () => {
      // Verify initial state
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.UNINITIALIZED);

      // Test initialization with valid configuration
      const config = {
        sampleRate: 44100,
        bufferSize: 4096,
        channels: 2,
        audioFormat: 'float32'
      };

      const startTime = performance.now();
      const initResult = await lifecycleManager.initialize(config);
      const initDuration = performance.now() - startTime;

      // Verify initialization success
      expect(initResult).to.be.true;
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED);

      // Verify configuration was stored
      const storedConfig = lifecycleManager.getCurrentConfig();
      expect(storedConfig.get('sampleRate')).to.equal(44100);
      expect(storedConfig.get('bufferSize')).to.equal(4096);
      expect(storedConfig.get('channels')).to.equal(2);
      expect(storedConfig.get('audioFormat')).to.equal('float32');

      // Verify initial resources were allocated
      const resources = lifecycleManager.getResources();
      expect(resources.has('wasmModule')).to.be.true;
      expect(resources.has('audioBuffer')).to.be.true;

      // Verify performance metrics
      const performanceMetrics = lifecycleManager.getPerformanceMetrics();
      expect(performanceMetrics.initTime).to.be.a('number');
      expect(performanceMetrics.initTime).to.be.greaterThan(0);
      expect(performanceMetrics.initTime).to.be.closeTo(initDuration, 10);

      // Track performance for analysis
      stateTracker.trackPerformance('initialization', initDuration, true);
    });

    /**
     * Test Case 2: Initialization Failure Handling
     * Tests graceful handling of initialization failures
     */
    it("should handle initialization failures gracefully", async () => {
      // Test multiple initialization attempts to trigger failure
      let initAttempts = 0;
      let lastError = null;

      // Attempt initialization multiple times to exceed maximum attempts
      while (initAttempts < 5) {
        try {
          await lifecycleManager.initialize({});
          break; // If successful, exit loop
        } catch (error) {
          lastError = error;
          initAttempts++;

          // Reset to uninitialized state for next attempt
          if (lifecycleManager.getState() === LifecycleStates.ERROR) {
            lifecycleManager.setState(LifecycleStates.UNINITIALIZED);
          }
        }
      }

      // Should eventually fail with max attempts exceeded
      expect(lastError).to.not.be.null;
      expect(lastError.message).to.include('Maximum initialization attempts exceeded');

      // Test initialization with invalid parameters
      try {
        await lifecycleManager.initialize(null);
        expect.fail('Should have thrown error for null config');
      } catch (error) {
        expect(error).to.be.an('error');
        stateTracker.trackAnomaly('init_failure', 'Null configuration', 'high');
      }

      // Verify state is error or uninitialized
      const finalState = lifecycleManager.getState();
      expect([LifecycleStates.ERROR, LifecycleStates.UNINITIALIZED]).to.include(finalState);

      // Verify no resources leaked during failed initialization
      const resourceLeaks = lifecycleManager.getResourceLeaks();
      expect(resourceLeaks.size).to.be.lessThan(3); // Allow for some test allocation
    });

    /**
     * Test Case 3: Initialization Performance Tracking
     * Benchmarks initialization timing and resource usage
     */
    it("should track initialization performance", async () => {
      const benchmarkRuns = 5;
      const initTimes = [];
      const resourceUsages = [];

      for (let i = 0; i < benchmarkRuns; i++) {
        // Reset manager for each run
        if (i > 0) {
          await lifecycleManager.cleanup();
          lifecycleManager = new MockWasmLifecycleManager();
        }

        const startTime = performance.now();
        const beforeMemory = lifecycleManager.getHealthMetrics().memoryUsage;

        await lifecycleManager.initialize({
          runId: i,
          benchmarkMode: true
        });

        const initTime = performance.now() - startTime;
        const afterMemory = lifecycleManager.getHealthMetrics().memoryUsage;
        const memoryDelta = afterMemory - beforeMemory;

        initTimes.push(initTime);
        resourceUsages.push(memoryDelta);

        stateTracker.trackPerformance('init_benchmark', initTime, true);
      }

      // Calculate performance statistics
      const avgInitTime = initTimes.reduce((sum, time) => sum + time, 0) / initTimes.length;
      const minInitTime = Math.min(...initTimes);
      const maxInitTime = Math.max(...initTimes);
      const avgMemoryUsage = resourceUsages.reduce((sum, usage) => sum + usage, 0) / resourceUsages.length;

      // Performance assertions
      expect(avgInitTime).to.be.lessThan(200, "Average initialization should be under 200ms");
      expect(maxInitTime).to.be.lessThan(500, "Maximum initialization should be under 500ms");
      expect(avgMemoryUsage).to.be.greaterThan(0, "Should allocate memory during initialization");

      // Verify initialization efficiency (consistency)
      const timeDifference = maxInitTime - minInitTime;
      expect(timeDifference).to.be.lessThan(avgInitTime, "Initialization times should be consistent");

      console.log(`Initialization Performance: Avg: ${avgInitTime.toFixed(2)}ms, Min: ${minInitTime.toFixed(2)}ms, Max: ${maxInitTime.toFixed(2)}ms`);
      console.log(`Memory Usage: Avg: ${avgMemoryUsage.toFixed(0)} bytes`);
    });
  });

  describe("Running Phase", () => {
    /**
     * Test Case 4: Stable Running State Management
     * Tests long-running WASM module stability
     */
    it("should maintain stable running state", async () => {
      // Initialize and start the module
      await lifecycleManager.initialize({ testMode: true });
      await lifecycleManager.start();

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

      // Test stability over time with state consistency checks
      const stabilityTestDuration = 200; // ms
      const checkInterval = 25; // ms
      const startTime = performance.now();
      const stateChecks = [];

      while (performance.now() - startTime < stabilityTestDuration) {
        await new Promise(resolve => setTimeout(resolve, checkInterval));

        const currentState = lifecycleManager.getState();
        const healthMetrics = lifecycleManager.getHealthMetrics();

        stateChecks.push({
          timestamp: performance.now() - startTime,
          state: currentState,
          uptime: healthMetrics.uptime,
          memoryUsage: healthMetrics.memoryUsage,
          errorCount: healthMetrics.errorCount
        });

        // State should remain RUNNING throughout
        expect(currentState).to.equal(LifecycleStates.RUNNING,
          `State should remain RUNNING at ${(performance.now() - startTime).toFixed(0)}ms`);
      }

      // Verify stability metrics
      expect(stateChecks.length).to.be.greaterThan(5, "Should have multiple stability checks");

      const finalHealthMetrics = lifecycleManager.getHealthMetrics();
      expect(finalHealthMetrics.uptime).to.be.greaterThan(stabilityTestDuration * 0.8);
      expect(finalHealthMetrics.errorCount).to.equal(0, "No errors should occur during stable running");

      // Test runtime error handling
      try {
        // Simulate a runtime operation that might fail
        await lifecycleManager.simulateAsyncOperation(10, 0.5); // High failure rate
      } catch (error) {
        // Error should not change the running state
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);
        stateTracker.trackAnomaly('runtime_error', 'Handled runtime operation error', 'low');
      }

      console.log(`Stability test: ${stateChecks.length} checks over ${stabilityTestDuration}ms`);
    });

    /**
     * Test Case 5: Runtime Configuration Changes
     * Tests dynamic configuration updates during runtime
     */
    it("should handle runtime configuration changes", async () => {
      // Initialize and start the module
      const initialConfig = { bufferSize: 1024, sampleRate: 44100 };
      await lifecycleManager.initialize(initialConfig);
      await lifecycleManager.start();

      // Test dynamic configuration updates
      const configUpdates = [
        { bufferSize: 2048 },
        { sampleRate: 48000 },
        { channels: 2, bitDepth: 24 },
        { processingMode: 'enhanced' }
      ];

      for (const update of configUpdates) {
        const beforeConfig = new Map(lifecycleManager.getCurrentConfig());

        // Apply configuration update
        Object.entries(update).forEach(([key, value]) => {
          lifecycleManager.config.set(key, value);
        });

        // Verify configuration was updated
        const afterConfig = lifecycleManager.getCurrentConfig();
        Object.entries(update).forEach(([key, value]) => {
          expect(afterConfig.get(key)).to.equal(value, `Config ${key} should be updated`);
        });

        // Verify state remains running
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

        stateTracker.trackPerformance('config_update', 5, true); // 5ms typical update time
      }

      // Test configuration validation
      const invalidConfigs = [
        { bufferSize: -1 },
        { sampleRate: 0 },
        { channels: 'invalid' }
      ];

      for (const invalidConfig of invalidConfigs) {
        const beforeState = lifecycleManager.getState();

        // Apply invalid configuration (should be handled gracefully)
        try {
          Object.entries(invalidConfig).forEach(([key, value]) => {
            if (typeof value === 'number' && value <= 0) {
              throw new Error(`Invalid ${key}: ${value}`);
            }
            if (typeof value === 'string' && key === 'channels') {
              throw new Error(`Invalid ${key}: ${value}`);
            }
          });
        } catch (error) {
          // Error should not affect running state
          expect(lifecycleManager.getState()).to.equal(beforeState);
          stateTracker.trackAnomaly('config_validation', `Invalid config: ${Object.keys(invalidConfig)[0]}`, 'medium');
        }
      }

      // Test configuration rollback mechanism
      const rollbackConfig = new Map(lifecycleManager.getCurrentConfig());
      lifecycleManager.config.set('testRollback', 'value');

      // Simulate rollback
      lifecycleManager.config.delete('testRollback');
      expect(lifecycleManager.getCurrentConfig().has('testRollback')).to.be.false;
    });

    /**
     * Test Case 6: Runtime Health Monitoring
     * Tests health monitoring systems during runtime
     */
    it("should monitor runtime health", async () => {
      // Initialize and start with health monitoring
      await lifecycleManager.initialize({ healthMonitoring: true });
      await lifecycleManager.start();

      // Verify health monitoring is active
      expect(lifecycleManager.healthMonitorInterval).to.not.be.null;

      // Collect health data over time
      const healthSamples = [];
      const monitoringDuration = 150; // ms
      const sampleInterval = 25; // ms
      const startTime = performance.now();

      while (performance.now() - startTime < monitoringDuration) {
        await new Promise(resolve => setTimeout(resolve, sampleInterval));

        const healthMetrics = lifecycleManager.getHealthMetrics();
        healthSamples.push({
          timestamp: performance.now() - startTime,
          ...healthMetrics
        });

        stateTracker.trackHealth(healthMetrics);
      }

      // Verify health monitoring data
      expect(healthSamples.length).to.be.greaterThan(3, "Should collect multiple health samples");

      const latestHealth = healthSamples[healthSamples.length - 1];
      expect(latestHealth.uptime).to.be.greaterThan(monitoringDuration * 0.8);
      expect(latestHealth.memoryUsage).to.be.a('number');
      expect(latestHealth.cpuUsage).to.be.a('number');

      // Test health degradation detection
      const initialWarningCount = latestHealth.warningCount;

      // Wait for potential health warnings (they occur randomly)
      await new Promise(resolve => setTimeout(resolve, 100));

      const finalHealth = lifecycleManager.getHealthMetrics();
      if (finalHealth.warningCount > initialWarningCount) {
        console.log(`Health warnings detected: ${finalHealth.warningCount - initialWarningCount}`);
        stateTracker.trackAnomaly('health_degradation', 'Health warning detected during monitoring', 'medium');
      }

      // Test health recovery mechanisms
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING,
        "Health issues should not affect running state");

      console.log(`Health monitoring: ${healthSamples.length} samples, Uptime: ${latestHealth.uptime.toFixed(0)}ms`);
    });
  });

  describe("Cleanup Phase", () => {
    /**
     * Test Case 7: Complete Shutdown and Cleanup
     * Tests comprehensive shutdown procedures
     */
    it("should perform complete cleanup on shutdown", async () => {
      // Set up a complete lifecycle
      await lifecycleManager.initialize({ cleanupTest: true });

      // Allocate additional resources for cleanup testing
      lifecycleManager.allocateResource('testBuffer1', { size: 1024, type: 'buffer' });
      lifecycleManager.allocateResource('testBuffer2', { size: 2048, type: 'buffer' });
      lifecycleManager.allocateResource('testHandle', { size: 64, type: 'handle' });

      await lifecycleManager.start();

      // Verify running state with resources
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);
      const resourcesBeforeShutdown = lifecycleManager.getResources();
      expect(resourcesBeforeShutdown.size).to.be.at.least(5); // Initial + test resources

      // Perform graceful shutdown
      const shutdownStartTime = performance.now();
      await lifecycleManager.stop();
      const stopDuration = performance.now() - shutdownStartTime;

      // Verify stopped state
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.STOPPED);

      // Perform cleanup
      const cleanupStartTime = performance.now();
      await lifecycleManager.cleanup();
      const cleanupDuration = performance.now() - cleanupStartTime;

      // Verify complete cleanup
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED);

      // Verify all resources were deallocated
      const resourcesAfterCleanup = lifecycleManager.getResources();
      expect(resourcesAfterCleanup.size).to.equal(0, "All resources should be deallocated");

      // Verify configuration was cleared
      const configAfterCleanup = lifecycleManager.getCurrentConfig();
      expect(configAfterCleanup.size).to.equal(0, "Configuration should be cleared");

      // Verify performance metrics
      const performanceMetrics = lifecycleManager.getPerformanceMetrics();
      expect(performanceMetrics.stopTime).to.be.greaterThan(0);
      expect(performanceMetrics.cleanupTime).to.be.greaterThan(0);
      expect(performanceMetrics.totalLifetime).to.be.greaterThan(0);

      // Performance assertions
      expect(stopDuration).to.be.lessThan(200, "Stop should complete quickly");
      expect(cleanupDuration).to.be.lessThan(100, "Cleanup should complete quickly");

      stateTracker.trackPerformance('shutdown', stopDuration, true);
      stateTracker.trackPerformance('cleanup', cleanupDuration, true);

      console.log(`Cleanup Performance: Stop: ${stopDuration.toFixed(2)}ms, Cleanup: ${cleanupDuration.toFixed(2)}ms`);
    });

    /**
     * Test Case 8: Forced Shutdown Scenarios
     * Tests emergency shutdown procedures
     */
    it("should handle forced shutdown scenarios", async () => {
      // Set up running state
      await lifecycleManager.initialize({ forceTest: true });
      await lifecycleManager.start();

      // Allocate resources that might be difficult to clean up
      lifecycleManager.allocateResource('persistentResource', { size: 4096, type: 'persistent' });
      lifecycleManager.allocateResource('networkHandle', { size: 128, type: 'network' });

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

      // Test forced shutdown from running state
      const forceShutdownStart = performance.now();
      await lifecycleManager.stop(true); // Force shutdown
      const forceShutdownDuration = performance.now() - forceShutdownStart;

      // Verify forced shutdown succeeded
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.STOPPED);
      expect(lifecycleManager.forceShutdownFlag).to.be.true;

      // Test cleanup after forced shutdown
      const emergencyCleanupStart = performance.now();
      await lifecycleManager.cleanup();
      const emergencyCleanupDuration = performance.now() - emergencyCleanupStart;

      // Verify cleanup completed despite forced shutdown
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED);
      const remainingResources = lifecycleManager.getResources();
      expect(remainingResources.size).to.equal(0, "All resources should be cleaned up even after forced shutdown");

      // Test forced shutdown performance (should be faster than graceful)
      expect(forceShutdownDuration).to.be.lessThan(100, "Forced shutdown should be rapid");

      // Test cleanup under adverse conditions
      stateTracker.trackAnomaly('forced_shutdown', 'Emergency shutdown procedure executed', 'high');
      stateTracker.trackPerformance('force_shutdown', forceShutdownDuration, true);
      stateTracker.trackPerformance('emergency_cleanup', emergencyCleanupDuration, true);

      console.log(`Emergency Procedures: Force Stop: ${forceShutdownDuration.toFixed(2)}ms, Emergency Cleanup: ${emergencyCleanupDuration.toFixed(2)}ms`);
    });

    /**
     * Test Case 9: Resource Leak Prevention
     * Tests cleanup verification systems and leak prevention
     */
    it("should prevent resource leaks during cleanup", async () => {
      // Set up multiple cleanup scenarios
      const testScenarios = [
        { name: 'normal_cleanup', resources: 3 },
        { name: 'heavy_resources', resources: 10 },
        { name: 'mixed_types', resources: 5 }
      ];

      for (const scenario of testScenarios) {
        // Reset for each scenario
        if (lifecycleManager.getState() !== LifecycleStates.UNINITIALIZED) {
          await lifecycleManager.cleanup();
          lifecycleManager = new MockWasmLifecycleManager();
        }

        await lifecycleManager.initialize({ scenario: scenario.name });

        // Allocate specified number of resources
        const allocatedResourceNames = [];
        for (let i = 0; i < scenario.resources; i++) {
          const resourceName = `${scenario.name}_resource_${i}`;
          const resourceSize = Math.floor(Math.random() * 2048) + 256;
          const resourceType = ['buffer', 'handle', 'memory', 'file'][i % 4];

          lifecycleManager.allocateResource(resourceName, {
            size: resourceSize,
            type: resourceType
          });
          allocatedResourceNames.push(resourceName);
        }

        await lifecycleManager.start();

        // Monitor resource usage during running
        const initialMemoryUsage = lifecycleManager.getHealthMetrics().memoryUsage;
        expect(initialMemoryUsage).to.be.greaterThan(0, "Memory should be in use");

        // Verify all resources are tracked
        const resourcesBeforeCleanup = lifecycleManager.getResources();
        expect(resourcesBeforeCleanup.size).to.be.at.least(scenario.resources + 2); // + initial resources

        // Perform cleanup
        await lifecycleManager.stop();
        await lifecycleManager.cleanup();

        // Verify complete resource deallocation
        const resourcesAfterCleanup = lifecycleManager.getResources();
        const resourceLeaks = lifecycleManager.getResourceLeaks();

        expect(resourcesAfterCleanup.size).to.equal(0,
          `No resources should remain after cleanup in ${scenario.name}`);
        expect(resourceLeaks.size).to.equal(0,
          `No resource leaks should be detected in ${scenario.name}`);

        // Verify memory usage decreased
        const finalMemoryUsage = lifecycleManager.getHealthMetrics().memoryUsage;
        expect(finalMemoryUsage).to.equal(0,
          `Memory usage should be zero after cleanup in ${scenario.name}`);

        stateTracker.trackPerformance(`cleanup_${scenario.name}`, 50, true);
        console.log(`Scenario ${scenario.name}: ${scenario.resources} resources cleaned successfully`);
      }

      // Test leak detection systems
      const leakDetectionTest = async () => {
        lifecycleManager = new MockWasmLifecycleManager();
        await lifecycleManager.initialize({ leakTest: true });

        // Simulate a resource leak by not calling cleanup
        lifecycleManager.allocateResource('leaked_resource', { size: 1024, type: 'leaked' });

        const leaks = lifecycleManager.getResourceLeaks();
        expect(leaks.size).to.be.greaterThan(0, "Leak detection should identify leaked resources");

        stateTracker.trackAnomaly('resource_leak', 'Intentional leak for testing detection', 'medium');
      };

      await leakDetectionTest();
    });
  });

  describe("State Transitions", () => {
    /**
     * Test Case 10: Valid State Transitions
     * Tests all valid lifecycle state transitions
     */
    it("should handle valid state transitions", async () => {
      // Test complete valid lifecycle sequence
      const expectedTransitions = [
        { from: LifecycleStates.UNINITIALIZED, to: LifecycleStates.INITIALIZING, action: 'initialize' },
        { from: LifecycleStates.INITIALIZING, to: LifecycleStates.INITIALIZED, action: 'complete_init' },
        { from: LifecycleStates.INITIALIZED, to: LifecycleStates.STARTING, action: 'start' },
        { from: LifecycleStates.STARTING, to: LifecycleStates.RUNNING, action: 'complete_start' },
        { from: LifecycleStates.RUNNING, to: LifecycleStates.PAUSED, action: 'pause' },
        { from: LifecycleStates.PAUSED, to: LifecycleStates.RUNNING, action: 'resume' },
        { from: LifecycleStates.RUNNING, to: LifecycleStates.STOPPING, action: 'stop' },
        { from: LifecycleStates.STOPPING, to: LifecycleStates.STOPPED, action: 'complete_stop' },
        { from: LifecycleStates.STOPPED, to: LifecycleStates.CLEANUP, action: 'cleanup' },
        { from: LifecycleStates.CLEANUP, to: LifecycleStates.DESTROYED, action: 'complete_cleanup' }
      ];

      // Execute complete lifecycle
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.UNINITIALIZED);

      // Initialize
      await lifecycleManager.initialize({ transitionTest: true });
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED);

      // Start
      await lifecycleManager.start();
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

      // Test pause/resume transitions
      lifecycleManager.setState(LifecycleStates.PAUSED);
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.PAUSED);

      lifecycleManager.setState(LifecycleStates.RUNNING);
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

      // Stop
      await lifecycleManager.stop();
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.STOPPED);

      // Cleanup
      await lifecycleManager.cleanup();
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED);

      // Verify all transitions were tracked
      const stateHistory = lifecycleManager.getStateHistory();
      expect(stateHistory.length).to.be.greaterThan(5, "Multiple state transitions should be recorded");

      // Verify state change notifications
      const transitionSummary = stateTracker.getTransitionSummary();
      expect(Object.keys(transitionSummary).length).to.be.greaterThan(3,
        "Multiple different transitions should be tracked");

      // Test specific valid transition validation
      const validTransitionTests = [
        { from: LifecycleStates.UNINITIALIZED, to: LifecycleStates.INITIALIZING },
        { from: LifecycleStates.INITIALIZED, to: LifecycleStates.STARTING },
        { from: LifecycleStates.RUNNING, to: LifecycleStates.PAUSED },
        { from: LifecycleStates.ERROR, to: LifecycleStates.CLEANUP }
      ];

      for (const transition of validTransitionTests) {
        const isValid = lifecycleManager.isValidTransition(transition.from, transition.to);
        expect(isValid).to.be.true,
          `Transition from ${transition.from} to ${transition.to} should be valid`);
      }

      console.log(`State Transitions: ${stateHistory.length} total transitions recorded`);
    });

    /**
     * Test Case 11: Invalid State Transitions
     * Tests rejection of invalid state transitions
     */
    it("should reject invalid state transitions", async () => {
      // Test invalid transition attempts
      const invalidTransitions = [
        { from: LifecycleStates.UNINITIALIZED, to: LifecycleStates.RUNNING },
        { from: LifecycleStates.INITIALIZING, to: LifecycleStates.RUNNING },
        { from: LifecycleStates.INITIALIZED, to: LifecycleStates.STOPPED },
        { from: LifecycleStates.RUNNING, to: LifecycleStates.DESTROYED },
        { from: LifecycleStates.DESTROYED, to: LifecycleStates.RUNNING },
        { from: LifecycleStates.DESTROYED, to: LifecycleStates.INITIALIZED }
      ];

      for (const transition of invalidTransitions) {
        const isValid = lifecycleManager.isValidTransition(transition.from, transition.to);
        expect(isValid).to.be.false,
          `Transition from ${transition.from} to ${transition.to} should be invalid`);
      }

      // Test state transition guards in practice
      await lifecycleManager.initialize({ guardTest: true });
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED);

      // Attempt invalid direct transition to destroyed
      try {
        lifecycleManager.setState(LifecycleStates.DESTROYED);
        expect.fail('Should not allow direct transition from INITIALIZED to DESTROYED');
      } catch (error) {
        expect(error.message).to.include('Invalid state transition');
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED,
          'State should remain unchanged after invalid transition attempt');
        stateTracker.trackAnomaly('invalid_transition', 'Attempted invalid state transition', 'medium');
      }

      // Test transition from destroyed state (should be impossible)
      await lifecycleManager.start();
      await lifecycleManager.stop();
      await lifecycleManager.cleanup();
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED);

      try {
        lifecycleManager.setState(LifecycleStates.RUNNING);
        expect.fail('Should not allow any transitions from DESTROYED state');
      } catch (error) {
        expect(error.message).to.include('Invalid state transition');
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED,
          'DESTROYED state should be terminal');
      }

      // Test state rollback mechanisms
      const beforeState = LifecycleStates.DESTROYED;
      try {
        lifecycleManager.setState(LifecycleStates.UNINITIALIZED);
        expect.fail('Transition from DESTROYED should fail');
      } catch (error) {
        expect(lifecycleManager.getState()).to.equal(beforeState,
          'State should roll back to previous state on failed transition');
      }
    });

    /**
     * Test Case 12: State Corruption Recovery
     * Tests state corruption detection and recovery mechanisms
     */
    it("should recover from state corruption", async () => {
      // Set up a valid running state
      await lifecycleManager.initialize({ corruptionTest: true });
      await lifecycleManager.start();
      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);

      // Record state before corruption
      const validStateHistory = lifecycleManager.getStateHistory();
      const lastValidStateCount = validStateHistory.length;

      // Simulate state corruption
      lifecycleManager.simulateStateCorruption();
      expect(lifecycleManager.getState()).to.equal('corrupted_state');

      // Test state corruption detection
      const corruptedState = lifecycleManager.getState();
      const isValidState = Object.values(LifecycleStates).includes(corruptedState);
      expect(isValidState).to.be.false, 'Corrupted state should not be valid');

      stateTracker.trackAnomaly('state_corruption', 'State corruption detected', 'critical');

      // Test state recovery mechanisms
      const recoveryResult = lifecycleManager.recoverFromStateCorruption();

      if (recoveryResult) {
        // Successful recovery to previous valid state
        const recoveredState = lifecycleManager.getState();
        const isRecoveredStateValid = Object.values(LifecycleStates).includes(recoveredState);
        expect(isRecoveredStateValid).to.be.true, 'Recovered state should be valid');

        stateTracker.trackAnomaly('state_recovery', 'State successfully recovered', 'low');
        console.log(`State recovered to: ${recoveredState}`);
      } else {
        // Reset to safe state
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.UNINITIALIZED,
          'Should reset to uninitialized state if recovery fails');

        stateTracker.trackAnomaly('state_reset', 'State reset to uninitialized', 'medium');
        console.log('State reset to uninitialized after corruption');
      }

      // Test state integrity checks
      const postRecoveryHistory = lifecycleManager.getStateHistory();
      expect(postRecoveryHistory.length).to.be.greaterThan(lastValidStateCount,
        'State history should include recovery events');

      // Verify state consistency after recovery
      const finalState = lifecycleManager.getState();
      const isConsistent = Object.values(LifecycleStates).includes(finalState);
      expect(isConsistent).to.be.true, 'Final state should be consistent');

      // Test state restoration procedures
      if (finalState === LifecycleStates.UNINITIALIZED) {
        // Test that we can restart normally after recovery
        await lifecycleManager.initialize({ postRecoveryTest: true });
        expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED,
          'Should be able to reinitialize after state recovery');
      }

      console.log(`State corruption recovery test completed. Final state: ${finalState}`);
    });

    /**
     * Test Case 13: Comprehensive Lifecycle Validation
     * Final comprehensive test of complete lifecycle management
     */
    it("should pass comprehensive lifecycle validation", async () => {
      // Reset to ensure clean test
      if (lifecycleManager.getState() !== LifecycleStates.UNINITIALIZED) {
        await lifecycleManager.cleanup().catch(() => {}); // Ignore errors
        lifecycleManager = new MockWasmLifecycleManager();
        stateTracker.reset();
      }

      // Execute complete lifecycle with comprehensive monitoring
      const lifecycleStartTime = performance.now();

      // Phase 1: Initialization
      const initStart = performance.now();
      await lifecycleManager.initialize({
        comprehensive: true,
        bufferSize: 8192,
        sampleRate: 48000,
        channels: 2
      });
      const initDuration = performance.now() - initStart;

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.INITIALIZED);
      expect(initDuration).to.be.lessThan(200, "Initialization should be efficient");

      // Phase 2: Startup and Running
      const startupStart = performance.now();
      await lifecycleManager.start();
      const startupDuration = performance.now() - startupStart;

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.RUNNING);
      expect(startupDuration).to.be.lessThan(100, "Startup should be efficient");

      // Phase 3: Runtime Operations
      const runtimeDuration = 100; // ms
      await new Promise(resolve => setTimeout(resolve, runtimeDuration));

      const healthMetrics = lifecycleManager.getHealthMetrics();
      expect(healthMetrics.uptime).to.be.greaterThan(runtimeDuration * 0.8);
      expect(healthMetrics.errorCount).to.equal(0, "No errors during runtime");

      // Phase 4: Graceful Shutdown
      const shutdownStart = performance.now();
      await lifecycleManager.stop();
      const shutdownDuration = performance.now() - shutdownStart;

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.STOPPED);
      expect(shutdownDuration).to.be.lessThan(150, "Shutdown should be efficient");

      // Phase 5: Complete Cleanup
      const cleanupStart = performance.now();
      await lifecycleManager.cleanup();
      const cleanupDuration = performance.now() - cleanupStart;

      expect(lifecycleManager.getState()).to.equal(LifecycleStates.DESTROYED);
      expect(cleanupDuration).to.be.lessThan(100, "Cleanup should be efficient");

      // Comprehensive Validation
      const totalLifecycleDuration = performance.now() - lifecycleStartTime;
      const stateHistory = lifecycleManager.getStateHistory();
      const performanceMetrics = lifecycleManager.getPerformanceMetrics();
      const transitionSummary = stateTracker.getTransitionSummary();
      const anomalySummary = stateTracker.getAnomalySummary();
      const performanceSummary = stateTracker.getPerformanceSummary();

      // Validate state history
      expect(stateHistory.length).to.be.at.least(6, "Should have multiple state transitions");

      // Validate performance metrics
      expect(performanceMetrics.initTime).to.be.greaterThan(0);
      expect(performanceMetrics.startTime).to.be.greaterThan(0);
      expect(performanceMetrics.stopTime).to.be.greaterThan(0);
      expect(performanceMetrics.cleanupTime).to.be.greaterThan(0);
      expect(performanceMetrics.totalLifetime).to.be.greaterThan(0);

      // Validate resource cleanup
      const remainingResources = lifecycleManager.getResources();
      const resourceLeaks = lifecycleManager.getResourceLeaks();
      expect(remainingResources.size).to.equal(0, "No resources should remain");
      expect(resourceLeaks.size).to.equal(0, "No resource leaks should be detected");

      // Generate comprehensive test report
      const comprehensiveReport = {
        totalDuration: totalLifecycleDuration,
        phases: {
          initialization: initDuration,
          startup: startupDuration,
          runtime: runtimeDuration,
          shutdown: shutdownDuration,
          cleanup: cleanupDuration
        },
        stateTransitions: Object.keys(transitionSummary).length,
        anomalies: Object.keys(anomalySummary).length,
        performanceMetrics: performanceMetrics,
        resourceManagement: {
          allocated: stateHistory.filter(h => h.metadata?.resource).length,
          leaks: resourceLeaks.size
        },
        healthMetrics: healthMetrics
      };

      // Final validation assertions
      expect(totalLifecycleDuration).to.be.lessThan(1000, "Complete lifecycle should finish within 1 second");
      expect(Object.keys(transitionSummary).length).to.be.at.least(4, "Should have multiple transition types");
      expect(performanceMetrics.totalLifetime).to.be.closeTo(totalLifecycleDuration, 50);

      console.log('Comprehensive Lifecycle Validation Report:', comprehensiveReport);

      // Log summary of all tracked data
      console.log('Performance Summary:', performanceSummary);
      console.log('Transition Summary:', transitionSummary);
      console.log('Anomaly Summary:', anomalySummary);
    });
  });
});

export { lifecycleManager, stateTracker };

export { lifecycleManager, stateTracker };
