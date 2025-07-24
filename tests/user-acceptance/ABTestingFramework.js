/**
 * @file ABTestingFramework.js
 * @brief Advanced A/B Testing and Experimentation Platform
 *
 * This framework provides comprehensive A/B testing capabilities including
 * test design, statistical analysis, automated optimization, and
 * multivariate testing for continuous product improvement.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

// TODO: Phase 3.2 - User Acceptance Testing Framework - COMPREHENSIVE FILE TODO
// =============================================================================

// TODO 3.2.26: A/B Testing Framework and Statistical Analysis
// -----------------------------------------------------------
/**
 * TODO: Implement comprehensive A/B testing framework with:
 * [ ] Statistical significance testing and power analysis
 * [ ] Multivariate testing and factorial experiments
 * [ ] Automated test allocation and traffic splitting
 * [ ] Real-time performance monitoring and alerts
 * [ ] Bayesian optimization and adaptive testing
 * [ ] Segment-based testing and personalization
 * [ ] Test result analysis and confidence intervals
 * [ ] Automated winner selection and graduation
 * [ ] Historical test performance tracking
 * [ ] Integration with analytics and monitoring systems
 */

class ABTestingFramework {
  constructor(config = {}) {
    this.config = {
      defaultSignificanceLevel: config.defaultSignificanceLevel || 0.05,
      defaultPower: config.defaultPower || 0.8,
      defaultMinimumDetectableEffect:
        config.defaultMinimumDetectableEffect || 0.05,
      maxConcurrentTests: config.maxConcurrentTests || 10,
      defaultTestDuration:
        config.defaultTestDuration || 14 * 24 * 60 * 60 * 1000, // 14 days
      enableAutoGraduation: config.enableAutoGraduation ?? true,
      enableBayesianOptimization: config.enableBayesianOptimization ?? false,
      trafficAllocationStrategy: config.trafficAllocationStrategy || "random", // 'random', 'contextual', 'adaptive'
      ...config,
    };

    // TODO: Initialize framework components
    this.testRegistry = new Map();
    this.activeTests = new Map();
    this.userAssignments = new Map();
    this.statisticalAnalyzer = new StatisticalAnalyzer(this.config);
    this.trafficAllocator = new TrafficAllocator(this.config);
    this.resultTracker = new ResultTracker(this.config);
    this.bayesianOptimizer = this.config.enableBayesianOptimization
      ? new BayesianOptimizer(this.config)
      : null;

    // TODO: Performance tracking
    this.testMetrics = new Map();
    this.conversionTracking = new Map();
    this.segmentAnalysis = new Map();

    this.isInitialized = false;
  }

  // TODO 3.2.27: Framework Initialization and Test Management
  // ---------------------------------------------------------
  async initialize() {
    if (this.isInitialized) return true;

    try {
      // TODO: Load existing tests from storage
      await this.loadTestsFromStorage();

      // TODO: Initialize statistical analyzer
      await this.statisticalAnalyzer.initialize();

      // TODO: Initialize traffic allocator
      await this.trafficAllocator.initialize();

      // TODO: Initialize result tracker
      await this.resultTracker.initialize();

      // TODO: Initialize Bayesian optimizer if enabled
      if (this.bayesianOptimizer) {
        await this.bayesianOptimizer.initialize();
      }

      // TODO: Start periodic analysis of active tests
      this.startPeriodicAnalysis();

      // TODO: Clean up expired tests
      await this.cleanupExpiredTests();

      this.isInitialized = true;
      console.log("A/B Testing Framework initialized successfully");
      return true;
    } catch (error) {
      console.error("Failed to initialize A/B Testing Framework:", error);
      return false;
    }
  }

  // TODO 3.2.28: Test Creation and Configuration
  // --------------------------------------------
  async createTest(testConfig) {
    if (!this.isInitialized) {
      throw new Error("A/B Testing Framework not initialized");
    }

    // TODO: Validate test configuration
    const validation = this.validateTestConfig(testConfig);
    if (!validation.isValid) {
      throw new Error(
        `Invalid test configuration: ${validation.errors.join(", ")}`
      );
    }

    // TODO: Check if we can run more tests
    if (this.activeTests.size >= this.config.maxConcurrentTests) {
      throw new Error("Maximum concurrent tests limit reached");
    }

    // TODO: Create test object
    const test = {
      id: this.generateTestId(),
      name: testConfig.name,
      description: testConfig.description || "",
      hypothesis: testConfig.hypothesis || "",

      // Test parameters
      variants: testConfig.variants || [
        { id: "control", name: "Control", allocation: 50 },
        { id: "treatment", name: "Treatment", allocation: 50 },
      ],

      // Statistical parameters
      significanceLevel:
        testConfig.significanceLevel || this.config.defaultSignificanceLevel,
      power: testConfig.power || this.config.defaultPower,
      minimumDetectableEffect:
        testConfig.minimumDetectableEffect ||
        this.config.defaultMinimumDetectableEffect,

      // Targeting and segmentation
      targetingRules: testConfig.targetingRules || [],
      segments: testConfig.segments || [],

      // Metrics and goals
      primaryMetric: testConfig.primaryMetric,
      secondaryMetrics: testConfig.secondaryMetrics || [],
      guardrailMetrics: testConfig.guardrailMetrics || [],

      // Timing
      startDate: testConfig.startDate || Date.now(),
      endDate:
        testConfig.endDate || Date.now() + this.config.defaultTestDuration,
      rampUpPeriod: testConfig.rampUpPeriod || 0,

      // Status and results
      status: "draft",
      results: null,
      createdAt: Date.now(),
      updatedAt: Date.now(),
      createdBy: testConfig.createdBy || "system",
    };

    // TODO: Calculate required sample size
    const sampleSize = await this.statisticalAnalyzer.calculateSampleSize(test);
    test.requiredSampleSize = sampleSize;

    // TODO: Store test
    this.testRegistry.set(test.id, test);
    await this.saveTestToStorage(test);

    console.log(`Created A/B test: ${test.name} (${test.id})`);
    return test;
  }

  async startTest(testId) {
    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    if (test.status !== "draft") {
      throw new Error(`Cannot start test in status: ${test.status}`);
    }

    // TODO: Perform pre-flight checks
    const preflightResult = await this.performPreflightChecks(test);
    if (!preflightResult.success) {
      throw new Error(
        `Preflight checks failed: ${preflightResult.errors.join(", ")}`
      );
    }

    // TODO: Initialize traffic allocation
    await this.trafficAllocator.initializeTest(test);

    // TODO: Start tracking metrics
    await this.resultTracker.startTracking(test);

    // TODO: Update test status
    test.status = "running";
    test.actualStartDate = Date.now();
    test.updatedAt = Date.now();

    this.activeTests.set(testId, test);

    // TODO: Initialize Bayesian optimization if enabled
    if (this.bayesianOptimizer && test.enableBayesianOptimization) {
      await this.bayesianOptimizer.initializeTest(test);
    }

    console.log(`Started A/B test: ${test.name} (${test.id})`);
    return test;
  }

  async stopTest(testId, reason = "manual_stop") {
    const test = this.activeTests.get(testId);
    if (!test) {
      throw new Error(`Active test not found: ${testId}`);
    }

    // TODO: Perform final analysis
    const finalResults = await this.statisticalAnalyzer.performFinalAnalysis(
      test
    );

    // TODO: Update test with results
    test.status = "stopped";
    test.stopReason = reason;
    test.actualEndDate = Date.now();
    test.results = finalResults;
    test.updatedAt = Date.now();

    // TODO: Stop tracking
    await this.resultTracker.stopTracking(testId);

    // TODO: Stop Bayesian optimization if running
    if (this.bayesianOptimizer) {
      await this.bayesianOptimizer.stopTest(testId);
    }

    // TODO: Move from active to completed
    this.activeTests.delete(testId);
    this.testRegistry.set(testId, test);

    console.log(`Stopped A/B test: ${test.name}, Reason: ${reason}`);
    return test;
  }

  // TODO 3.2.29: User Assignment and Variant Selection
  // --------------------------------------------------
  async assignUserToVariant(testId, userId, context = {}) {
    const test = this.activeTests.get(testId) || this.testRegistry.get(testId);
    if (!test || test.status !== "running") {
      return null;
    }

    // TODO: Check if user is already assigned
    const assignmentKey = `${testId}:${userId}`;
    if (this.userAssignments.has(assignmentKey)) {
      return this.userAssignments.get(assignmentKey);
    }

    // TODO: Check targeting rules
    const isEligible = await this.checkUserEligibility(test, userId, context);
    if (!isEligible) {
      return null;
    }

    // TODO: Determine variant assignment
    const variant = await this.trafficAllocator.assignVariant(
      test,
      userId,
      context
    );
    if (!variant) {
      return null;
    }

    // TODO: Create assignment record
    const assignment = {
      testId,
      userId,
      variantId: variant.id,
      assignedAt: Date.now(),
      context: this.sanitizeContext(context),
      sessionId: context.sessionId || null,
    };

    // TODO: Store assignment
    this.userAssignments.set(assignmentKey, assignment);
    await this.saveAssignmentToStorage(assignment);

    // TODO: Track assignment event
    await this.resultTracker.trackAssignment(assignment);

    // TODO: Update Bayesian optimizer if enabled
    if (this.bayesianOptimizer && test.enableBayesianOptimization) {
      await this.bayesianOptimizer.updateAssignment(test, assignment);
    }

    return assignment;
  }

  async getUserVariant(testId, userId) {
    const assignmentKey = `${testId}:${userId}`;
    const assignment = this.userAssignments.get(assignmentKey);

    if (!assignment) {
      return null;
    }

    const test = this.testRegistry.get(testId);
    if (!test) {
      return null;
    }

    const variant = test.variants.find((v) => v.id === assignment.variantId);
    return variant || null;
  }

  // TODO 3.2.30: Metrics Tracking and Conversion Events
  // ---------------------------------------------------
  async trackConversion(testId, userId, metricName, value = 1, metadata = {}) {
    const assignment = this.userAssignments.get(`${testId}:${userId}`);
    if (!assignment) {
      // User not in test, skip tracking
      return false;
    }

    const test = this.testRegistry.get(testId);
    if (!test || test.status !== "running") {
      return false;
    }

    // TODO: Create conversion event
    const conversionEvent = {
      testId,
      userId,
      variantId: assignment.variantId,
      metricName,
      value,
      metadata: this.sanitizeContext(metadata),
      timestamp: Date.now(),
      sessionId: assignment.sessionId,
    };

    // TODO: Store conversion
    const conversionKey = `${testId}:${metricName}`;
    if (!this.conversionTracking.has(conversionKey)) {
      this.conversionTracking.set(conversionKey, []);
    }
    this.conversionTracking.get(conversionKey).push(conversionEvent);

    // TODO: Track with result tracker
    await this.resultTracker.trackConversion(conversionEvent);

    // TODO: Update Bayesian optimizer if enabled
    if (this.bayesianOptimizer && test.enableBayesianOptimization) {
      await this.bayesianOptimizer.updateConversion(test, conversionEvent);
    }

    // TODO: Check for early stopping conditions
    if (this.config.enableAutoGraduation) {
      await this.checkEarlyStoppingConditions(test);
    }

    return true;
  }

  async trackMetric(testId, userId, metricName, value, timestamp = Date.now()) {
    return await this.trackConversion(testId, userId, metricName, value, {
      timestamp,
    });
  }

  // TODO 3.2.31: Statistical Analysis and Results
  // ---------------------------------------------
  async getTestResults(testId, includeInterimResults = false) {
    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    // TODO: Get current results from analyzer
    const results = await this.statisticalAnalyzer.analyzeTest(
      test,
      includeInterimResults
    );

    // TODO: Add additional context
    results.testInfo = {
      id: test.id,
      name: test.name,
      status: test.status,
      startDate: test.actualStartDate || test.startDate,
      endDate: test.actualEndDate || test.endDate,
      duration: test.actualEndDate
        ? test.actualEndDate - (test.actualStartDate || test.startDate)
        : Date.now() - (test.actualStartDate || test.startDate),
    };

    // TODO: Add sample size information
    results.sampleInfo = await this.getSampleSizeInfo(test);

    // TODO: Add segment analysis if available
    if (test.segments && test.segments.length > 0) {
      results.segmentAnalysis = await this.getSegmentAnalysis(test);
    }

    return results;
  }

  async getTestSummary(testId) {
    const results = await this.getTestResults(testId, true);

    // TODO: Create concise summary
    const summary = {
      testId: results.testInfo.id,
      testName: results.testInfo.name,
      status: results.testInfo.status,
      duration: results.testInfo.duration,

      // Key metrics
      primaryMetric: results.primaryMetric,
      significantResult: results.primaryMetric
        ? results.primaryMetric.isSignificant
        : false,
      winningVariant: results.winningVariant || null,

      // Sample information
      totalSamples: results.sampleInfo ? results.sampleInfo.total : 0,
      samplePower: results.sampleInfo ? results.sampleInfo.actualPower : 0,

      // Recommendations
      recommendation: await this.generateRecommendation(results),
      confidence: results.primaryMetric ? results.primaryMetric.confidence : 0,
    };

    return summary;
  }

  // TODO 3.2.32: Advanced Statistical Analysis
  // ------------------------------------------
  async performBayesianAnalysis(testId) {
    if (!this.bayesianOptimizer) {
      throw new Error("Bayesian optimization not enabled");
    }

    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    return await this.bayesianOptimizer.performAnalysis(test);
  }

  async calculateConfidenceIntervals(testId, confidenceLevel = 0.95) {
    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    return await this.statisticalAnalyzer.calculateConfidenceIntervals(
      test,
      confidenceLevel
    );
  }

  async performPowerAnalysis(testId) {
    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    return await this.statisticalAnalyzer.performPowerAnalysis(test);
  }

  // TODO 3.2.33: Multivariate Testing Support
  // -----------------------------------------
  async createMultivariateTest(testConfig) {
    // TODO: Extend test creation for multivariate experiments
    const mvtConfig = {
      ...testConfig,
      type: "multivariate",
      factors: testConfig.factors || [],
      interactions: testConfig.interactions || [],
    };

    // TODO: Validate multivariate configuration
    const validation = this.validateMultivariateConfig(mvtConfig);
    if (!validation.isValid) {
      throw new Error(
        `Invalid MVT configuration: ${validation.errors.join(", ")}`
      );
    }

    // TODO: Generate all variant combinations
    mvtConfig.variants = this.generateVariantCombinations(mvtConfig.factors);

    return await this.createTest(mvtConfig);
  }

  generateVariantCombinations(factors) {
    // TODO: Generate all possible combinations of factor levels
    const combinations = [];
    const factorLevels = factors.map((factor) => factor.levels);

    const generateCombos = (current, remaining) => {
      if (remaining.length === 0) {
        combinations.push(current);
        return;
      }

      const [first, ...rest] = remaining;
      for (const level of first) {
        generateCombos([...current, level], rest);
      }
    };

    generateCombos([], factorLevels);

    return combinations.map((combo, index) => ({
      id: `variant_${index}`,
      name: `Variant ${index + 1}`,
      factors: combo,
      allocation: Math.floor(100 / combinations.length),
    }));
  }

  // TODO 3.2.34: Automated Test Management
  // --------------------------------------
  async checkEarlyStoppingConditions(test) {
    if (!this.config.enableAutoGraduation) return false;

    // TODO: Get current results
    const results = await this.statisticalAnalyzer.analyzeTest(test, true);

    // TODO: Check for early stopping criteria
    const earlyStopChecks = [
      this.checkStatisticalSignificance(results),
      this.checkPracticalSignificance(results),
      this.checkSampleSizeAdequacy(results),
      this.checkGuardrailMetrics(results),
    ];

    const shouldStop = await Promise.all(earlyStopChecks);

    if (shouldStop.every((check) => check === true)) {
      // TODO: Auto-graduate test
      await this.autoGraduateTest(test, results);
      return true;
    }

    return false;
  }

  async autoGraduateTest(test, results) {
    console.log(`Auto-graduating test: ${test.name}`);

    // TODO: Determine winning variant
    const winningVariant = results.winningVariant;

    // TODO: Stop test with graduation reason
    await this.stopTest(test.id, "auto_graduated");

    // TODO: Notify stakeholders
    await this.notifyStakeholders(test, "graduated", {
      winningVariant,
      results,
    });

    // TODO: Optionally implement winning variant
    if (this.config.autoImplementWinners) {
      await this.implementWinningVariant(test, winningVariant);
    }
  }

  // TODO 3.2.35: Segment Analysis and Personalization
  // -------------------------------------------------
  async performSegmentAnalysis(testId, segments = []) {
    const test = this.testRegistry.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    const segmentResults = {};

    for (const segment of segments) {
      // TODO: Filter data by segment
      const segmentData = await this.filterTestDataBySegment(test, segment);

      // TODO: Perform analysis on segment
      const segmentAnalysis = await this.statisticalAnalyzer.analyzeTestData(
        segmentData
      );

      segmentResults[segment.name] = {
        segment,
        results: segmentAnalysis,
        sampleSize: segmentData.assignments.length,
      };
    }

    // TODO: Store segment analysis
    this.segmentAnalysis.set(testId, segmentResults);

    return segmentResults;
  }

  async getPersonalizationRecommendations(testId) {
    const segmentResults = this.segmentAnalysis.get(testId);
    if (!segmentResults) {
      return null;
    }

    const recommendations = [];

    // TODO: Analyze segment differences
    for (const [segmentName, analysis] of Object.entries(segmentResults)) {
      if (analysis.results.isSignificant) {
        recommendations.push({
          segment: analysis.segment,
          recommendedVariant: analysis.results.winningVariant,
          confidence: analysis.results.confidence,
          expectedLift: analysis.results.lift,
        });
      }
    }

    return recommendations;
  }

  // TODO 3.2.36: Test Monitoring and Alerts
  // ---------------------------------------
  startPeriodicAnalysis() {
    // TODO: Run analysis every hour for active tests
    this.analysisInterval = setInterval(async () => {
      for (const test of this.activeTests.values()) {
        try {
          await this.performPeriodicAnalysis(test);
        } catch (error) {
          console.error(
            `Error in periodic analysis for test ${test.id}:`,
            error
          );
        }
      }
    }, 60 * 60 * 1000); // 1 hour
  }

  async performPeriodicAnalysis(test) {
    // TODO: Check test health and performance
    const healthCheck = await this.checkTestHealth(test);

    if (!healthCheck.healthy) {
      await this.sendAlert(test, "test_health_issue", healthCheck);
    }

    // TODO: Check for significant results
    const results = await this.getTestResults(test.id, true);

    if (results.primaryMetric && results.primaryMetric.isSignificant) {
      await this.sendAlert(test, "significant_result", results);
    }

    // TODO: Check sample size progress
    const sampleProgress = await this.checkSampleSizeProgress(test);

    if (sampleProgress.isComplete) {
      await this.sendAlert(test, "sample_size_reached", sampleProgress);
    }
  }

  async checkTestHealth(test) {
    // TODO: Implement comprehensive test health checks
    const checks = {
      trafficAllocation: await this.checkTrafficAllocation(test),
      conversionRates: await this.checkConversionRates(test),
      sampleBalance: await this.checkSampleBalance(test),
      dataQuality: await this.checkDataQuality(test),
    };

    const healthy = Object.values(checks).every((check) => check.passed);

    return {
      healthy,
      checks,
      timestamp: Date.now(),
    };
  }

  async sendAlert(test, alertType, data) {
    const alert = {
      testId: test.id,
      testName: test.name,
      alertType,
      data,
      timestamp: Date.now(),
      severity: this.getAlertSeverity(alertType),
    };

    // TODO: Send to configured alert channels
    console.log(
      `A/B Test Alert [${alert.severity}]: ${alertType} for ${test.name}`,
      data
    );

    // TODO: Store alert for later review
    await this.storeAlert(alert);
  }

  // TODO 3.2.37: Utility Methods and Helpers
  // ----------------------------------------
  validateTestConfig(config) {
    const errors = [];

    if (!config.name) errors.push("Test name is required");
    if (!config.primaryMetric) errors.push("Primary metric is required");
    if (!config.variants || config.variants.length < 2) {
      errors.push("At least 2 variants are required");
    }

    // TODO: Validate variant allocations sum to 100
    if (config.variants) {
      const totalAllocation = config.variants.reduce(
        (sum, v) => sum + (v.allocation || 0),
        0
      );
      if (Math.abs(totalAllocation - 100) > 0.01) {
        errors.push("Variant allocations must sum to 100%");
      }
    }

    // TODO: Validate statistical parameters
    if (
      config.significanceLevel &&
      (config.significanceLevel <= 0 || config.significanceLevel >= 1)
    ) {
      errors.push("Significance level must be between 0 and 1");
    }

    return {
      isValid: errors.length === 0,
      errors,
    };
  }

  validateMultivariateConfig(config) {
    const errors = [];

    if (!config.factors || config.factors.length === 0) {
      errors.push("At least one factor is required for multivariate testing");
    }

    // TODO: Validate factors have levels
    if (config.factors) {
      for (const factor of config.factors) {
        if (!factor.levels || factor.levels.length < 2) {
          errors.push(`Factor ${factor.name} must have at least 2 levels`);
        }
      }
    }

    return {
      isValid: errors.length === 0,
      errors,
    };
  }

  async checkUserEligibility(test, userId, context) {
    // TODO: Apply targeting rules
    for (const rule of test.targetingRules) {
      const isMatch = await this.evaluateTargetingRule(rule, userId, context);
      if (!isMatch) {
        return false;
      }
    }

    return true;
  }

  async evaluateTargetingRule(rule, userId, context) {
    // TODO: Implement targeting rule evaluation
    switch (rule.type) {
      case "user_segment":
        return this.checkUserSegment(userId, rule.segment);
      case "device_type":
        return context.deviceType === rule.value;
      case "location":
        return this.checkLocation(context.location, rule.value);
      case "custom":
        return await this.evaluateCustomRule(rule, userId, context);
      default:
        return true;
    }
  }

  sanitizeContext(context) {
    // TODO: Remove sensitive information from context
    const sanitized = { ...context };
    delete sanitized.personalInfo;
    delete sanitized.privateData;
    delete sanitized.credentials;
    return sanitized;
  }

  generateTestId() {
    return `test_${Date.now()}_${Math.random().toString(36).substring(2, 8)}`;
  }

  getAlertSeverity(alertType) {
    const severityMap = {
      test_health_issue: "high",
      significant_result: "medium",
      sample_size_reached: "low",
      early_stop_triggered: "medium",
    };

    return severityMap[alertType] || "low";
  }

  // TODO 3.2.38: Storage and Persistence Methods
  // --------------------------------------------
  async loadTestsFromStorage() {
    // TODO: Load tests from persistent storage
    try {
      if (typeof localStorage !== "undefined") {
        const testsData = localStorage.getItem("ab_tests");
        if (testsData) {
          const tests = JSON.parse(testsData);
          for (const test of tests) {
            this.testRegistry.set(test.id, test);
            if (test.status === "running") {
              this.activeTests.set(test.id, test);
            }
          }
        }
      }
    } catch (error) {
      console.error("Error loading tests from storage:", error);
    }
  }

  async saveTestToStorage(test) {
    // TODO: Save test to persistent storage
    try {
      if (typeof localStorage !== "undefined") {
        const existingTests = JSON.parse(
          localStorage.getItem("ab_tests") || "[]"
        );
        const updatedTests = existingTests.filter((t) => t.id !== test.id);
        updatedTests.push(test);
        localStorage.setItem("ab_tests", JSON.stringify(updatedTests));
      }
    } catch (error) {
      console.error("Error saving test to storage:", error);
    }
  }

  async saveAssignmentToStorage(assignment) {
    // TODO: Save assignment to persistent storage
    try {
      if (typeof localStorage !== "undefined") {
        const existingAssignments = JSON.parse(
          localStorage.getItem("ab_assignments") || "[]"
        );
        existingAssignments.push(assignment);
        localStorage.setItem(
          "ab_assignments",
          JSON.stringify(existingAssignments)
        );
      }
    } catch (error) {
      console.error("Error saving assignment to storage:", error);
    }
  }

  // TODO 3.2.39: Cleanup and Resource Management
  // --------------------------------------------
  async cleanupExpiredTests() {
    const now = Date.now();
    const expiredTests = [];

    for (const [testId, test] of this.testRegistry.entries()) {
      if (test.endDate && test.endDate < now && test.status === "running") {
        expiredTests.push(test);
      }
    }

    for (const test of expiredTests) {
      await this.stopTest(test.id, "expired");
    }

    console.log(`Cleaned up ${expiredTests.length} expired tests`);
  }

  async shutdown() {
    try {
      // TODO: Stop periodic analysis
      if (this.analysisInterval) {
        clearInterval(this.analysisInterval);
        this.analysisInterval = null;
      }

      // TODO: Stop all active tests
      const activeTestIds = Array.from(this.activeTests.keys());
      for (const testId of activeTestIds) {
        await this.stopTest(testId, "framework_shutdown");
      }

      // TODO: Shutdown components
      await this.statisticalAnalyzer.shutdown();
      await this.trafficAllocator.shutdown();
      await this.resultTracker.shutdown();

      if (this.bayesianOptimizer) {
        await this.bayesianOptimizer.shutdown();
      }

      // TODO: Final save of all data
      await this.saveAllTestsToStorage();

      console.log("A/B Testing Framework shutdown completed");
    } catch (error) {
      console.error("Error during A/B Testing Framework shutdown:", error);
    }
  }

  async saveAllTestsToStorage() {
    const allTests = Array.from(this.testRegistry.values());

    try {
      if (typeof localStorage !== "undefined") {
        localStorage.setItem("ab_tests", JSON.stringify(allTests));
      }
    } catch (error) {
      console.error("Error saving all tests to storage:", error);
    }
  }
}

// TODO 3.2.40: Supporting Classes (Stubs for Complex Components)
// --------------------------------------------------------------

class StatisticalAnalyzer {
  constructor(config) {
    this.config = config;
  }

  async initialize() {
    console.log("Statistical analyzer initialized");
  }

  async calculateSampleSize(test) {
    // TODO: Implement power analysis for sample size calculation
    const effect = test.minimumDetectableEffect;
    const alpha = test.significanceLevel;
    const power = test.power;

    // Simplified calculation (real implementation would use proper statistical formulas)
    const sampleSize = Math.ceil(16 / (effect * effect));

    return {
      perVariant: sampleSize,
      total: sampleSize * test.variants.length,
      assumptions: { effect, alpha, power },
    };
  }

  async analyzeTest(test, includeInterim = false) {
    // TODO: Implement comprehensive statistical analysis
    return {
      primaryMetric: {
        name: test.primaryMetric,
        isSignificant: false,
        pValue: 0.5,
        confidence: 0.95,
        effect: 0.02,
      },
      winningVariant: null,
      sampleSizes: { control: 100, treatment: 100 },
    };
  }

  async performFinalAnalysis(test) {
    // TODO: Implement final statistical analysis
    return await this.analyzeTest(test, false);
  }

  async shutdown() {
    console.log("Statistical analyzer shutdown");
  }
}

class TrafficAllocator {
  constructor(config) {
    this.config = config;
  }

  async initialize() {
    console.log("Traffic allocator initialized");
  }

  async initializeTest(test) {
    console.log(`Initialized traffic allocation for test: ${test.id}`);
  }

  async assignVariant(test, userId, context) {
    // TODO: Implement consistent variant assignment
    const hash = this.hashUserId(userId + test.id);
    const variantIndex = hash % test.variants.length;
    return test.variants[variantIndex];
  }

  hashUserId(input) {
    let hash = 0;
    for (let i = 0; i < input.length; i++) {
      const char = input.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash = hash & hash;
    }
    return Math.abs(hash);
  }

  async shutdown() {
    console.log("Traffic allocator shutdown");
  }
}

class ResultTracker {
  constructor(config) {
    this.config = config;
  }

  async initialize() {
    console.log("Result tracker initialized");
  }

  async startTracking(test) {
    console.log(`Started tracking for test: ${test.id}`);
  }

  async stopTracking(testId) {
    console.log(`Stopped tracking for test: ${testId}`);
  }

  async trackAssignment(assignment) {
    // TODO: Track user assignment
    console.log(
      `Tracked assignment: ${assignment.userId} -> ${assignment.variantId}`
    );
  }

  async trackConversion(conversionEvent) {
    // TODO: Track conversion event
    console.log(
      `Tracked conversion: ${conversionEvent.metricName} = ${conversionEvent.value}`
    );
  }

  async shutdown() {
    console.log("Result tracker shutdown");
  }
}

class BayesianOptimizer {
  constructor(config) {
    this.config = config;
  }

  async initialize() {
    console.log("Bayesian optimizer initialized");
  }

  async initializeTest(test) {
    console.log(`Initialized Bayesian optimization for test: ${test.id}`);
  }

  async updateAssignment(test, assignment) {
    // TODO: Update Bayesian model with assignment
  }

  async updateConversion(test, conversionEvent) {
    // TODO: Update Bayesian model with conversion
  }

  async performAnalysis(test) {
    // TODO: Perform Bayesian analysis
    return {
      posteriorDistributions: {},
      credibleIntervals: {},
      probabilityOfSuperiority: {},
    };
  }

  async stopTest(testId) {
    console.log(`Stopped Bayesian optimization for test: ${testId}`);
  }

  async shutdown() {
    console.log("Bayesian optimizer shutdown");
  }
}

export {
  ABTestingFramework,
  StatisticalAnalyzer,
  TrafficAllocator,
  ResultTracker,
  BayesianOptimizer,
};
