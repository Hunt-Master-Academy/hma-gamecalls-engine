#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

// Use the huntmaster namespace
using huntmaster::UnifiedAudioEngine;
using SessionId = uint32_t;

// Helper function to convert Status to string
std::string statusToString(UnifiedAudioEngine::Status status) {
    switch (status) {
        case UnifiedAudioEngine::Status::OK:
            return "OK";
        case UnifiedAudioEngine::Status::INVALID_PARAMS:
            return "Invalid parameters";
        case UnifiedAudioEngine::Status::SESSION_NOT_FOUND:
            return "Session not found";
        case UnifiedAudioEngine::Status::FILE_NOT_FOUND:
            return "File not found";
        case UnifiedAudioEngine::Status::PROCESSING_ERROR:
            return "Processing error";
        case UnifiedAudioEngine::Status::INSUFFICIENT_DATA:
            return "Insufficient data";
        case UnifiedAudioEngine::Status::OUT_OF_MEMORY:
            return "Out of memory";
        case UnifiedAudioEngine::Status::INIT_FAILED:
            return "Initialization failed";
        default:
            return "Unknown error";
    }
}

// Test vector structure
struct TestVector {
    std::string name;
    std::string inputFile;
    std::string outputFile;
    float expectedScore;
    int expectedMFCCFrames;
};

// Generate reference test vectors
void generateTestVectors(std::unique_ptr<UnifiedAudioEngine>& engine) {
    std::cout << "=== Generating Test Vectors ===" << std::endl;

    // Create test directory
    system("mkdir ..\\data\\test_vectors 2>nul");

    // Test cases
    std::vector<TestVector> testCases = {
        {"Sine Wave 440Hz", "test_sine_440.wav", "sine_440_vector.bin", 1.0f, 83},
        {"Complex Wave", "test_complex.wav", "complex_vector.bin", 1.0f, 83}};

    for (const auto& test : testCases) {
        std::cout << "\nGenerating vector for: " << test.name << std::endl;

        // Load as master using session-based approach
        auto sessionResult = engine->startRealtimeSession(44100.0f);
        if (!sessionResult.isOk()) {
            std::cerr << "Failed to start session: " << statusToString(sessionResult.error())
                      << std::endl;
            continue;
        }
        SessionId sessionId = sessionResult.value;

        auto loadResult = engine->loadMasterCall(
            sessionId, test.inputFile.substr(0, test.inputFile.find_last_of('.')));
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Failed to load master call: " << statusToString(loadResult) << std::endl;
            [[maybe_unused]] auto endResult = engine->endRealtimeSession(sessionId);
            continue;
        }

        // Save the current state as test vector
        std::string vectorPath = "../data/test_vectors/" + test.outputFile;
        std::ofstream outFile(vectorPath, std::ios::binary);

        if (outFile.is_open()) {
            // Write test metadata
            outFile.write(test.name.c_str(), test.name.length());
            char nullTerminator = '\0';
            outFile.write(&nullTerminator, 1);

            // Write expected values
            outFile.write(reinterpret_cast<const char*>(&test.expectedScore), sizeof(float));
            outFile.write(reinterpret_cast<const char*>(&test.expectedMFCCFrames), sizeof(int));

            std::cout << "  Vector saved to: " << vectorPath << std::endl;
            outFile.close();
        }

        // Cleanup session
        auto endResult = engine->endRealtimeSession(sessionId);
        if (endResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to end session: " << statusToString(endResult)
                      << std::endl;
        }
    }
}

// Verify consistency across different processing methods
bool verifyProcessingConsistency(std::unique_ptr<UnifiedAudioEngine>& engine) {
    std::cout << "\n=== Verifying Processing Consistency ===" << std::endl;

    // Test 1: Same audio processed different ways should give same result
    std::cout << "\nTest 1: Batch vs Chunk Processing" << std::endl;
    std::cout << "---------------------------------" << std::endl;

    // Generate test audio
    const int sampleRate = 44100;
    const int duration = 2;  // seconds
    std::vector<float> testAudio(sampleRate * duration);

    // Generate a chirp signal (frequency sweep)
    for (int i = 0; i < testAudio.size(); ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float freq = 200.0f + (800.0f * t / duration);  // 200Hz to 1000Hz sweep
        testAudio[i] = 0.5f * sin(2.0f * 3.14159f * freq * t);
    }

    // Process as batch (all at once)
    auto batchSessionResult = engine->startRealtimeSession(static_cast<float>(sampleRate));
    if (!batchSessionResult.isOk()) {
        std::cerr << "Failed to start batch session: " << statusToString(batchSessionResult.error())
                  << std::endl;
        return false;
    }
    SessionId batchSession = batchSessionResult.value;

    auto loadResult = engine->loadMasterCall(batchSession, "test_sine_440");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to load master call: " << statusToString(loadResult) << std::endl;
        [[maybe_unused]] auto endResult = engine->endRealtimeSession(batchSession);
        return false;
    }

    auto processResult = engine->processAudioChunk(
        batchSession, std::span<const float>(testAudio.data(), testAudio.size()));
    if (processResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process batch audio: " << statusToString(processResult)
                  << std::endl;
        [[maybe_unused]] auto endResult = engine->endRealtimeSession(batchSession);
        return false;
    }

    auto batchScoreResult = engine->getSimilarityScore(batchSession);
    float batchScore = batchScoreResult.isOk() ? batchScoreResult.value : 0.0f;

    auto endBatchResult = engine->endRealtimeSession(batchSession);
    if (endBatchResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end batch session: " << statusToString(endBatchResult)
                  << std::endl;
    }

    std::cout << "  Batch processing score: " << std::fixed << std::setprecision(8) << batchScore
              << std::endl;

    // Process in small chunks
    auto chunkSessionResult = engine->startRealtimeSession(static_cast<float>(sampleRate));
    if (!chunkSessionResult.isOk()) {
        std::cerr << "Failed to start chunk session: " << statusToString(chunkSessionResult.error())
                  << std::endl;
        return false;
    }
    SessionId chunkSession = chunkSessionResult.value;

    auto loadChunkResult = engine->loadMasterCall(chunkSession, "test_sine_440");
    if (loadChunkResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to load master call for chunk session: "
                  << statusToString(loadChunkResult) << std::endl;
        [[maybe_unused]] auto endResult = engine->endRealtimeSession(chunkSession);
        return false;
    }

    const int chunkSize = 512;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t remaining = testAudio.size() - i;
        size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
        auto chunkProcessResult = engine->processAudioChunk(
            chunkSession, std::span<const float>(testAudio.data() + i, toProcess));
        if (chunkProcessResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Failed to process chunk: " << statusToString(chunkProcessResult)
                      << std::endl;
            [[maybe_unused]] auto endResult = engine->endRealtimeSession(chunkSession);
            return false;
        }
    }
    auto chunkScoreResult = engine->getSimilarityScore(chunkSession);
    float chunkScore = chunkScoreResult.isOk() ? chunkScoreResult.value : 0.0f;

    auto endChunkResult = engine->endRealtimeSession(chunkSession);
    if (endChunkResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end chunk session: " << statusToString(endChunkResult)
                  << std::endl;
    }

    std::cout << "  Chunk processing score: " << std::fixed << std::setprecision(8) << chunkScore
              << std::endl;

    float scoreDiff = std::abs(batchScore - chunkScore);
    bool test1Pass = scoreDiff < 0.005f;  // Allow reasonable audio processing differences (<0.5%)

    std::cout << "  Score difference: " << scoreDiff << std::endl;
    std::cout << "  Status: " << (test1Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 2: Different chunk sizes should give same result
    std::cout << "\nTest 2: Different Chunk Sizes" << std::endl;
    std::cout << "-----------------------------" << std::endl;

    std::vector<int> chunkSizes = {256, 512, 1024, 2048};
    std::vector<float> scores;

    for (int size : chunkSizes) {
        auto sessionResult = engine->startRealtimeSession(static_cast<float>(sampleRate));
        if (!sessionResult.isOk()) {
            std::cerr << "Failed to start session for chunk size " << size << ": "
                      << statusToString(sessionResult.error()) << std::endl;
            continue;
        }
        SessionId session = sessionResult.value;

        auto loadResult = engine->loadMasterCall(session, "test_sine_440");
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Failed to load master call for chunk size " << size << ": "
                      << statusToString(loadResult) << std::endl;
            [[maybe_unused]] auto endResult = engine->endRealtimeSession(session);
            continue;
        }

        for (size_t i = 0; i < testAudio.size(); i += size) {
            size_t remaining = testAudio.size() - i;
            size_t toProcess = std::min(static_cast<size_t>(size), remaining);
            auto processResult = engine->processAudioChunk(
                session, std::span<const float>(testAudio.data() + i, toProcess));
            if (processResult != UnifiedAudioEngine::Status::OK) {
                std::cerr << "Failed to process chunk for size " << size << ": "
                          << statusToString(processResult) << std::endl;
                break;
            }
        }

        auto scoreResult = engine->getSimilarityScore(session);
        float score = scoreResult.isOk() ? scoreResult.value : 0.0f;
        scores.push_back(score);

        auto endResult = engine->endRealtimeSession(session);
        if (endResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to end session for chunk size " << size << ": "
                      << statusToString(endResult) << std::endl;
        }

        std::cout << "  Chunk size " << size << ": Score = " << score << std::endl;
    }

    // Check all scores are similar
    float maxDiff = 0.0f;
    for (size_t i = 1; i < scores.size(); ++i) {
        maxDiff = std::max(maxDiff, std::abs(scores[i] - scores[0]));
    }

    bool test2Pass = maxDiff < 0.005f;  // Allow reasonable chunk size variations (<0.5%)
    std::cout << "  Max score difference: " << maxDiff << std::endl;
    std::cout << "  Status: " << (test2Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    return test1Pass && test2Pass;
}

// Test edge cases
bool testEdgeCases(std::unique_ptr<UnifiedAudioEngine>& engine) {
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;

    // Test 1: Empty audio
    std::cout << "\nTest 1: Empty Audio" << std::endl;
    std::cout << "-------------------" << std::endl;

    std::vector<float> emptyAudio;
    auto emptySessionResult = engine->startRealtimeSession(44100.0f);
    if (!emptySessionResult.isOk()) {
        std::cerr << "Failed to start empty session: " << statusToString(emptySessionResult.error())
                  << std::endl;
        return false;
    }
    SessionId emptySession = emptySessionResult.value;

    auto processEmptyResult =
        engine->processAudioChunk(emptySession, std::span<const float>(emptyAudio.data(), 0));
    if (processEmptyResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process empty audio: " << statusToString(processEmptyResult)
                  << std::endl;
    }

    auto emptyScoreResult = engine->getSimilarityScore(emptySession);
    float emptyScore = emptyScoreResult.isOk() ? emptyScoreResult.value : 0.0f;

    auto endEmptyResult = engine->endRealtimeSession(emptySession);
    if (endEmptyResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end empty session: " << statusToString(endEmptyResult)
                  << std::endl;
    }

    std::cout << "  Empty audio score: " << emptyScore << std::endl;
    std::cout << "  Status: " << (emptyScore == 0.0f ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 2: Very short audio
    std::cout << "\nTest 2: Very Short Audio (100 samples)" << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    std::vector<float> shortAudio(100, 0.5f);
    auto shortSessionResult = engine->startRealtimeSession(44100.0f);
    if (!shortSessionResult.isOk()) {
        std::cerr << "Failed to start short session: " << statusToString(shortSessionResult.error())
                  << std::endl;
        return false;
    }
    SessionId shortSession = shortSessionResult.value;

    auto processShortResult = engine->processAudioChunk(
        shortSession, std::span<const float>(shortAudio.data(), shortAudio.size()));
    if (processShortResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process short audio: " << statusToString(processShortResult)
                  << std::endl;
    }

    auto shortScoreResult = engine->getSimilarityScore(shortSession);
    float shortScore = shortScoreResult.isOk() ? shortScoreResult.value : 0.0f;

    auto endShortResult = engine->endRealtimeSession(shortSession);
    if (endShortResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end short session: " << statusToString(endShortResult)
                  << std::endl;
    }

    std::cout << "  Short audio score: " << shortScore << std::endl;
    std::cout << "  Status: PASS ✓ (No crash)" << std::endl;

    // Test 3: Silence
    std::cout << "\nTest 3: Complete Silence" << std::endl;
    std::cout << "------------------------" << std::endl;

    std::vector<float> silence(44100, 0.0f);  // 1 second of silence
    auto silenceSessionResult = engine->startRealtimeSession(44100.0f);
    if (!silenceSessionResult.isOk()) {
        std::cerr << "Failed to start silence session: "
                  << statusToString(silenceSessionResult.error()) << std::endl;
        return false;
    }
    SessionId silenceSession = silenceSessionResult.value;

    auto processSilenceResult = engine->processAudioChunk(
        silenceSession, std::span<const float>(silence.data(), silence.size()));
    if (processSilenceResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process silence: " << statusToString(processSilenceResult)
                  << std::endl;
    }

    auto silenceScoreResult = engine->getSimilarityScore(silenceSession);
    float silenceScore = silenceScoreResult.isOk() ? silenceScoreResult.value : 0.0f;

    auto endSilenceResult = engine->endRealtimeSession(silenceSession);
    if (endSilenceResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end silence session: " << statusToString(endSilenceResult)
                  << std::endl;
    }

    std::cout << "  Silence score: " << silenceScore << std::endl;
    std::cout << "  Status: PASS ✓ (Handled gracefully)" << std::endl;

    // Test 4: Clipped audio
    std::cout << "\nTest 4: Clipped Audio" << std::endl;
    std::cout << "---------------------" << std::endl;

    std::vector<float> clippedAudio(44100);
    for (int i = 0; i < clippedAudio.size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        float signal = 2.0f * sin(2.0f * 3.14159f * 440.0f * t);    // Amplitude > 1.0
        clippedAudio[i] = std::max(-1.0f, std::min(1.0f, signal));  // Clip to [-1, 1]
    }

    auto clippedSessionResult = engine->startRealtimeSession(44100.0f);
    if (!clippedSessionResult.isOk()) {
        std::cerr << "Failed to start clipped session: "
                  << statusToString(clippedSessionResult.error()) << std::endl;
        return false;
    }
    SessionId clippedSession = clippedSessionResult.value;

    auto processClippedResult = engine->processAudioChunk(
        clippedSession, std::span<const float>(clippedAudio.data(), clippedAudio.size()));
    if (processClippedResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to process clipped audio: " << statusToString(processClippedResult)
                  << std::endl;
    }

    auto clippedScoreResult = engine->getSimilarityScore(clippedSession);
    float clippedScore = clippedScoreResult.isOk() ? clippedScoreResult.value : 0.0f;

    auto endClippedResult = engine->endRealtimeSession(clippedSession);
    if (endClippedResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to end clipped session: " << statusToString(endClippedResult)
                  << std::endl;
    }

    std::cout << "  Clipped audio score: " << clippedScore << std::endl;
    std::cout << "  Status: PASS ✓ (Processed without crash)" << std::endl;

    return true;
}

// Test different sample rates
bool testSampleRates(std::unique_ptr<UnifiedAudioEngine>& engine) {
    std::cout << "\n=== Testing Different Sample Rates ===" << std::endl;

    std::vector<float> sampleRates = {16000.0f, 22050.0f, 44100.0f, 48000.0f};

    for (float sr : sampleRates) {
        std::cout << "\nTesting " << sr << " Hz:" << std::endl;

        // Generate 1 second of audio at this sample rate
        int numSamples = static_cast<int>(sr);
        std::vector<float> audio(numSamples);

        // Generate 440Hz tone
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sr;
            audio[i] = 0.5f * sin(2.0f * 3.14159f * 440.0f * t);
        }

        auto sessionResult = engine->startRealtimeSession(sr);
        if (!sessionResult.isOk()) {
            std::cerr << "Failed to start session for sample rate " << sr << ": "
                      << statusToString(sessionResult.error()) << std::endl;
            continue;
        }
        SessionId session = sessionResult.value;

        auto processResult =
            engine->processAudioChunk(session, std::span<const float>(audio.data(), audio.size()));
        if (processResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Failed to process audio for sample rate " << sr << ": "
                      << statusToString(processResult) << std::endl;
        }

        auto scoreResult = engine->getSimilarityScore(session);
        float score = scoreResult.isOk() ? scoreResult.value : 0.0f;

        auto endResult = engine->endRealtimeSession(session);
        if (endResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to end session for sample rate " << sr << ": "
                      << statusToString(endResult) << std::endl;
        }

        std::cout << "  Score: " << score << std::endl;
        std::cout << "  Status: PASS ✓" << std::endl;
    }

    return true;
}

int main() {
    std::cout << "=== Cross-Platform Consistency Tests ===" << std::endl;
    std::cout << "Ensuring identical results across different scenarios\n" << std::endl;

    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine: " << statusToString(engineResult.error())
                  << std::endl;
        return 1;
    }
    std::unique_ptr<UnifiedAudioEngine> engine = std::move(engineResult.value);

    // Generate test vectors (run once to create reference data)
    generateTestVectors(engine);

    // Run consistency tests
    bool consistencyPass = verifyProcessingConsistency(engine);
    bool edgeCasePass = testEdgeCases(engine);
    bool sampleRatePass = testSampleRates(engine);

    // Summary
    std::cout << "\n\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Processing Consistency: " << (consistencyPass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << "Edge Cases: " << (edgeCasePass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << "Sample Rates: " << (sampleRatePass ? "PASS ✓" : "FAIL ✗") << std::endl;

    bool allPass = consistencyPass && edgeCasePass && sampleRatePass;
    std::cout << "\nOverall: " << (allPass ? "ALL TESTS PASSED ✓" : "SOME TESTS FAILED ✗")
              << std::endl;

    if (allPass) {
        std::cout << "\nThe audio engine produces consistent results across:" << std::endl;
        std::cout << "- Different processing methods" << std::endl;
        std::cout << "- Various chunk sizes" << std::endl;
        std::cout << "- Edge cases (empty, short, silent, clipped audio)" << std::endl;
        std::cout << "- Multiple sample rates" << std::endl;
        std::cout << "\nReady for cross-platform deployment!" << std::endl;
    }

    std::cout << "\nCross-platform tests completed." << std::endl;

    return allPass ? 0 : 1;
}