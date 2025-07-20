#include <gtest/gtest.h>

#include <memory>  // For std::unique_ptr

#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

// A simple test fixture for the main engine.
class HuntmasterEngineTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(engineResult.value);
    }

    void TearDown() override {
        // UnifiedAudioEngine manages its own cleanup
        engine.reset();
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
};

// Test case to ensure the engine can be initialized and shut down.
TEST_F(HuntmasterEngineTest, CanInitializeAndShutdown) {
    // The SetUp and TearDown methods already handle this.
    // We just need to assert that it doesn't crash.
    ASSERT_TRUE(true);
}

// Test case to check that empty session returns appropriate error.
TEST_F(HuntmasterEngineTest, EmptySessionReturnsZeroScore) {
    auto sessionResult = engine->startRealtimeSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());

    uint32_t sessionId = sessionResult.value;
    auto scoreResult = engine->getSimilarityScore(sessionId);

    // Without processing audio or loading master call, should return error
    ASSERT_FALSE(scoreResult.isOk());
    ASSERT_EQ(scoreResult.status, UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    [[maybe_unused]] auto endResult = engine->endRealtimeSession(sessionId);
}

// NOTE: This test remains our target for completing Sprint 2.
// It will be enabled once file loading and processing is fully implemented.
TEST_F(HuntmasterEngineTest, DISABLED_CanProcessAudioFiles) {
    // TODO: Implement file loading and processing to make this test pass.
    ASSERT_TRUE(true);
}
