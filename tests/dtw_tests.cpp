#include <gtest/gtest.h>
#include "huntmaster_engine/HuntmasterAudioEngine.h"
#include "dr_wav.h" // For loading WAV files
#include <vector>   // For std::vector
#include <iostream> // For std::cerr

class CoreValidationTest : public ::testing::Test
{
protected:
    HuntmasterAudioEngine *engine;

    // Helper function to load audio file (duplicate from analyze_recording.cpp for self-contained test)
    std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels, unsigned int &sampleRate)
    {
        drwav_uint64 totalPCMFrameCount = 0;
        float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

        if (pSampleData == nullptr)
        {
            std::cerr << "Error: Could not load audio file for test: " << filePath << std::endl;
            return {};
        }

        // Convert to mono
        std::vector<float> monoSamples(totalPCMFrameCount);
        if (channels > 1)
        {
            for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i)
            {
                float monoSample = 0.0f;
                for (unsigned int j = 0; j < channels; ++j)
                {
                    monoSample += pSampleData[i * channels + j];
                }
                monoSamples[i] = monoSample / channels;
            }
        }
        else
        {
            monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
        }
        drwav_free(pSampleData, nullptr);
        return monoSamples;
    }

    void SetUp() override
    {
        engine = &HuntmasterAudioEngine::getInstance();
        Unchanged lines
        TEST_F(CoreValidationTest, DTWSelfSimilarity)
        {
            engine->loadMasterCall("buck_grunt");
            const std::string masterCallId = "buck_grunt";
            const std::string audioFilePath = "../data/master_calls/" + masterCallId + ".wav";

            // Process same file as "user attempt"
            auto score = engine->analyzeRecording("buck_grunt.wav");
            engine->loadMasterCall(masterCallId);

            // Should be nearly perfect match
            EXPECT_GT(score, 0.99f); // Assuming normalized 0-1
            // Load the same file as the "user attempt"
            unsigned int channels, sampleRate;
            std::vector<float> audioData = load_audio_file(audioFilePath, channels, sampleRate);
            ASSERT_FALSE(audioData.empty()) << "Failed to load audio file for DTW self-similarity test: " << audioFilePath;

            // Start a real-time session
            int sessionId = engine->startRealtimeSession(static_cast<float>(sampleRate), 1024);
            ASSERT_NE(sessionId, -1) << "Failed to start real-time session for DTW self-similarity test.";

            // Process in chunks (simulating real-time)
            const int chunkSize = 1024;
            for (size_t i = 0; i < audioData.size(); i += chunkSize)
            {
                size_t remainingSamples = audioData.size() - i;
                size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;
                engine->processAudioChunk(sessionId, audioData.data() + i, samplesToProcess);
            }

            float score = engine->getSimilarityScore(sessionId);
            engine->endRealtimeSession(sessionId);

            EXPECT_GT(score, 0.99f); // Expecting a very high score for self-similarity
        }
