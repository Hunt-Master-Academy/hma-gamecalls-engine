#include <gtest/gtest.h>

#include <chrono>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

// A simple test fixture for the main engine.
class HuntmasterEngineTest : public ::testing::Test {
   protected:
    void SetUp() override { engine.initialize(); }

    void TearDown() override { engine.shutdown(); }

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
};

// Test case to ensure the engine can be initialized and shut down.
TEST_F(HuntmasterEngineTest, CanInitializeAndShutdown) {
    // The SetUp and TearDown methods already handle this.
    // We just need to assert that it doesn't crash.
    ASSERT_TRUE(true);
}

// Test case to check the dummy scoring functionality.
TEST_F(HuntmasterEngineTest, EmptySessionReturnsZeroScore) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(5);
    const auto start_time = std::chrono::steady_clock::now();

    auto sessionResult = engine.startRealtimeSession(44100.0, 1024);
    ASSERT_TRUE(sessionResult.isOk());

    // Check timeout during processing
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during session operations";

    int sessionId = sessionResult.value;
    auto scoreResult = engine.getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk());

    float score = scoreResult.value;
    engine.endRealtimeSession(sessionId);

    // Without processing audio, score should be 0
    ASSERT_EQ(score, 0.0f);
}

// NOTE: This test remains our target for completing Sprint 2.
// It will be enabled once file loading and processing is fully implemented.
TEST_F(HuntmasterEngineTest, DISABLED_CanProcessAudioFiles) {
    // TODO: Implement file loading and processing to make this test pass.
    ASSERT_TRUE(true);
}
