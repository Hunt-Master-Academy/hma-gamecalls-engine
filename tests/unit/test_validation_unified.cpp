/**
 * @file test_validation_unified.cpp
 * @brief Validation tests using the UnifiedAudioEngine API
 *
 * This test suite validates core engine functionality and deterministic behavior
 * using the new UnifiedEngine session-based architecture.
 */

#include <cmath>
#include <iostream>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace huntmaster::test;

class ValidationUnifiedTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Create engine instance using the new UnifiedEngine API
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk())
            << "Failed to create UnifiedAudioEngine: " << static_cast<int>(engineResult.error());
        engine = std::move(*engineResult);
    }

    void TearDown() override {
        // Clean up any remaining sessions
        auto activeSessions = engine->getActiveSessions();
        for (auto sessionId : activeSessions) {
            auto destroyResult = engine->destroySession(sessionId);
            (void)destroyResult;
        }
        engine.reset();
        TestFixtureBase::TearDown();
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
};

TEST_F(ValidationUnifiedTest, MFCCDeterministic) {
    std::cout << "=== MFCC Deterministic Test ===" << std::endl;

    // Test that processing the same audio multiple times gives consistent results
    std::vector<float> scores;
    constexpr int numRuns = 5;

    for (int i = 0; i < numRuns; i++) {
        std::cout << "Run " << (i + 1) << "/" << numRuns << std::endl;

        // Create a session for each run
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk())
            << "Failed to create session: " << static_cast<int>(sessionResult.error());
        SessionId sessionId = sessionResult.value;

        // Try to load the master call
        auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "  buck_grunt not available, skipping test" << std::endl;
            auto destroyResult = engine->destroySession(sessionId);
            (void)destroyResult;
            GTEST_SKIP() << "buck_grunt master call not available";
            return;
        }

        // Generate consistent test audio (sine wave)
        std::vector<float> testAudio(44100);  // 1 second at 44.1kHz
        for (int j = 0; j < 44100; ++j) {
            testAudio[j] = 0.5f * sin(2.0f * 3.14159f * 440.0f * j / 44100.0f);
        }

        // Process the audio using span-based API
        auto processResult =
            engine->processAudioChunk(sessionId, std::span<const float>(testAudio));
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

        // Get the similarity score
        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        float score = scoreResult.value;
        scores.push_back(score);

        std::cout << "  Score: " << std::fixed << std::setprecision(8) << score << std::endl;

        // Clean up
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
    }

    // Analyze consistency
    std::cout << "\nDeterministic Analysis:" << std::endl;

    float minScore = *std::min_element(scores.begin(), scores.end());
    float maxScore = *std::max_element(scores.begin(), scores.end());
    float avgScore = 0.0f;
    for (float score : scores) {
        avgScore += score;
    }
    avgScore /= scores.size();

    float maxDeviation = maxScore - minScore;
    float maxDeviationPercent = (maxDeviation / avgScore) * 100.0f;

    std::cout << "  Average score: " << avgScore << std::endl;
    std::cout << "  Min score: " << minScore << std::endl;
    std::cout << "  Max score: " << maxScore << std::endl;
    std::cout << "  Max deviation: " << maxDeviation << " (" << maxDeviationPercent << "%)"
              << std::endl;

    // MFCC processing should be deterministic
    bool isDeterministic = maxDeviation < 0.0001f;
    std::cout << "  Status: " << (isDeterministic ? "DETERMINISTIC ✓" : "NON-DETERMINISTIC ✗")
              << std::endl;

    EXPECT_TRUE(isDeterministic) << "MFCC processing is not deterministic. Max deviation: "
                                 << maxDeviation << " (" << maxDeviationPercent << "%)";
}

TEST_F(ValidationUnifiedTest, SessionLifecycleValidation) {
    std::cout << "\n=== Session Lifecycle Validation ===" << std::endl;

    // Test session creation and destruction
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    std::cout << "Created session: " << sessionId << std::endl;

    // Verify session is active
    EXPECT_TRUE(engine->isSessionActive(sessionId)) << "Session should be active after creation";

    // Check that session appears in active sessions list
    auto activeSessions = engine->getActiveSessions();
    EXPECT_TRUE(std::find(activeSessions.begin(), activeSessions.end(), sessionId)
                != activeSessions.end())
        << "Session should appear in active sessions list";

    // Test session duration (should be zero or very small initially)
    auto durationResult = engine->getSessionDuration(sessionId);
    EXPECT_TRUE(durationResult.isOk()) << "Should be able to get session duration";
    float duration = durationResult.value;
    EXPECT_GE(duration, 0.0f) << "Session duration should be non-negative";
    std::cout << "Initial session duration: " << duration << " seconds" << std::endl;

    // Test feature count (should be zero initially)
    auto featureCountResult = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCountResult.isOk()) << "Should be able to get feature count";
    int featureCount = featureCountResult.value;
    EXPECT_EQ(featureCount, 0) << "Feature count should be zero initially";

    // Destroy the session
    auto destroyResult = engine->destroySession(sessionId);
    (void)destroyResult;  // Suppress unused variable warning
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK)
        << "Session destruction should succeed";

    // Verify session is no longer active
    EXPECT_FALSE(engine->isSessionActive(sessionId))
        << "Session should be inactive after destruction";

    // Check that session no longer appears in active sessions list
    activeSessions = engine->getActiveSessions();
    EXPECT_TRUE(std::find(activeSessions.begin(), activeSessions.end(), sessionId)
                == activeSessions.end())
        << "Session should not appear in active sessions list after destruction";

    std::cout << "Session lifecycle validation completed successfully" << std::endl;
}

TEST_F(ValidationUnifiedTest, InvalidSessionHandling) {
    std::cout << "\n=== Invalid Session Handling Test ===" << std::endl;

    // Test operations on invalid session ID
    SessionId invalidSessionId = 99999;

    // Should return false for invalid session
    EXPECT_FALSE(engine->isSessionActive(invalidSessionId))
        << "Invalid session should not be active";

    // Should return appropriate error for invalid session operations
    auto loadResult = engine->loadMasterCall(invalidSessionId, "test");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Should return SESSION_NOT_FOUND for invalid session";

    auto featureResult = engine->getFeatureCount(invalidSessionId);
    EXPECT_EQ(featureResult.status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Should return SESSION_NOT_FOUND for invalid session";

    auto scoreResult = engine->getSimilarityScore(invalidSessionId);
    EXPECT_EQ(scoreResult.status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Should return SESSION_NOT_FOUND for invalid session";

    auto durationResult = engine->getSessionDuration(invalidSessionId);
    EXPECT_EQ(durationResult.status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Should return SESSION_NOT_FOUND for invalid session";

    // Test processing with invalid session
    std::vector<float> testAudio(1024, 0.5f);
    auto processResult =
        engine->processAudioChunk(invalidSessionId, std::span<const float>(testAudio));
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Should return SESSION_NOT_FOUND for invalid session";

    std::cout << "Invalid session handling validation completed successfully" << std::endl;
}

TEST_F(ValidationUnifiedTest, MultipleSessionsTest) {
    std::cout << "\n=== Multiple Sessions Test ===" << std::endl;

    // Create multiple sessions
    constexpr int numSessions = 3;
    std::vector<SessionId> sessionIds;

    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk())
            << "Failed to create session " << i << ": " << static_cast<int>(sessionResult.error());
        SessionId sessionId = sessionResult.value;
        sessionIds.push_back(sessionId);
        std::cout << "Created session " << i << ": " << sessionId << std::endl;
    }

    // Verify all sessions are active
    for (size_t i = 0; i < sessionIds.size(); ++i) {
        EXPECT_TRUE(engine->isSessionActive(sessionIds[i]))
            << "Session " << i << " should be active";
    }

    // Check active sessions list
    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), numSessions)
        << "Should have " << numSessions << " active sessions";

    // Process different audio in each session
    for (size_t i = 0; i < sessionIds.size(); ++i) {
        // Generate different sine waves for each session
        float frequency = 440.0f * (i + 1);  // 440, 880, 1320 Hz
        std::vector<float> sineWave(44100);  // 1 second
        for (int j = 0; j < 44100; ++j) {
            sineWave[j] = 0.5f * sin(2.0f * 3.14159f * frequency * j / 44100.0f);
        }

        auto processResult =
            engine->processAudioChunk(sessionIds[i], std::span<const float>(sineWave));
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
            << "Processing failed for session " << i;

        // Check that features were extracted
        auto featureResult = engine->getFeatureCount(sessionIds[i]);
        EXPECT_TRUE(featureResult.isOk()) << "Failed to get features for session " << i;
        int features = featureResult.value;
        EXPECT_GT(features, 0) << "No features extracted for session " << i;

        std::cout << "Session " << i << " (" << frequency << " Hz): " << features << " features"
                  << std::endl;
    }

    // Destroy all sessions
    for (size_t i = 0; i < sessionIds.size(); ++i) {
        auto destroyResult = engine->destroySession(sessionIds[i]);
        EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK)
            << "Failed to destroy session " << i;
    }

    // Verify all sessions are destroyed
    for (size_t i = 0; i < sessionIds.size(); ++i) {
        EXPECT_FALSE(engine->isSessionActive(sessionIds[i]))
            << "Session " << i << " should be inactive after destruction";
    }

    // Check that active sessions list is empty
    activeSessions = engine->getActiveSessions();
    EXPECT_TRUE(activeSessions.empty()) << "Should have no active sessions after destruction";

    std::cout << "Multiple sessions test completed successfully" << std::endl;
}

TEST_F(ValidationUnifiedTest, AudioProcessingValidation) {
    std::cout << "\n=== Audio Processing Validation ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Test empty audio processing
    std::vector<float> emptyAudio;
    auto emptyResult = engine->processAudioChunk(sessionId, std::span<const float>(emptyAudio));
    // Empty audio should either succeed (no-op) or return appropriate error
    EXPECT_TRUE(emptyResult == UnifiedAudioEngine::Status::OK
                || emptyResult == UnifiedAudioEngine::Status::INVALID_PARAMS)
        << "Empty audio processing should handle gracefully";

    // Test normal audio processing
    std::vector<float> normalAudio(1024);
    for (int i = 0; i < 1024; ++i) {
        normalAudio[i] = 0.5f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
    }

    auto normalResult = engine->processAudioChunk(sessionId, std::span<const float>(normalAudio));
    EXPECT_EQ(normalResult, UnifiedAudioEngine::Status::OK)
        << "Normal audio processing should succeed";

    // Test very large audio chunk
    std::vector<float> largeAudio(44100 * 10);  // 10 seconds
    for (int i = 0; i < static_cast<int>(largeAudio.size()); ++i) {
        largeAudio[i] = 0.5f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
    }

    auto largeResult = engine->processAudioChunk(sessionId, std::span<const float>(largeAudio));
    EXPECT_EQ(largeResult, UnifiedAudioEngine::Status::OK)
        << "Large audio processing should succeed";

    // Check that features were extracted
    auto featureResult = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureResult.isOk()) << "Should be able to get feature count";
    int features = featureResult.value;
    EXPECT_GT(features, 0) << "Should have extracted features";

    std::cout << "Total features extracted: " << features << std::endl;

    // Clean up
    auto destroyResult = engine->destroySession(sessionId);
    (void)destroyResult;

    std::cout << "Audio processing validation completed successfully" << std::endl;
}

TEST_F(ValidationUnifiedTest, ErrorHandlingValidation) {
    std::cout << "\n=== Error Handling Validation ===" << std::endl;

    // Test invalid master call loading
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Try to load non-existent master call
    auto loadResult = engine->loadMasterCall(sessionId, "non_existent_call");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::FILE_NOT_FOUND)
        << "Should return FILE_NOT_FOUND for non-existent master call";

    // Try to get similarity score without master call
    auto scoreResult = engine->getSimilarityScore(sessionId);
    EXPECT_NE(scoreResult.status, UnifiedAudioEngine::Status::OK)
        << "Should not succeed getting similarity score without master call";

    // Test session operations after double destruction
    auto destroyResult1 = engine->destroySession(sessionId);
    (void)destroyResult1;  // Suppress unused variable warning

    auto destroyResult2 = engine->destroySession(sessionId);
    EXPECT_EQ(destroyResult2, UnifiedAudioEngine::Status::SESSION_NOT_FOUND)
        << "Second destruction should return SESSION_NOT_FOUND";

    std::cout << "Error handling validation completed successfully" << std::endl;
}
