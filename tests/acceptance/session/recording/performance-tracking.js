/**
 * Performance Tracking Module for Session Recording
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This module provides comprehensive performance metrics capture and monitoring
 * for user session recording, including real-time performance tracking,
 * resource monitoring, and optimization recommendations.
 *
 * @fileoverview Performance tracking and monitoring for session recording
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires DataValidator - For performance data validation
 * @requires PrivacyCompliance - For privacy-compliant metrics collection
 * @requires DataEncryption - For secure performance data transmission
 * @requires EventCapture - For performance event capture
 */

import { DataValidator } from "../validation/data-validator.js";
import { PrivacyCompliance } from "../recording/privacy-compliance.js";
import { DataEncryption } from "../recording/data-encryption.js";
import { EventCapture } from "./event-capture.js";

/**
 * PerformanceTracker class for comprehensive performance monitoring during session recording
 * Provides real-time performance metrics, resource monitoring, and optimization insights
 */
class PerformanceTracker {
  constructor(options = {}) {
    // TODO: Initialize performance tracking configuration
    this.config = {
      trackingInterval: options.trackingInterval || 1000, // 1 second default
      metricsBufferSize: options.metricsBufferSize || 1000,
      enableRealTimeMonitoring: options.enableRealTimeMonitoring !== false,
      enableResourceMonitoring: options.enableResourceMonitoring !== false,
      enableNetworkMonitoring: options.enableNetworkMonitoring !== false,
      enableMemoryMonitoring: options.enableMemoryMonitoring !== false,
      enableCPUMonitoring: options.enableCPUMonitoring !== false,
      enableDOMMonitoring: options.enableDOMMonitoring !== false,
      thresholds: {
        cpu: options.cpuThreshold || 80, // CPU usage threshold
        memory: options.memoryThreshold || 85, // Memory usage threshold
        network: options.networkThreshold || 5000, // Network latency threshold (ms)
        fps: options.fpsThreshold || 30, // Minimum FPS threshold
        dom: options.domThreshold || 1000, // DOM node count threshold
      },
      ...options,
    };

    // TODO: Initialize performance monitoring components
    this.validator = new DataValidator();
    this.privacy = new PrivacyCompliance();
    this.encryption = new DataEncryption();
    this.eventCapture = new EventCapture();

    // TODO: Initialize performance metrics storage
    this.metrics = {
      cpu: [],
      memory: [],
      network: [],
      rendering: [],
      dom: [],
      audio: [],
      user: [],
    };

    // TODO: Initialize monitoring state
    this.isTracking = false;
    this.trackingStartTime = null;
    this.lastTrackingTime = null;
    this.performanceObserver = null;
    this.intervalId = null;

    // TODO: Initialize performance APIs availability
    this.apis = {
      performance: typeof performance !== "undefined",
      observer: typeof PerformanceObserver !== "undefined",
      memory: !!(performance && performance.memory),
      navigation: !!(performance && performance.navigation),
      timing: !!(performance && performance.timing),
      measure: !!(performance && performance.measure),
      mark: !!(performance && performance.mark),
    };

    this.initializePerformanceTracking();
  }

  /**
   * Initialize performance tracking components and observers
   * TODO: Set up performance observers and monitoring infrastructure
   */
  initializePerformanceTracking() {
    try {
      // TODO: Initialize Performance Observer for navigation timing
      if (this.apis.observer) {
        this.setupPerformanceObserver();
      }

      // TODO: Initialize custom performance markers
      this.setupPerformanceMarkers();

      // TODO: Initialize resource monitoring
      this.setupResourceMonitoring();

      // TODO: Initialize network monitoring
      this.setupNetworkMonitoring();

      // TODO: Initialize frame rate monitoring
      this.setupFrameRateMonitoring();

      console.log("PerformanceTracker: Initialized successfully");
    } catch (error) {
      console.error("PerformanceTracker: Initialization failed:", error);
    }
  }

  /**
   * Set up Performance Observer for automatic metrics collection
   * TODO: Configure observers for various performance entry types
   */
  setupPerformanceObserver() {
    try {
      // TODO: Observe navigation timing
      this.performanceObserver = new PerformanceObserver((list) => {
        this.processPerformanceEntries(list.getEntries());
      });

      // TODO: Start observing performance entries
      this.performanceObserver.observe({
        entryTypes: [
          "navigation",
          "resource",
          "measure",
          "mark",
          "paint",
          "largest-contentful-paint",
        ],
      });

      console.log("PerformanceTracker: Performance Observer initialized");
    } catch (error) {
      console.error(
        "PerformanceTracker: Performance Observer setup failed:",
        error
      );
    }
  }

  /**
   * Set up custom performance markers for session tracking
   * TODO: Create custom markers for session milestones
   */
  setupPerformanceMarkers() {
    try {
      // TODO: Mark session start
      if (this.apis.mark) {
        performance.mark("session-recording-start");
        performance.mark("performance-tracking-init");
      }

      // TODO: Set up milestone markers
      this.milestones = {
        "session-start": null,
        "first-interaction": null,
        "audio-loaded": null,
        "ui-ready": null,
        "recording-active": null,
      };

      console.log("PerformanceTracker: Performance markers initialized");
    } catch (error) {
      console.error(
        "PerformanceTracker: Performance markers setup failed:",
        error
      );
    }
  }

  /**
   * Set up resource monitoring for memory, CPU, and other system resources
   * TODO: Implement comprehensive resource monitoring
   */
  setupResourceMonitoring() {
    try {
      // TODO: Initialize memory monitoring
      if (this.config.enableMemoryMonitoring && this.apis.memory) {
        this.setupMemoryMonitoring();
      }

      // TODO: Initialize CPU monitoring (approximation)
      if (this.config.enableCPUMonitoring) {
        this.setupCPUMonitoring();
      }

      // TODO: Initialize DOM monitoring
      if (this.config.enableDOMMonitoring) {
        this.setupDOMMonitoring();
      }

      console.log("PerformanceTracker: Resource monitoring initialized");
    } catch (error) {
      console.error(
        "PerformanceTracker: Resource monitoring setup failed:",
        error
      );
    }
  }

  /**
   * Set up memory usage monitoring
   * TODO: Monitor memory usage patterns and detect leaks
   */
  setupMemoryMonitoring() {
    // TODO: Track memory usage over time
    this.memoryTracker = {
      baseline: performance.memory ? performance.memory.usedJSHeapSize : 0,
      peak: 0,
      samples: [],
      gcDetected: false,
    };

    console.log("PerformanceTracker: Memory monitoring initialized");
  }

  /**
   * Set up CPU monitoring using timing-based approximation
   * TODO: Implement CPU usage estimation
   */
  setupCPUMonitoring() {
    // TODO: Use timing measurements to estimate CPU load
    this.cpuTracker = {
      baseline: performance.now(),
      samples: [],
      heavyTaskThreshold: 16.67, // 60fps = 16.67ms per frame
    };

    console.log("PerformanceTracker: CPU monitoring initialized");
  }

  /**
   * Set up DOM monitoring for DOM size and mutation tracking
   * TODO: Monitor DOM performance metrics
   */
  setupDOMMonitoring() {
    // TODO: Track DOM node count and mutations
    this.domTracker = {
      nodeCount: document.querySelectorAll("*").length,
      mutationCount: 0,
      maxDepth: this.calculateDOMDepth(),
    };

    // TODO: Set up mutation observer for DOM changes
    this.setupDOMObserver();

    console.log("PerformanceTracker: DOM monitoring initialized");
  }

  /**
   * Set up network monitoring for request performance
   * TODO: Monitor network requests and response times
   */
  setupNetworkMonitoring() {
    try {
      // TODO: Track network requests performance
      this.networkTracker = {
        requests: [],
        totalRequests: 0,
        failedRequests: 0,
        averageLatency: 0,
        slowRequests: [],
      };

      // TODO: Monitor fetch and XMLHttpRequest
      this.interceptNetworkRequests();

      console.log("PerformanceTracker: Network monitoring initialized");
    } catch (error) {
      console.error(
        "PerformanceTracker: Network monitoring setup failed:",
        error
      );
    }
  }

  /**
   * Set up frame rate monitoring for rendering performance
   * TODO: Monitor rendering performance and FPS
   */
  setupFrameRateMonitoring() {
    // TODO: Track frame rate and rendering performance
    this.frameTracker = {
      frames: 0,
      startTime: performance.now(),
      lastTime: performance.now(),
      fps: 0,
      minFps: Infinity,
      maxFps: 0,
    };

    // TODO: Start frame rate monitoring
    this.startFrameRateMonitoring();

    console.log("PerformanceTracker: Frame rate monitoring initialized");
  }

  /**
   * Start performance tracking for the session
   * TODO: Begin comprehensive performance monitoring
   */
  startTracking() {
    if (this.isTracking) {
      console.warn("PerformanceTracker: Already tracking");
      return;
    }

    try {
      // TODO: Mark tracking start
      this.isTracking = true;
      this.trackingStartTime = performance.now();
      this.lastTrackingTime = this.trackingStartTime;

      // TODO: Start periodic metrics collection
      this.intervalId = setInterval(() => {
        this.collectPerformanceMetrics();
      }, this.config.trackingInterval);

      // TODO: Mark session milestone
      this.markMilestone("session-start");

      // TODO: Start real-time monitoring if enabled
      if (this.config.enableRealTimeMonitoring) {
        this.startRealTimeMonitoring();
      }

      console.log("PerformanceTracker: Started tracking");
      return true;
    } catch (error) {
      console.error("PerformanceTracker: Failed to start tracking:", error);
      return false;
    }
  }

  /**
   * Stop performance tracking
   * TODO: End performance monitoring and generate summary
   */
  stopTracking() {
    if (!this.isTracking) {
      console.warn("PerformanceTracker: Not currently tracking");
      return;
    }

    try {
      // TODO: Stop periodic collection
      if (this.intervalId) {
        clearInterval(this.intervalId);
        this.intervalId = null;
      }

      // TODO: Stop performance observer
      if (this.performanceObserver) {
        this.performanceObserver.disconnect();
      }

      // TODO: Mark tracking end
      this.isTracking = false;
      const trackingDuration = performance.now() - this.trackingStartTime;

      // TODO: Generate final performance summary
      const summary = this.generatePerformanceSummary(trackingDuration);

      console.log("PerformanceTracker: Stopped tracking");
      return summary;
    } catch (error) {
      console.error("PerformanceTracker: Failed to stop tracking:", error);
      return null;
    }
  }

  /**
   * Collect current performance metrics
   * TODO: Gather comprehensive performance data
   */
  collectPerformanceMetrics() {
    try {
      const timestamp = performance.now();
      const metrics = {};

      // TODO: Collect memory metrics
      if (this.config.enableMemoryMonitoring) {
        metrics.memory = this.collectMemoryMetrics();
      }

      // TODO: Collect CPU metrics
      if (this.config.enableCPUMonitoring) {
        metrics.cpu = this.collectCPUMetrics();
      }

      // TODO: Collect network metrics
      if (this.config.enableNetworkMonitoring) {
        metrics.network = this.collectNetworkMetrics();
      }

      // TODO: Collect DOM metrics
      if (this.config.enableDOMMonitoring) {
        metrics.dom = this.collectDOMMetrics();
      }

      // TODO: Collect rendering metrics
      metrics.rendering = this.collectRenderingMetrics();

      // TODO: Collect audio-specific metrics
      metrics.audio = this.collectAudioMetrics();

      // TODO: Store metrics with timestamp
      this.storeMetrics(timestamp, metrics);

      // TODO: Check for performance thresholds
      this.checkPerformanceThresholds(metrics);

      this.lastTrackingTime = timestamp;
    } catch (error) {
      console.error("PerformanceTracker: Failed to collect metrics:", error);
    }
  }

  /**
   * Collect memory usage metrics
   * TODO: Gather detailed memory usage information
   */
  collectMemoryMetrics() {
    const metrics = {};

    try {
      // TODO: Collect JavaScript heap metrics
      if (this.apis.memory) {
        const memory = performance.memory;
        metrics.jsHeapSizeLimit = memory.jsHeapSizeLimit;
        metrics.totalJSHeapSize = memory.totalJSHeapSize;
        metrics.usedJSHeapSize = memory.usedJSHeapSize;
        metrics.memoryUsagePercent =
          (memory.usedJSHeapSize / memory.jsHeapSizeLimit) * 100;

        // TODO: Update memory tracker
        this.memoryTracker.samples.push(memory.usedJSHeapSize);
        if (memory.usedJSHeapSize > this.memoryTracker.peak) {
          this.memoryTracker.peak = memory.usedJSHeapSize;
        }

        // TODO: Detect potential garbage collection
        if (this.memoryTracker.samples.length > 1) {
          const lastSample =
            this.memoryTracker.samples[this.memoryTracker.samples.length - 2];
          if (memory.usedJSHeapSize < lastSample * 0.9) {
            metrics.gcDetected = true;
            this.memoryTracker.gcDetected = true;
          }
        }
      }
    } catch (error) {
      console.error(
        "PerformanceTracker: Memory metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Collect CPU usage metrics (approximation)
   * TODO: Estimate CPU usage based on timing measurements
   */
  collectCPUMetrics() {
    const metrics = {};

    try {
      // TODO: Measure task execution time
      const startTime = performance.now();

      // TODO: Perform timing-based CPU estimation
      const endTime = performance.now();
      const executionTime = endTime - startTime;

      // TODO: Calculate CPU usage approximation
      metrics.estimatedCPUUsage = Math.min(
        100,
        (executionTime / this.cpuTracker.heavyTaskThreshold) * 100
      );

      // TODO: Track timing samples
      this.cpuTracker.samples.push(executionTime);
      if (this.cpuTracker.samples.length > 100) {
        this.cpuTracker.samples.shift();
      }

      // TODO: Calculate average execution time
      metrics.averageExecutionTime =
        this.cpuTracker.samples.reduce((a, b) => a + b, 0) /
        this.cpuTracker.samples.length;
    } catch (error) {
      console.error(
        "PerformanceTracker: CPU metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Collect network performance metrics
   * TODO: Gather network request and response metrics
   */
  collectNetworkMetrics() {
    const metrics = {};

    try {
      // TODO: Get network request statistics
      const tracker = this.networkTracker;
      metrics.totalRequests = tracker.totalRequests;
      metrics.failedRequests = tracker.failedRequests;
      metrics.successRate =
        tracker.totalRequests > 0
          ? ((tracker.totalRequests - tracker.failedRequests) /
              tracker.totalRequests) *
            100
          : 100;

      // TODO: Calculate average latency
      if (tracker.requests.length > 0) {
        const totalLatency = tracker.requests.reduce(
          (sum, req) => sum + (req.duration || 0),
          0
        );
        metrics.averageLatency = totalLatency / tracker.requests.length;
      }

      // TODO: Count slow requests
      metrics.slowRequestCount = tracker.slowRequests.length;

      // TODO: Get connection information if available
      if (navigator.connection) {
        metrics.connectionType = navigator.connection.effectiveType;
        metrics.downlink = navigator.connection.downlink;
        metrics.rtt = navigator.connection.rtt;
      }
    } catch (error) {
      console.error(
        "PerformanceTracker: Network metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Collect DOM performance metrics
   * TODO: Gather DOM size and performance information
   */
  collectDOMMetrics() {
    const metrics = {};

    try {
      // TODO: Count DOM nodes
      const nodeCount = document.querySelectorAll("*").length;
      metrics.nodeCount = nodeCount;
      metrics.nodeCountChange = nodeCount - this.domTracker.nodeCount;

      // TODO: Calculate DOM depth
      metrics.maxDepth = this.calculateDOMDepth();

      // TODO: Track mutation count
      metrics.mutationCount = this.domTracker.mutationCount;

      // TODO: Update DOM tracker
      this.domTracker.nodeCount = nodeCount;

      // TODO: Check for DOM performance issues
      if (nodeCount > this.config.thresholds.dom) {
        metrics.domSizeWarning = true;
      }
    } catch (error) {
      console.error(
        "PerformanceTracker: DOM metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Collect rendering performance metrics
   * TODO: Gather rendering and frame rate metrics
   */
  collectRenderingMetrics() {
    const metrics = {};

    try {
      // TODO: Get current frame rate
      metrics.currentFPS = this.frameTracker.fps;
      metrics.minFPS = this.frameTracker.minFps;
      metrics.maxFPS = this.frameTracker.maxFps;

      // TODO: Check for performance issues
      if (metrics.currentFPS < this.config.thresholds.fps) {
        metrics.lowFPSWarning = true;
      }

      // TODO: Get paint timing if available
      if (this.apis.performance) {
        const paintEntries = performance.getEntriesByType("paint");
        if (paintEntries.length > 0) {
          metrics.firstPaint = paintEntries.find(
            (entry) => entry.name === "first-paint"
          )?.startTime;
          metrics.firstContentfulPaint = paintEntries.find(
            (entry) => entry.name === "first-contentful-paint"
          )?.startTime;
        }
      }
    } catch (error) {
      console.error(
        "PerformanceTracker: Rendering metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Collect audio-specific performance metrics
   * TODO: Gather audio processing performance data
   */
  collectAudioMetrics() {
    const metrics = {};

    try {
      // TODO: Get audio context metrics if available
      if (window.AudioContext || window.webkitAudioContext) {
        const audioContext = window.audioContext || window.webkitAudioContext;
        if (audioContext) {
          metrics.audioContextState = audioContext.state;
          metrics.sampleRate = audioContext.sampleRate;
          metrics.currentTime = audioContext.currentTime;
          metrics.baseLatency = audioContext.baseLatency || 0;
          metrics.outputLatency = audioContext.outputLatency || 0;
        }
      }

      // TODO: Track audio buffer performance
      metrics.audioBufferPerformance = this.getAudioBufferMetrics();

      // TODO: Track WebRTC audio metrics if applicable
      metrics.webrtcMetrics = this.getWebRTCAudioMetrics();
    } catch (error) {
      console.error(
        "PerformanceTracker: Audio metrics collection failed:",
        error
      );
    }

    return metrics;
  }

  /**
   * Process performance entries from Performance Observer
   * TODO: Handle different types of performance entries
   */
  processPerformanceEntries(entries) {
    entries.forEach((entry) => {
      try {
        // TODO: Process different entry types
        switch (entry.entryType) {
          case "navigation":
            this.processNavigationEntry(entry);
            break;
          case "resource":
            this.processResourceEntry(entry);
            break;
          case "measure":
            this.processMeasureEntry(entry);
            break;
          case "mark":
            this.processMarkEntry(entry);
            break;
          case "paint":
            this.processPaintEntry(entry);
            break;
          case "largest-contentful-paint":
            this.processLCPEntry(entry);
            break;
        }
      } catch (error) {
        console.error("PerformanceTracker: Failed to process entry:", error);
      }
    });
  }

  /**
   * Mark a performance milestone
   * TODO: Create custom performance markers for session events
   */
  markMilestone(name, details = {}) {
    try {
      // TODO: Create performance mark
      if (this.apis.mark) {
        performance.mark(`milestone-${name}`);
      }

      // TODO: Store milestone data
      this.milestones[name] = {
        timestamp: performance.now(),
        details,
        sessionTime: this.trackingStartTime
          ? performance.now() - this.trackingStartTime
          : 0,
      };

      console.log(`PerformanceTracker: Milestone '${name}' marked`);
    } catch (error) {
      console.error("PerformanceTracker: Failed to mark milestone:", error);
    }
  }

  /**
   * Generate comprehensive performance summary
   * TODO: Create detailed performance analysis report
   */
  generatePerformanceSummary(duration) {
    try {
      const summary = {
        sessionDuration: duration,
        trackingStart: this.trackingStartTime,
        milestones: { ...this.milestones },
        metrics: {
          memory: this.summarizeMemoryMetrics(),
          cpu: this.summarizeCPUMetrics(),
          network: this.summarizeNetworkMetrics(),
          dom: this.summarizeDOMMetrics(),
          rendering: this.summarizeRenderingMetrics(),
          audio: this.summarizeAudioMetrics(),
        },
        warnings: this.generateWarnings(),
        recommendations: this.generateRecommendations(),
        timestamp: new Date().toISOString(),
      };

      // TODO: Validate and encrypt summary data
      const validatedSummary = this.validator.validate(summary);
      const encryptedSummary = this.encryption.encrypt(validatedSummary);

      return {
        summary: validatedSummary,
        encrypted: encryptedSummary,
        size: JSON.stringify(validatedSummary).length,
      };
    } catch (error) {
      console.error("PerformanceTracker: Failed to generate summary:", error);
      return null;
    }
  }

  /**
   * Calculate DOM depth
   * TODO: Determine maximum DOM tree depth
   */
  calculateDOMDepth() {
    // TODO: Implement DOM depth calculation
    let maxDepth = 0;

    function getDepth(element, currentDepth = 0) {
      if (currentDepth > maxDepth) {
        maxDepth = currentDepth;
      }

      for (const child of element.children) {
        getDepth(child, currentDepth + 1);
      }
    }

    getDepth(document.documentElement);
    return maxDepth;
  }

  /**
   * Start frame rate monitoring
   * TODO: Monitor rendering frame rate continuously
   */
  startFrameRateMonitoring() {
    const updateFPS = () => {
      const now = performance.now();
      this.frameTracker.frames++;

      if (now >= this.frameTracker.lastTime + 1000) {
        this.frameTracker.fps = Math.round(
          (this.frameTracker.frames * 1000) / (now - this.frameTracker.lastTime)
        );

        if (this.frameTracker.fps < this.frameTracker.minFps) {
          this.frameTracker.minFps = this.frameTracker.fps;
        }

        if (this.frameTracker.fps > this.frameTracker.maxFps) {
          this.frameTracker.maxFps = this.frameTracker.fps;
        }

        this.frameTracker.frames = 0;
        this.frameTracker.lastTime = now;
      }

      if (this.isTracking) {
        requestAnimationFrame(updateFPS);
      }
    };

    requestAnimationFrame(updateFPS);
  }

  /**
   * Export performance data
   * TODO: Export collected metrics for analysis
   */
  exportData(format = "json") {
    try {
      const data = {
        config: this.config,
        metrics: this.metrics,
        milestones: this.milestones,
        summary: this.generatePerformanceSummary(
          this.isTracking ? performance.now() - this.trackingStartTime : 0
        ),
      };

      // TODO: Apply privacy compliance
      const compliantData = this.privacy.filterData(data);

      // TODO: Format data according to requested format
      switch (format.toLowerCase()) {
        case "json":
          return JSON.stringify(compliantData, null, 2);
        case "csv":
          return this.convertToCSV(compliantData);
        default:
          throw new Error(`Unsupported format: ${format}`);
      }
    } catch (error) {
      console.error("PerformanceTracker: Export failed:", error);
      return null;
    }
  }

  /**
   * Cleanup and destroy the performance tracker
   * TODO: Clean up all monitoring and free resources
   */
  destroy() {
    try {
      // TODO: Stop tracking if active
      if (this.isTracking) {
        this.stopTracking();
      }

      // TODO: Disconnect observers
      if (this.performanceObserver) {
        this.performanceObserver.disconnect();
        this.performanceObserver = null;
      }

      // TODO: Clear intervals
      if (this.intervalId) {
        clearInterval(this.intervalId);
        this.intervalId = null;
      }

      // TODO: Clear stored data
      this.metrics = null;
      this.milestones = null;

      console.log("PerformanceTracker: Destroyed successfully");
    } catch (error) {
      console.error("PerformanceTracker: Destruction failed:", error);
    }
  }
}

// TODO: Export the PerformanceTracker class
export { PerformanceTracker };

// TODO: Export convenience functions
export const createPerformanceTracker = (options) =>
  new PerformanceTracker(options);
export const trackSessionPerformance = (sessionId, options) => {
  const tracker = new PerformanceTracker({ sessionId, ...options });
  tracker.startTracking();
  return tracker;
};

// TODO: Export performance utilities
export const PerformanceUtils = {
  formatBytes: (bytes) => {
    if (bytes === 0) return "0 Bytes";
    const k = 1024;
    const sizes = ["Bytes", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i];
  },

  formatDuration: (milliseconds) => {
    const seconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);

    if (hours > 0) {
      return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${seconds % 60}s`;
    } else {
      return `${seconds}s`;
    }
  },

  calculatePercentile: (values, percentile) => {
    const sorted = [...values].sort((a, b) => a - b);
    const index = Math.ceil(sorted.length * (percentile / 100)) - 1;
    return sorted[index];
  },
};

console.log("PerformanceTracker module loaded successfully");
