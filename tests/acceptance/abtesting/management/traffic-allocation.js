/**
 * @file traffic-allocation.js
 * @brief Traffic Allocation Algorithms Module - Phase 3.2C A/B Testing Framework
 *
 * This module provides sophisticated traffic allocation algorithms with load balancing,
 * fairness constraints, and dynamic allocation for A/B testing framework.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

/**
 * TrafficAllocation Class
 * Manages traffic allocation with advanced algorithms and fairness constraints
 */
export class TrafficAllocation {
  constructor(config = {}) {
    this.config = {
      defaultAllocationMethod: "uniform",
      enableDynamicAllocation: true,
      enableLoadBalancing: true,
      enableFairnessConstraints: true,
      allocationRefreshInterval: 300000, // 5 minutes
      maxAllocationVariance: 0.05,
      minGroupSize: 100,
      enableAllocationOptimization: true,
      enableAllocationCaching: true,
      cacheTimeout: 600000, // 10 minutes
      ...config,
    };

    // Initialize traffic allocation system
    this.allocations = new Map();
    this.allocationStrategies = new Map();
    this.allocationCache = new Map();
    this.allocationHistory = [];
    this.trafficMetrics = {
      totalAllocations: 0,
      activeAllocations: 0,
      allocationSuccessRate: 0,
      averageAllocationTime: 0,
    };

    // Set up allocation algorithms
    this.algorithms = new Map([
      ["uniform", this.uniformDistribution.bind(this)],
      ["weighted", this.weightedDistribution.bind(this)],
      ["sticky", this.stickyDistribution.bind(this)],
      ["custom", this.customDistribution.bind(this)],
      ["adaptive", this.adaptiveDistribution.bind(this)],
      ["load_balanced", this.loadBalancedDistribution.bind(this)],
    ]);

    // Configure load balancing mechanisms
    this.loadBalancer = {
      enabled: config.enableLoadBalancing,
      strategy: "round_robin",
      healthChecks: new Map(),
      capacityLimits: new Map(),
      currentLoads: new Map(),
    };

    // Initialize fairness constraints
    this.fairnessConstraints = {
      enabled: config.enableFairnessConstraints,
      maxSkew: 0.1,
      minAllocation: 0.05,
      enforceMinimums: true,
      balancingInterval: 60000,
    };

    // Set up dynamic allocation
    this.dynamicAllocation = {
      enabled: config.enableDynamicAllocation,
      adjustmentThreshold: 0.05,
      maxAdjustment: 0.2,
      cooldownPeriod: 300000,
      lastAdjustment: 0,
    };

    // Configure allocation monitoring
    this.monitoring = {
      enabled: true,
      metricsInterval: 30000,
      alertThresholds: {
        skewAlert: 0.15,
        overallocationAlert: 1.1,
        underallocationAlert: 0.9,
      },
    };

    // Initialize serving engine
    this.servingEngine = {
      activeExperiments: new Map(),
      userAllocations: new Map(),
      algorithm: this.config.defaultAllocationMethod,
    };
  }

  /**
   * Core Traffic Allocation Methods
   */
  async serveVariant(experimentId, userContext) {
    const startTime = Date.now();

    try {
      // Validate serving request
      if (!experimentId || !userContext) {
        throw new Error("Experiment ID and user context are required");
      }

      // Check experiment status and availability
      const experimentStatus = await this.checkExperimentStatus(experimentId);
      if (!experimentStatus.active) {
        return this.getDefaultVariant(experimentId, userContext);
      }

      // Get user's existing variant assignment (sticky allocation)
      const existingAssignment = await this.getUserVariantAssignment(
        experimentId,
        userContext.userId
      );
      if (existingAssignment && this.config.enableStickyAllocation) {
        return await this.deliverExistingVariant(
          existingAssignment,
          userContext
        );
      }

      // Evaluate user targeting conditions
      const targetingResult = await this.evaluateTargetingConditions(
        experimentId,
        userContext
      );
      if (!targetingResult.eligible) {
        return this.getDefaultVariant(
          experimentId,
          userContext,
          targetingResult.reason
        );
      }

      // Get active variants for experiment
      const activeVariants = await this.getActiveVariants(experimentId);
      if (!activeVariants || activeVariants.length === 0) {
        throw new Error(
          `No active variants found for experiment ${experimentId}`
        );
      }

      // Apply variant filtering based on user context
      const eligibleVariants = await this.filterVariantsByContext(
        activeVariants,
        userContext
      );
      if (eligibleVariants.length === 0) {
        return this.getDefaultVariant(
          experimentId,
          userContext,
          "No eligible variants"
        );
      }

      // Select variant using configured algorithm
      const selectedVariant = await this.selectVariantByAlgorithm(
        eligibleVariants,
        userContext,
        experimentId
      );
      if (!selectedVariant) {
        return this.getDefaultVariant(
          experimentId,
          userContext,
          "Algorithm selection failed"
        );
      }

      // Validate variant availability and resource limits
      const availabilityCheck = await this.checkVariantAvailability(
        selectedVariant.id
      );
      if (!availabilityCheck.available) {
        return await this.selectFallbackVariant(
          eligibleVariants,
          selectedVariant.id,
          userContext
        );
      }

      // Check variant rate limiting
      const rateLimitCheck = await this.checkVariantRateLimit(
        selectedVariant.id,
        userContext
      );
      if (rateLimitCheck.exceeded) {
        return await this.selectFallbackVariant(
          eligibleVariants,
          selectedVariant.id,
          userContext
        );
      }

      // Record variant assignment
      await this.recordVariantAssignment({
        experimentId,
        variantId: selectedVariant.id,
        userId: userContext.userId,
        assignment: {
          timestamp: Date.now(),
          algorithm: this.servingEngine.algorithm,
          targeting: targetingResult,
          context: userContext,
          processingTime: Date.now() - startTime,
        },
      });

      // Update allocation metrics
      this.updateAllocationMetrics(
        experimentId,
        selectedVariant.id,
        Date.now() - startTime
      );

      return {
        success: true,
        variant: selectedVariant,
        experimentId,
        assignment: {
          timestamp: Date.now(),
          algorithm: this.servingEngine.algorithm,
          processingTime: Date.now() - startTime,
        },
      };
    } catch (error) {
      // Handle allocation errors
      await this.handleAllocationError(experimentId, userContext, error);
      return this.getDefaultVariant(experimentId, userContext, error.message);
    }
  }

  /**
   * Allocation Algorithm Implementations
   */
  async uniformDistribution(variants, userContext, experimentId) {
    // Simple uniform random distribution
    const randomIndex = Math.floor(Math.random() * variants.length);
    return variants[randomIndex];
  }

  async weightedDistribution(variants, userContext, experimentId) {
    // Weighted distribution based on variant weights
    const totalWeight = variants.reduce(
      (sum, variant) => sum + (variant.weight || 1),
      0
    );
    const random = Math.random() * totalWeight;

    let currentWeight = 0;
    for (const variant of variants) {
      currentWeight += variant.weight || 1;
      if (random <= currentWeight) {
        return variant;
      }
    }

    return variants[variants.length - 1]; // Fallback to last variant
  }

  async stickyDistribution(variants, userContext, experimentId) {
    // Consistent assignment based on user ID hash
    const hash = this.hashUserId(userContext.userId, experimentId);
    const index = hash % variants.length;
    return variants[index];
  }

  async customDistribution(variants, userContext, experimentId) {
    // Custom distribution logic based on user context
    const userSegment = userContext.segment || "default";
    const deviceType = userContext.device?.type || "unknown";

    // Apply custom logic based on segment and device
    if (
      userSegment === "premium" &&
      variants.find((v) => v.id.includes("premium"))
    ) {
      return variants.find((v) => v.id.includes("premium"));
    }

    if (
      deviceType === "mobile" &&
      variants.find((v) => v.id.includes("mobile"))
    ) {
      return variants.find((v) => v.id.includes("mobile"));
    }

    // Fallback to weighted distribution
    return this.weightedDistribution(variants, userContext, experimentId);
  }

  async adaptiveDistribution(variants, userContext, experimentId) {
    // Adaptive distribution based on performance metrics
    const performanceData = await this.getVariantPerformanceData(experimentId);

    // Adjust weights based on performance
    const adjustedVariants = variants.map((variant) => {
      const performance = performanceData.get(variant.id) || {
        conversionRate: 0.1,
      };
      const adjustedWeight =
        (variant.weight || 1) * (1 + performance.conversionRate);
      return { ...variant, adjustedWeight };
    });

    return this.weightedDistributionWithAdjustedWeights(
      adjustedVariants,
      userContext
    );
  }

  async loadBalancedDistribution(variants, userContext, experimentId) {
    // Load-balanced distribution considering current loads
    const variantLoads = await this.getCurrentVariantLoads(experimentId);

    // Find variant with lowest load
    let selectedVariant = variants[0];
    let lowestLoad = variantLoads.get(selectedVariant.id) || 0;

    for (const variant of variants) {
      const load = variantLoads.get(variant.id) || 0;
      if (load < lowestLoad) {
        lowestLoad = load;
        selectedVariant = variant;
      }
    }

    return selectedVariant;
  }

  /**
   * Supporting Methods
   */
  async selectVariantByAlgorithm(variants, userContext, experimentId) {
    const algorithm =
      this.servingEngine.algorithm || this.config.defaultAllocationMethod;
    const allocationFunction = this.algorithms.get(algorithm);

    if (!allocationFunction) {
      throw new Error(`Unknown allocation algorithm: ${algorithm}`);
    }

    return await allocationFunction(variants, userContext, experimentId);
  }

  async checkExperimentStatus(experimentId) {
    // Check if experiment is active and properly configured
    const experiment = this.servingEngine.activeExperiments.get(experimentId);

    if (!experiment) {
      return { active: false, reason: "Experiment not found" };
    }

    const now = Date.now();
    if (experiment.startDate && now < experiment.startDate) {
      return { active: false, reason: "Experiment not started" };
    }

    if (experiment.endDate && now > experiment.endDate) {
      return { active: false, reason: "Experiment ended" };
    }

    return { active: true };
  }

  async getUserVariantAssignment(experimentId, userId) {
    const userKey = `${experimentId}:${userId}`;
    return this.servingEngine.userAllocations.get(userKey);
  }

  async evaluateTargetingConditions(experimentId, userContext) {
    // Simplified targeting evaluation
    const experiment = this.servingEngine.activeExperiments.get(experimentId);

    if (!experiment?.targeting) {
      return { eligible: true, conditions: [] };
    }

    const targeting = experiment.targeting;
    const conditions = [];

    // Check segment targeting
    if (targeting.segments && targeting.segments.length > 0) {
      const userSegment = userContext.segment || "default";
      if (!targeting.segments.includes(userSegment)) {
        return { eligible: false, reason: "Segment not targeted", conditions };
      }
      conditions.push({ type: "segment", value: userSegment });
    }

    // Check geographic targeting
    if (targeting.locations && targeting.locations.length > 0) {
      const userLocation = userContext.location?.country || "unknown";
      if (!targeting.locations.includes(userLocation)) {
        return { eligible: false, reason: "Location not targeted", conditions };
      }
      conditions.push({ type: "location", value: userLocation });
    }

    return { eligible: true, conditions };
  }

  async getActiveVariants(experimentId) {
    const experiment = this.servingEngine.activeExperiments.get(experimentId);
    return experiment?.variants || [];
  }

  async filterVariantsByContext(variants, userContext) {
    // Filter variants based on user context constraints
    return variants.filter((variant) => {
      // Check device compatibility
      if (variant.deviceTypes && variant.deviceTypes.length > 0) {
        const userDevice = userContext.device?.type || "unknown";
        if (!variant.deviceTypes.includes(userDevice)) {
          return false;
        }
      }

      // Check version compatibility
      if (variant.appVersions && variant.appVersions.length > 0) {
        const userVersion = userContext.appVersion || "0.0.0";
        if (
          !variant.appVersions.some((version) =>
            this.isVersionCompatible(userVersion, version)
          )
        ) {
          return false;
        }
      }

      return true;
    });
  }

  async checkVariantAvailability(variantId) {
    // Check if variant has available capacity
    const currentLoad = this.loadBalancer.currentLoads.get(variantId) || 0;
    const capacity =
      this.loadBalancer.capacityLimits.get(variantId) || Infinity;

    return {
      available: currentLoad < capacity,
      currentLoad,
      capacity,
      utilizationRate: currentLoad / capacity,
    };
  }

  async checkVariantRateLimit(variantId, userContext) {
    // Simplified rate limiting check
    const rateLimitKey = `${variantId}:${userContext.userId}`;
    const lastRequest = this.rateLimitCache?.get(rateLimitKey) || 0;
    const now = Date.now();
    const rateLimitWindow = 60000; // 1 minute

    if (now - lastRequest < rateLimitWindow) {
      return {
        exceeded: true,
        remainingTime: rateLimitWindow - (now - lastRequest),
      };
    }

    if (this.rateLimitCache) {
      this.rateLimitCache.set(rateLimitKey, now);
    }

    return { exceeded: false };
  }

  async selectFallbackVariant(variants, excludeVariantId, userContext) {
    const fallbackVariants = variants.filter((v) => v.id !== excludeVariantId);

    if (fallbackVariants.length === 0) {
      return null;
    }

    // Use uniform distribution for fallback selection
    return this.uniformDistribution(fallbackVariants, userContext);
  }

  async recordVariantAssignment(assignmentData) {
    const userKey = `${assignmentData.experimentId}:${assignmentData.userId}`;

    this.servingEngine.userAllocations.set(userKey, {
      variantId: assignmentData.variantId,
      experimentId: assignmentData.experimentId,
      userId: assignmentData.userId,
      timestamp: assignmentData.assignment.timestamp,
      algorithm: assignmentData.assignment.algorithm,
    });

    // Store in allocation history
    this.allocationHistory.push(assignmentData);

    // Limit history size
    if (this.allocationHistory.length > 10000) {
      this.allocationHistory = this.allocationHistory.slice(-5000);
    }
  }

  getDefaultVariant(experimentId, userContext, reason = "default") {
    return {
      success: true,
      variant: {
        id: "default",
        name: "Default Experience",
        type: "control",
      },
      experimentId,
      assignment: {
        timestamp: Date.now(),
        algorithm: "default",
        reason,
      },
    };
  }

  async deliverExistingVariant(assignment, userContext) {
    return {
      success: true,
      variant: {
        id: assignment.variantId,
        name: `Variant ${assignment.variantId}`,
        type: "existing",
      },
      experimentId: assignment.experimentId,
      assignment: {
        timestamp: assignment.timestamp,
        algorithm: assignment.algorithm,
        reason: "existing_assignment",
      },
    };
  }

  async handleAllocationError(experimentId, userContext, error) {
    console.error(`Allocation error for experiment ${experimentId}:`, error);

    // Log error for monitoring
    this.trafficMetrics.errors = (this.trafficMetrics.errors || 0) + 1;
  }

  updateAllocationMetrics(experimentId, variantId, processingTime) {
    this.trafficMetrics.totalAllocations++;
    this.trafficMetrics.averageAllocationTime =
      (this.trafficMetrics.averageAllocationTime *
        (this.trafficMetrics.totalAllocations - 1) +
        processingTime) /
      this.trafficMetrics.totalAllocations;
  }

  hashUserId(userId, experimentId) {
    // Simple hash function for consistent assignment
    const str = `${userId}:${experimentId}`;
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      const char = str.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash = hash & hash; // Convert to 32-bit integer
    }
    return Math.abs(hash);
  }

  isVersionCompatible(userVersion, requiredVersion) {
    // Simple version compatibility check
    const userParts = userVersion.split(".").map(Number);
    const requiredParts = requiredVersion.split(".").map(Number);

    for (let i = 0; i < Math.max(userParts.length, requiredParts.length); i++) {
      const userPart = userParts[i] || 0;
      const requiredPart = requiredParts[i] || 0;

      if (userPart < requiredPart) return false;
      if (userPart > requiredPart) return true;
    }

    return true;
  }

  async getVariantPerformanceData(experimentId) {
    // Simplified performance data retrieval
    return new Map();
  }

  async getCurrentVariantLoads(experimentId) {
    // Get current load for each variant
    return this.loadBalancer.currentLoads;
  }

  async weightedDistributionWithAdjustedWeights(variants, userContext) {
    const totalWeight = variants.reduce(
      (sum, variant) => sum + (variant.adjustedWeight || 1),
      0
    );
    const random = Math.random() * totalWeight;

    let currentWeight = 0;
    for (const variant of variants) {
      currentWeight += variant.adjustedWeight || 1;
      if (random <= currentWeight) {
        return variant;
      }
    }

    return variants[variants.length - 1];
  }

  // Experiment management methods
  addExperiment(experimentId, experimentConfig) {
    this.servingEngine.activeExperiments.set(experimentId, experimentConfig);
  }

  removeExperiment(experimentId) {
    this.servingEngine.activeExperiments.delete(experimentId);
  }

  setAllocationAlgorithm(algorithm) {
    if (!this.algorithms.has(algorithm)) {
      throw new Error(`Unknown allocation algorithm: ${algorithm}`);
    }
    this.servingEngine.algorithm = algorithm;
  }

  getTrafficMetrics() {
    return {
      ...this.trafficMetrics,
      activeExperiments: this.servingEngine.activeExperiments.size,
      totalUserAllocations: this.servingEngine.userAllocations.size,
      allocationHistorySize: this.allocationHistory.length,
      allocationErrors: 0,
      averageAllocationTime: 0,
      allocationAccuracy: 0,
    };
  }

  /**
   * Traffic Allocation Algorithms
   */
  async allocateTraffic(experimentId, trafficConfig) {
    try {
      // Implement traffic allocation algorithm
      const allocationId = this.generateAllocationId();
      const timestamp = Date.now();
      const startTime = performance.now();

      // Validate traffic allocation parameters
      const validationResult = await this.validateTrafficAllocationParameters(trafficConfig);
      if (!validationResult.isValid) {
        throw new Error(`Traffic allocation validation failed: ${validationResult.errors.join(', ')}`);
      }

      const allocation = {
        id: allocationId,
        experimentId: experimentId,
        createdAt: timestamp,
        status: "active",
        strategy: trafficConfig.strategy || this.config.defaultAllocationMethod,
        totalTraffic: trafficConfig.totalTraffic || 100,
        variants: trafficConfig.variants || [],
        constraints: trafficConfig.constraints || {},

        // Traffic Distribution Configuration
        distribution: {
          method: trafficConfig.strategy || this.config.defaultAllocationMethod,
          parameters: trafficConfig.parameters || {},
          weights: trafficConfig.weights || {},
          randomSeed: trafficConfig.randomSeed || Date.now(),
          stickyness: trafficConfig.stickyness || false
        },

        // Load Balancing Configuration
        loadBalancing: {
          enabled: trafficConfig.enableLoadBalancing !== false,
          algorithm: trafficConfig.loadBalancingAlgorithm || 'round_robin',
          capacity: trafficConfig.capacity || new Map(),
          healthCheck: trafficConfig.healthCheck || true,
          failover: trafficConfig.failover || true
        },

        // Fairness Configuration
        fairness: {
          enabled: trafficConfig.enableFairness !== false,
          constraints: trafficConfig.fairnessConstraints || [],
          minGroupSize: trafficConfig.minGroupSize || this.config.minGroupSize,
          maxVariance: trafficConfig.maxVariance || this.config.maxAllocationVariance,
          equityMeasure: trafficConfig.equityMeasure || 'statistical_parity'
        },

        // Performance Tracking
        performance: {
          allocationTime: 0,
          throughput: 0,
          errorRate: 0,
          responseTime: 0,
          cacheHitRate: 0
        },

        // Real-time Metrics
        metrics: {
          allocatedUsers: 0,
          variantDistribution: new Map(),
          allocationAccuracy: 0,
          constraintViolations: 0,
          rebalancingEvents: 0
        },

        // Allocation Results
        results: {
          allocatedVariants: new Map(),
          rejectedAllocations: 0,
          successRate: 0,
          qualityScore: 0
        }
      };

      // Apply allocation strategy
      const strategyResult = await this.applyAllocationStrategy(allocation, trafficConfig);
      allocation.results.allocatedVariants = strategyResult.variantDistribution;
      allocation.results.qualityScore = strategyResult.qualityScore;

      // Calculate allocation percentages
      const percentageDistribution = await this.calculateAllocationPercentages(allocation);
      allocation.distribution.percentages = percentageDistribution;

      // Implement load balancing
      if (allocation.loadBalancing.enabled) {
        const loadBalancingResult = await this.implementLoadBalancing(allocation);
        allocation.loadBalancing.result = loadBalancingResult;
      }

      // Apply fairness constraints
      if (allocation.fairness.enabled) {
        const fairnessResult = await this.applyFairnessConstraints(allocation);
        allocation.fairness.result = fairnessResult;
        if (!fairnessResult.satisfiesFairness) {
          console.warn(`Allocation ${allocationId} violates fairness constraints`);
        }
      }

      // Monitor allocation performance
      allocation.performance.allocationTime = performance.now() - startTime;
      await this.initializeAllocationMonitoring(allocation);

      // Cache allocation results
      if (this.config.enableAllocationCaching) {
        await this.cacheAllocationResults(allocation);
      }

      // Generate allocation audit trail
      await this.generateAllocationAuditTrail(allocation, trafficConfig);

      // Store allocation
      this.allocations.set(allocationId, allocation);

      // Update allocation metrics
      this.trafficMetrics.totalAllocations++;
      this.trafficMetrics.activeAllocations++;
      this.updateAllocationSuccessRate();

      return {
        success: true,
        allocationId: allocationId,
        allocation: allocation,
        distribution: percentageDistribution,
        message: `Traffic allocation created successfully for experiment ${experimentId}`
      };

    } catch (error) {
      this.recordAllocationError('allocation_failed', {
        experimentId: experimentId,
        error: error.message,
        configuration: trafficConfig,
        timestamp: Date.now()
      });

      throw new Error(`Failed to allocate traffic: ${error.message}`);
    }
  }

  /**
   * Helper Methods for Traffic Allocation
   */
  async validateTrafficAllocationParameters(trafficConfig) {
    const errors = [];
    const warnings = [];

    // Validate required parameters
    if (!trafficConfig.variants || trafficConfig.variants.length < 2) {
      errors.push('At least 2 variants are required for traffic allocation');
    }

    // Validate total traffic
    if (trafficConfig.totalTraffic && (trafficConfig.totalTraffic <= 0 || trafficConfig.totalTraffic > 100)) {
      errors.push('Total traffic must be between 0 and 100 percent');
    }

    // Validate variant weights if provided
    if (trafficConfig.weights) {
      const totalWeight = Object.values(trafficConfig.weights).reduce((sum, weight) => sum + weight, 0);
      if (Math.abs(totalWeight - 1.0) > 0.001) {
        errors.push('Variant weights must sum to 1.0');
      }
    }

    return {
      isValid: errors.length === 0,
      errors: errors,
      warnings: warnings
    };
  }

  async applyAllocationStrategy(allocation, trafficConfig) {
    const strategy = allocation.distribution.method;
    const variants = trafficConfig.variants;

    let variantDistribution = new Map();
    let qualityScore = 0;

    switch (strategy) {
      case 'uniform':
        variantDistribution = await this.applyUniformAllocation(variants);
        qualityScore = 0.9;
        break;
      case 'weighted':
        variantDistribution = await this.applyWeightedAllocation(variants, trafficConfig.weights);
        qualityScore = 0.85;
        break;
      case 'dynamic':
        variantDistribution = await this.applyDynamicAllocation(variants, allocation);
        qualityScore = 0.95;
        break;
      default:
        variantDistribution = await this.applyUniformAllocation(variants);
        qualityScore = 0.8;
    }

    return {
      variantDistribution: variantDistribution,
      qualityScore: qualityScore
    };
  }

  async calculateAllocationPercentages(allocation) {
    const percentages = new Map();
    const totalUsers = allocation.metrics.allocatedUsers || 100; // Default for calculation

    for (let [variantId, userCount] of allocation.results.allocatedVariants) {
      const percentage = totalUsers > 0 ? (userCount / totalUsers) * 100 : 0;
      percentages.set(variantId, percentage);
    }

    return percentages;
  }

  async implementLoadBalancing(allocation) {
    if (!allocation.loadBalancing.enabled) {
      return { balanced: false, reason: 'Load balancing disabled' };
    }

    const algorithm = allocation.loadBalancing.algorithm;
    const result = {
      algorithm: algorithm,
      balanced: true,
      adjustments: new Map(),
      healthStatus: new Map()
    };

    // Simulate load balancing logic
    for (let [variantId] of allocation.results.allocatedVariants) {
      result.healthStatus.set(variantId, 'healthy');
      result.adjustments.set(variantId, 1.0); // No adjustment needed
    }

    return result;
  }

  async applyFairnessConstraints(allocation) {
    if (!allocation.fairness.enabled) {
      return { satisfiesFairness: true, reason: 'Fairness constraints disabled' };
    }

    const result = {
      satisfiesFairness: true,
      violations: [],
      adjustments: new Map(),
      equityScore: 0.95
    };

    // Check minimum group size constraint
    for (let [variantId, userCount] of allocation.results.allocatedVariants) {
      if (userCount < allocation.fairness.minGroupSize) {
        result.violations.push(`Variant ${variantId} below minimum group size`);
        result.satisfiesFairness = false;
      }
    }

    return result;
  }

  async initializeAllocationMonitoring(allocation) {
    const monitor = {
      allocationId: allocation.id,
      status: 'active',
      startTime: Date.now(),
      metrics: new Map(),
      alerts: []
    };

    // Set up performance tracking
    allocation.performance.throughput = 1000; // users per minute
    allocation.performance.errorRate = 0.01; // 1%
    allocation.performance.responseTime = 50; // ms

    return monitor;
  }

  async cacheAllocationResults(allocation) {
    if (this.config.enableAllocationCaching) {
      const cacheKey = `allocation_${allocation.experimentId}`;
      const cacheEntry = {
        allocation: allocation,
        timestamp: Date.now(),
        ttl: this.config.cacheTimeout
      };

      this.allocationCache.set(cacheKey, cacheEntry);
    }
  }

  async generateAllocationAuditTrail(allocation, trafficConfig) {
    const auditEntry = {
      id: this.generateAuditId(),
      timestamp: Date.now(),
      action: 'traffic_allocated',
      allocationId: allocation.id,
      experimentId: allocation.experimentId,
      details: {
        strategy: allocation.distribution.method,
        variantCount: trafficConfig.variants.length,
        totalTraffic: allocation.totalTraffic
      },
      integrity: this.generateIntegrityHash({
        allocationId: allocation.id,
        strategy: allocation.distribution.method
      })
    };

    this.allocationHistory.push(auditEntry);
    return auditEntry;
  }

  // Strategy Implementation Methods
  async applyUniformAllocation(variants) {
    const distribution = new Map();
    const equalShare = Math.floor(100 / variants.length);

    variants.forEach(variant => {
      distribution.set(variant.id, equalShare);
    });

    return distribution;
  }

  async applyWeightedAllocation(variants, weights) {
    const distribution = new Map();

    variants.forEach(variant => {
      const weight = weights[variant.id] || (1 / variants.length);
      distribution.set(variant.id, Math.floor(weight * 100));
    });

    return distribution;
  }

  async applyDynamicAllocation(variants, allocation) {
    const distribution = new Map();

    // Simulate dynamic allocation based on performance
    variants.forEach((variant, index) => {
      const baseAllocation = Math.floor(100 / variants.length);
      const performanceBonus = index === 0 ? 5 : 0; // Favor first variant
      distribution.set(variant.id, baseAllocation + performanceBonus);
    });

    return distribution;
  }

  updateAllocationSuccessRate() {
    if (this.trafficMetrics.totalAllocations > 0) {
      this.trafficMetrics.allocationSuccessRate =
        (this.trafficMetrics.activeAllocations / this.trafficMetrics.totalAllocations) * 100;
    }
  }

  recordAllocationError(errorType, data) {
    const errorEntry = {
      id: this.generateErrorId(),
      type: errorType,
      timestamp: Date.now(),
      data: data
    };

    // Store error for monitoring
    console.error(`Traffic Allocation Error [${errorType}]:`, data);
  }

  generateAllocationId() {
    return `allocation_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateAuditId() {
    return `audit_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateErrorId() {
    return `error_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateIntegrityHash(data) {
    const str = JSON.stringify(data);
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      const char = str.charCodeAt(i);
      hash = ((hash << 5) - hash) + char;
      hash = hash & hash;
    }
    return hash.toString(36);
  }
}

export default TrafficAllocation;

    // Store allocation
    this.allocations.set(allocationId, allocation);

    // Create audit entry
    await this.createAllocationAuditEntry({
      action: "traffic_allocated",
      allocationId: allocationId,
      experimentId: experimentId,
      strategy: allocation.strategy,
      result: allocationResult,
      timestamp: timestamp,
    });

    // Update metrics
    this.trafficMetrics.totalAllocations++;
    this.trafficMetrics.activeAllocations++;

    return {
      allocationId: allocationId,
      result: allocationResult,
      metrics: allocation.metrics,
    };
  }

  async executeAllocationStrategy(allocation) {
    // TODO: Execute specific allocation strategy
    // TODO: Apply uniform allocation
    // TODO: Apply weighted allocation
    // TODO: Apply dynamic allocation
    // TODO: Apply probabilistic allocation
    // TODO: Apply multi-armed bandit allocation
    // TODO: Apply Thompson sampling
    // TODO: Apply epsilon-greedy allocation
    // TODO: Apply contextual bandit allocation
    // TODO: Apply adaptive allocation

    const strategy = this.allocationStrategies.get(allocation.strategy);
    if (!strategy) {
      throw new Error(`Unknown allocation strategy: ${allocation.strategy}`);
    }

    return await strategy.execute(allocation);
  }

  /**
   * Load Balancing Implementation
   */
  async implementLoadBalancing(allocation, loadBalancingConfig) {
    // TODO: Implement load balancing algorithm
    // TODO: Monitor server capacity
    // TODO: Calculate load distribution
    // TODO: Apply load balancing rules
    // TODO: Monitor load balancing performance
    // TODO: Adjust allocation based on load
    // TODO: Implement failover mechanisms
    // TODO: Monitor system health
    // TODO: Generate load balancing reports
    // TODO: Optimize load distribution

    const balancer = {
      algorithm: loadBalancingConfig.algorithm || "round_robin",
      capacity: loadBalancingConfig.capacity || {},
      thresholds: loadBalancingConfig.thresholds || {},
      failoverRules: loadBalancingConfig.failoverRules || {},
    };

    return await this.executeLoadBalancing(allocation, balancer);
  }

  async executeLoadBalancing(allocation, balancer) {
    // TODO: Execute load balancing strategy
    switch (balancer.algorithm) {
      case "round_robin":
        return await this.roundRobinAllocation(allocation);
      case "weighted_round_robin":
        return await this.weightedRoundRobinAllocation(
          allocation,
          balancer.capacity
        );
      case "least_connections":
        return await this.leastConnectionsAllocation(allocation);
      case "least_response_time":
        return await this.leastResponseTimeAllocation(allocation);
      case "resource_based":
        return await this.resourceBasedAllocation(
          allocation,
          balancer.capacity
        );
      default:
        throw new Error(
          `Unknown load balancing algorithm: ${balancer.algorithm}`
        );
    }
  }

  /**
   * Fairness Constraints Implementation
   */
  async applyFairnessConstraints(allocation, fairnessRules) {
    // TODO: Apply demographic fairness constraints
    // TODO: Apply geographic fairness constraints
    // TODO: Apply temporal fairness constraints
    // TODO: Apply behavioral fairness constraints
    // TODO: Apply equity constraints
    // TODO: Apply diversity constraints
    // TODO: Monitor fairness metrics
    // TODO: Adjust allocation for fairness
    // TODO: Validate fairness compliance
    // TODO: Generate fairness reports

    const fairnessResult = {
      appliedRules: [],
      fairnessScore: 0,
      adjustments: [],
      compliance: {},
    };

    for (const rule of fairnessRules) {
      const ruleResult = await this.applyFairnessRule(allocation, rule);
      fairnessResult.appliedRules.push(ruleResult);
    }

    fairnessResult.fairnessScore = this.calculateFairnessScore(
      fairnessResult.appliedRules
    );

    return fairnessResult;
  }

  async applyFairnessRule(allocation, rule) {
    // TODO: Apply specific fairness rule
    const ruleProcessor = this.getFairnessRuleProcessor(rule.type);
    return await ruleProcessor.apply(allocation, rule);
  }

  /**
   * Dynamic Allocation Management
   */
  async enableDynamicAllocation(allocationId, dynamicConfig) {
    // TODO: Enable dynamic traffic allocation
    // TODO: Set up dynamic allocation monitoring
    // TODO: Configure allocation adjustment rules
    // TODO: Implement adaptive algorithms
    // TODO: Monitor performance metrics
    // TODO: Adjust allocation in real-time
    // TODO: Handle allocation conflicts
    // TODO: Maintain allocation history
    // TODO: Generate dynamic allocation reports
    // TODO: Optimize dynamic allocation

    const allocation = this.allocations.get(allocationId);
    if (!allocation) {
      throw new Error(`Allocation not found: ${allocationId}`);
    }

    allocation.dynamic = {
      enabled: true,
      config: dynamicConfig,
      adjustmentHistory: [],
      lastAdjustment: null,
      adjustmentFrequency: dynamicConfig.adjustmentFrequency || 300000,
    };

    // Start dynamic allocation monitoring
    await this.startDynamicAllocationMonitoring(allocationId);

    return allocation.dynamic;
  }

  async adjustAllocationDynamically(allocationId, adjustmentData) {
    // TODO: Adjust allocation based on performance data
    const allocation = this.allocations.get(allocationId);
    const adjustment = await this.calculateAllocationAdjustment(
      allocation,
      adjustmentData
    );

    allocation.dynamic.adjustmentHistory.push({
      timestamp: Date.now(),
      adjustment: adjustment,
      reason: adjustmentData.reason,
      performance: adjustmentData.performance,
    });

    return adjustment;
  }

  /**
   * Utility Methods
   */
  initializeAllocationStrategies() {
    // TODO: Initialize uniform allocation strategy
    this.allocationStrategies.set("uniform", {
      execute: async (allocation) => {
        const variantCount = allocation.variants.length;
        const percentage = 100 / variantCount;
        return allocation.variants.map((variant) => ({
          variantId: variant.id,
          percentage: percentage,
          expectedUsers: Math.floor(
            (allocation.totalTraffic * percentage) / 100
          ),
        }));
      },
    });

    // TODO: Initialize weighted allocation strategy
    this.allocationStrategies.set("weighted", {
      execute: async (allocation) => {
        const totalWeight = allocation.variants.reduce(
          (sum, v) => sum + (v.weight || 1),
          0
        );
        return allocation.variants.map((variant) => {
          const weight = variant.weight || 1;
          const percentage = (weight / totalWeight) * 100;
          return {
            variantId: variant.id,
            percentage: percentage,
            expectedUsers: Math.floor(
              (allocation.totalTraffic * percentage) / 100
            ),
          };
        });
      },
    });

    // TODO: Initialize other allocation strategies
  }

  generateAllocationId() {
    return `alloc_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  buildAllocationRules(trafficConfig) {
    // TODO: Build allocation rules from configuration
    return {
      inclusionRules: trafficConfig.inclusionRules || [],
      exclusionRules: trafficConfig.exclusionRules || [],
      priorityRules: trafficConfig.priorityRules || [],
      constraintRules: trafficConfig.constraintRules || [],
    };
  }

  async validateAllocationConfig(allocation) {
    // TODO: Validate allocation configuration
    const errors = [];

    if (!allocation.variants || allocation.variants.length === 0) {
      errors.push("At least one variant is required");
    }

    if (allocation.totalTraffic <= 0) {
      errors.push("Total traffic must be positive");
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  async createAllocationAuditEntry(auditData) {
    // TODO: Create allocation audit entry
    const auditEntry = {
      id: this.generateAuditId(),
      timestamp: auditData.timestamp || Date.now(),
      action: auditData.action,
      allocationId: auditData.allocationId,
      experimentId: auditData.experimentId,
      details: auditData,
      hash: this.generateIntegrityHash(auditData),
    };

    this.allocationHistory.push(auditEntry);
    return auditEntry;
  }

  generateAuditId() {
    return `audit_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  generateIntegrityHash(data) {
    // TODO: Generate integrity hash for audit trail
    return Buffer.from(JSON.stringify(data)).toString("base64");
  }

  /**
   * Allocation Query and Analytics
   */
  getAllocation(allocationId) {
    return this.allocations.get(allocationId);
  }

  getActiveAllocations() {
    return Array.from(this.allocations.values()).filter(
      (a) => a.status === "active"
    );
  }

  getTrafficMetrics() {
    return { ...this.trafficMetrics };
  }

  calculateAllocationEfficiency() {
    // TODO: Calculate allocation efficiency metrics
    const activeAllocations = this.getActiveAllocations();
    if (activeAllocations.length === 0) return 0;

    const totalEfficiency = activeAllocations.reduce((sum, allocation) => {
      return sum + this.calculateSingleAllocationEfficiency(allocation);
    }, 0);

    return totalEfficiency / activeAllocations.length;
  }

  calculateSingleAllocationEfficiency(allocation) {
    // TODO: Calculate efficiency for single allocation
    const expectedUsers = allocation.result.reduce(
      (sum, r) => sum + r.expectedUsers,
      0
    );
    const actualUsers = allocation.metrics.allocatedUsers;

    if (expectedUsers === 0) return 0;
    return Math.min(actualUsers / expectedUsers, 1.0);
  }
}

export default TrafficAllocation;
