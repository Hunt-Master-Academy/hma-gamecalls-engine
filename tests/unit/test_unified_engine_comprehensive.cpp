/**
 * @file test_unified_engine_comprehensive.cpp
 * @brief Comprehensive tests for UnifiedAudioEngine covering edge cases and error handling
 */

#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class UnifiedEngineComprehensiveTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            engine->destroySession(sessionId);
        }
    }

    // Helper functions
    std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> samples(numSamples);

        const float twoPi = 2.0f * M_PI;
        for (int i = 0; i < numSamples; ++i) {
            samples[i] = 0.5f * std::sin(twoPi * frequency * i / sampleRate);
        }
        return samples;
    }

    std::vector<float> generateWhiteNoise(size_t numSamples, float amplitude = 0.1f) {
        std::vector<float> noise(numSamples);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-amplitude, amplitude);

        for (size_t i = 0; i < numSamples; ++i) {
            noise[i] = dis(gen);
        }
        return noise;
    }
};

TEST_F(UnifiedEngineComprehensiveTest, InvalidSessionHandling) {
    SessionId invalidSessionId = 99999;

    // Test various operations with invalid session ID
    std::vector<float> audio(1024, 0.1f);

    auto processResult = engine->processAudioChunk(invalidSessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto loadResult = engine->loadMasterCall(invalidSessionId, "test_call");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto vadConfigResult = engine->getVADConfig(invalidSessionId);
    EXPECT_FALSE(vadConfigResult.isOk());

    auto featureCountResult = engine->getFeatureCount(invalidSessionId);
    EXPECT_FALSE(featureCountResult.isOk());

    auto scoreResult = engine->getSimilarityScore(invalidSessionId);
    EXPECT_FALSE(scoreResult.isOk());
}

TEST_F(UnifiedEngineComprehensiveTest, EmptyAudioBufferHandling) {
    std::vector<float> emptyAudio;

    auto result = engine->processAudioChunk(sessionId, emptyAudio);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(UnifiedEngineComprehensiveTest, LargeAudioBufferHandling) {
    // Test with very large buffer (should handle gracefully)
    std::vector<float> largeAudio(500000, 0.1f);  // 500k samples

    auto result = engine->processAudioChunk(sessionId, largeAudio);
    // Should either succeed or handle gracefully
    EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                || result == UnifiedAudioEngine::Status::PROCESSING_ERROR);
}

TEST_F(UnifiedEngineComprehensiveTest, InvalidAudioDataHandling) {
    std::vector<float> invalidAudio = {1.0f,
                                       2.0f,
                                       std::numeric_limits<float>::infinity(),
                                       0.5f,
                                       std::numeric_limits<float>::quiet_NaN(),
                                       -1.0f};

    auto result = engine->processAudioChunk(sessionId, invalidAudio);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(UnifiedEngineComprehensiveTest, ConcurrentSessionOperations) {
    const int numSessions = 5;
    std::vector<SessionId> sessions;

    // Create multiple sessions
    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);
    }

    // Test concurrent operations
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int i = 0; i < numSessions; ++i) {
        threads.emplace_back([this, &sessions, &successCount, i]() {
            auto audio = generateSineWave(440.0f + i * 100.0f, 0.1f, 44100.0f);
            auto result = engine->processAudioChunk(sessions[i], audio);
            if (result == UnifiedAudioEngine::Status::OK) {
                successCount++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successCount.load(), numSessions);

    // Cleanup sessions
    for (auto sid : sessions) {
        engine->destroySession(sid);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, SessionLifecycleStressTest) {
    const int numIterations = 20;

    for (int i = 0; i < numIterations; ++i) {
        // Create session
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        SessionId testSessionId = *sessionResult;

        // Process some audio
        auto audio = generateSineWave(440.0f, 0.05f, 44100.0f);
        auto processResult = engine->processAudioChunk(testSessionId, audio);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        // Get feature count
        auto featureResult = engine->getFeatureCount(testSessionId);
        EXPECT_TRUE(featureResult.isOk());

        // Destroy session
        auto destroyResult = engine->destroySession(testSessionId);
        EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK);

        // Verify session is no longer active
        EXPECT_FALSE(engine->isSessionActive(testSessionId));
    }
}

TEST_F(UnifiedEngineComprehensiveTest, FeatureExtractionConsistency) {
    auto audio = generateSineWave(440.0f, 1.0f, 44100.0f);

    // Process the same audio multiple times
    std::vector<int> featureCounts;

    for (int i = 0; i < 5; ++i) {
        engine->resetSession(sessionId);

        auto processResult = engine->processAudioChunk(sessionId, audio);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        auto featureResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(featureResult.isOk());
        featureCounts.push_back(*featureResult);
    }

    // All feature counts should be identical
    for (size_t i = 1; i < featureCounts.size(); ++i) {
        EXPECT_EQ(featureCounts[0], featureCounts[i])
            << "Feature extraction is not consistent across runs";
    }
}

TEST_F(UnifiedEngineComprehensiveTest, AudioProcessingWithDifferentSignalTypes) {
    // Test with silence
    std::vector<float> silence(4410, 0.0f);  // 0.1 seconds at 44.1kHz
    auto result1 = engine->processAudioChunk(sessionId, silence);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);

    engine->resetSession(sessionId);

    // Test with sine wave
    auto sineWave = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto result2 = engine->processAudioChunk(sessionId, sineWave);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);

    engine->resetSession(sessionId);

    // Test with white noise
    auto noise = generateWhiteNoise(4410);
    auto result3 = engine->processAudioChunk(sessionId, noise);
    EXPECT_EQ(result3, UnifiedAudioEngine::Status::OK);

    engine->resetSession(sessionId);

    // Test with clipped audio
    std::vector<float> clipped(4410, 1.0f);
    auto result4 = engine->processAudioChunk(sessionId, clipped);
    EXPECT_EQ(result4, UnifiedAudioEngine::Status::OK);
}

TEST_F(UnifiedEngineComprehensiveTest, VADConfigurationEdgeCases) {
    huntmaster::VADConfig config;

    // Test with extreme values
    config.energy_threshold = 0.0f;      // Very low threshold
    config.window_duration = 0.001f;     // Very short window
    config.min_sound_duration = 0.001f;  // Very short duration
    config.enabled = true;

    auto result1 = engine->configureVAD(sessionId, config);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);

    // Test with very high threshold
    config.energy_threshold = 1.0f;    // Very high threshold
    config.window_duration = 0.5f;     // Very long window
    config.min_sound_duration = 1.0f;  // Very long duration

    auto result2 = engine->configureVAD(sessionId, config);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}

TEST_F(UnifiedEngineComprehensiveTest, MultipleResetOperations) {
    auto audio = generateSineWave(440.0f, 0.1f, 44100.0f);

    // Process audio
    engine->processAudioChunk(sessionId, audio);

    // Multiple resets should be safe
    for (int i = 0; i < 10; ++i) {
        auto result = engine->resetSession(sessionId);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

        // Session should still be active after reset
        EXPECT_TRUE(engine->isSessionActive(sessionId));

        // Feature count should be zero after reset
        auto featureResult = engine->getFeatureCount(sessionId);
        EXPECT_TRUE(featureResult.isOk());
        EXPECT_EQ(*featureResult, 0);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, SessionDurationTracking) {
    auto startTime = std::chrono::steady_clock::now();

    // Process audio over time
    auto audio = generateSineWave(440.0f, 0.1f, 44100.0f);
    engine->processAudioChunk(sessionId, audio);

    std::this_thread::sleep_for(50ms);

    auto durationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(durationResult.isOk());
    auto duration = *durationResult;

    auto elapsed = std::chrono::steady_clock::now() - startTime;
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    // Duration should be reasonable (within some tolerance)
    EXPECT_GE(duration, 0.040f);                                // At least 40ms
    EXPECT_LE(duration, (elapsedMs.count() / 1000.0f) + 0.1f);  // Not too much more than elapsed
}

TEST_F(UnifiedEngineComprehensiveTest, MasterCallLifecycle) {
    // Test loading non-existent master call
    auto loadResult1 = engine->loadMasterCall(sessionId, "non_existent_call");
    EXPECT_EQ(loadResult1, UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Current master call should be empty
    auto currentResult1 = engine->getCurrentMasterCall(sessionId);
    EXPECT_TRUE(currentResult1.isOk());
    EXPECT_TRUE(currentResult1.value.empty());

    // Test unloading when no master call is loaded
    auto unloadResult1 = engine->unloadMasterCall(sessionId);
    EXPECT_EQ(unloadResult1, UnifiedAudioEngine::Status::OK);
}

TEST_F(UnifiedEngineComprehensiveTest, ChunkedAudioProcessing) {
    // Create a longer audio signal
    auto longAudio = generateSineWave(440.0f, 2.0f, 44100.0f);  // 2 seconds

    const size_t chunkSize = 1024;
    size_t totalChunks = (longAudio.size() + chunkSize - 1) / chunkSize;

    int totalFeaturesBefore = 0;

    for (size_t i = 0; i < longAudio.size(); i += chunkSize) {
        size_t currentChunkSize = std::min(chunkSize, longAudio.size() - i);
        std::vector<float> chunk(longAudio.begin() + i, longAudio.begin() + i + currentChunkSize);

        auto result = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }

    // Should have extracted features from the long audio
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0);
}

TEST_F(UnifiedEngineComprehensiveTest, ErrorRecoveryAfterFailure) {
    // Force an error with invalid audio
    std::vector<float> invalidAudio = {std::numeric_limits<float>::quiet_NaN()};
    auto failResult = engine->processAudioChunk(sessionId, invalidAudio);
    EXPECT_EQ(failResult, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Engine should recover and process valid audio
    auto validAudio = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto successResult = engine->processAudioChunk(sessionId, validAudio);
    EXPECT_EQ(successResult, UnifiedAudioEngine::Status::OK);

    // Should be able to get features
    auto featureResult = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureResult.isOk());
}
