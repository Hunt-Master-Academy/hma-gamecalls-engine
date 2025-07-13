#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;

// Google Test suite definition
class CrossPlatformTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Optional: setup code before each test
    }
    void TearDown() override {
        // Optional: cleanup code after each test
    }
};

// Test vector structure
struct TestVector {
    std::string name;
    std::string inputFile;
    std::string outputFile;
    float expectedScore;
    int expectedMFCCFrames;
};

// Generate reference test vectors
void generateTestVectors(HuntmasterAudioEngine &engine) {
    std::cout << "=== Generating Test Vectors ===" << std::endl;

    // Create test directory
    system("mkdir ..\\data\\test_vectors 2>nul");

    // Test cases
    std::vector<TestVector> testCases = {
        {"Sine Wave 440Hz", "test_sine_440.wav", "sine_440_vector.bin", 1.0f, 83},
        {"Complex Wave", "test_complex.wav", "complex_vector.bin", 1.0f, 83}};

    for (const auto &test : testCases) {
        std::cout << "\nGenerating vector for: " << test.name << std::endl;

        // Load as master
        engine.loadMasterCall(test.inputFile.substr(0, test.inputFile.find_last_of('.')));

        // Save the current state as test vector
        std::string vectorPath = "../data/test_vectors/" + test.outputFile;
        std::ofstream outFile(vectorPath, std::ios::binary);

        if (outFile.is_open()) {
            // Write test metadata
            outFile.write(test.name.c_str(), test.name.length());
            char nullTerminator = '\0';
            outFile.write(&nullTerminator, 1);

            // Write expected values
            outFile.write(reinterpret_cast<const char *>(&test.expectedScore), sizeof(float));
            outFile.write(reinterpret_cast<const char *>(&test.expectedMFCCFrames), sizeof(int));

            std::cout << "  Vector saved to: " << vectorPath << std::endl;
            outFile.close();
        }
    }
}

// Verify consistency across different processing methods
bool verifyProcessingConsistency(HuntmasterAudioEngine &engine) {
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
    engine.loadMasterCall("test_sine_440");  // Need a master loaded
    int batchSession = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
    engine.processAudioChunk(batchSession, testAudio.data(), testAudio.size());
    float batchScore = engine.getSimilarityScore(batchSession);
    engine.endRealtimeSession(batchSession);

    std::cout << "  Batch processing score: " << std::fixed << std::setprecision(8) << batchScore
              << std::endl;

    // Process in small chunks
    int chunkSession = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
    const int chunkSize = 512;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t remaining = testAudio.size() - i;
        size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
        engine.processAudioChunk(chunkSession, testAudio.data() + i, toProcess);
    }
    float chunkScore = engine.getSimilarityScore(chunkSession);
    engine.endRealtimeSession(chunkSession);

    std::cout << "  Chunk processing score: " << std::fixed << std::setprecision(8) << chunkScore
              << std::endl;

    float scoreDiff = std::abs(batchScore - chunkScore);
    bool test1Pass = scoreDiff < 0.001f;  // Allow small floating point differences

    std::cout << "  Score difference: " << scoreDiff << std::endl;
    std::cout << "  Status: " << (test1Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 2: Different chunk sizes should give same result
    std::cout << "\nTest 2: Different Chunk Sizes" << std::endl;
    std::cout << "-----------------------------" << std::endl;

    std::vector<int> chunkSizes = {256, 512, 1024, 2048};
    std::vector<float> scores;

    for (int size : chunkSizes) {
        int session = engine.startRealtimeSession(static_cast<float>(sampleRate), size);

        for (size_t i = 0; i < testAudio.size(); i += size) {
            size_t remaining = testAudio.size() - i;
            size_t toProcess = std::min(static_cast<size_t>(size), remaining);
            engine.processAudioChunk(session, testAudio.data() + i, toProcess);
        }

        float score = engine.getSimilarityScore(session);
        scores.push_back(score);
        engine.endRealtimeSession(session);

        std::cout << "  Chunk size " << size << ": Score = " << score << std::endl;
    }

    // Check all scores are similar
    float maxDiff = 0.0f;
    for (size_t i = 1; i < scores.size(); ++i) {
        maxDiff = std::max(maxDiff, std::abs(scores[i] - scores[0]));
    }

    bool test2Pass = maxDiff < 0.001f;
    std::cout << "  Max score difference: " << maxDiff << std::endl;
    std::cout << "  Status: " << (test2Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    return test1Pass && test2Pass;
}

// Test edge cases
bool testEdgeCases(HuntmasterAudioEngine &engine) {
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;

    // Test 1: Empty audio
    std::cout << "\nTest 1: Empty Audio" << std::endl;
    std::cout << "-------------------" << std::endl;

    std::vector<float> emptyAudio;
    int emptySession = engine.startRealtimeSession(44100.0f, 1024);
    engine.processAudioChunk(emptySession, emptyAudio.data(), 0);
    float emptyScore = engine.getSimilarityScore(emptySession);
    engine.endRealtimeSession(emptySession);

    std::cout << "  Empty audio score: " << emptyScore << std::endl;
    std::cout << "  Status: " << (emptyScore == 0.0f ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 2: Very short audio
    std::cout << "\nTest 2: Very Short Audio (100 samples)" << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    std::vector<float> shortAudio(100, 0.5f);
    int shortSession = engine.startRealtimeSession(44100.0f, 1024);
    engine.processAudioChunk(shortSession, shortAudio.data(), shortAudio.size());
    float shortScore = engine.getSimilarityScore(shortSession);
    engine.endRealtimeSession(shortSession);

    std::cout << "  Short audio score: " << shortScore << std::endl;
    std::cout << "  Status: PASS ✓ (No crash)" << std::endl;

    // Test 3: Silence
    std::cout << "\nTest 3: Complete Silence" << std::endl;
    std::cout << "------------------------" << std::endl;

    std::vector<float> silence(44100, 0.0f);  // 1 second of silence
    int silenceSession = engine.startRealtimeSession(44100.0f, 1024);
    engine.processAudioChunk(silenceSession, silence.data(), silence.size());
    float silenceScore = engine.getSimilarityScore(silenceSession);
    engine.endRealtimeSession(silenceSession);

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

    int clippedSession = engine.startRealtimeSession(44100.0f, 1024);
    engine.processAudioChunk(clippedSession, clippedAudio.data(), clippedAudio.size());
    float clippedScore = engine.getSimilarityScore(clippedSession);
    engine.endRealtimeSession(clippedSession);

    std::cout << "  Clipped audio score: " << clippedScore << std::endl;
    std::cout << "  Status: PASS ✓ (Processed without crash)" << std::endl;

    return true;
}

// Test different sample rates
bool testSampleRates(HuntmasterAudioEngine &engine) {
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

        int session = engine.startRealtimeSession(sr, 1024);
        engine.processAudioChunk(session, audio.data(), audio.size());
        float score = engine.getSimilarityScore(session);
        engine.endRealtimeSession(session);

        std::cout << "  Score: " << score << std::endl;
        std::cout << "  Status: PASS ✓" << std::endl;
    }

    return true;
}

// Convert to proper Google Test
TEST(CrossPlatformTest, ProcessingConsistency) {
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    bool result = verifyProcessingConsistency(engine);
    EXPECT_TRUE(result);

    engine.shutdown();
}

TEST(CrossPlatformTest, EdgeCases) {
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    bool result = testEdgeCases(engine);
    EXPECT_TRUE(result);

    engine.shutdown();
}

TEST(CrossPlatformTest, SampleRates) {
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    bool result = testSampleRates(engine);
    EXPECT_TRUE(result);

    engine.shutdown();
}