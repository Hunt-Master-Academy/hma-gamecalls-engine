#include <iostream>
#include <vector>
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

    std::cout << "Loaded: " << filePath << " (" << totalPCMFrameCount << " frames, "
              << sampleRate << " Hz, " << channels << " channels)" << std::endl;

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
    std::cout << "=== Huntmaster Recording Analyzer ===" << std::endl;

    std::string recordingPath = "../data/recordings/user_attempt_buck_grunt.wav";
    std::string masterCallId = "buck_grunt"; // Add this line!

    if (argc > 1)
    {
        recordingPath = argv[1];
    }

    if (argc > 2)
    {
        masterCallId = argv[2]; // Change this line (was trying to use undeclared variable)
    }

    std::cout << "\nAnalyzing: " << recordingPath << std::endl;
    std::cout << "Comparing against master call: " << masterCallId << std::endl;

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load master call
    std::cout << "\n1. Loading master call..." << std::endl;
    engine.loadMasterCall(masterCallId);

    // Load the recording
    std::cout << "\n2. Loading recording..." << std::endl;
    unsigned int channels, sampleRate;
    std::vector<float> audioData = load_audio_file(recordingPath, channels, sampleRate);

    if (audioData.empty())
    {
        std::cerr << "Failed to load recording!" << std::endl;
        engine.shutdown();
        return 1;
    }

    // Process through real-time session
    std::cout << "\n3. Starting analysis session..." << std::endl;
    int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);

    // Process in chunks (simulating real-time)
    const int chunkSize = 1024;
    int totalChunks = 0;
    for (size_t i = 0; i < audioData.size(); i += chunkSize)
    {
        size_t remainingSamples = audioData.size() - i;
        size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;

        engine.processAudioChunk(sessionId, audioData.data() + i, samplesToProcess);
        totalChunks++;

        // Show progress
        if (totalChunks % 10 == 0)
        {
            std::cout << ".";
            std::cout.flush();
        }
    }
    std::cout << std::endl;

    // Get final score
    std::cout << "\n4. Calculating similarity score..." << std::endl;
    float score = engine.getSimilarityScore(sessionId);

    std::cout << "\n========================================" << std::endl;
    std::cout << "Recording: " << recordingPath << std::endl;
    std::cout << "Duration: " << (float)audioData.size() / sampleRate << " seconds" << std::endl;
    std::cout << "Similarity Score: " << score << std::endl;
    std::cout << "========================================" << std::endl;

    // Note: Higher scores = better match (score = 1/(1+distance))
    if (score > 0.01)
    {
        std::cout << "Interpretation: EXCELLENT match to master call!" << std::endl;
    }
    else if (score > 0.005)
    {
        std::cout << "Interpretation: Good match to master call" << std::endl;
    }
    else if (score > 0.002)
    {
        std::cout << "Interpretation: Fair match to master call" << std::endl;
    }
    else if (score > 0.001)
    {
        std::cout << "Interpretation: Some similarity to master call" << std::endl;
    }
    else
    {
        std::cout << "Interpretation: Different from master call" << std::endl;
    }

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    return 0;
}