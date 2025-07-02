#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <string_view>

// These headers are needed for the tool's functionality
#include "huntmaster_engine/HuntmasterAudioEngine.h"
#include "dr_wav.h"
// Let's assume we have a shared audio utility header like we discussed
// #include "shared/audio_utils.h"

// For now, we'll include the function directly until we make the shared header
namespace AudioUtils
{
    std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels, unsigned int &sampleRate)
    {
        drwav_uint64 totalPCMFrameCount = 0;
        float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
        if (!pSampleData)
        {
            std::cerr << "Error: Could not load audio file: " << filePath << std::endl;
            return {};
        }
        std::cout << "  - Loaded: " << filePath << std::endl;
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
}

using huntmaster::HuntmasterAudioEngine;

int main(int argc, char *argv[])
{
    std::cout << "=== Detailed Recording Analysis ===" << std::endl;

    if (argc < 2)
    {
        std::cout << "\nUsage: " << argv[0] << " <path_to_your_recording.wav>" << std::endl;
        return 1;
    }

    std::string recordingPath = argv[1];
    std::cout << "\nAnalyzing recording: " << recordingPath << std::endl;

    // --- 1. Load the User's Recording ---
    unsigned int channels, sampleRate;
    std::vector<float> recordingAudio = AudioUtils::load_audio_file(recordingPath, channels, sampleRate);
    if (recordingAudio.empty())
    {
        return 1;
    }

    // --- 2. Initialize the Engine ---
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // --- 3. Iterate and Compare Against All Master Calls ---
    std::vector<std::string> masterCalls = {
        "buck_grunt", "doe-grunt", "buck-bawl", "breeding_bellow",
        "contact-bleatr", "estrus_bleat", "fawn-bleat"
        // Add all your master call IDs here
    };

    float bestScore = -1.0f;
    std::string bestMatchName = "None";

    std::cout << "\nComparing against all master calls...\n"
              << std::string(40, '-') << std::endl;

    for (const auto &masterId : masterCalls)
    {
        // A. Load the master call
        if (engine.loadMasterCall(masterId) != HuntmasterAudioEngine::EngineStatus::OK)
        {
            std::cerr << "Could not load master call: " << masterId << ". Skipping." << std::endl;
            continue;
        }

        // B. Start a session for this comparison
        auto sessionResult = engine.startRealtimeSession(static_cast<float>(sampleRate), 4096);
        if (!sessionResult.isOk())
        {
            std::cerr << "Could not start session for " << masterId << ". Skipping." << std::endl;
            continue;
        }
        int sessionId = sessionResult.value;

        // C. Process the entire user recording
        if (engine.processAudioChunk(sessionId, recordingAudio.data(), recordingAudio.size()) != HuntmasterAudioEngine::EngineStatus::OK)
        {
            std::cerr << "Could not process audio for " << masterId << ". Skipping." << std::endl;
            engine.endRealtimeSession(sessionId);
            continue;
        }

        // D. Get the score
        auto scoreResult = engine.getSimilarityScore(sessionId);
        if (scoreResult.isOk())
        {
            float currentScore = scoreResult.value;
            std::cout << "  - vs " << std::setw(20) << std::left << masterId
                      << " -> Score: " << std::fixed << std::setprecision(5) << currentScore << std::endl;

            if (currentScore > bestScore)
            {
                bestScore = currentScore;
                bestMatchName = masterId;
            }
        }
        else
        {
            std::cout << "  - vs " << std::setw(20) << std::left << masterId
                      << " -> Error calculating score." << std::endl;
        }

        // E. Clean up the session for the next loop
        engine.endRealtimeSession(sessionId);
    }

    // --- 4. Report the Best Match ---
    std::cout << "\n========================================" << std::endl;
    std::cout << "           ANALYSIS COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Recording '" << recordingPath << "'\nmost closely matches:\n"
              << std::endl;
    std::cout << "  -> Master Call: " << bestMatchName << std::endl;
    std::cout << "  -> Similarity Score: " << bestScore << std::endl;
    std::cout << "========================================\n"
              << std::endl;

    engine.shutdown();
    return 0;
}