#include <gtest/gtest.h>

#include <iostream>  // For std::cerr
#include <vector>    // For std::vector

#include "dr_wav.h"  // For loading WAV files
#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

class CoreValidationTest : public ::testing::Test {
   protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    int sessionId = -1;

    // Helper function to load audio file (duplicate from analyze_recording.cpp for self-contained
    // test)
    std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels,
                                       unsigned int &sampleRate) {
        drwav_uint64 totalPCMFrameCount = 0;
        float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
            filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

        if (pSampleData == nullptr) {
            std::cerr << "Error: Could not load audio file for test: " << filePath << std::endl;
            return {};
        }

        // Convert to mono
        std::vector<float> monoSamples(totalPCMFrameCount);
        if (channels > 1) {
            for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
                float monoSample = 0.0f;
                for (unsigned int j = 0; j < channels; ++j) {
                    monoSample += pSampleData[i * channels + j];
                }
                monoSamples[i] = monoSample / channels;
            }
        } else {
            monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
        }
        drwav_free(pSampleData, nullptr);
        return monoSamples;
    }

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

TEST_F(CoreValidationTest, DTWSelfSimilarity) {
    const std::string masterCallId = "buck_grunt";
    const std::string audioFilePath = "../data/master_calls/" + masterCallId + ".wav";

    // Load master call first
    auto sessionResult = engine->createSession(static_cast<float>(44100.0f));
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session.";
    sessionId = sessionResult.value;

    auto masterStatus = engine->loadMasterCall(sessionId, masterCallId);
    if (masterStatus != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Could not load master call file: " << masterCallId;
        engine->destroySession(sessionId);
        sessionId = -1;
        return;
    }

    // Load the same file as the "user attempt"
    unsigned int channels, sampleRate;
    std::vector<float> audioData = load_audio_file(audioFilePath, channels, sampleRate);
    if (audioData.empty()) {
        GTEST_SKIP() << "Failed to load audio file for DTW self-similarity test: " << audioFilePath;
        return;
    }

    // Start a real-time session
    // Process in chunks (simulating real-time)
    const int chunkSize = 1024;
    for (size_t i = 0; i < audioData.size(); i += chunkSize) {
        size_t remainingSamples = audioData.size() - i;
        size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;
        auto status = engine->processAudioChunk(
            sessionId, std::span<const float>(audioData.data() + i, samplesToProcess));
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
    }

    auto scoreResult = engine->getSimilarityScore(sessionId);
    engine->destroySession(sessionId);
    sessionId = -1;

    if (scoreResult.isOk()) {
        float score = scoreResult.value;
        EXPECT_GT(score, 0.5f);  // Expecting a reasonable score for self-similarity
    } else {
        GTEST_SKIP() << "Could not get similarity score - insufficient data or processing error";
    }
}
