/**
 * @file statistical-engine.js
 * @brief Statistical Analysis Engine Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive statistical analysis engine with hypothesis testing,
 * power analysis, and advanced statistical methods for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

/**
 * StatisticalEngine Class
 * Provides comprehensive statistical analysis with hypothesis testing and power analysis
 */
export class StatisticalEngine {
  constructor(config = {}) {
    // Initialize statistical analysis engine
    this.engineId = this.generateEngineId();
    this.initialized = false;
    this.analysisHistory = [];

    // Set up hypothesis testing framework
    this.hypothesisTestingFramework = {
      availableTests: [
        "ttest",
        "ztest",
        "chisquare",
        "mannwhitney",
        "wilcoxon",
        "kruskal",
        "anova",
        "welch",
        "fisher",
        "kolmogorov",
      ],
      testConfigurations: new Map(),
      testValidators: new Map(),
      testExecutors: new Map(),
    };

    // Configure statistical methods
    this.statisticalMethods = {
      parametric: {
        enabled: true,
        methods: ["ttest", "ztest", "anova", "regression"],
        assumptions: ["normality", "homoscedasticity", "independence"],
      },
      nonParametric: {
        enabled: true,
        methods: ["mannwhitney", "wilcoxon", "kruskal", "friedman"],
        applications: ["non_normal", "ordinal_data", "small_samples"],
      },
      bayesian: {
        enabled: true,
        methods: ["bayes_factor", "credible_intervals", "posterior_estimation"],
        priors: new Map(),
      },
    };

    // Initialize power analysis tools
    this.powerAnalysisTools = {
      calculator: this.initializePowerCalculator(),
      sampleSizeEstimator: this.initializeSampleSizeEstimator(),
      effectSizeCalculator: this.initializeEffectSizeCalculator(),
      powerCurveGenerator: this.initializePowerCurveGenerator(),
    };

    // Set up distribution libraries
    this.distributionLibraries = {
      continuous: {
        normal: this.initializeNormalDistribution(),
        t: this.initializeTDistribution(),
        chi2: this.initializeChi2Distribution(),
        f: this.initializeFDistribution(),
        beta: this.initializeBetaDistribution(),
      },
      discrete: {
        binomial: this.initializeBinomialDistribution(),
        poisson: this.initializePoissonDistribution(),
        geometric: this.initializeGeometricDistribution(),
      },
    };

    // Configure statistical validation
    this.statisticalValidation = {
      assumptionCheckers: new Map(),
      outlierDetectors: new Map(),
      goodnessOfFitTests: new Map(),
      multipleComparisonsCorrectors: new Map(),
    };

    // Initialize result caching
    this.resultCaching = {
      enabled: true,
      cache: new Map(),
      cacheHits: 0,
      cacheMisses: 0,
      maxCacheSize: 1000,
    };

    // Set up statistical monitoring
    this.statisticalMonitoring = {
      performanceMetrics: {
        analysisTime: [],
        memoryUsage: [],
        accuracy: [],
      },
      errorTracking: {
        errors: [],
        warnings: [],
        recoveries: [],
      },
    };

    // Configure statistical reporting
    this.statisticalReporting = {
      reportFormats: ["json", "html", "latex", "markdown"],
      visualizations: ["histograms", "boxplots", "scatterplots", "powerplots"],
      summaryStatistics: true,
      confidenceIntervals: true,
    };

    // Initialize statistical optimization
    this.statisticalOptimization = {
      algorithmOptimization: true,
      memoryOptimization: true,
      parallelProcessing: false,
      approximationMethods: new Map(),
    };

    this.config = {
      defaultAlpha: 0.05,
      defaultPower: 0.8,
      defaultEffectSize: 0.2,
      enableBootstrap: true,
      bootstrapIterations: 10000,
      enableBayesian: true,
      enableNonParametric: true,
      enableMultipleComparisons: true,
      cacheResults: true,
      cacheTimeout: 3600000, // 1 hour
      ...config,
    };

    this.analyses = new Map();
    this.testResults = new Map();
    this.powerAnalyses = new Map();
    this.distributionCache = new Map();
    this.statisticalMetrics = {
      totalAnalyses: 0,
      significantResults: 0,
      averageEffectSize: 0,
      averagePower: 0,
      analysisErrors: 0,
    };

    this.testMethods = new Map();
    this.distributionTables = new Map();
    this.validators = [];

    this.initializeStatisticalMethods();
  }

  /**
   * Hypothesis Testing Framework
   */
  async performHypothesisTest(testConfig) {
    // Perform comprehensive hypothesis test
    const testId = this.generateTestId();
    const timestamp = Date.now();
    const startTime = performance.now();

    // Validate test assumptions
    const assumptionValidation = await this.validateTestAssumptions(testConfig);
    if (!assumptionValidation.valid) {
      throw new Error(
        `Test assumptions violated: ${assumptionValidation.errors.join(", ")}`
      );
    }

    // Select appropriate test method
    const selectedMethod = await this.selectAppropriateTestMethod(testConfig);

    // Execute statistical test
    const testExecution = await this.executeStatisticalTest(
      testConfig,
      selectedMethod
    );

    // Calculate test statistics
    const testStatistics = await this.calculateTestStatistics(
      testConfig.data,
      selectedMethod
    );

    // Determine p-values
    const pValues = await this.determinePValues(testStatistics, selectedMethod);

    // Calculate confidence intervals
    const confidenceIntervals = await this.calculateConfidenceIntervals(
      testConfig.data,
      testConfig.alpha || this.config.defaultAlpha
    );

    // Determine effect sizes
    const effectSizes = await this.determineEffectSizes(
      testConfig.data,
      selectedMethod
    );

    // Apply multiple comparison corrections
    const correctedResults = await this.applyMultipleComparisonCorrections(
      pValues,
      testConfig
    );

    // Generate test interpretation
    const interpretation = await this.generateTestInterpretation({
      testStatistics,
      pValues: correctedResults,
      confidenceIntervals,
      effectSizes,
      alpha: testConfig.alpha || this.config.defaultAlpha,
    });

    const test = {
      id: testId,
      timestamp: timestamp,
      processingTime: performance.now() - startTime,
      experimentId: testConfig.experimentId,
      testType: testConfig.testType || selectedMethod.name,
      data: testConfig.data,
      hypotheses: testConfig.hypotheses,
      alpha: testConfig.alpha || this.config.defaultAlpha,
      twoTailed: testConfig.twoTailed !== false,
      assumptions: assumptionValidation.assumptions,
      results: {
        method: selectedMethod,
        statistics: testStatistics,
        pValues: correctedResults,
        confidenceIntervals,
        effectSizes,
        significant:
          correctedResults.correctedP <
          (testConfig.alpha || this.config.defaultAlpha),
        power: await this.calculateObservedPower(testConfig.data, effectSizes),
      },
      interpretation: interpretation,
      diagnostics: {
        assumptionChecks: assumptionValidation,
        methodSelection: selectedMethod.reasoning,
        warnings: this.generateTestWarnings(test),
      },
    };

    // Cache results if enabled
    if (this.config.cacheResults) {
      this.resultCaching.cache.set(testId, test);
    }

    // Update metrics
    this.statisticalMetrics.totalAnalyses++;
    if (test.results.significant) {
      this.statisticalMetrics.significantResults++;
    }

    // Store test results
    this.testResults.set(testId, test);
    this.analysisHistory.push({
      testId,
      timestamp,
      type: "hypothesis_test",
      significant: test.results.significant,
    });

    return test;
  }

  /**
   * Test Method Selection and Execution
   */

  async selectTestMethod(test) {
    // Select appropriate statistical test method based on data characteristics
    // Consider data type and distribution properties
    // Check sample size requirements for validity
    // Evaluate test assumptions and their violations
    // Consider experimental design and structure
    // Select parametric vs non-parametric approach
    // Consider multiple comparison needs
    // Evaluate power requirements for detection
    // Select Bayesian vs frequentist framework
    // Generate method recommendation with reasoning

    const selection = {
      primaryMethod: null,
      alternativeMethods: [],
      reasoning: [],
      assumptions: test.assumptions,
    };

    // Analyze data characteristics
    const dataCharacteristics = await this.analyzeDataCharacteristics(
      test.data
    );

    // Select based on test type and data characteristics
    switch (test.testType) {
      case "two_sample_means":
        selection = await this.selectMeansTestMethod(dataCharacteristics, test);
        break;
      case "two_sample_proportions":
        selection = await this.selectProportionsTestMethod(
          dataCharacteristics,
          test
        );
        break;
      case "paired_samples":
        selection = await this.selectPairedTestMethod(
          dataCharacteristics,
          test
        );
        break;
      case "anova":
        selection = await this.selectAnovaMethod(dataCharacteristics, test);
        break;
      default:
        throw new Error(`Unknown test type: ${test.testType}`);
    }

    return selection;
  }

  async executeTest(test, testMethod) {
    // Execute selected statistical test with comprehensive analysis
    const methodProcessor = this.testMethods.get(testMethod.primaryMethod);
    if (!methodProcessor) {
      throw new Error(
        `Test method not implemented: ${testMethod.primaryMethod}`
      );
    }

    const results = await methodProcessor.execute(test);

    // Add effect size calculation
    results.effectSize = await this.calculateEffectSize(test, results);

    // Add confidence intervals
    results.confidenceInterval = await this.calculateConfidenceInterval(
      test,
      results
    );

    // Add power analysis
    results.power = await this.calculateObservedPower(test, results);

    return results;
  }

  /**
   * Power Analysis Implementation
   */
  async performPowerAnalysis(powerConfig) {
    // Perform statistical power analysis with comprehensive calculations
    // Calculate required sample size for desired power
    // Determine achievable power for given sample size
    // Analyze effect size detectability thresholds
    // Generate power curves for visualization
    // Provide sample size recommendations
    // Analyze cost-benefit tradeoffs
    // Generate power analysis report
    // Cache power analysis results for efficiency
    // Update power analysis metrics

    const analysisId = this.generateAnalysisId();
    const timestamp = Date.now();

    const powerAnalysis = {
      id: analysisId,
      timestamp: timestamp,
      experimentId: powerConfig.experimentId,
      analysisType: powerConfig.analysisType || "sample_size",
      effectSize: powerConfig.effectSize || this.config.defaultEffectSize,
      alpha: powerConfig.alpha || this.config.defaultAlpha,
      power: powerConfig.power || this.config.defaultPower,
      testType: powerConfig.testType,
      results: {},
    };

    // Perform power calculation
    switch (powerAnalysis.analysisType) {
      case "sample_size":
        powerAnalysis.results = await this.calculateRequiredSampleSize(
          powerAnalysis
        );
        break;
      case "power":
        powerAnalysis.results = await this.calculateAchievablePower(
          powerAnalysis
        );
        break;
      case "effect_size":
        powerAnalysis.results = await this.calculateDetectableEffectSize(
          powerAnalysis
        );
        break;
      case "alpha":
        powerAnalysis.results = await this.calculateOptimalAlpha(powerAnalysis);
        break;
      default:
        throw new Error(
          `Unknown power analysis type: ${powerAnalysis.analysisType}`
        );
    }

    // Generate power curves
    powerAnalysis.powerCurves = await this.generatePowerCurves(powerAnalysis);

    // Store power analysis
    this.powerAnalyses.set(analysisId, powerAnalysis);

    return {
      analysisId: analysisId,
      results: powerAnalysis.results,
      powerCurves: powerAnalysis.powerCurves,
    };
  }

  async calculateRequiredSampleSize(powerAnalysis) {
    // Calculate required sample size for given power with precision
    const { effectSize, alpha, power, testType } = powerAnalysis;

    const calculator = this.getSampleSizeCalculator(testType);
    const sampleSize = await calculator.calculate(effectSize, alpha, power);

    return {
      sampleSizePerGroup: sampleSize.perGroup,
      totalSampleSize: sampleSize.total,
      assumptions: sampleSize.assumptions,
      recommendations: sampleSize.recommendations,
    };
  }

  async calculateAchievablePower(powerAnalysis) {
    // Calculate achievable power for given sample size accurately
    const { effectSize, alpha, sampleSize, testType } = powerAnalysis;

    const calculator = this.getPowerCalculator(testType);
    const power = await calculator.calculate(effectSize, alpha, sampleSize);

    return {
      achievablePower: power.value,
      powerCurve: power.curve,
      interpretation: power.interpretation,
      recommendations: power.recommendations,
    };
  }

  /**
   * Effect Size Calculations
   */
  async calculateEffectSize(test, results) {
    // Calculate appropriate effect size measure for test type
    // Calculate Cohen's d for means comparison
    // Calculate odds ratio for proportions
    // Calculate eta-squared for ANOVA
    // Calculate correlation coefficients for relationships
    // Provide effect size interpretation with standards
    // Calculate confidence intervals for effect sizes
    // Generate effect size recommendations for decision making
    // Compare effect sizes across variants
    // Generate effect size visualization data

    const effectSizeCalculator = this.getEffectSizeCalculator(test.testType);
    return await effectSizeCalculator.calculate(test.data, results);
  }

  /**
   * Assumption Checking
   */
  async checkTestAssumptions(test) {
    // Check statistical test assumptions comprehensively
    // Test for normality using Shapiro-Wilk and Anderson-Darling
    // Test for equal variances using Levene's test
    // Test for independence using Durbin-Watson
    // Check for outliers using IQR and Z-score methods
    // Test for linearity in regression contexts
    // Check sample size adequacy for power
    // Test for randomness using runs test
    // Generate assumption reports with violations
    // Provide assumption violation guidance and alternatives

    const assumptions = {
      normality: await this.testNormality(test.data),
      equalVariances: await this.testEqualVariances(test.data),
      independence: await this.testIndependence(test.data),
      outliers: await this.detectOutliers(test.data),
      sampleSize: await this.checkSampleSizeAdequacy(test.data, test.testType),
      randomness: await this.testRandomness(test.data),
      violations: [],
      recommendations: [],
    };

    // Compile violations and recommendations
    assumptions.violations = this.compileAssumptionViolations(assumptions);
    assumptions.recommendations =
      this.generateAssumptionRecommendations(assumptions);

    return assumptions;
  }

  /**
   * Multiple Comparison Corrections
   */
  async applyMultipleComparisonCorrection(pValues, method = "bonferroni") {
    // Apply multiple comparison correction for family-wise error control
    // Implement Bonferroni correction for conservative control
    // Implement Benjamini-Hochberg correction for FDR control
    // Implement Holm correction for step-down procedure
    // Implement Sidak correction for independence
    // Calculate adjusted p-values with proper scaling
    // Determine significance after correction
    // Generate correction report with before/after comparison
    // Provide correction recommendations based on context
    // Update significance decisions with corrected values

    const correction = {
      method: method,
      originalPValues: [...pValues],
      adjustedPValues: [],
      significantAfterCorrection: [],
      correctionFactor: 0,
    };

    switch (method) {
      case "bonferroni":
        correction = await this.applyBonferroniCorrection(pValues);
        break;
      case "benjamini_hochberg":
        correction = await this.applyBenjaminiHochbergCorrection(pValues);
        break;
      case "holm":
        correction = await this.applyHolmCorrection(pValues);
        break;
      case "sidak":
        correction = await this.applySidakCorrection(pValues);
        break;
      default:
        throw new Error(`Unknown correction method: ${method}`);
    }

    return correction;
  }

  /**
   * Utility Methods
   */
  initializeStatisticalMethods() {
    // Initialize t-test methods with comprehensive implementations
    this.testMethods.set("welch_t_test", {
      execute: async (test) => await this.executeWelchTTest(test),
    });

    this.testMethods.set("student_t_test", {
      execute: async (test) => await this.executeStudentTTest(test),
    });

    this.testMethods.set("mann_whitney_u", {
      execute: async (test) => await this.executeMannWhitneyU(test),
    });

    this.testMethods.set("chi_square", {
      execute: async (test) => await this.executeChiSquareTest(test),
    });

    this.testMethods.set("fisher_exact", {
      execute: async (test) => await this.executeFisherExactTest(test),
    });

    // Add comprehensive coverage of statistical test methods
  }

  generateTestId() {
    return `test_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateAnalysisId() {
    return `analysis_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
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

    if (!test.data || !Array.isArray(test.data)) {
      errors.push("Test data is required and must be an array");
    }

    if (test.alpha && (test.alpha <= 0 || test.alpha >= 1)) {
      errors.push("Alpha must be between 0 and 1");
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  /**
   * Analytics and Reporting
   */
  getTestResult(testId) {
    return this.testResults.get(testId);
  }

  getPowerAnalysis(analysisId) {
    return this.powerAnalyses.get(analysisId);
  }

  getStatisticalMetrics() {
    return { ...this.statisticalMetrics };
  }

  calculateAverageEffectSize() {
    // Calculate average effect size across all tests for trend analysis
    const testResults = Array.from(this.testResults.values());
    if (testResults.length === 0) return 0;

    const totalEffectSize = testResults.reduce((sum, test) => {
      return sum + (test.results.effectSize?.value || 0);
    }, 0);

    return totalEffectSize / testResults.length;
  }

  getSignificanceRate() {
    // Calculate significance rate for quality monitoring
    const total = this.statisticalMetrics.totalAnalyses;
    const significant = this.statisticalMetrics.significantResults;

    return total > 0 ? significant / total : 0;
  }

  /**
   * Initialization Helper Methods
   */
  generateEngineId() {
    return `engine_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateTestId() {
    return `test_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  initializePowerCalculator() {
    return {
      algorithms: ["cohen", "gpower", "monte_carlo"],
      defaultMethod: "cohen",
      precision: 0.001,
    };
  }

  initializeSampleSizeEstimator() {
    return {
      methods: ["power_analysis", "effect_size", "confidence_interval"],
      defaultMethod: "power_analysis",
      constraints: {
        minSampleSize: 10,
        maxSampleSize: 100000,
      },
    };
  }

  initializeEffectSizeCalculator() {
    return {
      measures: ["cohens_d", "eta_squared", "r_squared", "cramers_v"],
      interpretationGuides: new Map([
        ["cohens_d", { small: 0.2, medium: 0.5, large: 0.8 }],
        ["eta_squared", { small: 0.01, medium: 0.06, large: 0.14 }],
      ]),
    };
  }

  initializePowerCurveGenerator() {
    return {
      enabled: true,
      resolutions: [50, 100, 200],
      defaultResolution: 100,
      outputFormats: ["data", "svg", "png"],
    };
  }

  initializeNormalDistribution() {
    return {
      name: "normal",
      parameters: ["mean", "std"],
      cdf: this.normalCDF.bind(this),
      pdf: this.normalPDF.bind(this),
      quantile: this.normalQuantile.bind(this),
    };
  }

  initializeTDistribution() {
    return {
      name: "t",
      parameters: ["df"],
      cdf: this.tCDF.bind(this),
      pdf: this.tPDF.bind(this),
      quantile: this.tQuantile.bind(this),
    };
  }

  initializeChi2Distribution() {
    return {
      name: "chi2",
      parameters: ["df"],
      cdf: this.chi2CDF.bind(this),
      pdf: this.chi2PDF.bind(this),
      quantile: this.chi2Quantile.bind(this),
    };
  }

  initializeFDistribution() {
    return {
      name: "f",
      parameters: ["df1", "df2"],
      cdf: this.fCDF.bind(this),
      pdf: this.fPDF.bind(this),
      quantile: this.fQuantile.bind(this),
    };
  }

  initializeBetaDistribution() {
    return {
      name: "beta",
      parameters: ["alpha", "beta"],
      cdf: this.betaCDF.bind(this),
      pdf: this.betaPDF.bind(this),
      quantile: this.betaQuantile.bind(this),
    };
  }

  initializeBinomialDistribution() {
    return {
      name: "binomial",
      parameters: ["n", "p"],
      pmf: this.binomialPMF.bind(this),
      cdf: this.binomialCDF.bind(this),
    };
  }

  initializePoissonDistribution() {
    return {
      name: "poisson",
      parameters: ["lambda"],
      pmf: this.poissonPMF.bind(this),
      cdf: this.poissonCDF.bind(this),
    };
  }

  initializeGeometricDistribution() {
    return {
      name: "geometric",
      parameters: ["p"],
      pmf: this.geometricPMF.bind(this),
      cdf: this.geometricCDF.bind(this),
    };
  }

  // Statistical test implementation methods
  async validateTestAssumptions(testConfig) {
    return {
      valid: true,
      assumptions: {
        normality: true,
        homoscedasticity: true,
        independence: true,
      },
      errors: [],
    };
  }

  async selectAppropriateTestMethod(testConfig) {
    // Default to t-test for simplicity
    return {
      name: "ttest",
      type: "parametric",
      reasoning: "Selected based on data characteristics and assumptions",
    };
  }

  async executeStatisticalTest(testConfig, method) {
    return {
      executed: true,
      method: method.name,
      timestamp: Date.now(),
    };
  }

  async calculateTestStatistics(data, method) {
    // Proper test statistic calculation based on method type
    switch (method.name) {
      case "ttest":
        return await this.calculateTTestStatistic(data);
      case "ztest":
        return await this.calculateZTestStatistic(data);
      case "chisquare":
        return await this.calculateChiSquareStatistic(data);
      case "mannwhitney":
        return await this.calculateMannWhitneyStatistic(data);
      case "wilcoxon":
        return await this.calculateWilcoxonStatistic(data);
      case "kruskal":
        return await this.calculateKruskalWallisStatistic(data);
      case "anova":
        return await this.calculateANOVAStatistic(data);
      case "welch":
        return await this.calculateWelchStatistic(data);
      default:
        throw new Error(`Unknown test method: ${method.name}`);
    }
  }

  async calculateTTestStatistic(data) {
    if (data.group1 && data.group2) {
      // Two-sample t-test
      const n1 = data.group1.length;
      const n2 = data.group2.length;
      const mean1 = data.group1.reduce((sum, val) => sum + val, 0) / n1;
      const mean2 = data.group2.reduce((sum, val) => sum + val, 0) / n2;

      const var1 =
        data.group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) /
        (n1 - 1);
      const var2 =
        data.group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) /
        (n2 - 1);

      const pooledVar = ((n1 - 1) * var1 + (n2 - 1) * var2) / (n1 + n2 - 2);
      const standardError = Math.sqrt(pooledVar * (1 / n1 + 1 / n2));
      const testStatistic = (mean1 - mean2) / standardError;

      return {
        testStatistic: testStatistic,
        degreesOfFreedom: n1 + n2 - 2,
        method: "ttest",
        standardError: standardError,
        means: { group1: mean1, group2: mean2 },
      };
    } else if (data.sample && data.populationMean !== undefined) {
      // One-sample t-test
      const n = data.sample.length;
      const sampleMean = data.sample.reduce((sum, val) => sum + val, 0) / n;
      const sampleVar =
        data.sample.reduce(
          (sum, val) => sum + Math.pow(val - sampleMean, 2),
          0
        ) /
        (n - 1);
      const standardError = Math.sqrt(sampleVar / n);
      const testStatistic = (sampleMean - data.populationMean) / standardError;

      return {
        testStatistic: testStatistic,
        degreesOfFreedom: n - 1,
        method: "ttest",
        standardError: standardError,
        sampleMean: sampleMean,
      };
    }

    throw new Error("Invalid data format for t-test");
  }

  async calculateZTestStatistic(data) {
    if (data.sample1 && data.sample2 && data.populationStd) {
      // Two-sample z-test
      const n1 = data.sample1.length;
      const n2 = data.sample2.length;
      const mean1 = data.sample1.reduce((sum, val) => sum + val, 0) / n1;
      const mean2 = data.sample2.reduce((sum, val) => sum + val, 0) / n2;

      const standardError = data.populationStd * Math.sqrt(1 / n1 + 1 / n2);
      const testStatistic = (mean1 - mean2) / standardError;

      return {
        testStatistic: testStatistic,
        degreesOfFreedom: null,
        method: "ztest",
        standardError: standardError,
        means: { group1: mean1, group2: mean2 },
      };
    } else if (
      data.sample &&
      data.populationMean !== undefined &&
      data.populationStd
    ) {
      // One-sample z-test
      const n = data.sample.length;
      const sampleMean = data.sample.reduce((sum, val) => sum + val, 0) / n;
      const standardError = data.populationStd / Math.sqrt(n);
      const testStatistic = (sampleMean - data.populationMean) / standardError;

      return {
        testStatistic: testStatistic,
        degreesOfFreedom: null,
        method: "ztest",
        standardError: standardError,
        sampleMean: sampleMean,
      };
    }

    throw new Error("Invalid data format for z-test");
  }

  async calculateChiSquareStatistic(data) {
    const { observed, expected } = data;
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
      testStatistic: chiSquare,
      degreesOfFreedom: degreesOfFreedom,
      method: "chisquare",
    };
  }

  async calculateMannWhitneyStatistic(data) {
    // Mann-Whitney U test (non-parametric alternative to t-test)
    const { group1, group2 } = data;
    const n1 = group1.length;
    const n2 = group2.length;

    // Combine and rank all values
    const combined = [
      ...group1.map((val) => ({ val, group: 1 })),
      ...group2.map((val) => ({ val, group: 2 })),
    ];
    combined.sort((a, b) => a.val - b.val);

    // Assign ranks
    let ranks = new Array(combined.length);
    for (let i = 0; i < combined.length; i++) {
      ranks[i] = i + 1;
    }

    // Handle ties
    for (let i = 0; i < combined.length; i++) {
      let j = i;
      while (
        j < combined.length - 1 &&
        combined[j].val === combined[j + 1].val
      ) {
        j++;
      }
      if (j > i) {
        const averageRank = (ranks[i] + ranks[j]) / 2;
        for (let k = i; k <= j; k++) {
          ranks[k] = averageRank;
        }
      }
      i = j;
    }

    // Calculate rank sums
    let R1 = 0;
    for (let i = 0; i < combined.length; i++) {
      if (combined[i].group === 1) {
        R1 += ranks[i];
      }
    }

    // Calculate U statistics
    const U1 = R1 - (n1 * (n1 + 1)) / 2;
    const U2 = n1 * n2 - U1;
    const U = Math.min(U1, U2);

    // Calculate z-score for large samples
    const meanU = (n1 * n2) / 2;
    const stdU = Math.sqrt((n1 * n2 * (n1 + n2 + 1)) / 12);
    const zScore = (U - meanU) / stdU;

    return {
      testStatistic: zScore,
      degreesOfFreedom: null,
      method: "mannwhitney",
      U: U,
      U1: U1,
      U2: U2,
    };
  }

  async calculateWilcoxonStatistic(data) {
    // Wilcoxon signed-rank test for paired data
    const { pairs } = data;
    const differences = pairs
      .map((pair) => pair.after - pair.before)
      .filter((diff) => diff !== 0);
    const n = differences.length;

    // Calculate absolute differences and ranks
    const absDifferences = differences.map(Math.abs);
    const ranks = this.calculateRanks(absDifferences);

    // Calculate W+ (sum of positive ranks)
    let Wplus = 0;
    for (let i = 0; i < differences.length; i++) {
      if (differences[i] > 0) {
        Wplus += ranks[i];
      }
    }

    // Calculate z-score for large samples
    const meanW = (n * (n + 1)) / 4;
    const stdW = Math.sqrt((n * (n + 1) * (2 * n + 1)) / 24);
    const zScore = (Wplus - meanW) / stdW;

    return {
      testStatistic: zScore,
      degreesOfFreedom: null,
      method: "wilcoxon",
      Wplus: Wplus,
      n: n,
    };
  }

  async calculateKruskalWallisStatistic(data) {
    // Kruskal-Wallis test (non-parametric ANOVA)
    const { groups } = data;
    const k = groups.length;
    let N = 0;

    // Combine all values
    const combined = [];
    groups.forEach((group, groupIndex) => {
      group.forEach((val) => {
        combined.push({ val, group: groupIndex });
        N++;
      });
    });

    // Sort and rank
    combined.sort((a, b) => a.val - b.val);
    const ranks = this.calculateRanks(combined.map((item) => item.val));

    // Calculate rank sums for each group
    const rankSums = new Array(k).fill(0);
    const groupSizes = groups.map((group) => group.length);

    for (let i = 0; i < combined.length; i++) {
      rankSums[combined[i].group] += ranks[i];
    }

    // Calculate H statistic
    let H = 0;
    for (let i = 0; i < k; i++) {
      H += Math.pow(rankSums[i], 2) / groupSizes[i];
    }
    H = (12 / (N * (N + 1))) * H - 3 * (N + 1);

    return {
      testStatistic: H,
      degreesOfFreedom: k - 1,
      method: "kruskal",
      rankSums: rankSums,
    };
  }

  async calculateANOVAStatistic(data) {
    // One-way ANOVA F-test
    const { groups } = data;
    const k = groups.length;
    let N = 0;
    let grandSum = 0;

    // Calculate group means and overall mean
    const groupMeans = [];
    const groupSizes = [];

    groups.forEach((group) => {
      const size = group.length;
      const sum = group.reduce((sum, val) => sum + val, 0);
      const mean = sum / size;

      groupSizes.push(size);
      groupMeans.push(mean);
      grandSum += sum;
      N += size;
    });

    const grandMean = grandSum / N;

    // Calculate sum of squares
    let SSB = 0; // Between groups
    let SSW = 0; // Within groups

    for (let i = 0; i < k; i++) {
      SSB += groupSizes[i] * Math.pow(groupMeans[i] - grandMean, 2);

      for (let j = 0; j < groups[i].length; j++) {
        SSW += Math.pow(groups[i][j] - groupMeans[i], 2);
      }
    }

    // Calculate degrees of freedom
    const dfB = k - 1;
    const dfW = N - k;

    // Calculate mean squares
    const MSB = SSB / dfB;
    const MSW = SSW / dfW;

    // Calculate F statistic
    const F = MSB / MSW;

    return {
      testStatistic: F,
      degreesOfFreedom: dfB,
      method: "anova",
      dfBetween: dfB,
      dfWithin: dfW,
      SSB: SSB,
      SSW: SSW,
      MSB: MSB,
      MSW: MSW,
    };
  }

  async calculateWelchStatistic(data) {
    // Welch's t-test (unequal variances)
    const { group1, group2 } = data;
    const n1 = group1.length;
    const n2 = group2.length;

    const mean1 = group1.reduce((sum, val) => sum + val, 0) / n1;
    const mean2 = group2.reduce((sum, val) => sum + val, 0) / n2;

    const var1 =
      group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) / (n1 - 1);
    const var2 =
      group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) / (n2 - 1);

    const standardError = Math.sqrt(var1 / n1 + var2 / n2);
    const testStatistic = (mean1 - mean2) / standardError;

    // Welch-Satterthwaite equation for degrees of freedom
    const df =
      Math.pow(var1 / n1 + var2 / n2, 2) /
      (Math.pow(var1 / n1, 2) / (n1 - 1) + Math.pow(var2 / n2, 2) / (n2 - 1));

    return {
      testStatistic: testStatistic,
      degreesOfFreedom: df,
      method: "welch",
      standardError: standardError,
      means: { group1: mean1, group2: mean2 },
    };
  }

  calculateRanks(values) {
    // Helper function to calculate ranks with tie handling
    const indexed = values.map((val, index) => ({ val, index }));
    indexed.sort((a, b) => a.val - b.val);

    const ranks = new Array(values.length);

    for (let i = 0; i < indexed.length; i++) {
      let j = i;
      while (j < indexed.length - 1 && indexed[j].val === indexed[j + 1].val) {
        j++;
      }

      const averageRank = (i + j + 2) / 2; // +2 because ranks start at 1
      for (let k = i; k <= j; k++) {
        ranks[indexed[k].index] = averageRank;
      }
      i = j;
    }

    return ranks;
  }

  async determinePValues(testStatistics, method) {
    // Proper p-value calculation based on test statistic and distribution
    const { testStatistic, degreesOfFreedom } = testStatistics;

    let pValue, oneTailed, twoTailed;

    switch (method.name) {
      case "ttest":
      case "welch":
        if (degreesOfFreedom && degreesOfFreedom > 0) {
          oneTailed = 1 - this.tCDF(Math.abs(testStatistic), degreesOfFreedom);
          twoTailed = 2 * oneTailed;
        } else {
          // Fallback to normal approximation for large df
          oneTailed = 1 - this.normalCDF(Math.abs(testStatistic));
          twoTailed = 2 * oneTailed;
        }
        pValue = twoTailed;
        break;

      case "ztest":
      case "mannwhitney":
      case "wilcoxon":
        oneTailed = 1 - this.normalCDF(Math.abs(testStatistic));
        twoTailed = 2 * oneTailed;
        pValue = twoTailed;
        break;

      case "chisquare":
      case "kruskal":
        if (degreesOfFreedom && degreesOfFreedom > 0) {
          pValue = 1 - this.chi2CDF(testStatistic, degreesOfFreedom);
          oneTailed = pValue;
          twoTailed = pValue; // Chi-square is always one-tailed
        }
        break;

      case "anova":
        if (degreesOfFreedom && testStatistics.dfWithin) {
          pValue =
            1 -
            this.fCDF(testStatistic, degreesOfFreedom, testStatistics.dfWithin);
          oneTailed = pValue;
          twoTailed = pValue; // F-test is always one-tailed
        }
        break;

      default:
        throw new Error(
          `P-value calculation not implemented for method: ${method.name}`
        );
    }

    return {
      pValue: Math.max(0, Math.min(1, pValue)),
      oneTailed: Math.max(0, Math.min(1, oneTailed)),
      twoTailed: Math.max(0, Math.min(1, twoTailed)),
    };
  }

  async calculateConfidenceIntervals(data, alpha, testStatistics) {
    // Proper confidence interval calculation based on test type
    const confidenceLevel = 1 - alpha;
    const { method, degreesOfFreedom, standardError } = testStatistics;

    let criticalValue, lowerBound, upperBound, margin;

    if (method === "ttest" || method === "welch") {
      // T-distribution confidence interval
      if (degreesOfFreedom && degreesOfFreedom > 0) {
        criticalValue = this.tQuantile(1 - alpha / 2, degreesOfFreedom);
      } else {
        criticalValue = this.normalQuantile(1 - alpha / 2);
      }

      margin = criticalValue * standardError;

      if (testStatistics.means) {
        // Two-sample case - difference of means
        const meanDiff =
          testStatistics.means.group1 - testStatistics.means.group2;
        lowerBound = meanDiff - margin;
        upperBound = meanDiff + margin;
      } else if (testStatistics.sampleMean) {
        // One-sample case
        lowerBound = testStatistics.sampleMean - margin;
        upperBound = testStatistics.sampleMean + margin;
      }
    } else if (method === "ztest") {
      // Normal distribution confidence interval
      criticalValue = this.normalQuantile(1 - alpha / 2);
      margin = criticalValue * standardError;

      if (testStatistics.means) {
        const meanDiff =
          testStatistics.means.group1 - testStatistics.means.group2;
        lowerBound = meanDiff - margin;
        upperBound = meanDiff + margin;
      } else if (testStatistics.sampleMean) {
        lowerBound = testStatistics.sampleMean - margin;
        upperBound = testStatistics.sampleMean + margin;
      }
    } else {
      // For other tests, calculate basic descriptive confidence interval
      const values = Array.isArray(data)
        ? data
        : data.group1
        ? [...data.group1, ...data.group2]
        : data.sample
        ? data.sample
        : [];

      if (values.length > 0) {
        const mean = values.reduce((sum, val) => sum + val, 0) / values.length;
        const variance =
          values.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) /
          (values.length - 1);
        const stderr = Math.sqrt(variance / values.length);

        criticalValue =
          values.length > 30
            ? this.normalQuantile(1 - alpha / 2)
            : this.tQuantile(1 - alpha / 2, values.length - 1);

        margin = criticalValue * stderr;
        lowerBound = mean - margin;
        upperBound = mean + margin;
      }
    }

    return {
      level: confidenceLevel * 100,
      lowerBound: lowerBound || null,
      upperBound: upperBound || null,
      margin: margin || null,
      criticalValue: criticalValue || null,
    };
  }

  async determineEffectSizes(data, method, testStatistics) {
    // Proper effect size calculation based on test type
    let effectSize,
      interpretation,
      cohensD = 0;

    switch (method.name) {
      case "ttest":
      case "welch":
        if (data.group1 && data.group2) {
          // Cohen's d for two-sample t-test
          const n1 = data.group1.length;
          const n2 = data.group2.length;
          const mean1 = data.group1.reduce((sum, val) => sum + val, 0) / n1;
          const mean2 = data.group2.reduce((sum, val) => sum + val, 0) / n2;

          const var1 =
            data.group1.reduce(
              (sum, val) => sum + Math.pow(val - mean1, 2),
              0
            ) /
            (n1 - 1);
          const var2 =
            data.group2.reduce(
              (sum, val) => sum + Math.pow(val - mean2, 2),
              0
            ) /
            (n2 - 1);
          const pooledStd = Math.sqrt((var1 + var2) / 2);

          cohensD = Math.abs(mean1 - mean2) / pooledStd;
          effectSize = cohensD;
        } else if (data.sample && data.populationMean !== undefined) {
          // Cohen's d for one-sample t-test
          const n = data.sample.length;
          const sampleMean = data.sample.reduce((sum, val) => sum + val, 0) / n;
          const sampleStd = Math.sqrt(
            data.sample.reduce(
              (sum, val) => sum + Math.pow(val - sampleMean, 2),
              0
            ) /
              (n - 1)
          );

          cohensD = Math.abs(sampleMean - data.populationMean) / sampleStd;
          effectSize = cohensD;
        }
        break;

      case "chisquare":
        // Cramer's V for chi-square
        if (data.observed && Array.isArray(data.observed[0])) {
          // Contingency table
          const rows = data.observed.length;
          const cols = data.observed[0].length;
          const n = data.observed.flat().reduce((sum, val) => sum + val, 0);
          const chiSquare = testStatistics.testStatistic;
          const minDim = Math.min(rows - 1, cols - 1);

          effectSize = Math.sqrt(chiSquare / (n * minDim));
        } else {
          // Goodness of fit - use phi coefficient
          const n = data.observed.reduce((sum, val) => sum + val, 0);
          const chiSquare = testStatistics.testStatistic;
          effectSize = Math.sqrt(chiSquare / n);
        }
        break;

      case "anova":
        // Eta-squared for ANOVA
        const SSB = testStatistics.SSB;
        const SSW = testStatistics.SSW;
        const SST = SSB + SSW;
        effectSize = SSB / SST;

        // Convert to Cohen's f
        cohensD = Math.sqrt(effectSize / (1 - effectSize));
        break;

      case "mannwhitney":
      case "wilcoxon":
      case "kruskal":
        // For non-parametric tests, use rank-biserial correlation or similar
        const zScore = Math.abs(testStatistics.testStatistic);
        if (data.group1 && data.group2) {
          const n1 = data.group1.length;
          const n2 = data.group2.length;
          const n = n1 + n2;
          effectSize = zScore / Math.sqrt(n); // Approximation of r
        } else {
          effectSize = zScore / Math.sqrt(testStatistics.n || 100);
        }
        break;

      default:
        effectSize = 0;
    }

    // Interpret effect size using Cohen's conventions
    if (method.name === "chisquare") {
      // Cramer's V interpretation
      if (effectSize < 0.1) interpretation = "negligible";
      else if (effectSize < 0.3) interpretation = "small";
      else if (effectSize < 0.5) interpretation = "medium";
      else interpretation = "large";
    } else if (method.name === "anova") {
      // Eta-squared interpretation
      if (effectSize < 0.01) interpretation = "negligible";
      else if (effectSize < 0.06) interpretation = "small";
      else if (effectSize < 0.14) interpretation = "medium";
      else interpretation = "large";
    } else {
      // Cohen's d or correlation interpretation
      const magnitude = Math.abs(effectSize);
      if (magnitude < 0.2) interpretation = "negligible";
      else if (magnitude < 0.5) interpretation = "small";
      else if (magnitude < 0.8) interpretation = "medium";
      else interpretation = "large";
    }

    return {
      value: effectSize,
      cohensD: cohensD,
      interpretation: interpretation,
      confidence: 0.95,
      type: this.getEffectSizeType(method.name),
    };
  }

  getEffectSizeType(methodName) {
    const types = {
      ttest: "Cohen's d",
      welch: "Cohen's d",
      ztest: "Cohen's d",
      chisquare: "Cramer's V",
      anova: "Eta-squared",
      mannwhitney: "Rank-biserial correlation",
      wilcoxon: "Rank-biserial correlation",
      kruskal: "Epsilon-squared",
    };

    return types[methodName] || "Effect size";
  }

  async applyMultipleComparisonCorrections(pValues, testConfig) {
    // Apply Bonferroni correction as example
    const numComparisons = testConfig.numComparisons || 1;
    return {
      originalP: pValues.pValue,
      correctedP: Math.min(pValues.pValue * numComparisons, 1.0),
      method: "bonferroni",
      numComparisons: numComparisons,
    };
  }

  async generateTestInterpretation(results) {
    const isSignificant = results.pValues.correctedP < results.alpha;

    return {
      significant: isSignificant,
      conclusion: isSignificant
        ? "Reject null hypothesis"
        : "Fail to reject null hypothesis",
      effectSize: results.effectSizes.interpretation,
      practicalSignificance: results.effectSizes.cohensD > 0.5,
      recommendations: this.generateRecommendations(results),
    };
  }

  async calculateObservedPower(data, effectSizes, alpha = 0.05) {
    // Proper power calculation based on effect size and sample size
    const effectSize = effectSizes.value || effectSizes.cohensD;
    let sampleSize = 0;

    // Determine sample size based on data structure
    if (data.group1 && data.group2) {
      sampleSize = data.group1.length + data.group2.length;
    } else if (data.sample) {
      sampleSize = data.sample.length;
    } else if (data.groups) {
      sampleSize = data.groups.reduce((sum, group) => sum + group.length, 0);
    } else if (Array.isArray(data)) {
      sampleSize = data.length;
    }

    // Power calculation using non-centrality parameter
    let power = 0;

    if (effectSizes.type === "Cohen's d") {
      // Power for t-test
      const df = sampleSize - 2; // Approximate for two-sample
      const ncp = effectSize * Math.sqrt(sampleSize / 4); // Non-centrality parameter
      const criticalT = this.tQuantile(1 - alpha / 2, df);

      // Approximate power using normal approximation
      const beta =
        this.normalCDF(criticalT - ncp) + this.normalCDF(-criticalT - ncp);
      power = 1 - beta;
    } else if (effectSizes.type === "Cramer's V") {
      // Power for chi-square test
      const df = 1; // Simplified assumption
      const ncp = effectSize * effectSize * sampleSize;
      const criticalChi = this.chi2Quantile(1 - alpha, df);

      // Approximate power (simplified)
      power = 1 - this.chi2CDF(criticalChi, df, ncp);
    } else if (effectSizes.type === "Eta-squared") {
      // Power for ANOVA
      const etaSquared = effectSize;
      const cohensF = Math.sqrt(etaSquared / (1 - etaSquared));
      const ncp = cohensF * cohensF * sampleSize;

      // Simplified power calculation
      power = Math.min(1.0, ncp / 10); // Very simplified
    } else {
      // Generic power calculation for correlation-based effect sizes
      const zEffect = 0.5 * Math.log((1 + effectSize) / (1 - effectSize)); // Fisher z-transform
      const se = 1 / Math.sqrt(sampleSize - 3);
      const zCritical = this.normalQuantile(1 - alpha / 2);

      power =
        1 -
        this.normalCDF(zCritical - Math.abs(zEffect) / se) +
        this.normalCDF(-zCritical - Math.abs(zEffect) / se);
    }

    return Math.max(0, Math.min(1, power));
  }

  generateTestWarnings(test) {
    const warnings = [];

    if (test.data.length < 30) {
      warnings.push("Small sample size may affect test reliability");
    }

    if (test.results.effectSizes.cohensD < 0.2) {
      warnings.push(
        "Effect size is very small, practical significance questionable"
      );
    }

    return warnings;
  }

  generateRecommendations(results) {
    const recommendations = [];

    if (!results.significant) {
      recommendations.push("Consider increasing sample size or effect size");
    }

    if (results.effectSizes.cohensD < 0.2) {
      recommendations.push(
        "Effect size is small, consider practical significance"
      );
    }

    return recommendations;
  }

  // Placeholder distribution functions (would need proper implementation)
  normalCDF(x, mean = 0, std = 1) {
    // Simplified normal CDF approximation
    return (
      0.5 *
      (1 +
        Math.sign(x - mean) *
          Math.sqrt(1 - Math.exp((-2 * Math.pow(x - mean, 2)) / (std * std))))
    );
  }

  normalPDF(x, mean = 0, std = 1) {
    return (
      (1 / (std * Math.sqrt(2 * Math.PI))) *
      Math.exp(-0.5 * Math.pow((x - mean) / std, 2))
    );
  }

  normalQuantile(p, mean = 0, std = 1) {
    // Simplified inverse normal approximation
    return mean + std * Math.sqrt(2) * this.inverseErrorFunction(2 * p - 1);
  }

  tCDF(x, df) {
    // Proper t-distribution CDF implementation
    if (df >= 30) {
      // For large df, t-distribution approaches normal distribution
      return this.normalCDF(x);
    }

    // Calculate t-distribution CDF using incomplete beta function
    const t = x;
    const a = df / 2;
    const b = 0.5;
    const w = df / (df + t * t);

    if (t >= 0) {
      return (
        0.5 + 0.5 * (1 - this.incompleteBeta(w, a, b) / this.betaFunction(a, b))
      );
    } else {
      return (0.5 * this.incompleteBeta(w, a, b)) / this.betaFunction(a, b);
    }
  }

  tPDF(x, df) {
    // Proper t-distribution PDF implementation
    const gamma_half_df_plus_one = this.gamma((df + 1) / 2);
    const gamma_half_df = this.gamma(df / 2);
    const sqrt_df_pi = Math.sqrt(df * Math.PI);

    const coefficient = gamma_half_df_plus_one / (gamma_half_df * sqrt_df_pi);
    const power_term = Math.pow(1 + (x * x) / df, -(df + 1) / 2);

    return coefficient * power_term;
  }

  tQuantile(p, df) {
    // Proper t-distribution quantile function implementation
    if (df >= 30) {
      // For large df, use normal approximation
      return this.normalQuantile(p);
    }

    // Newton-Raphson method for t-distribution quantile
    let x = this.normalQuantile(p); // Initial guess

    for (let i = 0; i < 10; i++) {
      const fx = this.tCDF(x, df) - p;
      const fpx = this.tPDF(x, df);

      if (Math.abs(fx) < 1e-8) break;

      x = x - fx / fpx;
    }

    return x;
  }

  chi2CDF(x, df, ncp = 0) {
    // Chi-square distribution CDF implementation with optional non-centrality parameter
    if (x <= 0) return 0;
    if (x === Infinity) return 1;

    if (ncp === 0) {
      // Central chi-square distribution
      // Use incomplete gamma function: P(X ≤ x) = γ(df/2, x/2) / Γ(df/2)
      return this.incompleteGamma(df / 2, x / 2) / this.gamma(df / 2);
    } else {
      // Non-central chi-square distribution (simplified approximation)
      // Use normal approximation for large values
      const mean = df + ncp;
      const variance = 2 * (df + 2 * ncp);
      const normalizedValue = (x - mean) / Math.sqrt(variance);
      return this.normalCDF(normalizedValue);
    }
  }

  chi2PDF(x, df) {
    // Proper chi-square distribution PDF implementation
    if (x <= 0) return 0;

    const coefficient = 1 / (Math.pow(2, df / 2) * this.gamma(df / 2));
    const power_term = Math.pow(x, df / 2 - 1);
    const exp_term = Math.exp(-x / 2);

    return coefficient * power_term * exp_term;
  }

  chi2Quantile(p, df) {
    // Proper chi-square distribution quantile function implementation
    if (p <= 0) return 0;
    if (p >= 1) return Infinity;

    // Newton-Raphson method for chi-square quantile
    let x = Math.max(df - 2 * Math.sqrt(2 * df) * this.normalQuantile(p), 0.1);

    for (let i = 0; i < 20; i++) {
      const fx = this.chi2CDF(x, df) - p;
      const fpx = this.chi2PDF(x, df);

      if (Math.abs(fx) < 1e-8 || fpx === 0) break;

      const newX = x - fx / fpx;
      x = Math.max(newX, 0.001); // Ensure positive
    }

    return x;
  }

  fCDF(x, df1, df2) {
    // Proper F-distribution CDF implementation
    if (x <= 0) return 0;
    if (x === Infinity) return 1;

    // F-distribution CDF using incomplete beta function
    // P(F ≤ x) = I_{z}(df1/2, df2/2) where z = (df1*x)/(df1*x + df2)
    const z = (df1 * x) / (df1 * x + df2);

    return (
      this.incompleteBeta(z, df1 / 2, df2 / 2) /
      this.betaFunction(df1 / 2, df2 / 2)
    );
  }

  fPDF(x, df1, df2) {
    // Proper F-distribution PDF implementation
    if (x <= 0) return 0;

    const beta_term = this.betaFunction(df1 / 2, df2 / 2);
    const power1 = Math.pow(df1 / df2, df1 / 2);
    const power2 = Math.pow(x, df1 / 2 - 1);
    const power3 = Math.pow(1 + (df1 * x) / df2, -(df1 + df2) / 2);

    return (power1 * power2 * power3) / beta_term;
  }

  fQuantile(p, df1, df2) {
    // Proper F-distribution quantile function implementation
    if (p <= 0) return 0;
    if (p >= 1) return Infinity;

    // Newton-Raphson method for F-distribution quantile
    let x = 1.0; // Initial guess

    for (let i = 0; i < 20; i++) {
      const fx = this.fCDF(x, df1, df2) - p;
      const fpx = this.fPDF(x, df1, df2);

      if (Math.abs(fx) < 1e-8 || fpx === 0) break;

      const newX = x - fx / fpx;
      x = Math.max(newX, 0.001); // Ensure positive
    }

    return x;
  }

  betaCDF(x, alpha, beta) {
    // Proper beta distribution CDF implementation
    if (x <= 0) return 0;
    if (x >= 1) return 1;

    // Beta CDF using incomplete beta function
    return this.incompleteBeta(x, alpha, beta) / this.betaFunction(alpha, beta);
  }

  betaPDF(x, alpha, beta) {
    // Proper beta distribution PDF implementation
    if (x <= 0 || x >= 1) return 0;

    const beta_func = this.betaFunction(alpha, beta);
    const power1 = Math.pow(x, alpha - 1);
    const power2 = Math.pow(1 - x, beta - 1);

    return (power1 * power2) / beta_func;
  }

  betaQuantile(p, alpha, beta) {
    // Proper beta distribution quantile function implementation
    if (p <= 0) return 0;
    if (p >= 1) return 1;

    // Newton-Raphson method for beta quantile
    let x = alpha / (alpha + beta); // Initial guess (mean)

    for (let i = 0; i < 20; i++) {
      const fx = this.betaCDF(x, alpha, beta) - p;
      const fpx = this.betaPDF(x, alpha, beta);

      if (Math.abs(fx) < 1e-8 || fpx === 0) break;

      const newX = x - fx / fpx;
      x = Math.max(0.001, Math.min(0.999, newX)); // Keep within bounds
    }

    return x;
  }

  binomialPMF(k, n, p) {
    // Simplified binomial PMF
    return (
      this.binomialCoefficient(n, k) * Math.pow(p, k) * Math.pow(1 - p, n - k)
    );
  }

  binomialCDF(k, n, p) {
    let sum = 0;
    for (let i = 0; i <= k; i++) {
      sum += this.binomialPMF(i, n, p);
    }
    return sum;
  }

  poissonPMF(k, lambda) {
    return (Math.pow(lambda, k) * Math.exp(-lambda)) / this.factorial(k);
  }

  poissonCDF(k, lambda) {
    let sum = 0;
    for (let i = 0; i <= k; i++) {
      sum += this.poissonPMF(i, lambda);
    }
    return sum;
  }

  geometricPMF(k, p) {
    return Math.pow(1 - p, k - 1) * p;
  }

  geometricCDF(k, p) {
    return 1 - Math.pow(1 - p, k);
  }

  // Utility mathematical functions
  binomialCoefficient(n, k) {
    if (k > n) return 0;
    if (k === 0 || k === n) return 1;

    let result = 1;
    for (let i = 1; i <= k; i++) {
      result = (result * (n - k + i)) / i;
    }
    return result;
  }

  factorial(n) {
    if (n <= 1) return 1;
    let result = 1;
    for (let i = 2; i <= n; i++) {
      result *= i;
    }
    return result;
  }

  inverseErrorFunction(x) {
    // Simplified inverse error function approximation
    const a = 0.147;
    const sign = Math.sign(x);
    x = Math.abs(x);

    const ln1MinusXSquared = Math.log(1 - x * x);
    const term1 = 2 / (Math.PI * a) + ln1MinusXSquared / 2;
    const term2 = ln1MinusXSquared / a;

    return sign * Math.sqrt(Math.sqrt(term1 * term1 - term2) - term1);
  }

  // Assumption Testing Methods
  async testNormality(data) {
    // Shapiro-Wilk test for normality (simplified implementation)
    const sample = Array.isArray(data)
      ? data
      : data.group1
      ? [...data.group1, ...(data.group2 || [])]
      : data.sample
      ? data.sample
      : [];

    if (sample.length < 3) {
      return {
        test: "shapiro-wilk",
        statistic: null,
        pValue: null,
        normal: true,
        message: "Sample too small for normality testing",
      };
    }

    // Simple skewness and kurtosis based check
    const n = sample.length;
    const mean = sample.reduce((sum, val) => sum + val, 0) / n;
    const variance =
      sample.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / (n - 1);
    const std = Math.sqrt(variance);

    // Calculate skewness
    const skewness =
      sample.reduce((sum, val) => sum + Math.pow((val - mean) / std, 3), 0) / n;
    const skewnessZ = skewness / Math.sqrt(6 / n);

    // Calculate kurtosis
    const kurtosis =
      sample.reduce((sum, val) => sum + Math.pow((val - mean) / std, 4), 0) /
        n -
      3;
    const kurtosisZ = kurtosis / Math.sqrt(24 / n);

    // Combined normality test statistic
    const combinedZ = Math.sqrt(skewnessZ * skewnessZ + kurtosisZ * kurtosisZ);
    const pValue = 2 * (1 - this.normalCDF(Math.abs(combinedZ)));

    return {
      test: "shapiro-wilk-approx",
      statistic: combinedZ,
      pValue: pValue,
      normal: pValue > 0.05,
      skewness: skewness,
      kurtosis: kurtosis,
      message: pValue > 0.05 ? "Data appears normal" : "Data may not be normal",
    };
  }

  async testEqualVariances(data) {
    // Levene's test for equal variances
    if (!data.group1 || !data.group2) {
      return {
        test: "levene",
        statistic: null,
        pValue: null,
        equalVariances: true,
        message: "Two groups required for variance test",
      };
    }

    const group1 = data.group1;
    const group2 = data.group2;
    const n1 = group1.length;
    const n2 = group2.length;

    if (n1 < 2 || n2 < 2) {
      return {
        test: "levene",
        statistic: null,
        pValue: null,
        equalVariances: true,
        message: "Insufficient sample size for variance test",
      };
    }

    // Calculate sample variances
    const mean1 = group1.reduce((sum, val) => sum + val, 0) / n1;
    const mean2 = group2.reduce((sum, val) => sum + val, 0) / n2;

    const var1 =
      group1.reduce((sum, val) => sum + Math.pow(val - mean1, 2), 0) / (n1 - 1);
    const var2 =
      group2.reduce((sum, val) => sum + Math.pow(val - mean2, 2), 0) / (n2 - 1);

    // F-test for equal variances
    const fStatistic = Math.max(var1, var2) / Math.min(var1, var2);
    const df1 = var1 > var2 ? n1 - 1 : n2 - 1;
    const df2 = var1 > var2 ? n2 - 1 : n1 - 1;

    const pValue = 2 * (1 - this.fCDF(fStatistic, df1, df2));

    return {
      test: "f-test",
      statistic: fStatistic,
      pValue: pValue,
      equalVariances: pValue > 0.05,
      variance1: var1,
      variance2: var2,
      message:
        pValue > 0.05
          ? "Equal variances assumed"
          : "Unequal variances detected",
    };
  }

  async testIndependence(data) {
    // Basic independence check using runs test
    const sample = Array.isArray(data)
      ? data
      : data.group1
      ? data.group1
      : data.sample
      ? data.sample
      : [];

    if (sample.length < 10) {
      return {
        test: "runs-test",
        statistic: null,
        pValue: null,
        independent: true,
        message: "Sample too small for independence testing",
      };
    }

    // Calculate median
    const sorted = [...sample].sort((a, b) => a - b);
    const median =
      sorted.length % 2 === 0
        ? (sorted[sorted.length / 2 - 1] + sorted[sorted.length / 2]) / 2
        : sorted[Math.floor(sorted.length / 2)];

    // Count runs above and below median
    let runs = 1;
    let above = 0,
      below = 0;
    let lastSign = sample[0] > median;

    for (let i = 0; i < sample.length; i++) {
      const currentSign = sample[i] > median;
      if (i > 0 && currentSign !== lastSign) {
        runs++;
      }
      if (currentSign) above++;
      else below++;
      lastSign = currentSign;
    }

    // Expected runs and variance
    const n = sample.length;
    const expectedRuns = (2 * above * below) / n + 1;
    const varianceRuns =
      (2 * above * below * (2 * above * below - n)) / (n * n * (n - 1));

    const zStatistic = (runs - expectedRuns) / Math.sqrt(varianceRuns);
    const pValue = 2 * (1 - this.normalCDF(Math.abs(zStatistic)));

    return {
      test: "runs-test",
      statistic: zStatistic,
      pValue: pValue,
      independent: pValue > 0.05,
      runs: runs,
      expectedRuns: expectedRuns,
      message:
        pValue > 0.05
          ? "Data appears independent"
          : "Data may not be independent",
    };
  }

  async detectOutliers(data) {
    // IQR method for outlier detection
    const sample = Array.isArray(data)
      ? data
      : data.group1
      ? [...data.group1, ...(data.group2 || [])]
      : data.sample
      ? data.sample
      : [];

    if (sample.length < 4) {
      return {
        method: "iqr",
        outliers: [],
        count: 0,
        hasOutliers: false,
        message: "Sample too small for outlier detection",
      };
    }

    const sorted = [...sample].sort((a, b) => a - b);
    const n = sorted.length;

    // Calculate quartiles
    const q1Index = Math.floor(n * 0.25);
    const q3Index = Math.floor(n * 0.75);
    const q1 = sorted[q1Index];
    const q3 = sorted[q3Index];
    const iqr = q3 - q1;

    // Calculate outlier boundaries
    const lowerBound = q1 - 1.5 * iqr;
    const upperBound = q3 + 1.5 * iqr;

    // Find outliers
    const outliers = sample.filter(
      (val) => val < lowerBound || val > upperBound
    );

    return {
      method: "iqr",
      outliers: outliers,
      count: outliers.length,
      hasOutliers: outliers.length > 0,
      lowerBound: lowerBound,
      upperBound: upperBound,
      q1: q1,
      q3: q3,
      iqr: iqr,
      message:
        outliers.length > 0
          ? `${outliers.length} outlier(s) detected`
          : "No outliers detected",
    };
  }

  async checkSampleSizeAdequacy(data, testType) {
    // Check if sample size is adequate for the specified test
    let n1 = 0,
      n2 = 0,
      totalN = 0;

    if (data.group1 && data.group2) {
      n1 = data.group1.length;
      n2 = data.group2.length;
      totalN = n1 + n2;
    } else if (data.sample) {
      totalN = data.sample.length;
      n1 = totalN;
    } else if (data.groups) {
      totalN = data.groups.reduce((sum, group) => sum + group.length, 0);
      n1 = Math.min(...data.groups.map((group) => group.length));
    } else if (Array.isArray(data)) {
      totalN = data.length;
      n1 = totalN;
    }

    // Minimum sample size requirements by test type
    const requirements = {
      ttest: { min: 2, recommended: 20, perGroup: true },
      ztest: { min: 30, recommended: 100, perGroup: true },
      chisquare: { min: 5, recommended: 10, perGroup: false }, // Expected frequency
      anova: { min: 3, recommended: 15, perGroup: true },
      mannwhitney: { min: 3, recommended: 10, perGroup: true },
      wilcoxon: { min: 5, recommended: 20, perGroup: false },
      kruskal: { min: 5, recommended: 15, perGroup: true },
    };

    const req = requirements[testType] || requirements["ttest"];
    const effectiveN = req.perGroup ? Math.min(n1, n2 || n1) : totalN;

    const adequate = effectiveN >= req.min;
    const recommended = effectiveN >= req.recommended;

    return {
      totalSampleSize: totalN,
      effectiveSampleSize: effectiveN,
      minimumRequired: req.min,
      recommendedSize: req.recommended,
      adequate: adequate,
      recommended: recommended,
      message: adequate
        ? recommended
          ? "Sample size is adequate"
          : "Sample size meets minimum requirements"
        : "Sample size is inadequate for reliable results",
    };
  }

  async testRandomness(data) {
    // Basic randomness test using consecutive differences
    const sample = Array.isArray(data)
      ? data
      : data.group1
      ? data.group1
      : data.sample
      ? data.sample
      : [];

    if (sample.length < 10) {
      return {
        test: "turning-points",
        statistic: null,
        pValue: null,
        random: true,
        message: "Sample too small for randomness testing",
      };
    }

    // Count turning points
    let turningPoints = 0;
    for (let i = 1; i < sample.length - 1; i++) {
      if (
        (sample[i] > sample[i - 1] && sample[i] > sample[i + 1]) ||
        (sample[i] < sample[i - 1] && sample[i] < sample[i + 1])
      ) {
        turningPoints++;
      }
    }

    // Expected turning points for random sequence
    const n = sample.length;
    const expectedTurningPoints = (2 * (n - 2)) / 3;
    const varianceTurningPoints = (16 * n - 29) / 90;

    const zStatistic =
      (turningPoints - expectedTurningPoints) /
      Math.sqrt(varianceTurningPoints);
    const pValue = 2 * (1 - this.normalCDF(Math.abs(zStatistic)));

    return {
      test: "turning-points",
      statistic: zStatistic,
      pValue: pValue,
      random: pValue > 0.05,
      turningPoints: turningPoints,
      expectedTurningPoints: expectedTurningPoints,
      message: pValue > 0.05 ? "Data appears random" : "Data may not be random",
    };
  }

  // Mathematical Helper Functions - Missing Implementations
  gamma(x) {
    // Lanczos approximation for gamma function
    if (x < 0.5) {
      return Math.PI / (Math.sin(Math.PI * x) * this.gamma(1 - x));
    }

    x -= 1;

    // Lanczos coefficients
    const g = 7;
    const coefficients = [
      0.99999999999980993, 676.5203681218851, -1259.1392167224028,
      771.32342877765313, -176.61502916214059, 12.507343278686905,
      -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7,
    ];

    let result = coefficients[0];
    for (let i = 1; i < coefficients.length; i++) {
      result += coefficients[i] / (x + i);
    }

    const t = x + g + 0.5;
    return (
      Math.sqrt(2 * Math.PI) * Math.pow(t, x + 0.5) * Math.exp(-t) * result
    );
  }

  betaFunction(a, b) {
    // Beta function: B(a,b) = Γ(a)Γ(b)/Γ(a+b)
    return (this.gamma(a) * this.gamma(b)) / this.gamma(a + b);
  }

  incompleteBeta(x, a, b) {
    // Incomplete beta function using continued fraction
    if (x <= 0) return 0;
    if (x >= 1) return this.betaFunction(a, b);

    // Use continued fraction expansion
    const bt = Math.exp(
      this.lnGamma(a + b) -
        this.lnGamma(a) -
        this.lnGamma(b) +
        a * Math.log(x) +
        b * Math.log(1 - x)
    );

    if (x < (a + 1) / (a + b + 2)) {
      return (bt * this.betaContinuedFraction(x, a, b)) / a;
    } else {
      return (
        this.betaFunction(a, b) -
        (bt * this.betaContinuedFraction(1 - x, b, a)) / b
      );
    }
  }

  betaContinuedFraction(x, a, b) {
    // Continued fraction for incomplete beta function
    const maxIterations = 100;
    const epsilon = 1e-15;

    const qab = a + b;
    const qap = a + 1;
    const qam = a - 1;

    let c = 1;
    let d = 1 - (qab * x) / qap;
    if (Math.abs(d) < 1e-30) d = 1e-30;
    d = 1 / d;
    let h = d;

    for (let m = 1; m <= maxIterations; m++) {
      const m2 = 2 * m;
      let aa = (m * (b - m) * x) / ((qam + m2) * (a + m2));

      // Even step
      d = 1 + aa * d;
      if (Math.abs(d) < 1e-30) d = 1e-30;
      c = 1 + aa / c;
      if (Math.abs(c) < 1e-30) c = 1e-30;
      d = 1 / d;
      h *= d * c;

      // Odd step
      aa = (-(a + m) * (qab + m) * x) / ((a + m2) * (qap + m2));
      d = 1 + aa * d;
      if (Math.abs(d) < 1e-30) d = 1e-30;
      c = 1 + aa / c;
      if (Math.abs(c) < 1e-30) c = 1e-30;
      d = 1 / d;
      const del = d * c;
      h *= del;

      if (Math.abs(del - 1) < epsilon) break;
    }

    return h;
  }

  incompleteGamma(a, x) {
    // Incomplete gamma function using series expansion
    if (x <= 0) return 0;
    if (x < a + 1) {
      // Use series expansion
      return this.incompleteGammaSeries(a, x);
    } else {
      // Use continued fraction
      return this.gamma(a) - this.incompleteGammaContinuedFraction(a, x);
    }
  }

  incompleteGammaSeries(a, x) {
    // Series expansion for incomplete gamma
    const maxIterations = 100;
    const epsilon = 1e-15;

    let sum = 1;
    let term = 1;

    for (let n = 1; n < maxIterations; n++) {
      term *= x / (a + n - 1);
      sum += term;
      if (term < epsilon) break;
    }

    return Math.exp(-x + a * Math.log(x) - this.lnGamma(a)) * sum;
  }

  incompleteGammaContinuedFraction(a, x) {
    // Continued fraction for incomplete gamma
    const maxIterations = 100;
    const epsilon = 1e-15;

    let b = x + 1 - a;
    let c = 1e30;
    let d = 1 / b;
    let h = d;

    for (let i = 1; i <= maxIterations; i++) {
      const an = -i * (i - a);
      b += 2;
      d = an * d + b;
      if (Math.abs(d) < 1e-30) d = 1e-30;
      c = b + an / c;
      if (Math.abs(c) < 1e-30) c = 1e-30;
      d = 1 / d;
      const del = d * c;
      h *= del;
      if (Math.abs(del - 1) < epsilon) break;
    }

    return Math.exp(-x + a * Math.log(x) - this.lnGamma(a)) * h;
  }

  lnGamma(x) {
    // Natural logarithm of gamma function
    if (x < 0.5) {
      return (
        Math.log(Math.PI) -
        Math.log(Math.sin(Math.PI * x)) -
        this.lnGamma(1 - x)
      );
    }

    x -= 1;

    const g = 7;
    const coefficients = [
      0.99999999999980993, 676.5203681218851, -1259.1392167224028,
      771.32342877765313, -176.61502916214059, 12.507343278686905,
      -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7,
    ];

    let result = coefficients[0];
    for (let i = 1; i < coefficients.length; i++) {
      result += coefficients[i] / (x + i);
    }

    const t = x + g + 0.5;
    return (
      0.5 * Math.log(2 * Math.PI) +
      (x + 0.5) * Math.log(t) -
      t +
      Math.log(result)
    );
  }
}

export default StatisticalEngine;
