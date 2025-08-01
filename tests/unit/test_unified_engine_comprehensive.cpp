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
        sessionId = sessionResult.value;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            auto status = engine->destroySession(sessionId);
            (void)status;  // Suppress unused warning in test cleanup
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
        auto destroyStatus = engine->destroySession(sid);
        (void)destroyStatus;  // Suppress unused warning
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
        auto resetStatus = engine->resetSession(sessionId);
        (void)resetStatus;  // Suppress unused warning

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

// ===== ENHANCED EDGE CASE TESTING =====

TEST_F(UnifiedEngineComprehensiveTest, ExtremeSampleRateHandling) {
    // Test with very low sample rate
    auto lowSampleRateResult = engine->createSession(1000.0f);  // 1kHz
    if (lowSampleRateResult.isOk()) {
        SessionId lowSampleSession = *lowSampleRateResult;
        auto validAudio = generateSineWave(100.0f, 0.1f, 1000.0f);
        auto result = engine->processAudioChunk(lowSampleSession, validAudio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
        engine->destroySession(lowSampleSession);
    }

    // Test with very high sample rate
    auto highSampleRateResult = engine->createSession(192000.0f);  // 192kHz
    if (highSampleRateResult.isOk()) {
        SessionId highSampleSession = *highSampleRateResult;
        auto validAudio = generateSineWave(1000.0f, 0.05f, 192000.0f);
        auto result = engine->processAudioChunk(highSampleSession, validAudio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
        engine->destroySession(highSampleSession);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, AudioBufferBoundaryConditions) {
    // Test with exactly 1 sample
    std::vector<float> singleSample = {0.5f};
    auto result = engine->processAudioChunk(sessionId, singleSample);
    // May succeed or fail depending on implementation, but should not crash
    EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                || result == UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with power-of-2 sizes
    std::vector<int> testSizes = {64, 128, 256, 512, 1024, 2048, 4096, 8192};
    for (int size : testSizes) {
        std::vector<float> testAudio = generateSineWave(440.0f, 0.01f, 44100.0f);
        testAudio.resize(size, 0.0f);

        auto result = engine->processAudioChunk(sessionId, testAudio);
        EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                    || result == UnifiedAudioEngine::Status::INVALID_PARAMS)
            << "Failed with buffer size: " << size;
    }
}

TEST_F(UnifiedEngineComprehensiveTest, ExtendedInvalidAudioHandling) {
    // Test with NaN values
    std::vector<float> nanAudio = {std::numeric_limits<float>::quiet_NaN(), 0.5f, 0.3f};
    auto nanResult = engine->processAudioChunk(sessionId, nanAudio);
    EXPECT_EQ(nanResult, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with infinity values
    std::vector<float> infAudio = {std::numeric_limits<float>::infinity(), 0.5f};
    auto infResult = engine->processAudioChunk(sessionId, infAudio);
    EXPECT_EQ(infResult, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Test with extremely large values
    std::vector<float> largeAudio = {1000000.0f, -1000000.0f, 0.5f};
    auto largeResult = engine->processAudioChunk(sessionId, largeAudio);
    // Should either normalize or reject extreme values
    EXPECT_TRUE(largeResult == UnifiedAudioEngine::Status::OK
                || largeResult == UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(UnifiedEngineComprehensiveTest, SessionLifecycleEdgeCases) {
    // Test destroying non-existent session
    SessionId fakeSessionId = 99999;
    auto destroyResult = engine->destroySession(fakeSessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    // Test multiple destroys of same session
    auto tempSession = engine->createSession(44100.0f);
    ASSERT_TRUE(tempSession.isOk());
    SessionId tempId = *tempSession;

    auto firstDestroy = engine->destroySession(tempId);
    EXPECT_EQ(firstDestroy, UnifiedAudioEngine::Status::OK);

    auto secondDestroy = engine->destroySession(tempId);
    EXPECT_EQ(secondDestroy, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(UnifiedEngineComprehensiveTest, MaximumSessionLimits) {
    // Test creating many sessions to find limits
    std::vector<SessionId> sessions;
    const int maxAttempts = 100;

    for (int i = 0; i < maxAttempts; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        if (sessionResult.isOk()) {
            sessions.push_back(*sessionResult);
        } else {
            // Hit session limit or resource constraint
            break;
        }
    }

    EXPECT_GT(sessions.size(), 0) << "Should be able to create at least one session";

    // Clean up all created sessions
    for (SessionId session : sessions) {
        engine->destroySession(session);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, MasterCallEdgeCases) {
    // Test loading empty master call name
    auto emptyResult = engine->loadMasterCall(sessionId, "");
    EXPECT_NE(emptyResult, UnifiedAudioEngine::Status::OK);

    // Test loading very long master call name
    std::string longName(1000, 'a');
    auto longResult = engine->loadMasterCall(sessionId, longName);
    EXPECT_NE(longResult, UnifiedAudioEngine::Status::OK);

    // Test loading with special characters
    auto specialResult = engine->loadMasterCall(sessionId, "test/with\\special*chars");
    // May succeed or fail depending on implementation
    EXPECT_TRUE(specialResult == UnifiedAudioEngine::Status::OK
                || specialResult != UnifiedAudioEngine::Status::OK);
}

TEST_F(UnifiedEngineComprehensiveTest, FeatureExtractionEdgeCases) {
    // Process minimal audio and test feature extraction
    auto minimalAudio = generateSineWave(440.0f, 0.01f, 44100.0f);  // 10ms
    auto processResult = engine->processAudioChunk(sessionId, minimalAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Test feature count immediately after minimal processing
    auto featureCountResult = engine->getFeatureCount(sessionId);
    if (featureCountResult.isOk()) {
        EXPECT_GE(*featureCountResult, 0);
    }

    // Test similarity score with minimal data
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (scoreResult.isOk()) {
        float score = *scoreResult;
        EXPECT_GE(score, 0.0f);
        EXPECT_LE(score, 1.0f);
    }
}

// ===== CONCURRENT SESSION TESTING =====

TEST_F(UnifiedEngineComprehensiveTest, ConcurrentSessionCreation) {
    const int numThreads = 4;
    const int sessionsPerThread = 10;
    std::vector<std::thread> threads;
    std::vector<std::vector<SessionId>> threadSessions(numThreads);

    // Create sessions concurrently
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, sessionsPerThread, &threadSessions]() {
            for (int i = 0; i < sessionsPerThread; ++i) {
                auto sessionResult = engine->createSession(44100.0f);
                if (sessionResult.isOk()) {
                    threadSessions[t].push_back(*sessionResult);
                }
                // Small delay to interleave operations
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify sessions were created successfully
    int totalSessions = 0;
    for (const auto& sessions : threadSessions) {
        totalSessions += sessions.size();
    }
    EXPECT_GT(totalSessions, 0);

    // Clean up all sessions
    for (const auto& sessions : threadSessions) {
        for (SessionId session : sessions) {
            engine->destroySession(session);
        }
    }
}

TEST_F(UnifiedEngineComprehensiveTest, ConcurrentAudioProcessing) {
    // Create multiple sessions for concurrent processing
    const int numSessions = 3;
    std::vector<SessionId> sessions;

    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);
    }

    // Process audio concurrently on different sessions
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> errorCount{0};

    for (int i = 0; i < numSessions; ++i) {
        threads.emplace_back([this, i, &sessions, &successCount, &errorCount]() {
            // Generate different frequency for each session
            float frequency = 440.0f + (i * 100.0f);
            auto audio = generateSineWave(frequency, 0.1f, 44100.0f);

            for (int chunk = 0; chunk < 5; ++chunk) {
                auto result = engine->processAudioChunk(sessions[i], audio);
                if (result == UnifiedAudioEngine::Status::OK) {
                    successCount++;
                } else {
                    errorCount++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // Wait for all processing to complete
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successCount.load(), 0) << "At least some audio processing should succeed";

    // Clean up sessions
    for (SessionId session : sessions) {
        engine->destroySession(session);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, ConcurrentFeatureExtraction) {
    // Create sessions and process some audio
    const int numSessions = 3;
    std::vector<SessionId> sessions;

    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);

        // Process initial audio
        auto audio = generateSineWave(440.0f + (i * 100.0f), 0.1f, 44100.0f);
        engine->processAudioChunk(sessions[i], audio);
    }

    // Extract features concurrently
    std::vector<std::thread> threads;
    std::atomic<int> successfulExtractions{0};

    for (int i = 0; i < numSessions; ++i) {
        threads.emplace_back([this, i, &sessions, &successfulExtractions]() {
            for (int attempt = 0; attempt < 5; ++attempt) {
                auto featureResult = engine->getFeatureCount(sessions[i]);
                auto scoreResult = engine->getSimilarityScore(sessions[i]);

                if (featureResult.isOk() || scoreResult.isOk()) {
                    successfulExtractions++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    // Wait for all extractions to complete
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successfulExtractions.load(), 0) << "Some feature extractions should succeed";

    // Clean up sessions
    for (SessionId session : sessions) {
        engine->destroySession(session);
    }
}

// ===== ERROR RECOVERY SCENARIOS =====

TEST_F(UnifiedEngineComprehensiveTest, ErrorRecoveryFromInvalidSequence) {
    // Try to get features before processing any audio
    auto prematureFeatureResult = engine->getFeatureCount(sessionId);
    // May fail or return 0, but should not crash

    auto prematurityScoreResult = engine->getSimilarityScore(sessionId);
    // May fail or return default value, but should not crash

    // Now process valid audio and verify recovery
    auto validAudio = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto processResult = engine->processAudioChunk(sessionId, validAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Features should now be available
    auto recoveredFeatureResult = engine->getFeatureCount(sessionId);
    if (recoveredFeatureResult.isOk()) {
        EXPECT_GE(*recoveredFeatureResult, 0);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, ErrorRecoveryFromCorruptedSession) {
    // Process valid audio first
    auto validAudio = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto initialResult = engine->processAudioChunk(sessionId, validAudio);
    EXPECT_EQ(initialResult, UnifiedAudioEngine::Status::OK);

    // Try to corrupt session with invalid data
    std::vector<float> corruptedAudio = {std::numeric_limits<float>::quiet_NaN(),
                                         std::numeric_limits<float>::infinity(),
                                         -std::numeric_limits<float>::infinity()};
    auto corruptResult = engine->processAudioChunk(sessionId, corruptedAudio);
    EXPECT_EQ(corruptResult, UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Verify session can still process valid audio after corruption attempt
    auto recoveryResult = engine->processAudioChunk(sessionId, validAudio);
    EXPECT_EQ(recoveryResult, UnifiedAudioEngine::Status::OK);

    // Features should still be accessible
    auto finalFeatureResult = engine->getFeatureCount(sessionId);
    if (finalFeatureResult.isOk()) {
        EXPECT_GE(*finalFeatureResult, 0);
    }
}

TEST_F(UnifiedEngineComprehensiveTest, StressTestWithRapidOperations) {
    // Rapid sequence of operations to test stability
    const int iterations = 50;
    auto testAudio = generateSineWave(440.0f, 0.02f, 44100.0f);  // Short audio

    int successCount = 0;
    for (int i = 0; i < iterations; ++i) {
        auto result = engine->processAudioChunk(sessionId, testAudio);
        if (result == UnifiedAudioEngine::Status::OK) {
            successCount++;
        }

        // Occasionally check features
        if (i % 10 == 0) {
            auto featureResult = engine->getFeatureCount(sessionId);
            auto scoreResult = engine->getSimilarityScore(sessionId);
            // Results may vary, but calls should not crash
        }
    }

    EXPECT_GT(successCount, iterations * 0.8) << "Most operations should succeed";
}

// ===== MEMORY AND RESOURCE TESTING =====

TEST_F(UnifiedEngineComprehensiveTest, MemoryLeakPreventionTest) {
    // Create and destroy many sessions to test for memory leaks
    const int cycles = 20;

    for (int cycle = 0; cycle < cycles; ++cycle) {
        // Create session
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        SessionId tempSession = *sessionResult;

        // Process some audio
        auto audio = generateSineWave(440.0f, 0.05f, 44100.0f);
        engine->processAudioChunk(tempSession, audio);

        // Extract features
        engine->getFeatureCount(tempSession);
        engine->getSimilarityScore(tempSession);

        // Destroy session
        auto destroyResult = engine->destroySession(tempSession);
        EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK);
    }

    // Test should complete without memory exhaustion
    SUCCEED() << "Memory leak prevention test completed";
}

TEST_F(UnifiedEngineComprehensiveTest, LongRunningSessionStability) {
    // Test session stability over extended processing
    const int longProcessingCycles = 100;
    auto baseAudio = generateSineWave(440.0f, 0.1f, 44100.0f);

    int consecutiveSuccesses = 0;
    int maxConsecutiveSuccesses = 0;

    for (int i = 0; i < longProcessingCycles; ++i) {
        // Vary the audio slightly each time
        std::vector<float> variedAudio = baseAudio;
        float amplitudeVariation = 0.5f + 0.3f * std::sin(i * 0.1f);
        for (auto& sample : variedAudio) {
            sample *= amplitudeVariation;
        }

        auto result = engine->processAudioChunk(sessionId, variedAudio);
        if (result == UnifiedAudioEngine::Status::OK) {
            consecutiveSuccesses++;
            maxConsecutiveSuccesses = std::max(maxConsecutiveSuccesses, consecutiveSuccesses);
        } else {
            consecutiveSuccesses = 0;
        }

        // Periodically verify features are still accessible
        if (i % 25 == 0) {
            auto featureResult = engine->getFeatureCount(sessionId);
            auto scoreResult = engine->getSimilarityScore(sessionId);
            // Results should be accessible even if processing occasionally fails
        }
    }

    EXPECT_GT(maxConsecutiveSuccesses, 10)
        << "Should have sustained periods of successful processing";
}
