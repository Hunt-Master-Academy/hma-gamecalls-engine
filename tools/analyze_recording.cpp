#include <iostream>
#include <string_view>  // Use string_view for consistency with the API
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

// The 'using' declarations can be at the top level for clarity
using huntmaster::HuntmasterAudioEngine;

// Helper function to load an audio file into a float vector (This function is already well-written)
std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels,
                                   unsigned int &sampleRate) {
    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        std::cerr << "Error: Could not load audio file: " << filePath << std::endl;
        return {};
    }

    std::cout << "Loaded: " << filePath << " (" << totalPCMFrameCount << " frames, " << sampleRate
              << " Hz, " << channels << " channels)" << std::endl;

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

int main(int argc, char *argv[]) {
    std::cout << "=== Huntmaster Recording Analyzer ===" << std::endl;

    std::string recordingPath = "../data/recordings/user_attempt_buck_grunt.wav";
    std::string masterCallId = "buck_grunt";

    if (argc > 1) {
        recordingPath = argv[1];
    }
    if (argc > 2) {
        masterCallId = argv[2];
    }

    std::cout << "\nAnalyzing: " << recordingPath << std::endl;
    std::cout << "Comparing against master call: " << masterCallId << std::endl;

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load master call
    std::cout << "\n1. Loading master call..." << std::endl;
    // UPDATED: Check the return status of the function call.
    if (engine.loadMasterCall(masterCallId) != HuntmasterAudioEngine::EngineStatus::OK) {
        std::cerr << "Error: Failed to load master call '" << masterCallId << "'." << std::endl;
        engine.shutdown();
        return 1;
    }

    // Load the recording
    std::cout << "\n2. Loading recording..." << std::endl;
    unsigned int channels, sampleRate;
    std::vector<float> audioData = load_audio_file(recordingPath, channels, sampleRate);

    if (audioData.empty()) {
        std::cerr << "Failed to load recording!" << std::endl;
        engine.shutdown();
        return 1;
    }

    // Process through real-time session
    std::cout << "\n3. Starting analysis session..." << std::endl;
    // UPDATED: The function returns a Result object. We must check its status before using the
    // value.
    auto sessionResult = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
    if (!sessionResult.isOk()) {
        std::cerr << "Error: Failed to start a realtime session." << std::endl;
        engine.shutdown();
        return 1;
    }
    int sessionId = sessionResult.value;

    // Process in chunks (simulating real-time)
    const int chunkSize = 1024;
    int totalChunks = 0;
    for (size_t i = 0; i < audioData.size(); i += chunkSize) {
        size_t remainingSamples = audioData.size() - i;
        size_t samplesToProcess = (remainingSamples < chunkSize) ? remainingSamples : chunkSize;

        // UPDATED: Also check the status of chunk processing.
        auto processStatus =
            engine.processAudioChunk(sessionId, audioData.data() + i, samplesToProcess);
        if (processStatus != HuntmasterAudioEngine::EngineStatus::OK) {
            std::cerr << "\nError processing audio chunk. Aborting." << std::endl;
            break;  // Exit the loop if a chunk fails
        }

        totalChunks++;
        if (totalChunks % 10 == 0) {
            std::cout << ".";
            std::cout.flush();
        }
    }
    std::cout << std::endl;

    // Get final score
    std::cout << "\n4. Calculating similarity score..." << std::endl;
    // UPDATED: This also returns a Result object. Check status, then get value.
    auto scoreResult = engine.getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cerr << "Error: Could not calculate similarity score." << std::endl;
        engine.endRealtimeSession(sessionId);
        engine.shutdown();
        return 1;
    }
    float score = scoreResult.value;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Recording: " << recordingPath << std::endl;
    std::cout << "Duration: " << (float)audioData.size() / sampleRate << " seconds" << std::endl;
    std::cout << "Similarity Score: " << score << std::endl;
    std::cout << "========================================" << std::endl;

    // Note: Higher scores = better match (score = 1/(1+distance))
    // Bonus Tip: Using named constants makes this more readable than "magic numbers".
    const float EXCELLENT_THRESHOLD = 0.01f;
    const float GOOD_THRESHOLD = 0.005f;
    const float FAIR_THRESHOLD = 0.002f;
    const float SOME_SIMILARITY_THRESHOLD = 0.001f;

    if (score > EXCELLENT_THRESHOLD) {
        std::cout << "Interpretation: EXCELLENT match to master call!" << std::endl;
    } else if (score > GOOD_THRESHOLD) {
        std::cout << "Interpretation: Good match to master call" << std::endl;
    } else if (score > FAIR_THRESHOLD) {
        std::cout << "Interpretation: Fair match to master call" << std::endl;
    } else if (score > SOME_SIMILARITY_THRESHOLD) {
        std::cout << "Interpretation: Some similarity to master call" << std::endl;
    } else {
        std::cout << "Interpretation: Different from master call" << std::endl;
    }

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    return 0;
}