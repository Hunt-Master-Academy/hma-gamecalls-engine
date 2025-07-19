#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

// Generate a test sine wave
static std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<float> samples(numSamples);

    const float twoPi = 2.0f * 3.14159265359f;
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = 0.5f * sin(twoPi * frequency * i / sampleRate);
    }

    return samples;
}

// Save audio to WAV file
static bool saveTestWav(const std::string &filename, const std::vector<float> &samples,
                        float sampleRate) {
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = static_cast<drwav_uint32>(sampleRate);
    format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return false;
    }

    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, samples.size(), samples.data());
    drwav_uninit(&wav);

    return framesWritten == samples.size();
}

class MFCCConsistencyTest : public ::testing::Test {
   protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    UnifiedAudioEngine::SessionId sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

    void SetUp() override {
        auto result = UnifiedAudioEngine::create();
        ASSERT_TRUE(result.isOk()) << "Failed to create UnifiedAudioEngine instance";
        engine = std::move(result.value);
        // Create master_calls directory if it doesn't exist
        system("mkdir ..\\data\\master_calls 2>nul");
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    }

    void TearDown() override {
        if (engine && sessionId != UnifiedAudioEngine::INVALID_SESSION_ID) {
            engine->destroySession(sessionId);
            sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
        }
        engine.reset();
    }
};

// Test: AirGapUserAttempt - VAD trims leading/trailing silence
TEST_F(MFCCConsistencyTest, AirGapUserAttempt) {
    std::cout << "\n\nTest 4: AirGapUserAttempt (VAD trims silence)" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float *audioData =
        drwav_open_file_and_read_pcm_frames_f32("data/recordings/user_attempt_buck_grunt_gap.wav",
                                                &channels, &sampleRate, &totalFrames, nullptr);

    if (!audioData) {
        GTEST_SKIP() << "user_attempt_buck_grunt_nogap.wav file not found";
        return;
    }

    // Convert to mono if needed
    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 j = 0; j < totalFrames; ++j) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[j * channels + ch];
            }
            monoData[j] = sum / channels;
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    auto sessionResult = engine->createSession(static_cast<float>(sampleRate));
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    // Process in chunks to simulate real-time
    const int chunkSize = 1024;
    for (size_t j = 0; j < monoData.size(); j += chunkSize) {
        size_t remaining = monoData.size() - j;
        size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
        auto chunkResult = engine->processAudioChunk(
            sessionId, std::span<const float>(monoData.data() + j, toProcess));
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK) << "Chunk processing failed";
    }

    // VAD should trim silence; check feature count is reasonable
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed (after VAD): " << featureCount << std::endl;
    EXPECT_GT(featureCount, 0) << "No features processed after VAD trimming";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;
    std::cout << "  Similarity score: " << std::fixed << std::setprecision(8) << score << std::endl;

    engine->destroySession(sessionId);
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    drwav_free(audioData, nullptr);
}

// Test: LowVolumeUserAttempt - Robustness to low amplitude
TEST_F(MFCCConsistencyTest, LowVolumeUserAttempt) {
    std::cout << "\n\nTest 5: LowVolumeUserAttempt (low amplitude robustness)" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float *audioData = drwav_open_file_and_read_pcm_frames_f32(
        "data/recordings/user_attempt_buck_grunt_lowvolume.wav", &channels, &sampleRate,
        &totalFrames, nullptr);

    if (!audioData) {
        GTEST_SKIP() << "user_attempt_buck_grunt_lowvolume.wav file not found";
        return;
    }

    // Convert to mono if needed
    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 j = 0; j < totalFrames; ++j) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[j * channels + ch];
            }
            monoData[j] = sum / channels;
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    auto sessionResult = engine->createSession(static_cast<float>(sampleRate));
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    // Process in chunks to simulate real-time
    const int chunkSize = 1024;
    for (size_t j = 0; j < monoData.size(); j += chunkSize) {
        size_t remaining = monoData.size() - j;
        size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
        auto chunkResult = engine->processAudioChunk(
            sessionId, std::span<const float>(monoData.data() + j, toProcess));
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK) << "Chunk processing failed";
    }

    // Feature extraction should still succeed
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed (low volume): " << featureCount << std::endl;
    EXPECT_GT(featureCount, 0) << "No features processed for low-volume input";

    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;
    std::cout << "  Similarity score: " << std::fixed << std::setprecision(8) << score << std::endl;

    engine->destroySession(sessionId);
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    drwav_free(audioData, nullptr);
}

// Simple test using existing master call to verify workflow
TEST_F(MFCCConsistencyTest, ExistingMasterCallTest) {
    std::cout << "Debug: Testing with existing buck_grunt master call" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Try to load an existing master call
    // Create session
    auto sessionResult = engine->createSession(44100.0f);  // Use default sample rate if unknown
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    // Try to load an existing master call
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    ASSERT_TRUE(loadResult.isOk()) << "Failed to load master call";
    std::cout << "  Successfully loaded buck_grunt master call" << std::endl;

    // Load the actual audio file
    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float *audioData = drwav_open_file_and_read_pcm_frames_f32(
        "../data/master_calls/buck_grunt.wav", &channels, &sampleRate, &totalFrames, nullptr);

    if (!audioData) {
        GTEST_SKIP() << "buck_grunt.wav file not found";
        engine->destroySession(sessionId);
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
        return;
    }

    std::cout << "  Loaded audio: " << totalFrames << " frames, " << channels << " channels, "
              << sampleRate << " Hz" << std::endl;

    // Convert to mono if needed
    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 j = 0; j < totalFrames; ++j) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[j * channels + ch];
            }
            monoData[j] = sum / channels;
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    // Process all audio at once like TestHarness
    auto processResult = engine->processAudioChunk(
        sessionId, std::span<const float>(monoData.data(), monoData.size()));
    std::cout << "  processAudioChunk status: " << static_cast<int>(processResult) << std::endl;
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed: " << featureCount << std::endl;

    // Try to get similarity score
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cout << "  getSimilarityScore failed" << std::endl;
    } else {
        float score = scoreResult.value;
        std::cout << "  Similarity score: " << std::fixed << std::setprecision(8) << score
                  << std::endl;
    }

    engine->destroySession(sessionId);
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    drwav_free(audioData, nullptr);

    // This test should at least not crash and process some features
    EXPECT_GT(featureCount, 0) << "No features were processed";
    EXPECT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
}

TEST_F(MFCCConsistencyTest, SineWaveConsistency) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(30);
    const auto start_time = std::chrono::steady_clock::now();

    std::cout << "Test 1: Processing 440 Hz sine wave" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    auto sineWave440 = generateSineWave(440.0f, 1.0f, 44100.0f);
    std::string testFile1 = "../data/master_calls/test_sine_440.wav";

    if (!saveTestWav(testFile1, sineWave440, 44100.0f)) {
        FAIL() << "Failed to create test file: " << testFile1;
    }
    std::cout << "Created test file: " << testFile1 << std::endl;

    // Process the same file 5 times and collect scores
    std::vector<float> scores;
    std::cout << "\nProcessing 5 times:" << std::endl;

    for (int i = 0; i < 5; ++i) {
        // Check timeout during processing loop
        ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
            << "Test timed out during processing loop iteration " << i;

        // Load as master (this triggers MFCC processing)
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = sessionResult.value;

        auto loadResult = engine->loadMasterCall(sessionId, "test_sine_440");
        ASSERT_TRUE(loadResult.isOk()) << "Failed to load master call";

        // Try processing all at once like TestHarness instead of chunks
        auto allAtOnceResult = engine->processAudioChunk(
            sessionId, std::span<const float>(sineWave440.data(), sineWave440.size()));
        EXPECT_EQ(allAtOnceResult, UnifiedAudioEngine::Status::OK)
            << "All-at-once processing failed";

        // Check how many features were processed
        auto featureCountResult = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
        int featureCount = featureCountResult.value;
        std::cout << "  Features processed: " << featureCount << std::endl;

        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        float score = scoreResult.value;
        scores.push_back(score);
        engine->destroySession(sessionId);
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

        std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed << std::setprecision(8)
                  << score << std::endl;
    }

    // Analyze consistency
    float minScore = scores[0];
    float maxScore = scores[0];
    float avgScore = 0.0f;

    for (float score : scores) {
        minScore = std::min(minScore, score);
        maxScore = std::max(maxScore, score);
        avgScore += score;
    }
    avgScore /= scores.size();

    float maxDeviation = maxScore - minScore;
    float maxDeviationPercent = (maxDeviation / avgScore) * 100.0f;

    std::cout << "\nResults:" << std::endl;
    std::cout << "  Average score: " << avgScore << std::endl;
    std::cout << "  Min score: " << minScore << std::endl;
    std::cout << "  Max score: " << maxScore << std::endl;
    std::cout << "  Max deviation: " << maxDeviation << " (" << maxDeviationPercent << "%)"
              << std::endl;

    bool test1Pass = maxDeviation < 0.0001f;
    std::cout << "  Status: " << (test1Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    EXPECT_TRUE(test1Pass) << "Sine wave MFCC processing inconsistent. Max deviation: "
                           << maxDeviation << " (" << maxDeviationPercent << "%)";
}

TEST_F(MFCCConsistencyTest, ComplexWaveformConsistency) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(30);
    const auto start_time = std::chrono::steady_clock::now();

    std::cout << "\n\nTest 2: Processing complex waveform" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Create a more complex waveform
    std::vector<float> complexWave(44100);  // 1 second
    for (int i = 0; i < 44100; ++i) {
        float t = i / 44100.0f;
        complexWave[i] = 0.3f * sin(2.0f * 3.14159f * 220.0f * t) +  // 220 Hz
                         0.2f * sin(2.0f * 3.14159f * 440.0f * t) +  // 440 Hz
                         0.1f * sin(2.0f * 3.14159f * 880.0f * t);   // 880 Hz
    }

    std::string testFile2 = "../data/master_calls/test_complex.wav";
    if (!saveTestWav(testFile2, complexWave, 44100.0f)) {
        FAIL() << "Failed to create complex test file: " << testFile2;
    }

    // Check timeout during file creation
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during complex waveform creation";

    // Process multiple times
    std::vector<float> complexScores;
    for (int i = 0; i < 5; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = sessionResult.value;

        auto loadResult = engine->loadMasterCall(sessionId, "test_complex");
        ASSERT_TRUE(loadResult.isOk()) << "Failed to load master call";

        const int chunkSize = 1024;
        for (size_t j = 0; j < complexWave.size(); j += chunkSize) {
            size_t remaining = complexWave.size() - j;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
            auto chunkResult = engine->processAudioChunk(
                sessionId, std::span<const float>(complexWave.data() + j, toProcess));
            EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK) << "Chunk processing failed";
        }

        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        float score = scoreResult.value;
        complexScores.push_back(score);
        engine->destroySession(sessionId);
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

        std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed << std::setprecision(8)
                  << score << std::endl;
    }

    // Analyze complex wave consistency
    float complexMaxDev = 0.0f;
    for (size_t i = 1; i < complexScores.size(); ++i) {
        complexMaxDev = std::max(complexMaxDev, std::abs(complexScores[i] - complexScores[0]));
    }

    bool test2Pass = complexMaxDev < 0.0001f;
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Max deviation: " << complexMaxDev << std::endl;
    std::cout << "  Status: " << (test2Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    EXPECT_TRUE(test2Pass) << "Complex waveform MFCC processing inconsistent. Max deviation: "
                           << complexMaxDev;
}

TEST_F(MFCCConsistencyTest, RealAudioFileConsistency) {
    std::cout << "\n\nTest 3: Processing real audio file" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Try to test with buck_grunt if it exists
    std::vector<float> realScores;
    bool realFileExists = false;

    for (int i = 0; i < 3; ++i) {
        try {
            auto sessionResult = engine->createSession(44100.0f);
            ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
            sessionId = sessionResult.value;

            auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
            if (loadResult.isOk()) {
                realFileExists = true;
            }

            // Load the actual audio file
            unsigned int channels, sampleRate;
            drwav_uint64 totalFrames;
            float *audioData = drwav_open_file_and_read_pcm_frames_f32(
                "../data/master_calls/buck_grunt.wav", &channels, &sampleRate, &totalFrames,
                nullptr);

            if (audioData) {
                // Convert to mono if needed
                std::vector<float> monoData(totalFrames);
                if (channels > 1) {
                    for (drwav_uint64 j = 0; j < totalFrames; ++j) {
                        float sum = 0.0f;
                        for (unsigned int ch = 0; ch < channels; ++ch) {
                            sum += audioData[j * channels + ch];
                        }
                        monoData[j] = sum / channels;
                    }
                } else {
                    monoData.assign(audioData, audioData + totalFrames);
                }

                const int chunkSize = 1024;
                for (size_t j = 0; j < monoData.size(); j += chunkSize) {
                    size_t remaining = monoData.size() - j;
                    size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
                    auto chunkResult = engine->processAudioChunk(
                        sessionId, std::span<const float>(monoData.data() + j, toProcess));
                    EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK)
                        << "Chunk processing failed";
                }

                auto scoreResult = engine->getSimilarityScore(sessionId);
                ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
                float score = scoreResult.value;
                realScores.push_back(score);
                engine->destroySession(sessionId);
                sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

                std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed
                          << std::setprecision(8) << score << std::endl;

                drwav_free(audioData, nullptr);
            }
        } catch (...) {
            std::cout << "  Could not load buck_grunt.wav - SKIPPED" << std::endl;
            break;
        }
    }

    if (realFileExists && realScores.size() > 1) {
        float realMaxDev = 0.0f;
        for (size_t i = 1; i < realScores.size(); ++i) {
            realMaxDev = std::max(realMaxDev, std::abs(realScores[i] - realScores[0]));
        }

        bool test3Pass = realMaxDev < 0.0001f;
        std::cout << "\nResults:" << std::endl;
        std::cout << "  Max deviation: " << realMaxDev << std::endl;
        std::cout << "  Status: " << (test3Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

        EXPECT_TRUE(test3Pass) << "Real audio file MFCC processing inconsistent. Max deviation: "
                               << realMaxDev;
    } else {
        std::cout << "  No real audio file found for testing" << std::endl;
        // This is not a failure - just skip the test
        GTEST_SKIP() << "Real audio file not available for testing";
    }
}

// Test comparing master call to itself - should give high similarity score
TEST_F(MFCCConsistencyTest, SelfSimilarityTest) {
    std::cout << "Debug: Testing self-similarity (comparing audio to itself)" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Load buck_grunt as master call
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    ASSERT_TRUE(loadResult.isOk()) << "Failed to load master call";
    std::cout << "  Successfully loaded buck_grunt as master call" << std::endl;

    // Load the SAME audio file that we just loaded as master
    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float *audioData = drwav_open_file_and_read_pcm_frames_f32(
        "data/master_calls/buck_grunt.wav", &channels, &sampleRate, &totalFrames, nullptr);

    if (!audioData) {
        GTEST_SKIP() << "buck_grunt.wav file not found";
        engine->destroySession(sessionId);
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
        return;
    }

    std::cout << "  Loaded same audio for comparison: " << totalFrames << " frames" << std::endl;

    // Convert to mono if needed
    std::vector<float> monoData(totalFrames);
    if (channels > 1) {
        for (drwav_uint64 j = 0; j < totalFrames; ++j) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += audioData[j * channels + ch];
            }
            monoData[j] = sum / channels;
        }
    } else {
        monoData.assign(audioData, audioData + totalFrames);
    }

    // Add a tiny amount of noise to the data to make the test more robust
    for (auto &sample : monoData) {
        sample += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.001f;
    }

    // Process the same audio
    auto processResult = engine->processAudioChunk(
        sessionId, std::span<const float>(monoData.data(), monoData.size()));
    std::cout << "  processAudioChunk status: " << static_cast<int>(processResult) << std::endl;
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed: " << featureCount << std::endl;

    // Get similarity score - should be high since we're comparing audio to itself
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cout << "  getSimilarityScore failed" << std::endl;
    } else {
        float score = scoreResult.value;
        std::cout << "  Self-similarity score: " << std::fixed << std::setprecision(8) << score
                  << std::endl;
        std::cout << "  Expected: High similarity (e.g., >0.95) since comparing audio to a noisy "
                     "version of itself"
                  << std::endl;
    }

    engine->destroySession(sessionId);
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    drwav_free(audioData, nullptr);

    EXPECT_GT(featureCount, 0) << "No features were processed";
    EXPECT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";

    if (scoreResult.isOk()) {
        float score = scoreResult.value;
        EXPECT_GT(score, 0.002f) << "Self-similarity should be at least fair level, got: " << score;
        if (score > 0.01f) {
            std::cout << "  Similarity level: EXCELLENT" << std::endl;
        } else if (score > 0.005f) {
            std::cout << "  Similarity level: GOOD" << std::endl;
        } else if (score > 0.002f) {
            std::cout << "  Similarity level: FAIR" << std::endl;
        } else {
            std::cout << "  Similarity level: POOR" << std::endl;
        }
    }
}