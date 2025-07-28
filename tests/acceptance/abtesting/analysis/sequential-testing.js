/**
 * @file sequential-testing.js
 * @brief Sequential Testing Methods Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive sequential testing methods with early stopping rules,
 * SPRT implementation, and adaptive testing for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

/**
 * SequentialTesting Class
 * Provides sequential testing methods with early stopping rules and adaptive testing
 */
export class SequentialTesting {
  constructor(config = {}) {
    // Initialize sequential testing system
    // Set up early stopping rule framework
    // Configure SPRT implementation
    // Initialize group sequential methods
    // Set up adaptive testing algorithms
    // Configure stopping boundary calculations
    // Initialize sequential monitoring
    // Set up sequential optimization
    // Configure sequential reporting
    // Initialize sequential validation

    this.config = {
      defaultAlpha: 0.05,
      defaultPower: 0.8,
      enableEarlyStoppingForSuperiority: true,
      enableEarlyStoppingForFutility: true,
      enableAdaptiveSampleSize: true,
      maxAnalyses: 5,
      minimumSampleSize: 100,
      spendingFunctionType: "obrien_fleming",
      futilityThreshold: 0.1,
      cacheSequentialResults: true,
      ...config,
    };

    this.sequentialTests = new Map();
    this.stoppingBoundaries = new Map();
    this.analysisHistory = new Map();
    this.spendingFunctions = new Map();
    this.sequentialMetrics = {
      totalTests: 0,
      earlyStoppedTests: 0,
      averageAnalyses: 0,
      averageSampleSizeReduction: 0,
      futilityStops: 0,
    };

    this.spendingFunctionTypes = new Map();
    this.stoppingRules = new Map();
    this.validators = [];

    this.initializeSpendingFunctions();
  }

  /**
   * Sequential Test Design and Setup
   */
  async designSequentialTest(testConfig) {
    // Design sequential testing procedure
    // Validate test configuration
    // Calculate stopping boundaries
    // Set up interim analysis schedule
    // Configure spending functions
    // Initialize early stopping rules
    // Set up adaptive procedures
    // Generate sequential test protocol
    // Create monitoring framework
    // Update sequential test registry

    const testId = this.generateTestId();
    const timestamp = Date.now();

    const sequentialTest = {
      id: testId,
      timestamp: timestamp,
      experimentId: testConfig.experimentId,
      testType: testConfig.testType || "two_sample_means",
      designParameters: {
        alpha: testConfig.alpha || this.config.defaultAlpha,
        power: testConfig.power || this.config.defaultPower,
        effectSize: testConfig.effectSize,
        maxAnalyses: testConfig.maxAnalyses || this.config.maxAnalyses,
        spendingFunction:
          testConfig.spendingFunction || this.config.spendingFunctionType,
      },
      stoppingBoundaries: {},
      interimAnalyses: [],
      currentAnalysis: 0,
      status: "active",
      decision: "continue",
      finalResult: null,
    };

    // Validate test configuration
    const validation = await this.validateSequentialTestConfig(sequentialTest);
    if (!validation.valid) {
      throw new Error(
        `Invalid sequential test configuration: ${validation.errors.join(", ")}`
      );
    }

    // Calculate stopping boundaries
    sequentialTest.stoppingBoundaries = await this.calculateStoppingBoundaries(
      sequentialTest
    );

    // Store sequential test
    this.sequentialTests.set(testId, sequentialTest);

    // Update metrics
    this.sequentialMetrics.totalTests++;

    return {
      testId: testId,
      sequentialTest: sequentialTest,
    };
  }

  async calculateStoppingBoundaries(sequentialTest) {
    // Calculate stopping boundaries for sequential test
    // Apply spending function approach
    // Calculate efficacy boundaries
    // Calculate futility boundaries
    // Handle different test types
    // Optimize boundary calculations
    // Validate boundary properties
    // Generate boundary documentation
    // Cache boundary calculations
    // Update boundary metrics

    const { alpha, maxAnalyses, spendingFunction } =
      sequentialTest.designParameters;
    const boundaryId = this.generateBoundaryId();

    const boundaries = {
      id: boundaryId,
      testId: sequentialTest.id,
      efficacyBoundaries: [],
      futilityBoundaries: [],
      spendingFunction: spendingFunction,
      alphaSpent: [],
      betaSpent: [],
    };

    // Get spending function
    const spendingFunctionCalculator =
      this.spendingFunctions.get(spendingFunction);
    if (!spendingFunctionCalculator) {
      throw new Error(`Unknown spending function: ${spendingFunction}`);
    }

    // Calculate boundaries for each analysis
    for (let k = 1; k <= maxAnalyses; k++) {
      const fraction = k / maxAnalyses;

      // Calculate alpha spending
      const alphaSpent = spendingFunctionCalculator.calculateAlphaSpending(
        fraction,
        alpha
      );
      boundaries.alphaSpent.push(alphaSpent);

      // Calculate efficacy boundary
      const efficacyBoundary = await this.calculateEfficacyBoundary(
        k,
        maxAnalyses,
        alphaSpent,
        sequentialTest.testType
      );
      boundaries.efficacyBoundaries.push(efficacyBoundary);

      // Calculate futility boundary if enabled
      if (this.config.enableEarlyStoppingForFutility) {
        const betaSpent = spendingFunctionCalculator.calculateBetaSpending(
          fraction,
          1 - sequentialTest.designParameters.power
        );
        boundaries.betaSpent.push(betaSpent);

        const futilityBoundary = await this.calculateFutilityBoundary(
          k,
          maxAnalyses,
          betaSpent,
          sequentialTest.testType
        );
        boundaries.futilityBoundaries.push(futilityBoundary);
      }
    }

    // Store stopping boundaries
    this.stoppingBoundaries.set(boundaryId, boundaries);

    return boundaries;
  }

  /**
   * Interim Analysis Implementation
   */
  async performInterimAnalysis(testId, analysisData) {
    // Perform interim analysis for sequential test
    // Validate analysis data
    // Calculate test statistic
    // Compare to stopping boundaries
    // Make stopping decision
    // Update test status
    // Generate interim report
    // Handle adaptive modifications
    // Update analysis history
    // Update sequential metrics

    const sequentialTest = this.sequentialTests.get(testId);
    if (!sequentialTest) {
      throw new Error(`Sequential test not found: ${testId}`);
    }

    if (sequentialTest.status !== "active") {
      throw new Error(
        `Sequential test is not active: ${sequentialTest.status}`
      );
    }

    const analysisId = this.generateAnalysisId();
    const timestamp = Date.now();
    sequentialTest.currentAnalysis++;

    const interimAnalysis = {
      id: analysisId,
      testId: testId,
      analysisNumber: sequentialTest.currentAnalysis,
      timestamp: timestamp,
      data: analysisData,
      testStatistic: 0,
      pValue: 0,
      decision: "continue",
      stoppingReason: null,
      boundaryComparison: {},
    };

    // Calculate test statistic
    interimAnalysis.testStatistic = await this.calculateTestStatistic(
      analysisData,
      sequentialTest.testType
    );

    // Calculate p-value
    interimAnalysis.pValue = await this.calculatePValue(
      interimAnalysis.testStatistic,
      sequentialTest.testType
    );

    // Compare to stopping boundaries
    const boundaryComparison = await this.compareToStoppingBoundaries(
      interimAnalysis,
      sequentialTest.stoppingBoundaries,
      sequentialTest.currentAnalysis
    );
    interimAnalysis.boundaryComparison = boundaryComparison;

    // Make stopping decision
    const stoppingDecision = await this.makeStoppingDecision(
      interimAnalysis,
      boundaryComparison,
      sequentialTest
    );
    interimAnalysis.decision = stoppingDecision.decision;
    interimAnalysis.stoppingReason = stoppingDecision.reason;

    // Update sequential test status
    if (stoppingDecision.decision !== "continue") {
      sequentialTest.status = "stopped";
      sequentialTest.decision = stoppingDecision.decision;
      sequentialTest.finalResult = interimAnalysis;

      // Update metrics
      this.sequentialMetrics.earlyStoppedTests++;
      if (stoppingDecision.reason === "futility") {
        this.sequentialMetrics.futilityStops++;
      }
    }

    // Add to analysis history
    sequentialTest.interimAnalyses.push(interimAnalysis);

    // Store analysis history
    if (!this.analysisHistory.has(testId)) {
      this.analysisHistory.set(testId, []);
    }
    this.analysisHistory.get(testId).push(interimAnalysis);

    // Update average analyses metric
    this.updateAverageAnalyses();

    return {
      analysisId: analysisId,
      interimAnalysis: interimAnalysis,
      stoppingDecision: stoppingDecision,
    };
  }

  async compareToStoppingBoundaries(analysis, boundaries, analysisNumber) {
    // Compare test statistic to stopping boundaries
    // Check efficacy boundary
    // Check futility boundary
    // Handle boundary crossing
    // Calculate boundary distances
    // Generate boundary comparison report
    // Validate boundary properties
    // Document boundary decisions
    // Update boundary statistics
    // Return comparison results

    const comparison = {
      analysisNumber: analysisNumber,
      testStatistic: analysis.testStatistic,
      efficacyBoundary: boundaries.efficacyBoundaries[analysisNumber - 1],
      futilityBoundary:
        boundaries.futilityBoundaries[analysisNumber - 1] || null,
      crossesEfficacyBoundary: false,
      crossesFutilityBoundary: false,
      distanceToEfficacy: 0,
      distanceToFutility: 0,
    };

    // Check efficacy boundary
    if (Math.abs(analysis.testStatistic) >= comparison.efficacyBoundary) {
      comparison.crossesEfficacyBoundary = true;
    }
    comparison.distanceToEfficacy =
      comparison.efficacyBoundary - Math.abs(analysis.testStatistic);

    // Check futility boundary if available
    if (comparison.futilityBoundary !== null) {
      if (Math.abs(analysis.testStatistic) <= comparison.futilityBoundary) {
        comparison.crossesFutilityBoundary = true;
      }
      comparison.distanceToFutility =
        Math.abs(analysis.testStatistic) - comparison.futilityBoundary;
    }

    return comparison;
  }

  async makeStoppingDecision(analysis, boundaryComparison, sequentialTest) {
    // Make stopping decision based on boundary comparison
    // Apply stopping rules
    // Consider efficacy stopping
    // Consider futility stopping
    // Handle maximum analyses reached
    // Apply business rules
    // Generate stopping rationale
    // Validate stopping decision
    // Document decision process
    // Update decision metrics

    const decision = {
      decision: "continue",
      reason: null,
      rationale: "",
      confidence: 0,
    };

    // Check for efficacy stopping
    if (boundaryComparison.crossesEfficacyBoundary) {
      decision.decision = "stop_for_efficacy";
      decision.reason = "efficacy";
      decision.rationale = `Test statistic ${analysis.testStatistic.toFixed(
        4
      )} crossed efficacy boundary ${boundaryComparison.efficacyBoundary.toFixed(
        4
      )}`;
      decision.confidence = 0.95;
      return decision;
    }

    // Check for futility stopping
    if (boundaryComparison.crossesFutilityBoundary) {
      decision.decision = "stop_for_futility";
      decision.reason = "futility";
      decision.rationale = `Test statistic ${analysis.testStatistic.toFixed(
        4
      )} crossed futility boundary ${boundaryComparison.futilityBoundary.toFixed(
        4
      )}`;
      decision.confidence = 0.9;
      return decision;
    }

    // Check if maximum analyses reached
    if (
      sequentialTest.currentAnalysis >=
      sequentialTest.designParameters.maxAnalyses
    ) {
      decision.decision = "stop_for_max_analyses";
      decision.reason = "max_analyses";
      decision.rationale = "Maximum number of analyses reached";
      decision.confidence = 0.85;
      return decision;
    }

    // Continue testing
    decision.rationale = "No stopping boundary crossed, continue testing";
    decision.confidence = 0.75;

    return decision;
  }

  /**
   * SPRT Implementation
   */
  async performSPRT(sprtConfig) {
    // Perform Sequential Probability Ratio Test
    // Define null and alternative hypotheses
    // Set Type I and Type II error rates
    // Calculate likelihood ratios
    // Update cumulative log-likelihood ratio
    // Compare to SPRT boundaries
    // Make sequential decisions
    // Handle continuous monitoring
    // Generate SPRT reports
    // Update SPRT metrics

    const sprtId = this.generateSPRTId();
    const timestamp = Date.now();

    const sprt = {
      id: sprtId,
      timestamp: timestamp,
      experimentId: sprtConfig.experimentId,
      nullHypothesis: sprtConfig.nullHypothesis,
      alternativeHypothesis: sprtConfig.alternativeHypothesis,
      alpha: sprtConfig.alpha || this.config.defaultAlpha,
      beta: sprtConfig.beta || 1 - this.config.defaultPower,
      upperBoundary: 0,
      lowerBoundary: 0,
      cumulativeLogLR: 0,
      observations: [],
      decision: "continue",
      status: "active",
    };

    // Calculate SPRT boundaries
    sprt.upperBoundary = Math.log((1 - sprt.beta) / sprt.alpha);
    sprt.lowerBoundary = Math.log(sprt.beta / (1 - sprt.alpha));

    return {
      sprtId: sprtId,
      sprt: sprt,
    };
  }

  async updateSPRT(sprtId, newObservation) {
    // Update SPRT with new observation
    const sprt = this.sequentialTests.get(sprtId);
    if (!sprt) {
      throw new Error(`SPRT not found: ${sprtId}`);
    }

    // Calculate likelihood ratio for new observation
    const likelihoodRatio = await this.calculateLikelihoodRatio(
      newObservation,
      sprt.nullHypothesis,
      sprt.alternativeHypothesis
    );

    // Update cumulative log-likelihood ratio
    sprt.cumulativeLogLR += Math.log(likelihoodRatio);
    sprt.observations.push({
      observation: newObservation,
      likelihoodRatio: likelihoodRatio,
      cumulativeLogLR: sprt.cumulativeLogLR,
      timestamp: Date.now(),
    });

    // Check stopping criteria
    if (sprt.cumulativeLogLR >= sprt.upperBoundary) {
      sprt.decision = "reject_null";
      sprt.status = "stopped";
    } else if (sprt.cumulativeLogLR <= sprt.lowerBoundary) {
      sprt.decision = "accept_null";
      sprt.status = "stopped";
    }

    return {
      decision: sprt.decision,
      cumulativeLogLR: sprt.cumulativeLogLR,
      status: sprt.status,
    };
  }

  /**
   * Utility Methods
   */
  initializeSpendingFunctions() {
    // Initialize spending function calculators
    this.spendingFunctions.set("obrien_fleming", {
      calculateAlphaSpending: (fraction, totalAlpha) => {
        return 2 * (1 - this.standardNormalCDF(1.96 / Math.sqrt(fraction)));
      },
      calculateBetaSpending: (fraction, totalBeta) => {
        return totalBeta * fraction;
      },
    });

    this.spendingFunctions.set("pocock", {
      calculateAlphaSpending: (fraction, totalAlpha) => {
        return totalAlpha * Math.log(1 + fraction * (Math.E - 1));
      },
      calculateBetaSpending: (fraction, totalBeta) => {
        return totalBeta * fraction;
      },
    });

    this.spendingFunctions.set("linear", {
      calculateAlphaSpending: (fraction, totalAlpha) => {
        return totalAlpha * fraction;
      },
      calculateBetaSpending: (fraction, totalBeta) => {
        return totalBeta * fraction;
      },
    });

    // Add more spending functions
  }

  generateTestId() {
    return `seq_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateBoundaryId() {
    return `boundary_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateAnalysisId() {
    return `analysis_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateSPRTId() {
    return `sprt_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  standardNormalCDF(z) {
    // Calculate standard normal CDF
    return 0.5 * (1 + this.erf(z / Math.sqrt(2)));
  }

  erf(x) {
    // Calculate error function approximation
    const a1 = 0.254829592;
    const a2 = -0.284496736;
    const a3 = 1.421413741;
    const a4 = -1.453152027;
    const a5 = 1.061405429;
    const p = 0.3275911;

    const sign = x < 0 ? -1 : 1;
    x = Math.abs(x);

    const t = 1.0 / (1.0 + p * x);
    const y =
      1.0 -
      ((((a5 * t + a4) * t + a3) * t + a2) * t + a1) * t * Math.exp(-x * x);

    return sign * y;
  }

  async validateSequentialTestConfig(test) {
    // Validate sequential test configuration
    const errors = [];

    if (!test.experimentId) {
      errors.push("Experiment ID is required");
    }

    if (test.designParameters.alpha <= 0 || test.designParameters.alpha >= 1) {
      errors.push("Alpha must be between 0 and 1");
    }

    if (test.designParameters.power <= 0 || test.designParameters.power >= 1) {
      errors.push("Power must be between 0 and 1");
    }

    if (test.designParameters.maxAnalyses <= 0) {
      errors.push("Maximum analyses must be positive");
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  updateAverageAnalyses() {
    // Update average number of analyses metric
    const completedTests = Array.from(this.sequentialTests.values()).filter(
      (test) => test.status === "stopped"
    );

    if (completedTests.length === 0) return;

    const totalAnalyses = completedTests.reduce(
      (sum, test) => sum + test.currentAnalysis,
      0
    );
    this.sequentialMetrics.averageAnalyses =
      totalAnalyses / completedTests.length;
  }

  /**
   * Analytics and Reporting
   */
  getSequentialTest(testId) {
    return this.sequentialTests.get(testId);
  }

  getStoppingBoundaries(boundaryId) {
    return this.stoppingBoundaries.get(boundaryId);
  }

  getAnalysisHistory(testId) {
    return this.analysisHistory.get(testId) || [];
  }

  getSequentialMetrics() {
    return { ...this.sequentialMetrics };
  }

  calculateEarlyStoppingRate() {
    // Calculate early stopping rate
    const total = this.sequentialMetrics.totalTests;
    const earlyStopped = this.sequentialMetrics.earlyStoppedTests;

    return total > 0 ? earlyStopped / total : 0;
  }

  calculateAverageSampleSizeReduction() {
    // Calculate average sample size reduction from sequential testing
    const completedTests = Array.from(this.sequentialTests.values()).filter(
      (test) => test.status === "stopped"
    );

    if (completedTests.length === 0) return 0;

    const totalReduction = completedTests.reduce((sum, test) => {
      const plannedAnalyses = test.designParameters.maxAnalyses;
      const actualAnalyses = test.currentAnalysis;
      return sum + (plannedAnalyses - actualAnalyses) / plannedAnalyses;
    }, 0);

    return totalReduction / completedTests.length;
  }

  /**
   * Missing Method Implementations
   */
  async calculateEfficacyBoundary(k, maxAnalyses, alphaSpent, testType) {
    // Calculate efficacy boundary using spending function approach
    // For group sequential designs, this typically uses the inverse normal distribution

    if (testType === "two_sample_means" || testType === "one_sample_mean") {
      // Use Lan-DeMets boundary calculation
      const fraction = k / maxAnalyses;
      const cumulativeAlpha = alphaSpent;

      // Calculate boundary using inverse normal distribution
      // This is a simplified implementation - production would use more sophisticated methods
      const zAlpha = this.inverseNormalCDF(1 - cumulativeAlpha / 2);

      // Adjust for information fraction (simplified O'Brien-Fleming type boundary)
      const informationFraction = fraction;
      const boundary = zAlpha / Math.sqrt(informationFraction);

      return Math.max(1.96, boundary); // Minimum boundary of 1.96
    }

    return 1.96; // Default boundary
  }

  async calculateFutilityBoundary(k, maxAnalyses, betaSpent, testType) {
    // Calculate futility boundary for early stopping due to lack of efficacy

    if (testType === "two_sample_means" || testType === "one_sample_mean") {
      const fraction = k / maxAnalyses;

      // Futility boundary is typically much lower than efficacy boundary
      // This represents the minimum effect size we would consider meaningful
      const futilityThreshold = this.config.futilityThreshold || 0.1;

      // Simple linear futility boundary
      const boundary = futilityThreshold * (1 - fraction);

      return Math.max(0, boundary);
    }

    return 0.1; // Default futility boundary
  }

  async calculateTestStatistic(analysisData, testType) {
    // Calculate appropriate test statistic based on test type

    switch (testType) {
      case "two_sample_means":
        return this.calculateTwoSampleTStatistic(analysisData);

      case "one_sample_mean":
        return this.calculateOneSampleTStatistic(analysisData);

      case "two_sample_proportions":
        return this.calculateTwoSampleZStatistic(analysisData);

      case "one_sample_proportion":
        return this.calculateOneSampleZStatistic(analysisData);

      default:
        throw new Error(`Unsupported test type: ${testType}`);
    }
  }

  calculateTwoSampleTStatistic(data) {
    // Calculate two-sample t-statistic
    const { group1, group2 } = data;

    const n1 = group1.length;
    const n2 = group2.length;

    const mean1 = group1.reduce((sum, val) => sum + val, 0) / n1;
    const mean2 = group2.reduce((sum, val) => sum + val, 0) / n2;

    const var1 =
      group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) / (n1 - 1);
    const var2 =
      group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) / (n2 - 1);

    // Pooled standard error
    const pooledSE = Math.sqrt(var1 / n1 + var2 / n2);

    return (mean1 - mean2) / pooledSE;
  }

  calculateOneSampleTStatistic(data) {
    // Calculate one-sample t-statistic
    const { sample, hypothesizedMean = 0 } = data;

    const n = sample.length;
    const mean = sample.reduce((sum, val) => sum + val, 0) / n;
    const variance =
      sample.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / (n - 1);
    const standardError = Math.sqrt(variance / n);

    return (mean - hypothesizedMean) / standardError;
  }

  calculateTwoSampleZStatistic(data) {
    // Calculate two-sample z-statistic for proportions
    const { successes1, n1, successes2, n2 } = data;

    const p1 = successes1 / n1;
    const p2 = successes2 / n2;
    const pooledP = (successes1 + successes2) / (n1 + n2);

    const standardError = Math.sqrt(
      pooledP * (1 - pooledP) * (1 / n1 + 1 / n2)
    );

    return (p1 - p2) / standardError;
  }

  calculateOneSampleZStatistic(data) {
    // Calculate one-sample z-statistic for proportion
    const { successes, n, hypothesizedP = 0.5 } = data;

    const p = successes / n;
    const standardError = Math.sqrt((hypothesizedP * (1 - hypothesizedP)) / n);

    return (p - hypothesizedP) / standardError;
  }

  async calculatePValue(testStatistic, testType) {
    // Calculate p-value based on test statistic and test type

    // For most common tests, use two-tailed p-value
    const pValue = 2 * (1 - this.standardNormalCDF(Math.abs(testStatistic)));

    return Math.max(0, Math.min(1, pValue)); // Ensure p-value is between 0 and 1
  }

  async calculateLikelihoodRatio(
    observation,
    nullHypothesis,
    alternativeHypothesis
  ) {
    // Calculate likelihood ratio for SPRT
    // This is a simplified implementation for normal distributions

    const { mean: nullMean, variance: nullVar } = nullHypothesis;
    const { mean: altMean, variance: altVar } = alternativeHypothesis;

    // Calculate likelihood under null hypothesis
    const nullLikelihood = this.normalPDF(
      observation,
      nullMean,
      Math.sqrt(nullVar)
    );

    // Calculate likelihood under alternative hypothesis
    const altLikelihood = this.normalPDF(
      observation,
      altMean,
      Math.sqrt(altVar)
    );

    // Return likelihood ratio
    return nullLikelihood > 0 ? altLikelihood / nullLikelihood : 1;
  }

  normalPDF(x, mean, stdDev) {
    // Calculate normal probability density function
    const coefficient = 1 / (stdDev * Math.sqrt(2 * Math.PI));
    const exponent = -0.5 * Math.pow((x - mean) / stdDev, 2);
    return coefficient * Math.exp(exponent);
  }

  inverseNormalCDF(p) {
    // Simplified inverse normal CDF (quantile function)
    // Using Beasley-Springer-Moro algorithm approximation

    if (p === 0.5) return 0;
    if (p < 0.5) return -this.inverseNormalCDF(1 - p);

    const a = [
      0, -3.969683028665376e1, 2.209460984245205e2, -2.759285104469687e2,
      1.38357751867269e2, -3.066479806614716e1, 2.506628277459239,
    ];
    const b = [
      0, -5.447609879822406e1, 1.615858368580409e2, -1.556989798598866e2,
      6.680131188771972e1, -1.328068155288572e1,
    ];

    const pLow = 0.02425;
    const pHigh = 1 - pLow;

    let q, r;

    if (p <= pHigh) {
      q = p - 0.5;
      r = q * q;
      return (
        ((((((a[1] * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * r + a[6]) *
          q) /
        (((((b[1] * r + b[2]) * r + b[3]) * r + b[4]) * r + b[5]) * r + 1)
      );
    } else {
      q = Math.sqrt(-2 * Math.log(1 - p));
      return (
        -(((((a[1] * q + a[2]) * q + a[3]) * q + a[4]) * q + a[5]) * q + a[6]) /
        ((((b[1] * q + b[2]) * q + b[3]) * q + b[4]) * q + 1)
      );
    }
  }
}

export default SequentialTesting;
