#include <iostream>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

int main() {
    std::cout << "=== Quick MFCC Integration Test ===" << std::endl;

    // Initialize engine
    auto engine = huntmaster::HuntmasterAudioEngine::create();
    if (!engine.isOk()) {
        std::cout << "❌ Failed to create engine: " << static_cast<int>(engine.error())
                  << std::endl;
        return 1;
    }

    std::cout << "✅ Engine created successfully" << std::endl;
    auto& eng = *engine;

    // Try to load an existing master call
    auto loadResult = eng.loadMasterCall("buck_grunt");
    if (!loadResult.isOk()) {
        std::cout << "❌ Failed to load buck_grunt: " << static_cast<int>(loadResult.error())
                  << std::endl;
    } else {
        std::cout << "✅ Master call loaded successfully" << std::endl;

        // Start a session
        auto sessionResult = eng.startRealtimeSession();
        if (!sessionResult.isOk()) {
            std::cout << "❌ Failed to start session: " << static_cast<int>(sessionResult.error())
                      << std::endl;
        } else {
            std::cout << "✅ Session started successfully" << std::endl;

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
                std::vector<float> chunk(testAudio.begin() + i,
                                         testAudio.begin() + i + currentChunkSize);

                auto processResult = eng.processAudioChunk(chunk);
                if (!processResult.isOk()) {
                    std::cout << "❌ Failed to process chunk " << totalChunks << ": "
                              << static_cast<int>(processResult.error()) << std::endl;
                } else {
                    totalChunks++;
                }
            }

            std::cout << "✅ Processed " << totalChunks << " audio chunks" << std::endl;

            // Try to get similarity score
            auto scoreResult = eng.getSimilarityScore();
            if (!scoreResult.isOk()) {
                std::cout << "❌ Failed to get similarity score: "
                          << static_cast<int>(scoreResult.error()) << std::endl;
                std::cout << "   Error code meaning: ";
                switch (scoreResult.error()) {
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::INSUFFICIENT_DATA:
                        std::cout << "INSUFFICIENT_DATA (features not extracted)";
                        break;
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::NO_MASTER_CALL:
                        std::cout << "NO_MASTER_CALL";
                        break;
                    case huntmaster::HuntmasterAudioEngine::EngineStatus::NO_ACTIVE_SESSION:
                        std::cout << "NO_ACTIVE_SESSION";
                        break;
                    default:
                        std::cout << "Unknown error";
                }
                std::cout << std::endl;
            } else {
                std::cout << "✅ Similarity score: " << *scoreResult << std::endl;
            }

            // Stop the session
            auto stopResult = eng.stopRealtimeSession();
            if (!stopResult.isOk()) {
                std::cout << "❌ Failed to stop session: " << static_cast<int>(stopResult.error())
                          << std::endl;
            } else {
                std::cout << "✅ Session stopped successfully" << std::endl;
            }
        }
    }

    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
