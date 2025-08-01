/**
 * @file test_error_handling_comprehensive.cpp
 * @brief Comprehensive error handling tests for UnifiedAudioEngine
 */

#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class ErrorHandlingTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId validSessionId;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        validSessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && validSessionId != 0) {
            engine->destroySession(validSessionId);
        }
    }
};

TEST_F(ErrorHandlingTest, InvalidParameterHandling) {
    // Test invalid sample rates
    auto invalidSession1 = engine->createSession(-1.0f);
    EXPECT_FALSE(invalidSession1.isOk());
    EXPECT_EQ(invalidSession1.error(), UnifiedAudioEngine::Status::INVALID_PARAMS);

    auto invalidSession2 = engine->createSession(0.0f);
    EXPECT_FALSE(invalidSession2.isOk());
    EXPECT_EQ(invalidSession2.error(), UnifiedAudioEngine::Status::INVALID_PARAMS);

    auto invalidSession3 = engine->createSession(std::numeric_limits<float>::infinity());
    EXPECT_FALSE(invalidSession3.isOk());
    EXPECT_EQ(invalidSession3.error(), UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with NaN
    auto invalidSession4 = engine->createSession(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FALSE(invalidSession4.isOk());
    EXPECT_EQ(invalidSession4.error(), UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(ErrorHandlingTest, InvalidAudioDataHandling) {
    // Test with NaN values
    std::vector<float> nanAudio = {1.0f, std::numeric_limits<float>::quiet_NaN(), 0.5f};
    auto result1 = engine->processAudioChunk(validSessionId, nanAudio);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with infinity values
    std::vector<float> infAudio = {1.0f, std::numeric_limits<float>::infinity(), 0.5f};
    auto result2 = engine->processAudioChunk(validSessionId, infAudio);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with negative infinity
    std::vector<float> negInfAudio = {1.0f, -std::numeric_limits<float>::infinity(), 0.5f};
    auto result3 = engine->processAudioChunk(validSessionId, negInfAudio);
    EXPECT_EQ(result3, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with extremely large values
    std::vector<float> largeAudio(1000, 1e20f);
    auto result4 = engine->processAudioChunk(validSessionId, largeAudio);
    // Should either handle gracefully or return appropriate error
    EXPECT_TRUE(result4 == UnifiedAudioEngine::Status::OK
                || result4 == UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(ErrorHandlingTest, EmptyBufferHandling) {
    std::vector<float> emptyBuffer;
    auto result = engine->processAudioChunk(validSessionId, emptyBuffer);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(ErrorHandlingTest, InvalidSessionOperations) {
    SessionId invalidSessionId = 99999;

    // Ensure this session doesn't exist
    while (engine->isSessionActive(invalidSessionId)) {
        invalidSessionId++;
    }

    std::vector<float> testAudio(1024, 0.1f);

    // Test all operations with invalid session
    auto processResult = engine->processAudioChunk(invalidSessionId, testAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto loadResult = engine->loadMasterCall(invalidSessionId, "test_call");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto vadResult = engine->getVADConfig(invalidSessionId);
    EXPECT_FALSE(vadResult.isOk());
    EXPECT_EQ(vadResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto featureResult = engine->getFeatureCount(invalidSessionId);
    EXPECT_FALSE(featureResult.isOk());
    EXPECT_EQ(featureResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto similarityResult = engine->getSimilarityScore(invalidSessionId);
    EXPECT_FALSE(similarityResult.isOk());
    EXPECT_EQ(similarityResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto durationResult = engine->getSessionDuration(invalidSessionId);
    EXPECT_FALSE(durationResult.isOk());
    EXPECT_EQ(durationResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto resetResult = engine->resetSession(invalidSessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto destroyResult = engine->destroySession(invalidSessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(ErrorHandlingTest, VADConfigurationErrors) {
    huntmaster::VADConfig invalidConfig;

    // Test with negative energy threshold
    invalidConfig.energy_threshold = -1.0f;
    invalidConfig.window_duration = 0.020f;
    invalidConfig.min_sound_duration = 0.100f;
    invalidConfig.enabled = true;

    auto result1 = engine->configureVAD(validSessionId, invalidConfig);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with zero window duration
    invalidConfig.energy_threshold = 0.01f;
    invalidConfig.window_duration = 0.0f;
    auto result2 = engine->configureVAD(validSessionId, invalidConfig);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with extremely large values
    invalidConfig.window_duration = 10.0f;     // 10 seconds
    invalidConfig.min_sound_duration = 20.0f;  // 20 seconds
    auto result3 = engine->configureVAD(validSessionId, invalidConfig);
    EXPECT_EQ(result3, UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(ErrorHandlingTest, LoadMasterCallErrors) {
    // Test with empty string
    auto result1 = engine->loadMasterCall(validSessionId, "");
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with non-existent master call
    auto result2 = engine->loadMasterCall(validSessionId, "non_existent_call_12345");
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Test with invalid characters
    auto result3 = engine->loadMasterCall(validSessionId, "../../../etc/passwd");
    EXPECT_EQ(result3, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with very long name
    std::string longName(1000, 'a');
    auto result4 = engine->loadMasterCall(validSessionId, longName);
    EXPECT_EQ(result4, UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(ErrorHandlingTest, ConcurrentErrorConditions) {
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> errorCount{0};
    std::atomic<int> successCount{0};

    // Create multiple sessions concurrently
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, &errorCount, &successCount, i]() {
            // Try to create session with invalid sample rate
            auto sessionResult = engine->createSession(-44100.0f);
            if (!sessionResult.isOk()) {
                errorCount++;
            } else {
                successCount++;
                engine->destroySession(*sessionResult);
            }

            // Try invalid operations
            SessionId fakeId = 999999 + i;
            std::vector<float> audio(100, 0.1f);
            auto processResult = engine->processAudioChunk(fakeId, audio);
            if (processResult == UnifiedAudioEngine::Status::SESSION_NOT_FOUND) {
                errorCount++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have appropriate error handling
    EXPECT_GT(errorCount.load(), 0);
    EXPECT_EQ(successCount.load(), 0);  // All session creations should fail
}

TEST_F(ErrorHandlingTest, MemoryStressTest) {
    std::vector<SessionId> sessions;
    const int maxSessions = 100;

    // Try to create many sessions
    for (int i = 0; i < maxSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        if (sessionResult.isOk()) {
            sessions.push_back(*sessionResult);
        } else {
            // Engine should gracefully handle resource limits
            EXPECT_TRUE(sessionResult.error() == UnifiedAudioEngine::Status::OUT_OF_MEMORY
                        || sessionResult.error() == UnifiedAudioEngine::Status::INIT_FAILED);
            break;
        }
    }

    // Clean up sessions
    for (auto sessionId : sessions) {
        auto result = engine->destroySession(sessionId);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }
}

TEST_F(ErrorHandlingTest, BufferOverflowProtection) {
    // Test with very large buffer
    const size_t largeSize = 10 * 1024 * 1024;  // 10MB
    std::vector<float> largeBuffer(largeSize, 0.1f);

    auto result = engine->processAudioChunk(validSessionId, largeBuffer);
    // Should either succeed or handle gracefully
    EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                || result == UnifiedAudioEngine::Status::OUT_OF_MEMORY
                || result == UnifiedAudioEngine::Status::PROCESSING_ERROR);
}

TEST_F(ErrorHandlingTest, RecoveryAfterErrors) {
    // Cause an error condition
    std::vector<float> invalidAudio = {std::numeric_limits<float>::quiet_NaN()};
    auto errorResult = engine->processAudioChunk(validSessionId, invalidAudio);
    EXPECT_EQ(errorResult, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Verify we can still use the session normally
    std::vector<float> validAudio(1024, 0.1f);
    auto recoveryResult = engine->processAudioChunk(validSessionId, validAudio);
    EXPECT_EQ(recoveryResult, UnifiedAudioEngine::Status::OK);

    // Verify session is still functional
    EXPECT_TRUE(engine->isSessionActive(validSessionId));

    auto featureResult = engine->getFeatureCount(validSessionId);
    EXPECT_TRUE(featureResult.isOk());
}
