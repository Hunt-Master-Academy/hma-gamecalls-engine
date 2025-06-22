#include <gtest/gtest.h>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

// A simple test fixture for the main engine.
class HuntmasterEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine.initialize();
    }

    void TearDown() override
    {
        engine.shutdown();
    }

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
};

// Test case to ensure the engine can be initialized and shut down.
TEST_F(HuntmasterEngineTest, CanInitializeAndShutdown)
{
    // The SetUp and TearDown methods already handle this.
    // We just need to assert that it doesn't crash.
    ASSERT_TRUE(true);
}

// Test case to check the dummy scoring functionality.
TEST_F(HuntmasterEngineTest, EmptySessionReturnsZeroScore) {
    int sessionId = engine.startRealtimeSession(44100.0, 1024);
    float score = engine.getSimilarityScore(sessionId);
    engine.endRealtimeSession(sessionId);

    // Without processing audio, score should be 0
    ASSERT_EQ(score, 0.0f);
}

// NOTE: This test remains our target for completing Sprint 2.
// It will be enabled once file loading and processing is fully implemented.
TEST_F(HuntmasterEngineTest, DISABLED_CanProcessAudioFiles)
{
    // TODO: Implement file loading and processing to make this test pass.
    ASSERT_TRUE(true);
}
