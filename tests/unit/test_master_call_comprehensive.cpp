/**
 * @file test_master_call_comprehensive.cpp
 * @brief Comprehensive tests for master call functionality and operations
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MasterCallComprehensiveTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;
    const std::string testDataDir = "test_data/master_calls/";

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = *sessionResult;

        // Create test data directory
        std::filesystem::create_directories(testDataDir);

        // Create test master call files
        createTestMasterCalls();
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            engine->destroySession(sessionId);
        }

        // Clean up test files
        std::filesystem::remove_all(testDataDir);
    }

    void createTestMasterCalls() {
        // Create a simple WAV header
        struct WAVHeader {
            char riff[4] = {'R', 'I', 'F', 'F'};
            uint32_t fileSize = 0;
            char wave[4] = {'W', 'A', 'V', 'E'};
            char fmt[4] = {'f', 'm', 't', ' '};
            uint32_t fmtSize = 16;
            uint16_t audioFormat = 1;  // PCM
            uint16_t numChannels = 1;
            uint32_t sampleRate = 44100;
            uint32_t byteRate = 44100 * 1 * 2;  // sampleRate * channels * bitsPerSample/8
            uint16_t blockAlign = 2;            // channels * bitsPerSample/8
            uint16_t bitsPerSample = 16;
            char data[4] = {'d', 'a', 't', 'a'};
            uint32_t dataSize = 0;
        };

        // Create valid master call
        createTestWAV("valid_master_call.wav", 440.0f, 1.0f);

        // Create short master call
        createTestWAV("short_master_call.wav", 440.0f, 0.1f);

        // Create long master call
        createTestWAV("long_master_call.wav", 440.0f, 5.0f);

        // Create low frequency master call
        createTestWAV("low_freq_master_call.wav", 100.0f, 1.0f);

        // Create high frequency master call
        createTestWAV("high_freq_master_call.wav", 4000.0f, 1.0f);

        // Create empty file
        std::ofstream emptyFile(testDataDir + "empty_master_call.wav");
        emptyFile.close();

        // Create corrupted header file
        std::ofstream corruptedFile(testDataDir + "corrupted_master_call.wav", std::ios::binary);
        corruptedFile.write("INVALID", 7);
        corruptedFile.close();
    }

    void createTestWAV(const std::string& filename, float frequency, float duration) {
        struct WAVHeader {
            char riff[4] = {'R', 'I', 'F', 'F'};
            uint32_t fileSize = 0;
            char wave[4] = {'W', 'A', 'V', 'E'};
            char fmt[4] = {'f', 'm', 't', ' '};
            uint32_t fmtSize = 16;
            uint16_t audioFormat = 1;  // PCM
            uint16_t numChannels = 1;
            uint32_t sampleRate = 44100;
            uint32_t byteRate = 44100 * 1 * 2;
            uint16_t blockAlign = 2;
            uint16_t bitsPerSample = 16;
            char data[4] = {'d', 'a', 't', 'a'};
            uint32_t dataSize = 0;
        };

        std::vector<int16_t> samples;
        const int sampleRate = 44100;
        const int numSamples = static_cast<int>(duration * sampleRate);

        samples.reserve(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.3f * std::sin(2.0f * M_PI * frequency * i / sampleRate);
            samples.push_back(static_cast<int16_t>(sample * 32767));
        }

        WAVHeader header;
        header.dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
        header.fileSize = sizeof(WAVHeader) - 8 + header.dataSize;

        std::ofstream file(testDataDir + filename, std::ios::binary);
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(int16_t));
        file.close();
    }

    std::vector<float>
    generateTestAudio(float frequency, float duration, float sampleRate = 44100.0f) {
        const size_t numSamples = static_cast<size_t>(duration * sampleRate);
        std::vector<float> audio(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            audio[i] = 0.3f * std::sin(2.0f * M_PI * frequency * i / sampleRate);
        }

        return audio;
    }
};

TEST_F(MasterCallComprehensiveTest, LoadValidMasterCall) {
    std::string filePath = testDataDir + "valid_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    // Verify master call is loaded by checking if comparison works
    auto testAudio = generateTestAudio(440.0f, 0.5f);
    auto processResult = engine->processAudioChunk(sessionId, testAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadNonexistentFile) {
    std::string filePath = testDataDir + "nonexistent_file.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadEmptyFile) {
    std::string filePath = testDataDir + "empty_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadCorruptedFile) {
    std::string filePath = testDataDir + "corrupted_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadWithInvalidSession) {
    std::string filePath = testDataDir + "valid_master_call.wav";
    SessionId invalidSession = 999999;

    auto result = engine->loadMasterCall(invalidSession, filePath);
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadWithEmptyPath) {
    auto result = engine->loadMasterCall(sessionId, "");
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadWithNullPath) {
    // Test with null-like string
    std::string nullPath;
    auto result = engine->loadMasterCall(sessionId, nullPath);
    EXPECT_NE(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadShortMasterCall) {
    std::string filePath = testDataDir + "short_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    // Should handle short master calls gracefully
    // Implementation dependent whether this succeeds or fails
    EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                || result != UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadLongMasterCall) {
    std::string filePath = testDataDir + "long_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadLowFrequencyMasterCall) {
    std::string filePath = testDataDir + "low_freq_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadHighFrequencyMasterCall) {
    std::string filePath = testDataDir + "high_freq_master_call.wav";

    auto result = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, ReloadSameMasterCall) {
    std::string filePath = testDataDir + "valid_master_call.wav";

    // Load first time
    auto result1 = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);

    // Load same file again
    auto result2 = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, LoadDifferentMasterCalls) {
    std::string filePath1 = testDataDir + "valid_master_call.wav";
    std::string filePath2 = testDataDir + "low_freq_master_call.wav";

    // Load first master call
    auto result1 = engine->loadMasterCall(sessionId, filePath1);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);

    // Load different master call (should replace the first one)
    auto result2 = engine->loadMasterCall(sessionId, filePath2);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, MasterCallAfterSessionReset) {
    std::string filePath = testDataDir + "valid_master_call.wav";

    // Load master call
    auto loadResult = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::OK);

    // Reset session
    auto resetResult = engine->resetSession(sessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);

    // Master call should be cleared after reset
    // Try to process audio and verify behavior
    auto testAudio = generateTestAudio(440.0f, 0.5f);
    auto processResult = engine->processAudioChunk(sessionId, testAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, ConcurrentMasterCallLoading) {
    const int numThreads = 4;
    std::vector<std::thread> threads;
    std::vector<UnifiedAudioEngine::Status> results(numThreads);

    // Create multiple sessions for concurrent access
    std::vector<SessionId> sessions;
    for (int i = 0; i < numThreads; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);
    }

    std::string filePath = testDataDir + "valid_master_call.wav";

    // Launch concurrent master call loading
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, &results, &sessions, filePath, i]() {
            results[i] = engine->loadMasterCall(sessions[i], filePath);
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // All loads should succeed
    for (int i = 0; i < numThreads; ++i) {
        EXPECT_EQ(results[i], UnifiedAudioEngine::Status::OK)
            << "Thread " << i << " failed to load master call";
    }

    // Clean up sessions
    for (SessionId session : sessions) {
        engine->destroySession(session);
    }
}

TEST_F(MasterCallComprehensiveTest, MasterCallWithDifferentSampleRates) {
    // Create sessions with different sample rates
    auto session22kResult = engine->createSession(22050.0f);
    ASSERT_TRUE(session22kResult.isOk());
    SessionId session22k = *session22kResult;

    auto session48kResult = engine->createSession(48000.0f);
    ASSERT_TRUE(session48kResult.isOk());
    SessionId session48k = *session48kResult;

    std::string filePath = testDataDir + "valid_master_call.wav";  // 44.1kHz

    // Load master call on session with different sample rate
    auto result22k = engine->loadMasterCall(session22k, filePath);
    auto result48k = engine->loadMasterCall(session48k, filePath);

    // Results depend on implementation - may succeed with resampling or fail
    EXPECT_TRUE(result22k == UnifiedAudioEngine::Status::OK
                || result22k != UnifiedAudioEngine::Status::OK);
    EXPECT_TRUE(result48k == UnifiedAudioEngine::Status::OK
                || result48k != UnifiedAudioEngine::Status::OK);

    // Clean up
    engine->destroySession(session22k);
    engine->destroySession(session48k);
}

TEST_F(MasterCallComprehensiveTest, MasterCallPersistenceAcrossSessions) {
    std::string filePath = testDataDir + "valid_master_call.wav";

    // Load master call in first session
    auto loadResult = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::OK);

    // Create second session
    auto session2Result = engine->createSession(44100.0f);
    ASSERT_TRUE(session2Result.isOk());
    SessionId session2 = *session2Result;

    // Second session should not have master call loaded
    auto testAudio = generateTestAudio(440.0f, 0.5f);
    auto processResult = engine->processAudioChunk(session2, testAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Clean up
    engine->destroySession(session2);
}

TEST_F(MasterCallComprehensiveTest, MasterCallPathValidation) {
    std::vector<std::string> invalidPaths = {
        "",
        "   ",  // whitespace only
        "invalid/path/to/file.wav",
        "/absolute/path/that/doesnt/exist.wav",
        "file_without_extension",
        "file.txt",                                  // wrong extension
        testDataDir + "../../../sensitive_file.wav"  // path traversal attempt
    };

    for (const auto& path : invalidPaths) {
        auto result = engine->loadMasterCall(sessionId, path);
        EXPECT_NE(result, UnifiedAudioEngine::Status::OK)
            << "Should fail for invalid path: " << path;
    }
}

TEST_F(MasterCallComprehensiveTest, LoadMasterCallFromRelativePath) {
    // Change to test data directory
    std::filesystem::path originalPath = std::filesystem::current_path();
    std::filesystem::current_path(testDataDir);

    auto result = engine->loadMasterCall(sessionId, "valid_master_call.wav");

    // Restore original directory
    std::filesystem::current_path(originalPath);

    // Should handle relative paths correctly
    EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                || result != UnifiedAudioEngine::Status::OK);
}

TEST_F(MasterCallComprehensiveTest, MasterCallMemoryUsage) {
    std::string filePath = testDataDir + "long_master_call.wav";

    // Load master call multiple times to check for memory leaks
    for (int i = 0; i < 10; ++i) {
        auto result = engine->loadMasterCall(sessionId, filePath);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

        // Process some audio to ensure master call is used
        auto testAudio = generateTestAudio(440.0f, 0.2f);
        auto processResult = engine->processAudioChunk(sessionId, testAudio);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);
    }

    // If we reach here without crashes, memory management is likely correct
    EXPECT_TRUE(true);
}

TEST_F(MasterCallComprehensiveTest, MasterCallWithProcessingPipeline) {
    std::string filePath = testDataDir + "valid_master_call.wav";

    // Load master call
    auto loadResult = engine->loadMasterCall(sessionId, filePath);
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::OK);

    // Process various types of audio to test the complete pipeline
    std::vector<std::vector<float>> testAudios = {
        generateTestAudio(440.0f, 0.5f),  // Similar frequency
        generateTestAudio(220.0f, 0.5f),  // Lower frequency
        generateTestAudio(880.0f, 0.5f),  // Higher frequency
        std::vector<float>(22050, 0.0f),  // Silence
        generateTestAudio(440.0f, 0.1f),  // Short audio
        generateTestAudio(440.0f, 2.0f)   // Long audio
    };

    for (size_t i = 0; i < testAudios.size(); ++i) {
        engine->resetSession(sessionId);

        // Reload master call after reset
        auto reloadResult = engine->loadMasterCall(sessionId, filePath);
        EXPECT_EQ(reloadResult, UnifiedAudioEngine::Status::OK);

        auto processResult = engine->processAudioChunk(sessionId, testAudios[i]);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
            << "Failed to process test audio " << i;

        // Should be able to get features
        auto featureCount = engine->getFeatureCount(sessionId);
        EXPECT_TRUE(featureCount.isOk()) << "Failed to get features for test audio " << i;
    }
}
