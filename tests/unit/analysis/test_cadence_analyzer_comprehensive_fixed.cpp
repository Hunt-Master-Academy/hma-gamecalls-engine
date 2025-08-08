/**
 * @file test_cadence_analyzer_comprehensive_fixed.cpp
 * @brief Fixed CadenceAnalyzer comprehensive coverage test with correct API usage
 *
 * Targets CadenceAnalyzer coverage improvement using only existing API methods
 * Tests all major code paths, temporal analysis methods, and edge cases
 */

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/CadenceAnalyzer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class CadenceAnalyzerComprehensiveFixedTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Standard configuration based on actual header file API
        standard_config.sampleRate = 44100.0f;
        standard_config.frameSize = 0.025f;  // 25ms frames
        standard_config.hopSize = 0.010f;    // 10ms hop
        standard_config.minTempo = 30.0f;
        standard_config.maxTempo = 300.0f;
        standard_config.minPeriod = 0.1f;
        standard_config.maxPeriod = 5.0f;
        standard_config.onsetThreshold = 0.3f;
        standard_config.silenceThreshold = -30.0f;
        standard_config.autocorrelationLags = 1000;
        standard_config.enableBeatTracking = true;
        standard_config.enableOnsetDetection = true;
        standard_config.enableSyllableAnalysis = true;
        standard_config.adaptiveThreshold = 0.1f;
    }

    CadenceAnalyzer::Config standard_config;

    // Helper to generate rhythmic signals with beats
    std::vector<float> generateRhythmicSignal(float bpm, int num_beats, float duration) {
        size_t length = static_cast<size_t>(duration * standard_config.sampleRate);
        std::vector<float> signal(length, 0.0f);

        float beat_interval = 60.0f / bpm;  // Interval between beats in seconds
        for (int beat = 0; beat < num_beats; ++beat) {
            float beat_time = beat * beat_interval;
            size_t beat_sample = static_cast<size_t>(beat_time * standard_config.sampleRate);

            if (beat_sample < length) {
                // Create a short burst for each beat
                size_t burst_length =
                    static_cast<size_t>(0.05f * standard_config.sampleRate);  // 50ms burst
                for (size_t i = 0; i < burst_length && beat_sample + i < length; ++i) {
                    float envelope = std::exp(-i / (0.01f * standard_config.sampleRate));  // Decay
                    signal[beat_sample + i] =
                        envelope * std::sin(2.0f * M_PI * 440.0f * i / standard_config.sampleRate);
                }
            }
        }
        return signal;
    }

    // Helper to generate call sequence (multiple calls with intervals)
    std::vector<float> generateCallSequence(const std::vector<float>& call_times,
                                            const std::vector<float>& call_durations,
                                            float total_duration) {
        size_t length = static_cast<size_t>(total_duration * standard_config.sampleRate);
        std::vector<float> signal(length, 0.0f);

        for (size_t i = 0; i < call_times.size() && i < call_durations.size(); ++i) {
            size_t start_sample = static_cast<size_t>(call_times[i] * standard_config.sampleRate);
            size_t duration_samples =
                static_cast<size_t>(call_durations[i] * standard_config.sampleRate);

            for (size_t j = 0; j < duration_samples && start_sample + j < length; ++j) {
                float freq =
                    300.0f
                    + 200.0f * std::sin(2.0f * M_PI * j / (0.1f * standard_config.sampleRate));
                signal[start_sample + j] =
                    0.5f * std::sin(2.0f * M_PI * freq * j / standard_config.sampleRate);
            }
        }
        return signal;
    }

    std::vector<float> generateSilence(float duration) {
        size_t length = static_cast<size_t>(duration * standard_config.sampleRate);
        return std::vector<float>(length, 0.0f);
    }

    std::vector<float> generateNoise(float duration, float amplitude = 0.1f) {
        size_t length = static_cast<size_t>(duration * standard_config.sampleRate);
        std::vector<float> signal(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, amplitude);

        for (size_t i = 0; i < length; ++i) {
            signal[i] = dist(gen);
        }
        return signal;
    }
};

// Test 1: Factory method and initialization - targeting factory coverage
TEST_F(CadenceAnalyzerComprehensiveFixedTest, FactoryMethodAndInitialization) {
    // Test successful creation
    {
        auto result = CadenceAnalyzer::create(standard_config);
        EXPECT_TRUE(result.has_value()) << "Factory should create valid CadenceAnalyzer";
        EXPECT_NE(result.value().get(), nullptr) << "Should return valid pointer";
    }

    // Test invalid sample rate
    {
        auto config = standard_config;
        config.sampleRate = 0.0f;
        auto result = CadenceAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with zero sample rate";
    }

    // Test invalid frame size
    {
        auto config = standard_config;
        config.frameSize = 0.0f;
        auto result = CadenceAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with zero frame size";
    }

    // Test invalid hop size (larger than frame)
    {
        auto config = standard_config;
        config.hopSize = 0.050f;  // Larger than frame size (0.025f)
        auto result = CadenceAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with hop size > frame size";
    }

    // Test invalid tempo range
    {
        auto config = standard_config;
        config.minTempo = 200.0f;
        config.maxTempo = 100.0f;  // min > max
        auto result = CadenceAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with invalid tempo range";
    }
}

// Test 2: Basic cadence analysis - targeting main analysis path
TEST_F(CadenceAnalyzerComprehensiveFixedTest, BasicCadenceAnalysis) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test with rhythmic signal
    {
        auto signal = generateRhythmicSignal(120.0f, 8, 4.0f);  // 120 BPM, 8 beats, 4 seconds
        auto result = analyzer->analyzeCadence(signal);
        EXPECT_TRUE(result.has_value()) << "Analysis should succeed with rhythmic signal";

        if (result.has_value()) {
            const auto& profile = result.value();
            EXPECT_GT(profile.estimatedTempo, 0.0f) << "Should estimate some tempo";
            EXPECT_GE(profile.confidence, 0.0f) << "Confidence should be non-negative";
            EXPECT_LE(profile.confidence, 1.0f) << "Confidence should not exceed 1.0";
        }
    }

    // Test with call sequence
    {
        std::vector<float> call_times = {0.5f, 1.2f, 2.0f, 2.8f};
        std::vector<float> call_durations = {0.2f, 0.15f, 0.25f, 0.18f};
        auto signal = generateCallSequence(call_times, call_durations, 4.0f);

        auto result = analyzer->analyzeCadence(signal);
        EXPECT_TRUE(result.has_value()) << "Analysis should succeed with call sequence";

        if (result.has_value()) {
            const auto& profile = result.value();
            EXPECT_GE(profile.sequence.numCalls, 0) << "Should detect calls";
            EXPECT_GE(profile.sequence.callRate, 0.0f) << "Call rate should be non-negative";
        }
    }

    // Test with silence
    {
        auto signal = generateSilence(2.0f);
        auto result = analyzer->analyzeCadence(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle silence gracefully";

        if (result.has_value()) {
            const auto& profile = result.value();
            EXPECT_FALSE(profile.hasStrongRhythm) << "Silence should not have strong rhythm";
        }
    }

    // Test with noise
    {
        auto signal = generateNoise(2.0f, 0.1f);
        auto result = analyzer->analyzeCadence(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle noise gracefully";
    }
}

// Test 3: Continuous processing - targeting processAudioChunk path
TEST_F(CadenceAnalyzerComprehensiveFixedTest, ContinuousProcessing) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test chunk processing
    {
        auto full_signal = generateRhythmicSignal(100.0f, 10, 5.0f);
        size_t chunk_size =
            static_cast<size_t>(0.5f * standard_config.sampleRate);  // 0.5 second chunks

        for (size_t i = 0; i < full_signal.size(); i += chunk_size) {
            size_t end_idx = std::min(i + chunk_size, full_signal.size());
            std::span<const float> chunk(full_signal.data() + i, end_idx - i);

            auto result = analyzer->processAudioChunk(chunk);
            EXPECT_TRUE(result.has_value()) << "Chunk processing should succeed";
        }

        // Get current analysis after processing chunks
        auto analysisResult = analyzer->getCurrentAnalysis();
        EXPECT_TRUE(analysisResult.has_value()) << "Should get current analysis";
    }

    // Test reset functionality
    {
        analyzer->reset();

        auto analysisResult = analyzer->getCurrentAnalysis();
        // After reset, might return error or default values - both are acceptable
    }

    // Test empty chunk handling
    {
        std::vector<float> empty_chunk;
        auto result = analyzer->processAudioChunk(empty_chunk);
        EXPECT_FALSE(result.has_value()) << "Empty chunk should be rejected";
    }
}

// Test 4: Onset detection - targeting onset detection paths
TEST_F(CadenceAnalyzerComprehensiveFixedTest, OnsetDetection) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test onset detection with clear beats
    {
        auto signal = generateRhythmicSignal(90.0f, 6, 4.0f);  // 90 BPM, 6 beats
        auto result = analyzer->detectOnsets(signal);
        EXPECT_TRUE(result.has_value()) << "Onset detection should succeed";

        if (result.has_value()) {
            const auto& onsets = result.value();
            EXPECT_GT(onsets.size(), 0) << "Should detect some onsets";

            // Verify onsets are in ascending order
            for (size_t i = 1; i < onsets.size(); ++i) {
                EXPECT_GT(onsets[i], onsets[i - 1]) << "Onsets should be in chronological order";
            }
        }
    }

    // Test with onset detection disabled
    {
        auto config = standard_config;
        config.enableOnsetDetection = false;
        auto disabledAnalyzer = CadenceAnalyzer::create(config);
        ASSERT_TRUE(disabledAnalyzer.has_value());

        auto signal = generateRhythmicSignal(100.0f, 4, 2.0f);
        auto result = disabledAnalyzer.value()->detectOnsets(signal);
        // Should handle disabled onset detection appropriately
    }

    // Test onset detection function access
    {
        auto signal = generateRhythmicSignal(120.0f, 4, 2.0f);
        analyzer->processAudioChunk(signal);  // Process some audio first

        auto result = analyzer->getOnsetDetectionFunction();
        EXPECT_TRUE(result.has_value()) << "Should access onset detection function";
    }
}

// Test 5: Tempo estimation - targeting tempo estimation paths
TEST_F(CadenceAnalyzerComprehensiveFixedTest, TempoEstimation) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test tempo estimation with known tempo
    {
        auto signal = generateRhythmicSignal(120.0f, 12, 6.0f);  // 120 BPM
        auto result = analyzer->estimateTempo(signal);
        EXPECT_TRUE(result.has_value()) << "Tempo estimation should succeed";

        if (result.has_value()) {
            const auto& [tempo, confidence] = result.value();
            EXPECT_GT(tempo, 0.0f) << "Estimated tempo should be positive";
            EXPECT_GE(confidence, 0.0f) << "Confidence should be non-negative";
            EXPECT_LE(confidence, 1.0f) << "Confidence should not exceed 1.0";

            // Should be reasonably close to 120 BPM (allow for processing variations)
            EXPECT_GT(tempo, 80.0f) << "Tempo should be reasonable";
            EXPECT_LT(tempo, 200.0f) << "Tempo should be reasonable";
        }
    }

    // Test with different tempo
    {
        auto signal = generateRhythmicSignal(80.0f, 8, 4.0f);  // 80 BPM
        auto result = analyzer->estimateTempo(signal);
        EXPECT_TRUE(result.has_value()) << "Should estimate different tempo";
    }

    // Test with aperiodic signal
    {
        auto signal = generateNoise(3.0f, 0.2f);
        auto result = analyzer->estimateTempo(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle aperiodic signals";

        if (result.has_value()) {
            const auto& [tempo, confidence] = result.value();
            // Noise typically has low confidence
            EXPECT_LT(confidence, 0.8f) << "Noise should have low tempo confidence";
        }
    }
}

// Test 6: Periodicity analysis - targeting periodicity detection paths
TEST_F(CadenceAnalyzerComprehensiveFixedTest, PeriodicityAnalysis) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test periodicity analysis with rhythmic signal
    {
        auto signal = generateRhythmicSignal(100.0f, 10, 5.0f);  // 100 BPM
        auto result = analyzer->analyzePerodicity(signal);  // Note: using the typo in the header
        EXPECT_TRUE(result.has_value()) << "Periodicity analysis should succeed";

        if (result.has_value()) {
            const auto& periodicity = result.value();
            EXPECT_GE(periodicity.periodicityStrength, 0.0f)
                << "Periodicity strength should be non-negative";
            EXPECT_LE(periodicity.periodicityStrength, 1.0f)
                << "Periodicity strength should not exceed 1.0";
            EXPECT_GT(periodicity.dominantPeriod, 0.0f) << "Dominant period should be positive";
        }
    }

    // Test with aperiodic signal
    {
        auto signal = generateNoise(3.0f, 0.3f);
        auto result = analyzer->analyzePerodicity(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle aperiodic signals";

        if (result.has_value()) {
            const auto& periodicity = result.value();
            // Noise typically has weak periodicity
            EXPECT_LT(periodicity.periodicityStrength, 0.7f)
                << "Noise should have weak periodicity";
        }
    }

    // Test with call sequence pattern
    {
        std::vector<float> call_times = {
            0.0f, 1.0f, 2.0f, 3.0f, 4.0f};  // Regular 1-second intervals
        std::vector<float> call_durations = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
        auto signal = generateCallSequence(call_times, call_durations, 5.0f);

        auto result = analyzer->analyzePerodicity(signal);
        EXPECT_TRUE(result.has_value()) << "Should detect periodicity in regular calls";
    }
}

// Test 7: Rhythmic features - targeting rhythmic feature extraction
TEST_F(CadenceAnalyzerComprehensiveFixedTest, RhythmicFeatures) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test rhythmic feature extraction
    {
        // First detect onsets
        auto signal = generateRhythmicSignal(110.0f, 8, 4.0f);
        auto onsetResult = analyzer->detectOnsets(signal);
        ASSERT_TRUE(onsetResult.has_value());

        std::vector<float> onsets = onsetResult.value();

        // Extract rhythmic features from onsets
        auto result = analyzer->extractRhythmicFeatures(onsets);
        EXPECT_TRUE(result.has_value()) << "Rhythmic feature extraction should succeed";

        if (result.has_value()) {
            const auto& features = result.value();
            EXPECT_GE(features.rhythmComplexity, 0.0f)
                << "Rhythm complexity should be non-negative";
            EXPECT_LE(features.rhythmComplexity, 1.0f) << "Rhythm complexity should not exceed 1.0";
            EXPECT_GE(features.rhythmRegularity, 0.0f)
                << "Rhythm regularity should be non-negative";
            EXPECT_LE(features.rhythmRegularity, 1.0f) << "Rhythm regularity should not exceed 1.0";
        }
    }

    // Test with empty onset vector
    {
        std::vector<float> empty_onsets;
        auto result = analyzer->extractRhythmicFeatures(empty_onsets);
        EXPECT_FALSE(result.has_value()) << "Should reject empty onset vector";
    }

    // Test with single onset
    {
        std::vector<float> single_onset = {1.0f};
        auto result = analyzer->extractRhythmicFeatures(single_onset);
        // Single onset should be handled appropriately (may succeed or fail)
    }
}

// Test 8: Configuration management - targeting updateConfig path
TEST_F(CadenceAnalyzerComprehensiveFixedTest, ConfigurationManagement) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test getting current config
    {
        const auto& config = analyzer->getConfig();
        EXPECT_EQ(config.sampleRate, standard_config.sampleRate);
        EXPECT_EQ(config.frameSize, standard_config.frameSize);
        EXPECT_EQ(config.enableBeatTracking, standard_config.enableBeatTracking);
    }

    // Test updating configuration
    {
        auto newConfig = standard_config;
        newConfig.onsetThreshold = 0.5f;
        newConfig.enableBeatTracking = false;

        auto result = analyzer->updateConfig(newConfig);
        EXPECT_TRUE(result.has_value()) << "Configuration update should succeed";

        // Verify config was updated
        const auto& updatedConfig = analyzer->getConfig();
        EXPECT_EQ(updatedConfig.onsetThreshold, 0.5f);
        EXPECT_FALSE(updatedConfig.enableBeatTracking);
    }

    // Test invalid config update
    {
        auto invalidConfig = standard_config;
        invalidConfig.sampleRate = -1.0f;  // Invalid

        auto result = analyzer->updateConfig(invalidConfig);
        EXPECT_FALSE(result.has_value()) << "Should reject invalid config";
    }
}

// Test 9: Utility and diagnostic methods - targeting additional paths
TEST_F(CadenceAnalyzerComprehensiveFixedTest, UtilityMethods) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test isActive method
    {
        bool isActive = analyzer->isActive();
        EXPECT_FALSE(isActive) << "Should not be active initially";
        // Should return some valid state (true or false)
    }

    // Test processing stats
    {
        // Process some audio to generate stats
        auto signal = generateRhythmicSignal(120.0f, 4, 2.0f);
        analyzer->processAudioChunk(signal);

        std::string stats = analyzer->getProcessingStats();
        EXPECT_FALSE(stats.empty()) << "Processing stats should not be empty";
    }

    // Test beat tracking state
    {
        auto signal = generateRhythmicSignal(100.0f, 6, 3.0f);
        analyzer->processAudioChunk(signal);

        auto result = analyzer->getBeatTrackingState();
        EXPECT_TRUE(result.has_value()) << "Should access beat tracking state";
    }

    // Test JSON export
    {
        auto signal = generateRhythmicSignal(90.0f, 4, 2.0f);
        auto analysisResult = analyzer->analyzeCadence(signal);
        ASSERT_TRUE(analysisResult.has_value());

        CadenceAnalyzer::CadenceProfile profile = analysisResult.value();
        std::string json = CadenceAnalyzer::exportToJson(profile);
        EXPECT_FALSE(json.empty()) << "JSON export should not be empty";
        EXPECT_NE(json.find("{"), std::string::npos) << "JSON should contain valid format";
    }
}

// Test 10: Edge cases and boundary conditions
TEST_F(CadenceAnalyzerComprehensiveFixedTest, EdgeCasesAndBoundaries) {
    auto analyzerResult = CadenceAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test very short audio
    {
        std::vector<float> short_audio(100, 0.1f);  // Very short signal
        auto result = analyzer->analyzeCadence(short_audio);
        EXPECT_FALSE(result.has_value()) << "Should reject very short audio";
    }

    // Test audio with NaN values
    {
        auto signal = generateRhythmicSignal(120.0f, 4, 2.0f);
        signal[signal.size() / 2] = std::numeric_limits<float>::quiet_NaN();

        auto result = analyzer->analyzeCadence(signal);
        EXPECT_FALSE(result.has_value()) << "Should reject audio with NaN values";
    }

    // Test audio with infinite values
    {
        auto signal = generateRhythmicSignal(120.0f, 4, 2.0f);
        signal[100] = std::numeric_limits<float>::infinity();

        auto result = analyzer->analyzeCadence(signal);
        EXPECT_FALSE(result.has_value()) << "Should reject audio with infinite values";
    }

    // Test very loud audio (clipping)
    {
        auto signal = generateRhythmicSignal(120.0f, 4, 2.0f);
        for (auto& sample : signal) {
            sample *= 100.0f;  // Amplify to clipping levels
        }

        auto result = analyzer->analyzeCadence(signal);
        // Should handle clipped audio appropriately (may succeed or fail)
    }

    // Test extreme tempo ranges
    {
        auto config = standard_config;
        config.minTempo = 1.0f;     // Very slow
        config.maxTempo = 1000.0f;  // Very fast

        auto extremeAnalyzer = CadenceAnalyzer::create(config);
        EXPECT_TRUE(extremeAnalyzer.has_value()) << "Should handle extreme tempo ranges";
    }

    // Test with maximum configuration values
    {
        auto config = standard_config;
        config.autocorrelationLags = 10000;  // Very large
        config.onsetThreshold = 0.99f;       // Very high threshold

        auto extremeAnalyzer = CadenceAnalyzer::create(config);
        EXPECT_TRUE(extremeAnalyzer.has_value()) << "Should handle extreme config values";
    }
}
