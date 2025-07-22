/**
 * @file dtw_tests_unified.cpp
 * @brief DTW similarity tests using the UnifiedAudioEngine API
 *
 * This test suite validates DTW (Dynamic Time Warping) similarity calculations
 * using the new UnifiedEngine session-based architecture.
 */

#include <iostream>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class DTWUnifiedTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create engine instance using the new UnifiedEngine API
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk())
            << "Failed to create UnifiedAudioEngine: " << static_cast<int>(engineResult.error());
        engine = std::move(*engineResult);
    }

    void TearDown() override {
        // Clean up any remaining sessions
        auto activeSessions = engine->getActiveSessions();
        for (auto sessionId : activeSessions) {
            auto destroyResult = engine->destroySession(sessionId);
            (void)destroyResult;
        }
        engine.reset();
    }

    // Helper function to load audio file
    std::vector<float>
    loadAudioFile(const std::string& filePath, unsigned int& channels, unsigned int& sampleRate) {
        drwav_uint64 totalPCMFrameCount = 0;
        float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
            filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

        if (pSampleData == nullptr) {
            std::cerr << "Error: Could not load audio file for test: " << filePath << std::endl;
            return {};
        }

        // Convert to mono
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

    std::unique_ptr<UnifiedAudioEngine> engine;
};

TEST_F(DTWUnifiedTest, BasicDTWFunctionality) {
    std::cout << "=== Basic DTW Functionality Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Try to load a master call
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        std::cout << "  loadMasterCall failed with status: " << static_cast<int>(loadResult)
                  << std::endl;
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt master call not available";
        return;
    }
    std::cout << "  Successfully loaded buck_grunt master call" << std::endl;

    // Load the audio file for comparison
    unsigned int channels, sampleRate;
    auto audioData = loadAudioFile("../data/master_calls/buck_grunt.wav", channels, sampleRate);

    if (audioData.empty()) {
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt.wav file not found";
        return;
    }

    std::cout << "  Loaded audio: " << audioData.size() << " samples, " << channels << " channels, "
              << sampleRate << " Hz" << std::endl;

    // Process the audio using the new span-based API
    auto processResult = engine->processAudioChunk(sessionId, std::span<const float>(audioData));
    std::cout << "  processAudioChunk status: " << static_cast<int>(processResult) << std::endl;
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed: " << featureCount << std::endl;

    // Get similarity score
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (!scoreResult.isOk()) {
        std::cout << "  getSimilarityScore failed with status: "
                  << static_cast<int>(scoreResult.status) << std::endl;
    } else {
        float score = scoreResult.value;
        std::cout << "  DTW similarity score: " << std::fixed << std::setprecision(8) << score
                  << std::endl;
    }

    // Clean up
    auto destroyResult = engine->destroySession(sessionId);
    (void)destroyResult;

    // Validate results
    EXPECT_GT(featureCount, 0) << "No features were processed";
    EXPECT_TRUE(scoreResult.isOk())
        << "Failed to get similarity score, status: " << static_cast<int>(scoreResult.status);
}

TEST_F(DTWUnifiedTest, DTWWithChunkedProcessing) {
    std::cout << "\n=== DTW with Chunked Processing Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Try to load a master call
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt master call not available";
        return;
    }

    // Load the audio file for comparison
    unsigned int channels, sampleRate;
    auto audioData = loadAudioFile("../data/master_calls/buck_grunt.wav", channels, sampleRate);

    if (audioData.empty()) {
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt.wav file not found";
        return;
    }

    std::cout << "  Processing " << audioData.size() << " samples in chunks" << std::endl;

    // Process audio in chunks to simulate real-time processing
    const size_t chunkSize = 1024;
    size_t chunksProcessed = 0;

    for (size_t i = 0; i < audioData.size(); i += chunkSize) {
        size_t remaining = audioData.size() - i;
        size_t toProcess = std::min(chunkSize, remaining);

        std::span<const float> chunk(audioData.data() + i, toProcess);
        auto chunkResult = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK)
            << "Chunk " << chunksProcessed << " processing failed";

        chunksProcessed++;
    }

    std::cout << "  Processed " << chunksProcessed << " chunks" << std::endl;

    // Check feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk()) << "Failed to get feature count";
    int featureCount = featureCountResult.value;
    std::cout << "  Features processed: " << featureCount << std::endl;

    // Get similarity score
    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;
    std::cout << "  DTW similarity score: " << std::fixed << std::setprecision(8) << score
              << std::endl;

    // Clean up
    auto destroyResult = engine->destroySession(sessionId);
    (void)destroyResult;

    // Validate results
    EXPECT_GT(featureCount, 0) << "No features were processed";
    EXPECT_GT(chunksProcessed, 0) << "No chunks were processed";
    EXPECT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
}

TEST_F(DTWUnifiedTest, DTWConsistencyTest) {
    std::cout << "\n=== DTW Consistency Test ===" << std::endl;

    // Process the same audio multiple times to check consistency
    std::vector<float> scores;
    constexpr int numRuns = 3;

    for (int run = 0; run < numRuns; ++run) {
        std::cout << "  Run " << (run + 1) << "/" << numRuns << std::endl;

        // Create a session for each run
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk())
            << "Failed to create session: " << static_cast<int>(sessionResult.error());
        SessionId sessionId = sessionResult.value;

        // Load master call
        auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            auto destroyResult = engine->destroySession(sessionId);
            (void)destroyResult;
            GTEST_SKIP() << "buck_grunt master call not available";
            return;
        }

        // Load the audio file
        unsigned int channels, sampleRate;
        auto audioData = loadAudioFile("../data/master_calls/buck_grunt.wav", channels, sampleRate);

        if (audioData.empty()) {
            auto destroyResult = engine->destroySession(sessionId);
            (void)destroyResult;
            GTEST_SKIP() << "buck_grunt.wav file not found";
            return;
        }

        // Process the audio
        auto processResult =
            engine->processAudioChunk(sessionId, std::span<const float>(audioData));
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

        // Get similarity score
        auto scoreResult = engine->getSimilarityScore(sessionId);
        ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
        float score = scoreResult.value;
        scores.push_back(score);

        std::cout << "    Score: " << std::fixed << std::setprecision(8) << score << std::endl;

        // Clean up
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
    }

    // Analyze consistency
    if (scores.size() > 1) {
        float maxDeviation = 0.0f;
        float avgScore = 0.0f;

        for (float score : scores) {
            avgScore += score;
        }
        avgScore /= scores.size();

        for (size_t i = 1; i < scores.size(); ++i) {
            maxDeviation = std::max(maxDeviation, std::abs(scores[i] - scores[0]));
        }

        float maxDeviationPercent = (maxDeviation / avgScore) * 100.0f;

        std::cout << "\nConsistency Analysis:" << std::endl;
        std::cout << "  Average score: " << avgScore << std::endl;
        std::cout << "  Max deviation: " << maxDeviation << " (" << maxDeviationPercent << "%)"
                  << std::endl;

        // DTW should be consistent across runs
        bool isConsistent = maxDeviation < 0.0001f;
        std::cout << "  Status: " << (isConsistent ? "CONSISTENT ✓" : "INCONSISTENT ✗")
                  << std::endl;

        EXPECT_TRUE(isConsistent) << "DTW processing inconsistent. Max deviation: " << maxDeviation
                                  << " (" << maxDeviationPercent << "%)";
    }
}

TEST_F(DTWUnifiedTest, SelfSimilarityTest) {
    std::cout << "\n=== Self-Similarity Test ===" << std::endl;

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk())
        << "Failed to create session: " << static_cast<int>(sessionResult.error());
    SessionId sessionId = sessionResult.value;

    // Load master call
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt master call not available";
        return;
    }

    // Load the SAME audio file that we loaded as master
    unsigned int channels, sampleRate;
    auto audioData = loadAudioFile("../data/master_calls/buck_grunt.wav", channels, sampleRate);

    if (audioData.empty()) {
        auto destroyResult = engine->destroySession(sessionId);
        (void)destroyResult;
        GTEST_SKIP() << "buck_grunt.wav file not found";
        return;
    }

    std::cout << "  Comparing audio to itself (should give high similarity)" << std::endl;

    // Process the same audio we loaded as master
    auto processResult = engine->processAudioChunk(sessionId, std::span<const float>(audioData));
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Processing failed";

    // Get similarity score - should be high for self-similarity
    auto scoreResult = engine->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";
    float score = scoreResult.value;

    std::cout << "  Self-similarity score: " << std::fixed << std::setprecision(8) << score
              << std::endl;

    // Clean up
    auto destroyResult = engine->destroySession(sessionId);
    (void)destroyResult;

    // Self-similarity should be relatively high
    // Based on real-world thresholds: >0.01 = excellent, >0.005 = good, >0.002 = fair
    EXPECT_GT(score, 0.002f) << "Self-similarity should be at least fair level, got: " << score;

    // Provide feedback on similarity level
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
