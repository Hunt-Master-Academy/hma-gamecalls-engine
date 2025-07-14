#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

int main() {
    std::cout << "=== HuntmasterAudioEngine MFCC Integration Test ===" << std::endl;

    using huntmaster::HuntmasterAudioEngine;
    auto& engine = HuntmasterAudioEngine::getInstance();

    // Initialize engine
    engine.initialize();
    std::cout << "Engine initialized" << std::endl;

    // Test 1: Try to load an existing master call
    std::cout << "\n--- Test 1: Load Master Call ---" << std::endl;
    auto loadResult = engine.loadMasterCall("buck_grunt");
    std::cout << "loadMasterCall result: " << static_cast<int>(loadResult) << std::endl;

    if (loadResult == HuntmasterAudioEngine::EngineStatus::OK) {
        std::cout << "✅ Master call loaded successfully!" << std::endl;
    } else {
        std::cout << "❌ Master call failed to load (status: " << static_cast<int>(loadResult)
                  << ")" << std::endl;
    }

    // Test 2: Start a realtime session with generated audio
    std::cout << "\n--- Test 2: Realtime Session with Generated Audio ---" << std::endl;

    // Create a 2-second sine wave
    const float sampleRate = 44100.0f;
    const float duration = 2.0f;
    const float frequency = 440.0f;
    const int numSamples = static_cast<int>(sampleRate * duration);

    std::vector<float> testAudio(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        testAudio[i] = 0.5f * sin(2.0f * M_PI * frequency * i / sampleRate);
    }

    std::cout << "Generated " << numSamples << " samples of 440Hz sine wave" << std::endl;

    // Start session
    auto sessionResult = engine.startRealtimeSession(sampleRate, 1024);
    if (!sessionResult.isOk()) {
        std::cout << "❌ Failed to start session: " << static_cast<int>(sessionResult.status)
                  << std::endl;
        engine.shutdown();
        return 1;
    }

    int sessionId = sessionResult.value;
    std::cout << "✅ Started session ID: " << sessionId << std::endl;

    // Process audio in chunks to simulate realistic usage
    const size_t chunkSize = 4096;  // Larger chunks
    size_t totalProcessed = 0;
    int successfulChunks = 0;

    std::cout << "Processing audio in " << chunkSize << "-sample chunks..." << std::endl;

    for (size_t i = 0; i + chunkSize <= testAudio.size(); i += chunkSize) {
        auto status = engine.processAudioChunk(sessionId, testAudio.data() + i, chunkSize);
        if (status == HuntmasterAudioEngine::EngineStatus::OK) {
            successfulChunks++;
        } else {
            std::cout << "❌ Chunk " << (i / chunkSize)
                      << " failed with status: " << static_cast<int>(status) << std::endl;
        }
        totalProcessed += chunkSize;
    }

    std::cout << "Processed " << successfulChunks << " chunks successfully" << std::endl;
    std::cout << "Total samples processed: " << totalProcessed << std::endl;

    // Check feature count
    int featureCount = engine.getSessionFeatureCount(sessionId);
    std::cout << "Features extracted: " << featureCount << std::endl;

    if (featureCount > 0) {
        std::cout << "✅ SUCCESS: " << featureCount << " features extracted!" << std::endl;

        // Try to get similarity score if we have a loaded master call
        if (loadResult == HuntmasterAudioEngine::EngineStatus::OK) {
            auto scoreResult = engine.getSimilarityScore(sessionId);
            if (scoreResult.isOk()) {
                std::cout << "✅ Similarity score: " << scoreResult.value << std::endl;
            } else {
                std::cout << "❌ Similarity score failed: " << static_cast<int>(scoreResult.status)
                          << std::endl;
            }
        }
    } else {
        std::cout << "❌ FAILED: No features extracted!" << std::endl;
    }

    // Test 3: All-at-once processing (like the failing test)
    std::cout << "\n--- Test 3: All-at-Once Processing ---" << std::endl;

    auto session2Result = engine.startRealtimeSession(sampleRate, 1024);
    if (session2Result.isOk()) {
        int session2Id = session2Result.value;

        // Process all audio at once
        auto allAtOnceStatus =
            engine.processAudioChunk(session2Id, testAudio.data(), testAudio.size());
        std::cout << "All-at-once processing status: " << static_cast<int>(allAtOnceStatus)
                  << std::endl;

        int features2 = engine.getSessionFeatureCount(session2Id);
        std::cout << "Features from all-at-once: " << features2 << std::endl;

        if (features2 > 0) {
            std::cout << "✅ All-at-once processing: SUCCESS" << std::endl;
        } else {
            std::cout << "❌ All-at-once processing: FAILED" << std::endl;
        }

        engine.endRealtimeSession(session2Id);
    }

    // Cleanup
    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Master call load: "
              << (loadResult == HuntmasterAudioEngine::EngineStatus::OK ? "SUCCESS" : "FAILED")
              << std::endl;
    std::cout << "Feature extraction: " << (featureCount > 0 ? "SUCCESS" : "FAILED") << std::endl;

    return (featureCount > 0) ? 0 : 1;
}
