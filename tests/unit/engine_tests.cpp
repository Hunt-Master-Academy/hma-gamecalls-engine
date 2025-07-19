#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

// Helper function to load a WAV file into a mono float vector.
// This is a common utility in these tests.
static std::vector<float> load_wav_to_mono(const std::string &filePath, unsigned int &sampleRate) {
    unsigned int channels;
    drwav_uint64 totalFrames;
    float *audioData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels,
                                                               &sampleRate, &totalFrames, nullptr);

    if (!audioData) {
        return {};  // Return empty vector on failure
    }

    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < totalFrames; ++i) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[i * channels + ch];
            }
            monoData[i] = sum / static_cast<float>(channels);
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    drwav_free(audioData, nullptr);
    return monoData;
}
// A simple test fixture for the main engine.

class HuntmasterEngineTest : public ::testing::Test {
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

// Test case to ensure the engine can be initialized and shut down.
TEST_F(HuntmasterEngineTest, CanInitializeAndShutdown) {
    // The SetUp and TearDown methods already handle this.
    // We just need to assert that it doesn't crash.
    ASSERT_TRUE(engine != nullptr);
}

// Test case to check the dummy scoring functionality.
TEST_F(HuntmasterEngineTest, EmptySessionReturnsZeroScore) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(5);
    const auto start_time = std::chrono::steady_clock::now();

    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    sessionId = sessionResult.value;

    auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
    if (masterStatus != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "buck_grunt master call not available";
    }

    // Check timeout during processing
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during session operations";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk());
    float score = scoreResult.value;
    engine->destroySession(sessionId);
    sessionId = -1;

    // Without processing audio, score should be 0
    ASSERT_EQ(score, 0.0f);
}

// NOTE: This test remains our target for completing Sprint 2.
// It will be enabled once file loading and processing is fully implemented.
TEST_F(HuntmasterEngineTest, CanProcessAudioFiles) {
    // 1. Load the master call which generates and caches the features.
    unsigned int sampleRate;
    const std::string audioFilePath = "../data/master_calls/buck_grunt.wav";
    std::vector<float> audioData = load_wav_to_mono(audioFilePath, sampleRate);
    if (audioData.empty()) {
        GTEST_SKIP() << "Audio file '" << audioFilePath << "' not found. Skipping test.";
    }

    auto sessionResult = engine->createSession(static_cast<float>(sampleRate));
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session.";
    sessionId = sessionResult.value;

    auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
    if (masterStatus != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Master call file for 'buck_grunt' not found. Skipping test.";
    }

    auto processStatus = engine->processAudioChunk(sessionId, std::span<const float>(audioData));
    ASSERT_EQ(processStatus, UnifiedAudioEngine::Status::OK) << "Audio processing failed.";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score.";
    float score = scoreResult.value;

    engine->destroySession(sessionId);
    sessionId = -1;

    // Since we are comparing the audio to itself, the score should be very close to 1.0.
    EXPECT_GT(score, 0.99f) << "Self-similarity score should be very high.";
}
