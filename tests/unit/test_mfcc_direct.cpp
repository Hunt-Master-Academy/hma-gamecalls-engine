#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

// Generate a test sine wave (same as in existing test)
static std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<float> samples(numSamples);

    const float twoPi = 2.0f * 3.14159265359f;
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = 0.5f * sin(twoPi * frequency * i / sampleRate);
    }

    return samples;
}

class MFCCDirectTest : public ::testing::Test {
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

TEST_F(MFCCDirectTest, SineWaveProcessingTest) {
    std::cout << "=== Direct MFCC Integration Test ===" << std::endl;

    // Create a longer sine wave for better feature extraction
    auto sineWave = generateSineWave(440.0f, 2.0f, 44100.0f);  // 2 seconds
    std::cout << "Generated sine wave: " << sineWave.size() << " samples" << std::endl;

    // Start a realtime session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;
    std::cout << "Started session ID: " << sessionId << std::endl;

    // Process the audio in chunks (simulate real-time processing)
    const size_t chunkSize = 1024;
    size_t totalProcessed = 0;

    for (size_t i = 0; i + chunkSize <= sineWave.size(); i += chunkSize) {
        auto status = engine->processAudioChunk(
            sessionId, std::span<const float>(sineWave.data() + i, chunkSize));
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK)
            << "Processing failed at chunk starting at sample " << i;
        totalProcessed += chunkSize;
    }

    std::cout << "Processed " << totalProcessed << " samples in chunks" << std::endl;

    // Check how many features were extracted
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "Total features extracted: " << featureCount << std::endl;

    // We should have extracted some features from 2 seconds of audio
    EXPECT_GT(featureCount, 0) << "No features were extracted!";
    EXPECT_GT(featureCount, 10) << "Too few features extracted for 2 seconds of audio";

    engine->destroySession(sessionId);
    sessionId = -1;
    std::cout << "Test completed successfully" << std::endl;
}

TEST_F(MFCCDirectTest, AllAtOnceProcessingTest) {
    std::cout << "=== All-at-Once Processing Test ===" << std::endl;

    // Create a test signal
    auto testWave = generateSineWave(880.0f, 1.0f, 44100.0f);  // 1 second, 880 Hz
    std::cout << "Generated test wave: " << testWave.size() << " samples" << std::endl;

    // Start a realtime session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    // Process all at once (as in the original failing test)
    auto status = engine->processAudioChunk(sessionId, std::span<const float>(testWave));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "All-at-once processing failed";

    // Check features
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "Features from all-at-once processing: " << featureCount << std::endl;

    EXPECT_GT(featureCount, 0) << "No features extracted from all-at-once processing";

    engine->destroySession(sessionId);
    sessionId = -1;
}
