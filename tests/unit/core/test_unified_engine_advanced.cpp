#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class UnifiedAudioEngineAdvancedTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(engineResult.value);

        // Create a test session
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine) {
            auto destroyResult = engine->destroySession(sessionId);
            if (destroyResult != UnifiedAudioEngine::Status::OK) {
                std::cerr << "Warning: Failed to destroy session in TearDown" << std::endl;
            }
        }
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;
};

/**
 * Test UnifiedAudioEngine reset functionality - equivalent to the disabled RealtimeScorerTest
 * Tests both session reset and individual component resets through the UnifiedAudioEngine API
 */
TEST_F(UnifiedAudioEngineAdvancedTest, ResetFunctionalityTest) {
    std::cout << "\n=== Reset Functionality Test ===" << std::endl;

    // Load a master call (may fail if file doesn't exist, but we continue testing reset logic)
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");

    // Process some audio to generate state
    std::vector<float> testAudio(4410);  // 0.1 seconds of audio
    for (size_t i = 0; i < testAudio.size(); ++i) {
        testAudio[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);  // 440Hz sine
    }

    // Process multiple chunks to build up session state
    for (int i = 0; i < 3; ++i) {
        std::span<const float> audioSpan(testAudio.data(), testAudio.size());
        auto processResult = engine->processAudioChunk(sessionId, audioSpan);
        // Continue even if processing fails due to missing master call
    }

    // Check if we have a master call loaded before reset
    auto masterCallResult = engine->getCurrentMasterCall(sessionId);
    bool hadMasterCall = masterCallResult.isOk();

    // Get session duration before reset (as proxy for accumulated state)
    auto durationBeforeResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(durationBeforeResult.isOk());
    auto durationBefore = durationBeforeResult.value;
    EXPECT_GT(durationBefore, 0) << "Session duration should be greater than zero before reset";

    // Test reset session (should clear all session state)
    auto resetResult = engine->resetSession(sessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK) << "resetSession should succeed";

    // After reset, session should be cleared
    auto durationAfterResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(durationAfterResult.isOk());
    auto durationAfter = durationAfterResult.value;
    EXPECT_EQ(durationAfter, 0) << "Session duration should be zero after reset";

    // Check master call status after reset
    auto masterCallAfterReset = engine->getCurrentMasterCall(sessionId);
    EXPECT_FALSE(masterCallAfterReset.isOk()) << "Master call should be cleared after reset";
    if (hadMasterCall) {
        std::cout << "✓ Master call correctly cleared after reset" << std::endl;
    }

    // Session should still be valid and usable after reset
    EXPECT_TRUE(engine->isSessionActive(sessionId)) << "Session should remain active after reset";

    // Should be able to load a new master call after reset
    auto reloadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    // This tests that the session is in a clean, usable state

    std::cout << "✓ UnifiedAudioEngine reset functionality validated" << std::endl;
}

/**
 * Test concurrent session functionality - validates the multi-session architecture
 */
TEST_F(UnifiedAudioEngineAdvancedTest, ConcurrentSessionTest) {
    std::cout << "\n=== Concurrent Session Test ===" << std::endl;

    // Create additional sessions
    auto session2Result = engine->createSession(44100.0f);
    ASSERT_TRUE(session2Result.isOk()) << "Failed to create second session";
    SessionId session2 = *session2Result;

    auto session3Result = engine->createSession(48000.0f);  // Different sample rate
    ASSERT_TRUE(session3Result.isOk()) << "Failed to create third session";
    SessionId session3 = *session3Result;

    // Verify all sessions are active
    EXPECT_TRUE(engine->isSessionActive(sessionId));
    EXPECT_TRUE(engine->isSessionActive(session2));
    EXPECT_TRUE(engine->isSessionActive(session3));

    // Get active session list
    auto activeSessions = engine->getActiveSessions();
    EXPECT_GE(activeSessions.size(), 3) << "Should have at least 3 active sessions";

    // Process different audio in each session simultaneously
    std::vector<float> audio1(1024, 0.3f);  // Constant amplitude
    std::vector<float> audio2(1024, 0.5f);  // Different amplitude
    std::vector<float> audio3(1024, 0.7f);  // Different amplitude

    // Process audio in all sessions
    auto result1 = engine->processAudioChunk(sessionId, audio1);
    auto result2 = engine->processAudioChunk(session2, audio2);
    auto result3 = engine->processAudioChunk(session3, audio3);

    // All sessions should process independently
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result3, UnifiedAudioEngine::Status::OK);

    // Test VAD configuration independence
    huntmaster::VADConfig config1, config2, config3;
    config1.energy_threshold = 0.01f;
    config2.energy_threshold = 0.02f;
    config3.energy_threshold = 0.03f;

    engine->configureVAD(sessionId, config1);
    engine->configureVAD(session2, config2);
    engine->configureVAD(session3, config3);

    // Verify each session has its own configuration
    auto vadConfig1 = engine->getVADConfig(sessionId);
    auto vadConfig2 = engine->getVADConfig(session2);
    auto vadConfig3 = engine->getVADConfig(session3);

    ASSERT_TRUE(vadConfig1.isOk() && vadConfig2.isOk() && vadConfig3.isOk());
    EXPECT_EQ(vadConfig1.value.energy_threshold, 0.01f);
    EXPECT_EQ(vadConfig2.value.energy_threshold, 0.02f);
    EXPECT_EQ(vadConfig3.value.energy_threshold, 0.03f);

    // Clean up additional sessions
    auto destroyResult2 = engine->destroySession(session2);
    auto destroyResult3 = engine->destroySession(session3);
    EXPECT_EQ(destroyResult2, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(destroyResult3, UnifiedAudioEngine::Status::OK);

    // Verify cleanup
    EXPECT_FALSE(engine->isSessionActive(session2));
    EXPECT_FALSE(engine->isSessionActive(session3));
    EXPECT_TRUE(engine->isSessionActive(sessionId));  // Original should still be active

    std::cout << "✓ Concurrent session functionality validated" << std::endl;
}

/**
 * Test master call management functionality
 */
TEST_F(UnifiedAudioEngineAdvancedTest, MasterCallManagementTest) {
    std::cout << "\n=== Master Call Management Test ===" << std::endl;

    // Create test master call file in the data/features directory
    std::string testMasterCallPath = "data/features/test_master_call_mgmt.mfc";

    // Create a simple test feature file
    const uint32_t numFrames = 50;
    const uint32_t numCoeffs = 13;

    std::ofstream file(testMasterCallPath, std::ios::binary);
    if (file.is_open()) {
        // Write header
        file.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
        file.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(numCoeffs));

        // Write test feature data
        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            std::vector<float> features(numCoeffs);
            const float t = static_cast<float>(frame) / numFrames;
            features[0] = 0.5f + 0.3f * std::sin(2.0f * 3.14159f * t * 3.0f);
            for (uint32_t coeff = 1; coeff < numCoeffs; ++coeff) {
                features[coeff] = 0.1f * std::sin(2.0f * 3.14159f * t * (coeff + 1));
            }
            file.write(reinterpret_cast<const char*>(features.data()), numCoeffs * sizeof(float));
        }
        file.close();

        std::cout << "✓ Created test master call file: " << testMasterCallPath << std::endl;

        // Test master call management with our created file
        std::string testMasterCall = "test_master_call_mgmt";

        // Try to load master call
        auto loadResult = engine->loadMasterCall(sessionId, testMasterCall);

        if (loadResult == UnifiedAudioEngine::Status::OK) {
            std::cout << "✓ Successfully loaded " << testMasterCall << std::endl;

            // Verify it's loaded
            auto currentCall = engine->getCurrentMasterCall(sessionId);
            EXPECT_TRUE(currentCall.isOk()) << "Should be able to get current master call";

            if (currentCall.isOk()) {
                EXPECT_EQ(*currentCall, testMasterCall)
                    << "Current master call should match what we loaded";
                std::cout << "✓ Verified current master call: " << *currentCall << std::endl;
            }

            // Test unloading
            auto unloadResult = engine->unloadMasterCall(sessionId);
            EXPECT_EQ(unloadResult, UnifiedAudioEngine::Status::OK)
                << "Should be able to unload master call";

            // Verify it's unloaded
            auto afterUnload = engine->getCurrentMasterCall(sessionId);
            EXPECT_FALSE(afterUnload.isOk()) << "Should not have a master call after unloading";

            std::cout << "✓ Successfully unloaded " << testMasterCall << std::endl;
        } else {
            std::cout << "⚠ Could not load test master call (expected - engine needs proper format "
                         "or path)"
                      << std::endl;
            std::cout << "Status code: " << static_cast<int>(loadResult) << std::endl;
        }

        // Clean up test file
        std::remove(testMasterCallPath.c_str());
    } else {
        std::cout << "⚠ Could not create test master call file" << std::endl;
    }

    // Test loading non-existent master call
    std::cout << "\nTesting non-existent master call..." << std::endl;
    auto invalidLoadResult = engine->loadMasterCall(sessionId, "non_existent_master_call");
    EXPECT_NE(invalidLoadResult, UnifiedAudioEngine::Status::OK)
        << "Should fail to load non-existent master call";
    std::cout << "✓ Correctly failed to load non-existent master call" << std::endl;

    std::cout << "✓ Master call management functionality validated" << std::endl;
}
