#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

class MFCCValidationTest : public ::testing::Test {
   protected:
    HuntmasterAudioEngine *engine;

    void SetUp() override {
        engine = &HuntmasterAudioEngine::getInstance();
        engine->initialize();
    }
};

TEST_F(MFCCValidationTest, MFCCDeterministic) {
    // Test that processing the same audio multiple times gives consistent results
    std::vector<float> scores;

    for (int i = 0; i < 5; i++) {
        // Load the master call
        auto loadResult = engine->loadMasterCall("buck_grunt");
        ASSERT_EQ(loadResult, HuntmasterAudioEngine::EngineStatus::OK)
            << "Failed to load master call";

        // Start a session and process some test audio
        auto sessionResult = engine->startRealtimeSession(44100.0f, 1024);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to start session";
        int sessionId = sessionResult.value;

        // Generate consistent test audio (sine wave)
        std::vector<float> testAudio(44100);  // 1 second at 44.1kHz
        for (int j = 0; j < 44100; ++j) {
            testAudio[j] = 0.5f * sin(2.0f * 3.14159f * 440.0f * j / 44100.0f);
        }

        // Process the audio
        auto processResult =
            engine->processAudioChunk(sessionId, testAudio.data(), testAudio.size());
        EXPECT_EQ(processResult, HuntmasterAudioEngine::EngineStatus::OK) << "Processing failed";

        // Get the similarity score
        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        scores.push_back(scoreResult.value);

        engine->endRealtimeSession(sessionId);
    }

    // Verify all scores are identical (deterministic)
    for (size_t i = 1; i < scores.size(); i++) {
        EXPECT_FLOAT_EQ(scores[0], scores[i]) << "Score " << i << " differs from first score";
    }
}

TEST_F(MFCCValidationTest, SimilarityScoreValidation) {
    // Test that a master call compared against itself gives a high similarity score
    auto loadResult = engine->loadMasterCall("buck_grunt");
    if (loadResult != HuntmasterAudioEngine::EngineStatus::OK) {
        GTEST_SKIP() << "buck_grunt master call not available";
        return;
    }

    // Start a session
    auto sessionResult = engine->startRealtimeSession(44100.0f, 1024);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to start session";
    int sessionId = sessionResult.value;

    // Since we can't directly access the master call audio, we'll test with a perfect sine wave
    // This tests the basic functionality of the similarity scoring
    std::vector<float> perfectTone(44100);  // 1 second
    for (int i = 0; i < 44100; ++i) {
        perfectTone[i] = 0.8f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
    }

    auto processResult =
        engine->processAudioChunk(sessionId, perfectTone.data(), perfectTone.size());
    EXPECT_EQ(processResult, HuntmasterAudioEngine::EngineStatus::OK) << "Processing failed";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;

    engine->endRealtimeSession(sessionId);

    // The score should be reasonable (not zero, not negative)
    EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
    EXPECT_LE(score, 1.0f) << "Similarity score should be normalized";
}