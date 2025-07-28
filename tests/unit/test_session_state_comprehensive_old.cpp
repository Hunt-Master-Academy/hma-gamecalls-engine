/**
 * @file test_session_state_comprehensive.cpp
 * @brief Comprehensive session state management tests for UnifiedAudioEngine
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <set>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
auto checkConfig1Result = engine->getVADConfig(session1);
auto checkConfig2Result = engine->getVADConfig(session2);
ASSERT_TRUE(checkConfig1Result.isOk());
ASSERT_TRUE(checkConfig2Result.isOk());
auto checkConfig1 = *checkConfig1Result;
auto checkConfig2 = *checkConfig2Result;

EXPECT_FALSE(checkConfig1.enabled);  // Should be disabled
EXPECT_FALSE(
    checkConfig2.enabled);  // Should remain unchangedude "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace std::chrono_literals;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class SessionStateTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);
    }

    void TearDown() override {
        // Clean up any remaining sessions
        auto sessions = engine->getActiveSessions();
        for (auto sessionId : sessions) {
            engine->destroySession(sessionId);
        }
    }

    std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> samples(numSamples);
        const float twoPi = 2.0f * M_PI;
        for (int i = 0; i < numSamples; ++i) {
            samples[i] = 0.5f * std::sin(twoPi * frequency * i / sampleRate);
        }
        return samples;
    }
};

TEST_F(SessionStateTest, BasicSessionLifecycle) {
    // Initially no active sessions
    auto initialSessions = engine->getActiveSessions();
    EXPECT_TRUE(initialSessions.empty());

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Verify session is active
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), 1);
    EXPECT_EQ(activeSessions[0], sessionId);

    // Get initial session duration (should be zero or very small)
    auto durationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(durationResult.isOk());
    auto initialDuration = *durationResult;
    EXPECT_GE(initialDuration, 0.0f);

    // Process some audio
    auto audio = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto processResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Duration should have increased
    auto newDurationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(newDurationResult.isOk());
    auto newDuration = *newDurationResult;
    EXPECT_GT(newDuration, initialDuration);

    // Should have features now
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0);

    // Destroy session
    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK);

    // Session should no longer be active
    EXPECT_FALSE(engine->isSessionActive(sessionId));

    auto finalSessions = engine->getActiveSessions();
    EXPECT_TRUE(finalSessions.empty());
}

TEST_F(SessionStateTest, MultipleSessionIsolation) {
    // Create multiple sessions with different sample rates
    auto session1Result = engine->createSession(44100.0f);
    auto session2Result = engine->createSession(48000.0f);
    auto session3Result = engine->createSession(22050.0f);

    ASSERT_TRUE(session1Result.isOk());
    ASSERT_TRUE(session2Result.isOk());
    ASSERT_TRUE(session3Result.isOk());

    SessionId session1 = *session1Result;
    SessionId session2 = *session2Result;
    SessionId session3 = *session3Result;

    // All sessions should be unique
    EXPECT_NE(session1, session2);
    EXPECT_NE(session2, session3);
    EXPECT_NE(session1, session3);

    // All should be active
    EXPECT_TRUE(engine->isSessionActive(session1));
    EXPECT_TRUE(engine->isSessionActive(session2));
    EXPECT_TRUE(engine->isSessionActive(session3));

    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), 3);

    // Process different audio in each session
    auto audio1 = generateSineWave(440.0f, 0.1f, 44100.0f);
    auto audio2 = generateSineWave(880.0f, 0.1f, 48000.0f);
    auto audio3 = generateSineWave(220.0f, 0.1f, 22050.0f);

    EXPECT_EQ(engine->processAudioChunk(session1, audio1), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->processAudioChunk(session2, audio2), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->processAudioChunk(session3, audio3), UnifiedAudioEngine::Status::OK);

    // Each session should have its own feature count
    auto features1 = engine->getFeatureCount(session1);
    auto features2 = engine->getFeatureCount(session2);
    auto features3 = engine->getFeatureCount(session3);

    ASSERT_TRUE(features1.isOk());
    ASSERT_TRUE(features2.isOk());
    ASSERT_TRUE(features3.isOk());

    EXPECT_GT(*features1, 0);
    EXPECT_GT(*features2, 0);
    EXPECT_GT(*features3, 0);

    // Feature counts may be different due to different sample rates and frequencies
    std::cout << "Session 1 features: " << *features1 << std::endl;
    std::cout << "Session 2 features: " << *features2 << std::endl;
    std::cout << "Session 3 features: " << *features3 << std::endl;

    // Reset one session should not affect others
    auto resetResult = engine->resetSession(session2);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);

    // Session 2 should have no features now
    auto resetFeatures = engine->getFeatureCount(session2);
    ASSERT_TRUE(resetFeatures.isOk());
    EXPECT_EQ(*resetFeatures, 0);

    // Other sessions should be unaffected
    auto checkFeatures1 = engine->getFeatureCount(session1);
    auto checkFeatures3 = engine->getFeatureCount(session3);
    ASSERT_TRUE(checkFeatures1.isOk());
    ASSERT_TRUE(checkFeatures3.isOk());
    EXPECT_EQ(*checkFeatures1, *features1);
    EXPECT_EQ(*checkFeatures3, *features3);

    // Clean up
    EXPECT_EQ(engine->destroySession(session1), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->destroySession(session2), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->destroySession(session3), UnifiedAudioEngine::Status::OK);
}

TEST_F(SessionStateTest, SessionResetFunctionality) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Process some audio to build up state
    auto audio = generateSineWave(440.0f, 0.5f, 44100.0f);  // 0.5 seconds
    auto processResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Check initial state
    auto initialFeatures = engine->getFeatureCount(sessionId);
    auto initialDurationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(initialFeatures.isOk());
    ASSERT_TRUE(initialDurationResult.isOk());
    auto initialDuration = *initialDurationResult;
    EXPECT_GT(*initialFeatures, 0);
    EXPECT_GT(initialDuration, 0.0f);

    // Load a master call (may fail if file doesn't exist)
    auto loadResult = engine->loadMasterCall(sessionId, "test_call");
    auto masterCallBefore = engine->getCurrentMasterCall(sessionId);

    // Reset the session
    auto resetResult = engine->resetSession(sessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);

    // Session should still be active
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // State should be reset
    auto resetFeatures = engine->getFeatureCount(sessionId);
    auto resetDurationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(resetFeatures.isOk());
    ASSERT_TRUE(resetDurationResult.isOk());
    auto resetDuration = *resetDurationResult;
    EXPECT_EQ(*resetFeatures, 0);
    EXPECT_EQ(resetDuration, 0.0f);

    // Master call should be cleared
    auto masterCallAfter = engine->getCurrentMasterCall(sessionId);
    EXPECT_FALSE(masterCallAfter.isOk());

    // Session should still be functional after reset
    auto newProcessResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(newProcessResult, UnifiedAudioEngine::Status::OK);

    auto newFeatures = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(newFeatures.isOk());
    EXPECT_GT(*newFeatures, 0);

    engine->destroySession(sessionId);
}

TEST_F(SessionStateTest, VADConfigurationPerSession) {
    auto session1Result = engine->createSession(44100.0f);
    auto session2Result = engine->createSession(44100.0f);
    ASSERT_TRUE(session1Result.isOk());
    ASSERT_TRUE(session2Result.isOk());
    SessionId session1 = *session1Result;
    SessionId session2 = *session2Result;

    // Configure different VAD settings for each session
    VADConfig config1;
    config1.energy_threshold = 0.01f;
    config1.window_duration = 0.020f;     // 20ms in seconds
    config1.min_sound_duration = 0.100f;  // 100ms in seconds
    config1.enabled = true;

    VADConfig config2;
    config2.energy_threshold = 0.05f;
    config2.window_duration = 0.030f;     // 30ms in seconds
    config2.min_sound_duration = 0.200f;  // 200ms in seconds
    config2.enabled = false;

    auto setResult1 = engine->configureVAD(session1, config1);
    auto setResult2 = engine->configureVAD(session2, config2);
    EXPECT_EQ(setResult1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(setResult2, UnifiedAudioEngine::Status::OK);

    // Verify each session has its own configuration
    auto getConfig1Result = engine->getVADConfig(session1);
    auto getConfig2Result = engine->getVADConfig(session2);
    ASSERT_TRUE(getConfig1Result.isOk());
    ASSERT_TRUE(getConfig2Result.isOk());
    auto getConfig1 = *getConfig1Result;
    auto getConfig2 = *getConfig2Result;

    EXPECT_EQ(getConfig1.energy_threshold, 0.01f);
    EXPECT_EQ(getConfig1.window_duration, 0.020f);
    EXPECT_TRUE(getConfig1.enabled);

    EXPECT_EQ(getConfig2.energy_threshold, 0.05f);
    EXPECT_EQ(getConfig2.window_duration, 0.030f);
    EXPECT_FALSE(getConfig2.enabled);

    // Changing one session's VAD should not affect the other
    auto disableResult = engine->disableVAD(session1);
    EXPECT_EQ(disableResult, UnifiedAudioEngine::Status::OK);

    auto checkConfig1 = engine->getVADConfig(session1);
    auto checkConfig2 = engine->getVADConfig(session2);
    ASSERT_TRUE(checkConfig1.isOk());
    ASSERT_TRUE(checkConfig2.isOk());

    EXPECT_FALSE(checkConfig1->enabled);  // Should be disabled
    EXPECT_FALSE(checkConfig2->enabled);  // Should remain unchanged

    engine->destroySession(session1);
    engine->destroySession(session2);
}

TEST_F(SessionStateTest, SessionDurationTracking) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Initial duration should be zero
    auto initialDurationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(initialDurationResult.isOk());
    auto initialDuration = *initialDurationResult;
    EXPECT_EQ(initialDuration, 0.0f);

    // Process audio chunks of known duration
    const float chunkDurationSeconds = 0.1f;  // 100ms
    auto chunk = generateSineWave(440.0f, chunkDurationSeconds, 44100.0f);

    for (int i = 0; i < 5; ++i) {
        auto processResult = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        auto currentDurationResult = engine->getSessionDuration(sessionId);
        ASSERT_TRUE(currentDurationResult.isOk());
        auto currentDuration = *currentDurationResult;

        // Duration should approximately match processed audio
        float expectedSeconds = (i + 1) * chunkDurationSeconds;
        float actualSeconds = currentDuration / 1000.0f;  // Convert ms to seconds

        // Allow some tolerance for processing variations
        EXPECT_NEAR(actualSeconds, expectedSeconds, 0.05f)
            << "After processing " << (i + 1) << " chunks";
    }

    engine->destroySession(sessionId);
}

TEST_F(SessionStateTest, ConcurrentSessionAccess) {
    const int numSessions = 10;
    std::vector<SessionId> sessions;

    // Create multiple sessions
    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);
    }

    // Access sessions concurrently from multiple threads
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &sessions, &successCount, t]() {
            for (int i = 0; i < 20; ++i) {
                SessionId sessionId = sessions[i % sessions.size()];

                // Perform various operations
                auto audio = generateSineWave(440.0f + t * 100.0f, 0.05f, 44100.0f);
                auto processResult = engine->processAudioChunk(sessionId, audio);

                if (processResult == UnifiedAudioEngine::Status::OK) {
                    // Try to get features
                    auto featureResult = engine->getFeatureCount(sessionId);
                    if (featureResult.isOk()) {
                        successCount++;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have reasonable success rate
    EXPECT_GT(successCount.load(), 60);  // At least 75% success rate

    // Clean up
    for (auto sessionId : sessions) {
        engine->destroySession(sessionId);
    }
}

TEST_F(SessionStateTest, SessionStateConsistency) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Process audio and verify state consistency
    auto audio = generateSineWave(440.0f, 0.2f, 44100.0f);

    for (int i = 0; i < 10; ++i) {
        auto processResult = engine->processAudioChunk(sessionId, audio);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        // All state queries should succeed
        auto durationResult = engine->getSessionDuration(sessionId);
        auto featuresResult = engine->getFeatureCount(sessionId);
        auto vadConfigResult = engine->getVADConfig(sessionId);

        EXPECT_TRUE(durationResult.isOk());
        EXPECT_TRUE(featuresResult.isOk());
        EXPECT_TRUE(vadConfigResult.isOk());

        // Values should be reasonable
        if (durationResult.isOk()) {
            auto duration = *durationResult;
            EXPECT_GT(duration, 0.0f);
        }

        if (featuresResult.isOk()) {
            EXPECT_GE(*featuresResult, 0);
        }

        if (vadConfigResult.isOk()) {
            auto vadConfig = *vadConfigResult;
            EXPECT_GE(vadConfig.energy_threshold, 0.0f);
        }

        // Session should remain active
        EXPECT_TRUE(engine->isSessionActive(sessionId));
    }

    engine->destroySession(sessionId);
}
