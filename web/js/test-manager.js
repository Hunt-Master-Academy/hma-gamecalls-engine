/**
 * Test Manager for Huntmaster Engine User Testing
 * Coordinates test sessions, manages test data, and handles test workflows
 */

class TestManager {
  constructor() {
    this.activeTests = new Map();
    this.completedTests = [];
    this.testTemplates = new Map();
    this.currentSession = null;

    this.initializeTestTemplates();
  }

  initializeTestTemplates() {
    // Define standard test templates
    this.testTemplates.set("similarity_test", {
      name: "Similarity Test",
      description: "Test audio similarity against master calls",
      duration: 30000, // 30 seconds
      parameters: {
        minSimilarity: 0.3,
        maxProcessingTime: 100, // ms
        sampleRate: 44100,
        frameSize: 1024,
      },
      metrics: ["similarity_score", "processing_time", "confidence"],
    });

    this.testTemplates.set("performance_test", {
      name: "Performance Test",
      description: "Measure engine performance under load",
      duration: 60000, // 1 minute
      parameters: {
        maxLatency: 50, // ms
        maxMemoryUsage: 80, // percentage
        targetFrameRate: 60,
      },
      metrics: ["latency", "memory_usage", "cpu_usage", "frame_rate"],
    });

    this.testTemplates.set("stress_test", {
      name: "Stress Test",
      description: "Test engine stability under continuous load",
      duration: 300000, // 5 minutes
      parameters: {
        continuousAudio: true,
        memoryPressure: true,
        concurrentSessions: 3,
      },
      metrics: ["stability", "memory_leaks", "processing_consistency"],
    });

    this.testTemplates.set("audio_quality_test", {
      name: "Audio Quality Test",
      description: "Test with various audio quality and formats",
      duration: 120000, // 2 minutes
      parameters: {
        sampleRates: [22050, 44100, 48000],
        bitDepths: [16, 24, 32],
        formats: ["wav", "mp3", "ogg"],
      },
      metrics: ["format_compatibility", "quality_consistency"],
    });
  }

  createTestSession(templateId, customParameters = {}) {
    const template = this.testTemplates.get(templateId);
    if (!template) {
      throw new Error(`Test template '${templateId}' not found`);
    }

    const session = {
      id: this.generateSessionId(),
      templateId: templateId,
      name: template.name,
      description: template.description,
      status: "created",
      startTime: null,
      endTime: null,
      duration: template.duration,
      parameters: { ...template.parameters, ...customParameters },
      metrics: template.metrics,
      results: [],
      errors: [],
      performance: {
        startMemory: 0,
        endMemory: 0,
        peakMemory: 0,
        avgProcessingTime: 0,
        totalFrames: 0,
      },
    };

    this.activeTests.set(session.id, session);
    return session;
  }

  startTestSession(sessionId) {
    const session = this.activeTests.get(sessionId);
    if (!session) {
      throw new Error(`Test session '${sessionId}' not found`);
    }

    if (session.status !== "created") {
      throw new Error(
        `Test session '${sessionId}' cannot be started (current status: ${session.status})`
      );
    }

    session.status = "running";
    session.startTime = Date.now();
    session.performance.startMemory = this.getCurrentMemoryUsage();

    this.currentSession = session;

    // Set up automatic completion
    setTimeout(() => {
      if (session.status === "running") {
        this.completeTestSession(sessionId);
      }
    }, session.duration);

    console.log(`Test session '${session.name}' started`);
    return session;
  }

  addTestResult(sessionId, result) {
    const session = this.activeTests.get(sessionId);
    if (!session) {
      console.error(`Test session '${sessionId}' not found for result`);
      return;
    }

    if (session.status !== "running") {
      console.warn(`Adding result to non-running session '${sessionId}'`);
    }

    const timestampedResult = {
      ...result,
      timestamp: Date.now(),
      relativeTime: Date.now() - session.startTime,
    };

    session.results.push(timestampedResult);

    // Update performance metrics
    if (result.processingTime) {
      const frameCount = session.performance.totalFrames + 1;
      session.performance.avgProcessingTime =
        (session.performance.avgProcessingTime *
          session.performance.totalFrames +
          result.processingTime) /
        frameCount;
      session.performance.totalFrames = frameCount;
    }

    // Check memory usage
    const currentMemory = this.getCurrentMemoryUsage();
    session.performance.peakMemory = Math.max(
      session.performance.peakMemory,
      currentMemory
    );

    return timestampedResult;
  }

  addTestError(sessionId, error) {
    const session = this.activeTests.get(sessionId);
    if (!session) {
      console.error(`Test session '${sessionId}' not found for error`);
      return;
    }

    const timestampedError = {
      ...error,
      timestamp: Date.now(),
      relativeTime: Date.now() - session.startTime,
    };

    session.errors.push(timestampedError);
    console.error("Test error:", timestampedError);
  }

  completeTestSession(sessionId) {
    const session = this.activeTests.get(sessionId);
    if (!session) {
      throw new Error(`Test session '${sessionId}' not found`);
    }

    session.status = "completed";
    session.endTime = Date.now();
    session.performance.endMemory = this.getCurrentMemoryUsage();

    // Calculate final metrics
    this.calculateSessionMetrics(session);

    // Move to completed tests
    this.completedTests.push(session);
    this.activeTests.delete(sessionId);

    if (this.currentSession && this.currentSession.id === sessionId) {
      this.currentSession = null;
    }

    console.log(`Test session '${session.name}' completed`);
    return session;
  }

  abortTestSession(sessionId, reason = "User aborted") {
    const session = this.activeTests.get(sessionId);
    if (!session) {
      throw new Error(`Test session '${sessionId}' not found`);
    }

    session.status = "aborted";
    session.endTime = Date.now();
    session.abortReason = reason;

    this.addTestError(sessionId, {
      type: "session_aborted",
      message: reason,
    });

    // Move to completed tests for analysis
    this.completedTests.push(session);
    this.activeTests.delete(sessionId);

    if (this.currentSession && this.currentSession.id === sessionId) {
      this.currentSession = null;
    }

    console.log(`Test session '${session.name}' aborted: ${reason}`);
    return session;
  }

  calculateSessionMetrics(session) {
    const results = session.results;
    if (results.length === 0) {
      console.warn(`No results found for session '${session.id}'`);
      return;
    }

    // Calculate similarity metrics
    if (session.metrics.includes("similarity_score")) {
      const similarities = results
        .filter((r) => r.score !== undefined)
        .map((r) => r.score);
      if (similarities.length > 0) {
        session.calculatedMetrics = session.calculatedMetrics || {};
        session.calculatedMetrics.similarity = {
          average:
            similarities.reduce((a, b) => a + b, 0) / similarities.length,
          min: Math.min(...similarities),
          max: Math.max(...similarities),
          stdDev: this.calculateStandardDeviation(similarities),
        };
      }
    }

    // Calculate processing time metrics
    if (session.metrics.includes("processing_time")) {
      const processingTimes = results
        .filter((r) => r.processingTime !== undefined)
        .map((r) => r.processingTime);
      if (processingTimes.length > 0) {
        session.calculatedMetrics = session.calculatedMetrics || {};
        session.calculatedMetrics.processingTime = {
          average:
            processingTimes.reduce((a, b) => a + b, 0) / processingTimes.length,
          min: Math.min(...processingTimes),
          max: Math.max(...processingTimes),
          p95: this.calculatePercentile(processingTimes, 95),
          p99: this.calculatePercentile(processingTimes, 99),
        };
      }
    }

    // Calculate success rate
    const successCount = results.filter((r) => r.success === true).length;
    session.calculatedMetrics = session.calculatedMetrics || {};
    session.calculatedMetrics.successRate = successCount / results.length;

    // Calculate error rate
    session.calculatedMetrics.errorRate =
      session.errors.length / Math.max(1, results.length);

    // Memory usage trend
    if (
      session.performance.peakMemory >
      session.performance.startMemory * 1.5
    ) {
      session.calculatedMetrics.memoryIssue = true;
    }
  }

  generateTestReport(sessionId) {
    const session =
      this.completedTests.find((s) => s.id === sessionId) ||
      this.activeTests.get(sessionId);

    if (!session) {
      throw new Error(`Test session '${sessionId}' not found`);
    }

    const report = {
      session: {
        id: session.id,
        name: session.name,
        description: session.description,
        status: session.status,
        duration: session.endTime ? session.endTime - session.startTime : null,
        parameters: session.parameters,
      },
      results: {
        totalResults: session.results.length,
        totalErrors: session.errors.length,
        metrics: session.calculatedMetrics || {},
      },
      performance: session.performance,
      timeline: this.generateTimeline(session),
      recommendations: this.generateRecommendations(session),
    };

    return report;
  }

  generateTimeline(session) {
    const timeline = [];

    // Add start event
    timeline.push({
      time: 0,
      type: "session_start",
      description: "Test session started",
    });

    // Add significant results
    session.results.forEach((result, index) => {
      if (result.score > 0.8 || result.processingTime > 100) {
        timeline.push({
          time: result.relativeTime,
          type: "significant_result",
          description: `High similarity (${(result.score * 100).toFixed(
            1
          )}%) or slow processing (${result.processingTime.toFixed(1)}ms)`,
          data: result,
        });
      }
    });

    // Add errors
    session.errors.forEach((error) => {
      timeline.push({
        time: error.relativeTime,
        type: "error",
        description: error.message,
        data: error,
      });
    });

    // Add end event
    if (session.endTime) {
      timeline.push({
        time: session.endTime - session.startTime,
        type: "session_end",
        description: `Test session ${session.status}`,
      });
    }

    return timeline.sort((a, b) => a.time - b.time);
  }

  generateRecommendations(session) {
    const recommendations = [];
    const metrics = session.calculatedMetrics || {};

    // Performance recommendations
    if (metrics.processingTime && metrics.processingTime.average > 50) {
      recommendations.push({
        type: "performance",
        priority: "high",
        message:
          "Average processing time is above 50ms. Consider optimizing audio processing pipeline.",
        metric: "processing_time",
        value: metrics.processingTime.average,
      });
    }

    // Memory recommendations
    if (session.performance.peakMemory > session.performance.startMemory * 2) {
      recommendations.push({
        type: "memory",
        priority: "high",
        message: "Memory usage doubled during test. Check for memory leaks.",
        metric: "memory_usage",
        value: session.performance.peakMemory,
      });
    }

    // Accuracy recommendations
    if (metrics.similarity && metrics.similarity.average < 0.5) {
      recommendations.push({
        type: "accuracy",
        priority: "medium",
        message:
          "Low average similarity scores. Consider adjusting algorithm parameters or master call quality.",
        metric: "similarity",
        value: metrics.similarity.average,
      });
    }

    // Error rate recommendations
    if (metrics.errorRate > 0.1) {
      recommendations.push({
        type: "reliability",
        priority: "high",
        message:
          "High error rate detected. Review error log for common issues.",
        metric: "error_rate",
        value: metrics.errorRate,
      });
    }

    // Success rate recommendations
    if (metrics.successRate < 0.9) {
      recommendations.push({
        type: "reliability",
        priority: "medium",
        message: "Success rate below 90%. Investigate processing failures.",
        metric: "success_rate",
        value: metrics.successRate,
      });
    }

    return recommendations;
  }

  // Utility methods
  generateSessionId() {
    return "test_" + Date.now() + "_" + Math.random().toString(36).substr(2, 9);
  }

  getCurrentMemoryUsage() {
    if (performance.memory) {
      return performance.memory.usedJSHeapSize / (1024 * 1024); // MB
    }
    return 0;
  }

  calculateStandardDeviation(values) {
    const avg = values.reduce((a, b) => a + b, 0) / values.length;
    const squareDiffs = values.map((value) => Math.pow(value - avg, 2));
    const avgSquareDiff =
      squareDiffs.reduce((a, b) => a + b, 0) / squareDiffs.length;
    return Math.sqrt(avgSquareDiff);
  }

  calculatePercentile(values, percentile) {
    const sorted = values.slice().sort((a, b) => a - b);
    const index = Math.ceil((percentile / 100) * sorted.length) - 1;
    return sorted[Math.max(0, index)];
  }

  // Export methods
  exportTestResults(format = "json") {
    const data = {
      timestamp: new Date().toISOString(),
      completedTests: this.completedTests.length,
      activeTests: this.activeTests.size,
      testTemplates: Array.from(this.testTemplates.keys()),
      results: this.completedTests.map((session) =>
        this.generateTestReport(session.id)
      ),
    };

    if (format === "json") {
      const blob = new Blob([JSON.stringify(data, null, 2)], {
        type: "application/json",
      });
      return URL.createObjectURL(blob);
    } else if (format === "csv") {
      return this.exportToCSV(data);
    }

    throw new Error(`Unsupported export format: ${format}`);
  }

  exportToCSV(data) {
    const csvRows = [];

    // Headers
    csvRows.push(
      [
        "Session ID",
        "Test Name",
        "Status",
        "Duration (ms)",
        "Results Count",
        "Error Count",
        "Success Rate",
        "Avg Similarity",
        "Avg Processing Time (ms)",
        "Peak Memory (MB)",
      ].join(",")
    );

    // Data rows
    data.results.forEach((report) => {
      csvRows.push(
        [
          report.session.id,
          `"${report.session.name}"`,
          report.session.status,
          report.session.duration || 0,
          report.results.totalResults,
          report.results.totalErrors,
          report.results.metrics.successRate || 0,
          report.results.metrics.similarity?.average || 0,
          report.results.metrics.processingTime?.average || 0,
          report.performance.peakMemory,
        ].join(",")
      );
    });

    const csvContent = csvRows.join("\n");
    const blob = new Blob([csvContent], { type: "text/csv" });
    return URL.createObjectURL(blob);
  }

  // Public getters
  getActiveTests() {
    return Array.from(this.activeTests.values());
  }

  getCompletedTests() {
    return [...this.completedTests];
  }

  getTestTemplates() {
    return Array.from(this.testTemplates.entries());
  }

  getCurrentSession() {
    return this.currentSession;
  }
}

// Export for use in other modules
if (typeof module !== "undefined" && module.exports) {
  module.exports = TestManager;
} else {
  window.TestManager = TestManager;
}
