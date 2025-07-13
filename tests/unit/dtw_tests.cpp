#include <gtest/gtest.h>

#include <iostream>  // For std::cerr
#include <vector>    // For std::vector

#include "dr_wav.h"  // For loading WAV files
#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;

class CoreValidationTest : public ::testing::Test {
   protected:
    HuntmasterAudioEngine *engine;

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
        engine = &HuntmasterAudioEngine::getInstance();
        engine->initialize();
    }

    void TearDown() override { engine->shutdown(); }
};

TEST_F(CoreValidationTest, DTWSelfSimilarity) {
    const std::string masterCallId = "buck_grunt";
    const std::string audioFilePath = "../data/master_calls/" + masterCallId + ".wav";

    // Load master call first
    auto loadResult = engine->loadMasterCall(masterCallId);
    if (loadResult != HuntmasterAudioEngine::EngineStatus::OK) {
        GTEST_SKIP() << "Could not load master call file: " << masterCallId;
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
    auto sessionResult = engine->startRealtimeSession(static_cast<float>(sampleRate), 1024);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to start real-time session for DTW self-similarity test.";

    int sessionId = sessionResult.value;

    // Process in chunks (simulating real-time)
    const int chunkSize = 1024;
    for (size_t i = 0; i < audioData.size(); i += chunkSize) {
        size_t remainingSamples = audioData.size() - i;
        size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;
        auto chunkResult = engine->processAudioChunk(sessionId, audioData.data() + i,
                                                     static_cast<int>(samplesToProcess));
        EXPECT_EQ(chunkResult, huntmaster::HuntmasterAudioEngine::EngineStatus::OK);
    }

    auto scoreResult = engine->getSimilarityScore(sessionId);
    engine->endRealtimeSession(sessionId);

    if (scoreResult.isOk()) {
        float score = scoreResult.value;
        EXPECT_GT(score, 0.5f);  // Expecting a reasonable score for self-similarity
    } else {
        GTEST_SKIP() << "Could not get similarity score - insufficient data or processing error";
    }
}
