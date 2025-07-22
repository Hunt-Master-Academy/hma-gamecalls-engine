#include <iostream>
#include <span>
#include <string>
#include <string_view>  // Use string_view for API consistency
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

// This helper function is duplicated in other tools. See the architectural tip below.
std::vector<float> load_audio_file(const std::string &filePath, unsigned int &channels,
                                   unsigned int &sampleRate) {
    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        std::cerr << "TestHarness Error: Could not load audio file: " << filePath << std::endl;
        return {};
    }

    std::cout << "  - Test audio loaded: " << filePath << " (" << totalPCMFrameCount << " frames)"
              << std::endl;

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

int main() {
    std::cout << "--- Huntmaster Engine Test Harness ---" << std::endl;

    // Create UnifiedAudioEngine instance
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return -1;
    }
    auto engine = std::move(*engineResult);

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session!" << std::endl;
        return -1;
    }
    SessionId sessionId = *sessionResult;

    // --- 1. Load the Master Call ---
    std::cout << "\n[Step 1] Loading Master Call..." << std::endl;
    if (engine->loadMasterCall(sessionId, "buck_grunt") != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to load master call!" << std::endl;
        return -1;
    }

    // --- 2. Load the User's Attempt Audio ---
    std::cout << "\n[Step 2] Loading User Attempt Audio..." << std::endl;
    unsigned int channels, sampleRate;
    // UPDATED: Path made more consistent with other tools
    std::vector<float> userAttemptAudio =
        load_audio_file("../data/master_calls/buck_grunt.wav", channels, sampleRate);
    if (userAttemptAudio.empty()) {
        return 1;
    }

    // --- 3. Simulate a Practice Session ---
    std::cout << "\n[Step 3] Simulating Practice Session..." << std::endl;
    // Process the user's audio to generate MFCC features for the session
    // Convert to span for modern API
    std::span<const float> audioSpan(userAttemptAudio.data(), userAttemptAudio.size());
    auto processResult = engine->processAudioChunk(sessionId, audioSpan);
    if (processResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process audio chunk!" << std::endl;
        auto resetResult = engine->resetSession(sessionId);
        if (resetResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to reset session!" << std::endl;
        }
        return 1;
    }
    std::cout << "  - Processed user audio." << std::endl;

    // --- 4. Get the Final Score ---
    std::cout << "\n[Step 4] Calculating Final Score..." << std::endl;
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cerr << "Failed to get similarity score!" << std::endl;
        auto resetResult = engine->resetSession(sessionId);
        if (resetResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to reset session!" << std::endl;
        }
        return 1;
    }
    float finalScore = *scoreResult;

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "  Final Similarity Score: " << finalScore << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    auto resetResult = engine->resetSession(sessionId);
    if (resetResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to reset session!" << std::endl;
    }

    std::cout << "\n--- Test Harness Finished ---" << std::endl;

    return 0;
}