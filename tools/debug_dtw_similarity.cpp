/**
 * @file debug_dtw_similarity.cpp
 * @brief Debug tool to analyze DTW similarity scoring issues
 *
 * This tool helps investigate why self-similarity scores are low
 * and provides detailed analysis of the DTW calculation process.
 */

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

int main() {
    std::cout << "=== DTW Similarity Debugging Tool ===" << std::endl;

    HuntmasterAudioEngine& engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load master call
    std::cout << "Loading master call..." << std::endl;
    auto loadResult = engine.loadMasterCall("buck_grunt");
    if (loadResult != HuntmasterAudioEngine::EngineStatus::OK) {
        std::cerr << "Failed to load master call: " << static_cast<int>(loadResult) << std::endl;
        return 1;
    }
    std::cout << "✅ Master call loaded successfully" << std::endl;

    // Load the same audio file
    std::cout << "Loading identical audio for comparison..." << std::endl;
    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float* audioData = drwav_open_file_and_read_pcm_frames_f32(
        "data/master_calls/buck_grunt.wav", &channels, &sampleRate, &totalFrames, nullptr);

    if (!audioData) {
        std::cerr << "Failed to load audio file" << std::endl;
        return 1;
    }

    std::cout << "✅ Audio loaded: " << totalFrames << " frames, " << channels << " channels, "
              << sampleRate << " Hz" << std::endl;

    // Convert to mono if needed
    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < totalFrames; ++i) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[i * channels + ch];
            }
            monoData[i] = sum / channels;
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    // Start session and process
    std::cout << "\nStarting processing session..." << std::endl;
    auto sessionResult = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to start session" << std::endl;
        drwav_free(audioData, nullptr);
        return 1;
    }

    int sessionId = sessionResult.value;
    std::cout << "✅ Session started with ID: " << sessionId << std::endl;

    // Process the audio
    auto processResult = engine.processAudioChunk(sessionId, monoData.data(), monoData.size());
    if (processResult != HuntmasterAudioEngine::EngineStatus::OK) {
        std::cerr << "Failed to process audio: " << static_cast<int>(processResult) << std::endl;
        engine.endRealtimeSession(sessionId);
        drwav_free(audioData, nullptr);
        return 1;
    }

    std::cout << "✅ Audio processed successfully" << std::endl;

    // Get feature counts
    int featureCount = engine.getSessionFeatureCount(sessionId);
    std::cout << "Session feature count: " << featureCount << std::endl;

    // Get similarity score and analyze it
    auto scoreResult = engine.getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cerr << "Failed to get similarity score: " << static_cast<int>(scoreResult.status)
                  << std::endl;
        engine.endRealtimeSession(sessionId);
        drwav_free(audioData, nullptr);
        return 1;
    }

    float score = scoreResult.value;
    std::cout << "\n=== SIMILARITY ANALYSIS ===" << std::endl;
    std::cout << "Self-similarity score: " << std::fixed << std::setprecision(8) << score
              << std::endl;

    // Calculate what the DTW distance must be
    // Score = 1 / (1 + distance), so distance = (1 - score) / score
    float impliedDistance = (1.0f - score) / score;
    std::cout << "Implied DTW distance: " << impliedDistance << std::endl;

    // Calculate per-frame distance
    if (featureCount > 0) {
        float perFrameDistance = impliedDistance / featureCount;
        std::cout << "Per-frame distance: " << perFrameDistance << std::endl;
        std::cout << "Per-frame distance (sqrt): " << std::sqrt(perFrameDistance) << std::endl;
    }

    std::cout << "\n=== EXPECTED vs ACTUAL ===" << std::endl;
    std::cout << "Expected self-similarity: ~1.0 (perfect match)" << std::endl;
    std::cout << "Actual self-similarity: " << score << std::endl;

    if (score < 0.1) {
        std::cout
            << "❌ ISSUE: Very low self-similarity suggests DTW distance normalization problem"
            << std::endl;
        std::cout << "   - DTW distance should be close to 0 for identical sequences" << std::endl;
        std::cout << "   - Current distance (" << impliedDistance << ") is too high" << std::endl;
        std::cout << "   - Possible causes:" << std::endl;
        std::cout << "     * Numerical precision issues in MFCC extraction" << std::endl;
        std::cout << "     * DTW algorithm implementation issues" << std::endl;
        std::cout << "     * Feature vector differences due to processing" << std::endl;
    } else if (score < 0.8) {
        std::cout << "⚠️  WARNING: Low self-similarity suggests minor issues" << std::endl;
        std::cout << "   - Should investigate feature extraction consistency" << std::endl;
    } else {
        std::cout << "✅ GOOD: High self-similarity as expected" << std::endl;
    }

    // Test with a slightly different signal to see relative scoring
    std::cout << "\n=== RELATIVE SIMILARITY TEST ===" << std::endl;
    std::cout << "Testing with slightly modified signal..." << std::endl;

    // Create a slightly modified version (add small noise)
    std::vector<float> noisyData = monoData;
    for (size_t i = 0; i < noisyData.size(); ++i) {
        noisyData[i] +=
            0.01f * sin(2.0f * M_PI * 1000.0f * i / sampleRate);  // Add 1kHz sine at 1% amplitude
    }

    auto session2Result = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
    if (session2Result.isOk()) {
        int session2Id = session2Result.value;
        auto process2Result =
            engine.processAudioChunk(session2Id, noisyData.data(), noisyData.size());

        if (process2Result == HuntmasterAudioEngine::EngineStatus::OK) {
            auto score2Result = engine.getSimilarityScore(session2Id);
            if (score2Result.isOk()) {
                float score2 = score2Result.value;
                std::cout << "Noisy signal similarity: " << std::fixed << std::setprecision(8)
                          << score2 << std::endl;
                std::cout << "Difference: " << (score - score2) << std::endl;

                if (score > score2) {
                    std::cout << "✅ GOOD: Original has higher similarity than noisy version"
                              << std::endl;
                } else {
                    std::cout << "❌ ISSUE: Noisy version has equal or higher similarity"
                              << std::endl;
                }
            }
        }
        engine.endRealtimeSession(session2Id);
    }

    // Cleanup
    engine.endRealtimeSession(sessionId);
    drwav_free(audioData, nullptr);
    engine.shutdown();

    std::cout << "\n=== RECOMMENDATIONS ===" << std::endl;
    if (score < 0.1) {
        std::cout << "1. Check DTW distance normalization - consider dividing by sequence length"
                  << std::endl;
        std::cout
            << "2. Verify MFCC feature extraction produces identical results for identical input"
            << std::endl;
        std::cout
            << "3. Consider using a different similarity metric (cosine similarity, correlation)"
            << std::endl;
        std::cout << "4. Investigate if the DTW algorithm implementation is correct" << std::endl;
    }

    return 0;
}
