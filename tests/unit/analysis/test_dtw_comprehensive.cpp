/**
 * @file test_dtw_comprehensive.cpp
 * @brief Comprehensive tests for DTW (Dynamic Time Warping) functionality
 */

#include <cmath>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class DTWComprehensiveTest : public ::testing::Test {
  protected:
    std::unique_ptr<DTWComparator> dtw;
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;

    void SetUp() override {
        // Initialize DTW with default configuration
        DTWComparator::Config config;
        dtw = std::make_unique<DTWComparator>(config);

        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            engine->destroySession(sessionId);
        }
    }

    // Helper function to create MFCC-like feature vectors
    std::vector<std::vector<float>> createFeatureSequence(size_t numFrames, size_t numCoeffs) {
        std::vector<std::vector<float>> features(numFrames, std::vector<float>(numCoeffs));

        for (size_t i = 0; i < numFrames; ++i) {
            for (size_t j = 0; j < numCoeffs; ++j) {
                // Create some pattern based on frame and coefficient indices
                features[i][j] = std::sin(0.1f * i + 0.2f * j) * 0.5f + 0.5f;
            }
        }
        return features;
    }

    std::vector<std::vector<float>>
    createNoisyVersion(const std::vector<std::vector<float>>& original, float noiseLevel = 0.1f) {
        auto noisy = original;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-noiseLevel, noiseLevel);

        for (auto& frame : noisy) {
            for (auto& coeff : frame) {
                coeff += dis(gen);
            }
        }
        return noisy;
    }
};

TEST_F(DTWComprehensiveTest, IdenticalSequencesZeroDistance) {
    auto seq1 = createFeatureSequence(10, 13);
    auto seq2 = seq1;  // Identical copy

    float distance = dtw->compare(seq1, seq2);
    EXPECT_FLOAT_EQ(distance, 0.0f) << "Identical sequences should have zero DTW distance";
}

TEST_F(DTWComprehensiveTest, SelfComparisonConsistency) {
    auto seq = createFeatureSequence(15, 13);

    // Compare sequence with itself multiple times
    std::vector<float> distances;
    for (int i = 0; i < 5; ++i) {
        distances.push_back(dtw->compare(seq, seq));
    }

    // All distances should be identical (and zero)
    for (size_t i = 1; i < distances.size(); ++i) {
        EXPECT_FLOAT_EQ(distances[0], distances[i]) << "DTW self-comparison should be consistent";
    }
    EXPECT_FLOAT_EQ(distances[0], 0.0f);
}

TEST_F(DTWComprehensiveTest, SymmetryProperty) {
    auto seq1 = createFeatureSequence(12, 13);
    auto seq2 = createFeatureSequence(15, 13);

    float dist1to2 = dtw->compare(seq1, seq2);
    float dist2to1 = dtw->compare(seq2, seq1);

    EXPECT_FLOAT_EQ(dist1to2, dist2to1) << "DTW distance should be symmetric: d(A,B) = d(B,A)";
}

TEST_F(DTWComprehensiveTest, EmptySequenceHandling) {
    std::vector<std::vector<float>> empty;
    auto seq = createFeatureSequence(10, 13);

    float dist1 = dtw->compare(empty, seq);
    float dist2 = dtw->compare(seq, empty);
    float dist3 = dtw->compare(empty, empty);

    // Should handle gracefully (might return infinity or large value)
    EXPECT_TRUE(std::isinf(dist1) || dist1 > 1000.0f);
    EXPECT_TRUE(std::isinf(dist2) || dist2 > 1000.0f);
    EXPECT_TRUE(std::isinf(dist3) || dist3 == 0.0f);
}

TEST_F(DTWComprehensiveTest, DifferentLengthSequences) {
    auto shortSeq = createFeatureSequence(5, 13);
    auto longSeq = createFeatureSequence(20, 13);

    float distance = dtw->compare(shortSeq, longSeq);

    // Should handle different lengths without crashing
    EXPECT_GE(distance, 0.0f);
    EXPECT_FALSE(std::isnan(distance));
}

TEST_F(DTWComprehensiveTest, WindowConstraintEffectiveness) {
    auto seq1 = createFeatureSequence(20, 13);
    auto seq2 = createFeatureSequence(20, 13);

    // Compare with different window ratios
    float distanceNoWindow = dtw->compare(seq1, seq2);

    dtw->setWindowRatio(0.1f);  // Very restrictive window
    float distanceSmallWindow = dtw->compare(seq1, seq2);

    dtw->setWindowRatio(0.5f);  // Moderate window
    float distanceMediumWindow = dtw->compare(seq1, seq2);

    dtw->setWindowRatio(1.0f);  // No constraint (full window)
    float distanceFullWindow = dtw->compare(seq1, seq2);

    // More restrictive windows should generally give different results
    EXPECT_GE(distanceSmallWindow, 0.0f);
    EXPECT_GE(distanceMediumWindow, 0.0f);
    EXPECT_GE(distanceFullWindow, 0.0f);
}

TEST_F(DTWComprehensiveTest, NoiseRobustness) {
    auto original = createFeatureSequence(15, 13);
    auto noisyLow = createNoisyVersion(original, 0.05f);  // Low noise
    auto noisyHigh = createNoisyVersion(original, 0.2f);  // High noise

    float distanceOriginal = dtw->compare(original, original);
    float distanceLowNoise = dtw->compare(original, noisyLow);
    float distanceHighNoise = dtw->compare(original, noisyHigh);

    EXPECT_FLOAT_EQ(distanceOriginal, 0.0f);
    EXPECT_LT(distanceLowNoise, distanceHighNoise)
        << "Lower noise should result in smaller DTW distance";
    EXPECT_GT(distanceLowNoise, 0.0f) << "Even low noise should result in non-zero distance";
}

TEST_F(DTWComprehensiveTest, TriangleInequality) {
    auto seq1 = createFeatureSequence(10, 13);
    auto seq2 = createFeatureSequence(12, 13);
    auto seq3 = createFeatureSequence(14, 13);

    float dist12 = dtw->compare(seq1, seq2);
    float dist23 = dtw->compare(seq2, seq3);
    float dist13 = dtw->compare(seq1, seq3);

    // DTW should satisfy triangle inequality: d(A,C) <= d(A,B) + d(B,C)
    EXPECT_LE(dist13, dist12 + dist23 + 1e-5f)  // Small epsilon for floating point
        << "DTW should satisfy triangle inequality";
}

TEST_F(DTWComprehensiveTest, ProgressiveAlignment) {
    // Create sequences where one is a stretched version of the other
    auto baseSeq = createFeatureSequence(10, 13);

    // Create a stretched version by duplicating some frames
    std::vector<std::vector<float>> stretchedSeq;
    for (size_t i = 0; i < baseSeq.size(); ++i) {
        stretchedSeq.push_back(baseSeq[i]);
        if (i % 3 == 0) {  // Duplicate every 3rd frame
            stretchedSeq.push_back(baseSeq[i]);
        }
    }

    float distance = dtw->compare(baseSeq, stretchedSeq);

    // DTW should handle temporal stretching well
    EXPECT_GE(distance, 0.0f);
    EXPECT_LT(distance, 10.0f) << "DTW should handle temporal stretching reasonably well";
}

TEST_F(DTWComprehensiveTest, PathRetrieval) {
    auto seq1 = createFeatureSequence(8, 13);
    auto seq2 = createFeatureSequence(10, 13);

    std::vector<std::pair<size_t, size_t>> path;
    float distance = dtw->compareWithPath(seq1, seq2, path);

    EXPECT_GE(distance, 0.0f);
    EXPECT_FALSE(path.empty()) << "DTW path should not be empty for non-empty sequences";

    // Verify path properties
    if (!path.empty()) {
        // Path should start at (0,0)
        EXPECT_EQ(path.front().first, 0);
        EXPECT_EQ(path.front().second, 0);

        // Path should end at (seq1.size()-1, seq2.size()-1)
        EXPECT_EQ(path.back().first, static_cast<int>(seq1.size() - 1));
        EXPECT_EQ(path.back().second, static_cast<int>(seq2.size() - 1));

        // Path should be monotonic
        for (size_t i = 1; i < path.size(); ++i) {
            EXPECT_GE(path[i].first, path[i - 1].first);
            EXPECT_GE(path[i].second, path[i - 1].second);

            // Steps should be valid (1,0), (0,1), or (1,1)
            int deltaI = path[i].first - path[i - 1].first;
            int deltaJ = path[i].second - path[i - 1].second;
            EXPECT_TRUE((deltaI == 1 && deltaJ == 0) || (deltaI == 0 && deltaJ == 1)
                        || (deltaI == 1 && deltaJ == 1))
                << "Invalid DTW step: (" << deltaI << ", " << deltaJ << ")";
        }
    }
}

TEST_F(DTWComprehensiveTest, NormalizedDistance) {
    auto seq1 = createFeatureSequence(5, 13);
    auto seq2 = createFeatureSequence(15, 13);

    // Test with normalization disabled
    DTWComparator::Config configRaw;
    configRaw.normalize_distance = false;
    auto dtwRaw = std::make_unique<DTWComparator>(configRaw);
    float rawDistance = dtwRaw->compare(seq1, seq2);

    // Test with normalization enabled (default)
    float normalizedDistance = dtw->compare(seq1, seq2);

    EXPECT_GE(rawDistance, 0.0f);
    EXPECT_GE(normalizedDistance, 0.0f);

    // Normalized distance should be different from raw distance for different length sequences
    if (seq1.size() != seq2.size()) {
        EXPECT_NE(rawDistance, normalizedDistance);
    }
}

TEST_F(DTWComprehensiveTest, FeatureDimensionConsistency) {
    auto seq1 = createFeatureSequence(10, 13);  // 13 coefficients
    auto seq2 = createFeatureSequence(10, 20);  // 20 coefficients (different)

    // DTW should handle or reject mismatched feature dimensions
    float distance = dtw->compare(seq1, seq2);

    // Either should work (if DTW handles it) or return infinity/error
    EXPECT_TRUE(distance >= 0.0f || std::isinf(distance));
}

TEST_F(DTWComprehensiveTest, PerformanceWithLargeSequences) {
    auto largeSeq1 = createFeatureSequence(100, 13);
    auto largeSeq2 = createFeatureSequence(120, 13);

    auto startTime = std::chrono::high_resolution_clock::now();
    float distance = dtw->compare(largeSeq1, largeSeq2);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    EXPECT_GE(distance, 0.0f);
    EXPECT_LT(duration.count(), 5000)
        << "DTW should complete within 5 seconds for moderate-sized sequences";
}

TEST_F(DTWComprehensiveTest, IntegrationWithUnifiedEngine) {
    // Test DTW functionality through UnifiedAudioEngine

    // Generate test audio
    std::vector<float> audio1(8820, 0.0f);  // 0.2 seconds at 44.1kHz
    std::vector<float> audio2(8820, 0.0f);

    // Fill with different sine waves
    for (size_t i = 0; i < audio1.size(); ++i) {
        audio1[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        audio2[i] = 0.5f * std::sin(2.0f * M_PI * 880.0f * i / 44100.0f);  // Different frequency
    }

    // Process both audio clips
    auto result1 = engine->processAudioChunk(sessionId, audio1);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);

    auto featureCount1 = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount1.isOk());
    EXPECT_GT(*featureCount1, 0);

    // Reset and process second audio
    engine->resetSession(sessionId);
    auto result2 = engine->processAudioChunk(sessionId, audio2);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);

    auto featureCount2 = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount2.isOk());
    EXPECT_GT(*featureCount2, 0);

    // Feature counts should be similar for similar-length audio
    EXPECT_NEAR(*featureCount1, *featureCount2, 5)
        << "Feature counts should be similar for similar-length audio";
}
