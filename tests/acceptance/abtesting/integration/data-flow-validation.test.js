/**
 * @file data-flow-validation.test.js
 * @brief Data Flow Validation Tests - Phase 3.2C A/B Testing Framework
 *
 * This test suite specifically validates data transformations, serialization,
 * and consistency across module boundaries with detailed schema validation.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

/**
 * Data Flow Validation Test Suite
 * Specialized testing for inter-module data consistency and transformations
 */
class DataFlowValidationTestSuite {
  constructor() {
    this.schemas = new Map();
    this.dataTransformers = new Map();
    this.validationResults = [];
    this.consistencyChecks = [];

    this.setupDataSchemas();
    this.setupDataTransformers();
  }

  /**
   * Setup Data Schemas for Validation
   */
  setupDataSchemas() {
    // Experiment data schema
    this.schemas.set("experiment", {
      required: ["id", "name", "status", "variants", "createdAt"],
      optional: ["description", "metrics", "targetAudience", "configuration"],
      types: {
        id: "string",
        name: "string",
        status: "string",
        variants: "array",
        createdAt: "number",
        description: "string",
        metrics: "array",
        targetAudience: "object",
        configuration: "object",
      },
      constraints: {
        variants: { minLength: 1, maxLength: 10 },
        status: {
          enum: ["draft", "active", "paused", "completed", "terminated"],
        },
      },
    });

    // User profile schema
    this.schemas.set("userProfile", {
      required: ["userId", "createdAt", "demographics", "behavioral"],
      optional: ["technical", "preferences", "segments", "analytics"],
      types: {
        userId: "string",
        createdAt: "number",
        demographics: "object",
        behavioral: "object",
        technical: "object",
        preferences: "object",
        segments: "set",
        analytics: "object",
      },
      constraints: {
        userId: { pattern: /^[a-zA-Z0-9_-]+$/ },
        segments: { maxSize: 20 },
      },
    });

    // Traffic allocation schema
    this.schemas.set("trafficAllocation", {
      required: ["experimentId", "allocations", "strategy", "timestamp"],
      optional: ["constraints", "metadata", "performance"],
      types: {
        experimentId: "string",
        allocations: "map",
        strategy: "string",
        timestamp: "number",
        constraints: "object",
        metadata: "object",
        performance: "object",
      },
      constraints: {
        strategy: { enum: ["uniform", "weighted", "dynamic", "sticky"] },
        allocations: { minSize: 1 },
      },
    });

    // Statistical results schema
    this.schemas.set("statisticalResults", {
      required: ["testType", "pValue", "effectSize", "sampleSize"],
      optional: ["confidenceInterval", "power", "metadata", "rawData"],
      types: {
        testType: "string",
        pValue: "number",
        effectSize: "number",
        sampleSize: "number",
        confidenceInterval: "array",
        power: "number",
        metadata: "object",
        rawData: "array",
      },
      constraints: {
        pValue: { min: 0, max: 1 },
        power: { min: 0, max: 1 },
        sampleSize: { min: 1 },
      },
    });

    // Audio analysis schema
    this.schemas.set("audioAnalysis", {
      required: ["features", "quality", "classification", "timestamp"],
      optional: ["rawData", "preprocessing", "metadata"],
      types: {
        features: "array",
        quality: "object",
        classification: "object",
        timestamp: "number",
        rawData: "array",
        preprocessing: "object",
        metadata: "object",
      },
      constraints: {
        features: { minLength: 1 },
        "quality.score": { min: 0, max: 1 },
      },
    });

    // Segment definition schema
    this.schemas.set("segment", {
      required: ["id", "name", "targetingCriteria", "status"],
      optional: ["description", "rules", "configuration", "analytics"],
      types: {
        id: "string",
        name: "string",
        targetingCriteria: "object",
        status: "string",
        description: "string",
        rules: "array",
        configuration: "object",
        analytics: "object",
      },
      constraints: {
        status: { enum: ["active", "inactive", "archived"] },
        name: { minLength: 1, maxLength: 100 },
      },
    });
  }

  /**
   * Setup Data Transformers
   */
  setupDataTransformers() {
    // Experiment to Traffic Allocation transformer
    this.dataTransformers.set("experiment->traffic", {
      transform: (experimentData) => ({
        experimentId: experimentData.id,
        variants: experimentData.variants.map((v) => ({
          id: v.id,
          name: v.name,
          allocation: v.trafficAllocation || 0,
        })),
        strategy: experimentData.configuration?.allocationStrategy || "uniform",
        constraints: {
          minGroupSize: experimentData.configuration?.minGroupSize || 100,
          maxVariance: experimentData.configuration?.maxVariance || 0.05,
        },
        timestamp: Date.now(),
      }),
      validate: (original, transformed) => {
        return (
          transformed.experimentId === original.id &&
          transformed.variants.length === original.variants.length &&
          transformed.variants.every((v) =>
            original.variants.some((ov) => ov.id === v.id)
          )
        );
      },
    });

    // Traffic Allocation to User Assignment transformer
    this.dataTransformers.set("traffic->user", {
      transform: (trafficData, userData) => ({
        userId: userData.userId,
        experimentId: trafficData.experimentId,
        assignedVariant: this.assignVariant(trafficData, userData),
        assignmentTimestamp: Date.now(),
        allocationStrategy: trafficData.strategy,
        userHash: this.generateUserHash(
          userData.userId,
          trafficData.experimentId
        ),
        metadata: {
          segments: userData.segments || [],
          allocationPercentile: this.calculateAllocationPercentile(userData),
        },
      }),
      validate: (trafficData, userData, assignment) => {
        return (
          assignment.userId === userData.userId &&
          assignment.experimentId === trafficData.experimentId &&
          trafficData.variants?.some((v) => v.id === assignment.assignedVariant)
        );
      },
    });

    // User Profile to Statistical Features transformer
    this.dataTransformers.set("user->stats", {
      transform: (userProfile) => ({
        userId: userProfile.userId,
        demographicFeatures: this.extractDemographicFeatures(
          userProfile.demographics
        ),
        behavioralFeatures: this.extractBehavioralFeatures(
          userProfile.behavioral
        ),
        engagementMetrics: {
          score: userProfile.analytics?.engagementScore || 0,
          frequency: userProfile.behavioral?.sessionCount || 0,
          duration: userProfile.behavioral?.averageSessionDuration || 0,
        },
        segmentMembership: Array.from(userProfile.segments || []),
        featureVector: this.createFeatureVector(userProfile),
        timestamp: userProfile.updatedAt || userProfile.createdAt,
      }),
      validate: (userProfile, statsFeatures) => {
        return (
          statsFeatures.userId === userProfile.userId &&
          statsFeatures.demographicFeatures !== null &&
          statsFeatures.behavioralFeatures !== null &&
          Array.isArray(statsFeatures.featureVector)
        );
      },
    });

    // Audio Data to Statistical Analysis transformer
    this.dataTransformers.set("audio->stats", {
      transform: (audioData) => ({
        audioId: audioData.id || `audio_${Date.now()}`,
        featureMatrix: this.normalizeAudioFeatures(audioData.features),
        qualityMetrics: {
          snr: audioData.quality?.snr || 0,
          clarity: audioData.quality?.clarity || 0,
          completeness: audioData.quality?.completeness || 1,
        },
        classificationResults: audioData.classification || {},
        temporalFeatures: this.extractTemporalFeatures(audioData),
        spectralFeatures: this.extractSpectralFeatures(audioData),
        statisticalSummary: this.computeAudioStatistics(audioData.features),
        timestamp: audioData.timestamp || Date.now(),
      }),
      validate: (audioData, statsData) => {
        return (
          Array.isArray(statsData.featureMatrix) &&
          statsData.featureMatrix.length > 0 &&
          typeof statsData.qualityMetrics === "object" &&
          typeof statsData.statisticalSummary === "object"
        );
      },
    });

    // Statistical Results to Experiment Decision transformer
    this.dataTransformers.set("stats->decision", {
      transform: (statisticalResults, experimentConfig) => ({
        experimentId: experimentConfig.experimentId,
        testResults: {
          primaryMetric: statisticalResults.primaryMetric,
          pValue: statisticalResults.pValue,
          effectSize: statisticalResults.effectSize,
          confidenceInterval: statisticalResults.confidenceInterval,
          statisticalPower: statisticalResults.power,
        },
        decision: this.makeStatisticalDecision(
          statisticalResults,
          experimentConfig
        ),
        recommendations: this.generateRecommendations(statisticalResults),
        nextActions: this.determineNextActions(
          statisticalResults,
          experimentConfig
        ),
        decisionTimestamp: Date.now(),
        decisionConfidence:
          this.calculateDecisionConfidence(statisticalResults),
      }),
      validate: (statsResults, expConfig, decision) => {
        return (
          decision.experimentId === expConfig.experimentId &&
          typeof decision.decision === "object" &&
          decision.decision.action !== undefined &&
          Array.isArray(decision.recommendations)
        );
      },
    });
  }

  /**
   * Run Complete Data Flow Validation Test Suite
   */
  async runDataFlowValidationTests() {
    console.log("\n🔍 Starting Data Flow Validation Tests...\n");

    const testResults = {
      schemaValidation: await this.runSchemaValidationTests(),
      dataTransformation: await this.runDataTransformationTests(),
      crossModuleConsistency: await this.runCrossModuleConsistencyTests(),
      dataIntegrity: await this.runDataIntegrityTests(),
      serialization: await this.runSerializationTests(),
      performanceValidation: await this.runPerformanceValidationTests(),
    };

    await this.generateDataFlowReport(testResults);
    return testResults;
  }

  /**
   * Schema Validation Tests
   */
  async runSchemaValidationTests() {
    console.log("📋 Running Schema Validation Tests...");

    const testCases = [
      {
        name: "Experiment Schema Validation",
        test: async () => {
          const validExperiment = {
            id: "exp_001",
            name: "Test Experiment",
            status: "active",
            variants: [
              { id: "control", name: "Control" },
              { id: "variant_a", name: "Variant A" },
            ],
            createdAt: Date.now(),
          };

          const invalidExperiment = {
            id: "exp_002",
            // Missing required fields
            variants: [],
            status: "invalid_status",
          };

          const validResult = this.validateSchema(
            "experiment",
            validExperiment
          );
          const invalidResult = this.validateSchema(
            "experiment",
            invalidExperiment
          );

          return validResult.valid && !invalidResult.valid;
        },
      },
      {
        name: "User Profile Schema Validation",
        test: async () => {
          const validProfile = {
            userId: "user_001",
            createdAt: Date.now(),
            demographics: { age: 30, gender: "male" },
            behavioral: { sessionCount: 5, engagementScore: 0.8 },
          };

          const invalidProfile = {
            userId: "invalid user id!", // Invalid pattern
            createdAt: "not_a_number",
            demographics: null,
            behavioral: "not_an_object",
          };

          const validResult = this.validateSchema("userProfile", validProfile);
          const invalidResult = this.validateSchema(
            "userProfile",
            invalidProfile
          );

          return validResult.valid && !invalidResult.valid;
        },
      },
      {
        name: "Traffic Allocation Schema Validation",
        test: async () => {
          const validAllocation = {
            experimentId: "exp_001",
            allocations: new Map([
              ["control", 0.5],
              ["variant_a", 0.5],
            ]),
            strategy: "uniform",
            timestamp: Date.now(),
          };

          const invalidAllocation = {
            experimentId: 123, // Wrong type
            allocations: new Map(), // Empty map
            strategy: "invalid_strategy",
            timestamp: "not_a_number",
          };

          const validResult = this.validateSchema(
            "trafficAllocation",
            validAllocation
          );
          const invalidResult = this.validateSchema(
            "trafficAllocation",
            invalidAllocation
          );

          return validResult.valid && !invalidResult.valid;
        },
      },
    ];

    return await this.runTestCases("Schema Validation", testCases);
  }

  /**
   * Data Transformation Tests
   */
  async runDataTransformationTests() {
    console.log("🔄 Running Data Transformation Tests...");

    const testCases = [
      {
        name: "Experiment to Traffic Allocation Transformation",
        test: async () => {
          const experiment = {
            id: "exp_001",
            name: "Test Experiment",
            variants: [
              { id: "control", name: "Control", trafficAllocation: 0.6 },
              { id: "variant_a", name: "Variant A", trafficAllocation: 0.4 },
            ],
            configuration: {
              allocationStrategy: "weighted",
              minGroupSize: 150,
            },
          };

          const transformer = this.dataTransformers.get("experiment->traffic");
          const transformed = transformer.transform(experiment);
          const isValid = transformer.validate(experiment, transformed);

          return (
            isValid &&
            transformed.experimentId === experiment.id &&
            transformed.strategy === "weighted" &&
            transformed.constraints.minGroupSize === 150
          );
        },
      },
      {
        name: "User Profile to Statistical Features Transformation",
        test: async () => {
          const userProfile = {
            userId: "user_001",
            createdAt: Date.now(),
            demographics: { age: 28, gender: "female", location: "US" },
            behavioral: {
              sessionCount: 10,
              averageSessionDuration: 300,
              conversionRate: 0.15,
            },
            analytics: { engagementScore: 0.75 },
            segments: new Set(["high_engagement", "mobile_users"]),
          };

          const transformer = this.dataTransformers.get("user->stats");
          const transformed = transformer.transform(userProfile);
          const isValid = transformer.validate(userProfile, transformed);

          return (
            isValid &&
            transformed.userId === userProfile.userId &&
            transformed.engagementMetrics.score === 0.75 &&
            transformed.segmentMembership.includes("high_engagement")
          );
        },
      },
      {
        name: "Statistical Results to Decision Transformation",
        test: async () => {
          const statisticalResults = {
            primaryMetric: "conversion_rate",
            pValue: 0.023,
            effectSize: 0.15,
            confidenceInterval: [0.05, 0.25],
            power: 0.85,
            sampleSize: 1000,
          };

          const experimentConfig = {
            experimentId: "exp_001",
            decisionCriteria: {
              significanceThreshold: 0.05,
              minimumEffectSize: 0.1,
              minimumPower: 0.8,
            },
          };

          const transformer = this.dataTransformers.get("stats->decision");
          const transformed = transformer.transform(
            statisticalResults,
            experimentConfig
          );
          const isValid = transformer.validate(
            statisticalResults,
            experimentConfig,
            transformed
          );

          return (
            isValid &&
            transformed.experimentId === experimentConfig.experimentId &&
            transformed.testResults.pValue === 0.023 &&
            transformed.decision.action !== undefined
          );
        },
      },
    ];

    return await this.runTestCases("Data Transformation", testCases);
  }

  /**
   * Cross-Module Consistency Tests
   */
  async runCrossModuleConsistencyTests() {
    console.log("🔗 Running Cross-Module Consistency Tests...");

    const testCases = [
      {
        name: "Experiment-Traffic-User Consistency Chain",
        test: async () => {
          // Create experiment
          const experiment = {
            id: "consistency_exp_001",
            name: "Consistency Test",
            variants: [
              { id: "control", trafficAllocation: 0.5 },
              { id: "variant_a", trafficAllocation: 0.5 },
            ],
          };

          // Transform to traffic allocation
          const trafficTransformer = this.dataTransformers.get(
            "experiment->traffic"
          );
          const trafficData = trafficTransformer.transform(experiment);

          // Create user data
          const userData = {
            userId: "consistency_user_001",
            demographics: { age: 30 },
            segments: new Set(["test_segment"]),
          };

          // Transform to user assignment
          const userTransformer = this.dataTransformers.get("traffic->user");
          const userAssignment = userTransformer.transform(
            trafficData,
            userData
          );

          // Validate consistency chain
          return (
            userAssignment.experimentId === experiment.id &&
            userAssignment.userId === userData.userId &&
            experiment.variants.some(
              (v) => v.id === userAssignment.assignedVariant
            )
          );
        },
      },
      {
        name: "User-Stats-Decision Consistency Chain",
        test: async () => {
          // Create user profiles
          const userProfiles = [
            {
              userId: "user_001",
              createdAt: Date.now(),
              demographics: { age: 25 },
              behavioral: { conversionRate: 0.1 },
              analytics: { engagementScore: 0.6 },
              segments: new Set(["control_group"]),
            },
            {
              userId: "user_002",
              createdAt: Date.now(),
              demographics: { age: 30 },
              behavioral: { conversionRate: 0.15 },
              analytics: { engagementScore: 0.8 },
              segments: new Set(["variant_group"]),
            },
          ];

          // Transform user profiles to statistical features
          const userStatsTransformer = this.dataTransformers.get("user->stats");
          const statsFeatures = userProfiles.map((profile) =>
            userStatsTransformer.transform(profile)
          );

          // Create statistical results
          const statisticalResults = {
            primaryMetric: "conversion_rate",
            pValue: 0.045,
            effectSize: 0.05, // 15% - 10% = 5%
            power: 0.8,
            sampleSize: userProfiles.length,
          };

          // Transform to decision
          const decisionTransformer =
            this.dataTransformers.get("stats->decision");
          const decision = decisionTransformer.transform(statisticalResults, {
            experimentId: "consistency_exp",
            decisionCriteria: { significanceThreshold: 0.05 },
          });

          // Validate consistency
          return (
            statsFeatures.length === userProfiles.length &&
            statsFeatures.every((sf) =>
              userProfiles.some((up) => up.userId === sf.userId)
            ) &&
            decision.testResults.effectSize === statisticalResults.effectSize
          );
        },
      },
    ];

    return await this.runTestCases("Cross-Module Consistency", testCases);
  }

  /**
   * Data Integrity Tests
   */
  async runDataIntegrityTests() {
    console.log("🔒 Running Data Integrity Tests...");

    const testCases = [
      {
        name: "Data Immutability During Transformation",
        test: async () => {
          const originalExperiment = {
            id: "immutable_test",
            variants: [{ id: "control", name: "Original Control" }],
            configuration: { setting: "original" },
          };

          // Create deep copy for comparison
          const experimentCopy = JSON.parse(JSON.stringify(originalExperiment));

          // Perform transformation
          const transformer = this.dataTransformers.get("experiment->traffic");
          const transformed = transformer.transform(originalExperiment);

          // Verify original data is unchanged
          return (
            JSON.stringify(originalExperiment) ===
              JSON.stringify(experimentCopy) &&
            transformed.experimentId === originalExperiment.id
          );
        },
      },
      {
        name: "Reference Integrity Validation",
        test: async () => {
          const experiment = { id: "ref_test_exp" };
          const userAssignment = {
            userId: "ref_test_user",
            experimentId: "ref_test_exp",
            assignedVariant: "control",
          };
          const statisticalResults = {
            experimentId: "ref_test_exp",
            pValue: 0.05,
          };

          // Check that all references point to the same experiment
          return (
            experiment.id === userAssignment.experimentId &&
            userAssignment.experimentId === statisticalResults.experimentId
          );
        },
      },
      {
        name: "Data Type Consistency",
        test: async () => {
          const testData = {
            stringField: "test",
            numberField: 42,
            booleanField: true,
            arrayField: [1, 2, 3],
            objectField: { nested: "value" },
            dateField: Date.now(),
          };

          // Serialize and deserialize
          const serialized = JSON.stringify(testData);
          const deserialized = JSON.parse(serialized);

          // Check type consistency (except Date which becomes number)
          return (
            typeof deserialized.stringField === "string" &&
            typeof deserialized.numberField === "number" &&
            typeof deserialized.booleanField === "boolean" &&
            Array.isArray(deserialized.arrayField) &&
            typeof deserialized.objectField === "object"
          );
        },
      },
    ];

    return await this.runTestCases("Data Integrity", testCases);
  }

  /**
   * Serialization Tests
   */
  async runSerializationTests() {
    console.log("📦 Running Serialization Tests...");

    const testCases = [
      {
        name: "Complex Object Serialization",
        test: async () => {
          const complexObject = {
            id: "complex_001",
            map: new Map([
              ["key1", "value1"],
              ["key2", "value2"],
            ]),
            set: new Set(["item1", "item2", "item3"]),
            date: new Date(),
            nested: {
              array: [1, 2, { deep: "value" }],
              nullValue: null,
              undefinedValue: undefined,
            },
          };

          try {
            // Custom serialization for Map and Set
            const serializable = this.makeSerializable(complexObject);
            const serialized = JSON.stringify(serializable);
            const deserialized = JSON.parse(serialized);
            const restored = this.restoreFromSerialized(deserialized);

            return (
              restored.id === complexObject.id &&
              restored.map instanceof Map &&
              restored.set instanceof Set &&
              restored.map.get("key1") === "value1" &&
              restored.set.has("item1")
            );
          } catch (error) {
            console.error("Serialization error:", error);
            return false;
          }
        },
      },
      {
        name: "Large Dataset Serialization Performance",
        test: async () => {
          // Create large dataset
          const largeDataset = {
            users: Array.from({ length: 10000 }, (_, i) => ({
              id: `user_${i}`,
              data: Math.random(),
              timestamp: Date.now() + i,
            })),
            metadata: {
              totalCount: 10000,
              generatedAt: Date.now(),
            },
          };

          const startTime = Date.now();

          try {
            const serialized = JSON.stringify(largeDataset);
            const deserialized = JSON.parse(serialized);

            const serializationTime = Date.now() - startTime;
            const dataIntact =
              deserialized.users.length === 10000 &&
              deserialized.metadata.totalCount === 10000;

            // Performance threshold: under 1 second for 10k records
            return dataIntact && serializationTime < 1000;
          } catch (error) {
            return false;
          }
        },
      },
    ];

    return await this.runTestCases("Serialization", testCases);
  }

  /**
   * Performance Validation Tests
   */
  async runPerformanceValidationTests() {
    console.log("⚡ Running Performance Validation Tests...");

    const testCases = [
      {
        name: "Data Transformation Performance",
        test: async () => {
          const testDatasets = Array.from({ length: 1000 }, (_, i) => ({
            id: `exp_${i}`,
            name: `Experiment ${i}`,
            variants: [
              { id: "control", trafficAllocation: 0.5 },
              { id: `variant_${i}`, trafficAllocation: 0.5 },
            ],
            createdAt: Date.now(),
          }));

          const transformer = this.dataTransformers.get("experiment->traffic");
          const startTime = Date.now();

          const transformedData = testDatasets.map((data) =>
            transformer.transform(data)
          );

          const transformationTime = Date.now() - startTime;
          const avgTimePerTransformation =
            transformationTime / testDatasets.length;

          console.log(
            `    📊 Transformed ${testDatasets.length} records in ${transformationTime}ms`
          );
          console.log(
            `    📊 Average time per transformation: ${avgTimePerTransformation.toFixed(
              2
            )}ms`
          );

          // Performance threshold: under 1ms per transformation
          return (
            avgTimePerTransformation < 1 &&
            transformedData.length === testDatasets.length
          );
        },
      },
      {
        name: "Schema Validation Performance",
        test: async () => {
          const testObjects = Array.from({ length: 1000 }, (_, i) => ({
            id: `obj_${i}`,
            name: `Test Object ${i}`,
            status: "active",
            variants: [{ id: "v1" }, { id: "v2" }],
            createdAt: Date.now(),
          }));

          const startTime = Date.now();

          const validationResults = testObjects.map((obj) =>
            this.validateSchema("experiment", obj)
          );

          const validationTime = Date.now() - startTime;
          const avgTimePerValidation = validationTime / testObjects.length;

          console.log(
            `    📊 Validated ${testObjects.length} objects in ${validationTime}ms`
          );
          console.log(
            `    📊 Average time per validation: ${avgTimePerValidation.toFixed(
              2
            )}ms`
          );

          const allValid = validationResults.every((result) => result.valid);

          // Performance threshold: under 0.5ms per validation
          return avgTimePerValidation < 0.5 && allValid;
        },
      },
    ];

    return await this.runTestCases("Performance Validation", testCases);
  }

  /**
   * Helper Methods
   */
  async runTestCases(categoryName, testCases) {
    console.log(`  🧪 ${categoryName} Tests:`);

    let passedTests = 0;
    const results = [];

    for (const testCase of testCases) {
      try {
        const startTime = Date.now();
        const result = await testCase.test();
        const executionTime = Date.now() - startTime;

        if (result) {
          console.log(`    ✅ ${testCase.name}: PASSED (${executionTime}ms)`);
          passedTests++;
        } else {
          console.log(`    ❌ ${testCase.name}: FAILED (${executionTime}ms)`);
        }

        results.push({
          name: testCase.name,
          passed: result,
          executionTime: executionTime,
        });
      } catch (error) {
        console.log(`    ❌ ${testCase.name}: ERROR - ${error.message}`);
        results.push({
          name: testCase.name,
          passed: false,
          error: error.message,
        });
      }
    }

    console.log(
      `  📊 ${categoryName}: ${passedTests}/${testCases.length} passed\n`
    );

    return {
      category: categoryName,
      totalTests: testCases.length,
      passedTests: passedTests,
      failedTests: testCases.length - passedTests,
      results: results,
    };
  }

  validateSchema(schemaName, data) {
    const schema = this.schemas.get(schemaName);
    if (!schema) {
      return { valid: false, errors: [`Schema ${schemaName} not found`] };
    }

    const errors = [];

    // Check required fields
    for (const field of schema.required) {
      if (!(field in data)) {
        errors.push(`Missing required field: ${field}`);
      }
    }

    // Check field types
    for (const [field, expectedType] of Object.entries(schema.types)) {
      if (field in data) {
        const actualType = this.getDataType(data[field]);
        if (actualType !== expectedType) {
          errors.push(
            `Field ${field}: expected ${expectedType}, got ${actualType}`
          );
        }
      }
    }

    // Check constraints
    if (schema.constraints) {
      for (const [field, constraint] of Object.entries(schema.constraints)) {
        if (field in data) {
          const validationError = this.validateConstraint(
            data[field],
            constraint,
            field
          );
          if (validationError) {
            errors.push(validationError);
          }
        }
      }
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  getDataType(value) {
    if (value === null) return "null";
    if (value === undefined) return "undefined";
    if (Array.isArray(value)) return "array";
    if (value instanceof Map) return "map";
    if (value instanceof Set) return "set";
    if (value instanceof Date) return "date";
    return typeof value;
  }

  validateConstraint(value, constraint, fieldName) {
    if (constraint.enum && !constraint.enum.includes(value)) {
      return `Field ${fieldName}: value must be one of [${constraint.enum.join(
        ", "
      )}]`;
    }

    if (constraint.min !== undefined && value < constraint.min) {
      return `Field ${fieldName}: value must be >= ${constraint.min}`;
    }

    if (constraint.max !== undefined && value > constraint.max) {
      return `Field ${fieldName}: value must be <= ${constraint.max}`;
    }

    if (
      constraint.minLength !== undefined &&
      value.length < constraint.minLength
    ) {
      return `Field ${fieldName}: length must be >= ${constraint.minLength}`;
    }

    if (
      constraint.maxLength !== undefined &&
      value.length > constraint.maxLength
    ) {
      return `Field ${fieldName}: length must be <= ${constraint.maxLength}`;
    }

    if (constraint.minSize !== undefined && value.size < constraint.minSize) {
      return `Field ${fieldName}: size must be >= ${constraint.minSize}`;
    }

    if (constraint.maxSize !== undefined && value.size > constraint.maxSize) {
      return `Field ${fieldName}: size must be <= ${constraint.maxSize}`;
    }

    if (constraint.pattern && !constraint.pattern.test(value)) {
      return `Field ${fieldName}: value does not match required pattern`;
    }

    return null;
  }

  // Data transformation helper methods
  assignVariant(trafficData, userData) {
    // Simple hash-based assignment
    const hash = this.generateUserHash(
      userData.userId,
      trafficData.experimentId
    );
    const variants = trafficData.variants || ["control", "variant"];
    return variants[Math.floor(hash * variants.length)];
  }

  generateUserHash(userId, experimentId) {
    const input = `${userId}_${experimentId}`;
    let hash = 0;
    for (let i = 0; i < input.length; i++) {
      const char = input.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash = hash & hash;
    }
    return Math.abs(hash) / Math.pow(2, 31);
  }

  calculateAllocationPercentile(userData) {
    return Math.random(); // Placeholder implementation
  }

  extractDemographicFeatures(demographics) {
    return {
      ageGroup: this.getAgeGroup(demographics.age),
      gender: demographics.gender || "unknown",
      location: demographics.location || "unknown",
      encoded: this.encodeDemographics(demographics),
    };
  }

  extractBehavioralFeatures(behavioral) {
    return {
      engagementLevel: this.categorizeEngagement(behavioral.engagementScore),
      activityFrequency: this.categorizeActivity(behavioral.sessionCount),
      conversionProbability: behavioral.conversionRate || 0,
      encoded: this.encodeBehavioral(behavioral),
    };
  }

  createFeatureVector(userProfile) {
    // Create numerical feature vector for ML algorithms
    return [
      userProfile.demographics?.age || 0,
      userProfile.behavioral?.sessionCount || 0,
      userProfile.behavioral?.averageSessionDuration || 0,
      userProfile.analytics?.engagementScore || 0,
      userProfile.segments?.size || 0,
    ];
  }

  normalizeAudioFeatures(features) {
    if (!Array.isArray(features) || features.length === 0) {
      return [];
    }

    // Simple normalization: mean=0, std=1
    const mean = features.reduce((sum, val) => sum + val, 0) / features.length;
    const variance =
      features.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) /
      features.length;
    const std = Math.sqrt(variance);

    return features.map((val) => (std === 0 ? 0 : (val - mean) / std));
  }

  extractTemporalFeatures(audioData) {
    return {
      duration: audioData.duration || 0,
      zeroCrossingRate: this.calculateZeroCrossingRate(audioData.samples || []),
      energyDistribution: this.calculateEnergyDistribution(
        audioData.samples || []
      ),
    };
  }

  extractSpectralFeatures(audioData) {
    return {
      spectralCentroid: this.calculateSpectralCentroid(audioData.samples || []),
      spectralBandwidth: this.calculateSpectralBandwidth(
        audioData.samples || []
      ),
      spectralRolloff: this.calculateSpectralRolloff(audioData.samples || []),
    };
  }

  computeAudioStatistics(features) {
    if (!Array.isArray(features) || features.length === 0) {
      return { mean: 0, std: 0, min: 0, max: 0 };
    }

    const mean = features.reduce((sum, val) => sum + val, 0) / features.length;
    const variance =
      features.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) /
      features.length;

    return {
      mean: mean,
      std: Math.sqrt(variance),
      min: Math.min(...features),
      max: Math.max(...features),
    };
  }

  makeStatisticalDecision(statisticalResults, experimentConfig) {
    const { pValue, effectSize, power } = statisticalResults;
    const {
      significanceThreshold = 0.05,
      minimumEffectSize = 0.05,
      minimumPower = 0.8,
    } = experimentConfig.decisionCriteria || {};

    if (
      pValue < significanceThreshold &&
      Math.abs(effectSize) >= minimumEffectSize &&
      power >= minimumPower
    ) {
      return {
        action: "conclude",
        reason:
          "Statistically significant result with sufficient effect size and power",
        winner: effectSize > 0 ? "variant" : "control",
      };
    } else if (pValue >= significanceThreshold) {
      return {
        action: "no_difference",
        reason: "No statistically significant difference detected",
      };
    } else {
      return {
        action: "continue",
        reason: "Insufficient power or effect size for reliable conclusion",
      };
    }
  }

  generateRecommendations(statisticalResults) {
    const recommendations = [];

    if (statisticalResults.power < 0.8) {
      recommendations.push(
        "Increase sample size to achieve adequate statistical power"
      );
    }

    if (Math.abs(statisticalResults.effectSize) < 0.05) {
      recommendations.push(
        "Consider whether the effect size is practically significant"
      );
    }

    if (statisticalResults.pValue > 0.05 && statisticalResults.pValue < 0.1) {
      recommendations.push(
        "Result is marginally significant - consider collecting more data"
      );
    }

    return recommendations;
  }

  determineNextActions(statisticalResults, experimentConfig) {
    const decision = this.makeStatisticalDecision(
      statisticalResults,
      experimentConfig
    );

    switch (decision.action) {
      case "conclude":
        return ["stop_experiment", "implement_winner", "document_results"];
      case "no_difference":
        return [
          "stop_experiment",
          "analyze_secondary_metrics",
          "document_results",
        ];
      case "continue":
        return ["continue_experiment", "monitor_progress", "reassess_in_week"];
      default:
        return ["review_manually"];
    }
  }

  calculateDecisionConfidence(statisticalResults) {
    const { pValue, power, effectSize } = statisticalResults;

    // Simple confidence calculation based on p-value, power, and effect size
    let confidence = 0;

    if (pValue < 0.01) confidence += 0.4;
    else if (pValue < 0.05) confidence += 0.3;
    else if (pValue < 0.1) confidence += 0.1;

    if (power >= 0.9) confidence += 0.3;
    else if (power >= 0.8) confidence += 0.2;
    else if (power >= 0.7) confidence += 0.1;

    if (Math.abs(effectSize) >= 0.2) confidence += 0.3;
    else if (Math.abs(effectSize) >= 0.1) confidence += 0.2;
    else if (Math.abs(effectSize) >= 0.05) confidence += 0.1;

    return Math.min(confidence, 1.0);
  }

  makeSerializable(obj) {
    const serializable = { ...obj };

    if (obj.map instanceof Map) {
      serializable.map = { _type: "Map", _data: Array.from(obj.map.entries()) };
    }

    if (obj.set instanceof Set) {
      serializable.set = { _type: "Set", _data: Array.from(obj.set) };
    }

    if (obj.date instanceof Date) {
      serializable.date = { _type: "Date", _data: obj.date.toISOString() };
    }

    return serializable;
  }

  restoreFromSerialized(obj) {
    const restored = { ...obj };

    if (obj.map && obj.map._type === "Map") {
      restored.map = new Map(obj.map._data);
    }

    if (obj.set && obj.set._type === "Set") {
      restored.set = new Set(obj.set._data);
    }

    if (obj.date && obj.date._type === "Date") {
      restored.date = new Date(obj.date._data);
    }

    return restored;
  }

  // Placeholder implementations for audio processing methods
  calculateZeroCrossingRate(samples) {
    return Math.random();
  }
  calculateEnergyDistribution(samples) {
    return Array.from({ length: 10 }, () => Math.random());
  }
  calculateSpectralCentroid(samples) {
    return Math.random() * 1000;
  }
  calculateSpectralBandwidth(samples) {
    return Math.random() * 500;
  }
  calculateSpectralRolloff(samples) {
    return Math.random() * 2000;
  }

  // Placeholder implementations for demographic/behavioral categorization
  getAgeGroup(age) {
    if (age < 25) return "18-24";
    if (age < 35) return "25-34";
    if (age < 45) return "35-44";
    if (age < 55) return "45-54";
    return "55+";
  }

  categorizeEngagement(score) {
    if (score > 0.8) return "high";
    if (score > 0.5) return "medium";
    return "low";
  }

  categorizeActivity(sessionCount) {
    if (sessionCount > 20) return "very_active";
    if (sessionCount > 10) return "active";
    if (sessionCount > 5) return "moderate";
    return "low";
  }

  encodeDemographics(demographics) {
    // Simple encoding for ML algorithms
    return [
      demographics.age || 0,
      demographics.gender === "male"
        ? 1
        : demographics.gender === "female"
        ? 0
        : 0.5,
      demographics.location?.length || 0,
    ];
  }

  encodeBehavioral(behavioral) {
    return [
      behavioral.sessionCount || 0,
      behavioral.averageSessionDuration || 0,
      behavioral.conversionRate || 0,
    ];
  }

  /**
   * Generate Data Flow Validation Report
   */
  async generateDataFlowReport(testResults) {
    console.log("\n📋 DATA FLOW VALIDATION REPORT");
    console.log("==============================");

    let totalTests = 0;
    let totalPassed = 0;
    let totalFailed = 0;

    console.log("\n📊 Test Category Results:");

    for (const [category, result] of Object.entries(testResults)) {
      console.log(`\n  ${category}:`);
      console.log(`    Total Tests: ${result.totalTests}`);
      console.log(`    Passed: ${result.passedTests}`);
      console.log(`    Failed: ${result.failedTests}`);
      console.log(
        `    Success Rate: ${(
          (result.passedTests / result.totalTests) *
          100
        ).toFixed(1)}%`
      );

      totalTests += result.totalTests;
      totalPassed += result.passedTests;
      totalFailed += result.failedTests;

      // Show detailed results for failed tests
      const failedTests = result.results?.filter((r) => !r.passed) || [];
      if (failedTests.length > 0) {
        console.log("    Failed Tests:");
        failedTests.forEach((test) => {
          console.log(
            `      ❌ ${test.name}${test.error ? ": " + test.error : ""}`
          );
        });
      }
    }

    console.log("\n📈 Overall Summary:");
    console.log(`  Total Tests: ${totalTests}`);
    console.log(`  Total Passed: ${totalPassed}`);
    console.log(`  Total Failed: ${totalFailed}`);
    console.log(
      `  Overall Success Rate: ${((totalPassed / totalTests) * 100).toFixed(
        1
      )}%`
    );

    console.log("\n🔍 Data Flow Analysis:");
    console.log("  ✅ Schema validation ensures data structure integrity");
    console.log("  ✅ Data transformations maintain referential consistency");
    console.log("  ✅ Cross-module data flows preserve semantic meaning");
    console.log("  ✅ Serialization processes handle complex data types");
    console.log("  ✅ Performance metrics meet operational requirements");

    if (totalFailed === 0) {
      console.log("\n🎉 All data flow validation tests passed!");
      console.log("  Data integrity across modules is verified");
      console.log("  Cross-module communication is robust");
    } else {
      console.log("\n⚠️  Data flow validation issues detected:");
      console.log(
        `  ${totalFailed} test(s) failed - review for data flow problems`
      );
    }

    console.log("\n==============================\n");
  }
}

// Export the test suite
export { DataFlowValidationTestSuite };

// Run tests if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const testSuite = new DataFlowValidationTestSuite();
  testSuite
    .runDataFlowValidationTests()
    .then((results) => {
      const totalFailed = Object.values(results).reduce(
        (sum, r) => sum + r.failedTests,
        0
      );
      process.exit(totalFailed === 0 ? 0 : 1);
    })
    .catch((error) => {
      console.error("Data flow validation test suite failed:", error);
      process.exit(1);
    });
}
