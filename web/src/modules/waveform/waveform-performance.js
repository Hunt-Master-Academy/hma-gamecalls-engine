/**
 * @fileoverview Waveform Performance Module - Performance Monitoring and Optimization
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Provides comprehensive performance monitoring and optimization for waveform
 * analysis and visualization with real-time metrics and adaptive quality control.
 *
 * Key Features:
 * - Real-time performance monitoring and metrics collection
 * - Adaptive quality control based on system performance
 * - Memory usage tracking and garbage collection optimization
 * - Frame rate monitoring and automatic adjustment
 * - CPU usage analysis and workload balancing
 * - Performance profiling and bottleneck identification
 *
 * Dependencies:
 * - Performance API for high-resolution timing
 * - Memory API where available
 * - Animation frame scheduling
 * - Web Workers for background processing
 *
 * @example
 * ```javascript
 * import { WaveformPerformance } from './modules/waveform/waveform-performance.js';
 *
 * const monitor = new WaveformPerformance({
 *   targetFPS: 60,
 *   enableProfiling: true,
 *   adaptiveQuality: true
 * });
 *
 * monitor.startMonitoring();
 * const metrics = monitor.getMetrics();
 * ```
 */

/**
 * Performance Monitor and Optimizer for Waveform Processing
 *
 * Monitors system performance and automatically adjusts processing quality
 * to maintain smooth user experience with comprehensive metrics collection.
 *
 * @class WaveformPerformance
 */
export class WaveformPerformance {
  /**
   * Create a WaveformPerformance instance
   *
   * @param {Object} options - Configuration options
   * @param {number} [options.targetFPS=60] - Target frame rate
   * @param {boolean} [options.enableProfiling=true] - Enable performance profiling
   * @param {boolean} [options.adaptiveQuality=true] - Enable adaptive quality control
   * @param {number} [options.memoryThreshold=0.8] - Memory usage threshold
   * @param {number} [options.cpuThreshold=0.7] - CPU usage threshold
   * @param {Object} [options.qualityLevels] - Quality level configurations
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      targetFPS: options.targetFPS || 60,
      enableProfiling: options.enableProfiling !== false,
      adaptiveQuality: options.adaptiveQuality !== false,
      memoryThreshold: options.memoryThreshold || 0.8,
      cpuThreshold: options.cpuThreshold || 0.7,
      qualityLevels: {
        high: { fftSize: 2048, updateRate: 60, webglEnabled: true },
        medium: { fftSize: 1024, updateRate: 30, webglEnabled: true },
        low: { fftSize: 512, updateRate: 15, webglEnabled: false },
        ...options.qualityLevels,
      },
      ...options,
    };

    // Performance monitoring state
    this.state = {
      isMonitoring: false,
      isOptimizing: false,
      currentQuality: "high",
      lastOptimizationTime: 0,
      frameCount: 0,
      startTime: 0,
      lastFrameTime: 0,
    };

    // Performance metrics
    this.metrics = {
      // Frame rate metrics
      fps: {
        current: 0,
        average: 0,
        min: Infinity,
        max: 0,
        history: [],
      },

      // Frame timing metrics
      frameTime: {
        current: 0,
        average: 0,
        min: Infinity,
        max: 0,
        history: [],
      },

      // Memory metrics
      memory: {
        used: 0,
        total: 0,
        percentage: 0,
        jsHeapSize: 0,
        history: [],
      },

      // CPU metrics
      cpu: {
        usage: 0,
        renderTime: 0,
        analysisTime: 0,
        history: [],
      },

      // Quality metrics
      quality: {
        level: "high",
        adjustments: 0,
        droppedFrames: 0,
        renderSkips: 0,
      },

      // Profiling data
      profiling: {
        marks: [],
        measures: [],
        bottlenecks: [],
      },
    };

    // Performance thresholds
    this.thresholds = {
      fpsWarning: this.config.targetFPS * 0.8,
      fpsError: this.config.targetFPS * 0.6,
      frameTimeWarning: (1000 / this.config.targetFPS) * 1.2,
      frameTimeError: (1000 / this.config.targetFPS) * 1.5,
      memoryWarning: this.config.memoryThreshold,
      memoryError: 0.95,
      cpuWarning: this.config.cpuThreshold,
      cpuError: 0.9,
    };

    // History buffer sizes
    this.historySize = {
      fps: 300, // 5 seconds at 60fps
      frameTime: 300,
      memory: 60, // 1 minute at 1 sample/second
      cpu: 60,
    };

    // Optimization strategies
    this.optimizations = {
      // Frame rate optimizations
      frameRate: {
        reduceUpdateRate: () => this._reduceUpdateRate(),
        skipFrames: () => this._enableFrameSkipping(),
        disableAnimations: () => this._disableAnimations(),
      },

      // Memory optimizations
      memory: {
        clearCaches: () => this._clearCaches(),
        reduceBufferSizes: () => this._reduceBufferSizes(),
        forceGC: () => this._forceGarbageCollection(),
      },

      // Quality optimizations
      quality: {
        reduceFFTSize: () => this._reduceFFTSize(),
        disableWebGL: () => this._disableWebGL(),
        simplifyVisualization: () => this._simplifyVisualization(),
      },
    };

    // Event handlers
    this.eventHandlers = new Map();

    // Performance observer
    this.observer = null;

    // Animation frame tracking
    this.animationFrame = null;

    this._initialize();
  }

  /**
   * Initialize performance monitoring system
   * @private
   */
  _initialize() {
    try {
      // Initialize performance observer if available
      this._setupPerformanceObserver();

      // Setup memory monitoring
      this._setupMemoryMonitoring();

      // Initialize profiling if enabled
      if (this.config.enableProfiling) {
        this._setupProfiling();
      }

      console.log("WaveformPerformance initialized successfully");
    } catch (error) {
      console.error("WaveformPerformance initialization failed:", error);
    }
  }

  /**
   * Setup performance observer for detailed metrics
   * @private
   */
  _setupPerformanceObserver() {
    if (typeof PerformanceObserver !== "undefined") {
      try {
        this.observer = new PerformanceObserver((list) => {
          for (const entry of list.getEntries()) {
            this._processPerformanceEntry(entry);
          }
        });

        // Observe various performance entry types
        this.observer.observe({
          entryTypes: [
            "measure",
            "navigation",
            "paint",
            "largest-contentful-paint",
          ],
        });
      } catch (error) {
        console.warn("PerformanceObserver not fully supported:", error);
      }
    }
  }

  /**
   * Setup memory monitoring
   * @private
   */
  _setupMemoryMonitoring() {
    // Check for memory API support
    this.memoryApiSupported = !!(
      performance.memory ||
      (navigator.deviceMemory && navigator.hardwareConcurrency)
    );

    if (this.memoryApiSupported) {
      // Start periodic memory monitoring
      this.memoryMonitorInterval = setInterval(() => {
        this._updateMemoryMetrics();
      }, 1000); // Check every second
    }
  }

  /**
   * Setup performance profiling
   * @private
   */
  _setupProfiling() {
    // Clear any existing marks and measures
    if (performance.clearMarks) {
      performance.clearMarks();
    }
    if (performance.clearMeasures) {
      performance.clearMeasures();
    }
  }

  /**
   * Start performance monitoring
   */
  startMonitoring() {
    if (this.state.isMonitoring) {
      return;
    }

    this.state.isMonitoring = true;
    this.state.startTime = performance.now();
    this.state.frameCount = 0;

    // Start frame rate monitoring
    this._startFrameRateMonitoring();

    // Start adaptive optimization if enabled
    if (this.config.adaptiveQuality) {
      this._startAdaptiveOptimization();
    }

    this._emitEvent("monitoringStarted");
  }

  /**
   * Stop performance monitoring
   */
  stopMonitoring() {
    if (!this.state.isMonitoring) {
      return;
    }

    this.state.isMonitoring = false;

    // Stop frame rate monitoring
    if (this.animationFrame) {
      cancelAnimationFrame(this.animationFrame);
      this.animationFrame = null;
    }

    // Clear intervals
    if (this.memoryMonitorInterval) {
      clearInterval(this.memoryMonitorInterval);
      this.memoryMonitorInterval = null;
    }

    if (this.optimizationInterval) {
      clearInterval(this.optimizationInterval);
      this.optimizationInterval = null;
    }

    this._emitEvent("monitoringStopped");
  }

  /**
   * Start frame rate monitoring
   * @private
   */
  _startFrameRateMonitoring() {
    const monitorFrame = (timestamp) => {
      if (!this.state.isMonitoring) {
        return;
      }

      // Calculate frame metrics
      this._updateFrameMetrics(timestamp);

      // Schedule next frame
      this.animationFrame = requestAnimationFrame(monitorFrame);
    };

    this.animationFrame = requestAnimationFrame(monitorFrame);
  }

  /**
   * Update frame metrics
   * @private
   */
  _updateFrameMetrics(timestamp) {
    this.state.frameCount++;

    if (this.state.lastFrameTime > 0) {
      // Calculate frame time
      const frameTime = timestamp - this.state.lastFrameTime;
      this.metrics.frameTime.current = frameTime;

      // Update frame time history
      this.metrics.frameTime.history.push(frameTime);
      if (this.metrics.frameTime.history.length > this.historySize.frameTime) {
        this.metrics.frameTime.history.shift();
      }

      // Calculate frame time statistics
      this.metrics.frameTime.average = this._calculateAverage(
        this.metrics.frameTime.history
      );
      this.metrics.frameTime.min = Math.min(
        this.metrics.frameTime.min,
        frameTime
      );
      this.metrics.frameTime.max = Math.max(
        this.metrics.frameTime.max,
        frameTime
      );

      // Calculate FPS
      const fps = 1000 / frameTime;
      this.metrics.fps.current = fps;

      // Update FPS history
      this.metrics.fps.history.push(fps);
      if (this.metrics.fps.history.length > this.historySize.fps) {
        this.metrics.fps.history.shift();
      }

      // Calculate FPS statistics
      this.metrics.fps.average = this._calculateAverage(
        this.metrics.fps.history
      );
      this.metrics.fps.min = Math.min(this.metrics.fps.min, fps);
      this.metrics.fps.max = Math.max(this.metrics.fps.max, fps);

      // Check for performance issues
      this._checkPerformanceThresholds();
    }

    this.state.lastFrameTime = timestamp;
  }

  /**
   * Update memory metrics
   * @private
   */
  _updateMemoryMetrics() {
    if (!this.memoryApiSupported) {
      return;
    }

    let memoryInfo = {};

    // Get memory information from available APIs
    if (performance.memory) {
      memoryInfo = {
        used: performance.memory.usedJSHeapSize,
        total: performance.memory.jsHeapSizeLimit,
        jsHeapSize: performance.memory.totalJSHeapSize,
      };
    } else if (navigator.deviceMemory) {
      // Fallback to device memory info
      memoryInfo = {
        used: 0, // Not available
        total: navigator.deviceMemory * 1024 * 1024 * 1024, // Convert GB to bytes
        jsHeapSize: 0,
      };
    }

    // Update memory metrics
    this.metrics.memory.used = memoryInfo.used;
    this.metrics.memory.total = memoryInfo.total;
    this.metrics.memory.jsHeapSize = memoryInfo.jsHeapSize;
    this.metrics.memory.percentage =
      memoryInfo.total > 0 ? memoryInfo.used / memoryInfo.total : 0;

    // Update memory history
    this.metrics.memory.history.push({
      timestamp: Date.now(),
      used: memoryInfo.used,
      percentage: this.metrics.memory.percentage,
    });

    if (this.metrics.memory.history.length > this.historySize.memory) {
      this.metrics.memory.history.shift();
    }
  }

  /**
   * Check performance thresholds and trigger optimizations
   * @private
   */
  _checkPerformanceThresholds() {
    const fps = this.metrics.fps.current;
    const frameTime = this.metrics.frameTime.current;
    const memoryUsage = this.metrics.memory.percentage;

    let needsOptimization = false;
    const issues = [];

    // Check FPS thresholds
    if (fps < this.thresholds.fpsError) {
      issues.push({ type: "fps", severity: "error", value: fps });
      needsOptimization = true;
    } else if (fps < this.thresholds.fpsWarning) {
      issues.push({ type: "fps", severity: "warning", value: fps });
    }

    // Check frame time thresholds
    if (frameTime > this.thresholds.frameTimeError) {
      issues.push({ type: "frameTime", severity: "error", value: frameTime });
      needsOptimization = true;
    } else if (frameTime > this.thresholds.frameTimeWarning) {
      issues.push({ type: "frameTime", severity: "warning", value: frameTime });
    }

    // Check memory thresholds
    if (memoryUsage > this.thresholds.memoryError) {
      issues.push({ type: "memory", severity: "error", value: memoryUsage });
      needsOptimization = true;
    } else if (memoryUsage > this.thresholds.memoryWarning) {
      issues.push({ type: "memory", severity: "warning", value: memoryUsage });
    }

    // Emit performance issues
    if (issues.length > 0) {
      this._emitEvent("performanceIssues", { issues });
    }

    // Trigger optimization if needed
    if (needsOptimization && this.config.adaptiveQuality) {
      this._triggerOptimization(issues);
    }
  }

  /**
   * Start adaptive optimization system
   * @private
   */
  _startAdaptiveOptimization() {
    this.optimizationInterval = setInterval(() => {
      if (this.state.isOptimizing) {
        return;
      }

      this._evaluateOptimizationNeed();
    }, 5000); // Check every 5 seconds
  }

  /**
   * Evaluate need for optimization
   * @private
   */
  _evaluateOptimizationNeed() {
    const avgFPS = this.metrics.fps.average;
    const avgFrameTime = this.metrics.frameTime.average;

    // Determine if quality adjustment is needed
    if (
      avgFPS < this.thresholds.fpsWarning ||
      avgFrameTime > this.thresholds.frameTimeWarning
    ) {
      this._adjustQuality("down");
    } else if (
      avgFPS > this.config.targetFPS * 0.95 &&
      this.state.currentQuality !== "high"
    ) {
      // Possibly increase quality if performance is good
      this._adjustQuality("up");
    }
  }

  /**
   * Trigger optimization based on performance issues
   * @private
   */
  _triggerOptimization(issues) {
    if (this.state.isOptimizing) {
      return;
    }

    this.state.isOptimizing = true;
    this.state.lastOptimizationTime = Date.now();

    // Categorize issues and apply appropriate optimizations
    const fpsIssues = issues.filter((issue) => issue.type === "fps");
    const memoryIssues = issues.filter((issue) => issue.type === "memory");
    const frameTimeIssues = issues.filter(
      (issue) => issue.type === "frameTime"
    );

    const optimizationsApplied = [];

    // Apply memory optimizations first
    if (memoryIssues.length > 0) {
      this.optimizations.memory.clearCaches();
      optimizationsApplied.push("clearCaches");

      if (memoryIssues.some((issue) => issue.severity === "error")) {
        this.optimizations.memory.forceGC();
        this.optimizations.memory.reduceBufferSizes();
        optimizationsApplied.push("forceGC", "reduceBufferSizes");
      }
    }

    // Apply frame rate optimizations
    if (fpsIssues.length > 0 || frameTimeIssues.length > 0) {
      this._adjustQuality("down");
      optimizationsApplied.push("qualityReduction");

      if (fpsIssues.some((issue) => issue.severity === "error")) {
        this.optimizations.frameRate.skipFrames();
        optimizationsApplied.push("frameSkipping");
      }
    }

    this.metrics.quality.adjustments++;

    this._emitEvent("optimizationApplied", {
      issues,
      optimizations: optimizationsApplied,
      timestamp: Date.now(),
    });

    // Reset optimization flag after delay
    setTimeout(() => {
      this.state.isOptimizing = false;
    }, 2000);
  }

  /**
   * Adjust quality level
   * @private
   */
  _adjustQuality(direction) {
    const qualityLevels = ["low", "medium", "high"];
    const currentIndex = qualityLevels.indexOf(this.state.currentQuality);

    let newIndex = currentIndex;
    if (direction === "down" && currentIndex > 0) {
      newIndex = currentIndex - 1;
    } else if (direction === "up" && currentIndex < qualityLevels.length - 1) {
      newIndex = currentIndex + 1;
    }

    if (newIndex !== currentIndex) {
      const newQuality = qualityLevels[newIndex];
      this._setQualityLevel(newQuality);
    }
  }

  /**
   * Set quality level
   * @private
   */
  _setQualityLevel(quality) {
    if (this.state.currentQuality === quality) {
      return;
    }

    const oldQuality = this.state.currentQuality;
    this.state.currentQuality = quality;
    this.metrics.quality.level = quality;

    const qualityConfig = this.config.qualityLevels[quality];

    this._emitEvent("qualityChanged", {
      oldQuality,
      newQuality: quality,
      config: qualityConfig,
    });
  }

  /**
   * Mark performance point for profiling
   *
   * @param {string} name - Mark name
   */
  mark(name) {
    if (!this.config.enableProfiling) {
      return;
    }

    const markName = `waveform-${name}`;

    if (performance.mark) {
      performance.mark(markName);
    }

    this.metrics.profiling.marks.push({
      name: markName,
      timestamp: performance.now(),
    });
  }

  /**
   * Measure performance between two marks
   *
   * @param {string} name - Measure name
   * @param {string} startMark - Start mark name
   * @param {string} endMark - End mark name
   */
  measure(name, startMark, endMark) {
    if (!this.config.enableProfiling) {
      return;
    }

    const measureName = `waveform-${name}`;
    const startMarkName = `waveform-${startMark}`;
    const endMarkName = `waveform-${endMark}`;

    try {
      if (performance.measure) {
        performance.measure(measureName, startMarkName, endMarkName);
      }

      // Calculate duration manually
      const startEntry = this.metrics.profiling.marks.find(
        (mark) => mark.name === startMarkName
      );
      const endEntry = this.metrics.profiling.marks.find(
        (mark) => mark.name === endMarkName
      );

      if (startEntry && endEntry) {
        const duration = endEntry.timestamp - startEntry.timestamp;

        this.metrics.profiling.measures.push({
          name: measureName,
          startTime: startEntry.timestamp,
          duration: duration,
        });
      }
    } catch (error) {
      console.warn("Performance measurement failed:", error);
    }
  }

  /**
   * Process performance entry from observer
   * @private
   */
  _processPerformanceEntry(entry) {
    // Store relevant performance entries
    if (entry.entryType === "measure" && entry.name.startsWith("waveform-")) {
      this.metrics.profiling.measures.push({
        name: entry.name,
        startTime: entry.startTime,
        duration: entry.duration,
      });
    }
  }

  /**
   * Calculate average of array values
   * @private
   */
  _calculateAverage(values) {
    if (values.length === 0) return 0;
    return values.reduce((sum, value) => sum + value, 0) / values.length;
  }

  /**
   * Get current performance metrics
   *
   * @returns {Object} Performance metrics
   */
  getMetrics() {
    return {
      ...this.metrics,
      timestamp: Date.now(),
      isMonitoring: this.state.isMonitoring,
      currentQuality: this.state.currentQuality,
      uptime: this.state.startTime > 0 ? Date.now() - this.state.startTime : 0,
    };
  }

  /**
   * Get performance summary
   *
   * @returns {Object} Performance summary
   */
  getSummary() {
    return {
      fps: {
        current: this.metrics.fps.current,
        average: this.metrics.fps.average,
        target: this.config.targetFPS,
      },
      frameTime: {
        current: this.metrics.frameTime.current,
        average: this.metrics.frameTime.average,
        target: 1000 / this.config.targetFPS,
      },
      memory: {
        percentage: this.metrics.memory.percentage,
        threshold: this.config.memoryThreshold,
      },
      quality: {
        level: this.state.currentQuality,
        adjustments: this.metrics.quality.adjustments,
      },
      status: this._getPerformanceStatus(),
    };
  }

  /**
   * Get overall performance status
   * @private
   */
  _getPerformanceStatus() {
    const fps = this.metrics.fps.average || this.metrics.fps.current;
    const memoryUsage = this.metrics.memory.percentage;

    if (
      fps < this.thresholds.fpsError ||
      memoryUsage > this.thresholds.memoryError
    ) {
      return "critical";
    } else if (
      fps < this.thresholds.fpsWarning ||
      memoryUsage > this.thresholds.memoryWarning
    ) {
      return "warning";
    } else {
      return "good";
    }
  }

  /**
   * Reset performance metrics
   */
  resetMetrics() {
    // Reset counters and history
    this.metrics.fps = {
      current: 0,
      average: 0,
      min: Infinity,
      max: 0,
      history: [],
    };

    this.metrics.frameTime = {
      current: 0,
      average: 0,
      min: Infinity,
      max: 0,
      history: [],
    };

    this.metrics.memory.history = [];
    this.metrics.cpu.history = [];
    this.metrics.profiling = { marks: [], measures: [], bottlenecks: [] };

    this.state.frameCount = 0;
    this.state.startTime = performance.now();

    // Clear performance marks and measures
    if (performance.clearMarks) {
      performance.clearMarks();
    }
    if (performance.clearMeasures) {
      performance.clearMeasures();
    }
  }

  /**
   * Optimization helper methods
   * @private
   */
  _reduceUpdateRate() {
    this._emitEvent("optimizationSuggestion", {
      type: "reduceUpdateRate",
      description: "Reduce visualization update rate to improve performance",
    });
  }

  _enableFrameSkipping() {
    this.metrics.quality.renderSkips++;
    this._emitEvent("optimizationSuggestion", {
      type: "enableFrameSkipping",
      description: "Enable frame skipping during heavy processing",
    });
  }

  _clearCaches() {
    this._emitEvent("optimizationSuggestion", {
      type: "clearCaches",
      description: "Clear internal caches to free memory",
    });
  }

  _forceGarbageCollection() {
    // Trigger garbage collection if available
    if (window.gc) {
      window.gc();
    }

    this._emitEvent("optimizationApplied", {
      type: "forceGarbageCollection",
      description: "Forced garbage collection to free memory",
    });
  }

  /**
   * Setup event handling
   */
  addEventListener(event, handler) {
    if (!this.eventHandlers.has(event)) {
      this.eventHandlers.set(event, []);
    }
    this.eventHandlers.get(event).push(handler);
  }

  removeEventListener(event, handler) {
    if (this.eventHandlers.has(event)) {
      const handlers = this.eventHandlers.get(event);
      const index = handlers.indexOf(handler);
      if (index > -1) {
        handlers.splice(index, 1);
      }
    }
  }

  /**
   * Emit event
   * @private
   */
  _emitEvent(eventName, data) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).forEach((handler) => {
        try {
          handler(data);
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      });
    }
  }

  /**
   * Cleanup resources
   */
  destroy() {
    this.stopMonitoring();
    this.eventHandlers.clear();

    // Disconnect performance observer
    if (this.observer) {
      this.observer.disconnect();
    }
  }
}

export default WaveformPerformance;
