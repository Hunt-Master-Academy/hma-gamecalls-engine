#include <iostream>
#include <string>
#include <string_view>  // Use string_view for API consistency
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

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

    // Create engine instance
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine instance!" << std::endl;
        return -1;
    }
    auto engine = std::move(engineResult.value);

    // --- 2. Load the User's Attempt Audio ---
    std::cout << "\n[Step 2] Loading User Attempt Audio..." << std::endl;
    unsigned int channels, sampleRate;
    std::vector<float> userAttemptAudio =
        load_audio_file("../data/master_calls/buck_grunt.wav", channels, sampleRate);
    if (userAttemptAudio.empty()) {
        return 1;
    }

    // --- 3. Create Session ---
    std::cout << "\n[Step 3] Creating Session..." << std::endl;
    auto sessionResult = engine->createSession(static_cast<float>(sampleRate));
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session!" << std::endl;
        return 1;
    }
    int sessionId = sessionResult.value;
    std::cout << "  - Session started with ID: " << sessionId << std::endl;

    // --- 4. Load Master Call (per session) ---
    std::cout << "\n[Step 4] Loading Master Call for Session..." << std::endl;
    auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
    if (masterStatus != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to load master call for session!" << std::endl;
        engine->destroySession(sessionId);
        return 1;
    }

    // --- 5. Process Audio ---
    std::cout << "\n[Step 5] Processing Audio..." << std::endl;
    auto processStatus =
        engine->processAudioChunk(sessionId, std::span<const float>(userAttemptAudio));
    if (processStatus != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process audio chunk!" << std::endl;
        engine->destroySession(sessionId);
        return 1;
    }
    std::cout << "  - Processed user audio." << std::endl;

    // --- 6. Get the Final Score ---
    std::cout << "\n[Step 6] Calculating Final Score..." << std::endl;
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cerr << "Failed to get similarity score!" << std::endl;
        engine->destroySession(sessionId);
        return 1;
    }
    float finalScore = scoreResult.value;

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "  Final Similarity Score: " << finalScore << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    engine->destroySession(sessionId);

    std::cout << "\n--- Test Harness Finished ---" << std::endl;

    return 0;
}