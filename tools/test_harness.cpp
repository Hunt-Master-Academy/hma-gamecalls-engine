#include <iostream>
#include <vector>
// Use the standard include path. The CMakeLists.txt file tells the compiler where to find it.
#include "huntmaster_engine/HuntmasterAudioEngine.h"

// The dr_wav and dr_mp3 implementations are now compiled into the HuntmasterEngine
// library, so we only need to include the header files here for their declarations,
// without the implementation macros. This resolves the "multiple definition" linker error.
#include "dr_wav.h"

// Helper function to load an audio file into a float vector
std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels, unsigned int &sampleRate)
{
    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = nullptr;

    // Load WAV file
    pSampleData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr)
    {
        std::cerr << "TestHarness Error: Could not load audio file: " << filePath << std::endl;
        return {};
    }

    std::cout << "  - Test audio loaded: " << filePath << " (" << totalPCMFrameCount << " frames)" << std::endl;

    // Convert to mono for testing
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

int main()
{
    std::cout << "--- Huntmaster Engine Test Harness ---" << std::endl;

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // --- 1. Load the Master Call ---
    std::cout << "\n[Step 1] Loading Master Call..." << std::endl;
    engine.loadMasterCall("buck_grunt");

    // --- 2. Load the User's Attempt Audio ---
    std::cout << "\n[Step 2] Loading User Attempt Audio..." << std::endl;
    unsigned int channels, sampleRate;
    std::vector<float> userAttemptAudio = load_audio_file("buck_grunt.wav", channels, sampleRate);
    if (userAttemptAudio.empty())
    {
        engine.shutdown();
        return 1;
    }

    // --- 3. Simulate a Practice Session ---
    std::cout << "\n[Step 3] Simulating Practice Session..." << std::endl;
    int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);

    // Process the user's audio to generate MFCC features for the session
    engine.processAudioChunk(sessionId, userAttemptAudio.data(), userAttemptAudio.size());
    std::cout << "  - Processed user audio." << std::endl;

    // --- 4. Get the Final Score ---
    std::cout << "\n[Step 4] Calculating Final Score..." << std::endl;
    float finalScore = engine.getSimilarityScore(sessionId);
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "  Final Similarity Score: " << finalScore << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    std::cout << "\n--- Test Harness Finished ---" << std::endl;

    return 0;
}
