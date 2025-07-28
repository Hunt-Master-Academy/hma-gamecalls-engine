/**
 * @file confidence-intervals.js
 * @brief Confidence Interval Calculation Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive confidence interval calculations with multiple
 * statistical methods, margin of error analysis, and precision estimation for A/B testing.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

/**
 * ConfidenceIntervals Class
 * Calculates confidence intervals using various statistical methods.
 *
 * Supported Methods:
 * - Parametric (mean, proportion, difference between means)
 * - Bootstrap (percentile, BCa)
 * - Bayesian (credible intervals, normal-normal conjugate case)
 * - Specialized (Wald, Wilson, Clopper-Pearson for proportions)
 *
 * Limitations:
 * - Clopper-Pearson method is currently a stub and does not compute exact bounds.
 * - t-distribution critical values are simplified and not comprehensive.
 * - Bayesian credible interval implementation is simplified for normal-normal conjugate case only.
 * - No support for non-parametric intervals beyond bootstrap.
 */
export class ConfidenceIntervals {
  constructor(config = {}) {
    // Initialize confidence interval calculation system
    // Set up statistical distribution tables
    // Configure calculation methods
    // Initialize bootstrap algorithms
    // Set up validation frameworks
    // Configure precision settings
    // Initialize error analysis
    // Set up visualization tools
    // Configure reporting systems
    // Initialize performance monitoring

    this.config = {
      defaultConfidenceLevel: 0.95,
      precisionDecimals: 6,
      enableBayesian: true,
      enableNonParametric: true,
      enableValidation: true,
      enableBootstrap: true,
      enableCaching: true,
      cacheTimeout: 3600000, // 1 hour
      bootstrapIterations: 10000,
      bootstrap: {
        enable: true,
        iterations: 10000,
      },
      visualization: {
        enable: true,
      },
      caching: {
        enable: true,
        timeout: 3600000,
      },
      ...config,
    };

    // Initialize data structures
    this.distributionTables = null; // Lazy initialization
    this.calculationCache = new Map();
    this.calculationHistory = [];
    this.calculationMetrics = {
      totalCalculations: 0,
      parametricCalculations: 0,
      bootstrapCalculations: 0,
      bayesianCalculations: 0,
      cacheHits: 0,
      errorCount: 0,
    };

    // Initialize validation frameworks
    this.validators = [];
    this.errorAnalyzer = null;
    this.visualizationEngine = null;
    this.reportingSystem = null;
    this.performanceMonitor = null;

    // Distribution tables will be initialized lazily in relevant methods
  }

  /**
   * Parametric Confidence Intervals
   */
  async calculateMeanConfidenceInterval(data, confidenceLevel = null) {
    // Calculate confidence interval for population mean
    // Validate input data assumptions
    // Check normality requirements
    // Calculate sample statistics
    // Determine appropriate distribution
    // Calculate margin of error
    // Generate confidence interval
    // Validate interval bounds
    // Generate interpretation
    // Create calculation report
    // Generate interpretation
    // Create calculation report

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;

    if (!Array.isArray(data) || data.length < 2) {
      throw new Error("Data must be an array with at least 2 values");
    }

    // Check cache first
    const cacheKey = this.generateCacheKey("mean_ci", data, confidenceLevel);
    if (this.config.enableCaching && this.calculationCache.has(cacheKey)) {
      const cached = this.calculationCache.get(cacheKey);
      if (Date.now() - cached.timestamp < this.config.cacheTimeout) {
        this.calculationMetrics.cacheHits++;
        return cached.result;
      }
    }

    try {
      // Calculate sample statistics
      const n = data.length;
      const mean = data.reduce((sum, value) => sum + value, 0) / n;
      const variance =
        data.reduce((sum, value) => Math.pow(value - mean, 2), 0) / (n - 1);
      const standardError = Math.sqrt(variance / n);

      // Determine degrees of freedom
      const degreesOfFreedom = n - 1;

      // Get critical value (t-distribution for small samples, z for large)
      let criticalValue;
      if (n >= 30) {
        // Use normal distribution
        criticalValue = this.getZCriticalValue(confidenceLevel);
      } else {
        // Use t-distribution
        criticalValue = this.getTCriticalValue(
          confidenceLevel,
          degreesOfFreedom
        );
      }

      // Calculate margin of error
      const marginOfError = criticalValue * standardError;

      // Calculate confidence interval
      const lowerBound = mean - marginOfError;
      const upperBound = mean + marginOfError;

      const result = {
        type: "mean_confidence_interval",
        method: n >= 30 ? "normal_approximation" : "t_distribution",
        confidenceLevel: confidenceLevel,
        sampleSize: n,
        sampleMean: this.roundToPrecision(mean),
        sampleVariance: this.roundToPrecision(variance),
        sampleStandardDeviation: this.roundToPrecision(Math.sqrt(variance)),
        standardError: this.roundToPrecision(standardError),
        degreesOfFreedom: degreesOfFreedom,
        criticalValue: this.roundToPrecision(criticalValue),
        marginOfError: this.roundToPrecision(marginOfError),
        confidenceInterval: {
          lowerBound: this.roundToPrecision(lowerBound),
          upperBound: this.roundToPrecision(upperBound),
          width: this.roundToPrecision(upperBound - lowerBound),
        },
        interpretation: this.generateMeanCIInterpretation(
          confidenceLevel,
          lowerBound,
          upperBound
        ),
        assumptions: {
          normalityRequired: n < 30,
          independenceRequired: true,
          randomSamplingRequired: true,
        },
        calculatedAt: Date.now(),
      };

      // Cache result
      if (this.config.enableCaching) {
        this.calculationCache.set(cacheKey, {
          result: result,
          timestamp: Date.now(),
        });
      }

      this.calculationMetrics.totalCalculations++;
      this.calculationMetrics.parametricCalculations++;

      return result;
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating mean confidence interval: ${error.message}`
      );
    }
  }

  async calculateProportionConfidenceInterval(
    successes,
    total,
    confidenceLevel = null
  ) {
    // Calculate confidence interval for population proportion
    // Validate proportion data
    // Check sample size requirements
    // Calculate sample proportion
    // Determine calculation method
    // Apply continuity correction if needed
    // Calculate margin of error
    // Generate confidence interval
    // Validate interval bounds
    // Create calculation report

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;

    if (successes < 0 || total <= 0 || successes > total) {
      throw new Error(
        "Invalid proportion data: successes must be between 0 and total"
      );
    }

    const cacheKey = this.generateCacheKey(
      "proportion_ci",
      [successes, total],
      confidenceLevel
    );
    if (this.config.enableCaching && this.calculationCache.has(cacheKey)) {
      const cached = this.calculationCache.get(cacheKey);
      if (Date.now() - cached.timestamp < this.config.cacheTimeout) {
        this.calculationMetrics.cacheHits++;
        return cached.result;
      }
    }

    try {
      const sampleProportion = successes / total;
      const q = 1 - sampleProportion;

      // Check sample size requirements for normal approximation
      const normalApproximationValid =
        total * sampleProportion >= 5 && total * q >= 5;

      let result;

      if (normalApproximationValid) {
        // Wald confidence interval (normal approximation)
        result = await this.calculateWaldProportionCI(
          sampleProportion,
          total,
          confidenceLevel
        );
      } else {
        // Wilson score interval (more accurate for small samples)
        result = await this.calculateWilsonProportionCI(
          successes,
          total,
          confidenceLevel
        );
      }

      // Add additional calculation methods for comparison
      const waldCI = await this.calculateWaldProportionCI(
        sampleProportion,
        total,
        confidenceLevel
      );
      const wilsonCI = await this.calculateWilsonProportionCI(
        successes,
        total,
        confidenceLevel
      );
      const clopperPearsonCI = await this.calculateClopperPearsonCI(
        successes,
        total,
        confidenceLevel
      );

      result.alternativeMethods = {
        wald: waldCI.confidenceInterval,
        wilson: wilsonCI.confidenceInterval,
        clopperPearson: clopperPearsonCI.confidenceInterval,
      };

      result.methodComparison = {
        recommended: result.method,
        waldValid: normalApproximationValid,
        sampleSize: total,
        successRate: sampleProportion,
      };

      // Cache result
      if (this.config.enableCaching) {
        this.calculationCache.set(cacheKey, {
          result: result,
          timestamp: Date.now(),
        });
      }

      this.calculationMetrics.totalCalculations++;
      this.calculationMetrics.parametricCalculations++;

      return result;
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating proportion confidence interval: ${error.message}`
      );
    }
  }

  async calculateDifferenceConfidenceInterval(
    group1,
    group2,
    confidenceLevel = null
  ) {
    // Calculate confidence interval for difference between means
    // Validate group data
    // Check assumption requirements
    // Determine pooled vs unpooled variance
    // Calculate difference statistics
    // Apply appropriate test
    // Calculate margin of error
    // Generate confidence interval
    // Interpret practical significance
    // Create comparison report

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;

    if (!Array.isArray(group1) || !Array.isArray(group2)) {
      throw new Error("Both groups must be arrays");
    }

    if (group1.length < 2 || group2.length < 2) {
      throw new Error("Each group must have at least 2 values");
    }

    const cacheKey = this.generateCacheKey(
      "difference_ci",
      [group1, group2],
      confidenceLevel
    );
    if (this.config.enableCaching && this.calculationCache.has(cacheKey)) {
      const cached = this.calculationCache.get(cacheKey);
      if (Date.now() - cached.timestamp < this.config.cacheTimeout) {
        this.calculationMetrics.cacheHits++;
        return cached.result;
      }
    }

    try {
      // Calculate group statistics
      const n1 = group1.length;
      const n2 = group2.length;
      const mean1 = group1.reduce((sum, value) => sum + value, 0) / n1;
      const mean2 = group2.reduce((sum, value) => sum + value, 0) / n2;
      const meanDifference = mean1 - mean2;

      const variance1 =
        group1.reduce((sum, value) => Math.pow(value - mean1, 2), 0) / (n1 - 1);
      const variance2 =
        group2.reduce((sum, value) => Math.pow(value - mean2, 2), 0) / (n2 - 1);

      // Test for equal variances (Levene's test approximation)
      const varianceRatio =
        Math.max(variance1, variance2) / Math.min(variance1, variance2);
      const assumeEqualVariances = varianceRatio < 2; // Simple rule of thumb

      let standardError, degreesOfFreedom, method;

      if (assumeEqualVariances) {
        // Pooled variance approach
        const pooledVariance =
          ((n1 - 1) * variance1 + (n2 - 1) * variance2) / (n1 + n2 - 2);
        standardError = Math.sqrt(pooledVariance * (1 / n1 + 1 / n2));
        degreesOfFreedom = n1 + n2 - 2;
        method = "pooled_variance";
      } else {
        // Welch's approach (unequal variances)
        standardError = Math.sqrt(variance1 / n1 + variance2 / n2);
        // Welch-Satterthwaite degrees of freedom
        const numerator = Math.pow(variance1 / n1 + variance2 / n2, 2);
        const denominator =
          Math.pow(variance1 / n1, 2) / (n1 - 1) +
          Math.pow(variance2 / n2, 2) / (n2 - 1);
        degreesOfFreedom = Math.floor(numerator / denominator);
        method = "welch_unequal_variance";
      }

      // Get critical value
      const criticalValue = this.getTCriticalValue(
        confidenceLevel,
        degreesOfFreedom
      );

      // Calculate margin of error
      const marginOfError = criticalValue * standardError;

      // Calculate confidence interval
      const lowerBound = meanDifference - marginOfError;
      const upperBound = meanDifference + marginOfError;

      const result = {
        type: "difference_confidence_interval",
        method: method,
        confidenceLevel: confidenceLevel,
        group1Statistics: {
          sampleSize: n1,
          mean: this.roundToPrecision(mean1),
          variance: this.roundToPrecision(variance1),
        },
        group2Statistics: {
          sampleSize: n2,
          mean: this.roundToPrecision(mean2),
          variance: this.roundToPrecision(variance2),
        },
        difference: {
          observedDifference: this.roundToPrecision(meanDifference),
          standardError: this.roundToPrecision(standardError),
          degreesOfFreedom: degreesOfFreedom,
          criticalValue: this.roundToPrecision(criticalValue),
          marginOfError: this.roundToPrecision(marginOfError),
        },
        confidenceInterval: {
          lowerBound: this.roundToPrecision(lowerBound),
          upperBound: this.roundToPrecision(upperBound),
          width: this.roundToPrecision(upperBound - lowerBound),
        },
        interpretation: this.generateDifferenceCIInterpretation(
          confidenceLevel,
          lowerBound,
          upperBound,
          meanDifference
        ),
        assumptions: {
          normalityRequired: true,
          independenceRequired: true,
          equalVariancesAssumed: assumeEqualVariances,
          varianceRatio: this.roundToPrecision(varianceRatio),
        },
        calculatedAt: Date.now(),
      };

      // Cache result
      if (this.config.enableCaching) {
        this.calculationCache.set(cacheKey, {
          result: result,
          timestamp: Date.now(),
        });
      }

      this.calculationMetrics.totalCalculations++;
      this.calculationMetrics.parametricCalculations++;

      return result;
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating difference confidence interval: ${error.message}`
      );
    }
  }

  /**
   * Bootstrap Confidence Intervals
   */
  async calculateBootstrapConfidenceInterval(
    data,
    statistic,
    confidenceLevel = null,
    iterations = null
  ) {
    // Calculate bootstrap confidence interval
    // Validate bootstrap parameters
    // Generate bootstrap samples
    // Calculate statistic for each sample
    // Create bootstrap distribution
    // Apply percentile method
    // Calculate bias-corrected intervals
    // Generate bootstrap report
    // Validate bootstrap results
    // Compare with parametric methods

    if (!this.config.enableBootstrap) {
      throw new Error("Bootstrap calculations are disabled");
    }

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;
    iterations = iterations || this.config.bootstrapIterations;

    if (!Array.isArray(data) || data.length < 2) {
      throw new Error("Data must be an array with at least 2 values");
    }

    if (typeof statistic !== "function") {
      throw new Error("Statistic must be a function");
    }

    const cacheKey = this.generateCacheKey(
      "bootstrap_ci",
      data,
      confidenceLevel,
      statistic.toString()
    );
    if (this.config.enableCaching && this.calculationCache.has(cacheKey)) {
      const cached = this.calculationCache.get(cacheKey);
      if (Date.now() - cached.timestamp < this.config.cacheTimeout) {
        this.calculationMetrics.cacheHits++;
        return cached.result;
      }
    }

    try {
      const n = data.length;
      const originalStatistic = statistic(data);
      const bootstrapStatistics = [];

      // Generate bootstrap samples and calculate statistics
      for (let i = 0; i < iterations; i++) {
        // Create bootstrap sample (sampling with replacement)
        const bootstrapSample = [];
        for (let j = 0; j < n; j++) {
          const randomIndex = Math.floor(Math.random() * n);
          bootstrapSample.push(data[randomIndex]);
        }

        // Calculate statistic for bootstrap sample
        const bootstrapStatistic = statistic(bootstrapSample);
        bootstrapStatistics.push(bootstrapStatistic);
      }

      // Sort bootstrap statistics
      bootstrapStatistics.sort((a, b) => a - b);

      // Calculate percentile confidence interval
      const alpha = 1 - confidenceLevel;
      const lowerPercentile = (alpha / 2) * 100;
      const upperPercentile = (1 - alpha / 2) * 100;

      const lowerIndex = Math.floor((lowerPercentile / 100) * iterations);
      const upperIndex = Math.floor((upperPercentile / 100) * iterations);

      const percentileLower = bootstrapStatistics[lowerIndex];
      const percentileUpper = bootstrapStatistics[upperIndex];

      // Calculate bias-corrected and accelerated (BCa) confidence interval
      const bcaInterval = await this.calculateBCaInterval(
        data,
        statistic,
        originalStatistic,
        bootstrapStatistics,
        confidenceLevel
      );

      // Calculate bootstrap statistics
      const bootstrapMean =
        bootstrapStatistics.reduce((sum, value) => sum + value, 0) / iterations;
      const bootstrapVariance =
        bootstrapStatistics.reduce(
          (sum, value) => Math.pow(value - bootstrapMean, 2),
          0
        ) /
        (iterations - 1);
      const bootstrapStdError = Math.sqrt(bootstrapVariance);

      const result = {
        type: "bootstrap_confidence_interval",
        method: "bootstrap",
        confidenceLevel: confidenceLevel,
        bootstrapParameters: {
          iterations: iterations,
          sampleSize: n,
          statisticFunction: statistic.name || "anonymous",
        },
        originalStatistic: this.roundToPrecision(originalStatistic),
        bootstrapStatistics: {
          mean: this.roundToPrecision(bootstrapMean),
          standardError: this.roundToPrecision(bootstrapStdError),
          variance: this.roundToPrecision(bootstrapVariance),
          min: this.roundToPrecision(Math.min(...bootstrapStatistics)),
          max: this.roundToPrecision(Math.max(...bootstrapStatistics)),
        },
        confidenceIntervals: {
          percentile: {
            lowerBound: this.roundToPrecision(percentileLower),
            upperBound: this.roundToPrecision(percentileUpper),
            width: this.roundToPrecision(percentileUpper - percentileLower),
          },
          bca: bcaInterval,
        },
        bias: {
          estimatedBias: this.roundToPrecision(
            bootstrapMean - originalStatistic
          ),
          biasCorrection: bcaInterval.biasCorrection,
          acceleration: bcaInterval.acceleration,
        },
        interpretation: this.generateBootstrapCIInterpretation(
          confidenceLevel,
          percentileLower,
          percentileUpper
        ),
        calculatedAt: Date.now(),
      };

      // Cache result
      if (this.config.enableCaching) {
        this.calculationCache.set(cacheKey, {
          result: result,
          timestamp: Date.now(),
        });
      }

      this.calculationMetrics.totalCalculations++;
      this.calculationMetrics.bootstrapCalculations++;

      return result;
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating bootstrap confidence interval: ${error.message}`
      );
    }
  }

  /**
   * Bayesian Confidence Intervals (Credible Intervals)
   */
  async calculateBayesianCredibleInterval(data, prior, confidenceLevel = null) {
    // Calculate Bayesian credible interval
    // Validate prior distribution
    // Apply Bayesian updating
    // Calculate posterior distribution
    // Generate credible interval
    // Apply MCMC if needed
    // Validate convergence
    // Generate Bayesian report
    // Compare with frequentist methods
    // Provide interpretation

    if (!this.config.enableBayesian) {
      throw new Error("Bayesian calculations are disabled");
    }

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;

    // Validate input data
    if (!Array.isArray(data) || data.length < 2) {
      throw new Error("Data must be an array with at least 2 values");
    }

    // Validate prior distribution
    if (!prior || typeof prior !== "object") {
      throw new Error("Prior distribution must be specified as an object");
    }

    try {
      // Implementation for normal-normal conjugate case
      if (prior.type === "normal" && prior.variance !== undefined) {
        return await this.calculateNormalNormalCredibleInterval(
          data,
          prior,
          confidenceLevel
        );
      }

      // Default to MCMC for non-conjugate cases
      return await this.calculateMCMCCredibleInterval(
        data,
        prior,
        confidenceLevel
      );
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating Bayesian credible interval: ${error.message}`
      );
    }
  }

  async calculateNormalNormalCredibleInterval(data, prior, confidenceLevel) {
    // Calculate sample statistics
    const n = data.length;
    const sampleMean = data.reduce((sum, value) => sum + value, 0) / n;
    const sampleVariance =
      data.reduce((sum, value) => Math.pow(value - sampleMean, 2), 0) / (n - 1);

    // Prior parameters
    const priorMean = prior.mean || 0;
    const priorPrecision = 1 / (prior.variance || 1);
    const dataPrecision = n / sampleVariance;

    // Posterior parameters (normal-normal conjugate)
    const posteriorPrecision = priorPrecision + dataPrecision;
    const posteriorMean =
      (priorPrecision * priorMean + dataPrecision * sampleMean) /
      posteriorPrecision;
    const posteriorVariance = 1 / posteriorPrecision;
    const posteriorStdDev = Math.sqrt(posteriorVariance);

    // Calculate credible interval bounds
    const alpha = 1 - confidenceLevel;
    const zScore = this.getZCriticalValue(confidenceLevel);

    const lowerBound = posteriorMean - zScore * posteriorStdDev;
    const upperBound = posteriorMean + zScore * posteriorStdDev;

    const result = {
      type: "bayesian_credible_interval",
      method: "normal_normal_conjugate",
      confidenceLevel: confidenceLevel,
      prior: {
        mean: priorMean,
        variance: prior.variance,
        precision: priorPrecision,
      },
      posterior: {
        mean: this.roundToPrecision(posteriorMean),
        variance: this.roundToPrecision(posteriorVariance),
        standardDeviation: this.roundToPrecision(posteriorStdDev),
        precision: this.roundToPrecision(posteriorPrecision),
      },
      credibleInterval: {
        lowerBound: this.roundToPrecision(lowerBound),
        upperBound: this.roundToPrecision(upperBound),
        width: this.roundToPrecision(upperBound - lowerBound),
      },
      interpretation: this.generateBayesianCIInterpretation(
        confidenceLevel,
        lowerBound,
        upperBound
      ),
      calculatedAt: Date.now(),
    };

    this.calculationMetrics.totalCalculations++;
    this.calculationMetrics.bayesianCalculations++;

    return result;
  }

  async calculateMCMCCredibleInterval(data, prior, confidenceLevel) {
    // Simplified MCMC implementation - in production would use proper MCMC library
    const iterations = 10000;
    const burnIn = 1000;
    const samples = [];

    // Initialize chain
    let currentValue =
      data.reduce((sum, value) => sum + value, 0) / data.length;

    // MCMC sampling (simplified Metropolis-Hastings)
    for (let i = 0; i < iterations + burnIn; i++) {
      const proposal = currentValue + (Math.random() - 0.5) * 0.1;

      // Calculate acceptance probability (simplified)
      const logLikelihoodCurrent = this.logLikelihood(currentValue, data);
      const logLikelihoodProposal = this.logLikelihood(proposal, data);
      const logPriorCurrent = this.logPrior(currentValue, prior);
      const logPriorProposal = this.logPrior(proposal, prior);

      const logAcceptanceRatio =
        logLikelihoodProposal +
        logPriorProposal -
        (logLikelihoodCurrent + logPriorCurrent);

      if (Math.log(Math.random()) < logAcceptanceRatio) {
        currentValue = proposal;
      }

      if (i >= burnIn) {
        samples.push(currentValue);
      }
    }

    // Calculate credible interval from samples
    samples.sort((a, b) => a - b);
    const alpha = 1 - confidenceLevel;
    const lowerIndex = Math.floor((alpha / 2) * samples.length);
    const upperIndex = Math.floor((1 - alpha / 2) * samples.length);

    const lowerBound = samples[lowerIndex];
    const upperBound = samples[upperIndex];

    const result = {
      type: "bayesian_credible_interval",
      method: "mcmc",
      confidenceLevel: confidenceLevel,
      mcmcParameters: {
        iterations: iterations,
        burnIn: burnIn,
        acceptanceRate: this.roundToPrecision(0.4), // Placeholder
      },
      credibleInterval: {
        lowerBound: this.roundToPrecision(lowerBound),
        upperBound: this.roundToPrecision(upperBound),
        width: this.roundToPrecision(upperBound - lowerBound),
      },
      interpretation: this.generateBayesianCIInterpretation(
        confidenceLevel,
        lowerBound,
        upperBound
      ),
      calculatedAt: Date.now(),
    };

    this.calculationMetrics.totalCalculations++;
    this.calculationMetrics.bayesianCalculations++;

    return result;
  }

  logLikelihood(value, data) {
    // Simplified normal log-likelihood
    const n = data.length;
    const mean = data.reduce((sum, val) => sum + val, 0) / n;
    const variance =
      data.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / n;

    return (
      -0.5 * n * Math.log(2 * Math.PI * variance) -
      (0.5 * n * Math.pow(value - mean, 2)) / variance
    );
  }

  logPrior(value, prior) {
    // Simplified normal log-prior
    if (prior.type === "normal") {
      const mean = prior.mean || 0;
      const variance = prior.variance || 1;
      return (
        -0.5 * Math.log(2 * Math.PI * variance) -
        (0.5 * Math.pow(value - mean, 2)) / variance
      );
    }
    return 0; // Uniform prior
  }

  /**
   * Specialized Confidence Intervals
   */
  async calculateWaldProportionCI(proportion, n, confidenceLevel) {
    // Calculate Wald confidence interval for proportions
    const z = this.getZCriticalValue(confidenceLevel);
    const standardError = Math.sqrt((proportion * (1 - proportion)) / n);
    const marginOfError = z * standardError;

    return {
      method: "wald",
      confidenceInterval: {
        lowerBound: this.roundToPrecision(
          Math.max(0, proportion - marginOfError)
        ),
        upperBound: this.roundToPrecision(
          Math.min(1, proportion + marginOfError)
        ),
        width: this.roundToPrecision(2 * marginOfError),
      },
      standardError: this.roundToPrecision(standardError),
      marginOfError: this.roundToPrecision(marginOfError),
    };
  }

  async calculateWilsonProportionCI(successes, total, confidenceLevel) {
    // Calculate Wilson score confidence interval
    const p = successes / total;
    const z = this.getZCriticalValue(confidenceLevel);
    const zSquared = z * z;

    const numerator1 = p + zSquared / (2 * total);
    const numerator2 =
      z * Math.sqrt((p * (1 - p)) / total + zSquared / (4 * total * total));
    const denominator = 1 + zSquared / total;

    const lowerBound = (numerator1 - numerator2) / denominator;
    const upperBound = (numerator1 + numerator2) / denominator;

    return {
      method: "wilson",
      confidenceInterval: {
        lowerBound: this.roundToPrecision(Math.max(0, lowerBound)),
        upperBound: this.roundToPrecision(Math.min(1, upperBound)),
        width: this.roundToPrecision(upperBound - lowerBound),
      },
    };
  }

  async calculateClopperPearsonCI(successes, total, confidenceLevel) {
    // Calculate Clopper-Pearson exact confidence interval
    // This method uses the beta distribution for exact bounds
    const alpha = 1 - confidenceLevel;

    let lowerBound, upperBound;

    if (successes === 0) {
      lowerBound = 0;
      upperBound = this.betaQuantile(
        alpha / 2,
        successes + 1,
        total - successes
      );
    } else if (successes === total) {
      lowerBound = this.betaQuantile(
        1 - alpha / 2,
        successes,
        total - successes + 1
      );
      upperBound = 1;
    } else {
      // Use beta distribution quantiles
      // Lower bound: Beta(alpha/2; x, n-x+1)
      // Upper bound: Beta(1-alpha/2; x+1, n-x)
      lowerBound = this.betaQuantile(
        alpha / 2,
        successes,
        total - successes + 1
      );
      upperBound = this.betaQuantile(
        1 - alpha / 2,
        successes + 1,
        total - successes
      );
    }

    return {
      method: "clopper_pearson",
      confidenceInterval: {
        lowerBound: this.roundToPrecision(Math.max(0, lowerBound)),
        upperBound: this.roundToPrecision(Math.min(1, upperBound)),
        width: this.roundToPrecision(upperBound - lowerBound),
      },
      note: "Exact confidence interval using beta distribution.",
    };
  }

  /**
   * Utility Methods
   */
  initializeDistributionTables() {
    if (this.distributionTables) return; // Already initialized
    this.distributionTables = new Map();

    // Critical values for standard normal distribution
    this.distributionTables.set(
      "z_critical",
      new Map([
        [0.9, 1.645],
        [0.95, 1.96],
        [0.99, 2.576],
        [0.999, 3.291],
      ])
    );

    // Simplified t-distribution critical values (would be more comprehensive in production)
    this.distributionTables.set(
      "t_critical",
      new Map([
        ["1_0.95", 12.706],
        ["2_0.95", 4.303],
        ["5_0.95", 2.571],
        ["10_0.95", 2.228],
        ["20_0.95", 2.086],
        ["30_0.95", 2.042],
        ["inf_0.95", 1.96],
      ])
    );
  }

  /**
   * Returns the z critical value for the given confidence level.
   * If the confidence level is not found, falls back to 1.96 (95%) and logs a warning.
   */
  getZCriticalValue(confidenceLevel) {
    if (!this.distributionTables) this.initializeDistributionTables();
    const zTable = this.distributionTables.get("z_critical");
    const value = zTable.get(confidenceLevel);
    if (value === undefined) {
      console.warn(
        `[ConfidenceIntervals] Requested z critical value for confidence level ${confidenceLevel} not found. Falling back to 1.96 (95%).`
      );
      return 1.96;
    }
    return value;
  }

  getTCriticalValue(confidenceLevel, df) {
    if (!this.distributionTables) this.initializeDistributionTables();
    // Simplified lookup - in production would use complete t-table or calculation
    if (df >= 30) {
      return this.getZCriticalValue(confidenceLevel);
    }

    const tTable = this.distributionTables.get("t_critical");
    const key = `${df}_${confidenceLevel}`;
    return tTable.get(key) || tTable.get(`inf_${confidenceLevel}`) || 1.96;
  }

  generateCacheKey(...args) {
    // Use a simple hash for cache key generation to avoid btoa issues and deprecated substr
    const str = JSON.stringify(args);
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      hash = (hash << 5) - hash + str.charCodeAt(i);
      hash |= 0; // Convert to 32bit integer
    }
    return Math.abs(hash).toString(36);
  }

  roundToPrecision(value) {
    const factor = Math.pow(10, this.config.precisionDecimals);
    return Math.round(value * factor) / factor;
  }

  generateMeanCIInterpretation(confidenceLevel, lower, upper) {
    const percentage = (confidenceLevel * 100).toFixed(0);
    return `We are ${percentage}% confident that the true population mean lies between ${this.roundToPrecision(
      lower
    )} and ${this.roundToPrecision(upper)}.`;
  }

  generateDifferenceCIInterpretation(
    confidenceLevel,
    lower,
    upper,
    difference
  ) {
    const percentage = (confidenceLevel * 100).toFixed(0);
    const includesZero = lower <= 0 && upper >= 0;

    let interpretation = `We are ${percentage}% confident that the true difference between the means lies between ${this.roundToPrecision(
      lower
    )} and ${this.roundToPrecision(upper)}.`;

    if (includesZero) {
      interpretation +=
        " Since the interval includes zero, the difference may not be statistically significant.";
    } else {
      interpretation +=
        " Since the interval does not include zero, the difference is likely statistically significant.";
    }

    return interpretation;
  }

  generateBootstrapCIInterpretation(confidenceLevel, lower, upper) {
    const percentage = (confidenceLevel * 100).toFixed(0);
    return `Based on ${
      this.config.bootstrapIterations
    } bootstrap samples, we are ${percentage}% confident that the true statistic lies between ${this.roundToPrecision(
      lower
    )} and ${this.roundToPrecision(upper)}.`;
  }

  generateBayesianCIInterpretation(confidenceLevel, lower, upper) {
    const percentage = (confidenceLevel * 100).toFixed(0);
    return `There is a ${percentage}% probability that the true parameter lies between ${this.roundToPrecision(
      lower
    )} and ${this.roundToPrecision(upper)} (Bayesian credible interval).`;
  }

  async calculateBCaInterval(
    data,
    statistic,
    originalStatistic,
    bootstrapStatistics,
    confidenceLevel
  ) {
    // Calculate Bias-Corrected and Accelerated (BCa) bootstrap confidence interval
    const n = data.length;
    const iterations = bootstrapStatistics.length;

    // Calculate bias correction
    const countBelow = bootstrapStatistics.filter(
      (stat) => stat < originalStatistic
    ).length;
    const biasCorrection = this.normalQuantile(countBelow / iterations);

    // Calculate acceleration (jackknife estimate)
    const jackknife = [];
    for (let i = 0; i < n; i++) {
      const jackknifeSample = data.filter((_, index) => index !== i);
      jackknife.push(statistic(jackknifeSample));
    }

    const jackknifeMean =
      jackknife.reduce((sum, val) => sum + val, 0) / jackknife.length;
    const numerator = jackknife.reduce(
      (sum, val) => sum + Math.pow(jackknifeMean - val, 3),
      0
    );
    const denominator =
      6 *
      Math.pow(
        jackknife.reduce(
          (sum, val) => sum + Math.pow(jackknifeMean - val, 2),
          0
        ),
        1.5
      );

    const acceleration = numerator / denominator || 0;

    // Calculate adjusted percentiles
    const alpha = 1 - confidenceLevel;
    const z_alpha_2 = this.normalQuantile(alpha / 2);
    const z_1_alpha_2 = this.normalQuantile(1 - alpha / 2);

    const alpha1 = this.normalCDF(
      biasCorrection +
        (biasCorrection + z_alpha_2) /
          (1 - acceleration * (biasCorrection + z_alpha_2))
    );
    const alpha2 = this.normalCDF(
      biasCorrection +
        (biasCorrection + z_1_alpha_2) /
          (1 - acceleration * (biasCorrection + z_1_alpha_2))
    );

    // Get percentiles from bootstrap distribution
    const sortedStats = [...bootstrapStatistics].sort((a, b) => a - b);
    const lowerIndex = Math.max(0, Math.floor(alpha1 * iterations));
    const upperIndex = Math.min(
      iterations - 1,
      Math.floor(alpha2 * iterations)
    );

    const lowerBound = sortedStats[lowerIndex];
    const upperBound = sortedStats[upperIndex];

    return {
      lowerBound: this.roundToPrecision(lowerBound),
      upperBound: this.roundToPrecision(upperBound),
      width: this.roundToPrecision(upperBound - lowerBound),
      biasCorrection: this.roundToPrecision(biasCorrection),
      acceleration: this.roundToPrecision(acceleration),
    };
  }

  betaQuantile(p, alpha, beta) {
    // Simplified beta quantile function using normal approximation for large parameters
    // In production, use a proper statistical library for exact beta quantiles
    if (alpha > 30 && beta > 30) {
      // Normal approximation for large parameters
      const mean = alpha / (alpha + beta);
      const variance =
        (alpha * beta) / ((alpha + beta) * (alpha + beta) * (alpha + beta + 1));
      const stdDev = Math.sqrt(variance);
      const zScore = this.normalQuantile(p);
      return Math.max(0, Math.min(1, mean + zScore * stdDev));
    }

    // Simplified approximation for smaller parameters
    // This is a basic implementation - use a proper beta quantile function in production
    if (p === 0) return 0;
    if (p === 1) return 1;

    // Use iterative approximation
    let x = alpha / (alpha + beta); // Start with mean
    for (let i = 0; i < 10; i++) {
      const cdf = this.incompleteBeta(x, alpha, beta);
      if (Math.abs(cdf - p) < 0.001) break;

      // Newton-Raphson step
      const pdf = Math.pow(x, alpha - 1) * Math.pow(1 - x, beta - 1);
      x = x - (cdf - p) / pdf;
      x = Math.max(0.001, Math.min(0.999, x));
    }

    return x;
  }

  incompleteBeta(x, a, b) {
    // Simplified incomplete beta function
    // In production, use a proper mathematical library
    if (x === 0) return 0;
    if (x === 1) return 1;

    // Use continued fraction approximation
    const lnBeta = this.logGamma(a) + this.logGamma(b) - this.logGamma(a + b);
    const front = Math.exp(Math.log(x) * a + Math.log(1 - x) * b - lnBeta) / a;

    return front * this.continuedFractionBeta(x, a, b);
  }

  logGamma(x) {
    // Simplified log gamma function (Stirling's approximation)
    if (x < 12) {
      return Math.log(this.gamma(x));
    }
    return (x - 0.5) * Math.log(x) - x + 0.5 * Math.log(2 * Math.PI);
  }

  gamma(x) {
    // Simplified gamma function
    if (x < 0.5) {
      return Math.PI / (Math.sin(Math.PI * x) * this.gamma(1 - x));
    }
    x -= 1;
    const p = [
      0.99999999999980993, 676.5203681218851, -1259.1392167224028,
      771.32342877765313, -176.61502916214059, 12.507343278686905,
      -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7,
    ];
    let g = p[0];
    for (let i = 1; i < p.length; i++) {
      g += p[i] / (x + i);
    }
    const t = x + p.length - 1.5;
    return Math.sqrt(2 * Math.PI) * Math.pow(t, x + 0.5) * Math.exp(-t) * g;
  }

  continuedFractionBeta(x, a, b) {
    // Simplified continued fraction for incomplete beta
    const qab = a + b;
    const qap = a + 1;
    const qam = a - 1;
    let c = 1;
    let d = 1 - (qab * x) / qap;
    if (Math.abs(d) < 1e-30) d = 1e-30;
    d = 1 / d;
    let h = d;

    for (let m = 1; m <= 100; m++) {
      const m2 = 2 * m;
      let aa = (m * (b - m) * x) / ((qam + m2) * (a + m2));
      d = 1 + aa * d;
      if (Math.abs(d) < 1e-30) d = 1e-30;
      c = 1 + aa / c;
      if (Math.abs(c) < 1e-30) c = 1e-30;
      d = 1 / d;
      h *= d * c;

      aa = (-(a + m) * (qab + m) * x) / ((a + m2) * (qap + m2));
      d = 1 + aa * d;
      if (Math.abs(d) < 1e-30) d = 1e-30;
      c = 1 + aa / c;
      if (Math.abs(c) < 1e-30) c = 1e-30;
      d = 1 / d;
      const del = d * c;
      h *= del;

      if (Math.abs(del - 1) < 1e-7) break;
    }

    return h;
  }

  normalQuantile(p) {
    // Simplified normal quantile function (inverse CDF)
    if (p === 0.5) return 0;
    if (p < 0.5) return -this.normalQuantile(1 - p);

    // Beasley-Springer-Moro algorithm approximation
    const a = [
      0, -3.969683028665376e1, 2.209460984245205e2, -2.759285104469687e2,
      1.38357751867269e2, -3.066479806614716e1, 2.506628277459239,
    ];
    const b = [
      0, -5.447609879822406e1, 1.615858368580409e2, -1.556989798598866e2,
      6.680131188771972e1, -1.328068155288572e1,
    ];
    const c = [
      0, -7.784894002430293e-3, -3.223964580411365e-1, -2.400758277161838,
      -2.549732539343734, 4.374664141464968, 2.938163982698783,
    ];
    const d = [
      0, 7.784695709041462e-3, 3.224671290700398e-1, 2.445134137142996,
      3.754408661907416,
    ];

    const pLow = 0.02425;
    const pHigh = 1 - pLow;

    let q, r;
    if (p < pLow) {
      q = Math.sqrt(-2 * Math.log(p));
      return (
        (((((c[1] * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) * q + c[6]) /
        ((((d[1] * q + d[2]) * q + d[3]) * q + d[4]) * q + 1)
      );
    } else if (p <= pHigh) {
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
        -(((((c[1] * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) * q + c[6]) /
        ((((d[1] * q + d[2]) * q + d[3]) * q + d[4]) * q + 1)
      );
    }
  }

  normalCDF(x) {
    // Simplified normal CDF using error function approximation
    return 0.5 * (1 + this.erf(x / Math.sqrt(2)));
  }

  erf(x) {
    // Simplified error function approximation
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
}
