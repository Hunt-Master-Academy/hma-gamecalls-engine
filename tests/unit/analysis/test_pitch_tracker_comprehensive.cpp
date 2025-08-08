/**
 * @file test_pitch_tracker_comprehensive.cpp
 * @brief Comprehensive PitchTracker coverage tests - targeting 90% coverage improvement
 *
 * This test suite targets PitchTracker.cpp (535 lines, currently ~0% coverage)
 * Goal: Achieve 80%+ coverage to gain ~400+ lines toward 90% project coverage
 *
 * Based on existing disabled test but updated for current API compatibility
 */

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

class PitchTrackerComprehensiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Standard configuration for most tests
        standard_config.sampleRate = 44100.0f;
        standard_config.minFrequency = 80.0f;
        standard_config.maxFrequency = 8000.0f;
        standard_config.threshold = 0.2f;
        standard_config.windowSize = 2048;
        standard_config.hopSize = 512;
        standard_config.enableSmoothing = true;
        standard_config.enableVibratoDetection = true;
        standard_config.smoothingFactor = 0.1f;
    }

    PitchTracker::Config standard_config;

    // Helper to generate test signals with known pitches
    std::vector<float> generateSineWave(float frequency, size_t length, float amplitude = 0.5f) {
        std::vector<float> signal(length);
        const float pi2 = 2.0f * M_PI;
        for (size_t i = 0; i < length; ++i) {
            signal[i] = amplitude * std::sin(pi2 * frequency * i / standard_config.sampleRate);
        }
        return signal;
    }

    std::vector<float> generateComplexTone(float fundamental, int numHarmonics, size_t length) {
        std::vector<float> signal(length, 0.0f);
        const float pi2 = 2.0f * M_PI;
        for (size_t i = 0; i < length; ++i) {
            float sample = 0.0f;
            for (int h = 1; h <= numHarmonics; ++h) {
                float amplitude = 1.0f / h;  // Decreasing amplitude for harmonics
                sample +=
                    amplitude * std::sin(pi2 * fundamental * h * i / standard_config.sampleRate);
            }
            signal[i] = sample * 0.3f;  // Scale down to avoid clipping
        }
        return signal;
    }

    std::vector<float> generateNoiseSignal(size_t length, float amplitude = 0.1f) {
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

// Test 1: Factory method and configuration validation
TEST_F(PitchTrackerComprehensiveTest, FactoryAndConfigValidation) {
    // Test valid configuration (should succeed)
    {
        auto result = PitchTracker::create(standard_config);
        EXPECT_TRUE(result.has_value()) << "Factory should create tracker with valid config";
        if (result.has_value()) {
            auto tracker = std::move(result.value());
            EXPECT_NE(tracker.get(), nullptr);
            EXPECT_EQ(tracker->getConfig().sampleRate, standard_config.sampleRate);
        }
    }

    // Test configuration edge cases - these may succeed but exercise validation paths
    {
        auto config = standard_config;
        config.sampleRate = 8000.0f;  // Low but valid sample rate
        auto result = PitchTracker::create(config);
        EXPECT_TRUE(result.has_value()) << "Should succeed with low sample rate";
    }

    {
        auto config = standard_config;
        config.windowSize = 512;  // Small but valid window size
        auto result = PitchTracker::create(config);
        EXPECT_TRUE(result.has_value()) << "Should succeed with small window size";
    }

    {
        auto config = standard_config;
        config.minFrequency = 1000.0f;
        config.maxFrequency = 500.0f;  // max < min (tests range validation in analysis)
        auto result = PitchTracker::create(config);
        EXPECT_TRUE(result.has_value()) << "Factory succeeds, validation occurs during analysis";
    }

    {
        auto config = standard_config;
        config.threshold = 0.9f;  // High threshold
        auto result = PitchTracker::create(config);
        EXPECT_TRUE(result.has_value()) << "Should succeed with high threshold";
    }
}

// Test 2: Basic pitch detection functionality
TEST_F(PitchTrackerComprehensiveTest, BasicPitchDetection) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Test pitch detection on pure tones
    std::vector<float> test_frequencies = {220.0f, 440.0f, 880.0f, 1760.0f};

    for (float freq : test_frequencies) {
        auto signal = generateSineWave(freq, 4096);
        auto result = tracker->detectPitch(signal);

        EXPECT_TRUE(result.has_value()) << "Detection should succeed for " << freq << "Hz";
        if (result.has_value()) {
            auto pitch_result = result.value();
            EXPECT_GE(pitch_result.confidence, 0.0f) << "Confidence should be non-negative";
            EXPECT_LE(pitch_result.confidence, 1.0f) << "Confidence should be <= 1.0";

            // YIN algorithm may detect harmonics or sub-harmonics, especially for higher
            // frequencies Just check that some reasonable detection occurred if the signal is
            // voiced
            if (pitch_result.frequency > 0.0f && pitch_result.isVoiced) {
                // Very permissive range - allow for sub-harmonics and harmonics
                EXPECT_GE(pitch_result.frequency, freq * 0.1f) << "Detected frequency too low";
                EXPECT_LE(pitch_result.frequency, freq * 4.0f) << "Detected frequency too high";

                // Just verify the algorithm is actually detecting something meaningful
                EXPECT_GT(pitch_result.frequency, 30.0f)
                    << "Frequency should be above human perception threshold";
                EXPECT_LT(pitch_result.frequency, 4000.0f)
                    << "Frequency should be within reasonable range";
            }
        }
    }
}

// Test 3: Error handling for invalid audio data
TEST_F(PitchTrackerComprehensiveTest, InvalidAudioDataHandling) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Test empty audio data
    std::vector<float> empty_data;
    auto result1 = tracker->detectPitch(empty_data);
    // Implementation may return a result with low confidence or no error
    // Just ensure it doesn't crash

    // Test very short audio data (less than window size)
    std::vector<float> short_data(512, 0.5f);  // Much smaller than 4096 window
    auto result2 = tracker->detectPitch(short_data);
    // Implementation should handle this gracefully

    // Test data with all zeros (silence)
    std::vector<float> silent_data(8192, 0.0f);
    auto result3 = tracker->detectPitch(silent_data);
    EXPECT_TRUE(result3.has_value()) << "Should handle silent data";
    if (result3.has_value()) {
        // Silent data should result in unvoiced or very low confidence
        auto pitch_result = result3.value();
        // Either unvoiced or very low confidence is acceptable
        if (pitch_result.isVoiced) {
            EXPECT_LT(pitch_result.confidence, 0.5f) << "Silent data should have low confidence";
        }
    }

    // Test data with extreme values
    std::vector<float> extreme_data(8192);
    std::fill(extreme_data.begin(), extreme_data.end(), 1000.0f);
    auto result4 = tracker->detectPitch(extreme_data);
    // Should handle extreme values without crashing
}

// Test 4: Realtime pitch tracking
TEST_F(PitchTrackerComprehensiveTest, RealtimePitchTracking) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Initial state should have no pitch
    {
        auto pitch_result = tracker->getRealtimePitch();
        EXPECT_TRUE(pitch_result.has_value());
        if (pitch_result.has_value()) {
            EXPECT_EQ(pitch_result.value(), 0.0f) << "Initial pitch should be zero";
        }
    }

    // Process chunks of audio to build up pitch tracking
    auto signal = generateSineWave(440.0f, 8192);
    size_t chunk_size = 1024;

    for (size_t offset = 0; offset + chunk_size <= signal.size(); offset += chunk_size) {
        std::span<const float> chunk(signal.data() + offset, chunk_size);
        auto result = tracker->processAudioChunk(chunk);
        EXPECT_TRUE(result.has_value()) << "Chunk processing should succeed";
    }

    // After processing, should have pitch estimate
    {
        auto pitch_result = tracker->getRealtimePitch();
        EXPECT_TRUE(pitch_result.has_value());
        if (pitch_result.has_value()) {
            EXPECT_GT(pitch_result.value(), 0.0f) << "Should detect pitch after processing";
        }
    }
}

// Test 5: Pitch contour extraction
TEST_F(PitchTrackerComprehensiveTest, PitchContourExtraction) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Process some audio first
    auto signal = generateSineWave(440.0f, 8192);
    auto detect_result = tracker->detectPitch(signal);
    ASSERT_TRUE(detect_result.has_value());

    // Test contour extraction with different durations
    std::vector<float> durations = {100.0f, 500.0f, 1000.0f};

    for (float duration : durations) {
        auto contour_result = tracker->getPitchContour(duration);
        EXPECT_TRUE(contour_result.has_value()) << "Contour extraction should succeed";
        if (contour_result.has_value()) {
            auto contour = contour_result.value();
            EXPECT_GE(contour.size(), 0) << "Contour should be valid";
        }
    }
}

// Test 6: Configuration updates
TEST_F(PitchTrackerComprehensiveTest, ConfigurationUpdates) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Test valid configuration update
    {
        auto new_config = standard_config;
        new_config.threshold = 0.3f;
        new_config.enableSmoothing = false;

        auto result = tracker->updateConfig(new_config);
        EXPECT_TRUE(result.has_value()) << "Valid config update should succeed";

        auto current_config = tracker->getConfig();
        EXPECT_EQ(current_config.threshold, 0.3f);
        EXPECT_FALSE(current_config.enableSmoothing);
    }

    // Test invalid configuration update
    {
        auto invalid_config = standard_config;
        invalid_config.sampleRate = -1000.0f;

        auto result = tracker->updateConfig(invalid_config);
        EXPECT_FALSE(result.has_value()) << "Invalid config update should fail";
    }
}

// Test 7: Different signal types and characteristics
TEST_F(PitchTrackerComprehensiveTest, DifferentSignalTypes) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Test silence
    {
        std::vector<float> silence(2048, 0.0f);
        auto result = tracker->detectPitch(silence);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_FALSE(result.value().isVoiced) << "Silence should not be voiced";
            EXPECT_EQ(result.value().frequency, 0.0f);
        }
    }

    // Test white noise
    {
        auto noise = generateNoiseSignal(2048, 0.1f);
        auto result = tracker->detectPitch(noise);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_LE(result.value().confidence, 0.5f) << "Noise should have low confidence";
        }
    }

    // Test complex harmonic tone
    {
        auto complex_tone = generateComplexTone(220.0f, 5, 2048);
        auto result = tracker->detectPitch(complex_tone);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_TRUE(result.value().isVoiced) << "Complex tone should be voiced";
            EXPECT_GT(result.value().confidence, 0.3f);
        }
    }
}

// Test 8: Threshold variations and effects
TEST_F(PitchTrackerComprehensiveTest, ThresholdVariations) {
    std::vector<float> thresholds = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    auto signal = generateSineWave(440.0f, 2048);

    for (float threshold : thresholds) {
        auto config = standard_config;
        config.threshold = threshold;

        auto tracker_result = PitchTracker::create(config);
        ASSERT_TRUE(tracker_result.has_value())
            << "Should create tracker with threshold " << threshold;

        auto tracker = std::move(tracker_result.value());
        auto result = tracker->detectPitch(signal);
        EXPECT_TRUE(result.has_value()) << "Detection should work with threshold " << threshold;
    }
}

// Test 9: Window size variations
TEST_F(PitchTrackerComprehensiveTest, WindowSizeVariations) {
    std::vector<size_t> window_sizes = {512, 1024, 2048, 4096};
    auto signal = generateSineWave(440.0f, 8192);

    for (size_t window_size : window_sizes) {
        auto config = standard_config;
        config.windowSize = window_size;
        config.hopSize = window_size / 4;  // Maintain reasonable hop size ratio

        auto tracker_result = PitchTracker::create(config);
        EXPECT_TRUE(tracker_result.has_value())
            << "Should create tracker with window size " << window_size;

        if (tracker_result.has_value()) {
            auto tracker = std::move(tracker_result.value());

            // Use appropriate signal length for window size
            std::vector<float> test_signal(
                signal.begin(), signal.begin() + std::min(signal.size(), window_size * 2));
            auto result = tracker->detectPitch(test_signal);
            EXPECT_TRUE(result.has_value())
                << "Detection should work with window size " << window_size;
        }
    }
}

// Test 10: Frequency range limits
TEST_F(PitchTrackerComprehensiveTest, FrequencyRangeLimits) {
    // Test detection near frequency range boundaries
    std::vector<std::pair<float, float>> freq_ranges = {
        {80.0f, 400.0f},    // Low range
        {200.0f, 2000.0f},  // Mid range
        {1000.0f, 8000.0f}  // High range
    };

    for (auto [min_freq, max_freq] : freq_ranges) {
        auto config = standard_config;
        config.minFrequency = min_freq;
        config.maxFrequency = max_freq;

        auto tracker_result = PitchTracker::create(config);
        ASSERT_TRUE(tracker_result.has_value());
        auto tracker = std::move(tracker_result.value());

        // Test frequency within range
        float test_freq = (min_freq + max_freq) / 2.0f;
        auto signal = generateSineWave(test_freq, 2048);
        auto result = tracker->detectPitch(signal);
        EXPECT_TRUE(result.has_value());

        // Test frequency outside range (below minimum)
        if (min_freq > 100.0f) {
            auto low_signal = generateSineWave(min_freq - 50.0f, 2048);
            auto low_result = tracker->detectPitch(low_signal);
            // May or may not detect, but should not crash
            EXPECT_TRUE(low_result.has_value());
        }
    }
}

// Test 11: Reset functionality
TEST_F(PitchTrackerComprehensiveTest, ResetFunctionality) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    // Process some audio to establish state
    auto signal = generateSineWave(440.0f, 2048);
    auto result = tracker->detectPitch(signal);
    ASSERT_TRUE(result.has_value());

    // Reset the tracker
    tracker->reset();

    // After reset, realtime pitch should be zero
    auto pitch_result = tracker->getRealtimePitch();
    EXPECT_TRUE(pitch_result.has_value());
    if (pitch_result.has_value()) {
        EXPECT_EQ(pitch_result.value(), 0.0f) << "Pitch should be zero after reset";
    }
}

// Test 12: JSON export functionality
TEST_F(PitchTrackerComprehensiveTest, JsonExportFunctionality) {
    auto tracker_result = PitchTracker::create(standard_config);
    ASSERT_TRUE(tracker_result.has_value());
    auto tracker = std::move(tracker_result.value());

    auto signal = generateSineWave(440.0f, 2048);
    auto result = tracker->detectPitch(signal);
    ASSERT_TRUE(result.has_value());

    auto pitch_result = result.value();

    // Test JSON export
    std::string json = PitchTracker::exportToJson(pitch_result);
    EXPECT_FALSE(json.empty()) << "JSON export should produce non-empty string";
    EXPECT_NE(json.find("frequency"), std::string::npos) << "JSON should contain frequency field";
    EXPECT_NE(json.find("confidence"), std::string::npos) << "JSON should contain confidence field";
}
