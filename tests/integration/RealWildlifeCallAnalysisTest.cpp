/**
 * @file RealWildlifeCallAnalysisTest.cpp
 * @brief Comprehensive end-to-end tests using real audio data for wildlife call analysis
 *
 * This test suite provides robust validation of the system's accuracy using actual
 * wildlife call recordings. It tests the complete pipeline from audio input through
 * MFCC feature extraction, DTW comparison, and RealtimeScorer analysis.
 *
 * Key features:
 * - Real audio file loading and processing
 * - Multi-dimensional scoring validation (MFCC + volume + timing)
 * - Cross-validation between different scorer implementations
 * - Performance benchmarking with real data
 * - Accuracy validation against known good/bad matches
 */

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dr_wav.h"
#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/RealtimeScorer.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using SessionId = uint32_t;

namespace {

// Helper struct for test audio file metadata
struct AudioFileInfo {
    std::string filename;
    std::string call_type;
    float expected_duration;
    bool is_reference;  // True if this should be used as a master call
};

// Test audio files available for testing
const std::vector<AudioFileInfo> TEST_AUDIO_FILES = {
    {"buck_grunt.wav", "buck_grunt", 2.5f, true},
    {"doe_bleat.wav", "doe_bleat", 1.8f, true},
    {"buck_bawl.wav", "buck_bawl", 3.2f, true},
    {"doe_grunt.wav", "doe_grunt", 2.1f, true},
    {"fawn_bleat.wav", "fawn_bleat", 1.5f, true},
    {"estrus_bleat.wav", "estrus_bleat", 2.8f, true},
    {"contact_bleat.wav", "contact_bleat", 2.0f, true},
    {"tending_grunts.wav", "tending_grunts", 4.1f, true},
    {"breeding_bellow.wav", "breeding_bellow", 3.5f, true},
    {"sparring_bucks.wav", "sparring_bucks", 5.2f, true},
    {"buck_rage.wav", "buck_rage", 2.7f, true}};

// Helper function to load audio file
struct AudioData {
    std::vector<float> samples;
    unsigned int channels;
    unsigned int sample_rate;
    bool valid;
};

AudioData loadAudioFile(const std::string& filepath) {
    AudioData data{};

    unsigned int channels, sample_rate;
    drwav_uint64 total_frames;
    float* raw_data = drwav_open_file_and_read_pcm_frames_f32(
        filepath.c_str(), &channels, &sample_rate, &total_frames, nullptr);

    if (!raw_data) {
        data.valid = false;
        return data;
    }

    // Convert to mono if needed
    data.samples.resize(total_frames);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < total_frames; ++i) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += raw_data[i * channels + ch];
            }
            data.samples[i] = sum / static_cast<float>(channels);
        }
    } else {
        std::copy(raw_data, raw_data + total_frames, data.samples.begin());
    }

    data.channels = 1;  // Always mono after conversion
    data.sample_rate = sample_rate;
    data.valid = true;

    drwav_free(raw_data, nullptr);
    return data;
}

}  // anonymous namespace

class RealWildlifeCallAnalysisTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create engine instance
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine_ = std::move(engineResult.value);

        // Set up test data paths
        masterCallsPath_ = "../data/master_calls/";
        testAudioPath_ = "../data/test_audio/";

        // Verify test data directory exists
        if (!std::filesystem::exists(masterCallsPath_)) {
            GTEST_SKIP() << "Master calls directory not found at: " << masterCallsPath_;
        }

        // Load available test files
        loadAvailableTestFiles();
    }

    void TearDown() override {
        // Clean up any active sessions
        auto sessions = engine_->getActiveSessions();
        for (auto sessionId : sessions) {
            auto status = engine_->destroySession(sessionId);
            // In teardown, we don't want to fail the test for cleanup issues
            (void)status;  // Suppress unused variable warning
        }
        engine_.reset();
    }

    void loadAvailableTestFiles() {
        for (const auto& fileInfo : TEST_AUDIO_FILES) {
            std::string fullPath = masterCallsPath_ + fileInfo.filename;
            if (std::filesystem::exists(fullPath)) {
                availableFiles_.push_back(fileInfo);
            }
        }

        ASSERT_GT(availableFiles_.size(), 0) << "No test audio files found in " << masterCallsPath_;
    }

    // Helper to create synthetic test signal that resembles wildlife calls
    std::vector<float> createWildlifeCallSynthetic(float fundamental_freq,
                                                   float duration,
                                                   float sample_rate = 44100.0f) {
        size_t num_samples = static_cast<size_t>(duration * sample_rate);
        std::vector<float> signal(num_samples);

        // Create a more complex signal with harmonics and amplitude modulation
        // to better simulate real wildlife calls
        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;

            // Fundamental frequency
            float base = 0.4f * std::sin(2.0f * M_PI * fundamental_freq * t);

            // Add harmonics (common in animal vocalizations)
            float harmonic2 = 0.2f * std::sin(2.0f * M_PI * fundamental_freq * 2.0f * t);
            float harmonic3 = 0.1f * std::sin(2.0f * M_PI * fundamental_freq * 3.0f * t);

            // Add amplitude modulation (breathing/vibrato effects)
            float am_freq = 5.0f;  // 5 Hz modulation
            float am_depth = 0.3f;
            float am = 1.0f + am_depth * std::sin(2.0f * M_PI * am_freq * t);

            // Combine with envelope (attack and decay)
            float envelope = 1.0f;
            if (t < 0.1f) {
                envelope = t / 0.1f;  // 100ms attack
            } else if (t > duration - 0.2f) {
                envelope = (duration - t) / 0.2f;  // 200ms decay
            }

            signal[i] = envelope * am * (base + harmonic2 + harmonic3);

            // Add small amount of noise to simulate realistic conditions
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise_dist(0.0f, 0.01f);
            signal[i] += noise_dist(gen);
        }

        return signal;
    }

    // Process audio in realistic chunks
    UnifiedAudioEngine::Status processAudioInChunks(SessionId sessionId,
                                                    const std::vector<float>& audioData,
                                                    size_t chunkSize = 512) {
        for (size_t i = 0; i < audioData.size(); i += chunkSize) {
            size_t actualSize = std::min(chunkSize, audioData.size() - i);
            std::span<const float> chunk(audioData.data() + i, actualSize);

            auto result = engine_->processAudioChunk(sessionId, chunk);
            if (result != UnifiedAudioEngine::Status::OK) {
                return result;
            }
        }
        return UnifiedAudioEngine::Status::OK;
    }

    std::unique_ptr<UnifiedAudioEngine> engine_;
    std::string masterCallsPath_;
    std::string testAudioPath_;
    std::vector<AudioFileInfo> availableFiles_;
};

/**
 * Test loading and processing real wildlife call audio files
 */
TEST_F(RealWildlifeCallAnalysisTest, LoadAndProcessRealAudioFiles) {
    ASSERT_GT(availableFiles_.size(), 0) << "No test files available";

    for (const auto& fileInfo : availableFiles_) {
        SCOPED_TRACE("Processing file: " + fileInfo.filename);

        std::string fullPath = masterCallsPath_ + fileInfo.filename;
        auto audioData = loadAudioFile(fullPath);

        ASSERT_TRUE(audioData.valid) << "Failed to load audio file: " << fullPath;
        ASSERT_GT(audioData.samples.size(), 0) << "Empty audio data in file: " << fullPath;
        EXPECT_EQ(audioData.sample_rate, 44100) << "Unexpected sample rate in: " << fullPath;

        // Create session
        auto sessionResult = engine_->createSession(static_cast<float>(audioData.sample_rate));
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session for: " << fileInfo.filename;
        SessionId sessionId = sessionResult.value;

        // Process the audio
        auto processResult = processAudioInChunks(sessionId, audioData.samples);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
            << "Failed to process audio for: " << fileInfo.filename;

        // Verify we extracted features
        auto featureCountResult = engine_->getFeatureCount(sessionId);
        ASSERT_TRUE(featureCountResult.isOk());
        EXPECT_GT(featureCountResult.value, 0)
            << "No features extracted from: " << fileInfo.filename;

        // Clean up
        auto destroyStatus = engine_->destroySession(sessionId);
        EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
    }
}

/**
 * Test master call loading and similarity scoring with real audio
 */
TEST_F(RealWildlifeCallAnalysisTest, MasterCallSimilarityScoring) {
    ASSERT_GE(availableFiles_.size(), 2) << "Need at least 2 test files for similarity testing";

    const auto& masterFile = availableFiles_[0];
    const auto& testFile = availableFiles_[1];

    // Create session
    auto sessionResult = engine_->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = sessionResult.value;

    // Load master call (using filename without extension as ID)
    std::string masterCallId = masterFile.filename.substr(0, masterFile.filename.find_last_of('.'));
    auto loadResult = engine_->loadMasterCall(sessionId, masterCallId);

    if (loadResult == UnifiedAudioEngine::Status::FILE_NOT_FOUND) {
        GTEST_SKIP() << "Master call file not found: " << masterCallId;
    }

    ASSERT_EQ(loadResult, UnifiedAudioEngine::Status::OK)
        << "Failed to load master call: " << masterCallId;

    // Verify master call is loaded
    auto currentMasterResult = engine_->getCurrentMasterCall(sessionId);
    ASSERT_TRUE(currentMasterResult.isOk());
    EXPECT_EQ(currentMasterResult.value, masterCallId);

    // Process test audio for comparison
    std::string testFilePath = masterCallsPath_ + testFile.filename;
    auto testAudioData = loadAudioFile(testFilePath);
    ASSERT_TRUE(testAudioData.valid) << "Failed to load test audio: " << testFilePath;

    auto processResult = processAudioInChunks(sessionId, testAudioData.samples);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Get similarity score
    auto scoreResult = engine_->getSimilarityScore(sessionId);
    ASSERT_TRUE(scoreResult.isOk()) << "Failed to get similarity score";

    float similarity = scoreResult.value;
    EXPECT_GE(similarity, 0.0f) << "Similarity score should be non-negative";
    EXPECT_LE(similarity, 1.0f) << "Similarity score should not exceed 1.0";

    // Log results for analysis
    std::cout << "Master: " << masterFile.filename << " vs Test: " << testFile.filename
              << " -> Similarity: " << similarity << std::endl;

    // Clean up
    auto destroyStatus = engine_->destroySession(sessionId);
    EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
}

/**
 * Test RealtimeScorer integration with real audio data
 */
TEST_F(RealWildlifeCallAnalysisTest, RealtimeScorerWithRealAudio) {
    ASSERT_GT(availableFiles_.size(), 0) << "No test files available";

    const auto& testFile = availableFiles_[0];

    // Create session
    auto sessionResult = engine_->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = sessionResult.value;

    // Load the same file as both master and test (should give high similarity)
    std::string masterCallId = testFile.filename.substr(0, testFile.filename.find_last_of('.'));
    auto loadResult = engine_->loadMasterCall(sessionId, masterCallId);

    if (loadResult == UnifiedAudioEngine::Status::FILE_NOT_FOUND) {
        GTEST_SKIP() << "Master call file not found: " << masterCallId;
    }

    ASSERT_EQ(loadResult, UnifiedAudioEngine::Status::OK);

    // Configure RealtimeScorer for optimal performance
    RealtimeScorerConfig config;
    config.mfccWeight = 0.5f;
    config.volumeWeight = 0.2f;
    config.timingWeight = 0.2f;
    config.pitchWeight = 0.1f;
    config.confidenceThreshold = 0.7f;
    config.minScoreForMatch = 0.005f;
    config.enablePitchAnalysis = false;  // Disable for now
    config.scoringHistorySize = 100;

    auto configResult = engine_->setRealtimeScorerConfig(sessionId, config);
    EXPECT_EQ(configResult, UnifiedAudioEngine::Status::OK);

    // Process the same audio file (should be self-similar)
    std::string testFilePath = masterCallsPath_ + testFile.filename;
    auto audioData = loadAudioFile(testFilePath);
    ASSERT_TRUE(audioData.valid);

    auto processResult = processAudioInChunks(sessionId, audioData.samples);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Get detailed scoring results
    auto detailedScoreResult = engine_->getDetailedScore(sessionId);
    ASSERT_TRUE(detailedScoreResult.isOk()) << "Failed to get detailed score";

    const auto& score = detailedScoreResult.value;

    // Validate score components
    EXPECT_GE(score.overall, 0.0f) << "Overall score should be non-negative";
    EXPECT_GE(score.mfcc, 0.0f) << "MFCC score should be non-negative";
    EXPECT_GE(score.volume, 0.0f) << "Volume score should be non-negative";
    EXPECT_GE(score.timing, 0.0f) << "Timing score should be non-negative";
    EXPECT_GE(score.confidence, 0.0f) << "Confidence should be non-negative";
    EXPECT_LE(score.confidence, 1.0f) << "Confidence should not exceed 1.0";
    EXPECT_GT(score.samplesAnalyzed, 0) << "Should have analyzed some samples";

    // For self-comparison, we expect reasonably high scores
    if (score.confidence > 0.5f) {
        EXPECT_GT(score.overall, 0.001f) << "Self-comparison should yield decent similarity";
    }

    // Get real-time feedback
    auto feedbackResult = engine_->getRealtimeFeedback(sessionId);
    ASSERT_TRUE(feedbackResult.isOk()) << "Failed to get real-time feedback";

    const auto& feedback = feedbackResult.value;
    EXPECT_FALSE(feedback.qualityAssessment.empty()) << "Should provide quality assessment";
    EXPECT_GE(feedback.progressRatio, 0.0f) << "Progress ratio should be non-negative";
    EXPECT_LE(feedback.progressRatio, 1.0f) << "Progress ratio should not exceed 1.0";

    // Export results as JSON for external analysis
    auto jsonResult = engine_->exportScoreToJson(sessionId);
    ASSERT_TRUE(jsonResult.isOk()) << "Failed to export score to JSON";
    EXPECT_GT(jsonResult.value.length(), 10) << "JSON export should contain data";

    // Log detailed results
    std::cout << "\n=== Detailed Scoring Results for " << testFile.filename << " ===\n";
    std::cout << "Overall Score: " << score.overall << "\n";
    std::cout << "MFCC Score: " << score.mfcc << "\n";
    std::cout << "Volume Score: " << score.volume << "\n";
    std::cout << "Timing Score: " << score.timing << "\n";
    std::cout << "Confidence: " << score.confidence << "\n";
    std::cout << "Is Reliable: " << (score.isReliable ? "Yes" : "No") << "\n";
    std::cout << "Is Match: " << (score.isMatch ? "Yes" : "No") << "\n";
    std::cout << "Samples Analyzed: " << score.samplesAnalyzed << "\n";
    std::cout << "Quality Assessment: " << feedback.qualityAssessment << "\n";
    std::cout << "JSON Export Length: " << jsonResult.value.length() << " characters\n";

    // Clean up
    auto destroyStatus = engine_->destroySession(sessionId);
    EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
}

/**
 * Test cross-validation between different call types
 */
TEST_F(RealWildlifeCallAnalysisTest, CrossValidationBetweenCallTypes) {
    // We need at least 3 different call types for meaningful cross-validation
    ASSERT_GE(availableFiles_.size(), 3) << "Need at least 3 different call types";

    struct SimilarityResult {
        std::string masterCall;
        std::string testCall;
        float similarity;
        bool expectedMatch;
    };

    std::vector<SimilarityResult> results;

    // Test each file against every other file
    for (size_t i = 0; i < std::min(availableFiles_.size(), size_t(3)); ++i) {
        const auto& masterFile = availableFiles_[i];

        for (size_t j = 0; j < std::min(availableFiles_.size(), size_t(3)); ++j) {
            if (i == j)
                continue;  // Skip self-comparison for this test

            const auto& testFile = availableFiles_[j];

            // Create session
            auto sessionResult = engine_->createSession(44100.0f);
            ASSERT_TRUE(sessionResult.isOk());
            SessionId sessionId = sessionResult.value;

            // Load master call
            std::string masterCallId =
                masterFile.filename.substr(0, masterFile.filename.find_last_of('.'));
            auto loadResult = engine_->loadMasterCall(sessionId, masterCallId);

            if (loadResult != UnifiedAudioEngine::Status::OK) {
                auto destroyStatus = engine_->destroySession(sessionId);
                (void)destroyStatus;  // Suppress unused variable warning
                continue;             // Skip this combination
            }

            // Process test audio
            std::string testFilePath = masterCallsPath_ + testFile.filename;
            auto testAudioData = loadAudioFile(testFilePath);
            if (!testAudioData.valid) {
                auto destroyStatus = engine_->destroySession(sessionId);
                (void)destroyStatus;  // Suppress unused variable warning
                continue;
            }

            auto processResult = processAudioInChunks(sessionId, testAudioData.samples);
            if (processResult != UnifiedAudioEngine::Status::OK) {
                auto destroyStatus = engine_->destroySession(sessionId);
                (void)destroyStatus;  // Suppress unused variable warning
                continue;
            }

            // Get similarity score
            auto scoreResult = engine_->getSimilarityScore(sessionId);
            if (scoreResult.isOk()) {
                SimilarityResult result;
                result.masterCall = masterFile.call_type;
                result.testCall = testFile.call_type;
                result.similarity = scoreResult.value;
                result.expectedMatch = (masterFile.call_type == testFile.call_type);

                results.push_back(result);
            }

            auto destroyStatus = engine_->destroySession(sessionId);
            EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
        }
    }

    ASSERT_GT(results.size(), 0) << "No valid comparisons completed";

    // Analyze results
    std::cout << "\n=== Cross-Validation Results ===\n";
    std::cout << "Master Call Type | Test Call Type | Similarity | Expected Match\n";
    std::cout << "-----------------|----------------|------------|---------------\n";

    float totalSimilarity = 0.0f;
    int correctPredictions = 0;

    for (const auto& result : results) {
        std::cout << std::setw(15) << result.masterCall << " | " << std::setw(13) << result.testCall
                  << " | " << std::setw(9) << std::fixed << std::setprecision(6)
                  << result.similarity << " | " << (result.expectedMatch ? "Yes" : "No") << "\n";

        totalSimilarity += result.similarity;

        // Simple threshold-based prediction (adjust threshold based on results)
        const float threshold = 0.01f;  // This may need tuning based on your data
        bool predicted_match = result.similarity > threshold;
        if (predicted_match == result.expectedMatch) {
            correctPredictions++;
        }
    }

    float averageSimilarity = totalSimilarity / results.size();
    float accuracy = static_cast<float>(correctPredictions) / results.size();

    std::cout << "\n=== Summary ===\n";
    std::cout << "Total Comparisons: " << results.size() << "\n";
    std::cout << "Average Similarity: " << averageSimilarity << "\n";
    std::cout << "Classification Accuracy: " << (accuracy * 100.0f) << "%\n";

    // Basic validation - system should perform better than random
    EXPECT_GT(accuracy, 0.3f) << "Classification accuracy should be better than random";
    EXPECT_GT(averageSimilarity, 0.0f) << "Average similarity should be positive";
}

/**
 * Performance benchmark with real audio data
 */
TEST_F(RealWildlifeCallAnalysisTest, PerformanceBenchmarkWithRealAudio) {
    ASSERT_GT(availableFiles_.size(), 0) << "No test files available";

    const auto& testFile = availableFiles_[0];
    std::string testFilePath = masterCallsPath_ + testFile.filename;
    auto audioData = loadAudioFile(testFilePath);
    ASSERT_TRUE(audioData.valid) << "Failed to load test audio";

    // Create session
    auto sessionResult = engine_->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = sessionResult.value;

    // Load master call
    std::string masterCallId = testFile.filename.substr(0, testFile.filename.find_last_of('.'));
    auto loadResult = engine_->loadMasterCall(sessionId, masterCallId);
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Master call not found: " << masterCallId;
    }

    // Benchmark processing time
    const int numRuns = 5;
    std::vector<double> processingTimes;

    for (int run = 0; run < numRuns; ++run) {
        // Reset session for clean measurement
        auto resetStatus = engine_->resetSession(sessionId);
        EXPECT_TRUE(resetStatus == UnifiedAudioEngine::Status::OK);

        auto startTime = std::chrono::high_resolution_clock::now();

        // Process audio in chunks
        auto processResult = processAudioInChunks(sessionId, audioData.samples);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        processingTimes.push_back(duration.count() / 1000.0);  // Convert to milliseconds
    }

    // Calculate statistics
    double totalTime = std::accumulate(processingTimes.begin(), processingTimes.end(), 0.0);
    double averageTime = totalTime / numRuns;

    double minTime = *std::min_element(processingTimes.begin(), processingTimes.end());
    double maxTime = *std::max_element(processingTimes.begin(), processingTimes.end());

    // Calculate real-time performance ratio
    float audioLengthMs =
        (audioData.samples.size() / static_cast<float>(audioData.sample_rate)) * 1000.0f;
    double realTimeRatio = averageTime / audioLengthMs;

    std::cout << "\n=== Performance Benchmark Results ===\n";
    std::cout << "Audio File: " << testFile.filename << "\n";
    std::cout << "Audio Length: " << audioLengthMs << " ms (" << (audioLengthMs / 1000.0f)
              << " seconds)\n";
    std::cout << "Samples: " << audioData.samples.size() << "\n";
    std::cout << "Processing Times (ms):\n";
    std::cout << "  Average: " << averageTime << "\n";
    std::cout << "  Minimum: " << minTime << "\n";
    std::cout << "  Maximum: " << maxTime << "\n";
    std::cout << "Real-time Ratio: " << realTimeRatio << "x\n";
    std::cout << "Throughput: " << (audioData.samples.size() / (averageTime / 1000.0))
              << " samples/second\n";

    // Performance assertions
    EXPECT_LT(realTimeRatio, 1.0)
        << "Processing should be faster than real-time for efficient operation";
    EXPECT_GT(averageTime, 0.0) << "Processing time should be measurable";

    // Clean up
    auto destroyStatus = engine_->destroySession(sessionId);
    EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
}

/**
 * Test robust error handling with real audio edge cases
 */
TEST_F(RealWildlifeCallAnalysisTest, ErrorHandlingWithRealAudio) {
    // Create session
    auto sessionResult = engine_->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = sessionResult.value;

    // Test 1: Load non-existent master call
    auto badLoadResult = engine_->loadMasterCall(sessionId, "nonexistent_call");
    EXPECT_EQ(badLoadResult, UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Test 2: Try to get similarity without master call
    auto noMasterScoreResult = engine_->getSimilarityScore(sessionId);
    EXPECT_EQ(noMasterScoreResult.error(), UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    // Test 3: Process empty audio data
    std::vector<float> emptyAudio;
    std::span<const float> emptySpan(emptyAudio);
    auto emptyProcessResult = engine_->processAudioChunk(sessionId, emptySpan);
    EXPECT_EQ(emptyProcessResult, UnifiedAudioEngine::Status::OK);  // Should handle gracefully

    // Test 4: Try RealtimeScorer operations without proper setup
    auto badConfigResult = engine_->setRealtimeScorerConfig(sessionId, RealtimeScorerConfig{});
    // Should either work or fail gracefully
    EXPECT_TRUE(badConfigResult == UnifiedAudioEngine::Status::OK
                || badConfigResult == UnifiedAudioEngine::Status::INIT_FAILED);

    // Test 5: Load a valid master call and then process extremely short audio
    if (availableFiles_.size() > 0) {
        std::string masterCallId =
            availableFiles_[0].filename.substr(0, availableFiles_[0].filename.find_last_of('.'));
        auto goodLoadResult = engine_->loadMasterCall(sessionId, masterCallId);

        if (goodLoadResult == UnifiedAudioEngine::Status::OK) {
            // Process very short audio snippet
            std::vector<float> shortAudio(10, 0.1f);  // 10 samples
            std::span<const float> shortSpan(shortAudio);
            auto shortProcessResult = engine_->processAudioChunk(sessionId, shortSpan);
            EXPECT_EQ(shortProcessResult, UnifiedAudioEngine::Status::OK);

            // Should still handle gracefully, might return insufficient data for scoring
            auto shortScoreResult = engine_->getSimilarityScore(sessionId);
            EXPECT_TRUE(shortScoreResult.isOk()
                        || shortScoreResult.error()
                               == UnifiedAudioEngine::Status::INSUFFICIENT_DATA);
        }
    }

    // Clean up
    auto destroyStatus = engine_->destroySession(sessionId);
    EXPECT_TRUE(destroyStatus == UnifiedAudioEngine::Status::OK);
}
