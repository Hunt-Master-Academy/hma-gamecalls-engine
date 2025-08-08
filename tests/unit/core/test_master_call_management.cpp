/**
 * @file test_master_call_management.cpp
 * @brief Comprehensive test suite for UnifiedAudioEngine master call management
 *
 * Tests all master call related API methods of the UnifiedAudioEngine:
 * - loadMasterCall per session
 * - unloadMasterCall
 * - getCurrentMasterCall
 * - Master call feature extraction and validation
 * - Per-session master call isolation
 * - Error conditions and edge cases
 *
 * This test focuses specifically on the master call management API and ensures
 * proper per-session isolation of master calls.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date August 7, 2025
 */

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MasterCallManagementTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create engine";
        engine = std::move(engineResult.value);

        // Create test session
        auto sessionResult = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create test session";
        sessionId = sessionResult.value;

        // Ensure test data directories exist and create test master calls
        setupTestMasterCalls();
    }

    void TearDown() override {
        if (engine && sessionId != INVALID_SESSION_ID) {
            [[maybe_unused]] auto status = engine->destroySession(sessionId);
        }
        // Clean up test files
        cleanupTestMasterCalls();
    }

    void setupTestMasterCalls() {
        // Create test master call directories if they don't exist
        std::filesystem::create_directories(MASTER_CALLS_PATH);
        std::filesystem::create_directories(FEATURES_PATH);

        // Create a simple test .mfc file (MFCC features file)
        createTestMFCFile(VALID_MASTER_CALL_ID);
        createTestMFCFile(SECOND_MASTER_CALL_ID);
    }

    void cleanupTestMasterCalls() {
        // Remove test files
        std::filesystem::remove(FEATURES_PATH + VALID_MASTER_CALL_ID + ".mfc");
        std::filesystem::remove(FEATURES_PATH + SECOND_MASTER_CALL_ID + ".mfc");
    }

    void createTestMFCFile(const std::string& masterCallId) {
        std::string filePath = FEATURES_PATH + masterCallId + ".mfc";
        std::ofstream file(filePath, std::ios::binary);

        if (file.is_open()) {
            // Write a simple test MFCC feature set
            // Format: [num_frames][num_coefficients][feature_data...]
            uint32_t numFrames = 10;
            uint32_t numCoefficients = 13;

            file.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
            file.write(reinterpret_cast<const char*>(&numCoefficients), sizeof(numCoefficients));

            // Write test feature data (simple pattern)
            for (uint32_t frame = 0; frame < numFrames; ++frame) {
                for (uint32_t coeff = 0; coeff < numCoefficients; ++coeff) {
                    float value = static_cast<float>(frame * numCoefficients + coeff) * 0.1f;
                    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
                }
            }
            file.close();
        }
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId = INVALID_SESSION_ID;

    static constexpr float TEST_SAMPLE_RATE = 44100.0f;
    static constexpr SessionId INVALID_SESSION_ID = 0;

    // Test master call IDs
    static inline const std::string VALID_MASTER_CALL_ID = "test_call_valid";
    static inline const std::string SECOND_MASTER_CALL_ID = "test_call_second";
    static inline const std::string INVALID_MASTER_CALL_ID = "nonexistent_call";

    // Test data paths (should match UnifiedAudioEngine configuration)
    static inline const std::string MASTER_CALLS_PATH =
        "/workspaces/huntmaster-engine/data/master_calls/";
    static inline const std::string FEATURES_PATH =
        "/workspaces/huntmaster-engine/data/processed_calls/mfc/";
};

// === Basic Master Call Loading Tests ===

TEST_F(MasterCallManagementTest, LoadValidMasterCall) {
    // Initially no master call should be loaded
    auto currentResult = engine->getCurrentMasterCall(sessionId);
    if (currentResult.isOk()) {
        EXPECT_TRUE(currentResult.value.empty());
    }

    // Load valid master call
    auto status = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Failed to load valid master call";

    // Verify master call is loaded
    currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk()) << "Failed to get current master call";
    EXPECT_EQ(currentResult.value, VALID_MASTER_CALL_ID);
}

TEST_F(MasterCallManagementTest, LoadInvalidMasterCall) {
    // Attempt to load non-existent master call
    auto status = engine->loadMasterCall(sessionId, INVALID_MASTER_CALL_ID);
    EXPECT_NE(status, UnifiedAudioEngine::Status::OK) << "Should fail to load invalid master call";

    // Verify no master call is loaded
    auto currentResult = engine->getCurrentMasterCall(sessionId);
    if (currentResult.isOk()) {
        EXPECT_TRUE(currentResult.value.empty());
    }
}

TEST_F(MasterCallManagementTest, UnloadMasterCall) {
    // Load master call first
    auto status = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify it's loaded
    auto currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk());
    EXPECT_EQ(currentResult.value, VALID_MASTER_CALL_ID);

    // Unload master call
    status = engine->unloadMasterCall(sessionId);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify no master call is loaded
    currentResult = engine->getCurrentMasterCall(sessionId);
    if (currentResult.isOk()) {
        EXPECT_TRUE(currentResult.value.empty());
    }
}

TEST_F(MasterCallManagementTest, ReplaceMasterCall) {
    // Load first master call
    auto status = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    auto currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk());
    EXPECT_EQ(currentResult.value, VALID_MASTER_CALL_ID);

    // Load second master call (should replace first)
    status = engine->loadMasterCall(sessionId, SECOND_MASTER_CALL_ID);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify second master call is now loaded
    currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk());
    EXPECT_EQ(currentResult.value, SECOND_MASTER_CALL_ID);
}

// === Multi-Session Master Call Tests ===

TEST_F(MasterCallManagementTest, PerSessionMasterCallIsolation) {
    // Create second session
    auto session2Result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(session2Result.isOk());
    SessionId session2 = session2Result.value;

    // Load different master calls in each session
    auto status1 = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    auto status2 = engine->loadMasterCall(session2, SECOND_MASTER_CALL_ID);

    EXPECT_EQ(status1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(status2, UnifiedAudioEngine::Status::OK);

    // Verify each session has its own master call
    auto current1 = engine->getCurrentMasterCall(sessionId);
    auto current2 = engine->getCurrentMasterCall(session2);

    ASSERT_TRUE(current1.isOk());
    ASSERT_TRUE(current2.isOk());
    EXPECT_EQ(current1.value, VALID_MASTER_CALL_ID);
    EXPECT_EQ(current2.value, SECOND_MASTER_CALL_ID);

    // Unload master call from first session
    auto unloadStatus = engine->unloadMasterCall(sessionId);
    EXPECT_EQ(unloadStatus, UnifiedAudioEngine::Status::OK);

    // Verify first session has no master call, second still has its master call
    current1 = engine->getCurrentMasterCall(sessionId);
    current2 = engine->getCurrentMasterCall(session2);

    // Either current1 fails or it succeeds with empty value
    if (current1.isOk()) {
        EXPECT_TRUE(current1.value.empty());
    }
    ASSERT_TRUE(current2.isOk());
    EXPECT_EQ(current2.value, SECOND_MASTER_CALL_ID);

    // Clean up second session
    [[maybe_unused]] auto destroyStatus = engine->destroySession(session2);
}

TEST_F(MasterCallManagementTest, SameMasterCallMultipleSessions) {
    // Create second session
    auto session2Result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(session2Result.isOk());
    SessionId session2 = session2Result.value;

    // Load same master call in both sessions
    auto status1 = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    auto status2 = engine->loadMasterCall(session2, VALID_MASTER_CALL_ID);

    EXPECT_EQ(status1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(status2, UnifiedAudioEngine::Status::OK);

    // Verify both sessions have the same master call
    auto current1 = engine->getCurrentMasterCall(sessionId);
    auto current2 = engine->getCurrentMasterCall(session2);

    ASSERT_TRUE(current1.isOk());
    ASSERT_TRUE(current2.isOk());
    EXPECT_EQ(current1.value, VALID_MASTER_CALL_ID);
    EXPECT_EQ(current2.value, VALID_MASTER_CALL_ID);

    // Clean up second session
    [[maybe_unused]] auto destroyStatus = engine->destroySession(session2);
}

// === Error Handling Tests ===

TEST_F(MasterCallManagementTest, InvalidSessionOperations) {
    constexpr SessionId INVALID_SESSION = 99999;

    // Test master call operations on non-existent session
    auto status = engine->loadMasterCall(INVALID_SESSION, VALID_MASTER_CALL_ID);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    status = engine->unloadMasterCall(INVALID_SESSION);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto currentResult = engine->getCurrentMasterCall(INVALID_SESSION);
    EXPECT_FALSE(currentResult.isOk());
    EXPECT_EQ(currentResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(MasterCallManagementTest, UnloadWithoutLoad) {
    // Attempt to unload master call when none is loaded
    auto status = engine->unloadMasterCall(sessionId);
    // This should either succeed (no-op) or return appropriate status
    // The exact behavior may depend on implementation
    EXPECT_TRUE(status == UnifiedAudioEngine::Status::OK
                || status == UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(MasterCallManagementTest, EmptyMasterCallId) {
    // Test loading with empty master call ID
    auto status = engine->loadMasterCall(sessionId, "");
    EXPECT_NE(status, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallManagementTest, VeryLongMasterCallId) {
    // Test loading with very long master call ID
    std::string longId(1000, 'a');
    auto status = engine->loadMasterCall(sessionId, longId);
    EXPECT_NE(status, UnifiedAudioEngine::Status::OK);
}

// === Master Call Feature Validation Tests ===

TEST_F(MasterCallManagementTest, MasterCallFeatureExtraction) {
    // Load master call
    auto status = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    // After loading a master call, the session should have feature data available
    // We can verify this by checking if the session can process audio chunks properly
    std::vector<float> testAudio(512, 0.1f);  // Simple test audio

    auto processStatus = engine->processAudioChunk(sessionId, std::span<const float>(testAudio));
    EXPECT_EQ(processStatus, UnifiedAudioEngine::Status::OK)
        << "Should be able to process audio after loading master call";

    // Should be able to get a similarity score (might need sufficient data first)
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (scoreResult.isOk()) {
        float score = scoreResult.value;
        EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
        EXPECT_LE(score, 1.0f) << "Similarity score should not exceed 1.0";
    } else {
        // It's acceptable if insufficient data has been processed
        EXPECT_EQ(scoreResult.error(), UnifiedAudioEngine::Status::INSUFFICIENT_DATA)
            << "Similarity scoring should fail only due to insufficient data";
    }
}

TEST_F(MasterCallManagementTest, ProcessAudioWithoutMasterCall) {
    // Verify no master call is loaded initially
    auto currentResult = engine->getCurrentMasterCall(sessionId);
    if (currentResult.isOk()) {
        EXPECT_TRUE(currentResult.value.empty());
    }

    // Try to process audio without master call
    std::vector<float> testAudio(512, 0.1f);
    auto processStatus = engine->processAudioChunk(sessionId, std::span<const float>(testAudio));

    // This should either fail or succeed with limited functionality
    // The exact behavior depends on implementation
    if (processStatus == UnifiedAudioEngine::Status::OK) {
        // If processing succeeds, similarity score should indicate no comparison possible
        auto scoreResult = engine->getSimilarityScore(sessionId);
        if (scoreResult.isOk()) {
            // Score might be 0 or NaN to indicate no comparison
            float score = scoreResult.value;
            EXPECT_TRUE(score == 0.0f || std::isnan(score))
                << "Score should indicate no master call available";
        }
    }
}

// === Session Reset Impact Tests ===

TEST_F(MasterCallManagementTest, MasterCallPersistsThroughReset) {
    // Load master call
    auto status = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify master call is loaded
    auto currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk());
    EXPECT_EQ(currentResult.value, VALID_MASTER_CALL_ID);

    // Reset session
    auto resetStatus = engine->resetSession(sessionId);
    EXPECT_EQ(resetStatus, UnifiedAudioEngine::Status::OK);

    // Verify master call is still loaded after reset
    currentResult = engine->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentResult.isOk());
    EXPECT_EQ(currentResult.value, VALID_MASTER_CALL_ID);
}

// === Concurrent Access Tests ===

TEST_F(MasterCallManagementTest, ConcurrentMasterCallOperations) {
    constexpr int NUM_THREADS = 4;
    std::vector<SessionId> sessionIds;
    std::vector<std::thread> threads;

    // Create multiple sessions
    for (int i = 0; i < NUM_THREADS; ++i) {
        auto sessionResult = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(sessionResult.isOk());
        sessionIds.push_back(sessionResult.value);
    }

    // Load master calls concurrently
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, i, &sessionIds]() {
            std::string masterCallId = (i % 2 == 0) ? VALID_MASTER_CALL_ID : SECOND_MASTER_CALL_ID;
            auto status = engine->loadMasterCall(sessionIds[i], masterCallId);
            EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all sessions have correct master calls loaded
    for (int i = 0; i < NUM_THREADS; ++i) {
        auto currentResult = engine->getCurrentMasterCall(sessionIds[i]);
        ASSERT_TRUE(currentResult.isOk());

        std::string expectedId = (i % 2 == 0) ? VALID_MASTER_CALL_ID : SECOND_MASTER_CALL_ID;
        EXPECT_EQ(currentResult.value, expectedId);
    }

    // Clean up sessions
    for (auto sessionId : sessionIds) {
        [[maybe_unused]] auto destroyStatus = engine->destroySession(sessionId);
    }
}

// === Performance Tests ===

TEST_F(MasterCallManagementTest, MasterCallLoadingPerformance) {
    constexpr int NUM_ITERATIONS = 50;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Load and unload master call multiple times
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto loadStatus = engine->loadMasterCall(sessionId, VALID_MASTER_CALL_ID);
        EXPECT_EQ(loadStatus, UnifiedAudioEngine::Status::OK);

        auto unloadStatus = engine->unloadMasterCall(sessionId);
        EXPECT_EQ(unloadStatus, UnifiedAudioEngine::Status::OK);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    // Master call loading should be reasonably fast
    double avgTimePerOperation = duration.count() / double(NUM_ITERATIONS * 2);
    EXPECT_LT(avgTimePerOperation, 10000.0)
        << "Master call load/unload should take less than 10ms on average";
}
