#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

int main() {
    std::cout << "=== Quick MFCC Integration Test ===" << std::endl;

    // Get engine instance
    auto& engine = huntmaster::HuntmasterAudioEngine::getInstance();
    engine.initialize();
    std::cout << "✅ Engine initialized successfully" << std::endl;

    // Try to load an existing master call
    auto loadResult = engine.loadMasterCall("buck_grunt");
    if (loadResult != huntmaster::HuntmasterAudioEngine::EngineStatus::OK) {
        std::cout << "❌ Failed to load buck_grunt: " << static_cast<int>(loadResult) << std::endl;
    } else {
        std::cout << "✅ Master call loaded successfully" << std::endl;

        // Start a session
        auto sessionResult = engine.startRealtimeSession(44100.0f, 4096);
        if (!sessionResult.isOk()) {
            std::cout << "❌ Failed to start session: " << static_cast<int>(sessionResult.status)
                      << std::endl;
        } else {
            int sessionId = sessionResult.value;
            std::cout << "✅ Session started successfully (ID: " << sessionId << ")" << std::endl;

            // Create some test audio data (440Hz sine wave)
            const int sampleRate = 44100;
            const int duration = 2;  // 2 seconds
            const int numSamples = sampleRate * duration;
            std::vector<float> testAudio(numSamples);

            for (int i = 0; i < numSamples; ++i) {
                testAudio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
            }

            std::cout << "📊 Generated " << numSamples << " samples of 440Hz sine wave"
                      << std::endl;

            // Process audio in chunks
            const int chunkSize = 4096;
            int totalChunks = 0;
            for (int i = 0; i < numSamples; i += chunkSize) {
                int currentChunkSize = std::min(chunkSize, numSamples - i);

                auto processResult =
                    engine.processAudioChunk(sessionId, &testAudio[i], currentChunkSize);
                if (processResult != huntmaster::HuntmasterAudioEngine::EngineStatus::OK) {
                    std::cout << "❌ Failed to process chunk " << totalChunks << ": "
                              << static_cast<int>(processResult) << std::endl;
                } else {
                    totalChunks++;
                }
            }

            std::cout << "✅ Processed " << totalChunks << " audio chunks" << std::endl;

            // Check feature count
            int featureCount = engine.getSessionFeatureCount(sessionId);
            std::cout << "📊 Features extracted: " << featureCount << std::endl;

            // Try to get similarity score
            auto scoreResult = engine.getSimilarityScore(sessionId);
            if (!scoreResult.isOk()) {
                std::cout << "❌ Failed to get similarity score: "
                          << static_cast<int>(scoreResult.status) << std::endl;
                std::cout << "   Error code meaning: ";
                switch (scoreResult.status) {
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::INSUFFICIENT_DATA:
                        std::cout << "INSUFFICIENT_DATA (features not extracted)";
                        break;
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::FILE_NOT_FOUND:
                        std::cout << "FILE_NOT_FOUND";
                        break;
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::INVALID_SESSION:
                        std::cout << "INVALID_SESSION";
                        break;
                    default:
                        std::cout << "Unknown error";
                }
                std::cout << std::endl;
            } else {
                std::cout << "✅ Similarity score: " << scoreResult.value << std::endl;
            }

            // Stop the session
            engine.endRealtimeSession(sessionId);
            std::cout << "✅ Session ended successfully" << std::endl;
        }
    }

    engine.shutdown();
    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
