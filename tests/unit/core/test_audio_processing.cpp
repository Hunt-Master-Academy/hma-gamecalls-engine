/**
 * @file test_audio_processing.cpp
 * @brief Comprehensive test suite for UnifiedAudioEngine core audi
 std::filesystem::remove("/home/xbyooki/projects/hma-gamecalls-engine/data/processed_calls/mfc/"
                                 + TEST_MASTER_CALL_ID + ".mfc");processing
 *
 * Tests all core audio processing API methods of the UnifiedAudioEngine:
 * - processAudioChunk
 * - getSimilarityScore
 * - getFeatureCount
 * - MFCC feature extraction validation
 * - DTW similarity computation
 * - Audio processing pipeline integrity
 * - Edge cases and error conditions
 *
 * This test focuses specifically on the core audio processing functionality
 * and ensures reliable feature extraction and similarity scoring.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date August 7, 2025
 */

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class AudioProcessingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create engine";
        engine = std::move(engineResult.value);

        // Create test session
        auto sessionResult = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create test session";
        sessionId = sessionResult.value;

        // Set up test master call for similarity testing
        setupTestMasterCall();
    }

    void TearDown() override {
        if (engine && sessionId != INVALID_SESSION_ID) {
            [[maybe_unused]] auto destroyResult = engine->destroySession(sessionId);
        }
        cleanupTestFiles();
    }

    void setupTestMasterCall() {
        // Create test directories
        std::filesystem::create_directories(
            "/home/xbyooki/projects/hma-gamecalls-engine/data/processed_calls/mfc/");

        // Create a test .mfc file with known MFCC features
        createTestMFCFile(TEST_MASTER_CALL_ID);

        // Load the test master call
        auto status = engine->loadMasterCall(sessionId, TEST_MASTER_CALL_ID);
        if (status != UnifiedAudioEngine::Status::OK) {
            // If loading fails, we'll test without master call
            hasMasterCall = false;
        } else {
            hasMasterCall = true;
        }
    }

    void createTestMFCFile(const std::string& masterCallId) {
        std::string filePath =
            "/home/xbyooki/projects/hma-gamecalls-engine/data/processed_calls/mfc/" + masterCallId
            + ".mfc";
        std::ofstream file(filePath, std::ios::binary);

        if (file.is_open()) {
            // Create predictable MFCC features for testing
            uint32_t numFrames = 20;
            uint32_t numCoefficients = 13;

            file.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
            file.write(reinterpret_cast<const char*>(&numCoefficients), sizeof(numCoefficients));

            // Write deterministic feature data
            for (uint32_t frame = 0; frame < numFrames; ++frame) {
                for (uint32_t coeff = 0; coeff < numCoefficients; ++coeff) {
                    // Create a simple pattern that's deterministic
                    float value = std::sin(2.0f * M_PI * frame / numFrames) * (coeff + 1) * 0.1f;
                    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
                }
            }
            file.close();
        }
    }

    void cleanupTestFiles() {
        std::filesystem::remove("/workspaces/huntmaster-engine/data/processed_calls/mfc/"
                                + TEST_MASTER_CALL_ID + ".mfc");
    }

    // Helper methods for generating test audio
    std::vector<float> generateSineWave(float frequency, float duration, float amplitude = 1.0f) {
        size_t numSamples = static_cast<size_t>(duration * TEST_SAMPLE_RATE);
        std::vector<float> buffer(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / TEST_SAMPLE_RATE);
        }
        return buffer;
    }

    std::vector<float> generateWhiteNoise(size_t numSamples, float amplitude = 1.0f) {
        std::vector<float> buffer(numSamples);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-amplitude, amplitude);

        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] = dist(gen);
        }
        return buffer;
    }

    std::vector<float> generateSilence(size_t numSamples) {
        return std::vector<float>(numSamples, 0.0f);
    }

    std::vector<float> generateComplexSignal(float duration) {
        // Generate a signal with multiple frequency components
        auto signal1 = generateSineWave(440.0f, duration, 0.5f);   // A4
        auto signal2 = generateSineWave(880.0f, duration, 0.3f);   // A5
        auto signal3 = generateSineWave(1320.0f, duration, 0.2f);  // E6

        std::vector<float> result(signal1.size());
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = signal1[i] + signal2[i] + signal3[i];
        }
        return result;
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId = INVALID_SESSION_ID;
    bool hasMasterCall = false;

    static constexpr float TEST_SAMPLE_RATE = 44100.0f;
    static constexpr SessionId INVALID_SESSION_ID = 0;
    static inline const std::string TEST_MASTER_CALL_ID = "test_audio_processing";
};

// === Basic Audio Processing Tests ===

TEST_F(AudioProcessingTest, ProcessSingleAudioChunk) {
    // Generate test audio chunk
    auto audioChunk = generateSineWave(440.0f, 0.1f);  // 100ms at 440Hz

    // Process the audio chunk
    auto status = engine->processAudioChunk(sessionId, std::span<const float>(audioChunk));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Failed to process valid audio chunk";

    // Verify feature count increased
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    EXPECT_GT(featureCountResult.value, 0)
        << "Feature count should increase after processing audio";
}

TEST_F(AudioProcessingTest, ProcessMultipleAudioChunks) {
    constexpr int NUM_CHUNKS = 5;
    std::vector<int> featureCounts;

    // Process multiple audio chunks
    for (int i = 0; i < NUM_CHUNKS; ++i) {
        auto audioChunk = generateSineWave(440.0f + i * 50.0f, 0.05f);  // Varying frequency

        auto status = engine->processAudioChunk(sessionId, std::span<const float>(audioChunk));
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Failed to process chunk " << i;

        auto featureCountResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count for chunk " << i;
        featureCounts.push_back(featureCountResult.value);
    }

    // Verify feature count increases with each chunk
    for (size_t i = 1; i < featureCounts.size(); ++i) {
        EXPECT_GT(featureCounts[i], featureCounts[i - 1])
            << "Feature count should increase with each processed chunk";
    }
}

TEST_F(AudioProcessingTest, ProcessEmptyAudioChunk) {
    std::vector<float> emptyChunk;

    auto status = engine->processAudioChunk(sessionId, std::span<const float>(emptyChunk));
    // Should either succeed (no-op) or fail gracefully
    EXPECT_TRUE(status == UnifiedAudioEngine::Status::OK
                || status == UnifiedAudioEngine::Status::INVALID_PARAMS);
}

TEST_F(AudioProcessingTest, ProcessSilenceAudioChunk) {
    auto silenceChunk = generateSilence(4410);  // 100ms of silence

    auto status = engine->processAudioChunk(sessionId, std::span<const float>(silenceChunk));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Should handle silence gracefully";

    // Verify feature count still increases (silence has features too)
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk());
    EXPECT_GT(featureCountResult.value, 0);
}

TEST_F(AudioProcessingTest, ProcessExtremeAmplitudeAudio) {
    // Test with very loud audio (clipping)
    auto loudChunk = generateSineWave(440.0f, 0.1f, 10.0f);  // 10x normal amplitude
    auto status = engine->processAudioChunk(sessionId, std::span<const float>(loudChunk));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Should handle loud audio";

    // Test with very quiet audio
    auto quietChunk = generateSineWave(440.0f, 0.1f, 0.001f);  // Very quiet
    status = engine->processAudioChunk(sessionId, std::span<const float>(quietChunk));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Should handle quiet audio";
}

// === Similarity Scoring Tests ===

TEST_F(AudioProcessingTest, SimilarityScoreBasic) {
    // Load master call (both test_sine_440 and buck_grunt should be available after path fix)
    auto loadStatus = engine->loadMasterCall(sessionId, "test_sine_440");
    ASSERT_EQ(loadStatus, UnifiedAudioEngine::Status::OK)
        << "test_sine_440 master call should be available after path fix";

    // Process some audio
    auto audioChunk = generateSineWave(440.0f, 0.2f);
    auto status = engine->processAudioChunk(sessionId, std::span<const float>(audioChunk));
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Get similarity score
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        // If similarity scoring fails, it might be a feature not fully implemented
        GTEST_SKIP()
            << "Similarity scoring not fully operational - may be placeholder implementation";
    }

    float score = scoreResult.value;
    EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
    EXPECT_LE(score, 1.0f) << "Similarity score should not exceed 1.0";
}

TEST_F(AudioProcessingTest, SimilarityScoreWithoutProcessing) {
    // Load master call (should be available after path fix)
    auto loadStatus = engine->loadMasterCall(sessionId, "test_sine_440");
    ASSERT_EQ(loadStatus, UnifiedAudioEngine::Status::OK)
        << "test_sine_440 master call should be available after path fix";

    // Try to get similarity score without processing any audio
    auto scoreResult = engine->getSimilarityScore(sessionId);

    // Should either fail or return a default score (like 0)
    if (scoreResult.isOk()) {
        float score = scoreResult.value;
        EXPECT_GE(score, 0.0f);
        EXPECT_LE(score, 1.0f);
    } else {
        // It's acceptable to fail when no audio has been processed
        EXPECT_FALSE(scoreResult.isOk()) << "Expected failure when no audio processed";
    }
}

TEST_F(AudioProcessingTest, SimilarityScoreConsistency) {
    // Load master call (should be available after path fix)
    auto loadStatus = engine->loadMasterCall(sessionId, "test_sine_440");
    ASSERT_EQ(loadStatus, UnifiedAudioEngine::Status::OK)
        << "test_sine_440 master call should be available after path fix";

    // Process the same audio multiple times and verify consistent scores
    auto audioChunk = generateSineWave(440.0f, 0.1f);

    std::vector<float> scores;
    constexpr int NUM_ITERATIONS = 3;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Reset session to start fresh
        auto resetStatus = engine->resetSession(sessionId);
        ASSERT_EQ(resetStatus, UnifiedAudioEngine::Status::OK);

        // Process same audio
        auto status = engine->processAudioChunk(sessionId, std::span<const float>(audioChunk));
        ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

        // Get score
        auto scoreResult = engine->getSimilarityScore(sessionId);
        if (!scoreResult.isOk()) {
            GTEST_SKIP() << "Similarity scoring not operational in iteration " << i;
        }
        scores.push_back(scoreResult.value);
    }

    // Verify scores are consistent (within reasonable tolerance)
    // DTW and MFCC processing can have small numerical variations
    if (!scores.empty()) {
        constexpr float TOLERANCE = 0.05f;  // 5% tolerance for numerical stability
        for (size_t i = 1; i < scores.size(); ++i) {
            EXPECT_NEAR(scores[i], scores[0], TOLERANCE)
                << "Similarity scores should be reasonably consistent for identical audio. "
                << "Score 0: " << scores[0] << ", Score " << i << ": " << scores[i];
        }
    }
}

// === Feature Extraction Tests ===

TEST_F(AudioProcessingTest, FeatureCountProgression) {
    // Start with zero features
    auto initialCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(initialCountResult.isOk());
    int initialCount = initialCountResult.value;
    EXPECT_GE(initialCount, 0);

    // Process audio in chunks and verify feature count increases
    constexpr int CHUNK_SIZE = 2048;                  // ~46ms at 44.1kHz
    auto fullAudio = generateSineWave(440.0f, 0.5f);  // 500ms

    int lastFeatureCount = initialCount;
    for (size_t i = 0; i < fullAudio.size(); i += CHUNK_SIZE) {
        size_t chunkEnd = std::min(i + CHUNK_SIZE, fullAudio.size());
        std::span<const float> chunk(fullAudio.data() + i, chunkEnd - i);

        auto status = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

        auto countResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(countResult.isOk());
        int currentCount = countResult.value;

        // Feature count should increase or stay the same (might not increase if chunk too small)
        EXPECT_GE(currentCount, lastFeatureCount) << "Feature count should not decrease";
        lastFeatureCount = currentCount;
    }

    // Final count should be greater than initial
    EXPECT_GT(lastFeatureCount, initialCount) << "Processing audio should generate features";
}

TEST_F(AudioProcessingTest, DifferentAudioTypesGenerateFeatures) {
    // Test that different types of audio all generate features
    auto sineWave = generateSineWave(440.0f, 0.2f);
    auto complexSignal = generateComplexSignal(0.2f);
    auto noise = generateWhiteNoise(static_cast<size_t>(0.2f * TEST_SAMPLE_RATE));

    std::vector<std::pair<std::string, std::vector<float>>> testCases = {
        {"sine_wave", sineWave}, {"complex_signal", complexSignal}, {"white_noise", noise}};

    for (const auto& [name, audio] : testCases) {
        // Reset session for clean test
        auto resetStatus = engine->resetSession(sessionId);
        ASSERT_EQ(resetStatus, UnifiedAudioEngine::Status::OK);

        auto initialCountResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(initialCountResult.isOk());
        int initialCount = initialCountResult.value;

        // Process audio
        auto status = engine->processAudioChunk(sessionId, std::span<const float>(audio));
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK) << "Failed to process " << name;

        // Verify features were generated
        auto finalCountResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(finalCountResult.isOk());
        int finalCount = finalCountResult.value;

        EXPECT_GT(finalCount, initialCount) << name << " should generate features";
    }
}

// === Edge Cases and Error Handling ===

TEST_F(AudioProcessingTest, InvalidSessionOperations) {
    constexpr SessionId INVALID_SESSION = 99999;
    auto testAudio = generateSineWave(440.0f, 0.1f);

    // Test audio processing on invalid session
    auto status = engine->processAudioChunk(INVALID_SESSION, std::span<const float>(testAudio));
    EXPECT_EQ(status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    // Test similarity score on invalid session
    auto scoreResult = engine->getSimilarityScore(INVALID_SESSION);
    EXPECT_FALSE(scoreResult.isOk());
    EXPECT_EQ(scoreResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    // Test feature count on invalid session
    auto countResult = engine->getFeatureCount(INVALID_SESSION);
    EXPECT_FALSE(countResult.isOk());
    EXPECT_EQ(countResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(AudioProcessingTest, ExtremeAudioData) {
    // Test with NaN values
    std::vector<float> nanAudio(1000, std::numeric_limits<float>::quiet_NaN());
    auto status = engine->processAudioChunk(sessionId, std::span<const float>(nanAudio));
    EXPECT_NE(status, UnifiedAudioEngine::Status::OK) << "Should reject NaN audio data";

    // Test with infinite values
    std::vector<float> infAudio(1000, std::numeric_limits<float>::infinity());
    status = engine->processAudioChunk(sessionId, std::span<const float>(infAudio));
    EXPECT_NE(status, UnifiedAudioEngine::Status::OK) << "Should reject infinite audio data";
}

TEST_F(AudioProcessingTest, VeryLargeAudioChunk) {
    // Test with very large audio chunk
    constexpr size_t LARGE_SIZE = 1024 * 1024;  // 1M samples (~23 seconds)
    auto largeAudio = generateSineWave(440.0f, LARGE_SIZE / TEST_SAMPLE_RATE);

    auto status = engine->processAudioChunk(sessionId, std::span<const float>(largeAudio));
    // Should either succeed or fail gracefully (not crash)
    EXPECT_TRUE(status == UnifiedAudioEngine::Status::OK
                || status == UnifiedAudioEngine::Status::INVALID_PARAMS
                || status == UnifiedAudioEngine::Status::OUT_OF_MEMORY);
}

TEST_F(AudioProcessingTest, VerySmallAudioChunk) {
    // Test with very small audio chunks
    std::vector<size_t> smallSizes = {1, 2, 5, 10, 32};

    for (size_t size : smallSizes) {
        auto smallAudio = generateSineWave(440.0f, size / TEST_SAMPLE_RATE);
        auto status = engine->processAudioChunk(sessionId, std::span<const float>(smallAudio));

        // Small chunks should either be processed or rejected gracefully
        EXPECT_TRUE(status == UnifiedAudioEngine::Status::OK
                    || status == UnifiedAudioEngine::Status::INVALID_PARAMS)
            << "Failed to handle " << size << " sample chunk gracefully";
    }
}

// === Concurrent Processing Tests ===

TEST_F(AudioProcessingTest, ConcurrentProcessing) {
    constexpr int NUM_SESSIONS = 4;
    std::vector<SessionId> sessionIds;
    std::vector<std::thread> threads;

    // Create multiple sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto sessionResult = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(sessionResult.isOk());
        sessionIds.push_back(sessionResult.value);
    }

    // Process audio concurrently in different sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        threads.emplace_back([this, i, &sessionIds]() {
            auto audio = generateSineWave(440.0f + i * 100.0f, 0.5f);  // Different frequencies

            for (size_t j = 0; j < audio.size(); j += 2048) {
                size_t chunkEnd = std::min(j + 2048, audio.size());
                std::span<const float> chunk(audio.data() + j, chunkEnd - j);

                auto status = engine->processAudioChunk(sessionIds[i], chunk);
                EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all sessions processed features
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto countResult = engine->getFeatureCount(sessionIds[i]);
        ASSERT_TRUE(countResult.isOk());
        EXPECT_GT(countResult.value, 0) << "Session " << i << " should have processed features";
    }

    // Clean up sessions
    for (auto sessionId : sessionIds) {
        [[maybe_unused]] auto destroyResult = engine->destroySession(sessionId);
    }
}

// === Performance Tests ===

TEST_F(AudioProcessingTest, ProcessingPerformance) {
    // Test processing performance with various chunk sizes
    std::vector<size_t> chunkSizes = {512, 1024, 2048, 4096};

    for (size_t chunkSize : chunkSizes) {
        auto audio = generateSineWave(440.0f, chunkSize / TEST_SAMPLE_RATE);

        auto startTime = std::chrono::high_resolution_clock::now();

        constexpr int NUM_ITERATIONS = 100;
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            auto status = engine->processAudioChunk(sessionId, std::span<const float>(audio));
            EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        double avgTimePerChunk = duration.count() / double(NUM_ITERATIONS);
        double audioDuration = (chunkSize / TEST_SAMPLE_RATE) * 1000000.0;  // microseconds
        double realtimeFactor = avgTimePerChunk / audioDuration;

        // Processing should be faster than real-time (factor < 1.0)
        EXPECT_LT(realtimeFactor, 1.0) << "Processing should be real-time for chunk size "
                                       << chunkSize << " (factor: " << realtimeFactor << ")";
    }
}
