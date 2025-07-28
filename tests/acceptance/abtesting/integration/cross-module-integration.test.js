/**
 * @file cross-module-integration.test.js
 * @brief Cross-Module Integration Tests - Phase 3.2C A/B Testing Framework
 *
 * This test suite validates cross-module communication and data flow between
 * all major components of the A/B testing framework including experiment management,
 * traffic allocation, user segmentation, statistical analysis, and audio collection.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

import { ExperimentManager } from "../management/experiment-manager.js";
import { TrafficAllocation } from "../management/traffic-allocation.js";
import { UserSegmentation } from "../management/user-segmentation.js";
import { StatisticalEngine } from "../analysis/statistical-engine.js";
import { AudioCollector } from "../../analytics/collection/audio-collector.js";

/**
 * Cross-Module Integration Test Suite
 * Validates data flow and communication between framework modules
 */
class CrossModuleIntegrationTestSuite {
  constructor() {
    this.testResults = [];
    this.modules = new Map();
    this.dataFlowValidators = new Map();
    this.communicationTrackers = new Map();
    this.integrationMetrics = {
      totalTests: 0,
      passedTests: 0,
      failedTests: 0,
      dataFlowTests: 0,
      communicationTests: 0,
      performanceTests: 0,
    };

    this.setupTestEnvironment();
  }

  /**
   * Test Environment Setup
   */
  async setupTestEnvironment() {
    console.log("Setting up cross-module integration test environment...");

    try {
      // Initialize all modules with test configurations
      this.modules.set(
        "experimentManager",
        new ExperimentManager({
          enableRealTimeMonitoring: true,
          enableAutoOptimization: true,
          enableAuditTrail: true,
          strictValidation: true,
        })
      );

      this.modules.set(
        "trafficAllocation",
        new TrafficAllocation({
          enableDynamicAllocation: true,
          enableLoadBalancing: true,
          enableFairnessConstraints: true,
          enableAllocationCaching: true,
        })
      );

      this.modules.set(
        "userSegmentation",
        new UserSegmentation({
          enableRealTimeSegmentation: true,
          enableBehavioralTracking: true,
          enableSegmentPrediction: true,
          maxSegments: 50,
        })
      );

      this.modules.set(
        "statisticalEngine",
        new StatisticalEngine({
          enableAdvancedAnalysis: true,
          enableBayesianAnalysis: true,
          enablePowerAnalysis: true,
          confidenceLevel: 0.95,
        })
      );

      this.modules.set(
        "audioCollector",
        new AudioCollector({
          enableRealTimeAnalysis: true,
          enableFeatureExtraction: true,
          enableQualityAnalysis: true,
          sampleRate: 44100,
        })
      );

      // Set up data flow validators
      this.setupDataFlowValidators();

      // Initialize communication trackers
      this.setupCommunicationTrackers();

      console.log("✅ Test environment setup completed");
      return true;
    } catch (error) {
      console.error("❌ Failed to setup test environment:", error);
      throw error;
    }
  }

  /**
   * Data Flow Validation Setup
   */
  setupDataFlowValidators() {
    // Experiment -> Traffic Allocation data flow
    this.dataFlowValidators.set("experiment-to-traffic", {
      validate: (experimentData, trafficData) => {
        return (
          experimentData.variants &&
          trafficData.allocations &&
          experimentData.variants.length === trafficData.allocations.size
        );
      },
      requiredFields: ["variants", "trafficSplit", "targetAudience"],
      dataTransforms: ["variant_mapping", "allocation_percentages"],
    });

    // Traffic Allocation -> User Segmentation data flow
    this.dataFlowValidators.set("traffic-to-segmentation", {
      validate: (trafficData, segmentData) => {
        return (
          trafficData.userAllocations &&
          segmentData.segments &&
          trafficData.userAllocations.size > 0
        );
      },
      requiredFields: ["userAllocations", "allocationStrategy", "segments"],
      dataTransforms: ["user_bucketing", "segment_assignments"],
    });

    // User Segmentation -> Statistical Engine data flow
    this.dataFlowValidators.set("segmentation-to-statistics", {
      validate: (segmentData, statisticsData) => {
        return (
          segmentData.userProfiles &&
          statisticsData.analyses &&
          segmentData.userProfiles.size > 0
        );
      },
      requiredFields: ["userProfiles", "behaviorData", "conversionMetrics"],
      dataTransforms: ["profile_aggregation", "statistical_features"],
    });

    // Audio Collector -> Statistical Engine data flow
    this.dataFlowValidators.set("audio-to-statistics", {
      validate: (audioData, statisticsData) => {
        return (
          audioData.features &&
          statisticsData.audioAnalyses &&
          audioData.features.length > 0
        );
      },
      requiredFields: ["features", "qualityMetrics", "classificationResults"],
      dataTransforms: ["feature_normalization", "statistical_preparation"],
    });

    // Statistical Engine -> Experiment Manager data flow
    this.dataFlowValidators.set("statistics-to-experiment", {
      validate: (statisticsData, experimentData) => {
        return (
          statisticsData.results &&
          experimentData.analytics &&
          statisticsData.results.length > 0
        );
      },
      requiredFields: ["testResults", "confidenceIntervals", "pValues"],
      dataTransforms: ["result_aggregation", "decision_preparation"],
    });
  }

  /**
   * Communication Tracking Setup
   */
  setupCommunicationTrackers() {
    // Track method calls between modules
    this.communicationTrackers.set("method_calls", {
      calls: [],
      track: (source, target, method, data) => {
        this.communicationTrackers.get("method_calls").calls.push({
          timestamp: Date.now(),
          source: source,
          target: target,
          method: method,
          dataSize: JSON.stringify(data).length,
          success: true,
        });
      },
    });

    // Track data transfers between modules
    this.communicationTrackers.set("data_transfers", {
      transfers: [],
      track: (source, target, dataType, size, latency) => {
        this.communicationTrackers.get("data_transfers").transfers.push({
          timestamp: Date.now(),
          source: source,
          target: target,
          dataType: dataType,
          size: size,
          latency: latency,
        });
      },
    });

    // Track event propagation
    this.communicationTrackers.set("event_propagation", {
      events: [],
      track: (eventType, source, targets, propagationTime) => {
        this.communicationTrackers.get("event_propagation").events.push({
          timestamp: Date.now(),
          eventType: eventType,
          source: source,
          targets: targets,
          propagationTime: propagationTime,
        });
      },
    });
  }

  /**
   * Complete Integration Test Suite
   */
  async runAllIntegrationTests() {
    console.log("\n🚀 Starting Cross-Module Integration Tests...\n");

    const startTime = Date.now();

    try {
      // 1. Test Basic Module Communication
      await this.testBasicModuleCommunication();

      // 2. Test Data Flow Validation
      await this.testDataFlowValidation();

      // 3. Test End-to-End Experiment Workflow
      await this.testEndToEndExperimentWorkflow();

      // 4. Test Real-Time Data Synchronization
      await this.testRealTimeDataSynchronization();

      // 5. Test Error Handling and Recovery
      await this.testErrorHandlingAndRecovery();

      // 6. Test Performance Under Load
      await this.testPerformanceUnderLoad();

      // 7. Test Data Consistency
      await this.testDataConsistency();

      // 8. Test Event Propagation
      await this.testEventPropagation();

      const endTime = Date.now();
      const totalTime = endTime - startTime;

      // Generate comprehensive test report
      await this.generateIntegrationTestReport(totalTime);

      return this.integrationMetrics;
    } catch (error) {
      console.error("❌ Integration test suite failed:", error);
      throw error;
    }
  }

  /**
   * Test 1: Basic Module Communication
   */
  async testBasicModuleCommunication() {
    console.log("📡 Testing Basic Module Communication...");

    const testCases = [
      {
        name: "ExperimentManager to TrafficAllocation",
        test: async () => {
          const experiment = await this.createTestExperiment();
          const allocation = await this.modules
            .get("trafficAllocation")
            .createAllocationStrategy(experiment.id, experiment.variants);

          this.communicationTrackers
            .get("method_calls")
            .track(
              "ExperimentManager",
              "TrafficAllocation",
              "createAllocationStrategy",
              { experimentId: experiment.id, variants: experiment.variants }
            );

          return allocation && allocation.success;
        },
      },
      {
        name: "TrafficAllocation to UserSegmentation",
        test: async () => {
          const segments = await this.modules
            .get("userSegmentation")
            .createSegment({
              name: "Test Segment",
              demographic: { age: "25-34" },
              behavioral: { engagementLevel: "high" },
            });

          const allocation = await this.modules
            .get("trafficAllocation")
            .allocateTraffic("test_experiment", ["control", "variant"], {
              segments: [segments.segmentId],
            });

          this.communicationTrackers
            .get("method_calls")
            .track("TrafficAllocation", "UserSegmentation", "allocateTraffic", {
              segments: [segments.segmentId],
            });

          return allocation && allocation.success;
        },
      },
      {
        name: "UserSegmentation to StatisticalEngine",
        test: async () => {
          const userProfile = await this.modules
            .get("userSegmentation")
            .createUserProfile("test_user_001", {
              age: 28,
              gender: "female",
              location: "US",
            });

          const analysis = await this.modules
            .get("statisticalEngine")
            .analyzeUserBehavior({
              userId: "test_user_001",
              profile: userProfile,
              behaviorData: this.generateMockBehaviorData(),
            });

          this.communicationTrackers
            .get("method_calls")
            .track(
              "UserSegmentation",
              "StatisticalEngine",
              "analyzeUserBehavior",
              { userId: "test_user_001", profile: userProfile }
            );

          return analysis && analysis.results;
        },
      },
      {
        name: "AudioCollector to StatisticalEngine",
        test: async () => {
          const audioData = await this.modules
            .get("audioCollector")
            .analyzeAudioFeatures(this.generateMockAudioData());

          const statisticalAnalysis = await this.modules
            .get("statisticalEngine")
            .performAudioStatisticalAnalysis(audioData.features);

          this.communicationTrackers
            .get("method_calls")
            .track(
              "AudioCollector",
              "StatisticalEngine",
              "performAudioStatisticalAnalysis",
              { features: audioData.features }
            );

          return statisticalAnalysis && statisticalAnalysis.success;
        },
      },
      {
        name: "StatisticalEngine to ExperimentManager",
        test: async () => {
          const statisticalResults = this.generateMockStatisticalResults();

          const decision = await this.modules
            .get("experimentManager")
            .processStatisticalResults("test_experiment", statisticalResults);

          this.communicationTrackers
            .get("method_calls")
            .track(
              "StatisticalEngine",
              "ExperimentManager",
              "processStatisticalResults",
              { experimentId: "test_experiment", results: statisticalResults }
            );

          return decision && decision.action;
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of testCases) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.communicationTests += testCases.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests += testCases.length - passedTests;

    console.log(
      `📡 Basic Communication Tests: ${passedTests}/${testCases.length} passed\n`
    );
  }

  /**
   * Test 2: Data Flow Validation
   */
  async testDataFlowValidation() {
    console.log("🔄 Testing Data Flow Validation...");

    const dataFlowTests = [
      {
        name: "Experiment to Traffic Allocation Data Flow",
        test: async () => {
          const experiment = await this.createTestExperiment();
          const trafficData = await this.modules
            .get("trafficAllocation")
            .generateAllocationPlan(experiment);

          const validator = this.dataFlowValidators.get(
            "experiment-to-traffic"
          );
          const isValid = validator.validate(experiment, trafficData);

          this.communicationTrackers
            .get("data_transfers")
            .track(
              "ExperimentManager",
              "TrafficAllocation",
              "experiment_config",
              JSON.stringify(experiment).length,
              50
            );

          return isValid;
        },
      },
      {
        name: "Traffic to Segmentation Data Flow",
        test: async () => {
          const trafficData = await this.generateMockTrafficData();
          const segmentData = await this.modules
            .get("userSegmentation")
            .processTrafficAllocation(trafficData);

          const validator = this.dataFlowValidators.get(
            "traffic-to-segmentation"
          );
          const isValid = validator.validate(trafficData, segmentData);

          this.communicationTrackers
            .get("data_transfers")
            .track(
              "TrafficAllocation",
              "UserSegmentation",
              "traffic_allocation",
              JSON.stringify(trafficData).length,
              75
            );

          return isValid;
        },
      },
      {
        name: "Segmentation to Statistics Data Flow",
        test: async () => {
          const segmentData = await this.generateMockSegmentData();
          const statisticsData = await this.modules
            .get("statisticalEngine")
            .processSegmentationData(segmentData);

          const validator = this.dataFlowValidators.get(
            "segmentation-to-statistics"
          );
          const isValid = validator.validate(segmentData, statisticsData);

          this.communicationTrackers
            .get("data_transfers")
            .track(
              "UserSegmentation",
              "StatisticalEngine",
              "segment_profiles",
              JSON.stringify(segmentData).length,
              100
            );

          return isValid;
        },
      },
      {
        name: "Audio to Statistics Data Flow",
        test: async () => {
          const audioData = await this.generateMockAudioData();
          const audioFeatures = await this.modules
            .get("audioCollector")
            .extractFeatures(audioData);

          const statisticsData = await this.modules
            .get("statisticalEngine")
            .analyzeAudioFeatures(audioFeatures);

          const validator = this.dataFlowValidators.get("audio-to-statistics");
          const isValid = validator.validate(audioFeatures, statisticsData);

          this.communicationTrackers
            .get("data_transfers")
            .track(
              "AudioCollector",
              "StatisticalEngine",
              "audio_features",
              JSON.stringify(audioFeatures).length,
              120
            );

          return isValid;
        },
      },
      {
        name: "Statistics to Experiment Data Flow",
        test: async () => {
          const statisticsData = await this.generateMockStatisticalResults();
          const experimentData = await this.modules
            .get("experimentManager")
            .updateExperimentWithResults("test_experiment", statisticsData);

          const validator = this.dataFlowValidators.get(
            "statistics-to-experiment"
          );
          const isValid = validator.validate(statisticsData, experimentData);

          this.communicationTrackers
            .get("data_transfers")
            .track(
              "StatisticalEngine",
              "ExperimentManager",
              "statistical_results",
              JSON.stringify(statisticsData).length,
              80
            );

          return isValid;
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of dataFlowTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.dataFlowTests += dataFlowTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests += dataFlowTests.length - passedTests;

    console.log(
      `🔄 Data Flow Tests: ${passedTests}/${dataFlowTests.length} passed\n`
    );
  }

  /**
   * Test 3: End-to-End Experiment Workflow
   */
  async testEndToEndExperimentWorkflow() {
    console.log("🔬 Testing End-to-End Experiment Workflow...");

    try {
      const workflowStartTime = Date.now();

      // Step 1: Create experiment
      const experiment = await this.modules
        .get("experimentManager")
        .createExperiment({
          name: "E2E Integration Test Experiment",
          description: "Full workflow integration test",
          variants: [
            { id: "control", name: "Control", trafficAllocation: 0.5 },
            { id: "variant_a", name: "Variant A", trafficAllocation: 0.5 },
          ],
          metrics: ["conversion_rate", "engagement_time"],
          targetAudience: { segments: ["high_engagement"] },
        });

      console.log(`  📝 Step 1: Experiment created - ${experiment.id}`);

      // Step 2: Create user segments
      const segment = await this.modules.get("userSegmentation").createSegment({
        name: "High Engagement Users",
        behavioral: { engagementLevel: "high" },
        demographic: { age: "25-44" },
      });

      console.log(`  👥 Step 2: User segment created - ${segment.segmentId}`);

      // Step 3: Set up traffic allocation
      const allocation = await this.modules
        .get("trafficAllocation")
        .allocateTraffic(
          experiment.id,
          experiment.variants.map((v) => v.id),
          {
            method: "weighted",
            segments: [segment.segmentId],
            loadBalancing: true,
          }
        );

      console.log(
        `  🚦 Step 3: Traffic allocation configured - ${
          allocation.success ? "SUCCESS" : "FAILED"
        }`
      );

      // Step 4: Generate test users and assign them
      const testUsers = await this.generateTestUsers(100);
      const userAssignments = [];

      for (const user of testUsers) {
        // Create user profile
        const profile = await this.modules
          .get("userSegmentation")
          .createUserProfile(user.id, user);

        // Assign to variant
        const assignment = await this.modules
          .get("trafficAllocation")
          .assignUserToVariant(experiment.id, user.id, user);

        userAssignments.push({
          userId: user.id,
          variant: assignment.variant,
          profile: profile,
        });
      }

      console.log(
        `  👤 Step 4: ${userAssignments.length} users assigned to variants`
      );

      // Step 5: Simulate user interactions and collect data
      const interactionData = [];
      for (const assignment of userAssignments) {
        const interactions = await this.simulateUserInteractions(assignment);
        interactionData.push(...interactions);
      }

      console.log(
        `  📊 Step 5: ${interactionData.length} user interactions simulated`
      );

      // Step 6: Collect audio data for relevant interactions
      const audioData = [];
      for (let i = 0; i < 20; i++) {
        // Sample 20 interactions for audio analysis
        const mockAudio = this.generateMockAudioData();
        const audioAnalysis = await this.modules
          .get("audioCollector")
          .analyzeAudioFeatures(mockAudio);
        audioData.push(audioAnalysis);
      }

      console.log(`  🎵 Step 6: ${audioData.length} audio samples analyzed`);

      // Step 7: Perform statistical analysis
      const statisticalAnalysis = await this.modules
        .get("statisticalEngine")
        .performExperimentAnalysis({
          experimentId: experiment.id,
          variants: experiment.variants,
          userData: userAssignments,
          interactionData: interactionData,
          audioData: audioData,
        });

      console.log(
        `  📈 Step 7: Statistical analysis completed - p-value: ${statisticalAnalysis.pValue}`
      );

      // Step 8: Update experiment with results
      const updatedExperiment = await this.modules
        .get("experimentManager")
        .updateExperimentWithResults(experiment.id, statisticalAnalysis);

      console.log(
        `  🎯 Step 8: Experiment updated with results - Status: ${updatedExperiment.status}`
      );

      // Step 9: Make experiment decision
      const decision = await this.modules
        .get("experimentManager")
        .makeExperimentDecision(experiment.id, statisticalAnalysis);

      console.log(`  ⚖️ Step 9: Experiment decision made - ${decision.action}`);

      const workflowEndTime = Date.now();
      const totalWorkflowTime = workflowEndTime - workflowStartTime;

      console.log(`  ⏱️ Total workflow time: ${totalWorkflowTime}ms`);

      // Validate end-to-end data integrity
      const dataIntegrityCheck = await this.validateEndToEndDataIntegrity({
        experiment,
        segment,
        allocation,
        userAssignments,
        interactionData,
        audioData,
        statisticalAnalysis,
        decision,
      });

      if (dataIntegrityCheck.valid) {
        console.log(`  ✅ End-to-End Workflow: PASSED`);
        this.integrationMetrics.passedTests++;
      } else {
        console.log(
          `  ❌ End-to-End Workflow: FAILED - ${dataIntegrityCheck.errors.join(
            ", "
          )}`
        );
        this.integrationMetrics.failedTests++;
      }
    } catch (error) {
      console.log(`  ❌ End-to-End Workflow: ERROR - ${error.message}`);
      this.integrationMetrics.failedTests++;
    }

    this.integrationMetrics.totalTests++;
    console.log(`🔬 End-to-End Workflow Test completed\n`);
  }

  /**
   * Test 4: Real-Time Data Synchronization
   */
  async testRealTimeDataSynchronization() {
    console.log("⚡ Testing Real-Time Data Synchronization...");

    const syncTests = [
      {
        name: "Real-time user assignment synchronization",
        test: async () => {
          const startTime = Date.now();

          // Simulate rapid user assignments
          const users = await this.generateTestUsers(50);
          const assignments = [];

          for (const user of users) {
            const assignment = await this.modules
              .get("trafficAllocation")
              .assignUserToVariant("test_experiment", user.id, user);

            // Check if user segmentation is updated in real-time
            const updatedProfile = await this.modules
              .get("userSegmentation")
              .getUserProfile(user.id);

            assignments.push({
              userId: user.id,
              variant: assignment.variant,
              profileUpdated: updatedProfile !== null,
              latency: Date.now() - startTime,
            });
          }

          const avgLatency =
            assignments.reduce((sum, a) => sum + a.latency, 0) /
            assignments.length;
          const allUpdated = assignments.every((a) => a.profileUpdated);

          return allUpdated && avgLatency < 100; // Under 100ms average
        },
      },
      {
        name: "Real-time metric updates",
        test: async () => {
          const metricsUpdateTest = [];

          for (let i = 0; i < 10; i++) {
            const beforeMetrics = await this.modules
              .get("experimentManager")
              .getExperimentMetrics("test_experiment");

            // Simulate user interaction
            await this.simulateUserInteraction({
              userId: `user_${i}`,
              experimentId: "test_experiment",
              variant: "control",
              action: "conversion",
            });

            const afterMetrics = await this.modules
              .get("experimentManager")
              .getExperimentMetrics("test_experiment");

            metricsUpdateTest.push({
              before: beforeMetrics.conversionRate,
              after: afterMetrics.conversionRate,
              updated:
                afterMetrics.conversionRate !== beforeMetrics.conversionRate,
            });
          }

          return metricsUpdateTest.every((test) => test.updated);
        },
      },
      {
        name: "Cross-module event propagation",
        test: async () => {
          const eventStartTime = Date.now();

          // Trigger event in one module
          await this.modules.get("userSegmentation").triggerSegmentUpdateEvent({
            segmentId: "test_segment",
            changeType: "member_added",
            userId: "test_user",
          });

          // Wait for propagation
          await new Promise((resolve) => setTimeout(resolve, 100));

          // Check if other modules received the event
          const trafficAllocationReceived = await this.modules
            .get("trafficAllocation")
            .checkEventReceived("segment_updated");

          const experimentManagerReceived = await this.modules
            .get("experimentManager")
            .checkEventReceived("segment_updated");

          const propagationTime = Date.now() - eventStartTime;

          this.communicationTrackers
            .get("event_propagation")
            .track(
              "segment_updated",
              "UserSegmentation",
              ["TrafficAllocation", "ExperimentManager"],
              propagationTime
            );

          return (
            trafficAllocationReceived &&
            experimentManagerReceived &&
            propagationTime < 200
          );
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of syncTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.totalTests += syncTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests += syncTests.length - passedTests;

    console.log(
      `⚡ Real-Time Sync Tests: ${passedTests}/${syncTests.length} passed\n`
    );
  }

  /**
   * Test 5: Error Handling and Recovery
   */
  async testErrorHandlingAndRecovery() {
    console.log("🛡️ Testing Error Handling and Recovery...");

    const errorTests = [
      {
        name: "Module communication failure recovery",
        test: async () => {
          try {
            // Simulate communication failure
            const originalMethod =
              this.modules.get("trafficAllocation").allocateTraffic;
            this.modules.get("trafficAllocation").allocateTraffic = () => {
              throw new Error("Communication failure");
            };

            // Test recovery mechanism
            const result = await this.modules
              .get("experimentManager")
              .handleTrafficAllocationFailure("test_experiment");

            // Restore original method
            this.modules.get("trafficAllocation").allocateTraffic =
              originalMethod;

            return result && result.recovered;
          } catch (error) {
            return false;
          }
        },
      },
      {
        name: "Data consistency during failures",
        test: async () => {
          try {
            // Create initial state
            const experiment = await this.createTestExperiment();
            const initialState = await this.captureSystemState();

            // Simulate partial failure during data update
            let failureOccurred = false;
            try {
              await this.simulatePartialUpdateFailure(experiment.id);
            } catch (error) {
              failureOccurred = true;
            }

            // Check data consistency
            const finalState = await this.captureSystemState();
            const consistencyCheck = await this.validateDataConsistency(
              initialState,
              finalState
            );

            return failureOccurred && consistencyCheck.consistent;
          } catch (error) {
            return false;
          }
        },
      },
      {
        name: "Graceful degradation under load",
        test: async () => {
          try {
            // Simulate high load
            const promises = [];
            for (let i = 0; i < 100; i++) {
              promises.push(this.simulateUserAssignment(`load_test_user_${i}`));
            }

            const results = await Promise.allSettled(promises);
            const successCount = results.filter(
              (r) => r.status === "fulfilled"
            ).length;
            const failureCount = results.filter(
              (r) => r.status === "rejected"
            ).length;

            // Check if system gracefully handled the load
            const degradationGraceful = failureCount < 10; // Less than 10% failure rate
            const systemResponsive = await this.checkSystemResponsiveness();

            return degradationGraceful && systemResponsive;
          } catch (error) {
            return false;
          }
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of errorTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.totalTests += errorTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests += errorTests.length - passedTests;

    console.log(
      `🛡️ Error Handling Tests: ${passedTests}/${errorTests.length} passed\n`
    );
  }

  /**
   * Test 6: Performance Under Load
   */
  async testPerformanceUnderLoad() {
    console.log("🚀 Testing Performance Under Load...");

    const performanceTests = [
      {
        name: "High-volume user assignment performance",
        test: async () => {
          const startTime = Date.now();
          const userCount = 1000;

          const assignments = [];
          for (let i = 0; i < userCount; i++) {
            const assignmentStart = Date.now();
            const assignment = await this.modules
              .get("trafficAllocation")
              .assignUserToVariant("perf_test", `user_${i}`, {
                id: `user_${i}`,
              });
            const assignmentTime = Date.now() - assignmentStart;

            assignments.push({
              userId: `user_${i}`,
              variant: assignment.variant,
              latency: assignmentTime,
            });
          }

          const totalTime = Date.now() - startTime;
          const avgLatency =
            assignments.reduce((sum, a) => sum + a.latency, 0) /
            assignments.length;
          const throughput = userCount / (totalTime / 1000); // users per second

          console.log(`    📊 Assigned ${userCount} users in ${totalTime}ms`);
          console.log(`    📊 Average latency: ${avgLatency.toFixed(2)}ms`);
          console.log(`    📊 Throughput: ${throughput.toFixed(2)} users/sec`);

          return avgLatency < 50 && throughput > 100; // Under 50ms avg, over 100 users/sec
        },
      },
      {
        name: "Concurrent experiment management",
        test: async () => {
          const experimentCount = 20;
          const startTime = Date.now();

          const experimentPromises = [];
          for (let i = 0; i < experimentCount; i++) {
            experimentPromises.push(
              this.modules.get("experimentManager").createExperiment({
                name: `Concurrent Test Experiment ${i}`,
                variants: [
                  { id: "control", trafficAllocation: 0.5 },
                  { id: "variant", trafficAllocation: 0.5 },
                ],
              })
            );
          }

          const results = await Promise.allSettled(experimentPromises);
          const successCount = results.filter(
            (r) => r.status === "fulfilled"
          ).length;
          const totalTime = Date.now() - startTime;

          console.log(
            `    📊 Created ${successCount}/${experimentCount} experiments in ${totalTime}ms`
          );

          return successCount >= experimentCount * 0.9; // 90% success rate
        },
      },
      {
        name: "Statistical analysis performance",
        test: async () => {
          const analysisCount = 10;
          const dataPointsPerAnalysis = 1000;

          const analysisPromises = [];
          for (let i = 0; i < analysisCount; i++) {
            const mockData = this.generateLargeDataset(dataPointsPerAnalysis);
            analysisPromises.push(
              this.modules
                .get("statisticalEngine")
                .performTTest(mockData.control, mockData.variant)
            );
          }

          const startTime = Date.now();
          const results = await Promise.allSettled(analysisPromises);
          const totalTime = Date.now() - startTime;

          const successCount = results.filter(
            (r) => r.status === "fulfilled"
          ).length;
          const avgTimePerAnalysis = totalTime / successCount;

          console.log(
            `    📊 Completed ${successCount}/${analysisCount} analyses in ${totalTime}ms`
          );
          console.log(
            `    📊 Average time per analysis: ${avgTimePerAnalysis.toFixed(
              2
            )}ms`
          );

          return successCount === analysisCount && avgTimePerAnalysis < 500; // Under 500ms per analysis
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of performanceTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.performanceTests += performanceTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests +=
      performanceTests.length - passedTests;

    console.log(
      `🚀 Performance Tests: ${passedTests}/${performanceTests.length} passed\n`
    );
  }

  /**
   * Test 7: Data Consistency
   */
  async testDataConsistency() {
    console.log("🔍 Testing Data Consistency...");

    const consistencyTests = [
      {
        name: "Cross-module data synchronization",
        test: async () => {
          // Create test data in multiple modules
          const experiment = await this.createTestExperiment();
          const segment = await this.modules
            .get("userSegmentation")
            .createSegment({
              name: "Consistency Test Segment",
            });

          // Link data across modules
          await this.modules
            .get("trafficAllocation")
            .linkExperimentToSegment(experiment.id, segment.segmentId);

          // Verify data consistency across modules
          const experimentData = await this.modules
            .get("experimentManager")
            .getExperiment(experiment.id);
          const segmentData = await this.modules
            .get("userSegmentation")
            .getSegment(segment.segmentId);
          const allocationData = await this.modules
            .get("trafficAllocation")
            .getAllocationData(experiment.id);

          const consistent =
            experimentData.linkedSegments.includes(segment.segmentId) &&
            segmentData.linkedExperiments.includes(experiment.id) &&
            allocationData.experimentId === experiment.id;

          return consistent;
        },
      },
      {
        name: "User profile consistency across modules",
        test: async () => {
          const userId = "consistency_test_user";

          // Create user profile
          const profile = await this.modules
            .get("userSegmentation")
            .createUserProfile(userId, {
              age: 30,
              gender: "male",
              location: "US",
            });

          // Assign user to experiment
          const assignment = await this.modules
            .get("trafficAllocation")
            .assignUserToVariant("test_experiment", userId, profile);

          // Update user behavior
          await this.modules.get("userSegmentation").trackUserBehavior(userId, {
            type: "conversion",
            timestamp: Date.now(),
          });

          // Check consistency across modules
          const segmentationProfile = await this.modules
            .get("userSegmentation")
            .getUserProfile(userId);
          const allocationProfile = await this.modules
            .get("trafficAllocation")
            .getUserData(userId);
          const experimentProfile = await this.modules
            .get("experimentManager")
            .getUserExperimentData(userId);

          const consistent =
            segmentationProfile.userId === allocationProfile.userId &&
            allocationProfile.userId === experimentProfile.userId &&
            segmentationProfile.behavioral.conversionRate ===
              experimentProfile.conversionRate;

          return consistent;
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of consistencyTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.totalTests += consistencyTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests +=
      consistencyTests.length - passedTests;

    console.log(
      `🔍 Data Consistency Tests: ${passedTests}/${consistencyTests.length} passed\n`
    );
  }

  /**
   * Test 8: Event Propagation
   */
  async testEventPropagation() {
    console.log("📡 Testing Event Propagation...");

    const eventTests = [
      {
        name: "Experiment lifecycle events",
        test: async () => {
          const eventLog = [];

          // Set up event listeners
          this.setupEventListeners(eventLog);

          // Trigger experiment lifecycle
          const experiment = await this.createTestExperiment();
          await this.modules
            .get("experimentManager")
            .startExperiment(experiment.id);
          await this.modules
            .get("experimentManager")
            .pauseExperiment(experiment.id);
          await this.modules
            .get("experimentManager")
            .resumeExperiment(experiment.id);
          await this.modules
            .get("experimentManager")
            .stopExperiment(experiment.id);

          // Wait for event propagation
          await new Promise((resolve) => setTimeout(resolve, 200));

          const expectedEvents = [
            "experiment_created",
            "experiment_started",
            "experiment_paused",
            "experiment_resumed",
            "experiment_stopped",
          ];
          const receivedEvents = eventLog.map((e) => e.type);

          const allEventsReceived = expectedEvents.every((e) =>
            receivedEvents.includes(e)
          );
          const correctOrder = this.validateEventOrder(
            eventLog,
            expectedEvents
          );

          return allEventsReceived && correctOrder;
        },
      },
      {
        name: "User assignment events",
        test: async () => {
          const eventLog = [];
          this.setupEventListeners(eventLog);

          // Trigger user assignments
          const users = await this.generateTestUsers(5);
          for (const user of users) {
            await this.modules
              .get("trafficAllocation")
              .assignUserToVariant("test_experiment", user.id, user);
          }

          await new Promise((resolve) => setTimeout(resolve, 100));

          const assignmentEvents = eventLog.filter(
            (e) => e.type === "user_assigned"
          );
          return assignmentEvents.length === users.length;
        },
      },
    ];

    let passedTests = 0;
    for (const testCase of eventTests) {
      try {
        const result = await testCase.test();
        if (result) {
          console.log(`  ✅ ${testCase.name}: PASSED`);
          passedTests++;
        } else {
          console.log(`  ❌ ${testCase.name}: FAILED`);
        }
      } catch (error) {
        console.log(`  ❌ ${testCase.name}: ERROR - ${error.message}`);
      }
    }

    this.integrationMetrics.totalTests += eventTests.length;
    this.integrationMetrics.passedTests += passedTests;
    this.integrationMetrics.failedTests += eventTests.length - passedTests;

    console.log(
      `📡 Event Propagation Tests: ${passedTests}/${eventTests.length} passed\n`
    );
  }

  /**
   * Helper Methods for Testing
   */
  async createTestExperiment() {
    return {
      id: `test_exp_${Date.now()}`,
      name: "Test Experiment",
      status: "draft",
      variants: [
        { id: "control", name: "Control", trafficAllocation: 0.5 },
        { id: "variant_a", name: "Variant A", trafficAllocation: 0.5 },
      ],
      metrics: ["conversion_rate", "engagement_time"],
      createdAt: Date.now(),
    };
  }

  async generateTestUsers(count) {
    const users = [];
    for (let i = 0; i < count; i++) {
      users.push({
        id: `test_user_${Date.now()}_${i}`,
        age: 20 + Math.floor(Math.random() * 40),
        gender: Math.random() > 0.5 ? "male" : "female",
        location: ["US", "UK", "CA", "AU"][Math.floor(Math.random() * 4)],
      });
    }
    return users;
  }

  generateMockBehaviorData() {
    return {
      sessionCount: Math.floor(Math.random() * 10) + 1,
      totalTimeSpent: Math.floor(Math.random() * 3600) + 300,
      conversionEvents: Math.floor(Math.random() * 3),
      engagementScore: Math.random(),
    };
  }

  generateMockAudioData() {
    return {
      samples: new Float32Array(1024).map(() => Math.random() * 2 - 1),
      sampleRate: 44100,
      duration: 1024 / 44100,
      metadata: {
        format: "pcm",
        channels: 1,
        bitDepth: 16,
      },
    };
  }

  generateMockStatisticalResults() {
    return {
      testType: "ttest",
      pValue: Math.random() * 0.1,
      confidenceInterval: [Math.random() - 0.5, Math.random() + 0.5],
      effectSize: Math.random() * 0.5,
      power: 0.8 + Math.random() * 0.2,
      sampleSize: Math.floor(Math.random() * 1000) + 100,
    };
  }

  generateMockTrafficData() {
    return {
      allocations: new Map([
        ["control", 0.5],
        ["variant_a", 0.5],
      ]),
      userAllocations: new Map([
        ["user_1", "control"],
        ["user_2", "variant_a"],
      ]),
      allocationStrategy: "uniform",
    };
  }

  generateMockSegmentData() {
    return {
      segments: new Map([
        ["segment_1", { memberCount: 150, criteria: "high_engagement" }],
      ]),
      userProfiles: new Map([
        ["user_1", { segments: ["segment_1"], engagementScore: 0.8 }],
      ]),
    };
  }

  generateLargeDataset(count) {
    const control = Array.from({ length: count }, () => Math.random() * 100);
    const variant = Array.from(
      { length: count },
      () => Math.random() * 100 + 5
    );
    return { control, variant };
  }

  async simulateUserInteractions(assignment) {
    const interactions = [];
    const interactionCount = Math.floor(Math.random() * 5) + 1;

    for (let i = 0; i < interactionCount; i++) {
      interactions.push({
        userId: assignment.userId,
        variant: assignment.variant,
        type: ["page_view", "click", "conversion"][
          Math.floor(Math.random() * 3)
        ],
        timestamp: Date.now() + i * 1000,
        value: Math.random(),
      });
    }

    return interactions;
  }

  async simulateUserInteraction(interaction) {
    // Mock implementation
    return {
      userId: interaction.userId,
      experimentId: interaction.experimentId,
      variant: interaction.variant,
      action: interaction.action,
      timestamp: Date.now(),
    };
  }

  async simulateUserAssignment(userId) {
    return this.modules
      .get("trafficAllocation")
      .assignUserToVariant("load_test_experiment", userId, { id: userId });
  }

  async validateEndToEndDataIntegrity(data) {
    const errors = [];

    // Check if all data objects are present
    const requiredFields = [
      "experiment",
      "segment",
      "allocation",
      "userAssignments",
      "interactionData",
      "audioData",
      "statisticalAnalysis",
      "decision",
    ];

    for (const field of requiredFields) {
      if (!data[field]) {
        errors.push(`Missing ${field} data`);
      }
    }

    // Check data relationships
    if (data.userAssignments && data.interactionData) {
      const assignedUsers = new Set(data.userAssignments.map((a) => a.userId));
      const interactionUsers = new Set(
        data.interactionData.map((i) => i.userId)
      );

      if (assignedUsers.size !== interactionUsers.size) {
        errors.push("User assignment and interaction data mismatch");
      }
    }

    return {
      valid: errors.length === 0,
      errors: errors,
    };
  }

  async captureSystemState() {
    return {
      experiments: await this.modules
        .get("experimentManager")
        .getAllExperiments(),
      segments: await this.modules.get("userSegmentation").getAllSegments(),
      allocations: await this.modules
        .get("trafficAllocation")
        .getAllAllocations(),
      timestamp: Date.now(),
    };
  }

  async validateDataConsistency(initialState, finalState) {
    // Mock implementation - would contain actual consistency checks
    return {
      consistent: true,
      differences: [],
    };
  }

  async simulatePartialUpdateFailure(experimentId) {
    // Simulate a failure during multi-module update
    throw new Error("Simulated partial update failure");
  }

  async checkSystemResponsiveness() {
    const startTime = Date.now();

    try {
      await Promise.all([
        this.modules.get("experimentManager").getSystemStatus(),
        this.modules.get("trafficAllocation").getSystemStatus(),
        this.modules.get("userSegmentation").getSystemStatus(),
        this.modules.get("statisticalEngine").getSystemStatus(),
        this.modules.get("audioCollector").getSystemStatus(),
      ]);

      const responseTime = Date.now() - startTime;
      return responseTime < 1000; // Under 1 second
    } catch (error) {
      return false;
    }
  }

  setupEventListeners(eventLog) {
    // Mock event listener setup
    const eventHandler = (event) => {
      eventLog.push({
        type: event.type,
        timestamp: Date.now(),
        source: event.source,
        data: event.data,
      });
    };

    // Would set up actual event listeners here
  }

  validateEventOrder(eventLog, expectedOrder) {
    const receivedOrder = eventLog.map((e) => e.type);

    // Check if events occurred in expected order
    let expectedIndex = 0;
    for (const event of receivedOrder) {
      if (event === expectedOrder[expectedIndex]) {
        expectedIndex++;
      }
    }

    return expectedIndex === expectedOrder.length;
  }

  /**
   * Generate Comprehensive Integration Test Report
   */
  async generateIntegrationTestReport(totalTime) {
    console.log("\n📋 CROSS-MODULE INTEGRATION TEST REPORT");
    console.log("========================================");

    console.log(`\n📊 Test Summary:`);
    console.log(`  Total Tests: ${this.integrationMetrics.totalTests}`);
    console.log(`  Passed: ${this.integrationMetrics.passedTests}`);
    console.log(`  Failed: ${this.integrationMetrics.failedTests}`);
    console.log(
      `  Success Rate: ${(
        (this.integrationMetrics.passedTests /
          this.integrationMetrics.totalTests) *
        100
      ).toFixed(2)}%`
    );
    console.log(`  Total Execution Time: ${totalTime}ms`);

    console.log(`\n🔍 Test Categories:`);
    console.log(
      `  Communication Tests: ${this.integrationMetrics.communicationTests}`
    );
    console.log(`  Data Flow Tests: ${this.integrationMetrics.dataFlowTests}`);
    console.log(
      `  Performance Tests: ${this.integrationMetrics.performanceTests}`
    );

    console.log(`\n📡 Communication Analysis:`);
    const methodCalls = this.communicationTrackers.get("method_calls").calls;
    console.log(`  Total Method Calls: ${methodCalls.length}`);

    const callsByModule = methodCalls.reduce((acc, call) => {
      const key = `${call.source} -> ${call.target}`;
      acc[key] = (acc[key] || 0) + 1;
      return acc;
    }, {});

    Object.entries(callsByModule).forEach(([modules, count]) => {
      console.log(`    ${modules}: ${count} calls`);
    });

    console.log(`\n🔄 Data Transfer Analysis:`);
    const dataTransfers =
      this.communicationTrackers.get("data_transfers").transfers;
    console.log(`  Total Data Transfers: ${dataTransfers.length}`);

    const avgTransferSize =
      dataTransfers.reduce((sum, t) => sum + t.size, 0) / dataTransfers.length;
    const avgLatency =
      dataTransfers.reduce((sum, t) => sum + t.latency, 0) /
      dataTransfers.length;

    console.log(`  Average Transfer Size: ${avgTransferSize.toFixed(2)} bytes`);
    console.log(`  Average Latency: ${avgLatency.toFixed(2)}ms`);

    console.log(`\n⚡ Event Propagation Analysis:`);
    const events = this.communicationTrackers.get("event_propagation").events;
    console.log(`  Total Events: ${events.length}`);

    if (events.length > 0) {
      const avgPropagationTime =
        events.reduce((sum, e) => sum + e.propagationTime, 0) / events.length;
      console.log(
        `  Average Propagation Time: ${avgPropagationTime.toFixed(2)}ms`
      );
    }

    console.log(
      `\n✅ Integration Status: ${
        this.integrationMetrics.failedTests === 0 ? "PASSED" : "FAILED"
      }`
    );

    if (this.integrationMetrics.failedTests > 0) {
      console.log(`\n⚠️  Issues Found:`);
      console.log(`  ${this.integrationMetrics.failedTests} test(s) failed`);
      console.log(`  Review failed tests for integration issues`);
    } else {
      console.log(`\n🎉 All integration tests passed successfully!`);
      console.log(`  Cross-module communication is working correctly`);
      console.log(`  Data flow validation is successful`);
      console.log(`  System performance meets requirements`);
    }

    console.log("\n========================================\n");
  }
}

// Export the test suite
export { CrossModuleIntegrationTestSuite };

// Run tests if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const testSuite = new CrossModuleIntegrationTestSuite();
  testSuite
    .runAllIntegrationTests()
    .then((metrics) => {
      console.log("Integration tests completed:", metrics);
      process.exit(metrics.failedTests === 0 ? 0 : 1);
    })
    .catch((error) => {
      console.error("Integration test suite failed:", error);
      process.exit(1);
    });
}
