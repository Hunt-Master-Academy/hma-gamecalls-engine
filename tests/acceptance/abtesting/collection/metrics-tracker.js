/**
 * @file metrics-tracker.js
 * @brief Metrics Collection and Tracking Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive metrics tracking with custom metrics definition,
 * real-time collection, and statistical analysis for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

/**
 * MetricsTracker Class
 * Manages comprehensive metrics collection and analysis for A/B testing
 */
export class MetricsTracker {
  constructor(config = {}) {
    // TODO: Initialize metrics tracking system
    // TODO: Set up custom metrics registry
    // TODO: Configure real-time collection
    // TODO: Initialize statistical analysis
    // TODO: Set up metrics aggregation
    // TODO: Configure metrics validation
    // TODO: Initialize metrics storage
    // TODO: Set up metrics monitoring
    // TODO: Configure metrics alerting
    // TODO: Initialize metrics reporting

    this.config = {
      maxMetrics: 1000,
      enableRealTimeCollection: true,
      enableStatisticalAnalysis: true,
      aggregationInterval: 60000, // 1 minute
      retentionPeriod: 90 * 24 * 60 * 60 * 1000, // 90 days
      enableMetricsValidation: true,
      enableAnomalyDetection: true,
      enablePredictiveAnalysis: true,
      enableMetricsOptimization: true,
      enableCustomMetrics: true,
      ...config,
    };

    this.metrics = new Map();
    this.customMetrics = new Map();
    this.metricDefinitions = new Map();
    this.aggregatedMetrics = new Map();
    this.realTimeMetrics = new Map();
    this.metricsHistory = new Map();
    this.trackingMetrics = {
      totalMetrics: 0,
      activeMetrics: 0,
      customMetrics: 0,
      errorCount: 0,
      processingLatency: 0,
    };

    this.eventHandlers = new Map();
    this.aggregators = new Map();
    this.validators = [];
    this.anomalyDetectors = new Map();

    this.initializeStandardMetrics();
  }

  /**
   * Metrics Definition and Registration
   */
  async defineMetric(metricConfig) {
    // TODO: Define new custom metric
    // TODO: Validate metric configuration
    // TODO: Set up metric calculation logic
    // TODO: Configure metric aggregation
    // TODO: Initialize metric tracking
    // TODO: Set up metric validation
    // TODO: Configure metric alerts
    // TODO: Initialize metric storage
    // TODO: Set up metric monitoring
    // TODO: Generate metric documentation

    const metricId = this.generateMetricId();
    const timestamp = Date.now();

    const metricDefinition = {
      id: metricId,
      name: metricConfig.name || `metric_${metricId}`,
      description: metricConfig.description || "",
      createdAt: timestamp,
      createdBy: metricConfig.createdBy || "system",
      status: "active",
      metricType: metricConfig.metricType || "custom",
      dataType: metricConfig.dataType || "number",
      unit: metricConfig.unit || "",
      category: metricConfig.category || "general",
      calculationMethod: metricConfig.calculationMethod || "sum",
      aggregationPeriod:
        metricConfig.aggregationPeriod || this.config.aggregationInterval,
      validationRules: metricConfig.validationRules || [],
      alertThresholds: metricConfig.alertThresholds || {},
      tags: metricConfig.tags || [],
      metadata: metricConfig.metadata || {},
      formula: metricConfig.formula || null,
      dependencies: metricConfig.dependencies || [],
      isRealTime: metricConfig.isRealTime !== false,
      isPredictive: metricConfig.isPredictive || false,
      customProcessor: metricConfig.customProcessor || null,
      visualization: metricConfig.visualization || {
        chartType: "line",
        displayFormat: "number",
      },
    };

    // Validate metric definition
    const validation = await this.validateMetricDefinition(metricDefinition);
    if (!validation.valid) {
      throw new Error(
        `Invalid metric definition: ${validation.errors.join(", ")}`
      );
    }

    // Store metric definition
    this.metricDefinitions.set(metricId, metricDefinition);
    this.trackingMetrics.totalMetrics++;

    if (metricDefinition.metricType === "custom") {
      this.trackingMetrics.customMetrics++;
    }

    // Initialize metric tracking
    await this.initializeMetricTracking(metricDefinition);

    // Set up real-time collection if enabled
    if (metricDefinition.isRealTime && this.config.enableRealTimeCollection) {
      await this.initializeRealTimeCollection(metricDefinition);
    }

    // Create audit entry
    await this.createMetricAuditEntry({
      action: "metric_defined",
      metricId: metricId,
      timestamp: timestamp,
      details: {
        name: metricDefinition.name,
        type: metricDefinition.metricType,
        category: metricDefinition.category,
        createdBy: metricDefinition.createdBy,
      },
    });

    return metricDefinition;
  }

  async registerCustomMetric(metricName, calculationFunction, options = {}) {
    // TODO: Register custom metric with calculation function
    // TODO: Validate calculation function
    // TODO: Set up metric configuration
    // TODO: Initialize custom processing
    // TODO: Configure metric validation
    // TODO: Set up metric monitoring
    // TODO: Create metric documentation
    // TODO: Initialize metric analytics
    // TODO: Configure metric alerts
    // TODO: Generate metric registration audit

    const metricId = this.generateMetricId();
    const timestamp = Date.now();

    const customMetric = {
      id: metricId,
      name: metricName,
      calculationFunction: calculationFunction,
      createdAt: timestamp,
      status: "active",
      options: {
        dataType: options.dataType || "number",
        unit: options.unit || "",
        category: options.category || "custom",
        aggregationMethod: options.aggregationMethod || "sum",
        validationRules: options.validationRules || [],
        realTimeEnabled: options.realTimeEnabled !== false,
        historicalEnabled: options.historicalEnabled !== false,
        ...options,
      },
      statistics: {
        calculationCount: 0,
        lastCalculated: null,
        averageCalculationTime: 0,
        errorCount: 0,
        lastError: null,
      },
    };

    // Validate custom metric
    const validation = await this.validateCustomMetric(customMetric);
    if (!validation.valid) {
      throw new Error(`Invalid custom metric: ${validation.errors.join(", ")}`);
    }

    // Store custom metric
    this.customMetrics.set(metricId, customMetric);

    // Initialize tracking
    await this.initializeCustomMetricTracking(customMetric);

    return customMetric;
  }

  async updateMetricDefinition(metricId, updates) {
    // TODO: Update metric definition
    // TODO: Validate metric updates
    // TODO: Apply configuration changes
    // TODO: Update metric tracking
    // TODO: Refresh metric calculations
    // TODO: Update metric validation
    // TODO: Create update audit trail
    // TODO: Notify metric subscribers
    // TODO: Handle update conflicts
    // TODO: Generate update report

    const metric = this.metricDefinitions.get(metricId);
    if (!metric) {
      throw new Error(`Metric definition not found: ${metricId}`);
    }

    const timestamp = Date.now();
    const previousConfig = { ...metric };

    // Apply updates
    Object.keys(updates).forEach((key) => {
      if (key !== "id" && key !== "createdAt") {
        metric[key] = updates[key];
      }
    });

    metric.updatedAt = timestamp;
    metric.updatedBy = updates.updatedBy || "system";

    // Validate updated configuration
    const validation = await this.validateMetricDefinition(metric);
    if (!validation.valid) {
      // Rollback changes
      Object.keys(previousConfig).forEach((key) => {
        metric[key] = previousConfig[key];
      });
      throw new Error(`Invalid metric update: ${validation.errors.join(", ")}`);
    }

    // Update tracking if needed
    if (
      updates.isRealTime !== undefined ||
      updates.aggregationPeriod !== undefined
    ) {
      await this.updateMetricTracking(metric);
    }

    // Create audit entry
    await this.createMetricAuditEntry({
      action: "metric_updated",
      metricId: metricId,
      timestamp: timestamp,
      details: {
        updates: updates,
        previousConfig: previousConfig,
        updatedBy: metric.updatedBy,
      },
    });

    return metric;
  }

  /**
   * Real-Time Metrics Collection
   */
  async trackMetric(metricId, value, context = {}) {
    // TODO: Track metric value in real-time
    // TODO: Validate metric value
    // TODO: Apply metric transformations
    // TODO: Update metric aggregations
    // TODO: Check metric thresholds
    // TODO: Store metric data
    // TODO: Update metric statistics
    // TODO: Trigger metric alerts
    // TODO: Update real-time displays
    // TODO: Generate metric events

    const metricDefinition = this.metricDefinitions.get(metricId);
    if (!metricDefinition) {
      throw new Error(`Metric definition not found: ${metricId}`);
    }

    const timestamp = context.timestamp || Date.now();

    try {
      // Validate metric value
      const validation = await this.validateMetricValue(
        metricDefinition,
        value
      );
      if (!validation.valid) {
        throw new Error(
          `Invalid metric value: ${validation.errors.join(", ")}`
        );
      }

      // Apply transformations if needed
      const transformedValue = await this.transformMetricValue(
        metricDefinition,
        value,
        context
      );

      // Create metric data point
      const dataPoint = {
        metricId: metricId,
        value: transformedValue,
        originalValue: value,
        timestamp: timestamp,
        context: context,
        sessionId: context.sessionId || null,
        userId: context.userId || null,
        experimentId: context.experimentId || null,
        variantId: context.variantId || null,
        metadata: context.metadata || {},
      };

      // Store in real-time metrics
      if (!this.realTimeMetrics.has(metricId)) {
        this.realTimeMetrics.set(metricId, []);
      }
      this.realTimeMetrics.get(metricId).push(dataPoint);

      // Update aggregated metrics
      await this.updateAggregatedMetrics(metricId, dataPoint);

      // Check alert thresholds
      if (
        metricDefinition.alertThresholds &&
        Object.keys(metricDefinition.alertThresholds).length > 0
      ) {
        await this.checkAlertThresholds(
          metricDefinition,
          transformedValue,
          context
        );
      }

      // Update metric statistics
      await this.updateMetricStatistics(metricId, dataPoint);

      // Trigger events
      await this.triggerMetricEvents(metricId, dataPoint);

      return {
        metricId,
        value: transformedValue,
        timestamp,
        tracked: true,
      };
    } catch (error) {
      this.trackingMetrics.errorCount++;

      // Create error audit entry
      await this.createMetricAuditEntry({
        action: "metric_tracking_error",
        metricId: metricId,
        timestamp: timestamp,
        details: {
          value: value,
          context: context,
          error: error.message,
        },
      });

      throw error;
    }
  }

  async trackCustomMetric(metricName, data, context = {}) {
    // TODO: Track custom metric with calculation
    // TODO: Find custom metric definition
    // TODO: Execute calculation function
    // TODO: Validate calculation result
    // TODO: Store custom metric data
    // TODO: Update custom metric statistics
    // TODO: Handle calculation errors
    // TODO: Generate custom metric events
    // TODO: Update custom analytics
    // TODO: Create custom audit entries

    const customMetric = [...this.customMetrics.values()].find(
      (metric) => metric.name === metricName
    );

    if (!customMetric) {
      throw new Error(`Custom metric not found: ${metricName}`);
    }

    const timestamp = context.timestamp || Date.now();
    const calculationStart = Date.now();

    try {
      // Execute calculation function
      const calculatedValue = await customMetric.calculationFunction(
        data,
        context
      );

      const calculationTime = Date.now() - calculationStart;

      // Validate calculated value
      const validation = await this.validateCustomMetricValue(
        customMetric,
        calculatedValue
      );
      if (!validation.valid) {
        throw new Error(
          `Invalid custom metric value: ${validation.errors.join(", ")}`
        );
      }

      // Create custom metric data point
      const dataPoint = {
        metricId: customMetric.id,
        metricName: metricName,
        value: calculatedValue,
        inputData: data,
        timestamp: timestamp,
        context: context,
        calculationTime: calculationTime,
      };

      // Store custom metric data
      if (!this.realTimeMetrics.has(customMetric.id)) {
        this.realTimeMetrics.set(customMetric.id, []);
      }
      this.realTimeMetrics.get(customMetric.id).push(dataPoint);

      // Update custom metric statistics
      customMetric.statistics.calculationCount++;
      customMetric.statistics.lastCalculated = timestamp;
      customMetric.statistics.averageCalculationTime =
        (customMetric.statistics.averageCalculationTime + calculationTime) / 2;

      return {
        metricName,
        metricId: customMetric.id,
        value: calculatedValue,
        calculationTime,
        timestamp,
        tracked: true,
      };
    } catch (error) {
      // Update error statistics
      customMetric.statistics.errorCount++;
      customMetric.statistics.lastError = {
        message: error.message,
        timestamp: timestamp,
        data: data,
        context: context,
      };

      throw error;
    }
  }

  async batchTrackMetrics(metricsData) {
    // TODO: Track multiple metrics in batch
    // TODO: Validate batch data structure
    // TODO: Process metrics efficiently
    // TODO: Handle batch processing errors
    // TODO: Update batch statistics
    // TODO: Generate batch reports
    // TODO: Optimize batch performance
    // TODO: Handle partial failures
    // TODO: Create batch audit entries
    // TODO: Return batch results

    if (!Array.isArray(metricsData)) {
      throw new Error("Metrics data must be an array");
    }

    const batchId = this.generateBatchId();
    const timestamp = Date.now();
    const results = [];
    const errors = [];

    for (const [index, metricData] of metricsData.entries()) {
      try {
        const result = await this.trackMetric(
          metricData.metricId,
          metricData.value,
          { ...metricData.context, batchId, batchIndex: index }
        );
        results.push(result);
      } catch (error) {
        errors.push({
          index,
          metricId: metricData.metricId,
          error: error.message,
        });
      }
    }

    // Create batch audit entry
    await this.createMetricAuditEntry({
      action: "batch_metrics_tracked",
      batchId: batchId,
      timestamp: timestamp,
      details: {
        totalMetrics: metricsData.length,
        successCount: results.length,
        errorCount: errors.length,
        errors: errors,
      },
    });

    return {
      batchId,
      totalMetrics: metricsData.length,
      successCount: results.length,
      errorCount: errors.length,
      results,
      errors,
    };
  }

  /**
   * Metrics Aggregation and Analysis
   */
  async aggregateMetrics(metricId, timeRange, aggregationType = "sum") {
    // TODO: Aggregate metrics over time range
    // TODO: Apply aggregation algorithm
    // TODO: Calculate statistical measures
    // TODO: Generate aggregation insights
    // TODO: Update aggregated data
    // TODO: Store aggregation results
    // TODO: Handle aggregation errors
    // TODO: Optimize aggregation performance
    // TODO: Generate aggregation reports
    // TODO: Update aggregation cache

    const metricDefinition = this.metricDefinitions.get(metricId);
    if (!metricDefinition) {
      throw new Error(`Metric definition not found: ${metricId}`);
    }

    const { startTime, endTime } = this.parseTimeRange(timeRange);
    const metricData = await this.getMetricDataInRange(
      metricId,
      startTime,
      endTime
    );

    if (metricData.length === 0) {
      return {
        metricId,
        timeRange,
        aggregationType,
        result: null,
        count: 0,
        message: "No data available for the specified time range",
      };
    }

    let aggregatedValue;
    const values = metricData.map((point) => point.value);

    switch (aggregationType) {
      case "sum":
        aggregatedValue = values.reduce((sum, value) => sum + value, 0);
        break;
      case "average":
        aggregatedValue =
          values.reduce((sum, value) => sum + value, 0) / values.length;
        break;
      case "min":
        aggregatedValue = Math.min(...values);
        break;
      case "max":
        aggregatedValue = Math.max(...values);
        break;
      case "count":
        aggregatedValue = values.length;
        break;
      case "median":
        const sorted = [...values].sort((a, b) => a - b);
        const mid = Math.floor(sorted.length / 2);
        aggregatedValue =
          sorted.length % 2 === 0
            ? (sorted[mid - 1] + sorted[mid]) / 2
            : sorted[mid];
        break;
      case "stddev":
        const mean =
          values.reduce((sum, value) => sum + value, 0) / values.length;
        const squaredDiffs = values.map((value) => Math.pow(value - mean, 2));
        const avgSquaredDiff =
          squaredDiffs.reduce((sum, diff) => sum + diff, 0) / values.length;
        aggregatedValue = Math.sqrt(avgSquaredDiff);
        break;
      default:
        throw new Error(`Unsupported aggregation type: ${aggregationType}`);
    }

    // Store aggregated result
    const aggregationResult = {
      metricId,
      timeRange,
      aggregationType,
      result: aggregatedValue,
      count: values.length,
      dataPoints: metricData.length,
      calculatedAt: Date.now(),
      statistics: {
        sum: values.reduce((sum, value) => sum + value, 0),
        average: values.reduce((sum, value) => sum + value, 0) / values.length,
        min: Math.min(...values),
        max: Math.max(...values),
        count: values.length,
      },
    };

    // Cache aggregation result
    const cacheKey = `${metricId}_${startTime}_${endTime}_${aggregationType}`;
    this.aggregatedMetrics.set(cacheKey, aggregationResult);

    return aggregationResult;
  }

  async calculateMetricTrends(metricId, timeWindow, granularity = "hour") {
    // TODO: Calculate metric trends over time
    // TODO: Apply trend analysis algorithms
    // TODO: Identify trend patterns
    // TODO: Calculate trend coefficients
    // TODO: Generate trend predictions
    // TODO: Detect trend anomalies
    // TODO: Create trend visualizations
    // TODO: Generate trend insights
    // TODO: Store trend analysis
    // TODO: Update trend cache

    const endTime = Date.now();
    const startTime = endTime - timeWindow;

    // Get metric data
    const metricData = await this.getMetricDataInRange(
      metricId,
      startTime,
      endTime
    );

    if (metricData.length < 2) {
      return {
        metricId,
        trend: "insufficient_data",
        message: "Not enough data points for trend analysis",
      };
    }

    // Group data by granularity
    const groupedData = this.groupDataByGranularity(metricData, granularity);

    // Calculate trend coefficients
    const trendAnalysis = this.calculateLinearTrend(groupedData);

    // Detect trend direction
    const trendDirection = this.determineTrendDirection(trendAnalysis.slope);

    // Calculate trend strength
    const trendStrength = this.calculateTrendStrength(trendAnalysis.rSquared);

    // Generate predictions
    const predictions = this.generateTrendPredictions(trendAnalysis, 5); // Next 5 periods

    return {
      metricId,
      timeWindow,
      granularity,
      trend: trendDirection,
      strength: trendStrength,
      slope: trendAnalysis.slope,
      intercept: trendAnalysis.intercept,
      rSquared: trendAnalysis.rSquared,
      predictions: predictions,
      dataPoints: metricData.length,
      groupedPeriods: groupedData.length,
      analysis: {
        isSignificant: trendAnalysis.rSquared > 0.5,
        confidence: trendAnalysis.rSquared,
        volatility: this.calculateVolatility(groupedData),
      },
    };
  }

  /**
   * Metrics Validation and Quality Assurance
   */
  async validateMetricValue(metricDefinition, value) {
    // TODO: Validate metric value against rules
    // TODO: Check data type compliance
    // TODO: Apply range validations
    // TODO: Check format requirements
    // TODO: Validate business rules
    // TODO: Apply custom validators
    // TODO: Generate validation reports
    // TODO: Handle validation errors
    // TODO: Update validation metrics
    // TODO: Create validation audit

    const validation = { valid: true, errors: [], warnings: [] };

    // Check data type
    if (metricDefinition.dataType === "number" && typeof value !== "number") {
      validation.errors.push(`Expected number, got ${typeof value}`);
    }

    // Apply validation rules
    for (const rule of metricDefinition.validationRules) {
      const ruleResult = await this.applyValidationRule(value, rule);
      if (!ruleResult.valid) {
        validation.errors.push(ruleResult.error);
      }
    }

    validation.valid = validation.errors.length === 0;
    return validation;
  }

  /**
   * Utility Methods
   */
  initializeStandardMetrics() {
    // TODO: Initialize standard A/B testing metrics
    // TODO: Set up conversion rate metrics
    // TODO: Configure engagement metrics
    // TODO: Initialize performance metrics
    // TODO: Set up user experience metrics
    // TODO: Configure business metrics
    // TODO: Initialize technical metrics
    // TODO: Set up quality metrics
    // TODO: Configure compliance metrics
    // TODO: Initialize reporting metrics

    const standardMetrics = [
      {
        name: "Conversion Rate",
        metricType: "standard",
        dataType: "number",
        unit: "percentage",
        category: "conversion",
        calculationMethod: "rate",
        description: "Percentage of users who completed the desired action",
      },
      {
        name: "Click Through Rate",
        metricType: "standard",
        dataType: "number",
        unit: "percentage",
        category: "engagement",
        calculationMethod: "rate",
        description: "Percentage of users who clicked on the element",
      },
      {
        name: "Session Duration",
        metricType: "standard",
        dataType: "number",
        unit: "seconds",
        category: "engagement",
        calculationMethod: "average",
        description: "Average time users spend in the session",
      },
      {
        name: "Bounce Rate",
        metricType: "standard",
        dataType: "number",
        unit: "percentage",
        category: "engagement",
        calculationMethod: "rate",
        description: "Percentage of users who leave after viewing one page",
      },
      {
        name: "Revenue Per User",
        metricType: "standard",
        dataType: "number",
        unit: "currency",
        category: "business",
        calculationMethod: "average",
        description: "Average revenue generated per user",
      },
    ];

    standardMetrics.forEach(async (metricConfig) => {
      try {
        await this.defineMetric(metricConfig);
      } catch (error) {
        console.error(
          `Error creating standard metric ${metricConfig.name}:`,
          error
        );
      }
    });
  }

  generateMetricId() {
    return `metric_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateBatchId() {
    return `batch_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  async validateMetricDefinition(definition) {
    const validation = { valid: true, errors: [], warnings: [] };

    if (!definition.name || definition.name.trim().length === 0) {
      validation.errors.push("Metric name is required");
    }

    if (
      !["number", "string", "boolean", "object"].includes(definition.dataType)
    ) {
      validation.errors.push("Invalid data type");
    }

    validation.valid = validation.errors.length === 0;
    return validation;
  }

  async createMetricAuditEntry(auditData) {
    const auditEntry = {
      id: this.generateAuditId(),
      ...auditData,
      integrity: this.generateIntegrityHash(auditData),
    };
    return auditEntry;
  }

  generateAuditId() {
    return `audit_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateIntegrityHash(data) {
    return btoa(JSON.stringify(data)).substr(0, 16);
  }

  /**
   * Variant Metrics Generation
   * Extracted from variant-controller.js for modularization
   */
  generateVariantMetrics(variantDefinition) {
    const baseMetrics = [
      "conversion_rate",
      "user_engagement",
      "error_rate",
      "page_load_time",
      "bounce_rate",
      "session_duration",
      "click_through_rate",
      "user_satisfaction",
    ];

    // Add custom metrics based on variant configuration
    const customMetrics = [];
    if (variantDefinition.behaviorChanges) {
      customMetrics.push("behavior_adoption_rate");
    }
    if (variantDefinition.uiModifications) {
      customMetrics.push("ui_interaction_rate");
    }
    if (variantDefinition.contentVariations) {
      customMetrics.push("content_engagement_rate");
    }

    return [...baseMetrics, ...customMetrics];
  }

  /**
   * Evaluation Metrics Tracking
   * Extracted from variant-controller.js for modularization
   */
  updateEvaluationMetrics(flagId, evaluationResult, timestamp = Date.now()) {
    if (!this.evaluationMetrics) {
      this.evaluationMetrics = new Map();
    }

    let flagMetrics = this.evaluationMetrics.get(flagId);
    if (!flagMetrics) {
      flagMetrics = {
        totalEvaluations: 0,
        trueEvaluations: 0,
        falseEvaluations: 0,
        errorEvaluations: 0,
        lastEvaluated: null,
        averageEvaluationTime: 0,
        evaluationHistory: [],
      };
      this.evaluationMetrics.set(flagId, flagMetrics);
    }

    // Update counters
    flagMetrics.totalEvaluations++;
    flagMetrics.lastEvaluated = timestamp;

    if (evaluationResult.success) {
      if (evaluationResult.value === true) {
        flagMetrics.trueEvaluations++;
      } else {
        flagMetrics.falseEvaluations++;
      }
    } else {
      flagMetrics.errorEvaluations++;
    }

    // Update evaluation history
    flagMetrics.evaluationHistory.push({
      timestamp,
      result: evaluationResult,
      processingTime: evaluationResult.processingTime || 0,
    });

    // Keep only last 1000 evaluations
    if (flagMetrics.evaluationHistory.length > 1000) {
      flagMetrics.evaluationHistory =
        flagMetrics.evaluationHistory.slice(-1000);
    }

    // Update average evaluation time
    const recentEvaluations = flagMetrics.evaluationHistory.slice(-100);
    flagMetrics.averageEvaluationTime =
      recentEvaluations.reduce(
        (sum, evaluation) => sum + evaluation.processingTime,
        0
      ) / recentEvaluations.length;

    return flagMetrics;
  }

  /**
   * Rollout Metrics Tracking
   * Extracted from variant-controller.js for modularization
   */
  updateRolloutMetrics(rolloutId, stageData, metricsUpdate) {
    if (!this.rolloutMetrics) {
      this.rolloutMetrics = new Map();
    }

    let rolloutMetrics = this.rolloutMetrics.get(rolloutId);
    if (!rolloutMetrics) {
      rolloutMetrics = {
        stagesCompleted: 0,
        usersAffected: 0,
        successRate: 0,
        errorRate: 0,
        performanceImpact: 0,
        startTime: Date.now(),
        lastUpdated: null,
        stageHistory: [],
      };
      this.rolloutMetrics.set(rolloutId, rolloutMetrics);
    }

    // Update metrics with new data
    if (metricsUpdate.stagesCompleted !== undefined) {
      rolloutMetrics.stagesCompleted = metricsUpdate.stagesCompleted;
    }
    if (metricsUpdate.usersAffected !== undefined) {
      rolloutMetrics.usersAffected += metricsUpdate.usersAffected;
    }
    if (metricsUpdate.successRate !== undefined) {
      rolloutMetrics.successRate = metricsUpdate.successRate;
    }
    if (metricsUpdate.errorRate !== undefined) {
      rolloutMetrics.errorRate = metricsUpdate.errorRate;
    }
    if (metricsUpdate.performanceImpact !== undefined) {
      rolloutMetrics.performanceImpact = metricsUpdate.performanceImpact;
    }

    rolloutMetrics.lastUpdated = Date.now();

    // Add stage to history
    rolloutMetrics.stageHistory.push({
      timestamp: Date.now(),
      stage: stageData,
      metrics: { ...metricsUpdate },
    });

    return rolloutMetrics;
  }

  /**
   * Variant Performance Dashboard Generation
   * Extracted from variant-controller.js for modularization
   */
  async generateVariantDashboard(variantDefinition) {
    const metrics = this.generateVariantMetrics(variantDefinition);

    const dashboard = {
      name: `${variantDefinition.name}_dashboard`,
      variantId: variantDefinition.id,
      createdAt: Date.now(),
      widgets: [
        {
          type: "conversion_funnel",
          metrics: ["conversion_rate", "user_engagement"],
          chartType: "funnel",
        },
        {
          type: "error_monitoring",
          metrics: ["error_rate"],
          chartType: "line",
        },
        {
          type: "performance_metrics",
          metrics: ["page_load_time", "session_duration"],
          chartType: "area",
        },
        {
          type: "user_behavior",
          metrics: ["bounce_rate", "click_through_rate"],
          chartType: "bar",
        },
        {
          type: "satisfaction_score",
          metrics: ["user_satisfaction"],
          chartType: "gauge",
        },
      ],
      refreshInterval: 60000, // 1 minute
      alertThresholds: {
        conversion_rate: { min: 0.02, max: 0.2 },
        error_rate: { max: 0.05 },
        page_load_time: { max: 3000 },
      },
    };

    return dashboard;
  }

  /**
   * Alert Configuration Generation
   * Extracted from variant-controller.js for modularization
   */
  async generateVariantAlerts(variantDefinition) {
    const alerts = {
      variantId: variantDefinition.id,
      createdAt: Date.now(),
      channels: {
        email: {
          enabled: true,
          recipients: variantDefinition.alertRecipients || [],
          severity: ["critical", "warning"],
        },
        slack: {
          enabled: true,
          channel: variantDefinition.slackChannel || "#experiments",
          severity: ["critical", "warning", "info"],
        },
        pagerduty: {
          enabled: variantDefinition.enablePagerDuty || false,
          severity: ["critical"],
        },
      },
      rules: [
        {
          name: "High Error Rate",
          metric: "error_rate",
          condition: "greater_than",
          threshold: 0.05,
          severity: "critical",
        },
        {
          name: "Low Conversion Rate",
          metric: "conversion_rate",
          condition: "less_than",
          threshold: 0.01,
          severity: "warning",
        },
        {
          name: "Performance Degradation",
          metric: "page_load_time",
          condition: "greater_than",
          threshold: 5000,
          severity: "warning",
        },
      ],
    };

    return alerts;
  }

  /**
   * Metrics Summary for Variant Performance
   */
  getVariantMetricsSummary(variantId) {
    const evaluationMetrics = this.evaluationMetrics.get(variantId) || {};
    const rolloutMetrics = this.rolloutMetrics.get(variantId) || {};

    return {
      variantId,
      evaluation: {
        totalEvaluations: evaluationMetrics.totalEvaluations || 0,
        successRate:
          evaluationMetrics.totalEvaluations > 0
            ? (
                ((evaluationMetrics.trueEvaluations || 0) /
                  evaluationMetrics.totalEvaluations) *
                100
              ).toFixed(2)
            : 0,
        errorRate:
          evaluationMetrics.totalEvaluations > 0
            ? (
                ((evaluationMetrics.errorEvaluations || 0) /
                  evaluationMetrics.totalEvaluations) *
                100
              ).toFixed(2)
            : 0,
        averageEvaluationTime: evaluationMetrics.averageEvaluationTime || 0,
      },
      rollout: {
        stagesCompleted: rolloutMetrics.stagesCompleted || 0,
        usersAffected: rolloutMetrics.usersAffected || 0,
        currentSuccessRate: rolloutMetrics.successRate || 0,
        currentErrorRate: rolloutMetrics.errorRate || 0,
        performanceImpact: rolloutMetrics.performanceImpact || 0,
      },
    };
  }
}
