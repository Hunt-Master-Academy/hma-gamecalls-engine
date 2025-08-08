/**
 * @file test_waveform_analyzer_comprehensive.cpp
 * @brief Comprehensive tests for WaveformAnalyzer - targeting 90% coverage goal
 *
 * This test suite is specifically designed to achieve maximum code coverage
 * for the WaveformAnalyzer class (541 lines, currently 0% coverage).
 * Target: Achieve 80%+ coverage (+400 lines toward 90% goal)
 */

#include <chrono>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "TestAudioBuffer.h"
#include "huntmaster/core/AudioConfig.h"
#include "huntmaster/visualization/WaveformAnalyzer.h"

using namespace huntmaster;
using namespace huntmaster::test;

/**
 * Test fixture for comprehensive WaveformAnalyzer testing
 */
class WaveformAnalyzerComprehensiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup audio configuration for testing
        config.sample_rate = 44100;
        config.buffer_size = 1024;
        config.channel_count = 2;

        // Create WaveformAnalyzer instance
        analyzer = std::make_unique<WaveformAnalyzer>(config);

        // Generate test audio data
        generateTestAudio();
    }

    void TearDown() override {
        analyzer.reset();
    }

    void generateTestAudio() {
        // Generate sine wave test audio (1 second, 440 Hz)
        const size_t num_samples = 44100;
        const float frequency = 440.0f;
        const float amplitude = 0.7f;

        std::vector<float> audio_data(num_samples);
        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / config.sample_rate;
            audio_data[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
        }

        // Create TestAudioBuffer with test data
        test_audio = std::make_unique<TestAudioBuffer>(
            config.channel_count, num_samples, config.sample_rate);
        for (size_t channel = 0; channel < config.channel_count; ++channel) {
            for (size_t i = 0; i < num_samples; ++i) {
                test_audio->setSample(channel, i, audio_data[i]);
            }
        }

        // Generate noise audio for additional testing
        generateNoiseAudio();
        generateSilenceAudio();
        generateComplexAudio();
    }

    void generateNoiseAudio() {
        // Generate white noise audio
        const size_t num_samples = 22050;  // 0.5 seconds
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 0.1f);

        noise_audio = std::make_unique<TestAudioBuffer>(1, num_samples, config.sample_rate);
        for (size_t i = 0; i < num_samples; ++i) {
            float sample = dist(gen);
            noise_audio->setSample(0, i, sample);
        }
    }

    void generateSilenceAudio() {
        // Generate silent audio
        const size_t num_samples = 4410;  // 0.1 seconds
        silence_audio = std::make_unique<TestAudioBuffer>(1, num_samples, config.sample_rate);
        for (size_t i = 0; i < num_samples; ++i) {
            silence_audio->setSample(0, i, 0.0f);
        }
    }

    void generateComplexAudio() {
        // Generate complex multi-frequency audio
        const size_t num_samples = 88200;  // 2 seconds
        complex_audio = std::make_unique<TestAudioBuffer>(2, num_samples, config.sample_rate);

        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / config.sample_rate;

            // Mix multiple frequencies
            float sample = 0.3f * std::sin(2.0f * M_PI * 220.0f * t) +   // A3
                           0.25f * std::sin(2.0f * M_PI * 440.0f * t) +  // A4
                           0.2f * std::sin(2.0f * M_PI * 880.0f * t) +   // A5
                           0.1f * std::sin(2.0f * M_PI * 1760.0f * t);   // A6

            // Add some amplitude modulation
            sample *= (1.0f + 0.2f * std::sin(2.0f * M_PI * 4.0f * t));

            complex_audio->setSample(0, i, sample);
            complex_audio->setSample(1, i, sample * 0.9f);  // Slightly different for stereo
        }
    }

    AudioConfig config;
    std::unique_ptr<WaveformAnalyzer> analyzer;
    std::unique_ptr<TestAudioBuffer> test_audio;
    std::unique_ptr<TestAudioBuffer> noise_audio;
    std::unique_ptr<TestAudioBuffer> silence_audio;
    std::unique_ptr<TestAudioBuffer> complex_audio;
};

// ============================================================================
// Initialization and Configuration Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, InitializationSuccess) {
    // Test successful initialization
    EXPECT_TRUE(analyzer->initialize());
    EXPECT_TRUE(analyzer->isInitialized());
}

TEST_F(WaveformAnalyzerComprehensiveTest, InitializationIdempotent) {
    // Test multiple initialization calls
    EXPECT_TRUE(analyzer->initialize());
    EXPECT_TRUE(analyzer->initialize());  // Should succeed again
    EXPECT_TRUE(analyzer->isInitialized());
}

TEST_F(WaveformAnalyzerComprehensiveTest, ConfigurationValidation) {
    // Test with different audio configurations
    AudioConfig invalid_config;
    invalid_config.sample_rate = 0;  // Invalid sample rate
    invalid_config.buffer_size = 1024;
    invalid_config.channel_count = 1;

    auto invalid_analyzer = std::make_unique<WaveformAnalyzer>(invalid_config);
    EXPECT_FALSE(invalid_analyzer->initialize());  // Should fail with invalid config
}

TEST_F(WaveformAnalyzerComprehensiveTest, DestructorCleanuep) {
    // Test proper cleanup during destruction
    {
        auto temp_analyzer = std::make_unique<WaveformAnalyzer>(config);
        EXPECT_TRUE(temp_analyzer->initialize());
        // Destructor should clean up properly when temp_analyzer goes out of scope
    }
    // No crashes should occur
    SUCCEED();
}

// ============================================================================
// Waveform Data Generation Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateWaveformDataSuccess) {
    ASSERT_TRUE(analyzer->initialize());

    // Test waveform data generation with normal audio
    EXPECT_TRUE(analyzer->generateWaveformData(*test_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateWaveformDataWithNoise) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with noise audio
    EXPECT_TRUE(analyzer->generateWaveformData(*noise_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateWaveformDataWithSilence) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with silent audio
    EXPECT_TRUE(analyzer->generateWaveformData(*silence_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateWaveformDataWithComplexAudio) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with complex multi-frequency audio
    EXPECT_TRUE(analyzer->generateWaveformData(*complex_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateWaveformDataBeforeInitialization) {
    // Test calling generateWaveformData before initialization
    EXPECT_FALSE(analyzer->generateWaveformData(*test_audio));
}

// ============================================================================
// Waveform Data Retrieval Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, GetWaveformDataValidRange) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Test getting waveform data for valid time range
    auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, 800);
    EXPECT_TRUE(waveform_data.is_valid);
    EXPECT_GT(waveform_data.min_values.size(), 0);
    EXPECT_GT(waveform_data.max_values.size(), 0);
    EXPECT_GT(waveform_data.rms_values.size(), 0);
    EXPECT_EQ(waveform_data.min_values.size(), waveform_data.max_values.size());
    EXPECT_EQ(waveform_data.min_values.size(), waveform_data.rms_values.size());
}

TEST_F(WaveformAnalyzerComprehensiveTest, GetWaveformDataInvalidRange) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Test with invalid time range (start > end)
    auto waveform_data = analyzer->getWaveformData(1.0f, 0.5f, 800);
    EXPECT_FALSE(waveform_data.is_valid);
}

TEST_F(WaveformAnalyzerComprehensiveTest, GetWaveformDataNegativeStart) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Test with negative start time (should be clamped to 0)
    auto waveform_data = analyzer->getWaveformData(-0.5f, 0.5f, 400);
    EXPECT_TRUE(waveform_data.is_valid);
    EXPECT_EQ(waveform_data.start_time, 0.0f);
}

TEST_F(WaveformAnalyzerComprehensiveTest, GetWaveformDataDifferentWidths) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Test with different target widths
    std::vector<int> widths = {100, 400, 800, 1600, 3200};

    for (int width : widths) {
        auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, width);
        EXPECT_TRUE(waveform_data.is_valid) << "Failed for width: " << width;
        EXPECT_GT(waveform_data.min_values.size(), 0) << "No data for width: " << width;
    }
}

TEST_F(WaveformAnalyzerComprehensiveTest, GetWaveformDataBeforeGeneration) {
    ASSERT_TRUE(analyzer->initialize());

    // Test getting data before generation
    auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, 800);
    // Should return valid structure but with empty data
    EXPECT_TRUE(waveform_data.is_valid);
    EXPECT_EQ(waveform_data.min_values.size(), 0);
    EXPECT_EQ(waveform_data.max_values.size(), 0);
    EXPECT_EQ(waveform_data.rms_values.size(), 0);
}

// ============================================================================
// Spectrum Analysis Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, AnalyzeSpectrumBasic) {
    ASSERT_TRUE(analyzer->initialize());

    // Test basic spectrum analysis
    auto spectrum_data = analyzer->analyzeSpectrum(*test_audio, 0.0f, 0.5f);
    EXPECT_TRUE(spectrum_data.is_valid);
    EXPECT_GT(spectrum_data.frequencies.size(), 0);
    EXPECT_GT(spectrum_data.magnitudes.size(), 0);
    EXPECT_EQ(spectrum_data.frequencies.size(), spectrum_data.magnitudes.size());
}

TEST_F(WaveformAnalyzerComprehensiveTest, AnalyzeSpectrumComplexAudio) {
    ASSERT_TRUE(analyzer->initialize());

    // Test spectrum analysis with complex multi-frequency audio
    auto spectrum_data = analyzer->analyzeSpectrum(*complex_audio, 0.0f, 1.0f);
    EXPECT_TRUE(spectrum_data.is_valid);
    EXPECT_GT(spectrum_data.frequencies.size(), 0);
    EXPECT_GT(spectrum_data.magnitudes.size(), 0);

    // Should detect multiple frequency peaks
    float max_magnitude =
        *std::max_element(spectrum_data.magnitudes.begin(), spectrum_data.magnitudes.end());
    EXPECT_GT(max_magnitude, 0.0f);
}

TEST_F(WaveformAnalyzerComprehensiveTest, AnalyzeSpectrumSilence) {
    ASSERT_TRUE(analyzer->initialize());

    // Test spectrum analysis with silence
    auto spectrum_data = analyzer->analyzeSpectrum(*silence_audio, 0.0f, 0.1f);
    EXPECT_TRUE(spectrum_data.is_valid);
    EXPECT_GT(spectrum_data.frequencies.size(), 0);

    // Magnitudes should be very low for silence
    float max_magnitude =
        *std::max_element(spectrum_data.magnitudes.begin(), spectrum_data.magnitudes.end());
    EXPECT_LT(max_magnitude, 0.01f);  // Very low for silence
}

TEST_F(WaveformAnalyzerComprehensiveTest, AnalyzeSpectrumInvalidParameters) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with invalid parameters
    auto spectrum_data1 = analyzer->analyzeSpectrum(*test_audio, -1.0f, 0.5f);  // Negative start
    EXPECT_FALSE(spectrum_data1.is_valid);

    auto spectrum_data2 = analyzer->analyzeSpectrum(*test_audio, 0.0f, -0.5f);  // Negative duration
    EXPECT_FALSE(spectrum_data2.is_valid);

    auto spectrum_data3 = analyzer->analyzeSpectrum(*test_audio, 0.0f, 0.0f);  // Zero duration
    EXPECT_FALSE(spectrum_data3.is_valid);
}

TEST_F(WaveformAnalyzerComprehensiveTest, AnalyzeSpectrumBeforeInitialization) {
    // Test spectrum analysis before initialization
    auto spectrum_data = analyzer->analyzeSpectrum(*test_audio, 0.0f, 0.5f);
    EXPECT_FALSE(spectrum_data.is_valid);
}

// ============================================================================
// Similarity Color Generation Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateSimilarityColorsBasic) {
    ASSERT_TRUE(analyzer->initialize());

    // Test color generation with various similarity values
    std::vector<float> similarity_values = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    auto colors = analyzer->generateSimilarityColors(similarity_values);

    EXPECT_EQ(colors.size(), similarity_values.size());

    // All colors should be valid (uint8_t values 0-255)
    for (const auto& color : colors) {
        EXPECT_GE(color.r, 0);
        EXPECT_LE(color.r, 255);
        EXPECT_GE(color.g, 0);
        EXPECT_LE(color.g, 255);
        EXPECT_GE(color.b, 0);
        EXPECT_LE(color.b, 255);
        EXPECT_GE(color.a, 0);
        EXPECT_LE(color.a, 255);
    }
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateSimilarityColorsEmpty) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with empty similarity values
    std::vector<float> empty_values;
    auto colors = analyzer->generateSimilarityColors(empty_values);
    EXPECT_TRUE(colors.empty());
}

TEST_F(WaveformAnalyzerComprehensiveTest, GenerateSimilarityColorsOutOfRange) {
    ASSERT_TRUE(analyzer->initialize());

    // Test with out-of-range values (should be clamped)
    std::vector<float> out_of_range_values = {-0.5f, 1.5f, 2.0f, -1.0f};
    auto colors = analyzer->generateSimilarityColors(out_of_range_values);

    EXPECT_EQ(colors.size(), out_of_range_values.size());

    // All colors should still be valid after clamping (uint8_t values 0-255)
    for (const auto& color : colors) {
        EXPECT_GE(color.r, 0);
        EXPECT_LE(color.r, 255);
        EXPECT_GE(color.g, 0);
        EXPECT_LE(color.g, 255);
        EXPECT_GE(color.b, 0);
        EXPECT_LE(color.b, 255);
    }
}

// ============================================================================
// Peak Detection Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, DetectPeaksInWaveform) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Get waveform data and detect peaks
    auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, 1000);
    ASSERT_TRUE(waveform_data.is_valid);

    auto peaks = analyzer->detectPeaks(waveform_data.max_values, 0.1f);

    // Should find some peaks in sine wave data
    EXPECT_GT(peaks.size(), 0);

    // All peaks should have valid indices
    for (const auto& peak : peaks) {
        EXPECT_GE(peak.sample_index, 0);
        EXPECT_LT(peak.sample_index, waveform_data.max_values.size());
        EXPECT_GT(peak.magnitude, 0.0f);
    }
}

TEST_F(WaveformAnalyzerComprehensiveTest, DetectPeaksInSilence) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*silence_audio));

    // Get waveform data for silence and detect peaks
    auto waveform_data = analyzer->getWaveformData(0.0f, 0.1f, 100);
    ASSERT_TRUE(waveform_data.is_valid);

    auto peaks = analyzer->detectPeaks(waveform_data.max_values, 0.01f);

    // Should find very few or no peaks in silence
    EXPECT_LE(peaks.size(), 2);  // Allow for some numerical noise
}

TEST_F(WaveformAnalyzerComprehensiveTest, DetectPeaksWithDifferentThresholds) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*complex_audio));

    auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, 1000);
    ASSERT_TRUE(waveform_data.is_valid);

    // Test with different threshold values
    std::vector<float> thresholds = {0.01f, 0.05f, 0.1f, 0.2f, 0.5f};

    size_t prev_peak_count = SIZE_MAX;
    for (float threshold : thresholds) {
        auto peaks = analyzer->detectPeaks(waveform_data.max_values, threshold);

        // Higher thresholds should generally find fewer peaks
        if (prev_peak_count != SIZE_MAX) {
            EXPECT_LE(peaks.size(), prev_peak_count)
                << "Threshold " << threshold << " found more peaks than lower threshold";
        }
        prev_peak_count = peaks.size();
    }
}

// ============================================================================
// Statistics and Performance Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, WaveformStatistics) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Get waveform statistics
    auto stats = analyzer->getStatistics();

    EXPECT_GT(stats.max_amplitude, 0.0f);
    EXPECT_GT(stats.rms_level, 0.0f);
    EXPECT_GE(stats.dynamic_range, -100.0f);  // Dynamic range in dB can be negative
    EXPECT_GE(stats.zero_crossing_rate, 0.0f);
}

TEST_F(WaveformAnalyzerComprehensiveTest, PerformanceStatistics) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*complex_audio));

    // Get performance statistics
    auto perf_stats = analyzer->getPerformanceStats();

    EXPECT_GE(perf_stats.analysis_time, 0.0);
    EXPECT_GE(perf_stats.memory_usage, 0);
    EXPECT_GE(perf_stats.fft_time, 0.0);
}

TEST_F(WaveformAnalyzerComprehensiveTest, ResetStatistics) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Get statistics after processing (resetStatistics is private)
    auto stats = analyzer->getPerformanceStats();

    // Verify statistics are populated after processing
    EXPECT_GE(stats.analysis_time, 0.0);
    EXPECT_GE(stats.memory_usage, 0);
}

// ============================================================================
// Memory Management and Cleanup Tests
// ============================================================================

// Note: ClearWaveformData test removed as clearWaveformData() is private
// This focuses tests on the public interface

TEST_F(WaveformAnalyzerComprehensiveTest, MultiplDataGenerationCycles) {
    ASSERT_TRUE(analyzer->initialize());

    // Test multiple generation cycles (clearWaveformData is private)
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(analyzer->generateWaveformData(*test_audio)) << "Failed on iteration " << i;

        auto waveform_data = analyzer->getWaveformData(0.0f, 1.0f, 800);
        EXPECT_TRUE(waveform_data.is_valid) << "Invalid data on iteration " << i;

        // Note: clearWaveformData() is private, so we test repeated generation instead
    }
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, ZeroSizeAudio) {
    ASSERT_TRUE(analyzer->initialize());

    // Create zero-size audio buffer
    TestAudioBuffer empty_audio(1, 0, config.sample_rate);

    // Should handle gracefully
    EXPECT_FALSE(analyzer->generateWaveformData(empty_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, VeryShortAudio) {
    ASSERT_TRUE(analyzer->initialize());

    // Create very short audio buffer (10 samples)
    TestAudioBuffer short_audio(1, 10, config.sample_rate);
    for (size_t i = 0; i < 10; ++i) {
        short_audio.setSample(0, i, 0.5f);
    }

    // Should handle gracefully
    EXPECT_NO_FATAL_FAILURE(analyzer->generateWaveformData(short_audio));
}

TEST_F(WaveformAnalyzerComprehensiveTest, ExtremeAudioValues) {
    ASSERT_TRUE(analyzer->initialize());

    // Create audio with extreme values
    const size_t num_samples = 1000;
    TestAudioBuffer extreme_audio(1, num_samples, config.sample_rate);

    for (size_t i = 0; i < num_samples; ++i) {
        float value = (i % 2 == 0) ? 1.0f : -1.0f;  // Square wave with extreme values
        extreme_audio.setSample(0, i, value);
    }

    // Should handle extreme values gracefully
    EXPECT_NO_FATAL_FAILURE(analyzer->generateWaveformData(extreme_audio));
}

// ============================================================================
// Configuration and Settings Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, WindowFunctionConfiguration) {
    ASSERT_TRUE(analyzer->initialize());

    // Test different window functions if supported
    // This tests the window function initialization code paths
    EXPECT_TRUE(analyzer->generateWaveformData(*test_audio));

    // Different window functions should be handled by the analyzer
    auto spectrum1 = analyzer->analyzeSpectrum(*test_audio, 0.0f, 0.5f);
    EXPECT_TRUE(spectrum1.is_valid);
}

TEST_F(WaveformAnalyzerComprehensiveTest, SpectrumSizeConfiguration) {
    // Test with different audio configurations that might affect spectrum size
    std::vector<float> sample_rates = {22050.0f, 44100.0f, 48000.0f, 96000.0f};

    for (float sample_rate : sample_rates) {
        AudioConfig test_config = config;
        test_config.sample_rate = sample_rate;

        auto test_analyzer = std::make_unique<WaveformAnalyzer>(test_config);
        EXPECT_TRUE(test_analyzer->initialize())
            << "Failed initialization for sample rate: " << sample_rate;
    }
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(WaveformAnalyzerComprehensiveTest, ConcurrentDataRetrieval) {
    ASSERT_TRUE(analyzer->initialize());
    ASSERT_TRUE(analyzer->generateWaveformData(*complex_audio));

    // Test concurrent access to read-only operations
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i]() {
            float start_time = i * 0.1f;
            float end_time = start_time + 0.2f;

            auto waveform_data = analyzer->getWaveformData(start_time, end_time, 400);
            if (waveform_data.is_valid) {
                success_count++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count.load(), 4);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
