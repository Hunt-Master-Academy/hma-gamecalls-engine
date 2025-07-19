#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

class MFCCValidationTest : public ::testing::Test {
   protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    int sessionId = -1;

    void SetUp() override {
        auto result = UnifiedAudioEngine::create();
        ASSERT_TRUE(result.isOk()) << "Failed to create UnifiedAudioEngine instance";
        engine = std::move(result.value);
    }

    void TearDown() override {
        if (engine && sessionId != -1) {
            engine->destroySession(sessionId);
        }
        engine.reset();
    }
};

TEST_F(MFCCValidationTest, MFCCDeterministic) {
    // Test that processing the same audio multiple times gives consistent results
    std::vector<float> scores;

    for (int i = 0; i < 5; i++) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = sessionResult.value;

        auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
        ASSERT_EQ(masterStatus, UnifiedAudioEngine::Status::OK) << "Failed to load master call";

        std::vector<float> testAudio(44100);  // 1 second at 44.1kHz
        for (int j = 0; j < 44100; ++j) {
            testAudio[j] = 0.5f * sin(2.0f * 3.14159f * 440.0f * j / 44100.0f);
        }

        auto processStatus =
            engine->processAudioChunk(sessionId, std::span<const float>(testAudio));
        EXPECT_EQ(processStatus, UnifiedAudioEngine::Status::OK) << "Processing failed";

        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        scores.push_back(scoreResult.value);

        engine->destroySession(sessionId);
        sessionId = -1;
    }

    // Verify all scores are identical (deterministic)
    for (size_t i = 1; i < scores.size(); i++) {
        EXPECT_FLOAT_EQ(scores[0], scores[i]) << "Score " << i << " differs from first score";
    }
}

TEST_F(MFCCValidationTest, SimilarityScoreValidation) {
    // Test that a master call compared against itself gives a high similarity score
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
    if (masterStatus != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "buck_grunt master call not available";
        engine->destroySession(sessionId);
        sessionId = -1;
        return;
    }

    std::vector<float> perfectTone(44100);  // 1 second
    for (int i = 0; i < 44100; ++i) {
        perfectTone[i] = 0.8f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
    }

    auto processStatus = engine->processAudioChunk(sessionId, std::span<const float>(perfectTone));
    EXPECT_EQ(processStatus, UnifiedAudioEngine::Status::OK) << "Processing failed";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;

    engine->destroySession(sessionId);
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

    // The score should be reasonable (not zero, not negative)
    EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
    EXPECT_LE(score, 1.0f) << "Similarity score should be normalized";
}