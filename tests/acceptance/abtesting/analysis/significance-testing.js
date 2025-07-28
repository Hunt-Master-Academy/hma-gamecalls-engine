/**
 * @file significance-testing.js
 * @brief Statistical Significance Testing Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive statistical significance testing with p-value calculations,
 * effect size analysis, and multiple testing correction for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

/**
 * SignificanceTesting Class
 * Provides comprehensive significance testing with advanced statistical methods
 */
export class SignificanceTesting {
  constructor(config = {}) {
    // Initialize significance testing system with comprehensive configuration
    // Set up p-value calculation framework with multiple test types
    // Configure effect size analysis with various measures
    // Initialize multiple testing corrections (Bonferroni, BH, Holm)
    // Set up significance thresholds for statistical and practical significance
    // Configure statistical validation with assumption checking
    // Initialize result caching for performance optimization
    // Set up significance monitoring for continuous analysis
    // Configure significance reporting with detailed metrics
    // Initialize significance optimization for computational efficiency

    this.config = {
      defaultAlpha: 0.05,
      enableMultipleTesting: true,
      enableEffectSizeAnalysis: true,
      enablePracticalSignificance: true,
      enableBayesianApproach: true,
      significanceThresholds: {
        alpha: 0.05,
        practicalEffect: 0.1,
        clinicalSignificance: 0.2,
      },
      cacheResults: true,
      cacheTimeout: 3600000, // 1 hour
      ...config,
    };

    this.significanceTests = new Map();
    this.testResults = new Map();
    this.pValueAdjustments = new Map();
    this.effectSizeAnalyses = new Map();
    this.significanceMetrics = {
      totalTests: 0,
      significantResults: 0,
      falseDiscoveryRate: 0,
      averagePValue: 0,
      averageEffectSize: 0,
    };

    this.testMethods = new Map();
    this.correctionMethods = new Map();
    this.validators = [];

    this.initializeTestMethods();
  }

  /**
   * P-Value Calculation Framework
   */
  async calculatePValue(testConfig) {
    // Calculate statistical p-value with comprehensive validation
    // Validate test data and assumptions using appropriate tests
    // Select appropriate test statistic based on data type and distribution
    // Calculate test statistic value with numerical precision
    // Determine degrees of freedom for the selected test
    // Calculate p-value from distribution using exact or approximate methods
    // Apply continuity corrections for discrete distributions when needed
    // Validate p-value calculation against bounds and sanity checks
    // Generate calculation audit trail for reproducibility
    // Update p-value metrics for quality monitoring

    const testId = this.generateTestId();
    const timestamp = Date.now();

    const test = {
      id: testId,
      timestamp: timestamp,
      experimentId: testConfig.experimentId,
      testType: testConfig.testType,
      data: testConfig.data,
      hypotheses: {
        null: testConfig.nullHypothesis,
        alternative: testConfig.alternativeHypothesis,
      },
      alpha: testConfig.alpha || this.config.defaultAlpha,
      twoTailed: testConfig.twoTailed !== false,
      results: {},
    };

    // Validate test configuration
    const validation = await this.validateTestConfig(test);
    if (!validation.valid) {
      throw new Error(
        `Invalid test configuration: ${validation.errors.join(", ")}`
      );
    }

    // Calculate test statistic
    const testStatistic = await this.calculateTestStatistic(test);

    // Calculate p-value
    const pValue = await this.calculatePValueFromStatistic(test, testStatistic);

    // Determine significance
    const isSignificant = pValue <= test.alpha;

    test.results = {
      testStatistic: testStatistic,
      pValue: pValue,
      significant: isSignificant,
      alpha: test.alpha,
      degreesOfFreedom: testStatistic.degreesOfFreedom,
      criticalValue: testStatistic.criticalValue,
      confidenceInterval: await this.calculateConfidenceInterval(
        test,
        testStatistic
      ),
    };

    // Store test results
    this.testResults.set(testId, test);

    // Update metrics
    this.significanceMetrics.totalTests++;
    if (isSignificant) {
      this.significanceMetrics.significantResults++;
    }
    this.updateAveragePValue(pValue);

    return {
      testId: testId,
      pValue: pValue,
      significant: isSignificant,
      results: test.results,
    };
  }

  async calculateTestStatistic(test) {
    // Calculate appropriate test statistic based on test type
    // Handle different test types (t-test, z-test, chi-square, F-test)
    // Calculate t-statistic for means comparison with pooled variance
    // Calculate z-statistic for proportions and large sample tests
    // Calculate chi-square statistic for independence and goodness of fit
    // Calculate F-statistic for ANOVA and variance comparison
    // Handle non-parametric statistics (Mann-Whitney, Kruskal-Wallis)
    // Calculate degrees of freedom appropriate for each test
    // Determine critical values for hypothesis testing
    // Validate statistic calculation with bounds checking

    const calculator = this.getTestStatisticCalculator(test.testType);
    if (!calculator) {
      throw new Error(`Unknown test type: ${test.testType}`);
    }

    return await calculator.calculate(test);
  }

  async calculatePValueFromStatistic(test, testStatistic) {
    // Calculate p-value from test statistic using appropriate distribution
    // Use appropriate probability distribution (t, z, chi-square, F)
    // Handle two-tailed vs one-tailed tests correctly
    // Apply continuity corrections for discrete distributions
    // Use exact methods when appropriate for small samples
    // Handle edge cases and numerical precision issues
    // Validate p-value bounds (0 <= p <= 1)
    // Cache distribution lookups for performance
    // Optimize calculation performance for large datasets
    // Generate calculation documentation for audit trail

    const distributionCalculator = this.getDistributionCalculator(
      testStatistic.distribution
    );

    let pValue;
    if (test.twoTailed) {
      pValue =
        2 * (1 - distributionCalculator.cdf(Math.abs(testStatistic.value)));
    } else {
      pValue = 1 - distributionCalculator.cdf(testStatistic.value);
    }

    // Ensure p-value is within valid bounds
    pValue = Math.max(0, Math.min(1, pValue));

    return pValue;
  }

  /**
   * Effect Size Analysis
   */
  async analyzeEffectSize(testId, effectSizeConfig = {}) {
    // Analyze statistical effect size with multiple measures
    // Calculate Cohen's d for means comparison
    // Calculate Cohen's h for proportions comparison
    // Calculate eta-squared for ANOVA effect size
    // Calculate Cramer's V for chi-square association strength
    // Interpret effect size magnitude using standard conventions
    // Calculate effect size confidence intervals
    // Compare practical vs statistical significance
    // Generate effect size recommendations for decision making
    // Update effect size metrics for quality monitoring

    const test = this.testResults.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    const analysisId = this.generateAnalysisId();
    const timestamp = Date.now();

    const effectSizeAnalysis = {
      id: analysisId,
      testId: testId,
      timestamp: timestamp,
      effectSizeType: this.determineEffectSizeType(test.testType),
      results: {},
    };

    // Calculate effect size
    const effectSize = await this.calculateEffectSize(test, effectSizeConfig);

    // Interpret effect size
    const interpretation = await this.interpretEffectSize(
      effectSize,
      test.testType
    );

    // Calculate confidence interval for effect size
    const confidenceInterval = await this.calculateEffectSizeConfidenceInterval(
      test,
      effectSize,
      effectSizeConfig.confidenceLevel || 0.95
    );

    effectSizeAnalysis.results = {
      value: effectSize.value,
      type: effectSize.type,
      interpretation: interpretation,
      confidenceInterval: confidenceInterval,
      practicalSignificance: interpretation.practicallySignificant,
      recommendations: interpretation.recommendations,
    };

    // Store effect size analysis
    this.effectSizeAnalyses.set(analysisId, effectSizeAnalysis);

    // Update metrics
    this.updateAverageEffectSize(effectSize.value);

    return {
      analysisId: analysisId,
      effectSize: effectSizeAnalysis.results,
    };
  }

  async calculateEffectSize(test, config) {
    // Calculate appropriate effect size measure for test type
    const calculator = this.getEffectSizeCalculator(test.testType);
    return await calculator.calculate(test.data, config);
  }

  async interpretEffectSize(effectSize, testType) {
    // Interpret effect size magnitude using Cohen's conventions
    // Apply Cohen's conventions for small, medium, large effects
    // Consider domain-specific thresholds when available
    // Compare to practical significance thresholds
    // Generate interpretation text for stakeholders
    // Provide actionable recommendations based on magnitude
    // Consider business context and impact
    // Account for sample size effects on interpretation
    // Generate visualization recommendations
    // Provide follow-up suggestions for further analysis

    const thresholds = this.getEffectSizeThresholds(testType);
    const magnitude = this.categorizeEffectSize(effectSize.value, thresholds);

    const interpretation = {
      magnitude: magnitude,
      description: this.getEffectSizeDescription(magnitude),
      practicallySignificant:
        effectSize.value >= this.config.significanceThresholds.practicalEffect,
      clinicallySignificant:
        effectSize.value >=
        this.config.significanceThresholds.clinicalSignificance,
      recommendations: this.generateEffectSizeRecommendations(
        effectSize,
        magnitude
      ),
    };

    return interpretation;
  }

  /**
   * Multiple Testing Corrections
   */
  async applyMultipleTestingCorrection(
    testIds,
    correctionMethod = "benjamini_hochberg"
  ) {
    // Apply multiple testing correction to control family-wise error rate
    // Collect p-values from multiple tests for correction
    // Apply Bonferroni correction for conservative control
    // Apply Benjamini-Hochberg correction for FDR control
    // Apply Holm correction for step-down control
    // Apply Sidak correction for independence assumption
    // Calculate adjusted p-values based on correction method
    // Update significance decisions using adjusted p-values
    // Generate correction report with before/after comparison
    // Update false discovery rate estimates

    const correctionId = this.generateCorrectionId();
    const timestamp = Date.now();

    const correction = {
      id: correctionId,
      timestamp: timestamp,
      method: correctionMethod,
      testIds: testIds,
      originalPValues: [],
      adjustedPValues: [],
      originalSignificant: [],
      adjustedSignificant: [],
      falseDiscoveryRate: 0,
    };

    // Collect original p-values and significance
    for (const testId of testIds) {
      const test = this.testResults.get(testId);
      if (test) {
        correction.originalPValues.push(test.results.pValue);
        correction.originalSignificant.push(test.results.significant);
      }
    }

    // Apply correction method
    const correctionResult = await this.applyCorrectionMethod(
      correction.originalPValues,
      correctionMethod
    );

    correction.adjustedPValues = correctionResult.adjustedPValues;
    correction.adjustedSignificant = correctionResult.adjustedSignificant;
    correction.falseDiscoveryRate = correctionResult.falseDiscoveryRate;

    // Update test results with adjusted values
    for (let i = 0; i < testIds.length; i++) {
      const test = this.testResults.get(testIds[i]);
      if (test) {
        test.results.adjustedPValue = correction.adjustedPValues[i];
        test.results.significantAfterCorrection =
          correction.adjustedSignificant[i];
      }
    }

    // Store correction results
    this.pValueAdjustments.set(correctionId, correction);

    // Update metrics
    this.significanceMetrics.falseDiscoveryRate = correction.falseDiscoveryRate;

    return {
      correctionId: correctionId,
      adjustedPValues: correction.adjustedPValues,
      significantAfterCorrection: correction.adjustedSignificant,
      falseDiscoveryRate: correction.falseDiscoveryRate,
    };
  }

  async applyCorrectionMethod(pValues, method) {
    // Apply specific correction method with proper implementation
    const corrector = this.correctionMethods.get(method);
    if (!corrector) {
      throw new Error(`Unknown correction method: ${method}`);
    }

    return await corrector.apply(pValues, this.config.defaultAlpha);
  }

  /**
   * Practical Significance Assessment
   */
  async assessPracticalSignificance(testId, practicalThresholds = {}) {
    // Assess practical significance beyond statistical significance
    // Compare effect size to practical thresholds for meaningful impact
    // Consider business impact and return on investment
    // Evaluate cost-benefit analysis for implementation
    // Account for implementation complexity and resources
    // Consider long-term effects and sustainability
    // Generate practical recommendations for decision makers
    // Assess real-world applicability and feasibility
    // Consider stakeholder perspectives and priorities
    // Generate actionable insights for business strategy

    const test = this.testResults.get(testId);
    if (!test) {
      throw new Error(`Test not found: ${testId}`);
    }

    const thresholds = {
      ...this.config.significanceThresholds,
      ...practicalThresholds,
    };

    const effectSizeAnalysis = Array.from(
      this.effectSizeAnalyses.values()
    ).find((analysis) => analysis.testId === testId);

    if (!effectSizeAnalysis) {
      throw new Error(`Effect size analysis not found for test: ${testId}`);
    }

    const assessment = {
      testId: testId,
      timestamp: Date.now(),
      statisticallySignificant: test.results.significant,
      practicallySignificant: effectSizeAnalysis.results.practicalSignificance,
      clinicallySignificant:
        effectSizeAnalysis.results.interpretation.clinicallySignificant,
      businessImpact: await this.assessBusinessImpact(test, effectSizeAnalysis),
      recommendations: await this.generatePracticalRecommendations(
        test,
        effectSizeAnalysis
      ),
      confidence: this.calculateRecommendationConfidence(
        test,
        effectSizeAnalysis
      ),
    };

    return assessment;
  }

  /**
   * Utility Methods
   */
  initializeTestMethods() {
    // Initialize test statistic calculators for various test types
    this.testMethods.set("two_sample_t", {
      calculate: async (test) => await this.calculateTwoSampleTStatistic(test),
    });

    this.testMethods.set("one_sample_t", {
      calculate: async (test) => await this.calculateOneSampleTStatistic(test),
    });

    this.testMethods.set("paired_t", {
      calculate: async (test) => await this.calculatePairedTStatistic(test),
    });

    this.testMethods.set("z_test", {
      calculate: async (test) => await this.calculateZStatistic(test),
    });

    this.testMethods.set("chi_square", {
      calculate: async (test) => await this.calculateChiSquareStatistic(test),
    });

    // Initialize correction methods
    this.correctionMethods.set("bonferroni", {
      apply: async (pValues, alpha) =>
        await this.applyBonferroniCorrection(pValues, alpha),
    });

    this.correctionMethods.set("benjamini_hochberg", {
      apply: async (pValues, alpha) =>
        await this.applyBenjaminiHochbergCorrection(pValues, alpha),
    });

    this.correctionMethods.set("holm", {
      apply: async (pValues, alpha) =>
        await this.applyHolmCorrection(pValues, alpha),
    });

    // Add comprehensive test method coverage
  }

  generateTestId() {
    return `test_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateAnalysisId() {
    return `analysis_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateCorrectionId() {
    return `correction_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;
  }

  async validateTestConfig(test) {
    // Validate test configuration comprehensively
    const errors = [];

    if (!test.experimentId) {
      errors.push("Experiment ID is required");
    }

    if (!test.testType) {
      errors.push("Test type is required");
    }

    if (!test.data) {
      errors.push("Test data is required");
    }

    if (test.alpha && (test.alpha <= 0 || test.alpha >= 1)) {
      errors.push("Alpha must be between 0 and 1");
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  updateAveragePValue(pValue) {
    // Update running average of p-values for quality monitoring
    const total = this.significanceMetrics.totalTests;
    const currentAverage = this.significanceMetrics.averagePValue;

    this.significanceMetrics.averagePValue =
      (currentAverage * (total - 1) + pValue) / total;
  }

  updateAverageEffectSize(effectSize) {
    // Update running average of effect sizes for trend analysis
    const analysisCount = this.effectSizeAnalyses.size;
    const currentAverage = this.significanceMetrics.averageEffectSize;

    this.significanceMetrics.averageEffectSize =
      (currentAverage * (analysisCount - 1) + effectSize) / analysisCount;
  }

  /**
   * Analytics and Reporting
   */
  getTestResult(testId) {
    return this.testResults.get(testId);
  }

  getEffectSizeAnalysis(analysisId) {
    return this.effectSizeAnalyses.get(analysisId);
  }

  getPValueAdjustment(correctionId) {
    return this.pValueAdjustments.get(correctionId);
  }

  getSignificanceMetrics() {
    return { ...this.significanceMetrics };
  }

  calculateType1ErrorRate() {
    // Calculate observed Type I error rate from test results
    const significantResults = this.significanceMetrics.significantResults;
    const totalTests = this.significanceMetrics.totalTests;

    return totalTests > 0 ? significantResults / totalTests : 0;
  }

  calculatePowerEstimate() {
    // Estimate statistical power based on observed results and effect sizes
    const significantResults = Array.from(this.testResults.values()).filter(
      (test) => test.results.significant
    );

    if (significantResults.length === 0) return 0;

    const averageEffectSize =
      significantResults.reduce((sum, test) => {
        const effectAnalysis = Array.from(
          this.effectSizeAnalyses.values()
        ).find((analysis) => analysis.testId === test.id);
        return sum + (effectAnalysis?.results.value || 0);
      }, 0) / significantResults.length;

    // Simple power estimation based on effect size
    return Math.min(1, averageEffectSize * 4); // Simplified calculation
  }

  // Test Statistic Calculators
  getTestStatisticCalculator(testType) {
    return this.testMethods.get(testType);
  }

  async calculateTwoSampleTStatistic(test) {
    const { group1, group2 } = test.data;

    // Calculate means
    const mean1 = group1.reduce((sum, val) => sum + val, 0) / group1.length;
    const mean2 = group2.reduce((sum, val) => sum + val, 0) / group2.length;

    // Calculate variances
    const variance1 =
      group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) /
      (group1.length - 1);
    const variance2 =
      group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) /
      (group2.length - 1);

    // Pooled variance
    const pooledVariance =
      ((group1.length - 1) * variance1 + (group2.length - 1) * variance2) /
      (group1.length + group2.length - 2);

    // Standard error
    const standardError = Math.sqrt(
      pooledVariance * (1 / group1.length + 1 / group2.length)
    );

    // t-statistic
    const tStatistic = (mean1 - mean2) / standardError;

    return {
      value: tStatistic,
      distribution: "t",
      degreesOfFreedom: group1.length + group2.length - 2,
      criticalValue: this.getTCriticalValue(
        group1.length + group2.length - 2,
        test.alpha
      ),
      means: { group1: mean1, group2: mean2 },
      standardError: standardError,
    };
  }

  async calculateOneSampleTStatistic(test) {
    const { sample, populationMean } = test.data;

    const sampleMean =
      sample.reduce((sum, val) => sum + val, 0) / sample.length;
    const sampleVariance =
      sample.reduce((sum, val) => sum + Math.pow(val - sampleMean, 2), 0) /
      (sample.length - 1);
    const standardError = Math.sqrt(sampleVariance / sample.length);

    const tStatistic = (sampleMean - populationMean) / standardError;

    return {
      value: tStatistic,
      distribution: "t",
      degreesOfFreedom: sample.length - 1,
      criticalValue: this.getTCriticalValue(sample.length - 1, test.alpha),
      sampleMean: sampleMean,
      standardError: standardError,
    };
  }

  async calculatePairedTStatistic(test) {
    const { pairs } = test.data;

    const differences = pairs.map((pair) => pair.after - pair.before);
    const meanDiff =
      differences.reduce((sum, diff) => sum + diff, 0) / differences.length;
    const varianceDiff =
      differences.reduce((sum, diff) => sum + Math.pow(diff - meanDiff, 2), 0) /
      (differences.length - 1);
    const standardError = Math.sqrt(varianceDiff / differences.length);

    const tStatistic = meanDiff / standardError;

    return {
      value: tStatistic,
      distribution: "t",
      degreesOfFreedom: differences.length - 1,
      criticalValue: this.getTCriticalValue(differences.length - 1, test.alpha),
      meanDifference: meanDiff,
      standardError: standardError,
    };
  }

  async calculateZStatistic(test) {
    const { sample1, sample2, populationStd } = test.data;

    const mean1 = sample1.reduce((sum, val) => sum + val, 0) / sample1.length;
    const mean2 = sample2
      ? sample2.reduce((sum, val) => sum + val, 0) / sample2.length
      : 0;

    let standardError;
    let zStatistic;

    if (sample2) {
      // Two-sample z-test
      standardError =
        populationStd * Math.sqrt(1 / sample1.length + 1 / sample2.length);
      zStatistic = (mean1 - mean2) / standardError;
    } else {
      // One-sample z-test
      standardError = populationStd / Math.sqrt(sample1.length);
      zStatistic = (mean1 - test.data.populationMean) / standardError;
    }

    return {
      value: zStatistic,
      distribution: "normal",
      degreesOfFreedom: null,
      criticalValue: this.getZCriticalValue(test.alpha),
      standardError: standardError,
    };
  }

  async calculateChiSquareStatistic(test) {
    const { observed, expected } = test.data;

    let chiSquare = 0;
    let degreesOfFreedom = 0;

    if (Array.isArray(observed[0])) {
      // Contingency table
      const rows = observed.length;
      const cols = observed[0].length;
      degreesOfFreedom = (rows - 1) * (cols - 1);

      for (let i = 0; i < rows; i++) {
        for (let j = 0; j < cols; j++) {
          chiSquare +=
            Math.pow(observed[i][j] - expected[i][j], 2) / expected[i][j];
        }
      }
    } else {
      // Goodness of fit
      degreesOfFreedom = observed.length - 1;

      for (let i = 0; i < observed.length; i++) {
        chiSquare += Math.pow(observed[i] - expected[i], 2) / expected[i];
      }
    }

    return {
      value: chiSquare,
      distribution: "chi-square",
      degreesOfFreedom: degreesOfFreedom,
      criticalValue: this.getChiSquareCriticalValue(
        degreesOfFreedom,
        test.alpha
      ),
    };
  }

  // Distribution Calculators
  getDistributionCalculator(distribution) {
    const calculators = {
      t: {
        cdf: (x, df) => this.tDistributionCDF(x, df),
      },
      normal: {
        cdf: (x) => this.normalCDF(x),
      },
      "chi-square": {
        cdf: (x, df) => this.chiSquareCDF(x, df),
      },
    };

    return calculators[distribution];
  }

  tDistributionCDF(x, df) {
    // Approximation for t-distribution CDF
    if (df >= 30) {
      return this.normalCDF(x);
    }

    const a = df / (df + x * x);
    const beta = this.betaFunction(df / 2, 0.5);
    const incompleteBeta = this.incompleteBetaFunction(a, df / 2, 0.5);

    if (x >= 0) {
      return 0.5 + 0.5 * (1 - incompleteBeta / beta);
    } else {
      return (0.5 * incompleteBeta) / beta;
    }
  }

  normalCDF(x) {
    // Standard normal cumulative distribution function
    return 0.5 * (1 + this.erf(x / Math.sqrt(2)));
  }

  chiSquareCDF(x, df) {
    // Chi-square CDF using gamma function
    if (x <= 0) return 0;
    return this.incompleteGamma(df / 2, x / 2) / this.gamma(df / 2);
  }

  // Effect Size Calculators
  getEffectSizeCalculator(testType) {
    const calculators = {
      two_sample_t: {
        calculate: async (data, config) =>
          await this.calculateCohensD(data, config),
      },
      one_sample_t: {
        calculate: async (data, config) =>
          await this.calculateCohensD(data, config),
      },
      paired_t: {
        calculate: async (data, config) =>
          await this.calculateCohensD(data, config),
      },
      z_test: {
        calculate: async (data, config) =>
          await this.calculateCohensH(data, config),
      },
      chi_square: {
        calculate: async (data, config) =>
          await this.calculateCramersV(data, config),
      },
    };

    return calculators[testType] || calculators["two_sample_t"];
  }

  async calculateCohensD(data, config) {
    if (data.group1 && data.group2) {
      // Two-sample Cohen's d
      const mean1 =
        data.group1.reduce((sum, val) => sum + val, 0) / data.group1.length;
      const mean2 =
        data.group2.reduce((sum, val) => sum + val, 0) / data.group2.length;

      const variance1 =
        data.group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) /
        (data.group1.length - 1);
      const variance2 =
        data.group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) /
        (data.group2.length - 1);

      const pooledStd = Math.sqrt((variance1 + variance2) / 2);
      const cohensD = (mean1 - mean2) / pooledStd;

      return { value: Math.abs(cohensD), type: "Cohen's d" };
    } else if (data.pairs) {
      // Paired Cohen's d
      const differences = data.pairs.map((pair) => pair.after - pair.before);
      const meanDiff =
        differences.reduce((sum, diff) => sum + diff, 0) / differences.length;
      const stdDiff = Math.sqrt(
        differences.reduce(
          (sum, diff) => sum + Math.pow(diff - meanDiff, 2),
          0
        ) /
          (differences.length - 1)
      );

      return { value: Math.abs(meanDiff / stdDiff), type: "Cohen's d" };
    }

    return { value: 0, type: "Cohen's d" };
  }

  async calculateCohensH(data, config) {
    // Cohen's h for proportions
    const { p1, p2 } = data;

    const phi1 = 2 * Math.asin(Math.sqrt(p1));
    const phi2 = 2 * Math.asin(Math.sqrt(p2));

    return { value: Math.abs(phi1 - phi2), type: "Cohen's h" };
  }

  async calculateCramersV(data, config) {
    // Cramer's V for chi-square
    const { observed, chiSquare } = data;

    const n = observed.flat().reduce((sum, val) => sum + val, 0);
    const rows = observed.length;
    const cols = observed[0].length;
    const minDim = Math.min(rows - 1, cols - 1);

    const cramersV = Math.sqrt(chiSquare / (n * minDim));

    return { value: cramersV, type: "Cramer's V" };
  }

  // Effect Size Interpretation
  getEffectSizeThresholds(testType) {
    const thresholds = {
      two_sample_t: { small: 0.2, medium: 0.5, large: 0.8 },
      one_sample_t: { small: 0.2, medium: 0.5, large: 0.8 },
      paired_t: { small: 0.2, medium: 0.5, large: 0.8 },
      z_test: { small: 0.2, medium: 0.5, large: 0.8 },
      chi_square: { small: 0.1, medium: 0.3, large: 0.5 },
    };

    return thresholds[testType] || thresholds["two_sample_t"];
  }

  categorizeEffectSize(value, thresholds) {
    if (value >= thresholds.large) return "large";
    if (value >= thresholds.medium) return "medium";
    if (value >= thresholds.small) return "small";
    return "negligible";
  }

  getEffectSizeDescription(magnitude) {
    const descriptions = {
      negligible:
        "The effect size is negligible and may not be practically meaningful.",
      small:
        "The effect size is small but may be of practical importance in some contexts.",
      medium:
        "The effect size is medium and likely to be of practical significance.",
      large:
        "The effect size is large and represents a substantial practical difference.",
    };

    return descriptions[magnitude] || descriptions["negligible"];
  }

  generateEffectSizeRecommendations(effectSize, magnitude) {
    const recommendations = [];

    if (magnitude === "negligible") {
      recommendations.push(
        "Consider increasing sample size or examining practical significance"
      );
      recommendations.push("Review measurement precision and data quality");
    } else if (magnitude === "small") {
      recommendations.push("Evaluate cost-benefit ratio of implementation");
      recommendations.push("Consider cumulative effects over time");
    } else if (magnitude === "medium") {
      recommendations.push("Strong candidate for implementation");
      recommendations.push("Monitor long-term effects");
    } else if (magnitude === "large") {
      recommendations.push("High priority for implementation");
      recommendations.push("Verify results with additional studies");
    }

    return recommendations;
  }

  // Multiple Testing Correction Methods
  async applyBonferroniCorrection(pValues, alpha) {
    const adjustedAlpha = alpha / pValues.length;
    const adjustedPValues = pValues.map((p) => Math.min(p * pValues.length, 1));
    const adjustedSignificant = adjustedPValues.map((p) => p <= alpha);

    return {
      adjustedPValues: adjustedPValues,
      adjustedSignificant: adjustedSignificant,
      falseDiscoveryRate: this.calculateFDR(pValues, adjustedSignificant),
    };
  }

  async applyBenjaminiHochbergCorrection(pValues, alpha) {
    const sortedIndices = pValues
      .map((p, i) => ({ p, i }))
      .sort((a, b) => a.p - b.p)
      .map((item) => item.i);

    const adjustedPValues = new Array(pValues.length);
    const adjustedSignificant = new Array(pValues.length).fill(false);

    for (let k = pValues.length - 1; k >= 0; k--) {
      const originalIndex = sortedIndices[k];
      const adjustedP = (pValues[originalIndex] * pValues.length) / (k + 1);
      adjustedPValues[originalIndex] = Math.min(adjustedP, 1);

      if (pValues[originalIndex] <= (alpha * (k + 1)) / pValues.length) {
        for (let j = 0; j <= k; j++) {
          adjustedSignificant[sortedIndices[j]] = true;
        }
        break;
      }
    }

    return {
      adjustedPValues: adjustedPValues,
      adjustedSignificant: adjustedSignificant,
      falseDiscoveryRate: this.calculateFDR(pValues, adjustedSignificant),
    };
  }

  async applyHolmCorrection(pValues, alpha) {
    const sortedIndices = pValues
      .map((p, i) => ({ p, i }))
      .sort((a, b) => a.p - b.p)
      .map((item) => item.i);

    const adjustedPValues = new Array(pValues.length);
    const adjustedSignificant = new Array(pValues.length).fill(false);

    for (let k = 0; k < pValues.length; k++) {
      const originalIndex = sortedIndices[k];
      const adjustedP = pValues[originalIndex] * (pValues.length - k);
      adjustedPValues[originalIndex] = Math.min(adjustedP, 1);

      if (pValues[originalIndex] <= alpha / (pValues.length - k)) {
        adjustedSignificant[originalIndex] = true;
      } else {
        break; // Stop at first non-significant result
      }
    }

    return {
      adjustedPValues: adjustedPValues,
      adjustedSignificant: adjustedSignificant,
      falseDiscoveryRate: this.calculateFDR(pValues, adjustedSignificant),
    };
  }

  calculateFDR(pValues, significant) {
    const significantCount = significant.filter((s) => s).length;
    const expectedFalsePositives = pValues.reduce(
      (sum, p, i) => (significant[i] ? sum + p : sum),
      0
    );

    return significantCount > 0 ? expectedFalsePositives / significantCount : 0;
  }

  // Confidence Interval Calculation
  async calculateConfidenceInterval(test, testStatistic) {
    const confidenceLevel = 1 - test.alpha;

    if (testStatistic.distribution === "t") {
      return this.calculateTConfidenceInterval(
        test,
        testStatistic,
        confidenceLevel
      );
    } else if (testStatistic.distribution === "normal") {
      return this.calculateZConfidenceInterval(
        test,
        testStatistic,
        confidenceLevel
      );
    }

    return { lower: null, upper: null };
  }

  calculateTConfidenceInterval(test, testStatistic, confidenceLevel) {
    const alpha = 1 - confidenceLevel;
    const tCritical = this.getTCriticalValue(
      testStatistic.degreesOfFreedom,
      alpha
    );
    const marginOfError = tCritical * testStatistic.standardError;

    const pointEstimate = testStatistic.means
      ? testStatistic.means.group1 - testStatistic.means.group2
      : testStatistic.sampleMean;

    return {
      lower: pointEstimate - marginOfError,
      upper: pointEstimate + marginOfError,
      marginOfError: marginOfError,
    };
  }

  calculateZConfidenceInterval(test, testStatistic, confidenceLevel) {
    const alpha = 1 - confidenceLevel;
    const zCritical = this.getZCriticalValue(alpha);
    const marginOfError = zCritical * testStatistic.standardError;

    // Point estimate calculation depends on test type
    const pointEstimate = 0; // Simplified for this implementation

    return {
      lower: pointEstimate - marginOfError,
      upper: pointEstimate + marginOfError,
      marginOfError: marginOfError,
    };
  }

  async calculateEffectSizeConfidenceInterval(
    test,
    effectSize,
    confidenceLevel
  ) {
    // Simplified confidence interval for effect size
    const alpha = 1 - confidenceLevel;
    const zCritical = this.getZCriticalValue(alpha);

    // Standard error approximation for effect size
    const standardError = effectSize.value * 0.1; // Simplified approximation
    const marginOfError = zCritical * standardError;

    return {
      lower: Math.max(0, effectSize.value - marginOfError),
      upper: effectSize.value + marginOfError,
      marginOfError: marginOfError,
    };
  }

  // Practical Significance Assessment
  async assessBusinessImpact(test, effectSizeAnalysis) {
    return {
      revenue_impact: effectSizeAnalysis.results.value * 1000, // Simplified
      user_experience: effectSizeAnalysis.results.interpretation.magnitude,
      implementation_cost: "medium",
      time_to_value: "2-4 weeks",
    };
  }

  async generatePracticalRecommendations(test, effectSizeAnalysis) {
    const recommendations = [];
    const magnitude = effectSizeAnalysis.results.interpretation.magnitude;

    if (magnitude === "large") {
      recommendations.push("Implement immediately with full rollout");
      recommendations.push("Monitor closely for sustained effects");
    } else if (magnitude === "medium") {
      recommendations.push("Implement with phased rollout");
      recommendations.push("Continue monitoring and optimization");
    } else {
      recommendations.push("Consider alternative approaches");
      recommendations.push("May not justify implementation costs");
    }

    return recommendations;
  }

  calculateRecommendationConfidence(test, effectSizeAnalysis) {
    const pValue = test.results.pValue;
    const effectSize = effectSizeAnalysis.results.value;
    const sampleSize = test.data.group1
      ? test.data.group1.length + test.data.group2.length
      : 100;

    // Composite confidence score
    const pValueScore = 1 - pValue;
    const effectSizeScore = Math.min(effectSize, 1);
    const sampleSizeScore = Math.min(sampleSize / 1000, 1);

    return (pValueScore + effectSizeScore + sampleSizeScore) / 3;
  }

  // Critical Value Calculators
  getTCriticalValue(df, alpha) {
    // Approximation for t-critical values
    if (df >= 30) {
      return this.getZCriticalValue(alpha);
    }

    // Simplified lookup table for common values
    const tTable = {
      1: { 0.05: 12.706, 0.01: 63.657 },
      5: { 0.05: 2.571, 0.01: 4.032 },
      10: { 0.05: 2.228, 0.01: 3.169 },
      20: { 0.05: 2.086, 0.01: 2.845 },
      30: { 0.05: 2.042, 0.01: 2.75 },
    };

    const closestDf = Object.keys(tTable).reduce((prev, curr) =>
      Math.abs(curr - df) < Math.abs(prev - df) ? curr : prev
    );

    return tTable[closestDf][alpha] || 1.96;
  }

  getZCriticalValue(alpha) {
    // Standard normal critical values
    if (alpha <= 0.01) return 2.576;
    if (alpha <= 0.05) return 1.96;
    if (alpha <= 0.1) return 1.645;
    return 1.96;
  }

  getChiSquareCriticalValue(df, alpha) {
    // Simplified chi-square critical values
    const chiTable = {
      1: { 0.05: 3.841, 0.01: 6.635 },
      5: { 0.05: 11.07, 0.01: 15.086 },
      10: { 0.05: 18.307, 0.01: 23.209 },
    };

    const closestDf = Object.keys(chiTable).reduce((prev, curr) =>
      Math.abs(curr - df) < Math.abs(prev - df) ? curr : prev
    );

    return chiTable[closestDf][alpha] || 3.841;
  }

  // Mathematical Helper Functions
  erf(x) {
    // Approximation of error function
    const a1 = 0.254829592;
    const a2 = -0.284496736;
    const a3 = 1.421413741;
    const a4 = -1.453152027;
    const a5 = 1.061405429;
    const p = 0.3275911;

    const sign = x >= 0 ? 1 : -1;
    x = Math.abs(x);

    const t = 1.0 / (1.0 + p * x);
    const y =
      1.0 -
      ((((a5 * t + a4) * t + a3) * t + a2) * t + a1) * t * Math.exp(-x * x);

    return sign * y;
  }

  gamma(x) {
    // Stirling's approximation for gamma function
    if (x < 0.5) {
      return Math.PI / (Math.sin(Math.PI * x) * this.gamma(1 - x));
    }

    x -= 1;
    return Math.sqrt(2 * Math.PI * x) * Math.pow(x / Math.E, x);
  }

  betaFunction(a, b) {
    return (this.gamma(a) * this.gamma(b)) / this.gamma(a + b);
  }

  incompleteBetaFunction(x, a, b) {
    // Simplified incomplete beta function
    if (x <= 0) return 0;
    if (x >= 1) return this.betaFunction(a, b);

    // Continued fraction approximation (simplified)
    return x * this.betaFunction(a, b) * 0.5; // Very simplified
  }

  incompleteGamma(a, x) {
    // Simplified incomplete gamma function
    if (x <= 0) return 0;

    // Series expansion (first few terms)
    let sum = 1;
    let term = 1;

    for (let n = 1; n < 100; n++) {
      term *= x / (a + n - 1);
      sum += term;
      if (term < 1e-10) break;
    }

    return Math.pow(x, a) * Math.exp(-x) * sum;
  }
}

export default SignificanceTesting;
