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
 * Calculates confidence intervals using various statistical methods
 */
export class ConfidenceIntervals {
  constructor(config = {}) {
    // TODO: Initialize confidence interval calculation system
    // TODO: Set up statistical distribution tables
    // TODO: Configure calculation methods
    // TODO: Initialize bootstrap algorithms
    // TODO: Set up validation frameworks
    // TODO: Configure precision settings
    // TODO: Initialize error analysis
    // TODO: Set up visualization tools
    // TODO: Configure reporting systems
    // TODO: Initialize performance monitoring

    this.config = {
      defaultConfidenceLevel: 0.95,
      bootstrapIterations: 10000,
      enableBootstrap: true,
      enableBayesian: true,
      enableNonParametric: true,
      precisionDecimals: 6,
      enableValidation: true,
      enableVisualization: true,
      enableCaching: true,
      cacheTimeout: 3600000, // 1 hour
      ...config,
    };

    this.distributionTables = new Map();
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

    this.initializeDistributionTables();
  }

  /**
   * Parametric Confidence Intervals
   */
  async calculateMeanConfidenceInterval(data, confidenceLevel = null) {
    // TODO: Calculate confidence interval for population mean
    // TODO: Validate input data assumptions
    // TODO: Check normality requirements
    // TODO: Calculate sample statistics
    // TODO: Determine appropriate distribution
    // TODO: Calculate margin of error
    // TODO: Generate confidence interval
    // TODO: Validate interval bounds
    // TODO: Generate interpretation
    // TODO: Create calculation report

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
    // TODO: Calculate confidence interval for population proportion
    // TODO: Validate proportion data
    // TODO: Check sample size requirements
    // TODO: Calculate sample proportion
    // TODO: Determine calculation method
    // TODO: Apply continuity correction if needed
    // TODO: Calculate margin of error
    // TODO: Generate confidence interval
    // TODO: Validate interval bounds
    // TODO: Create calculation report

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
    // TODO: Calculate confidence interval for difference between means
    // TODO: Validate group data
    // TODO: Check assumption requirements
    // TODO: Determine pooled vs unpooled variance
    // TODO: Calculate difference statistics
    // TODO: Apply appropriate test
    // TODO: Calculate margin of error
    // TODO: Generate confidence interval
    // TODO: Interpret practical significance
    // TODO: Create comparison report

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
    // TODO: Calculate bootstrap confidence interval
    // TODO: Validate bootstrap parameters
    // TODO: Generate bootstrap samples
    // TODO: Calculate statistic for each sample
    // TODO: Create bootstrap distribution
    // TODO: Apply percentile method
    // TODO: Calculate bias-corrected intervals
    // TODO: Generate bootstrap report
    // TODO: Validate bootstrap results
    // TODO: Compare with parametric methods

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
    // TODO: Calculate Bayesian credible interval
    // TODO: Validate prior distribution
    // TODO: Apply Bayesian updating
    // TODO: Calculate posterior distribution
    // TODO: Generate credible interval
    // TODO: Apply MCMC if needed
    // TODO: Validate convergence
    // TODO: Generate Bayesian report
    // TODO: Compare with frequentist methods
    // TODO: Provide interpretation

    if (!this.config.enableBayesian) {
      throw new Error("Bayesian calculations are disabled");
    }

    confidenceLevel = confidenceLevel || this.config.defaultConfidenceLevel;

    // Implementation would depend on specific prior and likelihood
    // This is a simplified example for normal-normal conjugate case

    try {
      const result = {
        type: "bayesian_credible_interval",
        method: "bayesian",
        confidenceLevel: confidenceLevel,
        prior: prior,
        posterior: {
          // Posterior parameters would be calculated here
        },
        credibleInterval: {
          // Credible interval bounds would be calculated here
        },
        interpretation: "Bayesian credible interval interpretation",
        calculatedAt: Date.now(),
      };

      this.calculationMetrics.totalCalculations++;
      this.calculationMetrics.bayesianCalculations++;

      return result;
    } catch (error) {
      this.calculationMetrics.errorCount++;
      throw new Error(
        `Error calculating Bayesian credible interval: ${error.message}`
      );
    }
  }

  /**
   * Specialized Confidence Intervals
   */
  async calculateWaldProportionCI(proportion, n, confidenceLevel) {
    // TODO: Calculate Wald confidence interval for proportions
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
    // TODO: Calculate Wilson score confidence interval
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
    // TODO: Calculate Clopper-Pearson exact confidence interval
    // This would use the beta distribution
    // Simplified implementation
    return {
      method: "clopper_pearson",
      confidenceInterval: {
        lowerBound: 0, // Would calculate using beta distribution
        upperBound: 1, // Would calculate using beta distribution
        width: 1,
      },
    };
  }

  /**
   * Utility Methods
   */
  initializeDistributionTables() {
    // TODO: Initialize statistical distribution lookup tables
    // TODO: Set up t-distribution table
    // TODO: Configure z-distribution values
    // TODO: Initialize chi-square table
    // TODO: Set up F-distribution table
    // TODO: Configure beta distribution
    // TODO: Initialize gamma distribution
    // TODO: Set up normal distribution
    // TODO: Configure critical value tables
    // TODO: Initialize lookup optimizations

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

  getZCriticalValue(confidenceLevel) {
    const zTable = this.distributionTables.get("z_critical");
    return zTable.get(confidenceLevel) || 1.96; // Default to 95%
  }

  getTCriticalValue(confidenceLevel, df) {
    // Simplified lookup - in production would use complete t-table or calculation
    if (df >= 30) {
      return this.getZCriticalValue(confidenceLevel);
    }

    const tTable = this.distributionTables.get("t_critical");
    const key = `${df}_${confidenceLevel}`;
    return tTable.get(key) || tTable.get(`inf_${confidenceLevel}`) || 1.96;
  }

  generateCacheKey(...args) {
    return btoa(JSON.stringify(args)).substr(0, 16);
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
}
