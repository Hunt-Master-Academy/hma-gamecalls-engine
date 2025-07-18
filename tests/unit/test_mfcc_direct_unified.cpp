/**
 * @file test_mfcc_direct_unified.cpp
 * @brief Direct MFCC processing tests using the UnifiedAudioEngine API
 *
 * This test suite validates direct MFCC feature extraction using the new
 * UnifiedEngine session-based architecture.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <span>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

// Generate a test sine wave
static std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<float> samples(numSamples);

    const float twoPi = 2.0f * 3.14159265359f;
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = 0.5f * sin(twoPi * frequency * i / sampleRate);
    }

    return samples;
}

class MFCCDirectUnifiedTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create engine instance using the new UnifiedEngine API
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk())
            << "Failed to create UnifiedAudioEngine: " << static_cast<int>(engineResult.error());
        engine = std::move(*engineResult);
    }

    void TearDown() override {
        // Clean up any remaining sessions
        auto activeSessions = engine->getActiveSessions();
        for (auto sessionId : activeSessions) {
            auto destroyResult = engine->destroySession(sessionId); (void)destroyResult;
        }
        engine.reset();
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
};

TEST_F(MFCCDirectUnifiedTest, SineWaveProcessingTest) {
    std::cout << "=== Direct MFCC Integration Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Create a longer sine wave for better feature extraction
    auto sineWave = generateSineWave(440.0f, 2.0f, 44100.0f);  // 2 seconds
    std::cout << "Generated sine wave: " << sineWave.size() << " samples" << std::endl;

    // Process the audio in chunks (simulate real-time processing)
    const size_t chunkSize = 1024;
    size_t totalProcessed = 0;
    size_t chunksProcessed = 0;

    for (size_t i = 0; i + chunkSize <= sineWave.size(); i += chunkSize) {
        std::span<const float> chunk(sineWave.data() + i, chunkSize);
        auto chunkResult = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK)
            << "Chunk " << chunksProcessed << " processing failed";

        totalProcessed += chunkSize;
        chunksProcessed++;
    }

    // Process any remaining samples
    if (totalProcessed < sineWave.size()) {
        size_t remaining = sineWave.size() - totalProcessed;
        std::span<const float> lastChunk(sineWave.data() + totalProcessed, remaining);
        auto lastResult = engine->processAudioChunk(sessionId, lastChunk);
        EXPECT_EQ(lastResult, UnifiedAudioEngine::Status::OK) << "Last chunk processing failed";
        totalProcessed += remaining;
        chunksProcessed++;
    }

    std::cout << "Processed " << chunksProcessed << " chunks, " << totalProcessed
              << " samples total" << std::endl;

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "Features extracted: " << featureCount << std::endl;

    // Get session duration
    auto durationResult = engine->getSessionDuration(sessionId);
    if (durationResult.isOk()) {
        float duration = durationResult.value;
        std::cout << "Session duration: " << duration << " seconds" << std::endl;
    }

    // Clean up
    auto destroyResult = engine->destroySession(sessionId); (void)destroyResult;

    // Validate results
    EXPECT_GT(featureCount, 0) << "No MFCC features were extracted";
    EXPECT_EQ(totalProcessed, sineWave.size()) << "Not all samples were processed";
    EXPECT_GT(chunksProcessed, 0) << "No chunks were processed";
}

TEST_F(MFCCDirectUnifiedTest, MultipleFrequencyTest) {
    std::cout << "\n=== Multiple Frequency MFCC Test ===" << std::endl;

    // Test different frequencies to see if MFCC extraction works consistently
    std::vector<float> frequencies = {220.0f, 440.0f, 880.0f, 1760.0f};
    std::vector<int> featureCounts;

    for (float freq : frequencies) {
        std::cout << "Testing frequency: " << freq << " Hz" << std::endl;

        // Create a session for each frequency
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk())
            << "Failed to create session: " << static_cast<int>(sessionResult.error());
        SessionId sessionId = sessionResult.value;

        // Generate sine wave for this frequency
        auto sineWave = generateSineWave(freq, 1.0f, 44100.0f);  // 1 second

        // Process all at once
        auto processResult = engine->processAudioChunk(sessionId, std::span<const float>(sineWave));
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
            << "Processing failed for " << freq << " Hz";

        // Get feature count
        auto featureCountResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(featureCountResult.isOk())
            << "Failed to get feature count for " << freq << " Hz";
        int featureCount = featureCountResult.value;
        featureCounts.push_back(featureCount);

        std::cout << "  Features extracted: " << featureCount << std::endl;

        // Clean up
        auto destroyResult = engine->destroySession(sessionId); (void)destroyResult;

        // Validate each frequency produces features
        EXPECT_GT(featureCount, 0) << "No features extracted for " << freq << " Hz";
    }

    // Check that all frequencies produced similar numbers of features
    // (they should be similar since they're all 1 second long)
    std::cout << "\nFeature count comparison:" << std::endl;
    for (size_t i = 0; i < frequencies.size(); ++i) {
        std::cout << "  " << frequencies[i] << " Hz: " << featureCounts[i] << " features"
                  << std::endl;
    }

    // All should be greater than 0 and within reasonable range of each other
    int minFeatures = *std::min_element(featureCounts.begin(), featureCounts.end());
    int maxFeatures = *std::max_element(featureCounts.begin(), featureCounts.end());

    EXPECT_GT(minFeatures, 0) << "Some frequencies produced no features";

    // Feature counts should be reasonably consistent (within 20% of each other)
    float variance = static_cast<float>(maxFeatures - minFeatures) / minFeatures;
    EXPECT_LT(variance, 0.2f) << "Feature counts vary too much between frequencies: " << variance;
}

TEST_F(MFCCDirectUnifiedTest, ComplexWaveformTest) {
    std::cout << "\n=== Complex Waveform MFCC Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Create a complex waveform (mix of multiple frequencies)
    std::vector<float> complexWave(44100 * 2);  // 2 seconds
    for (int i = 0; i < static_cast<int>(complexWave.size()); ++i) {
        float t = i / 44100.0f;
        // Mix of fundamental and harmonics
        complexWave[i] = 0.4f * sin(2.0f * 3.14159f * 220.0f * t) +  // Fundamental
                         0.3f * sin(2.0f * 3.14159f * 440.0f * t) +  // First harmonic
                         0.2f * sin(2.0f * 3.14159f * 880.0f * t) +  // Second harmonic
                         0.1f * sin(2.0f * 3.14159f * 1760.0f * t);  // Third harmonic
    }

    std::cout << "Generated complex waveform: " << complexWave.size() << " samples" << std::endl;

    // Process in chunks
    const size_t chunkSize = 2048;
    size_t chunksProcessed = 0;

    for (size_t i = 0; i < complexWave.size(); i += chunkSize) {
        size_t remaining = complexWave.size() - i;
        size_t toProcess = std::min(chunkSize, remaining);

        std::span<const float> chunk(complexWave.data() + i, toProcess);
        auto chunkResult = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK)
            << "Chunk " << chunksProcessed << " processing failed";

        chunksProcessed++;
    }

    std::cout << "Processed " << chunksProcessed << " chunks" << std::endl;

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "Features extracted: " << featureCount << std::endl;

    // Clean up
    auto destroyResult = engine->destroySession(sessionId); (void)destroyResult;

    // Validate results
    EXPECT_GT(featureCount, 0) << "No MFCC features were extracted from complex waveform";
    EXPECT_GT(chunksProcessed, 0) << "No chunks were processed";
}

TEST_F(MFCCDirectUnifiedTest, SessionIsolationTest) {
    std::cout << "\n=== Session Isolation Test ===" << std::endl;

    // Create two sessions to test isolation
    auto session1Result = engine->createSession(44100.0f);
    ASSERT_TRUE(session1Result.isOk())
        << "Failed to create session 1: " << static_cast<int>(session1Result.error());
    SessionId session1 = session1Result.value;

    auto session2Result = engine->createSession(44100.0f);
    ASSERT_TRUE(session2Result.isOk())
        << "Failed to create session 2: " << static_cast<int>(session2Result.error());
    SessionId session2 = session2Result.value;

    std::cout << "Created sessions: " << session1 << " and " << session2 << std::endl;

    // Generate different waveforms for each session
    auto wave1 = generateSineWave(440.0f, 1.0f, 44100.0f);
    auto wave2 = generateSineWave(880.0f, 1.0f, 44100.0f);

    // Process different audio in each session
    auto process1Result = engine->processAudioChunk(session1, std::span<const float>(wave1));
    EXPECT_EQ(process1Result, UnifiedAudioEngine::Status::OK) << "Session 1 processing failed";

    auto process2Result = engine->processAudioChunk(session2, std::span<const float>(wave2));
    EXPECT_EQ(process2Result, UnifiedAudioEngine::Status::OK) << "Session 2 processing failed";

    // Check feature counts for each session
    auto features1Result = engine->getFeatureCount(session1);
    ASSERT_TRUE(features1Result.isOk()) << "Failed to get feature count for session 1";
    int features1 = features1Result.value;

    auto features2Result = engine->getFeatureCount(session2);
    ASSERT_TRUE(features2Result.isOk()) << "Failed to get feature count for session 2";
    int features2 = features2Result.value;

    std::cout << "Session 1 features: " << features1 << std::endl;
    std::cout << "Session 2 features: " << features2 << std::endl;

    // Check that sessions are truly isolated
    EXPECT_TRUE(engine->isSessionActive(session1)) << "Session 1 should be active";
    EXPECT_TRUE(engine->isSessionActive(session2)) << "Session 2 should be active";

    // Clean up
    auto destroy1Result = engine->destroySession(session1);
    EXPECT_EQ(destroy1Result, UnifiedAudioEngine::Status::OK) << "Failed to destroy session 1";

    auto destroy2Result = engine->destroySession(session2);
    EXPECT_EQ(destroy2Result, UnifiedAudioEngine::Status::OK) << "Failed to destroy session 2";

    // Validate results
    EXPECT_GT(features1, 0) << "Session 1 produced no features";
    EXPECT_GT(features2, 0) << "Session 2 produced no features";
    EXPECT_FALSE(engine->isSessionActive(session1))
        << "Session 1 should be inactive after destruction";
    EXPECT_FALSE(engine->isSessionActive(session2))
        << "Session 2 should be inactive after destruction";
}

TEST_F(MFCCDirectUnifiedTest, SessionResetTest) {
    std::cout << "\n=== Session Reset Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Process some audio
    auto sineWave = generateSineWave(440.0f, 1.0f, 44100.0f);
    auto processResult = engine->processAudioChunk(sessionId, std::span<const float>(sineWave));
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Initial processing failed";

    // Check that features were extracted
    auto featuresBefore = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featuresBefore.isOk()) << "Failed to get feature count before reset";
    int countBefore = featuresBefore.value;
    std::cout << "Features before reset: " << countBefore << std::endl;

    // Reset the session
    auto resetResult = engine->resetSession(sessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK) << "Session reset failed";

    // Check that features were cleared
    auto featuresAfter = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featuresAfter.isOk()) << "Failed to get feature count after reset";
    int countAfter = featuresAfter.value;
    std::cout << "Features after reset: " << countAfter << std::endl;

    // Process audio again
    auto processAfterReset = engine->processAudioChunk(sessionId, std::span<const float>(sineWave));
    EXPECT_EQ(processAfterReset, UnifiedAudioEngine::Status::OK) << "Processing after reset failed";

    // Check that features were extracted again
    auto featuresAfterProcess = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featuresAfterProcess.isOk()) << "Failed to get feature count after processing";
    int countAfterProcess = featuresAfterProcess.value;
    std::cout << "Features after processing again: " << countAfterProcess << std::endl;

    // Clean up
    auto destroyResult = engine->destroySession(sessionId); (void)destroyResult;

    // Validate results
    EXPECT_GT(countBefore, 0) << "No features before reset";
    EXPECT_EQ(countAfter, 0) << "Features not cleared after reset";
    EXPECT_GT(countAfterProcess, 0) << "No features after processing again";
}
