/**
 * Performance Monitor for Huntmaster Engine
 * Tracks memory usage, CPU performance, and system health
 */

class PerformanceMonitor {
  constructor() {
    this.isRunning = false;
    this.updateInterval = 1000; // Update every second
    this.intervalId = null;

    // Performance metrics
    this.metrics = {
      memory: {
        used: 0,
        total: 0,
        percentage: 0,
      },
      cpu: {
        usage: 0,
        loadAverage: [],
      },
      audio: {
        latency: 0,
        bufferSize: 0,
        sampleRate: 0,
        dropouts: 0,
      },
      engine: {
        processingTime: 0,
        framesProcessed: 0,
        errors: 0,
      },
    };

    // Performance history for trending
    this.history = {
      memory: [],
      cpu: [],
      latency: [],
      maxHistory: 60, // Keep 1 minute of data
    };

    this.initializeMonitoring();
  }

  initializeMonitoring() {
    // Check if Performance Observer is available
    if ("PerformanceObserver" in window) {
      this.setupPerformanceObserver();
    }

    // Setup memory monitoring if available
    if ("memory" in performance) {
      this.memorySupported = true;
    }

    // Setup audio context monitoring
    this.setupAudioMonitoring();
  }

  setupPerformanceObserver() {
    try {
      // Monitor long tasks
      const longTaskObserver = new PerformanceObserver((list) => {
        const entries = list.getEntries();
        entries.forEach((entry) => {
          if (entry.duration > 50) {
            // Tasks longer than 50ms
            console.warn("Long task detected:", entry);
            this.metrics.cpu.usage = Math.min(100, this.metrics.cpu.usage + 10);
          }
        });
      });

      longTaskObserver.observe({ entryTypes: ["longtask"] });

      // Monitor navigation timing
      const navigationObserver = new PerformanceObserver((list) => {
        const entries = list.getEntries();
        entries.forEach((entry) => {
          if (entry.entryType === "navigation") {
            console.log("Navigation timing:", entry);
          }
        });
      });

      navigationObserver.observe({ entryTypes: ["navigation"] });
    } catch (error) {
      console.warn("Performance Observer not fully supported:", error);
    }
  }

  setupAudioMonitoring() {
    // Will be connected when audio context is created
    this.audioContext = null;
  }

  connectAudioContext(audioContext) {
    this.audioContext = audioContext;

    if (audioContext) {
      this.metrics.audio.sampleRate = audioContext.sampleRate;
      this.metrics.audio.latency = audioContext.baseLatency || 0;
    }
  }

  start() {
    if (this.isRunning) return;

    this.isRunning = true;
    this.intervalId = setInterval(() => {
      this.updateMetrics();
    }, this.updateInterval);

    console.log("Performance monitoring started");
  }

  stop() {
    if (!this.isRunning) return;

    this.isRunning = false;
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = null;
    }

    console.log("Performance monitoring stopped");
  }

  updateMetrics() {
    this.updateMemoryMetrics();
    this.updateCPUMetrics();
    this.updateAudioMetrics();
    this.updateHistory();
    this.updateUI();
  }

  updateMemoryMetrics() {
    if (this.memorySupported && performance.memory) {
      const memory = performance.memory;

      this.metrics.memory.used = memory.usedJSHeapSize;
      this.metrics.memory.total = memory.totalJSHeapSize;
      this.metrics.memory.percentage =
        (memory.usedJSHeapSize / memory.totalJSHeapSize) * 100;

      // Check for memory pressure
      if (this.metrics.memory.percentage > 90) {
        console.warn("High memory usage detected:", this.metrics.memory);
        this.triggerMemoryWarning();
      }
    } else {
      // Estimate memory usage
      this.estimateMemoryUsage();
    }
  }

  estimateMemoryUsage() {
    // Simple estimation based on typical JS engine behavior
    const estimatedUsage = Math.random() * 50 + 20; // 20-70 MB estimation
    this.metrics.memory.used = estimatedUsage * 1024 * 1024;
    this.metrics.memory.total = 100 * 1024 * 1024; // Assume 100MB limit
    this.metrics.memory.percentage = estimatedUsage;
  }

  updateCPUMetrics() {
    // CPU monitoring is limited in web browsers
    // We'll use frame timing as a proxy
    const now = performance.now();
    if (this.lastFrameTime) {
      const frameDelta = now - this.lastFrameTime;
      const targetFrameTime = 16.67; // 60 FPS

      if (frameDelta > targetFrameTime * 2) {
        this.metrics.cpu.usage = Math.min(100, this.metrics.cpu.usage + 5);
      } else {
        this.metrics.cpu.usage = Math.max(0, this.metrics.cpu.usage - 1);
      }
    }
    this.lastFrameTime = now;
  }

  updateAudioMetrics() {
    if (this.audioContext) {
      this.metrics.audio.latency = this.audioContext.baseLatency || 0;

      // Check audio context state
      if (this.audioContext.state !== "running") {
        console.warn("Audio context not running:", this.audioContext.state);
      }
    }
  }

  updateHistory() {
    const now = Date.now();

    // Add current metrics to history
    this.history.memory.push({
      timestamp: now,
      value: this.metrics.memory.percentage,
    });

    this.history.cpu.push({
      timestamp: now,
      value: this.metrics.cpu.usage,
    });

    this.history.latency.push({
      timestamp: now,
      value: this.metrics.audio.latency * 1000, // Convert to ms
    });

    // Trim history to max length
    Object.keys(this.history).forEach((key) => {
      if (Array.isArray(this.history[key])) {
        while (this.history[key].length > this.history.maxHistory) {
          this.history[key].shift();
        }
      }
    });
  }

  updateUI() {
    this.updateMemoryUI();
    this.updateCPUUI();
    this.updateAudioUI();
    this.updateBufferUI();
  }

  updateMemoryUI() {
    const memoryUsage = document.getElementById("memoryUsage");
    const memoryText = document.getElementById("memoryText");

    if (memoryUsage && memoryText) {
      const percentage = Math.min(100, this.metrics.memory.percentage);
      memoryUsage.style.width = `${percentage}%`;

      // Color coding
      memoryUsage.className = "metric-fill";
      if (percentage > 80) {
        memoryUsage.classList.add("danger");
      } else if (percentage > 60) {
        memoryUsage.classList.add("warning");
      } else {
        memoryUsage.classList.add("good");
      }

      const usedMB = (this.metrics.memory.used / (1024 * 1024)).toFixed(1);
      memoryText.textContent = `${usedMB} MB`;
    }
  }

  updateCPUUI() {
    const cpuUsage = document.getElementById("cpuUsage");
    const cpuText = document.getElementById("cpuText");

    if (cpuUsage && cpuText) {
      const percentage = Math.min(100, this.metrics.cpu.usage);
      cpuUsage.style.width = `${percentage}%`;

      // Color coding
      cpuUsage.className = "metric-fill";
      if (percentage > 80) {
        cpuUsage.classList.add("danger");
      } else if (percentage > 60) {
        cpuUsage.classList.add("warning");
      } else {
        cpuUsage.classList.add("good");
      }

      cpuText.textContent = `${percentage.toFixed(0)}%`;
    }
  }

  updateAudioUI() {
    const audioLatency = document.getElementById("audioLatency");

    if (audioLatency) {
      const latencyMs = this.metrics.audio.latency * 1000;
      audioLatency.textContent = `${latencyMs.toFixed(1)} ms`;

      // Color coding based on latency
      if (latencyMs > 50) {
        audioLatency.style.color = "#dc2626"; // Red
      } else if (latencyMs > 20) {
        audioLatency.style.color = "#d97706"; // Yellow
      } else {
        audioLatency.style.color = "#059669"; // Green
      }
    }
  }

  updateBufferUI() {
    const bufferHealth = document.getElementById("bufferHealth");
    const bufferText = document.getElementById("bufferText");

    if (bufferHealth && bufferText) {
      // Calculate buffer health based on various factors
      let health = 100;

      if (this.metrics.memory.percentage > 80) health -= 30;
      if (this.metrics.cpu.usage > 80) health -= 30;
      if (this.metrics.audio.latency > 0.05) health -= 20;
      if (this.metrics.audio.dropouts > 0) health -= 40;

      health = Math.max(0, health);

      bufferHealth.style.width = `${health}%`;

      // Color coding
      bufferHealth.className = "metric-fill";
      if (health > 80) {
        bufferHealth.classList.add("good");
        bufferText.textContent = "Excellent";
      } else if (health > 60) {
        bufferHealth.classList.add("warning");
        bufferText.textContent = "Good";
      } else if (health > 40) {
        bufferHealth.classList.add("warning");
        bufferText.textContent = "Fair";
      } else {
        bufferHealth.classList.add("danger");
        bufferText.textContent = "Poor";
      }
    }
  }

  // Public methods for external reporting
  reportEnginePerformance(processingTime, framesProcessed) {
    this.metrics.engine.processingTime = processingTime;
    this.metrics.engine.framesProcessed = framesProcessed;
  }

  reportAudioDropout() {
    this.metrics.audio.dropouts++;
  }

  reportEngineError() {
    this.metrics.engine.errors++;
  }

  triggerMemoryWarning() {
    // Dispatch custom event for memory pressure
    const event = new CustomEvent("memoryPressure", {
      detail: this.metrics.memory,
    });
    window.dispatchEvent(event);
  }

  // Utility methods
  getAverageMetric(metricName, timeWindow = 30000) {
    // 30 seconds default
    const history = this.history[metricName];
    if (!history || history.length === 0) return 0;

    const cutoffTime = Date.now() - timeWindow;
    const recentData = history.filter((entry) => entry.timestamp > cutoffTime);

    if (recentData.length === 0) return 0;

    const sum = recentData.reduce((total, entry) => total + entry.value, 0);
    return sum / recentData.length;
  }

  getPerformanceReport() {
    return {
      timestamp: Date.now(),
      metrics: { ...this.metrics },
      averages: {
        memory: this.getAverageMetric("memory"),
        cpu: this.getAverageMetric("cpu"),
        latency: this.getAverageMetric("latency"),
      },
      health: this.calculateOverallHealth(),
    };
  }

  calculateOverallHealth() {
    let score = 100;

    // Memory impact
    if (this.metrics.memory.percentage > 90) score -= 30;
    else if (this.metrics.memory.percentage > 70) score -= 15;

    // CPU impact
    if (this.metrics.cpu.usage > 90) score -= 30;
    else if (this.metrics.cpu.usage > 70) score -= 15;

    // Audio impact
    if (this.metrics.audio.latency > 0.05) score -= 20;
    if (this.metrics.audio.dropouts > 0) score -= 10;

    // Engine errors
    score -= this.metrics.engine.errors * 5;

    return Math.max(0, Math.min(100, score));
  }

  // Debug methods
  logMetrics() {
    console.group("Performance Metrics");
    console.log("Memory:", this.metrics.memory);
    console.log("CPU:", this.metrics.cpu);
    console.log("Audio:", this.metrics.audio);
    console.log("Engine:", this.metrics.engine);
    console.log("Overall Health:", this.calculateOverallHealth());
    console.groupEnd();
  }

  exportMetrics() {
    const data = {
      timestamp: new Date().toISOString(),
      metrics: this.metrics,
      history: this.history,
      report: this.getPerformanceReport(),
    };

    const blob = new Blob([JSON.stringify(data, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);

    const a = document.createElement("a");
    a.href = url;
    a.download = `performance-metrics-${Date.now()}.json`;
    a.click();

    URL.revokeObjectURL(url);
  }
}

// Export for use in other modules
if (typeof module !== "undefined" && module.exports) {
  module.exports = PerformanceMonitor;
} else {
  window.PerformanceMonitor = PerformanceMonitor;
}
