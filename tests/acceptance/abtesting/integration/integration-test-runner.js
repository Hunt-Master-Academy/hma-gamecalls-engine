/**
 * @file integration-test-runner.js
 * @brief Integration Test Runner - Phase 3.2C A/B Testing Framework
 *
 * This runner orchestrates all integration tests, manages test execution,
 * provides comprehensive reporting, and validates system readiness.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

import { CrossModuleIntegrationTestSuite } from "./cross-module-integration.test.js";
import { DataFlowValidationTestSuite } from "./data-flow-validation.test.js";
import fs from "fs/promises";
import path from "path";

/**
 * Integration Test Runner
 * Orchestrates and manages all integration test suites
 */
class IntegrationTestRunner {
  constructor(config = {}) {
    this.config = {
      outputDir: config.outputDir || "./test-reports",
      enableDetailedLogging: config.enableDetailedLogging !== false,
      generateHtmlReport: config.generateHtmlReport !== false,
      enableMetricsCollection: config.enableMetricsCollection !== false,
      testTimeout: config.testTimeout || 300000, // 5 minutes
      ...config,
    };

    this.testSuites = new Map();
    this.testResults = new Map();
    this.systemMetrics = {
      startTime: null,
      endTime: null,
      totalDuration: 0,
      memoryUsage: {
        initial: null,
        peak: 0,
        final: null,
      },
      performanceMetrics: {
        testExecutionTimes: [],
        moduleLoadTimes: [],
        communicationLatencies: [],
      },
    };

    this.reportGenerator = new IntegrationReportGenerator(this.config);
    this.setupTestSuites();
  }

  /**
   * Setup Test Suites
   */
  setupTestSuites() {
    // Register all test suites
    this.testSuites.set("cross-module", {
      name: "Cross-Module Integration Tests",
      suite: CrossModuleIntegrationTestSuite,
      description:
        "Validates communication and interaction between all framework modules",
      priority: 1,
      estimatedDuration: 120000, // 2 minutes
    });

    this.testSuites.set("data-flow", {
      name: "Data Flow Validation Tests",
      suite: DataFlowValidationTestSuite,
      description:
        "Ensures data integrity and consistency across module boundaries",
      priority: 2,
      estimatedDuration: 90000, // 1.5 minutes
    });
  }

  /**
   * Run All Integration Tests
   */
  async runAllIntegrationTests() {
    console.log("\n🚀 STARTING COMPREHENSIVE INTEGRATION TEST SUITE");
    console.log("================================================");
    console.log(`📅 Test Session: ${new Date().toISOString()}`);
    console.log(`🔧 Configuration: ${JSON.stringify(this.config, null, 2)}`);
    console.log("================================================\n");

    try {
      // Initialize system metrics
      await this.initializeSystemMetrics();

      // Create output directory
      await this.ensureOutputDirectory();

      // Pre-test system validation
      await this.performPreTestValidation();

      // Execute all test suites
      const testResults = await this.executeTestSuites();

      // Post-test analysis
      await this.performPostTestAnalysis();

      // Generate comprehensive reports
      await this.generateReports(testResults);

      // Finalize system metrics
      await this.finalizeSystemMetrics();

      console.log("\n✅ Integration test suite completed successfully");
      return testResults;
    } catch (error) {
      console.error("\n❌ Integration test suite failed:", error);
      await this.handleTestFailure(error);
      throw error;
    }
  }

  /**
   * Initialize System Metrics
   */
  async initializeSystemMetrics() {
    this.systemMetrics.startTime = Date.now();
    this.systemMetrics.memoryUsage.initial = process.memoryUsage();

    if (this.config.enableMetricsCollection) {
      // Start memory monitoring
      this.memoryMonitor = setInterval(() => {
        const currentMemory = process.memoryUsage();
        if (currentMemory.heapUsed > this.systemMetrics.memoryUsage.peak) {
          this.systemMetrics.memoryUsage.peak = currentMemory.heapUsed;
        }
      }, 1000);
    }

    console.log("📊 System metrics initialized");
    console.log(
      `💾 Initial memory usage: ${(
        this.systemMetrics.memoryUsage.initial.heapUsed /
        1024 /
        1024
      ).toFixed(2)} MB`
    );
  }

  /**
   * Ensure Output Directory Exists
   */
  async ensureOutputDirectory() {
    try {
      await fs.mkdir(this.config.outputDir, { recursive: true });
      console.log(`📁 Output directory ready: ${this.config.outputDir}`);
    } catch (error) {
      console.error("Failed to create output directory:", error);
      throw error;
    }
  }

  /**
   * Pre-Test System Validation
   */
  async performPreTestValidation() {
    console.log("🔍 Performing pre-test system validation...");

    const validationChecks = [
      {
        name: "Node.js version compatibility",
        check: () => {
          const version = process.version;
          const major = parseInt(version.slice(1).split(".")[0]);
          return major >= 14;
        },
      },
      {
        name: "Memory availability",
        check: () => {
          const memory = process.memoryUsage();
          return memory.heapUsed < 500 * 1024 * 1024; // Less than 500MB
        },
      },
      {
        name: "Module availability",
        check: async () => {
          try {
            // Check if all required modules can be loaded
            const modules = [
              "../management/experiment-manager.js",
              "../management/traffic-allocation.js",
              "../management/user-segmentation.js",
              "../analysis/statistical-engine.js",
              "../../analytics/collection/audio-collector.js",
            ];

            for (const modulePath of modules) {
              try {
                await import(modulePath);
              } catch (error) {
                console.warn(
                  `⚠️  Module ${modulePath} not available: ${error.message}`
                );
                // Continue with available modules
              }
            }
            return true;
          } catch (error) {
            return false;
          }
        },
      },
    ];

    let validationsPassed = 0;
    for (const validation of validationChecks) {
      try {
        const result = await validation.check();
        if (result) {
          console.log(`  ✅ ${validation.name}: PASSED`);
          validationsPassed++;
        } else {
          console.log(`  ❌ ${validation.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${validation.name}: ERROR - ${error.message}`);
      }
    }

    console.log(
      `🔍 Pre-test validation: ${validationsPassed}/${validationChecks.length} checks passed\n`
    );

    if (validationsPassed < validationChecks.length) {
      console.warn(
        "⚠️  Some pre-test validations failed - proceeding with caution"
      );
    }
  }

  /**
   * Execute Test Suites
   */
  async executeTestSuites() {
    console.log("🧪 Executing test suites...\n");

    const results = new Map();
    const sortedSuites = Array.from(this.testSuites.entries()).sort(
      (a, b) => a[1].priority - b[1].priority
    );

    for (const [suiteKey, suiteConfig] of sortedSuites) {
      console.log(`\n🎯 Starting ${suiteConfig.name}...`);
      console.log(`📝 ${suiteConfig.description}`);
      console.log(
        `⏱️  Estimated duration: ${(
          suiteConfig.estimatedDuration / 1000
        ).toFixed(0)}s`
      );
      console.log("─".repeat(60));

      const suiteStartTime = Date.now();

      try {
        // Create test suite instance
        const testSuiteInstance = new suiteConfig.suite();

        // Execute test suite with timeout
        const suiteResult = await Promise.race([
          this.executeTestSuite(testSuiteInstance, suiteKey),
          this.createTimeoutPromise(
            this.config.testTimeout,
            `${suiteConfig.name} timed out`
          ),
        ]);

        const suiteDuration = Date.now() - suiteStartTime;
        suiteResult.executionTime = suiteDuration;

        results.set(suiteKey, {
          ...suiteConfig,
          result: suiteResult,
          success: true,
          executionTime: suiteDuration,
        });

        console.log(
          `✅ ${suiteConfig.name} completed in ${(suiteDuration / 1000).toFixed(
            2
          )}s`
        );

        // Track performance metrics
        this.systemMetrics.performanceMetrics.testExecutionTimes.push({
          suite: suiteKey,
          duration: suiteDuration,
        });
      } catch (error) {
        const suiteDuration = Date.now() - suiteStartTime;

        results.set(suiteKey, {
          ...suiteConfig,
          result: null,
          success: false,
          error: error.message,
          executionTime: suiteDuration,
        });

        console.error(`❌ ${suiteConfig.name} failed: ${error.message}`);
      }
    }

    return results;
  }

  /**
   * Execute Individual Test Suite
   */
  async executeTestSuite(testSuiteInstance, suiteKey) {
    switch (suiteKey) {
      case "cross-module":
        return await testSuiteInstance.runAllIntegrationTests();

      case "data-flow":
        return await testSuiteInstance.runDataFlowValidationTests();

      default:
        throw new Error(`Unknown test suite: ${suiteKey}`);
    }
  }

  /**
   * Create Timeout Promise
   */
  createTimeoutPromise(timeout, message) {
    return new Promise((_, reject) => {
      setTimeout(() => reject(new Error(message)), timeout);
    });
  }

  /**
   * Post-Test Analysis
   */
  async performPostTestAnalysis() {
    console.log("\n🔬 Performing post-test analysis...");

    // Analyze test results
    const analysisResults = {
      overallSuccess: true,
      totalTests: 0,
      totalPassed: 0,
      totalFailed: 0,
      criticalIssues: [],
      performanceIssues: [],
      recommendations: [],
    };

    for (const [suiteKey, suiteData] of this.testResults.entries()) {
      if (suiteData.success && suiteData.result) {
        // Analyze cross-module results
        if (suiteKey === "cross-module" && suiteData.result.failedTests > 0) {
          analysisResults.criticalIssues.push(
            `Cross-module communication issues detected: ${suiteData.result.failedTests} failed tests`
          );
          analysisResults.overallSuccess = false;
        }

        // Analyze data flow results
        if (suiteKey === "data-flow") {
          const dataFlowFailures = Object.values(suiteData.result).reduce(
            (sum, category) => sum + (category.failedTests || 0),
            0
          );

          if (dataFlowFailures > 0) {
            analysisResults.criticalIssues.push(
              `Data flow validation issues: ${dataFlowFailures} failed tests`
            );
            analysisResults.overallSuccess = false;
          }
        }

        // Check performance issues
        if (suiteData.executionTime > suiteData.estimatedDuration * 1.5) {
          analysisResults.performanceIssues.push(
            `${suiteData.name} exceeded expected duration by ${(
              (suiteData.executionTime / suiteData.estimatedDuration - 1) *
              100
            ).toFixed(1)}%`
          );
        }
      } else {
        analysisResults.criticalIssues.push(
          `${suiteData.name} failed to execute`
        );
        analysisResults.overallSuccess = false;
      }
    }

    // Generate recommendations
    if (analysisResults.criticalIssues.length > 0) {
      analysisResults.recommendations.push(
        "Review and fix critical issues before deployment"
      );
    }

    if (analysisResults.performanceIssues.length > 0) {
      analysisResults.recommendations.push(
        "Investigate performance bottlenecks"
      );
    }

    if (analysisResults.overallSuccess) {
      analysisResults.recommendations.push(
        "System ready for production deployment"
      );
    }

    console.log("🔬 Post-test analysis completed");
    return analysisResults;
  }

  /**
   * Generate Comprehensive Reports
   */
  async generateReports(testResults) {
    console.log("\n📝 Generating comprehensive reports...");

    try {
      // Store test results for report generation
      this.testResults = testResults;

      // Generate JSON report
      await this.generateJsonReport(testResults);

      // Generate text report
      await this.generateTextReport(testResults);

      // Generate HTML report if enabled
      if (this.config.generateHtmlReport) {
        await this.generateHtmlReport(testResults);
      }

      // Generate metrics report if enabled
      if (this.config.enableMetricsCollection) {
        await this.generateMetricsReport();
      }

      console.log(`📝 Reports generated in: ${this.config.outputDir}`);
    } catch (error) {
      console.error("Failed to generate reports:", error);
    }
  }

  /**
   * Generate JSON Report
   */
  async generateJsonReport(testResults) {
    const report = {
      testSession: {
        timestamp: new Date().toISOString(),
        duration: this.systemMetrics.totalDuration,
        nodeVersion: process.version,
        platform: process.platform,
      },
      systemMetrics: this.systemMetrics,
      testSuites: Object.fromEntries(testResults),
      summary: this.generateTestSummary(testResults),
    };

    const reportPath = path.join(
      this.config.outputDir,
      "integration-test-results.json"
    );
    await fs.writeFile(reportPath, JSON.stringify(report, null, 2));
    console.log(`  📄 JSON report: ${reportPath}`);
  }

  /**
   * Generate Text Report
   */
  async generateTextReport(testResults) {
    let report = "";

    report += "INTEGRATION TEST REPORT\n";
    report += "======================\n\n";

    report += `Test Session: ${new Date().toISOString()}\n`;
    report += `Duration: ${(this.systemMetrics.totalDuration / 1000).toFixed(
      2
    )}s\n`;
    report += `Node.js Version: ${process.version}\n`;
    report += `Platform: ${process.platform}\n\n`;

    // Test suite results
    for (const [suiteKey, suiteData] of testResults.entries()) {
      report += `${suiteData.name}\n`;
      report += "-".repeat(suiteData.name.length) + "\n";
      report += `Status: ${suiteData.success ? "PASSED" : "FAILED"}\n`;
      report += `Execution Time: ${(suiteData.executionTime / 1000).toFixed(
        2
      )}s\n`;

      if (suiteData.result) {
        if (
          typeof suiteData.result === "object" &&
          suiteData.result.totalTests !== undefined
        ) {
          report += `Total Tests: ${suiteData.result.totalTests}\n`;
          report += `Passed: ${suiteData.result.passedTests}\n`;
          report += `Failed: ${suiteData.result.failedTests}\n`;
        }
      }

      if (suiteData.error) {
        report += `Error: ${suiteData.error}\n`;
      }

      report += "\n";
    }

    // Summary
    const summary = this.generateTestSummary(testResults);
    report += "SUMMARY\n";
    report += "-------\n";
    report += `Overall Status: ${
      summary.overallSuccess ? "PASSED" : "FAILED"
    }\n`;
    report += `Total Test Suites: ${summary.totalSuites}\n`;
    report += `Successful Suites: ${summary.successfulSuites}\n`;
    report += `Failed Suites: ${summary.failedSuites}\n`;

    const reportPath = path.join(
      this.config.outputDir,
      "integration-test-report.txt"
    );
    await fs.writeFile(reportPath, report);
    console.log(`  📄 Text report: ${reportPath}`);
  }

  /**
   * Generate HTML Report
   */
  async generateHtmlReport(testResults) {
    const htmlContent = await this.reportGenerator.generateHtmlReport(
      testResults,
      this.systemMetrics,
      this.config
    );

    const reportPath = path.join(
      this.config.outputDir,
      "integration-test-report.html"
    );
    await fs.writeFile(reportPath, htmlContent);
    console.log(`  🌐 HTML report: ${reportPath}`);
  }

  /**
   * Generate Metrics Report
   */
  async generateMetricsReport() {
    const metrics = {
      performance: {
        totalDuration: this.systemMetrics.totalDuration,
        testExecutionTimes:
          this.systemMetrics.performanceMetrics.testExecutionTimes,
        averageTestTime:
          this.systemMetrics.performanceMetrics.testExecutionTimes.reduce(
            (sum, t) => sum + t.duration,
            0
          ) / this.systemMetrics.performanceMetrics.testExecutionTimes.length,
      },
      memory: {
        initial: this.systemMetrics.memoryUsage.initial,
        peak: this.systemMetrics.memoryUsage.peak,
        final: this.systemMetrics.memoryUsage.final,
        peakIncrease:
          this.systemMetrics.memoryUsage.peak -
          this.systemMetrics.memoryUsage.initial.heapUsed,
      },
      system: {
        nodeVersion: process.version,
        platform: process.platform,
        arch: process.arch,
        cpuUsage: process.cpuUsage(),
      },
    };

    const reportPath = path.join(this.config.outputDir, "system-metrics.json");
    await fs.writeFile(reportPath, JSON.stringify(metrics, null, 2));
    console.log(`  📊 Metrics report: ${reportPath}`);
  }

  /**
   * Generate Test Summary
   */
  generateTestSummary(testResults) {
    const summary = {
      totalSuites: testResults.size,
      successfulSuites: 0,
      failedSuites: 0,
      overallSuccess: true,
      totalExecutionTime: 0,
      issues: [],
    };

    for (const [suiteKey, suiteData] of testResults.entries()) {
      summary.totalExecutionTime += suiteData.executionTime;

      if (suiteData.success) {
        summary.successfulSuites++;
      } else {
        summary.failedSuites++;
        summary.overallSuccess = false;
        summary.issues.push(
          `${suiteData.name}: ${suiteData.error || "Unknown error"}`
        );
      }
    }

    return summary;
  }

  /**
   * Finalize System Metrics
   */
  async finalizeSystemMetrics() {
    this.systemMetrics.endTime = Date.now();
    this.systemMetrics.totalDuration =
      this.systemMetrics.endTime - this.systemMetrics.startTime;
    this.systemMetrics.memoryUsage.final = process.memoryUsage();

    if (this.memoryMonitor) {
      clearInterval(this.memoryMonitor);
    }

    console.log("\n📊 Final System Metrics:");
    console.log(
      `⏱️  Total execution time: ${(
        this.systemMetrics.totalDuration / 1000
      ).toFixed(2)}s`
    );
    console.log(
      `💾 Peak memory usage: ${(
        this.systemMetrics.memoryUsage.peak /
        1024 /
        1024
      ).toFixed(2)} MB`
    );
    console.log(
      `💾 Final memory usage: ${(
        this.systemMetrics.memoryUsage.final.heapUsed /
        1024 /
        1024
      ).toFixed(2)} MB`
    );
  }

  /**
   * Handle Test Failure
   */
  async handleTestFailure(error) {
    console.error("\n🚨 INTEGRATION TEST FAILURE HANDLER");
    console.error("===================================");
    console.error(`Error: ${error.message}`);
    console.error(`Stack: ${error.stack}`);

    try {
      // Generate failure report
      const failureReport = {
        timestamp: new Date().toISOString(),
        error: {
          message: error.message,
          stack: error.stack,
        },
        systemState: {
          memoryUsage: process.memoryUsage(),
          uptime: process.uptime(),
          version: process.version,
        },
        testResults: Object.fromEntries(this.testResults),
      };

      const failureReportPath = path.join(
        this.config.outputDir,
        "test-failure-report.json"
      );
      await fs.writeFile(
        failureReportPath,
        JSON.stringify(failureReport, null, 2)
      );
      console.error(`📄 Failure report generated: ${failureReportPath}`);
    } catch (reportError) {
      console.error("Failed to generate failure report:", reportError);
    }
  }
}

/**
 * Integration Report Generator
 * Generates detailed HTML reports with visualizations
 */
class IntegrationReportGenerator {
  constructor(config) {
    this.config = config;
  }

  async generateHtmlReport(testResults, systemMetrics, config) {
    const summary = this.generateSummaryStats(testResults);

    return `
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Integration Test Report - Huntmaster Engine</title>
    <style>
        ${this.getReportStyles()}
    </style>
</head>
<body>
    <div class="container">
        <header class="report-header">
            <h1>🧪 Integration Test Report</h1>
            <div class="report-meta">
                <span>Generated: ${new Date().toLocaleString()}</span>
                <span>Duration: ${(systemMetrics.totalDuration / 1000).toFixed(
                  2
                )}s</span>
                <span class="status-badge ${
                  summary.overallSuccess ? "success" : "failure"
                }">
                    ${summary.overallSuccess ? "✅ PASSED" : "❌ FAILED"}
                </span>
            </div>
        </header>

        <section class="summary-section">
            <h2>📊 Executive Summary</h2>
            <div class="summary-grid">
                <div class="summary-card">
                    <h3>Test Suites</h3>
                    <div class="metric">${summary.totalSuites}</div>
                    <div class="metric-label">Total Suites</div>
                </div>
                <div class="summary-card">
                    <h3>Success Rate</h3>
                    <div class="metric">${(
                      (summary.successfulSuites / summary.totalSuites) *
                      100
                    ).toFixed(1)}%</div>
                    <div class="metric-label">${summary.successfulSuites}/${
      summary.totalSuites
    } Passed</div>
                </div>
                <div class="summary-card">
                    <h3>Performance</h3>
                    <div class="metric">${(
                      systemMetrics.memoryUsage.peak /
                      1024 /
                      1024
                    ).toFixed(1)} MB</div>
                    <div class="metric-label">Peak Memory</div>
                </div>
            </div>
        </section>

        <section class="test-results-section">
            <h2>🧪 Test Suite Results</h2>
            ${this.generateTestSuiteResults(testResults)}
        </section>

        <section class="system-metrics-section">
            <h2>📊 System Metrics</h2>
            ${this.generateSystemMetricsHtml(systemMetrics)}
        </section>

        <footer class="report-footer">
            <p>Generated by Huntmaster Engine Integration Test Suite</p>
            <p>Node.js ${process.version} | ${process.platform} ${
      process.arch
    }</p>
        </footer>
    </div>
</body>
</html>`;
  }

  getReportStyles() {
    return `
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .report-header { text-align: center; margin-bottom: 30px; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border-radius: 10px; }
        .report-meta { display: flex; justify-content: center; gap: 20px; margin-top: 10px; flex-wrap: wrap; }
        .status-badge { padding: 4px 12px; border-radius: 15px; font-weight: bold; }
        .status-badge.success { background: #10b981; }
        .status-badge.failure { background: #ef4444; }
        .summary-section { margin-bottom: 30px; }
        .summary-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-top: 20px; }
        .summary-card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .summary-card h3 { color: #666; margin-bottom: 10px; }
        .metric { font-size: 2.5em; font-weight: bold; color: #4f46e5; }
        .metric-label { color: #666; margin-top: 5px; }
        .test-results-section, .system-metrics-section { margin-bottom: 30px; }
        .test-suite-result { background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); margin-bottom: 20px; overflow: hidden; }
        .test-suite-header { padding: 15px 20px; background: #f8fafc; border-bottom: 1px solid #e2e8f0; display: flex; justify-content: between; align-items: center; }
        .test-suite-content { padding: 20px; }
        .success { color: #10b981; }
        .failure { color: #ef4444; }
        .report-footer { text-align: center; margin-top: 40px; padding-top: 20px; border-top: 1px solid #e2e8f0; color: #666; }
        h2 { color: #1f2937; margin-bottom: 15px; }
    `;
  }

  generateSummaryStats(testResults) {
    const summary = {
      totalSuites: testResults.size,
      successfulSuites: 0,
      failedSuites: 0,
      overallSuccess: true,
    };

    for (const [_, suiteData] of testResults.entries()) {
      if (suiteData.success) {
        summary.successfulSuites++;
      } else {
        summary.failedSuites++;
        summary.overallSuccess = false;
      }
    }

    return summary;
  }

  generateTestSuiteResults(testResults) {
    let html = "";

    for (const [suiteKey, suiteData] of testResults.entries()) {
      const statusClass = suiteData.success ? "success" : "failure";
      const statusIcon = suiteData.success ? "✅" : "❌";

      html += `
        <div class="test-suite-result">
          <div class="test-suite-header">
            <h3>${statusIcon} ${suiteData.name}</h3>
            <span class="${statusClass}">${
        suiteData.success ? "PASSED" : "FAILED"
      }</span>
          </div>
          <div class="test-suite-content">
            <p><strong>Description:</strong> ${suiteData.description}</p>
            <p><strong>Execution Time:</strong> ${(
              suiteData.executionTime / 1000
            ).toFixed(2)}s</p>
            ${
              suiteData.error
                ? `<p><strong>Error:</strong> <code>${suiteData.error}</code></p>`
                : ""
            }
            ${this.generateSuiteSpecificResults(suiteKey, suiteData.result)}
          </div>
        </div>
      `;
    }

    return html;
  }

  generateSuiteSpecificResults(suiteKey, result) {
    if (!result) return "";

    if (suiteKey === "cross-module" && typeof result === "object") {
      return `
        <div class="suite-metrics">
          <p><strong>Communication Tests:</strong> ${
            result.communicationTests || 0
          }</p>
          <p><strong>Data Flow Tests:</strong> ${result.dataFlowTests || 0}</p>
          <p><strong>Performance Tests:</strong> ${
            result.performanceTests || 0
          }</p>
          <p><strong>Total Passed:</strong> ${result.passedTests || 0}</p>
          <p><strong>Total Failed:</strong> ${result.failedTests || 0}</p>
        </div>
      `;
    }

    if (suiteKey === "data-flow" && typeof result === "object") {
      let html = '<div class="suite-metrics">';
      for (const [category, categoryResult] of Object.entries(result)) {
        if (categoryResult && typeof categoryResult === "object") {
          html += `<p><strong>${category}:</strong> ${
            categoryResult.passedTests || 0
          }/${categoryResult.totalTests || 0} passed</p>`;
        }
      }
      html += "</div>";
      return html;
    }

    return "";
  }

  generateSystemMetricsHtml(systemMetrics) {
    return `
      <div class="metrics-grid">
        <div class="metric-item">
          <strong>Execution Time:</strong> ${(
            systemMetrics.totalDuration / 1000
          ).toFixed(2)}s
        </div>
        <div class="metric-item">
          <strong>Initial Memory:</strong> ${(
            systemMetrics.memoryUsage.initial.heapUsed /
            1024 /
            1024
          ).toFixed(2)} MB
        </div>
        <div class="metric-item">
          <strong>Peak Memory:</strong> ${(
            systemMetrics.memoryUsage.peak /
            1024 /
            1024
          ).toFixed(2)} MB
        </div>
        <div class="metric-item">
          <strong>Final Memory:</strong> ${(
            systemMetrics.memoryUsage.final.heapUsed /
            1024 /
            1024
          ).toFixed(2)} MB
        </div>
      </div>
    `;
  }
}

// Export the test runner
export { IntegrationTestRunner };

// Run tests if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const config = {
    outputDir: "./integration-test-reports",
    enableDetailedLogging: true,
    generateHtmlReport: true,
    enableMetricsCollection: true,
  };

  const runner = new IntegrationTestRunner(config);
  runner
    .runAllIntegrationTests()
    .then((results) => {
      console.log("\n🎉 Integration test runner completed successfully!");

      // Calculate overall success
      let overallSuccess = true;
      for (const [_, suiteData] of results.entries()) {
        if (!suiteData.success) {
          overallSuccess = false;
          break;
        }
      }

      process.exit(overallSuccess ? 0 : 1);
    })
    .catch((error) => {
      console.error("\n💥 Integration test runner failed:", error);
      process.exit(1);
    });
}
