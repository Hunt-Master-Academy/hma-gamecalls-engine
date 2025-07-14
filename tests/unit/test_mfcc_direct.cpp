#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

// Generate a test sine wave (same as in existing test)
std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
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
    void SetUp() override { engine.initialize(); }

    void TearDown() override { engine.shutdown(); }

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
};

TEST_F(MFCCDirectTest, SineWaveProcessingTest) {
    std::cout << "=== Direct MFCC Integration Test ===" << std::endl;

    // Create a longer sine wave for better feature extraction
    auto sineWave = generateSineWave(440.0f, 2.0f, 44100.0f);  // 2 seconds
    std::cout << "Generated sine wave: " << sineWave.size() << " samples" << std::endl;

    // Start a realtime session
    auto sessionResult = engine.startRealtimeSession(44100.0f, 1024);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to start realtime session";
    int sessionId = sessionResult.value;
    std::cout << "Started session ID: " << sessionId << std::endl;

    // Process the audio in chunks (simulate real-time processing)
    const size_t chunkSize = 1024;
    size_t totalProcessed = 0;

    for (size_t i = 0; i + chunkSize <= sineWave.size(); i += chunkSize) {
        auto status = engine.processAudioChunk(sessionId, sineWave.data() + i, chunkSize);
        EXPECT_EQ(status, HuntmasterAudioEngine::EngineStatus::OK)
            << "Processing failed at chunk starting at sample " << i;
        totalProcessed += chunkSize;
    }

    std::cout << "Processed " << totalProcessed << " samples in chunks" << std::endl;

    // Check how many features were extracted
    int featureCount = engine.getSessionFeatureCount(sessionId);
    std::cout << "Total features extracted: " << featureCount << std::endl;

    // We should have extracted some features from 2 seconds of audio
    EXPECT_GT(featureCount, 0) << "No features were extracted!";
    EXPECT_GT(featureCount, 10) << "Too few features extracted for 2 seconds of audio";

    engine.endRealtimeSession(sessionId);
    std::cout << "Test completed successfully" << std::endl;
}

TEST_F(MFCCDirectTest, AllAtOnceProcessingTest) {
    std::cout << "=== All-at-Once Processing Test ===" << std::endl;

    // Create a test signal
    auto testWave = generateSineWave(880.0f, 1.0f, 44100.0f);  // 1 second, 880 Hz
    std::cout << "Generated test wave: " << testWave.size() << " samples" << std::endl;

    // Start a realtime session
    auto sessionResult = engine.startRealtimeSession(44100.0f, 1024);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to start realtime session";
    int sessionId = sessionResult.value;

    // Process all at once (as in the original failing test)
    auto status = engine.processAudioChunk(sessionId, testWave.data(), testWave.size());
    EXPECT_EQ(status, HuntmasterAudioEngine::EngineStatus::OK) << "All-at-once processing failed";

    // Check features
    int featureCount = engine.getSessionFeatureCount(sessionId);
    std::cout << "Features from all-at-once processing: " << featureCount << std::endl;

    EXPECT_GT(featureCount, 0) << "No features extracted from all-at-once processing";

    engine.endRealtimeSession(sessionId);
}
