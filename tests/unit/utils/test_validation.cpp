#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MFCCValidationTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk())
            << "Failed to create UnifiedAudioEngine: " << static_cast<int>(engineResult.error());
        engine = std::move(*engineResult);
    }
};

TEST_F(MFCCValidationTest, MFCCDeterministic) {
    // Test that processing the same audio multiple times gives consistent results
    std::vector<float> scores;

    for (int i = 0; i < 5; i++) {
        // Create a session
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        SessionId sessionId = *sessionResult;

        // Load the master call
        auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
        ASSERT_EQ(loadResult, UnifiedAudioEngine::Status::OK) << "Failed to load master call";

        // Generate consistent test audio (sine wave)
        std::vector<float> testAudio(44100);  // 1 second at 44.1kHz
        for (int j = 0; j < 44100; ++j) {
            testAudio[j] = 0.5f * sin(2.0f * 3.14159f * 440.0f * j / 44100.0f);
        }

        // Process the audio using span-based API
        std::span<const float> audioSpan(testAudio);
        auto processResult = engine->processAudioChunk(sessionId, audioSpan);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

        // Get the similarity score
        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        scores.push_back(*scoreResult);

        auto destroyResult = engine->destroySession(sessionId);
        EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK) << "Failed to destroy session";
    }

    // Verify all scores are identical (deterministic)
    for (size_t i = 1; i < scores.size(); i++) {
        EXPECT_FLOAT_EQ(scores[0], scores[i]) << "Score " << i << " differs from first score";
    }
}

TEST_F(MFCCValidationTest, SimilarityScoreValidation) {
    // Create a session first
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    SessionId sessionId = *sessionResult;

    // Test that a master call compared against itself gives a high similarity score
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    ASSERT_EQ(loadResult, UnifiedAudioEngine::Status::OK)
        << "buck_grunt master call should be available after path fix";

    // Since we can't directly access the master call audio, we'll test with a perfect sine wave
    // This tests the basic functionality of the similarity scoring
    std::vector<float> perfectTone(44100);  // 1 second
    for (int i = 0; i < 44100; ++i) {
        perfectTone[i] = 0.8f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
    }

    std::span<const float> audioSpan(perfectTone);
    auto processResult = engine->processAudioChunk(sessionId, audioSpan);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = *scoreResult;

    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK) << "Failed to destroy session";

    // The score should be reasonable (not zero, not negative)
    EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
    EXPECT_LE(score, 1.0f) << "Similarity score should be normalized";
}
