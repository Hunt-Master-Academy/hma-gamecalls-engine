#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "huntmaster_engine/HuntmasterAudioEngine.h"
#include "dr_wav.h"

// Helper function to load audio file
std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels, unsigned int &sampleRate)
{
    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr)
    {
        std::cerr << "Error: Could not load audio file: " << filePath << std::endl;
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

int main(int argc, char *argv[])
{
    std::cout << "=== Find Best Matching Call ===" << std::endl;

    std::string recordingPath = "../data/recordings/user_attempt_buck_grunt.wav";
    if (argc > 1)
    {
        recordingPath = argv[1];
    }

    std::cout << "\nAnalyzing: " << recordingPath << std::endl;

    // List of all master calls to check
    std::vector<std::string> masterCalls = {
        "breeding_bellow", "buck_grunt", "buck_rage_grunts",
        "buck-bawl", "contact-bleatr", "doe-grunt", "doebleat",
        "estrus_bleat", "fawn-bleat", "sparring_bucks", "tending_grunts"};

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load the recording once
    unsigned int channels, sampleRate;
    std::vector<float> audioData = load_audio_file(recordingPath, channels, sampleRate);

    if (audioData.empty())
    {
        std::cerr << "Failed to load recording!" << std::endl;
        engine.shutdown();
        return 1;
    }

    std::cout << "Recording duration: " << (float)audioData.size() / sampleRate << " seconds" << std::endl;
    std::cout << "\nComparing against all master calls...\n"
              << std::endl;

    // Store results
    std::map<float, std::string> scoreToCall;

    // Test against each master call
    for (const auto &callName : masterCalls)
    {
        std::cout << "Testing against: " << callName << "... ";
        std::cout.flush();

        // Try to load the master call
        engine.loadMasterCall(callName);

        // Create a new session for this comparison
        int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);

        // Process the audio
        const int chunkSize = 1024;
        for (size_t i = 0; i < audioData.size(); i += chunkSize)
        {
            size_t remainingSamples = audioData.size() - i;
            size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;
            engine.processAudioChunk(sessionId, audioData.data() + i, samplesToProcess);
        }

        // Get score
        float score = engine.getSimilarityScore(sessionId);
        scoreToCall[score] = callName;

        std::cout << "Score: " << score << std::endl;

        engine.endRealtimeSession(sessionId);
    }

    // Display results sorted by score (best first)
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS (sorted by similarity):" << std::endl;
    std::cout << "========================================" << std::endl;

    int rank = 1;
    for (const auto &pair : scoreToCall)
    {
        std::cout << rank++ << ". " << pair.second
                  << " - Score: " << pair.first;

        // Higher scores = better match (since score = 1/(1+distance))
        if (pair.first > 0.01)
        {
            std::cout << " [EXCELLENT MATCH]";
        }
        else if (pair.first > 0.005)
        {
            std::cout << " [Good match]";
        }
        else if (pair.first > 0.002)
        {
            std::cout << " [Fair match]";
        }
        else if (pair.first > 0.001)
        {
            std::cout << " [Some similarity]";
        }

        std::cout << std::endl;
    }

    std::cout << "\nBest match: " << scoreToCall.begin()->second
              << " (Score: " << scoreToCall.begin()->first << ")" << std::endl;

    engine.shutdown();
    return 0;
}