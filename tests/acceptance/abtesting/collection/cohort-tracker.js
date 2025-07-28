/**
 * @file cohort-tracker.js
 * @brief Cohort Analysis and User Journey Tracking Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides comprehensive cohort analysis with user journey tracking,
 * retention analysis, and longitudinal user behavior analysis for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

/**
 * CohortTracker Class
 * Manages cohort analysis with user journey tracking and retention analysis
 */
export class CohortTracker {
  constructor(config = {}) {
    // Initialize cohort tracking system
    this.trackingSystem = {
      active: true,
      startTime: Date.now(),
      trackingEnabled: true,
      autoFlush: true,
      flushInterval: 60000, // 1 minute
    };

    // Set up cohort analysis framework
    this.analysisFramework = {
      analysisTypes: ["retention", "journey", "segmentation", "behavioral"],
      scheduledAnalysis: new Map(),
      analysisQueue: [],
      analysisHistory: [],
    };

    // Configure user journey tracking
    this.journeyTracking = {
      enabled: true,
      trackingDepth: "detailed",
      sessionTimeout: 1800000, // 30 minutes
      eventBuffer: new Map(),
      pathAnalysis: true,
      funnelTracking: true,
    };

    // Initialize retention analysis
    this.retentionAnalysis = {
      enabled: true,
      defaultPeriods: [1, 7, 14, 30, 60, 90],
      calculationMethod: "classic",
      segmentedRetention: true,
      retentionCache: new Map(),
    };

    // Set up longitudinal tracking
    this.longitudinalTracking = {
      enabled: true,
      trackingPeriod: 365, // days
      dataPoints: new Map(),
      trendAnalysis: true,
      seasonalAdjustment: false,
    };

    // Configure cohort segmentation
    this.segmentation = {
      enabled: true,
      segmentTypes: ["behavioral", "demographic", "temporal", "custom"],
      dynamicSegments: true,
      segmentCache: new Map(),
      segmentRules: new Map(),
    };

    // Initialize cohort analytics
    this.analytics = {
      enabled: true,
      realTimeAnalytics: true,
      batchAnalytics: true,
      customMetrics: new Map(),
      alertThresholds: new Map(),
    };

    // Set up cohort monitoring
    this.monitoring = {
      enabled: true,
      healthChecks: true,
      performanceMonitoring: true,
      alerting: true,
      monitoringInterval: 300000, // 5 minutes
    };

    // Configure cohort reporting
    this.reporting = {
      enabled: true,
      reportTypes: ["summary", "detailed", "trend", "comparison"],
      scheduledReports: new Map(),
      reportHistory: [],
      exportFormats: ["json", "csv", "pdf"],
    };

    // Initialize cohort optimization
    this.optimization = {
      enabled: true,
      autoOptimization: true,
      optimizationRules: new Map(),
      performanceMetrics: new Map(),
      optimizationHistory: [],
    };

    this.config = {
      defaultCohortPeriod: "daily",
      retentionPeriods: [1, 7, 14, 30, 90], // days
      maxCohortSize: 10000,
      enableRealTimeCohorts: true,
      enableRetentionTracking: true,
      enableJourneyTracking: true,
      cohortRefreshInterval: 86400000, // 24 hours
      enableCohortOptimization: true,
      enableCohortCache: true,
      cacheTimeout: 3600000, // 1 hour
      ...config,
    };

    this.cohorts = new Map();
    this.userJourneys = new Map();
    this.retentionData = new Map();
    this.cohortDefinitions = new Map();
    this.cohortMetrics = {
      totalCohorts: 0,
      activeCohorts: 0,
      trackedUsers: 0,
      retentionRate: 0,
      averageJourneyLength: 0,
    };

    this.eventHandlers = new Map();
    this.analyzers = new Map();
    this.trackers = new Map();

    this.initializeCohortTypes();
  }

  /**
   * Cohort Definition and Creation
   */
  async createCohort(cohortConfig) {
    // Create new user cohort with comprehensive validation
    const cohortId = this.generateCohortId();
    const timestamp = Date.now();

    // Validate cohort configuration thoroughly
    const validation = await this.validateCohortConfig(cohortConfig);
    if (!validation.valid) {
      throw new Error(
        `Invalid cohort configuration: ${validation.errors.join(", ")}`
      );
    }

    // Define cohort criteria with proper structure
    const cohortCriteria = {
      type: cohortConfig.cohortType || "acquisition",
      filters: cohortConfig.filters || {},
      rules: cohortConfig.rules || [],
      inclusionCriteria: cohortConfig.inclusionCriteria || {},
      exclusionCriteria: cohortConfig.exclusionCriteria || {},
      timeWindow: cohortConfig.timeWindow || null,
    };

    const cohort = {
      id: cohortId,
      name: cohortConfig.name || `cohort_${cohortId}`,
      description: cohortConfig.description || "",
      createdAt: timestamp,
      experimentId: cohortConfig.experimentId,
      cohortType: cohortConfig.cohortType || "acquisition",
      period: cohortConfig.period || this.config.defaultCohortPeriod,
      criteria: cohortCriteria,
      startDate: cohortConfig.startDate || timestamp,
      endDate: cohortConfig.endDate || null,
      users: new Set(),
      status: "active",
      analytics: {
        userCount: 0,
        retentionRates: {},
        journeyAnalytics: {},
        segmentAnalytics: {},
        conversionRates: {},
        engagementMetrics: {},
      },
      metadata: {
        createdBy: cohortConfig.createdBy || "system",
        tags: cohortConfig.tags || [],
        version: 1,
        lastModified: timestamp,
      },
    };

    // Set up cohort tracking infrastructure
    await this.initializeCohortTracking(cohort);

    // Initialize cohort analytics pipeline
    await this.initializeCohortAnalytics(cohort);

    // Configure cohort segmentation rules
    await this.configureCohortSegmentation(
      cohort,
      cohortConfig.segmentationRules
    );

    // Set up cohort monitoring and alerts
    await this.setupCohortMonitoring(cohort);

    // Generate cohort documentation
    const documentation = await this.generateCohortDocumentation(cohort);
    cohort.documentation = documentation;

    // Create cohort audit trail
    await this.createCohortAuditEntry({
      action: "cohort_created",
      cohortId: cohortId,
      experimentId: cohort.experimentId,
      timestamp: timestamp,
      details: {
        config: cohortConfig,
        criteria: cohortCriteria,
        initialMetrics: cohort.analytics,
      },
    });

    // Store cohort
    this.cohorts.set(cohortId, cohort);

    // Update cohort metrics
    this.cohortMetrics.totalCohorts++;
    this.cohortMetrics.activeCohorts++;

    // Initialize optimization for this cohort
    await this.initializeCohortOptimization(cohort);

    return {
      cohortId: cohortId,
      cohort: cohort,
      documentation: documentation,
      tracking: true,
    };
  }

  async addUserToCohort(cohortId, userId, userContext = {}) {
    // Add user to cohort with comprehensive validation
    const cohort = this.cohorts.get(cohortId);
    if (!cohort) {
      throw new Error(`Cohort not found: ${cohortId}`);
    }

    // Check if user already in cohort
    if (cohort.users.has(userId)) {
      return { added: false, reason: "User already in cohort" };
    }

    // Validate user eligibility against cohort criteria
    const eligibility = await this.validateUserEligibility(
      cohort,
      userId,
      userContext
    );
    if (!eligibility.eligible) {
      return { added: false, reason: eligibility.reason };
    }

    const timestamp = Date.now();

    // Update cohort membership
    cohort.users.add(userId);
    cohort.analytics.userCount++;
    cohort.metadata.lastModified = timestamp;

    // Initialize user journey tracking for this user
    await this.initializeUserJourneyTracking(cohortId, userId, userContext);

    // Set up retention tracking for this user
    await this.setupRetentionTracking(cohortId, userId, userContext);

    // Update cohort analytics with new user data
    await this.updateCohortAnalytics(cohort, userId, userContext);

    // Generate user audit trail entry
    await this.createUserAuditEntry({
      action: "user_added_to_cohort",
      cohortId: cohortId,
      userId: userId,
      timestamp: timestamp,
      context: userContext,
      experimentId: cohort.experimentId,
    });

    // Update global cohort metrics
    this.cohortMetrics.trackedUsers++;

    // Trigger cohort events for user addition
    await this.triggerCohortEvents({
      event: "user_added",
      cohortId: cohortId,
      userId: userId,
      timestamp: timestamp,
      data: { userContext, cohortSize: cohort.users.size },
    });

    // Optimize cohort performance if needed
    if (cohort.users.size % 100 === 0) {
      // Optimize every 100 users
      await this.optimizeCohortPerformance(cohort);
    }

    return {
      added: true,
      cohortId: cohortId,
      userId: userId,
      cohortSize: cohort.users.size,
      timestamp: timestamp,
      journeyInitialized: true,
      retentionTracking: true,
    };
  }

  /**
   * User Journey Tracking
   */
  async trackUserJourney(userId, journeyEvent) {
    // Track user journey event with comprehensive validation
    if (!journeyEvent || !journeyEvent.eventType) {
      throw new Error("Invalid journey event: eventType is required");
    }

    // Validate journey event data structure
    const validation = await this.validateJourneyEventData(journeyEvent);
    if (!validation.valid) {
      throw new Error(
        `Invalid journey event data: ${validation.errors.join(", ")}`
      );
    }

    const journeyId = this.getUserJourneyId(userId);
    let journey = this.userJourneys.get(journeyId);

    if (!journey) {
      journey = await this.initializeUserJourney(userId);
    }

    const eventId = this.generateJourneyEventId();
    const timestamp = Date.now();

    // Update user journey path with new event
    const journeyEventData = {
      id: eventId,
      timestamp: timestamp,
      userId: userId,
      eventType: journeyEvent.eventType,
      eventName: journeyEvent.eventName,
      properties: journeyEvent.properties || {},
      context: journeyEvent.context || {},
      cohortId: journeyEvent.cohortId,
      sessionId: journeyEvent.sessionId,
      source: journeyEvent.source || "unknown",
      sequence: journey.eventCount + 1,
      timeFromPrevious:
        journey.events.length > 0
          ? timestamp - journey.events[journey.events.length - 1].timestamp
          : 0,
    };

    // Analyze journey patterns for this event
    const patterns = await this.analyzeJourneyPatterns(
      journey,
      journeyEventData
    );

    // Update journey analytics with new insights
    await this.updateJourneyAnalytics(journey, journeyEventData, patterns);

    // Check journey milestones and achievements
    const milestones = await this.checkJourneyMilestones(
      journey,
      journeyEventData
    );

    // Generate journey insights based on patterns
    const insights = await this.generateJourneyInsights(
      journey,
      journeyEventData
    );

    // Add event to journey
    journey.events.push(journeyEventData);
    journey.lastActivity = timestamp;
    journey.eventCount++;
    journey.milestones = milestones;
    journey.insights = insights;

    // Update cohort analytics if cohort specified
    if (journeyEvent.cohortId) {
      await this.updateCohortJourneyAnalytics(
        journeyEvent.cohortId,
        journeyEventData
      );
    }

    // Cache journey data for performance
    this.userJourneys.set(journeyId, journey);

    // Add to event buffer for batch processing
    if (this.journeyTracking.eventBuffer.has(userId)) {
      this.journeyTracking.eventBuffer.get(userId).push(journeyEventData);
    } else {
      this.journeyTracking.eventBuffer.set(userId, [journeyEventData]);
    }

    // Generate journey audit trail
    await this.createJourneyAuditEntry({
      action: "journey_event_tracked",
      journeyId: journeyId,
      eventId: eventId,
      userId: userId,
      eventType: journeyEvent.eventType,
      timestamp: timestamp,
      cohortId: journeyEvent.cohortId,
    });

    return {
      eventId: eventId,
      journeyId: journeyId,
      tracked: true,
      timestamp: timestamp,
      sequence: journeyEventData.sequence,
      milestones: milestones,
      insights: insights,
    };
  }

  async initializeUserJourney(userId) {
    // Initialize user journey tracking with comprehensive setup
    const journeyId = this.getUserJourneyId(userId);
    const timestamp = Date.now();

    const journey = {
      id: journeyId,
      userId: userId,
      startedAt: timestamp,
      lastActivity: timestamp,
      events: [],
      eventCount: 0,
      milestones: [],
      patterns: {},
      insights: {
        patterns: [],
        recommendations: [],
        risks: [],
        opportunities: [],
      },
      analytics: {
        sessionCount: 0,
        totalDuration: 0,
        conversionEvents: 0,
        dropoffPoints: [],
        engagementScore: 0,
        patterns: {},
      },
      metadata: {
        userAgent: null,
        platform: null,
        source: "unknown",
        initialContext: {},
      },
    };

    this.userJourneys.set(journeyId, journey);
    return journey;
  }

  async analyzeJourneyPatterns(journey, newEvent = null) {
    // Analyze user journey patterns with comprehensive pattern recognition
    const patterns = {
      commonPaths: await this.identifyCommonPaths(journey),
      dropoffPoints: await this.identifyDropoffPoints(journey),
      conversionPatterns: await this.identifyConversionPatterns(journey),
      sessionPatterns: await this.analyzeSessionPatterns(journey),
      behavioralPatterns: await this.identifyBehavioralPatterns(journey),
      temporalPatterns: await this.analyzeTemporalPatterns(journey),
      engagementPatterns: await this.analyzeEngagementPatterns(journey),
      funnelPatterns: await this.analyzeFunnelPatterns(journey),
    };

    // Identify common paths through the user journey
    patterns.commonPaths = await this.identifyCommonPaths(journey);

    // Detect dropoff points where users commonly exit
    patterns.dropoffPoints = await this.identifyDropoffPoints(journey);

    // Calculate journey metrics for pattern analysis
    const journeyMetrics = await this.calculateJourneyMetrics(journey);

    // Identify conversion patterns and success paths
    patterns.conversionPatterns = await this.identifyConversionPatterns(
      journey
    );

    // Analyze session patterns and user engagement
    patterns.sessionPatterns = await this.analyzeSessionPatterns(journey);

    // Generate pattern insights with statistical analysis
    const patternInsights = await this.generatePatternInsights(
      patterns,
      journeyMetrics
    );

    // Update pattern analytics with new findings
    await this.updatePatternAnalytics(journey, patterns, patternInsights);

    // Cache pattern results for performance optimization
    if (this.analytics.enabled) {
      await this.cachePatternResults(journey.id, patterns, patternInsights);
    }

    // Generate pattern reports for cohort analysis
    const patternReports = await this.generatePatternReports(journey, patterns);

    // Store enhanced patterns with insights
    const enhancedPatterns = {
      ...patterns,
      insights: patternInsights,
      metrics: journeyMetrics,
      reports: patternReports,
      analyzedAt: Date.now(),
      eventCount: journey.events.length,
    };

    journey.patterns = enhancedPatterns;
    return enhancedPatterns;
  }

  /**
   * Retention Analysis
   */
  async analyzeRetention(cohortId, analysisConfig = {}) {
    // Analyze cohort retention with comprehensive metrics
    const cohort = this.cohorts.get(cohortId);
    if (!cohort) {
      throw new Error(`Cohort not found: ${cohortId}`);
    }

    const retentionPeriods =
      analysisConfig.periods || this.config.retentionPeriods;
    const timestamp = Date.now();

    // Calculate retention rates for each period
    const retentionRates = {};
    const retentionCurve = [];

    for (const period of retentionPeriods) {
      const retentionRate = await this.calculateRetentionRate(cohort, period);
      retentionRates[`day_${period}`] = retentionRate;
      retentionCurve.push({
        period: period,
        rate: retentionRate,
        users: Math.floor(cohort.users.size * retentionRate),
        percentage: Math.round(retentionRate * 10000) / 100, // 2 decimal places
      });
    }

    // Generate retention curves with trend analysis
    const retentionCurves = await this.generateRetentionCurves(
      cohort,
      retentionRates
    );

    // Identify retention patterns across different user segments
    const retentionPatterns = await this.identifyRetentionPatterns(
      cohort,
      retentionRates
    );

    // Compare retention across variants if A/B testing
    const variantComparison = await this.compareRetentionAcrossVariants(
      cohort,
      retentionRates
    );

    // Generate retention insights with statistical significance
    const retentionInsights = await this.generateRetentionInsights(
      cohort,
      retentionRates,
      retentionPatterns
    );

    // Perform segmented retention analysis
    const segmentedRetention = await this.analyzeSegmentedRetention(
      cohort,
      retentionPeriods
    );

    const retentionAnalysis = {
      cohortId: cohortId,
      analyzedAt: timestamp,
      totalUsers: cohort.users.size,
      retentionRates: retentionRates,
      retentionCurve: retentionCurve,
      retentionCurves: retentionCurves,
      patterns: retentionPatterns,
      insights: retentionInsights,
      variantComparison: variantComparison,
      segments: segmentedRetention,
      methodology: analysisConfig.methodology || "classic",
      confidence: analysisConfig.confidence || 0.95,
    };

    // Update retention analytics with advanced metrics
    await this.updateRetentionAnalytics(cohort, retentionAnalysis);

    // Cache retention results for performance
    this.retentionData.set(cohortId, retentionAnalysis);
    if (this.retentionAnalysis.retentionCache) {
      this.retentionAnalysis.retentionCache.set(cohortId, {
        analysis: retentionAnalysis,
        cachedAt: timestamp,
        expiresAt: timestamp + this.config.cacheTimeout,
      });
    }

    // Generate retention reports for stakeholders
    const retentionReports = await this.generateRetentionReports(
      retentionAnalysis
    );

    // Update cohort analytics
    cohort.analytics.retentionRates = retentionAnalysis.retentionRates;
    cohort.analytics.retentionInsights = retentionInsights;

    // Optimize retention tracking based on findings
    if (this.optimization.enabled) {
      await this.optimizeRetentionTracking(cohort, retentionAnalysis);
    }

    return {
      ...retentionAnalysis,
      reports: retentionReports,
      optimizations: this.optimization.enabled,
    };
  }

  async calculateRetentionRate(cohort, days) {
    // Calculate retention rate for specific period with comprehensive analysis
    const cutoffDate = Date.now() - days * 24 * 60 * 60 * 1000;
    let retainedUsers = 0;
    let totalEligibleUsers = 0;

    for (const userId of cohort.users) {
      // Only count users who have been in the cohort long enough
      const userJourney = this.getUserJourney(userId);
      if (userJourney && userJourney.startedAt <= cutoffDate) {
        totalEligibleUsers++;
        const isRetained = await this.isUserRetained(userId, cutoffDate);
        if (isRetained) {
          retainedUsers++;
        }
      }
    }

    // Return retention rate with proper denominator
    return totalEligibleUsers > 0 ? retainedUsers / totalEligibleUsers : 0;
  }

  async isUserRetained(userId, cutoffDate) {
    // Check if user is retained after cutoff date with comprehensive criteria
    const journeyId = this.getUserJourneyId(userId);
    const journey = this.userJourneys.get(journeyId);

    if (!journey) return false;

    // User is retained if they have activity after the cutoff date
    const hasRecentActivity = journey.lastActivity >= cutoffDate;

    // Additional retention criteria
    if (hasRecentActivity) {
      // Check for meaningful activity (not just single event)
      const eventsAfterCutoff = journey.events.filter(
        (e) => e.timestamp >= cutoffDate
      );

      // Consider user retained if they have:
      // 1. Recent activity AND
      // 2. Either multiple events OR at least one meaningful engagement event
      const meaningfulEvents = eventsAfterCutoff.filter(
        (e) =>
          e.eventType !== "page_view" ||
          e.eventType === "conversion" ||
          e.eventType === "purchase"
      );

      return eventsAfterCutoff.length > 1 || meaningfulEvents.length > 0;
    }

    return false;
  }

  /**
   * Cohort Segmentation
   */
  async segmentCohort(cohortId, segmentationConfig) {
    // Segment cohort based on criteria with advanced analytics
    const cohort = this.cohorts.get(cohortId);
    if (!cohort) {
      throw new Error(`Cohort not found: ${cohortId}`);
    }

    const segments = {};
    const segmentationRules = segmentationConfig.rules || [];
    const timestamp = Date.now();

    // Apply segmentation rules to create user segments
    for (const rule of segmentationRules) {
      const segmentUsers = await this.applySegmentationRule(cohort, rule);
      const segmentAnalytics = await this.calculateSegmentAnalytics(
        segmentUsers
      );

      segments[rule.name] = {
        users: segmentUsers,
        count: segmentUsers.length,
        percentage:
          cohort.users.size > 0 ? segmentUsers.length / cohort.users.size : 0,
        analytics: segmentAnalytics,
        rule: rule,
        createdAt: timestamp,
      };
    }

    // Create segment analytics with comparative analysis
    const segmentComparison = await this.createSegmentAnalytics(segments);

    // Generate segment insights with statistical analysis
    const segmentInsights = await this.generateSegmentInsights(
      cohort,
      segments
    );

    // Update cohort segmentation with new segments
    const segmentationResult = {
      cohortId: cohortId,
      segments: segments,
      comparison: segmentComparison,
      insights: segmentInsights,
      segmentedAt: timestamp,
      totalSegments: Object.keys(segments).length,
      methodology: segmentationConfig.methodology || "rule-based",
    };

    // Cache segmentation results for performance
    if (this.segmentation.segmentCache) {
      this.segmentation.segmentCache.set(cohortId, {
        result: segmentationResult,
        cachedAt: timestamp,
        expiresAt: timestamp + this.config.cacheTimeout,
      });
    }

    // Generate segmentation reports
    const segmentationReports = await this.generateSegmentationReports(
      segmentationResult
    );

    // Optimize segmentation performance
    if (this.optimization.enabled) {
      await this.optimizeSegmentationPerformance(cohort, segmentationResult);
    }

    // Validate segmentation accuracy
    const segmentationValidation = await this.validateSegmentationAccuracy(
      cohort,
      segmentationResult
    );

    // Update segmentation metrics
    await this.updateSegmentationMetrics(cohort, segmentationResult);

    // Update cohort analytics
    cohort.analytics.segmentAnalytics = segments;
    cohort.analytics.segmentInsights = segmentInsights;

    return {
      ...segmentationResult,
      reports: segmentationReports,
      validation: segmentationValidation,
      performance: this.optimization.enabled,
    };
  }

  /**
   * Helper Methods for Cohort Management
   */
  async initializeCohortTracking(cohort) {
    // Initialize comprehensive tracking infrastructure
    const trackingConfig = {
      cohortId: cohort.id,
      trackingEnabled: true,
      eventTracking: true,
      analyticsEnabled: true,
      realTimeUpdates: this.config.enableRealTimeCohorts,
    };

    // Set up event listeners for this cohort
    this.eventHandlers.set(cohort.id, new Map());

    // Initialize tracking analyzers
    this.analyzers.set(cohort.id, {
      retentionAnalyzer: new RetentionAnalyzer(cohort),
      journeyAnalyzer: new JourneyAnalyzer(cohort),
      segmentAnalyzer: new SegmentAnalyzer(cohort),
    });

    return trackingConfig;
  }

  async initializeCohortAnalytics(cohort) {
    // Set up comprehensive analytics pipeline
    const analyticsConfig = {
      cohortId: cohort.id,
      metricsEnabled: true,
      realTimeAnalytics: this.analytics.realTimeAnalytics,
      batchAnalytics: this.analytics.batchAnalytics,
      customMetrics: new Map(),
    };

    // Initialize metric collectors
    cohort.analytics.collectors = {
      retention: new RetentionCollector(),
      engagement: new EngagementCollector(),
      conversion: new ConversionCollector(),
      journey: new JourneyCollector(),
    };

    return analyticsConfig;
  }

  async configureCohortSegmentation(cohort, segmentationRules = []) {
    // Configure segmentation rules and processors
    const segmentationConfig = {
      cohortId: cohort.id,
      rules: segmentationRules,
      processors: new Map(),
      dynamicSegments: this.segmentation.dynamicSegments,
    };

    // Set up default segmentation rules
    if (segmentationRules.length === 0) {
      segmentationConfig.rules = [
        {
          name: "high_engagement",
          type: "behavioral",
          criteria: { engagement: "high" },
        },
        { name: "recent_users", type: "temporal", criteria: { recency: 7 } },
        {
          name: "power_users",
          type: "behavioral",
          criteria: { activity: "high" },
        },
      ];
    }

    this.segmentation.segmentRules.set(cohort.id, segmentationConfig);
    return segmentationConfig;
  }

  async setupCohortMonitoring(cohort) {
    // Set up monitoring and alerting
    const monitoringConfig = {
      cohortId: cohort.id,
      healthChecks: this.monitoring.healthChecks,
      performanceMonitoring: this.monitoring.performanceMonitoring,
      alerting: this.monitoring.alerting,
      thresholds: {
        retentionDrop: 0.1, // 10% drop triggers alert
        userGrowth: 0.05, // 5% growth threshold
        engagementDrop: 0.15, // 15% engagement drop
      },
    };

    return monitoringConfig;
  }

  async generateCohortDocumentation(cohort) {
    // Generate comprehensive cohort documentation
    return {
      cohortId: cohort.id,
      name: cohort.name,
      description: cohort.description,
      createdAt: cohort.createdAt,
      criteria: cohort.criteria,
      methodology: "Advanced cohort analysis with journey tracking",
      metrics: Object.keys(cohort.analytics),
      trackingCapabilities: [
        "User journey tracking",
        "Retention analysis",
        "Segmentation analysis",
        "Behavioral pattern recognition",
        "Real-time analytics",
      ],
      generatedAt: Date.now(),
    };
  }

  async initializeCohortOptimization(cohort) {
    // Initialize optimization routines
    const optimizationConfig = {
      cohortId: cohort.id,
      autoOptimization: this.optimization.autoOptimization,
      optimizationRules: new Map([
        ["performance", { enabled: true, threshold: 1000 }],
        ["memory", { enabled: true, threshold: 0.8 }],
        ["analytics", { enabled: true, interval: 3600000 }],
      ]),
    };

    this.optimization.optimizationRules.set(cohort.id, optimizationConfig);
    return optimizationConfig;
  }

  async validateUserEligibility(cohort, userId, userContext) {
    // Comprehensive user eligibility validation
    const validation = { eligible: true, reasons: [] };

    // Check cohort criteria
    if (cohort.criteria.inclusionCriteria) {
      const meetsInclusion = await this.checkInclusionCriteria(
        cohort.criteria.inclusionCriteria,
        userContext
      );
      if (!meetsInclusion.meets) {
        validation.eligible = false;
        validation.reasons.push(
          `Does not meet inclusion criteria: ${meetsInclusion.reason}`
        );
      }
    }

    // Check exclusion criteria
    if (cohort.criteria.exclusionCriteria) {
      const meetsExclusion = await this.checkExclusionCriteria(
        cohort.criteria.exclusionCriteria,
        userContext
      );
      if (meetsExclusion.meets) {
        validation.eligible = false;
        validation.reasons.push(
          `Meets exclusion criteria: ${meetsExclusion.reason}`
        );
      }
    }

    // Check cohort size limits
    if (cohort.users.size >= this.config.maxCohortSize) {
      validation.eligible = false;
      validation.reasons.push("Cohort has reached maximum size");
    }

    // Check time window if specified
    if (cohort.criteria.timeWindow) {
      const inTimeWindow = await this.checkTimeWindow(
        cohort.criteria.timeWindow
      );
      if (!inTimeWindow) {
        validation.eligible = false;
        validation.reasons.push("Outside cohort time window");
      }
    }

    return {
      eligible: validation.eligible,
      reason: validation.reasons.join("; ") || "User meets all criteria",
    };
  }

  async setupRetentionTracking(cohortId, userId, userContext) {
    // Set up retention tracking for specific user
    const retentionConfig = {
      cohortId: cohortId,
      userId: userId,
      startDate: Date.now(),
      trackingPeriods: this.config.retentionPeriods,
      checkpoints: this.config.retentionPeriods.map((period) => ({
        period: period,
        targetDate: Date.now() + period * 24 * 60 * 60 * 1000,
        checked: false,
        retained: false,
      })),
    };

    // Store retention tracking config
    const retentionKey = `${cohortId}_${userId}`;
    if (!this.retentionData.has("tracking")) {
      this.retentionData.set("tracking", new Map());
    }
    this.retentionData.get("tracking").set(retentionKey, retentionConfig);

    return retentionConfig;
  }

  async updateCohortAnalytics(cohort, userId, userContext) {
    // Update cohort analytics with new user data
    const timestamp = Date.now();

    // Update user demographics if available
    if (userContext.demographics) {
      if (!cohort.analytics.demographics) {
        cohort.analytics.demographics = {};
      }
      await this.updateDemographicAnalytics(
        cohort.analytics.demographics,
        userContext.demographics
      );
    }

    // Update engagement metrics
    if (userContext.engagementScore) {
      if (!cohort.analytics.engagement) {
        cohort.analytics.engagement = { total: 0, count: 0, average: 0 };
      }
      cohort.analytics.engagement.total += userContext.engagementScore;
      cohort.analytics.engagement.count++;
      cohort.analytics.engagement.average =
        cohort.analytics.engagement.total / cohort.analytics.engagement.count;
    }

    // Update acquisition metrics
    if (!cohort.analytics.acquisition) {
      cohort.analytics.acquisition = {
        sources: new Map(),
        channels: new Map(),
      };
    }

    if (userContext.source) {
      const sourceCount =
        cohort.analytics.acquisition.sources.get(userContext.source) || 0;
      cohort.analytics.acquisition.sources.set(
        userContext.source,
        sourceCount + 1
      );
    }

    cohort.analytics.lastUpdated = timestamp;
    return cohort.analytics;
  }

  async createUserAuditEntry(auditData) {
    // Create comprehensive user audit entry
    const auditEntry = {
      id: this.generateAuditId(),
      type: "user_action",
      timestamp: auditData.timestamp || Date.now(),
      action: auditData.action,
      cohortId: auditData.cohortId,
      userId: auditData.userId,
      experimentId: auditData.experimentId,
      context: auditData.context || {},
      metadata: {
        userAgent: auditData.userAgent,
        ipAddress: auditData.ipAddress,
        sessionId: auditData.sessionId,
      },
      hash: this.generateIntegrityHash(auditData),
    };

    return auditEntry;
  }

  async triggerCohortEvents(eventData) {
    // Trigger cohort-level events for external systems
    const cohortEvent = {
      id: this.generateEventId(),
      timestamp: eventData.timestamp,
      type: "cohort_event",
      event: eventData.event,
      cohortId: eventData.cohortId,
      userId: eventData.userId,
      data: eventData.data,
    };

    // Store event for processing
    if (!this.eventHandlers.has(eventData.cohortId)) {
      this.eventHandlers.set(eventData.cohortId, new Map());
    }

    const cohortEventHandlers = this.eventHandlers.get(eventData.cohortId);
    const eventQueue = cohortEventHandlers.get("queue") || [];
    eventQueue.push(cohortEvent);
    cohortEventHandlers.set("queue", eventQueue);

    return cohortEvent;
  }

  async optimizeCohortPerformance(cohort) {
    // Optimize cohort performance based on size and activity
    const optimizations = {
      memoryOptimization: false,
      cacheOptimization: false,
      queryOptimization: false,
      dataCompression: false,
    };

    // Check if cohort needs memory optimization
    if (cohort.users.size > 1000) {
      optimizations.memoryOptimization = true;

      // Implement memory optimization strategies
      await this.optimizeMemoryUsage(cohort);

      // Compress user data for large cohorts
      if (cohort.users.size > 5000) {
        await this.compressUserData(cohort);
      }

      // Archive old journey events to reduce memory footprint
      await this.archiveOldJourneyEvents(cohort);

      // Enable streaming mode for user processing
      optimizations.streamingMode = true;
    }

    // Check if analytics cache needs optimization
    if (Object.keys(cohort.analytics).length > 50) {
      optimizations.cacheOptimization = true;

      // Implement cache optimization strategies
      await this.optimizeAnalyticsCache(cohort);

      // Clear expired cache entries
      await this.clearExpiredCacheEntries(cohort);

      // Implement cache compression for large analytics
      if (Object.keys(cohort.analytics).length > 100) {
        await this.compressAnalyticsCache(cohort);
      }

      // Enable selective caching based on usage patterns
      optimizations.selectiveCaching = true;
    }

    return optimizations;
  }

  async optimizeMemoryUsage(cohort) {
    // Optimize memory usage for large cohorts
    try {
      // Remove unnecessary metadata from user objects
      for (const [userId, userData] of cohort.users) {
        if (userData.metadata && Object.keys(userData.metadata).length > 10) {
          // Keep only essential metadata
          const essentialKeys = [
            "signupDate",
            "lastActivity",
            "segment",
            "source",
          ];
          const optimizedMetadata = {};
          essentialKeys.forEach((key) => {
            if (userData.metadata[key]) {
              optimizedMetadata[key] = userData.metadata[key];
            }
          });
          userData.metadata = optimizedMetadata;
        }
      }

      // Implement weak references for journey data
      cohort._journeyReferences = new WeakMap();

      this.logPerformanceMetric("memory_optimization_completed", {
        cohortId: cohort.id,
        userCount: cohort.users.size,
        timestamp: Date.now(),
      });
    } catch (error) {
      console.error("Memory optimization failed:", error);
    }
  }

  async compressUserData(cohort) {
    // Compress user data for very large cohorts
    try {
      for (const [userId, userData] of cohort.users) {
        // Compress journey events if they're old
        if (userData.journeyEvents && userData.journeyEvents.length > 100) {
          const thirtyDaysAgo = Date.now() - 30 * 24 * 60 * 60 * 1000;
          const recentEvents = userData.journeyEvents.filter(
            (event) => event.timestamp > thirtyDaysAgo
          );
          const oldEvents = userData.journeyEvents.filter(
            (event) => event.timestamp <= thirtyDaysAgo
          );

          // Keep recent events, summarize old ones
          userData.journeyEvents = recentEvents;
          if (oldEvents.length > 0) {
            userData.archivedEventsSummary = {
              count: oldEvents.length,
              firstEvent: oldEvents[0].timestamp,
              lastEvent: oldEvents[oldEvents.length - 1].timestamp,
              eventTypes: [...new Set(oldEvents.map((e) => e.eventType))],
            };
          }
        }
      }
    } catch (error) {
      console.error("User data compression failed:", error);
    }
  }

  async archiveOldJourneyEvents(cohort) {
    // Archive old journey events to reduce memory
    try {
      const archiveThreshold = Date.now() - 90 * 24 * 60 * 60 * 1000; // 90 days
      let archivedCount = 0;

      for (const journey of this.userJourneys.values()) {
        if (journey.cohortId === cohort.id) {
          const recentEvents = journey.events.filter(
            (event) => event.timestamp > archiveThreshold
          );
          const oldEvents = journey.events.filter(
            (event) => event.timestamp <= archiveThreshold
          );

          if (oldEvents.length > 0) {
            journey.events = recentEvents;
            journey.archivedEvents = {
              count: oldEvents.length,
              summary: this.summarizeArchivedEvents(oldEvents),
            };
            archivedCount += oldEvents.length;
          }
        }
      }

      this.logPerformanceMetric("journey_events_archived", {
        cohortId: cohort.id,
        archivedCount,
        timestamp: Date.now(),
      });
    } catch (error) {
      console.error("Journey event archiving failed:", error);
    }
  }

  async optimizeAnalyticsCache(cohort) {
    // Optimize analytics cache performance
    try {
      const cache = cohort.analytics.cache || new Map();
      const expiredKeys = [];
      const now = Date.now();

      // Identify expired cache entries
      for (const [key, entry] of cache) {
        if (entry.expiresAt && entry.expiresAt < now) {
          expiredKeys.push(key);
        }
      }

      // Remove expired entries
      expiredKeys.forEach((key) => cache.delete(key));

      // Implement LRU eviction for cache size management
      if (cache.size > 100) {
        const entries = Array.from(cache.entries()).sort(
          (a, b) => (a[1].lastAccessed || 0) - (b[1].lastAccessed || 0)
        );

        // Remove oldest 25% of entries
        const toRemove = Math.floor(entries.length * 0.25);
        for (let i = 0; i < toRemove; i++) {
          cache.delete(entries[i][0]);
        }
      }

      cohort.analytics.cache = cache;
    } catch (error) {
      console.error("Analytics cache optimization failed:", error);
    }
  }

  async clearExpiredCacheEntries(cohort) {
    // Clear expired cache entries across all analytics
    try {
      const now = Date.now();
      const analytics = cohort.analytics;

      // Clear main cache
      if (analytics.cache) {
        for (const [key, entry] of analytics.cache) {
          if (entry.expiresAt && entry.expiresAt < now) {
            analytics.cache.delete(key);
          }
        }
      }

      // Clear pattern cache
      if (analytics.patterns && analytics.patterns.cache) {
        for (const [key, entry] of analytics.patterns.cache) {
          if (entry.expiresAt && entry.expiresAt < now) {
            analytics.patterns.cache.delete(key);
          }
        }
      }

      // Clear retention cache
      if (analytics.retention && analytics.retention.cache) {
        for (const [key, entry] of analytics.retention.cache) {
          if (entry.expiresAt && entry.expiresAt < now) {
            analytics.retention.cache.delete(key);
          }
        }
      }
    } catch (error) {
      console.error("Cache cleanup failed:", error);
    }
  }

  async compressAnalyticsCache(cohort) {
    // Compress analytics cache for large datasets
    try {
      const analytics = cohort.analytics;

      // Compress large analytical results
      Object.keys(analytics).forEach((key) => {
        if (typeof analytics[key] === "object" && analytics[key] !== null) {
          const size = JSON.stringify(analytics[key]).length;
          if (size > 10000) {
            // If data is larger than 10KB
            // Create compressed summary
            analytics[`${key}_compressed`] = this.compressAnalyticalData(
              analytics[key]
            );
            analytics[`${key}_compression_ratio`] =
              size / JSON.stringify(analytics[`${key}_compressed`]).length;
          }
        }
      });

      this.logPerformanceMetric("analytics_cache_compressed", {
        cohortId: cohort.id,
        timestamp: Date.now(),
      });
    } catch (error) {
      console.error("Analytics cache compression failed:", error);
    }
  }

  summarizeArchivedEvents(events) {
    // Create summary of archived events
    const summary = {
      totalEvents: events.length,
      eventTypes: {},
      timeRange: {
        start: Math.min(...events.map((e) => e.timestamp)),
        end: Math.max(...events.map((e) => e.timestamp)),
      },
      uniqueDays: new Set(
        events.map((e) => new Date(e.timestamp).toDateString())
      ).size,
    };

    // Count event types
    events.forEach((event) => {
      summary.eventTypes[event.eventType] =
        (summary.eventTypes[event.eventType] || 0) + 1;
    });

    return summary;
  }

  compressAnalyticalData(data) {
    // Compress analytical data by removing redundant information
    if (Array.isArray(data)) {
      // For arrays, keep sample data and statistics
      return {
        type: "compressed_array",
        length: data.length,
        sample: data.slice(0, 10), // First 10 items
        statistics: {
          mean:
            data.reduce((a, b) => a + (typeof b === "number" ? b : 0), 0) /
            data.length,
          min: Math.min(...data.filter((x) => typeof x === "number")),
          max: Math.max(...data.filter((x) => typeof x === "number")),
        },
      };
    } else if (typeof data === "object") {
      // For objects, keep essential keys and summarize large nested objects
      const compressed = {};
      Object.keys(data).forEach((key) => {
        if (typeof data[key] === "object" && data[key] !== null) {
          compressed[key] =
            Object.keys(data[key]).length > 10
              ? {
                  keys: Object.keys(data[key]).length,
                  sample: Object.keys(data[key]).slice(0, 5),
                }
              : data[key];
        } else {
          compressed[key] = data[key];
        }
      });
      return compressed;
    }

    return data;
  }

  logPerformanceMetric(metric, data) {
    // Log performance metrics for monitoring
    if (!this.performanceMetrics) {
      this.performanceMetrics = new Map();
    }

    const metricEntry = {
      metric,
      data,
      timestamp: Date.now(),
    };

    this.performanceMetrics.set(`${metric}_${Date.now()}`, metricEntry);

    // Keep only last 1000 metrics to prevent memory growth
    if (this.performanceMetrics.size > 1000) {
      const entries = Array.from(this.performanceMetrics.entries()).sort(
        (a, b) => a[1].timestamp - b[1].timestamp
      );

      // Remove oldest 200 entries
      for (let i = 0; i < 200; i++) {
        this.performanceMetrics.delete(entries[i][0]);
      }
    }
  }

  /**
   * Journey Tracking Helper Methods
   */
  async validateJourneyEventData(journeyEvent) {
    const validation = { valid: true, errors: [] };

    if (!journeyEvent.eventType) {
      validation.errors.push("eventType is required");
    }

    if (!journeyEvent.eventName) {
      validation.errors.push("eventName is required");
    }

    if (
      journeyEvent.properties &&
      typeof journeyEvent.properties !== "object"
    ) {
      validation.errors.push("properties must be an object");
    }

    if (journeyEvent.context && typeof journeyEvent.context !== "object") {
      validation.errors.push("context must be an object");
    }

    validation.valid = validation.errors.length === 0;
    return validation;
  }

  async updateJourneyAnalytics(journey, journeyEventData, patterns) {
    // Update journey analytics with new event data
    journey.analytics.sessionCount = await this.calculateSessionCount(journey);
    journey.analytics.totalDuration = await this.calculateTotalDuration(
      journey
    );
    journey.analytics.conversionEvents = await this.countConversionEvents(
      journey
    );
    journey.analytics.dropoffPoints = patterns.dropoffPoints || [];
    journey.analytics.engagementScore = await this.calculateEngagementScore(
      journey
    );

    return journey.analytics;
  }

  async checkJourneyMilestones(journey, journeyEventData) {
    // Check if user has reached any journey milestones
    const milestones = [];

    // Check event count milestones
    if (journey.eventCount === 1) {
      milestones.push({
        type: "first_event",
        timestamp: journeyEventData.timestamp,
      });
    }
    if (journey.eventCount === 10) {
      milestones.push({
        type: "ten_events",
        timestamp: journeyEventData.timestamp,
      });
    }
    if (journey.eventCount === 50) {
      milestones.push({
        type: "fifty_events",
        timestamp: journeyEventData.timestamp,
      });
    }

    // Check time-based milestones
    const journeyDuration = Date.now() - journey.startedAt;
    if (
      journeyDuration > 86400000 &&
      !journey.milestones.some((m) => m.type === "one_day")
    ) {
      // 1 day
      milestones.push({
        type: "one_day",
        timestamp: journeyEventData.timestamp,
      });
    }

    // Check conversion milestones
    if (journeyEventData.eventType === "conversion") {
      milestones.push({
        type: "conversion",
        timestamp: journeyEventData.timestamp,
      });
    }

    return [...(journey.milestones || []), ...milestones];
  }

  async generateJourneyInsights(journey, journeyEventData) {
    // Generate insights based on journey data
    const insights = {
      patterns: [],
      recommendations: [],
      risks: [],
      opportunities: [],
    };

    // Analyze event frequency
    if (journey.eventCount > 20) {
      insights.patterns.push("High engagement user - frequent events");
    }

    // Analyze session patterns
    const avgTimeBetweenEvents =
      journey.events.length > 1
        ? (journey.lastActivity - journey.startedAt) / journey.events.length
        : 0;

    if (avgTimeBetweenEvents < 60000) {
      // Less than 1 minute
      insights.patterns.push("Rapid interaction pattern");
    }

    // Check for conversion patterns
    const conversionEvents = journey.events.filter(
      (e) => e.eventType === "conversion"
    );
    if (conversionEvents.length > 1) {
      insights.opportunities.push(
        "Multiple conversion events - high value user"
      );
    }

    // Check for dropoff risks
    const timeSinceLastEvent = Date.now() - journey.lastActivity;
    if (timeSinceLastEvent > 86400000) {
      // 1 day
      insights.risks.push("User has not been active for over 24 hours");
    }

    return insights;
  }

  async updateCohortJourneyAnalytics(cohortId, journeyEventData) {
    // Update cohort-level journey analytics
    const cohort = this.cohorts.get(cohortId);
    if (!cohort) return;

    if (!cohort.analytics.journeyAnalytics) {
      cohort.analytics.journeyAnalytics = {
        totalEvents: 0,
        uniqueEventTypes: new Set(),
        averageJourneyLength: 0,
        commonPaths: [],
      };
    }

    cohort.analytics.journeyAnalytics.totalEvents++;
    cohort.analytics.journeyAnalytics.uniqueEventTypes.add(
      journeyEventData.eventType
    );

    return cohort.analytics.journeyAnalytics;
  }

  async createJourneyAuditEntry(auditData) {
    // Create journey-specific audit entry
    const auditEntry = {
      id: this.generateAuditId(),
      type: "journey_action",
      timestamp: auditData.timestamp || Date.now(),
      action: auditData.action,
      journeyId: auditData.journeyId,
      eventId: auditData.eventId,
      userId: auditData.userId,
      eventType: auditData.eventType,
      cohortId: auditData.cohortId,
      hash: this.generateIntegrityHash(auditData),
    };

    return auditEntry;
  }

  /**
   * Pattern Analysis Helper Methods
   */
  async identifyCommonPaths(journey) {
    // Identify common paths through user journey
    const paths = [];
    const events = journey.events;

    // Analyze sequential event patterns
    for (let i = 0; i < events.length - 2; i++) {
      const path = [
        events[i].eventType,
        events[i + 1].eventType,
        events[i + 2].eventType,
      ].join(" -> ");
      paths.push(path);
    }

    // Count path frequencies
    const pathCounts = {};
    paths.forEach((path) => {
      pathCounts[path] = (pathCounts[path] || 0) + 1;
    });

    // Return top paths
    return Object.entries(pathCounts)
      .sort(([, a], [, b]) => b - a)
      .slice(0, 10)
      .map(([path, count]) => ({ path, count }));
  }

  async identifyDropoffPoints(journey) {
    // Identify potential dropoff points in user journey
    const dropoffPoints = [];
    const events = journey.events;

    for (let i = 0; i < events.length - 1; i++) {
      const timeDiff = events[i + 1].timestamp - events[i].timestamp;

      // If time between events is > 1 hour, consider it a potential dropoff
      if (timeDiff > 3600000) {
        dropoffPoints.push({
          eventIndex: i,
          eventType: events[i].eventType,
          timeDiff: timeDiff,
          severity: timeDiff > 86400000 ? "high" : "medium", // > 1 day = high
        });
      }
    }

    return dropoffPoints;
  }

  async identifyConversionPatterns(journey) {
    // Identify conversion patterns in user journey
    const conversionEvents = journey.events.filter(
      (e) =>
        e.eventType === "conversion" ||
        e.eventType === "purchase" ||
        e.eventType === "signup"
    );

    const patterns = {
      conversionCount: conversionEvents.length,
      conversionRate:
        journey.events.length > 0
          ? conversionEvents.length / journey.events.length
          : 0,
      timeToConversion: [],
      conversionPaths: [],
    };

    // Calculate time to conversion
    conversionEvents.forEach((conversionEvent) => {
      const timeToConversion = conversionEvent.timestamp - journey.startedAt;
      patterns.timeToConversion.push(timeToConversion);
    });

    return patterns;
  }

  async analyzeSessionPatterns(journey) {
    // Analyze session patterns within user journey
    const sessions = [];
    let currentSession = null;
    const sessionTimeout = this.journeyTracking.sessionTimeout;

    journey.events.forEach((event) => {
      if (
        !currentSession ||
        event.timestamp - currentSession.lastActivity > sessionTimeout
      ) {
        // Start new session
        currentSession = {
          startTime: event.timestamp,
          endTime: event.timestamp,
          lastActivity: event.timestamp,
          events: [event],
          duration: 0,
        };
        sessions.push(currentSession);
      } else {
        // Continue current session
        currentSession.events.push(event);
        currentSession.endTime = event.timestamp;
        currentSession.lastActivity = event.timestamp;
        currentSession.duration =
          currentSession.endTime - currentSession.startTime;
      }
    });

    return {
      sessionCount: sessions.length,
      averageSessionDuration:
        sessions.length > 0
          ? sessions.reduce((sum, s) => sum + s.duration, 0) / sessions.length
          : 0,
      averageEventsPerSession:
        sessions.length > 0
          ? sessions.reduce((sum, s) => sum + s.events.length, 0) /
            sessions.length
          : 0,
      sessions: sessions,
    };
  }
  initializeCohortTypes() {
    // Initialize standard cohort types with comprehensive definitions
    this.cohortDefinitions.set("acquisition", {
      name: "Acquisition Cohort",
      description: "Users grouped by acquisition date and source",
      criteria: {
        type: "acquisition_date",
        timeWindow: true,
        sourceTracking: true,
      },
      defaultPeriod: "daily",
      retentionTracking: true,
      analytics: ["retention", "source_analysis", "journey_mapping"],
    });

    this.cohortDefinitions.set("behavioral", {
      name: "Behavioral Cohort",
      description: "Users grouped by behavior patterns and engagement levels",
      criteria: {
        type: "behavior_pattern",
        engagementThresholds: true,
        activityPatterns: true,
      },
      defaultPeriod: "weekly",
      retentionTracking: true,
      analytics: ["engagement", "pattern_analysis", "conversion_tracking"],
    });

    this.cohortDefinitions.set("demographic", {
      name: "Demographic Cohort",
      description:
        "Users grouped by demographic attributes and characteristics",
      criteria: {
        type: "demographic",
        ageGroups: true,
        locationBased: true,
        deviceType: true,
      },
      defaultPeriod: "monthly",
      retentionTracking: true,
      analytics: ["demographic_analysis", "geo_analysis", "device_analysis"],
    });

    this.cohortDefinitions.set("experimental", {
      name: "Experimental Cohort",
      description: "Users grouped for A/B testing and experimentation",
      criteria: {
        type: "experimental",
        variantAssignment: true,
        randomization: true,
      },
      defaultPeriod: "custom",
      retentionTracking: true,
      analytics: [
        "variant_comparison",
        "statistical_analysis",
        "conversion_optimization",
      ],
    });

    this.cohortDefinitions.set("temporal", {
      name: "Temporal Cohort",
      description: "Users grouped by time-based patterns and seasonality",
      criteria: {
        type: "temporal",
        seasonalPatterns: true,
        timeOfDay: true,
        dayOfWeek: true,
      },
      defaultPeriod: "seasonal",
      retentionTracking: true,
      analytics: ["temporal_analysis", "seasonal_trends", "activity_patterns"],
    });

    // Initialize cohort type metrics
    this.cohortMetrics.cohortTypes = {
      acquisition: 0,
      behavioral: 0,
      demographic: 0,
      experimental: 0,
      temporal: 0,
    };
  }

  generateCohortId() {
    return `cohort_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateJourneyEventId() {
    return `journey_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  getUserJourneyId(userId) {
    return `journey_${userId}`;
  }

  async validateCohortConfig(cohort) {
    // Validate cohort configuration with comprehensive checks
    const errors = [];
    const warnings = [];

    // Required field validation
    if (!cohort.name) {
      errors.push("Cohort name is required");
    }

    if (!cohort.experimentId) {
      errors.push("Experiment ID is required");
    }

    if (!cohort.criteria) {
      errors.push("Cohort criteria is required");
    }

    // Name validation
    if (cohort.name && cohort.name.length < 3) {
      errors.push("Cohort name must be at least 3 characters long");
    }

    if (cohort.name && cohort.name.length > 100) {
      errors.push("Cohort name must be less than 100 characters");
    }

    // Date validation
    if (
      cohort.startDate &&
      cohort.endDate &&
      cohort.startDate >= cohort.endDate
    ) {
      errors.push("Start date must be before end date");
    }

    // Criteria validation
    if (cohort.criteria) {
      if (!cohort.criteria.type) {
        errors.push("Cohort criteria type is required");
      }

      const validTypes = [
        "acquisition",
        "behavioral",
        "demographic",
        "experimental",
        "temporal",
      ];
      if (cohort.criteria.type && !validTypes.includes(cohort.criteria.type)) {
        errors.push(
          `Invalid criteria type. Must be one of: ${validTypes.join(", ")}`
        );
      }
    }

    // Period validation
    const validPeriods = ["daily", "weekly", "monthly", "custom"];
    if (cohort.period && !validPeriods.includes(cohort.period)) {
      warnings.push(
        `Invalid period '${cohort.period}'. Valid options: ${validPeriods.join(
          ", "
        )}`
      );
    }

    // Cohort type validation
    const validCohortTypes = Array.from(this.cohortDefinitions.keys());
    if (cohort.cohortType && !validCohortTypes.includes(cohort.cohortType)) {
      warnings.push(
        `Unknown cohort type '${
          cohort.cohortType
        }'. Valid types: ${validCohortTypes.join(", ")}`
      );
    }

    // Check for duplicate names
    const existingCohorts = Array.from(this.cohorts.values());
    const duplicateName = existingCohorts.find(
      (c) => c.name === cohort.name && c.id !== cohort.id
    );
    if (duplicateName) {
      warnings.push(`Cohort name '${cohort.name}' already exists`);
    }

    return {
      valid: errors.length === 0,
      errors: errors,
      warnings: warnings,
      severity:
        errors.length > 0 ? "error" : warnings.length > 0 ? "warning" : "valid",
    };
  }

  async createCohortAuditEntry(auditData) {
    // Create cohort audit entry with comprehensive tracking
    const auditEntry = {
      id: this.generateAuditId(),
      type: "cohort_action",
      timestamp: auditData.timestamp || Date.now(),
      action: auditData.action,
      cohortId: auditData.cohortId,
      experimentId: auditData.experimentId,
      details: {
        ...auditData,
        systemInfo: {
          version: "1.0",
          environment: process.env.NODE_ENV || "development",
          timestamp: new Date().toISOString(),
        },
      },
      hash: this.generateIntegrityHash(auditData),
      metadata: {
        userAgent: auditData.userAgent || "system",
        ipAddress: auditData.ipAddress || "localhost",
        sessionId: auditData.sessionId || this.generateSessionId(),
      },
      integrity: {
        checksumAlgorithm: "base64",
        verified: true,
        createdBy: "cohort-tracker",
      },
    };

    // Store audit entry (in production would persist to database)
    if (!this.auditTrail) {
      this.auditTrail = new Map();
    }
    this.auditTrail.set(auditEntry.id, auditEntry);

    return auditEntry;
  }

  generateAuditId() {
    return `audit_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateIntegrityHash(data) {
    // Generate integrity hash for audit trail with comprehensive checksum
    try {
      // Create a deterministic string representation of the data
      const dataString = JSON.stringify(data, Object.keys(data).sort());

      // Generate a simple hash (in production would use crypto module)
      let hash = 0;
      for (let i = 0; i < dataString.length; i++) {
        const char = dataString.charCodeAt(i);
        hash = (hash << 5) - hash + char;
        hash = hash & hash; // Convert to 32-bit integer
      }

      // Convert to base64-like string
      const hashString = Math.abs(hash).toString(36);
      const timestamp = Date.now().toString(36);
      const combined = `${hashString}_${timestamp}`;

      return Buffer.from(combined).toString("base64");
    } catch (error) {
      // Fallback hash generation
      const fallbackData = {
        timestamp: Date.now(),
        random: Math.random(),
        action: data.action || "unknown",
      };
      return Buffer.from(JSON.stringify(fallbackData)).toString("base64");
    }
  }

  generateSessionId() {
    return `session_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  /**
   * Additional Pattern Analysis Methods
   */
  async identifyBehavioralPatterns(journey) {
    // Identify behavioral patterns in user actions
    const behaviors = {
      repetitiveActions: await this.findRepetitiveActions(journey),
      explorationPattern: await this.analyzeExplorationPattern(journey),
      engagementLevels: await this.categorizeEngagementLevels(journey),
      habitFormation: await this.analyzeHabitFormation(journey),
    };

    return behaviors;
  }

  async analyzeTemporalPatterns(journey) {
    // Analyze temporal patterns in user behavior
    const temporalData = {
      activityByHour: new Array(24).fill(0),
      activityByDay: new Array(7).fill(0),
      peakActivityTimes: [],
      seasonalPatterns: {},
    };

    journey.events.forEach((event) => {
      const date = new Date(event.timestamp);
      const hour = date.getHours();
      const day = date.getDay();

      temporalData.activityByHour[hour]++;
      temporalData.activityByDay[day]++;
    });

    // Find peak activity times
    const maxHourlyActivity = Math.max(...temporalData.activityByHour);
    temporalData.activityByHour.forEach((count, hour) => {
      if (count === maxHourlyActivity) {
        temporalData.peakActivityTimes.push({ hour, count });
      }
    });

    return temporalData;
  }

  async analyzeEngagementPatterns(journey) {
    // Analyze user engagement patterns
    const engagement = {
      engagementScore: 0,
      deepEngagementEvents: 0,
      shallowEngagementEvents: 0,
      engagementTrend: "stable",
      engagementDistribution: {},
    };

    // Calculate engagement score based on event types and frequency
    journey.events.forEach((event) => {
      switch (event.eventType) {
        case "page_view":
          engagement.engagementScore += 1;
          engagement.shallowEngagementEvents++;
          break;
        case "click":
          engagement.engagementScore += 2;
          break;
        case "form_submit":
          engagement.engagementScore += 5;
          engagement.deepEngagementEvents++;
          break;
        case "purchase":
        case "conversion":
          engagement.engagementScore += 10;
          engagement.deepEngagementEvents++;
          break;
        default:
          engagement.engagementScore += 1;
      }
    });

    return engagement;
  }

  async analyzeFunnelPatterns(journey) {
    // Analyze funnel progression patterns
    const funnelStages = [
      "awareness",
      "interest",
      "consideration",
      "conversion",
    ];
    const funnelData = {
      stages: {},
      progression: [],
      dropoffs: [],
      conversions: [],
    };

    // Map events to funnel stages
    journey.events.forEach((event) => {
      let stage = "awareness"; // default

      switch (event.eventType) {
        case "page_view":
          stage = "awareness";
          break;
        case "click":
        case "engagement":
          stage = "interest";
          break;
        case "form_start":
        case "add_to_cart":
          stage = "consideration";
          break;
        case "purchase":
        case "conversion":
          stage = "conversion";
          break;
      }

      if (!funnelData.stages[stage]) {
        funnelData.stages[stage] = 0;
      }
      funnelData.stages[stage]++;
    });

    return funnelData;
  }

  async calculateJourneyMetrics(journey) {
    // Calculate comprehensive journey metrics
    const metrics = {
      totalEvents: journey.eventCount,
      uniqueEventTypes: new Set(journey.events.map((e) => e.eventType)).size,
      journeyDuration: journey.lastActivity - journey.startedAt,
      averageTimeBetweenEvents: 0,
      eventFrequency: {},
      conversionRate: 0,
      bounceRate: 0,
    };

    if (journey.events.length > 1) {
      const totalTimeDiffs = journey.events
        .slice(1)
        .reduce((sum, event, index) => {
          return sum + (event.timestamp - journey.events[index].timestamp);
        }, 0);
      metrics.averageTimeBetweenEvents =
        totalTimeDiffs / (journey.events.length - 1);
    }

    // Calculate event frequencies
    journey.events.forEach((event) => {
      metrics.eventFrequency[event.eventType] =
        (metrics.eventFrequency[event.eventType] || 0) + 1;
    });

    // Calculate conversion rate
    const conversionEvents = journey.events.filter(
      (e) => e.eventType === "conversion" || e.eventType === "purchase"
    );
    metrics.conversionRate =
      journey.events.length > 0
        ? conversionEvents.length / journey.events.length
        : 0;

    // Calculate bounce rate (single event sessions)
    metrics.bounceRate = journey.events.length === 1 ? 1 : 0;

    return metrics;
  }

  async generatePatternInsights(patterns, journeyMetrics) {
    // Generate insights from identified patterns
    const insights = {
      summary: [],
      recommendations: [],
      warnings: [],
      opportunities: [],
    };

    // Analyze common paths
    if (patterns.commonPaths.length > 0) {
      insights.summary.push(
        `Most common user path: ${patterns.commonPaths[0].path}`
      );
    }

    // Analyze dropoff points
    if (patterns.dropoffPoints.length > 0) {
      const highSeverityDropoffs = patterns.dropoffPoints.filter(
        (d) => d.severity === "high"
      );
      if (highSeverityDropoffs.length > 0) {
        insights.warnings.push(
          `${highSeverityDropoffs.length} high-severity dropoff points detected`
        );
      }
    }

    // Analyze conversion patterns
    if (patterns.conversionPatterns.conversionRate > 0.1) {
      insights.opportunities.push(
        "High conversion rate detected - optimize for retention"
      );
    } else if (patterns.conversionPatterns.conversionRate < 0.01) {
      insights.warnings.push(
        "Low conversion rate - consider journey optimization"
      );
    }

    // Analyze engagement patterns
    if (patterns.engagementPatterns.engagementScore > 50) {
      insights.opportunities.push(
        "High engagement user - candidate for premium features"
      );
    }

    return insights;
  }

  async updatePatternAnalytics(journey, patterns, insights) {
    // Update pattern analytics with new findings
    if (!journey.analytics.patterns) {
      journey.analytics.patterns = {};
    }

    journey.analytics.patterns = {
      lastAnalyzed: Date.now(),
      patterns: patterns,
      insights: insights,
      patternCount: Object.keys(patterns).length,
      insightCount: Object.values(insights).flat().length,
    };

    return journey.analytics.patterns;
  }

  async cachePatternResults(journeyId, patterns, insights) {
    // Cache pattern analysis results
    const cacheKey = `patterns_${journeyId}`;
    const cacheData = {
      patterns: patterns,
      insights: insights,
      cachedAt: Date.now(),
      expiresAt: Date.now() + this.config.cacheTimeout,
    };

    // Store in analytics cache
    if (!this.analytics.customMetrics.has("patternCache")) {
      this.analytics.customMetrics.set("patternCache", new Map());
    }

    this.analytics.customMetrics.get("patternCache").set(cacheKey, cacheData);
    return cacheData;
  }

  async generatePatternReports(journey, patterns) {
    // Generate comprehensive pattern reports
    const report = {
      journeyId: journey.id,
      userId: journey.userId,
      reportType: "pattern_analysis",
      generatedAt: Date.now(),
      summary: {
        totalPatterns: Object.keys(patterns).length,
        keyFindings: [],
        recommendations: [],
      },
      details: patterns,
    };

    // Add key findings
    if (patterns.commonPaths.length > 0) {
      report.summary.keyFindings.push(
        `Primary user path: ${patterns.commonPaths[0].path}`
      );
    }

    if (patterns.conversionPatterns.conversionCount > 0) {
      report.summary.keyFindings.push(
        `Conversion events: ${patterns.conversionPatterns.conversionCount}`
      );
    }

    // Add recommendations
    if (patterns.dropoffPoints.length > 0) {
      report.summary.recommendations.push(
        "Address identified dropoff points to improve retention"
      );
    }

    if (patterns.engagementPatterns.engagementScore > 30) {
      report.summary.recommendations.push(
        "High engagement user - consider personalized content"
      );
    }

    return report;
  }

  /**
   * Retention Analysis Helper Methods
   */
  async generateRetentionCurves(cohort, retentionRates) {
    // Generate retention curves for visualization
    const curves = {
      primary: [],
      smoothed: [],
      projected: [],
    };

    // Create primary retention curve
    Object.entries(retentionRates).forEach(([period, rate]) => {
      const day = parseInt(period.replace("day_", ""));
      curves.primary.push({ day, rate });
    });

    // Sort by day
    curves.primary.sort((a, b) => a.day - b.day);

    // Create smoothed curve using moving average
    for (let i = 0; i < curves.primary.length; i++) {
      const window = curves.primary.slice(Math.max(0, i - 1), i + 2);
      const avgRate =
        window.reduce((sum, point) => sum + point.rate, 0) / window.length;
      curves.smoothed.push({ day: curves.primary[i].day, rate: avgRate });
    }

    // Project future retention (simple linear projection)
    if (curves.primary.length >= 2) {
      const lastTwo = curves.primary.slice(-2);
      const slope =
        (lastTwo[1].rate - lastTwo[0].rate) / (lastTwo[1].day - lastTwo[0].day);

      for (
        let day = curves.primary[curves.primary.length - 1].day + 30;
        day <= 365;
        day += 30
      ) {
        const projectedRate = Math.max(
          0,
          lastTwo[1].rate + slope * (day - lastTwo[1].day)
        );
        curves.projected.push({ day, rate: projectedRate });
      }
    }

    return curves;
  }

  async identifyRetentionPatterns(cohort, retentionRates) {
    // Identify patterns in retention data
    const patterns = {
      trend: "stable",
      criticalDropPoints: [],
      recoveryPoints: [],
      seasonality: {},
      anomalies: [],
    };

    const rates = Object.entries(retentionRates)
      .map(([period, rate]) => ({
        day: parseInt(period.replace("day_", "")),
        rate,
      }))
      .sort((a, b) => a.day - b.day);

    // Analyze trend
    if (rates.length >= 2) {
      const firstRate = rates[0].rate;
      const lastRate = rates[rates.length - 1].rate;
      const change = (lastRate - firstRate) / firstRate;

      if (change < -0.2) patterns.trend = "declining";
      else if (change > 0.1) patterns.trend = "improving";
      else patterns.trend = "stable";
    }

    // Identify critical drop points (>20% drop between periods)
    for (let i = 1; i < rates.length; i++) {
      const drop = (rates[i - 1].rate - rates[i].rate) / rates[i - 1].rate;
      if (drop > 0.2) {
        patterns.criticalDropPoints.push({
          fromDay: rates[i - 1].day,
          toDay: rates[i].day,
          dropPercentage: drop * 100,
        });
      }
    }

    return patterns;
  }

  async compareRetentionAcrossVariants(cohort, retentionRates) {
    // Compare retention across A/B test variants
    if (!cohort.experimentId) {
      return { comparison: "not_applicable", reason: "No experiment ID" };
    }

    // This would integrate with A/B testing framework
    const comparison = {
      experimentId: cohort.experimentId,
      variantComparison: {},
      statisticalSignificance: false,
      confidenceLevel: 0.95,
    };

    // Get comparison data from other cohorts in the same experiment
    const experimentCohorts =
      this.cohorts.get(cohort.experimentId) || new Map();
    const otherCohorts = Array.from(experimentCohorts.values()).filter(
      (c) => c.id !== cohort.id && c.variant !== cohort.variant
    );

    if (otherCohorts.length > 0) {
      // Calculate comparative metrics
      for (const otherCohort of otherCohorts) {
        const otherMetrics = await this.calculateCohortMetrics(otherCohort);
        const conversionDiff =
          metrics.conversionRate - otherMetrics.conversionRate;
        const retentionDiff =
          metrics.retentionRate - otherMetrics.retentionRate;

        comparison.variants.push({
          variant: otherCohort.variant,
          conversionDiff: this.roundToPrecision(conversionDiff, 4),
          retentionDiff: this.roundToPrecision(retentionDiff, 4),
          significance: this.calculateStatisticalSignificance(
            metrics,
            otherMetrics
          ),
          sampleSize: otherCohort.userCount,
        });
      }

      // Determine performance ranking
      const allCohorts = [cohort, ...otherCohorts];
      const performanceRanking = allCohorts
        .map((c) => ({
          variant: c.variant,
          score: c.conversionRate * 0.6 + c.retentionRate * 0.4,
        }))
        .sort((a, b) => b.score - a.score);

      comparison.performance.ranking = performanceRanking;
      comparison.performance.isTopPerformer =
        performanceRanking[0].variant === cohort.variant;
    }

    return comparison;
  }

  async generateRetentionInsights(cohort, retentionRates, patterns) {
    // Generate actionable retention insights
    const insights = {
      summary: [],
      recommendations: [],
      alerts: [],
      benchmarks: {},
    };

    // Analyze 7-day retention (industry standard)
    const day7Retention = retentionRates.day_7 || 0;
    if (day7Retention > 0.4) {
      insights.summary.push("Strong 7-day retention rate");
    } else if (day7Retention < 0.2) {
      insights.alerts.push("Low 7-day retention rate requires attention");
      insights.recommendations.push("Implement early engagement campaigns");
    }

    // Analyze 30-day retention
    const day30Retention = retentionRates.day_30 || 0;
    if (day30Retention > 0.2) {
      insights.summary.push("Good long-term retention");
    } else if (day30Retention < 0.1) {
      insights.alerts.push("Poor long-term retention");
      insights.recommendations.push(
        "Focus on feature adoption and value delivery"
      );
    }

    // Analyze critical drop points
    if (patterns.criticalDropPoints.length > 0) {
      insights.alerts.push(
        `${patterns.criticalDropPoints.length} critical retention drop points identified`
      );
      insights.recommendations.push(
        "Investigate user experience at critical drop points"
      );
    }

    // Set benchmarks
    insights.benchmarks = {
      day1: 0.8, // 80% day 1 retention
      day7: 0.3, // 30% day 7 retention
      day30: 0.15, // 15% day 30 retention
    };

    return insights;
  }

  async analyzeSegmentedRetention(cohort, retentionPeriods) {
    // Analyze retention by user segments
    const segmentedAnalysis = {};

    // Analyze by acquisition source
    const sourceSegments = await this.segmentUsersBySource(cohort);
    for (const [source, users] of sourceSegments) {
      const segmentRetention = {};
      for (const period of retentionPeriods) {
        segmentRetention[`day_${period}`] =
          await this.calculateSegmentRetention(users, period);
      }
      segmentedAnalysis[`source_${source}`] = segmentRetention;
    }

    // Analyze by user engagement level
    const engagementSegments = await this.segmentUsersByEngagement(cohort);
    for (const [level, users] of engagementSegments) {
      const segmentRetention = {};
      for (const period of retentionPeriods) {
        segmentRetention[`day_${period}`] =
          await this.calculateSegmentRetention(users, period);
      }
      segmentedAnalysis[`engagement_${level}`] = segmentRetention;
    }

    return segmentedAnalysis;
  }

  async updateRetentionAnalytics(cohort, retentionAnalysis) {
    // Update cohort retention analytics
    cohort.analytics.retention = {
      lastAnalyzed: retentionAnalysis.analyzedAt,
      overallRetention: retentionAnalysis.retentionRates,
      insights: retentionAnalysis.insights,
      patterns: retentionAnalysis.patterns,
      segmentedAnalysis: retentionAnalysis.segments,
    };

    return cohort.analytics.retention;
  }

  async generateRetentionReports(retentionAnalysis) {
    // Generate comprehensive retention reports
    const report = {
      reportId: this.generateReportId(),
      cohortId: retentionAnalysis.cohortId,
      reportType: "retention_analysis",
      generatedAt: Date.now(),
      summary: {
        totalUsers: retentionAnalysis.totalUsers,
        keyMetrics: {},
        trends: [],
        recommendations: [],
      },
      data: retentionAnalysis,
    };

    // Extract key metrics
    report.summary.keyMetrics = {
      day1Retention: retentionAnalysis.retentionRates.day_1 || 0,
      day7Retention: retentionAnalysis.retentionRates.day_7 || 0,
      day30Retention: retentionAnalysis.retentionRates.day_30 || 0,
    };

    // Add trends
    if (retentionAnalysis.patterns.trend === "declining") {
      report.summary.trends.push("Retention rates are declining over time");
    } else if (retentionAnalysis.patterns.trend === "improving") {
      report.summary.trends.push("Retention rates are improving over time");
    }

    // Add recommendations from insights
    if (retentionAnalysis.insights.recommendations) {
      report.summary.recommendations =
        retentionAnalysis.insights.recommendations;
    }

    return report;
  }

  async optimizeRetentionTracking(cohort, retentionAnalysis) {
    // Optimize retention tracking based on analysis results
    const optimizations = {
      trackingFrequency: "daily",
      focusAreas: [],
      automatedAlerts: [],
    };

    // Adjust tracking frequency based on retention patterns
    if (retentionAnalysis.patterns.criticalDropPoints.length > 2) {
      optimizations.trackingFrequency = "hourly";
      optimizations.focusAreas.push("critical_drop_points");
    }

    // Set up automated alerts
    if (retentionAnalysis.retentionRates.day_7 < 0.2) {
      optimizations.automatedAlerts.push({
        type: "low_retention",
        threshold: 0.2,
        period: "day_7",
      });
    }

    return optimizations;
  }

  /**
   * Segmentation Helper Methods
   */
  async applySegmentationRule(cohort, rule) {
    // Apply segmentation rule to cohort users
    const segmentUsers = [];

    for (const userId of cohort.users) {
      const userJourney = this.getUserJourney(userId);
      const meetsRule = await this.evaluateSegmentationRule(
        rule,
        userId,
        userJourney
      );

      if (meetsRule) {
        segmentUsers.push(userId);
      }
    }

    return segmentUsers;
  }

  async calculateSegmentAnalytics(segmentUsers) {
    // Calculate analytics for user segment
    const analytics = {
      userCount: segmentUsers.length,
      avgEngagement: 0,
      avgRetention: 0,
      conversionRate: 0,
      topEvents: new Map(),
    };

    if (segmentUsers.length === 0) return analytics;

    let totalEngagement = 0;
    let totalConversions = 0;
    let totalEvents = 0;

    for (const userId of segmentUsers) {
      const journey = this.getUserJourney(userId);
      if (journey) {
        // Calculate engagement
        const engagementScore = await this.calculateEngagementScore(journey);
        totalEngagement += engagementScore;

        // Count conversions
        const conversions = journey.events.filter(
          (e) => e.eventType === "conversion" || e.eventType === "purchase"
        );
        totalConversions += conversions.length;

        // Count event types
        journey.events.forEach((event) => {
          const count = analytics.topEvents.get(event.eventType) || 0;
          analytics.topEvents.set(event.eventType, count + 1);
        });

        totalEvents += journey.events.length;
      }
    }

    analytics.avgEngagement = totalEngagement / segmentUsers.length;
    analytics.conversionRate =
      totalEvents > 0 ? totalConversions / totalEvents : 0;

    return analytics;
  }

  async createSegmentAnalytics(segments) {
    // Create comparative analytics across segments
    const comparison = {
      totalSegments: Object.keys(segments).length,
      largestSegment: null,
      highestEngagement: null,
      highestConversion: null,
      segmentOverlap: {},
    };

    let maxSize = 0;
    let maxEngagement = 0;
    let maxConversion = 0;

    for (const [segmentName, segment] of Object.entries(segments)) {
      if (segment.count > maxSize) {
        maxSize = segment.count;
        comparison.largestSegment = segmentName;
      }

      if (segment.analytics.avgEngagement > maxEngagement) {
        maxEngagement = segment.analytics.avgEngagement;
        comparison.highestEngagement = segmentName;
      }

      if (segment.analytics.conversionRate > maxConversion) {
        maxConversion = segment.analytics.conversionRate;
        comparison.highestConversion = segmentName;
      }
    }

    return comparison;
  }

  async generateSegmentInsights(cohort, segments) {
    // Generate insights from segmentation analysis
    const insights = {
      keyFindings: [],
      recommendations: [],
      opportunities: [],
      risks: [],
    };

    // Analyze segment sizes
    const segmentSizes = Object.values(segments).map((s) => s.count);
    const totalUsers = cohort.users.size;
    const avgSegmentSize =
      segmentSizes.reduce((sum, size) => sum + size, 0) / segmentSizes.length;

    if (avgSegmentSize / totalUsers > 0.3) {
      insights.keyFindings.push(
        "Large segment sizes indicate clear user patterns"
      );
    }

    // Analyze engagement differences
    const engagementScores = Object.values(segments).map(
      (s) => s.analytics.avgEngagement
    );
    const maxEngagement = Math.max(...engagementScores);
    const minEngagement = Math.min(...engagementScores);

    if (maxEngagement / minEngagement > 2) {
      insights.opportunities.push(
        "Significant engagement differences between segments - optimize low-performing segments"
      );
    }

    // Analyze conversion differences
    const conversionRates = Object.values(segments).map(
      (s) => s.analytics.conversionRate
    );
    const maxConversion = Math.max(...conversionRates);
    const minConversion = Math.min(...conversionRates);

    if (maxConversion > 0.1 && minConversion < 0.05) {
      insights.recommendations.push(
        "Focus on converting patterns from high-performing segments to low-performing ones"
      );
    }

    return insights;
  }

  async generateSegmentationReports(segmentationResult) {
    // Generate comprehensive segmentation reports
    const report = {
      reportId: this.generateReportId(),
      cohortId: segmentationResult.cohortId,
      reportType: "segmentation_analysis",
      generatedAt: Date.now(),
      summary: {
        totalSegments: segmentationResult.totalSegments,
        methodology: segmentationResult.methodology,
        keyFindings: [],
        recommendations: [],
      },
      data: segmentationResult,
    };

    // Add key findings from insights
    if (segmentationResult.insights.keyFindings) {
      report.summary.keyFindings = segmentationResult.insights.keyFindings;
    }

    // Add recommendations from insights
    if (segmentationResult.insights.recommendations) {
      report.summary.recommendations =
        segmentationResult.insights.recommendations;
    }

    return report;
  }

  async optimizeSegmentationPerformance(cohort, segmentationResult) {
    // Optimize segmentation performance
    const optimizations = {
      cacheStrategy: "aggressive",
      updateFrequency: "daily",
      performanceImprovements: [],
    };

    // Optimize based on cohort size
    if (cohort.users.size > 5000) {
      optimizations.cacheStrategy = "aggressive";
      optimizations.performanceImprovements.push(
        "Large cohort - using aggressive caching"
      );
    }

    // Optimize based on segment count
    if (segmentationResult.totalSegments > 10) {
      optimizations.updateFrequency = "hourly";
      optimizations.performanceImprovements.push(
        "Many segments - increasing update frequency"
      );
    }

    return optimizations;
  }

  async validateSegmentationAccuracy(cohort, segmentationResult) {
    // Validate segmentation accuracy
    const validation = {
      accurate: true,
      issues: [],
      coverage: 0,
      overlap: 0,
    };

    // Check coverage (what percentage of users are in at least one segment)
    const allSegmentedUsers = new Set();
    Object.values(segmentationResult.segments).forEach((segment) => {
      segment.users.forEach((userId) => allSegmentedUsers.add(userId));
    });

    validation.coverage = allSegmentedUsers.size / cohort.users.size;

    if (validation.coverage < 0.8) {
      validation.issues.push(
        "Low segment coverage - many users not categorized"
      );
    }

    // Check for excessive overlap
    const segments = Object.values(segmentationResult.segments);
    let overlapCount = 0;
    for (let i = 0; i < segments.length; i++) {
      for (let j = i + 1; j < segments.length; j++) {
        const intersection = segments[i].users.filter((user) =>
          segments[j].users.includes(user)
        );
        overlapCount += intersection.length;
      }
    }

    validation.overlap = overlapCount / allSegmentedUsers.size;

    if (validation.overlap > 0.3) {
      validation.issues.push("High segment overlap - consider refining rules");
    }

    validation.accurate = validation.issues.length === 0;
    return validation;
  }

  async updateSegmentationMetrics(cohort, segmentationResult) {
    // Update segmentation metrics
    if (!cohort.analytics.segmentation) {
      cohort.analytics.segmentation = {};
    }

    cohort.analytics.segmentation = {
      lastSegmented: segmentationResult.segmentedAt,
      totalSegments: segmentationResult.totalSegments,
      methodology: segmentationResult.methodology,
      insights: segmentationResult.insights,
      performance: segmentationResult.performance,
    };

    return cohort.analytics.segmentation;
  }

  /**
   * Additional Utility Methods
   */
  generateEventId() {
    return `event_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateReportId() {
    return `report_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  async calculateSessionCount(journey) {
    const sessionTimeout = this.journeyTracking.sessionTimeout;
    let sessionCount = 1;

    for (let i = 1; i < journey.events.length; i++) {
      const timeDiff =
        journey.events[i].timestamp - journey.events[i - 1].timestamp;
      if (timeDiff > sessionTimeout) {
        sessionCount++;
      }
    }

    return sessionCount;
  }

  async calculateTotalDuration(journey) {
    if (journey.events.length === 0) return 0;
    return journey.lastActivity - journey.startedAt;
  }

  async countConversionEvents(journey) {
    return journey.events.filter(
      (e) =>
        e.eventType === "conversion" ||
        e.eventType === "purchase" ||
        e.eventType === "signup"
    ).length;
  }

  async calculateEngagementScore(journey) {
    let score = 0;
    journey.events.forEach((event) => {
      switch (event.eventType) {
        case "page_view":
          score += 1;
          break;
        case "click":
          score += 2;
          break;
        case "form_submit":
          score += 5;
          break;
        case "purchase":
        case "conversion":
          score += 10;
          break;
        default:
          score += 1;
      }
    });
    return score;
  }

  async checkInclusionCriteria(criteria, userContext) {
    // Check if user meets inclusion criteria
    const meetsResult = { meets: true, reason: "" };

    // Example criteria checking
    if (criteria.minAge && userContext.age < criteria.minAge) {
      meetsResult.meets = false;
      meetsResult.reason = `Age ${userContext.age} below minimum ${criteria.minAge}`;
    }

    if (
      criteria.requiredSource &&
      userContext.source !== criteria.requiredSource
    ) {
      meetsResult.meets = false;
      meetsResult.reason = `Source ${userContext.source} not in required list`;
    }

    return meetsResult;
  }

  async checkExclusionCriteria(criteria, userContext) {
    // Check if user meets exclusion criteria (should be excluded)
    const meetsResult = { meets: false, reason: "" };

    // Example exclusion checking
    if (criteria.excludeTestUsers && userContext.isTestUser) {
      meetsResult.meets = true;
      meetsResult.reason = "Test user excluded";
    }

    if (
      criteria.excludeSources &&
      criteria.excludeSources.includes(userContext.source)
    ) {
      meetsResult.meets = true;
      meetsResult.reason = `Source ${userContext.source} is excluded`;
    }

    return meetsResult;
  }

  async checkTimeWindow(timeWindow) {
    const now = Date.now();
    return now >= timeWindow.start && now <= timeWindow.end;
  }

  async updateDemographicAnalytics(demographics, newDemographics) {
    // Update demographic analytics
    Object.keys(newDemographics).forEach((key) => {
      if (!demographics[key]) {
        demographics[key] = {};
      }
      const value = newDemographics[key];
      demographics[key][value] = (demographics[key][value] || 0) + 1;
    });
    return demographics;
  }

  async segmentUsersBySource(cohort) {
    // Segment users by acquisition source
    const segments = new Map();

    for (const userId of cohort.users) {
      const journey = this.getUserJourney(userId);
      if (journey && journey.events.length > 0) {
        const source = journey.events[0].source || "unknown";
        if (!segments.has(source)) {
          segments.set(source, []);
        }
        segments.get(source).push(userId);
      }
    }

    return segments;
  }

  async segmentUsersByEngagement(cohort) {
    // Segment users by engagement level
    const segments = new Map([
      ["low", []],
      ["medium", []],
      ["high", []],
    ]);

    for (const userId of cohort.users) {
      const journey = this.getUserJourney(userId);
      if (journey) {
        const engagementScore = await this.calculateEngagementScore(journey);
        let level = "low";

        if (engagementScore > 50) level = "high";
        else if (engagementScore > 20) level = "medium";

        segments.get(level).push(userId);
      }
    }

    return segments;
  }

  async calculateSegmentRetention(users, period) {
    // Calculate retention for a specific user segment
    const cutoffDate = Date.now() - period * 24 * 60 * 60 * 1000;
    let retainedUsers = 0;

    for (const userId of users) {
      const isRetained = await this.isUserRetained(userId, cutoffDate);
      if (isRetained) {
        retainedUsers++;
      }
    }

    return users.length > 0 ? retainedUsers / users.length : 0;
  }

  async evaluateSegmentationRule(rule, userId, userJourney) {
    // Evaluate if user meets segmentation rule
    switch (rule.type) {
      case "behavioral":
        return await this.evaluateBehavioralRule(rule, userJourney);
      case "demographic":
        return await this.evaluateDemographicRule(rule, userId);
      case "temporal":
        return await this.evaluateTemporalRule(rule, userJourney);
      case "custom":
        return await this.evaluateCustomRule(rule, userId, userJourney);
      default:
        return false;
    }
  }

  async evaluateBehavioralRule(rule, userJourney) {
    if (!userJourney) return false;

    switch (rule.criteria.engagement) {
      case "high":
        const engagementScore = await this.calculateEngagementScore(
          userJourney
        );
        return engagementScore > 50;
      case "medium":
        const mediumScore = await this.calculateEngagementScore(userJourney);
        return mediumScore > 20 && mediumScore <= 50;
      case "low":
        const lowScore = await this.calculateEngagementScore(userJourney);
        return lowScore <= 20;
      default:
        return false;
    }
  }

  async evaluateDemographicRule(rule, userId) {
    // Evaluate demographic rules using user profile data
    try {
      const userProfile = await this.getUserProfile(userId);
      if (!userProfile) return false;

      switch (rule.operator) {
        case "equals":
          return userProfile[rule.field] === rule.value;
        case "not_equals":
          return userProfile[rule.field] !== rule.value;
        case "in":
          return (
            Array.isArray(rule.value) &&
            rule.value.includes(userProfile[rule.field])
          );
        case "not_in":
          return (
            Array.isArray(rule.value) &&
            !rule.value.includes(userProfile[rule.field])
          );
        case "greater_than":
          return parseFloat(userProfile[rule.field]) > parseFloat(rule.value);
        case "less_than":
          return parseFloat(userProfile[rule.field]) < parseFloat(rule.value);
        case "contains":
          return String(userProfile[rule.field])
            .toLowerCase()
            .includes(String(rule.value).toLowerCase());
        default:
          console.warn(`Unknown demographic rule operator: ${rule.operator}`);
          return false;
      }
    } catch (error) {
      console.error("Error evaluating demographic rule:", error);
      return false;
    }
  }

  async evaluateTemporalRule(rule, userJourney) {
    if (!userJourney) return false;

    const daysSinceStart =
      (Date.now() - userJourney.startedAt) / (24 * 60 * 60 * 1000);

    if (rule.criteria.recency) {
      return daysSinceStart <= rule.criteria.recency;
    }

    return false;
  }

  async evaluateCustomRule(rule, userId, userJourney) {
    // Evaluate custom rules with flexible business logic
    try {
      const context = {
        userId,
        userJourney,
        userProfile: await this.getUserProfile(userId),
        timestamp: Date.now(),
        cohortTracker: this,
      };

      // If rule has a custom function, execute it
      if (rule.customFunction && typeof rule.customFunction === "function") {
        return await rule.customFunction(context);
      }

      // If rule has JavaScript code to evaluate
      if (rule.jsCode) {
        // Create a safe evaluation context
        const safeEval = new Function(
          "context",
          "userJourney",
          "userProfile",
          rule.jsCode
        );
        return safeEval(context, userJourney, context.userProfile);
      }

      // Handle predefined custom rule types
      switch (rule.type) {
        case "journey_completion_rate":
          return this.evaluateJourneyCompletionRate(
            userJourney,
            rule.threshold
          );
        case "engagement_score":
          return this.evaluateEngagementScore(
            userId,
            userJourney,
            rule.minScore
          );
        case "feature_usage":
          return this.evaluateFeatureUsage(
            userJourney,
            rule.features,
            rule.minUsageCount
          );
        case "retention_likelihood":
          return this.evaluateRetentionLikelihood(
            userId,
            userJourney,
            rule.minProbability
          );
        default:
          console.warn(`Unknown custom rule type: ${rule.type}`);
          return true; // Default to true for unknown rules
      }
    } catch (error) {
      console.error("Error evaluating custom rule:", error);
      return false;
    }
  }

  // Advanced analytics classes for cohort analysis
  RetentionAnalyzer = class {
    constructor(cohort) {
      this.cohort = cohort;
      this.retentionData = new Map();
      this.metrics = {
        dailyRetention: new Map(),
        weeklyRetention: new Map(),
        monthlyRetention: new Map(),
        cohortLifetime: 0,
      };
    }

    async calculateRetentionCurve(timeWindow = "30d") {
      const users = this.cohort.users;
      const retentionCurve = [];
      const startDate = new Date(this.cohort.createdAt);
      const days = this.parseDuration(timeWindow);

      for (let day = 0; day <= days; day++) {
        const targetDate = new Date(startDate);
        targetDate.setDate(startDate.getDate() + day);

        let retainedUsers = 0;
        for (const userId of users.keys()) {
          const userJourney = users.get(userId).journey;
          if (this.isUserActiveOnDate(userJourney, targetDate)) {
            retainedUsers++;
          }
        }

        const retentionRate = users.size > 0 ? retainedUsers / users.size : 0;
        retentionCurve.push({
          day,
          date: targetDate.toISOString().split("T")[0],
          retainedUsers,
          totalUsers: users.size,
          retentionRate: Math.round(retentionRate * 10000) / 100,
        });
      }

      return retentionCurve;
    }

    parseDuration(duration) {
      const match = duration.match(/(\d+)([dwmy])/);
      if (!match) return 30;

      const [, num, unit] = match;
      const multipliers = { d: 1, w: 7, m: 30, y: 365 };
      return parseInt(num) * (multipliers[unit] || 1);
    }

    isUserActiveOnDate(userJourney, targetDate) {
      if (!userJourney || !userJourney.events) return false;

      const targetTime = targetDate.getTime();
      return userJourney.events.some((event) => {
        const eventTime = new Date(event.timestamp).getTime();
        const dayStart = new Date(targetDate).setHours(0, 0, 0, 0);
        const dayEnd = new Date(targetDate).setHours(23, 59, 59, 999);
        return eventTime >= dayStart && eventTime <= dayEnd;
      });
    }
  };

  PatternAnalyzer = class {
    constructor(cohort) {
      this.cohort = cohort;
      this.patterns = new Map();
      this.insights = [];
    }

    async identifyBehavioralPatterns() {
      const patterns = {
        common_sequences: await this.findCommonSequences(),
        user_segments: await this.identifyUserSegments(),
        conversion_paths: await this.analyzeConversionPaths(),
        dropout_points: await this.identifyDropoutPoints(),
        engagement_patterns: await this.analyzeEngagementPatterns(),
      };

      this.patterns.set("behavioral", patterns);
      return patterns;
    }

    async findCommonSequences() {
      const sequences = new Map();
      const users = this.cohort.users;

      for (const [userId, userData] of users) {
        const events = userData.journey?.events || [];
        const eventSequence = events.map((e) => e.type).slice(0, 10); // First 10 events

        for (let len = 2; len <= Math.min(5, eventSequence.length); len++) {
          for (let i = 0; i <= eventSequence.length - len; i++) {
            const sequence = eventSequence.slice(i, i + len).join(" -> ");
            sequences.set(sequence, (sequences.get(sequence) || 0) + 1);
          }
        }
      }

      return Array.from(sequences.entries())
        .sort((a, b) => b[1] - a[1])
        .slice(0, 20)
        .map(([sequence, count]) => ({
          sequence,
          count,
          frequency: count / users.size,
        }));
    }

    async identifyUserSegments() {
      const segments = {
        highly_engaged: [],
        moderately_engaged: [],
        low_engaged: [],
        churned: [],
      };

      for (const [userId, userData] of this.cohort.users) {
        const engagementScore = this.calculateEngagementScore(userData.journey);
        const daysSinceLastActivity = this.getDaysSinceLastActivity(
          userData.journey
        );

        if (daysSinceLastActivity > 14) {
          segments.churned.push(userId);
        } else if (engagementScore > 0.8) {
          segments.highly_engaged.push(userId);
        } else if (engagementScore > 0.4) {
          segments.moderately_engaged.push(userId);
        } else {
          segments.low_engaged.push(userId);
        }
      }

      return segments;
    }

    calculateEngagementScore(journey) {
      if (!journey || !journey.events) return 0;

      const events = journey.events;
      const uniqueDays = new Set(
        events.map((e) => new Date(e.timestamp).toDateString())
      ).size;

      const avgEventsPerDay = events.length / Math.max(uniqueDays, 1);
      const recency = this.getDaysSinceLastActivity(journey);

      // Normalize score between 0 and 1
      const activityScore = Math.min(avgEventsPerDay / 10, 1);
      const recencyScore = Math.max(0, 1 - recency / 30);

      return activityScore * 0.7 + recencyScore * 0.3;
    }

    getDaysSinceLastActivity(journey) {
      if (!journey || !journey.events || journey.events.length === 0)
        return 999;

      const lastEvent = journey.events[journey.events.length - 1];
      const daysSince =
        (Date.now() - new Date(lastEvent.timestamp).getTime()) /
        (1000 * 60 * 60 * 24);
      return Math.floor(daysSince);
    }

    async analyzeConversionPaths() {
      const conversionPaths = new Map();

      for (const [userId, userData] of this.cohort.users) {
        if (userData.converted) {
          const events = userData.journey?.events || [];
          const pathToConversion = events
            .filter((e) => e.timestamp <= userData.conversionTimestamp)
            .map((e) => e.type)
            .join(" -> ");

          conversionPaths.set(
            pathToConversion,
            (conversionPaths.get(pathToConversion) || 0) + 1
          );
        }
      }

      return Array.from(conversionPaths.entries())
        .sort((a, b) => b[1] - a[1])
        .slice(0, 10)
        .map(([path, count]) => ({
          path,
          count,
          conversionRate: count / this.cohort.users.size,
        }));
    }

    async identifyDropoutPoints() {
      const dropoutPoints = new Map();

      for (const [userId, userData] of this.cohort.users) {
        const events = userData.journey?.events || [];
        if (events.length > 1) {
          const lastEventType = events[events.length - 1].type;
          if (!userData.converted) {
            dropoutPoints.set(
              lastEventType,
              (dropoutPoints.get(lastEventType) || 0) + 1
            );
          }
        }
      }

      return Array.from(dropoutPoints.entries())
        .sort((a, b) => b[1] - a[1])
        .map(([eventType, count]) => ({
          eventType,
          dropoutCount: count,
          dropoutRate: count / this.cohort.users.size,
        }));
    }

    async analyzeEngagementPatterns() {
      const patterns = {
        peak_hours: this.analyzeActivityByHour(),
        peak_days: this.analyzeActivityByDay(),
        session_patterns: this.analyzeSessionPatterns(),
      };

      return patterns;
    }

    analyzeActivityByHour() {
      const hourlyActivity = new Array(24).fill(0);

      for (const [userId, userData] of this.cohort.users) {
        const events = userData.journey?.events || [];
        events.forEach((event) => {
          const hour = new Date(event.timestamp).getHours();
          hourlyActivity[hour]++;
        });
      }

      return hourlyActivity.map((count, hour) => ({
        hour,
        activityCount: count,
        percentage: (count / this.getTotalEvents()) * 100,
      }));
    }

    analyzeActivityByDay() {
      const dailyActivity = new Array(7).fill(0);
      const dayNames = [
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
      ];

      for (const [userId, userData] of this.cohort.users) {
        const events = userData.journey?.events || [];
        events.forEach((event) => {
          const day = new Date(event.timestamp).getDay();
          dailyActivity[day]++;
        });
      }

      return dailyActivity.map((count, day) => ({
        day: dayNames[day],
        dayIndex: day,
        activityCount: count,
        percentage: (count / this.getTotalEvents()) * 100,
      }));
    }

    analyzeSessionPatterns() {
      let totalSessions = 0;
      let totalDuration = 0;
      const sessionLengths = [];

      for (const [userId, userData] of this.cohort.users) {
        const sessions = this.identifyUserSessions(userData.journey);
        totalSessions += sessions.length;

        sessions.forEach((session) => {
          const duration = session.endTime - session.startTime;
          totalDuration += duration;
          sessionLengths.push(duration);
        });
      }

      sessionLengths.sort((a, b) => a - b);
      const median = sessionLengths[Math.floor(sessionLengths.length / 2)] || 0;

      return {
        averageSessionsPerUser: totalSessions / this.cohort.users.size,
        averageSessionDuration: totalDuration / Math.max(totalSessions, 1),
        medianSessionDuration: median,
        totalSessions,
      };
    }

    identifyUserSessions(journey) {
      if (!journey || !journey.events) return [];

      const sessions = [];
      let currentSession = null;
      const sessionTimeout = 30 * 60 * 1000; // 30 minutes

      journey.events.forEach((event) => {
        const eventTime = new Date(event.timestamp).getTime();

        if (
          !currentSession ||
          eventTime - currentSession.lastActivity > sessionTimeout
        ) {
          if (currentSession) sessions.push(currentSession);
          currentSession = {
            startTime: eventTime,
            endTime: eventTime,
            lastActivity: eventTime,
            events: [event],
          };
        } else {
          currentSession.endTime = eventTime;
          currentSession.lastActivity = eventTime;
          currentSession.events.push(event);
        }
      });

      if (currentSession) sessions.push(currentSession);
      return sessions;
    }

    getTotalEvents() {
      let total = 0;
      for (const [userId, userData] of this.cohort.users) {
        total += userData.journey?.events?.length || 0;
      }
      return total;
    }
  };

  JourneyAnalyzer = class {
    constructor(cohort) {
      this.cohort = cohort;
    }
  };

  SegmentAnalyzer = class {
    constructor(cohort) {
      this.cohort = cohort;
    }
  };

  RetentionCollector = class {
    constructor() {
      this.data = new Map();
    }
  };

  EngagementCollector = class {
    constructor() {
      this.data = new Map();
    }
  };

  ConversionCollector = class {
    constructor() {
      this.data = new Map();
    }
  };

  JourneyCollector = class {
    constructor() {
      this.data = new Map();
    }
  };

  async findRepetitiveActions(journey) {
    // Find repetitive action patterns
    const actionCounts = {};
    journey.events.forEach((event) => {
      actionCounts[event.eventType] = (actionCounts[event.eventType] || 0) + 1;
    });

    return Object.entries(actionCounts)
      .filter(([, count]) => count >= 3)
      .map(([action, count]) => ({ action, count }));
  }

  async analyzeExplorationPattern(journey) {
    // Analyze user exploration patterns
    const uniqueEvents = new Set(journey.events.map((e) => e.eventType));
    return {
      uniqueEventTypes: uniqueEvents.size,
      explorationScore: uniqueEvents.size / Math.max(1, journey.events.length),
      breadth: uniqueEvents.size,
      depth: journey.events.length,
    };
  }

  async categorizeEngagementLevels(journey) {
    // Categorize engagement into levels
    const score = await this.calculateEngagementScore(journey);

    if (score > 50) return "high";
    if (score > 20) return "medium";
    return "low";
  }

  async analyzeHabitFormation(journey) {
    // Analyze habit formation patterns
    const habitMetrics = {
      consistency: 0,
      frequency: 0,
      patterns: [],
    };

    // Calculate consistency (events per day)
    const journeyDays = Math.max(
      1,
      (journey.lastActivity - journey.startedAt) / (24 * 60 * 60 * 1000)
    );
    habitMetrics.frequency = journey.events.length / journeyDays;

    // Consistency score based on regular activity
    if (habitMetrics.frequency > 1) habitMetrics.consistency = 0.8;
    else if (habitMetrics.frequency > 0.5) habitMetrics.consistency = 0.6;
    else habitMetrics.consistency = 0.3;

    return habitMetrics;
  }

  /**
   * Analytics and Reporting
   */
  getCohort(cohortId) {
    return this.cohorts.get(cohortId);
  }

  getUserJourney(userId) {
    const journeyId = this.getUserJourneyId(userId);
    return this.userJourneys.get(journeyId);
  }

  getRetentionData(cohortId) {
    return this.retentionData.get(cohortId);
  }

  getCohortMetrics() {
    return { ...this.cohortMetrics };
  }

  getActiveCohorts() {
    return Array.from(this.cohorts.values()).filter(
      (c) => c.status === "active"
    );
  }

  calculateAverageRetentionRate() {
    // Calculate average retention rate across all cohorts
    const retentionDataArray = Array.from(this.retentionData.values());
    if (retentionDataArray.length === 0) return 0;

    // Filter out tracking data and get only analysis data
    const analysisData = retentionDataArray.filter(
      (data) =>
        data && data.retentionRates && typeof data.retentionRates === "object"
    );

    if (analysisData.length === 0) return 0;

    // Calculate average based on 7-day retention (standard metric)
    const totalRetention = analysisData.reduce((sum, data) => {
      const day7Rate = data.retentionRates.day_7 || 0;
      return sum + day7Rate;
    }, 0);

    const averageRetention = totalRetention / analysisData.length;

    // Update cohort metrics
    this.cohortMetrics.averageRetentionRate = averageRetention;

    return averageRetention;
  }

  // Helper methods for custom rule evaluation
  evaluateJourneyCompletionRate(userJourney, threshold) {
    if (!userJourney || !userJourney.events) return false;

    const totalSteps = userJourney.expectedSteps || 10; // Default expected journey steps
    const completedSteps = new Set(userJourney.events.map((e) => e.type)).size;
    const completionRate = completedSteps / totalSteps;

    return completionRate >= threshold;
  }

  evaluateEngagementScore(userId, userJourney, minScore) {
    if (!userJourney || !userJourney.events) return false;

    // Calculate engagement based on activity frequency and diversity
    const events = userJourney.events;
    const uniqueEventTypes = new Set(events.map((e) => e.type)).size;
    const avgDailyEvents =
      events.length / Math.max(this.getDaysSinceJoinCohort(userId), 1);

    // Normalize engagement score (0-1)
    const diversityScore = Math.min(uniqueEventTypes / 10, 1); // Up to 10 event types
    const activityScore = Math.min(avgDailyEvents / 5, 1); // Up to 5 events per day
    const engagementScore = diversityScore * 0.4 + activityScore * 0.6;

    return engagementScore >= minScore;
  }

  evaluateFeatureUsage(userJourney, features, minUsageCount) {
    if (!userJourney || !userJourney.events) return false;

    const featureUsage = new Map();
    userJourney.events.forEach((event) => {
      if (features.includes(event.type)) {
        featureUsage.set(event.type, (featureUsage.get(event.type) || 0) + 1);
      }
    });

    return Array.from(featureUsage.values()).some(
      (count) => count >= minUsageCount
    );
  }

  evaluateRetentionLikelihood(userId, userJourney, minProbability) {
    if (!userJourney || !userJourney.events) return false;

    // Simple retention likelihood based on recent activity and engagement
    const daysSinceLastActivity = this.getDaysSinceLastActivity(userJourney);
    const engagementScore = this.calculateUserEngagementScore(userJourney);
    const sessionFrequency = this.calculateSessionFrequency(userJourney);

    // Combine factors to estimate retention likelihood
    const recencyScore = Math.max(0, 1 - daysSinceLastActivity / 7); // Within a week
    const retentionLikelihood =
      recencyScore * 0.4 + engagementScore * 0.4 + sessionFrequency * 0.2;

    return retentionLikelihood >= minProbability;
  }

  getDaysSinceJoinCohort(userId) {
    const user = this.cohorts.get(this.currentCohortId)?.users?.get(userId);
    if (!user) return 1;

    const joinDate = new Date(user.joinedAt);
    const daysSince = (Date.now() - joinDate.getTime()) / (1000 * 60 * 60 * 24);
    return Math.max(daysSince, 1);
  }

  getDaysSinceLastActivity(userJourney) {
    if (!userJourney || !userJourney.events || userJourney.events.length === 0)
      return 999;

    const lastEvent = userJourney.events[userJourney.events.length - 1];
    const daysSince =
      (Date.now() - new Date(lastEvent.timestamp).getTime()) /
      (1000 * 60 * 60 * 24);
    return Math.floor(daysSince);
  }

  calculateUserEngagementScore(userJourney) {
    if (!userJourney || !userJourney.events) return 0;

    const events = userJourney.events;
    const uniqueDays = new Set(
      events.map((e) => new Date(e.timestamp).toDateString())
    ).size;

    const avgEventsPerDay = events.length / Math.max(uniqueDays, 1);
    const uniqueEventTypes = new Set(events.map((e) => e.type)).size;

    // Normalize scores
    const activityScore = Math.min(avgEventsPerDay / 10, 1);
    const diversityScore = Math.min(uniqueEventTypes / 15, 1);

    return activityScore * 0.6 + diversityScore * 0.4;
  }

  calculateSessionFrequency(userJourney) {
    if (!userJourney || !userJourney.events) return 0;

    const sessions = this.identifyUserSessions(userJourney);
    const totalDays = Math.max(
      this.getDaysSinceJoinCohort(userJourney.userId || "unknown"),
      1
    );
    const sessionsPerDay = sessions.length / totalDays;

    // Normalize to 0-1 scale (assuming 2+ sessions per day is very active)
    return Math.min(sessionsPerDay / 2, 1);
  }

  identifyUserSessions(userJourney) {
    if (!userJourney || !userJourney.events) return [];

    const sessions = [];
    let currentSession = null;
    const sessionTimeout = 30 * 60 * 1000; // 30 minutes

    userJourney.events.forEach((event) => {
      const eventTime = new Date(event.timestamp).getTime();

      if (
        !currentSession ||
        eventTime - currentSession.lastActivity > sessionTimeout
      ) {
        if (currentSession) sessions.push(currentSession);
        currentSession = {
          startTime: eventTime,
          endTime: eventTime,
          lastActivity: eventTime,
          events: [event],
        };
      } else {
        currentSession.endTime = eventTime;
        currentSession.lastActivity = eventTime;
        currentSession.events.push(event);
      }
    });

    if (currentSession) sessions.push(currentSession);
    return sessions;
  }

  async getUserProfile(userId) {
    // In a real implementation, this would fetch from user service
    // For now, return a mock profile based on userId patterns
    return {
      id: userId,
      age: 25 + (userId.length % 40), // Mock age 25-65
      gender: userId.includes("f") ? "female" : "male",
      location: ["US", "UK", "CA", "DE", "FR"][userId.length % 5],
      accountType: userId.includes("premium") ? "premium" : "basic",
      signupDate: new Date(Date.now() - userId.length * 86400000), // Mock signup date
      preferences: {
        notifications: true,
        marketing: userId.includes("marketing"),
        analytics: true,
      },
    };
  }

  calculateStatisticalSignificance(metrics1, metrics2) {
    // Simple significance test - in production would use proper statistical tests
    const n1 = metrics1.sampleSize || 100;
    const n2 = metrics2.sampleSize || 100;
    const p1 = metrics1.conversionRate || 0;
    const p2 = metrics2.conversionRate || 0;

    // Simple z-test approximation
    const pooledP = (p1 * n1 + p2 * n2) / (n1 + n2);
    const se = Math.sqrt(pooledP * (1 - pooledP) * (1 / n1 + 1 / n2));
    const zScore = Math.abs(p1 - p2) / se;

    // Convert z-score to approximate p-value
    const pValue = 2 * (1 - this.normalCDF(Math.abs(zScore)));

    return {
      zScore: this.roundToPrecision(zScore, 3),
      pValue: this.roundToPrecision(pValue, 4),
      isSignificant: pValue < 0.05,
      confidenceLevel: pValue < 0.01 ? 0.99 : pValue < 0.05 ? 0.95 : 0.9,
    };
  }

  normalCDF(x) {
    // Approximation of the cumulative distribution function for standard normal
    return 0.5 * (1 + this.erf(x / Math.sqrt(2)));
  }

  erf(x) {
    // Approximation of the error function
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
}

export default CohortTracker;
