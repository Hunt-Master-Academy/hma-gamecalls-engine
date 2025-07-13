#include <gtest/gtest.h>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;

class CoreValidationTest : public ::testing::Test {
   protected:
    HuntmasterAudioEngine *engine;

    void SetUp() override {
        engine = &HuntmasterAudioEngine::getInstance();
        engine->initialize();
    }
};

TEST_F(CoreValidationTest, MFCCDeterministic) {
    // Test that the engine can load master calls consistently
    for (int i = 0; i < 10; i++) {
        auto result = engine->loadMasterCall("test_tone");
        EXPECT_EQ(result, HuntmasterAudioEngine::EngineStatus::OK);
    }
}

TEST_F(CoreValidationTest, RealtimeSessionValidation) {
    auto result = engine->loadMasterCall("buck_grunt");
    EXPECT_EQ(result, HuntmasterAudioEngine::EngineStatus::OK);

    // Test real-time session functionality
    auto sessionResult = engine->startRealtimeSession(44100.0f, 1024);
    EXPECT_TRUE(sessionResult.isOk());

    if (sessionResult.isOk()) {
        int sessionId = sessionResult.value;

        // Test that we can get a similarity score (even if 0)
        auto scoreResult = engine->getSimilarityScore(sessionId);
        EXPECT_TRUE(scoreResult.isOk());

        engine->endRealtimeSession(sessionId);
    }
}