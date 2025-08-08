#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/SpectrogramProcessor.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;
using namespace huntmaster::test;

class SpectrogramProcessorTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        config_.window_size = 1024;
        config_.hop_size = 512;
        config_.sample_rate = 44100.0f;
        config_.db_floor = -80.0f;

        auto processor_result = SpectrogramProcessor::create(config_);
        ASSERT_TRUE(processor_result.has_value());
        processor_ = std::move(processor_result.value());
    }

    void TearDown() override {
        processor_.reset();
        TestFixtureBase::TearDown();
    }

    SpectrogramProcessor::Config config_;
    std::unique_ptr<SpectrogramProcessor> processor_;

    // Helper to generate synthetic test audio
    std::vector<float>
    generateSineWave(float frequency, float duration_sec, float amplitude = 1.0f) {
        size_t num_samples = static_cast<size_t>(duration_sec * config_.sample_rate);
        std::vector<float> audio(num_samples);

        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / config_.sample_rate;
            audio[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
        }

        return audio;
    }

    // Helper to generate multi-frequency test signal
    std::vector<float> generateMultiTone(const std::vector<float>& frequencies,
                                         float duration_sec,
                                         float amplitude = 1.0f) {
        size_t num_samples = static_cast<size_t>(duration_sec * config_.sample_rate);
        std::vector<float> audio(num_samples, 0.0f);

        for (float freq : frequencies) {
            auto sine = generateSineWave(freq, duration_sec, amplitude / frequencies.size());
            for (size_t i = 0; i < num_samples; ++i) {
                audio[i] += sine[i];
            }
        }

        return audio;
    }
};

TEST_F(SpectrogramProcessorTest, InitializationTest) {
    // Test valid configuration
    EXPECT_TRUE(processor_ != nullptr);
    EXPECT_EQ(config_.window_size, 1024);
    EXPECT_EQ(config_.hop_size, 512);
    EXPECT_EQ(config_.sample_rate, 44100.0f);
    EXPECT_EQ(config_.db_floor, -80.0f);

    // Test configuration validation
    SpectrogramProcessor::Config validConfig = config_;
    EXPECT_TRUE(validConfig.isValid());
    auto validResult = SpectrogramProcessor::create(validConfig);
    EXPECT_TRUE(validResult.has_value());

    // Test invalid configurations
    SpectrogramProcessor::Config invalidConfig1 = config_;
    invalidConfig1.window_size = 0;  // Invalid window size
    EXPECT_FALSE(invalidConfig1.isValid());
    auto invalidResult1 = SpectrogramProcessor::create(invalidConfig1);
    EXPECT_FALSE(invalidResult1.has_value());
    EXPECT_EQ(invalidResult1.error(), SpectrogramError::INVALID_CONFIG);

    SpectrogramProcessor::Config invalidConfig2 = config_;
    invalidConfig2.hop_size = 0;  // Invalid hop size
    EXPECT_FALSE(invalidConfig2.isValid());
    auto invalidResult2 = SpectrogramProcessor::create(invalidConfig2);
    EXPECT_FALSE(invalidResult2.has_value());
    EXPECT_EQ(invalidResult2.error(), SpectrogramError::INVALID_CONFIG);

    SpectrogramProcessor::Config invalidConfig3 = config_;
    invalidConfig3.sample_rate = -1.0f;  // Invalid sample rate
    EXPECT_FALSE(invalidConfig3.isValid());
    auto invalidResult3 = SpectrogramProcessor::create(invalidConfig3);
    EXPECT_FALSE(invalidResult3.has_value());
    EXPECT_EQ(invalidResult3.error(), SpectrogramError::INVALID_CONFIG);
}

TEST_F(SpectrogramProcessorTest, SilenceProcessingTest) {
    // Create silent audio (all zeros)
    std::vector<float> silentAudio(4096, 0.0f);

    auto result = processor_->computeSpectrogram(silentAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogramData = result.value();
    EXPECT_GT(spectrogramData.magnitude_db.size(), 0);
    EXPECT_GT(spectrogramData.time_axis.size(), 0);
    EXPECT_GT(spectrogramData.frequency_axis.size(), 0);
    EXPECT_EQ(spectrogramData.time_bins, spectrogramData.magnitude_db.size());

    // For silence, all magnitude values should be at or near the floor
    for (const auto& timeSlice : spectrogramData.magnitude_db) {
        for (float magnitude : timeSlice) {
            EXPECT_LE(magnitude, config_.db_floor + 10.0f);  // Allow some numerical tolerance
        }
    }
}

TEST_F(SpectrogramProcessorTest, SingleToneProcessingTest) {
    // Generate a 1kHz sine wave
    float test_frequency = 1000.0f;
    auto testAudio = generateSineWave(test_frequency, 0.1f);  // 100ms of audio

    auto result = processor_->computeSpectrogram(testAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogramData = result.value();
    EXPECT_GT(spectrogramData.magnitude_db.size(), 0);
    EXPECT_GT(spectrogramData.frequency_axis.size(), 0);

    // Find the expected frequency bin
    float frequency_resolution = config_.sample_rate / config_.window_size;
    size_t expected_bin = static_cast<size_t>(test_frequency / frequency_resolution);

    // Check that the expected frequency bin has higher magnitude than neighboring bins
    bool found_peak = false;
    for (const auto& timeSlice : spectrogramData.magnitude_db) {
        if (expected_bin < timeSlice.size() && expected_bin > 2
            && expected_bin < timeSlice.size() - 2) {
            float peak_magnitude = timeSlice[expected_bin];
            float neighbor_avg = (timeSlice[expected_bin - 2] + timeSlice[expected_bin + 2]) / 2.0f;

            if (peak_magnitude > neighbor_avg + 10.0f) {  // At least 10dB above neighbors
                found_peak = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_peak) << "Expected to find a peak at " << test_frequency << "Hz";
}

TEST_F(SpectrogramProcessorTest, MultiToneProcessingTest) {
    // Generate audio with multiple frequency components
    std::vector<float> frequencies = {440.0f, 880.0f, 1760.0f};  // A4, A5, A6
    auto testAudio = generateMultiTone(frequencies, 0.1f);

    auto result = processor_->computeSpectrogram(testAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogramData = result.value();
    EXPECT_GT(spectrogramData.magnitude_db.size(), 0);

    float frequency_resolution = config_.sample_rate / config_.window_size;

    // Check for peaks at each expected frequency
    for (float freq : frequencies) {
        size_t expected_bin = static_cast<size_t>(freq / frequency_resolution);
        bool found_peak = false;

        for (const auto& timeSlice : spectrogramData.magnitude_db) {
            if (expected_bin < timeSlice.size() && expected_bin > 2
                && expected_bin < timeSlice.size() - 2) {
                float peak_magnitude = timeSlice[expected_bin];
                float neighbor_avg =
                    (timeSlice[expected_bin - 2] + timeSlice[expected_bin + 2]) / 2.0f;

                if (peak_magnitude > neighbor_avg + 5.0f) {  // At least 5dB above neighbors
                    found_peak = true;
                    break;
                }
            }
        }
        EXPECT_TRUE(found_peak) << "Expected to find a peak at " << freq << "Hz";
    }
}

TEST_F(SpectrogramProcessorTest, ProcessFrameTest) {
    // Test real-time frame processing
    auto testAudio = generateSineWave(1000.0f, 0.05f);  // 50ms of audio

    // Process in chunks
    size_t frame_size = config_.window_size;
    std::vector<std::vector<float>> frame_results;

    for (size_t i = 0; i + frame_size <= testAudio.size(); i += config_.hop_size) {
        std::span<const float> frame(testAudio.data() + i, frame_size);
        auto result = processor_->processFrame(frame);

        ASSERT_TRUE(result.has_value());
        frame_results.push_back(result.value());
    }

    EXPECT_GT(frame_results.size(), 0);

    // Check that all frames have the expected size
    for (const auto& frame_spectrum : frame_results) {
        EXPECT_EQ(frame_spectrum.size(), config_.window_size / 2 + 1);

        // Check for valid dB values
        for (float magnitude : frame_spectrum) {
            EXPECT_GE(magnitude, config_.db_floor);
            EXPECT_LE(magnitude, 100.0f);  // Reasonable upper bound
            EXPECT_TRUE(std::isfinite(magnitude));
        }
    }
}

TEST_F(SpectrogramProcessorTest, MagnitudeToDecibelsStaticTest) {
    // Test the static utility method
    std::vector<float> magnitudes = {0.0f, 0.1f, 0.5f, 1.0f, 2.0f, 10.0f};
    float floor_db = -60.0f;

    auto db_values = SpectrogramProcessor::magnitudeToDecibels(magnitudes, floor_db);

    ASSERT_EQ(db_values.size(), magnitudes.size());

    // Test known conversions
    EXPECT_EQ(db_values[0], floor_db);                        // 0 magnitude should be floor
    EXPECT_FLOAT_EQ(db_values[3], 0.0f);                      // Magnitude 1.0 should be 0 dB
    EXPECT_FLOAT_EQ(db_values[4], 20.0f * std::log10(2.0f));  // Magnitude 2.0
    EXPECT_FLOAT_EQ(db_values[5], 20.0f);                     // Magnitude 10.0 should be 20 dB

    // All values should be finite and above floor
    for (float db : db_values) {
        EXPECT_TRUE(std::isfinite(db));
        EXPECT_GE(db, floor_db);
    }
}

TEST_F(SpectrogramProcessorTest, GenerateColorMapTest) {
    // Create test spectrogram data
    SpectrogramData testData;
    testData.time_bins = 3;
    testData.frequency_bins = 3;  // Add missing frequency_bins
    testData.magnitude_db.resize(3);
    testData.magnitude_db[0] = {-60.0f, -40.0f, -20.0f};
    testData.magnitude_db[1] = {-50.0f, -30.0f, -10.0f};
    testData.magnitude_db[2] = {-40.0f, -20.0f, 0.0f};
    testData.min_db = -60.0f;
    testData.max_db = 0.0f;

    auto colorMap = SpectrogramProcessor::generateColorMap(testData);

    ASSERT_EQ(colorMap.size(), 3);
    ASSERT_EQ(colorMap[0].size(), 3);

    // Check that values are normalized between 0 and 1
    for (const auto& timeSlice : colorMap) {
        for (float colorValue : timeSlice) {
            EXPECT_GE(colorValue, 0.0f);
            EXPECT_LE(colorValue, 1.0f);
            EXPECT_TRUE(std::isfinite(colorValue));
        }
    }

    // Check that the mapping is monotonic (higher dB -> higher color value)
    EXPECT_LT(colorMap[0][0], colorMap[0][1]);  // -60 < -40
    EXPECT_LT(colorMap[0][1], colorMap[0][2]);  // -40 < -20
    EXPECT_LT(colorMap[2][0], colorMap[2][2]);  // -40 < 0
}

TEST_F(SpectrogramProcessorTest, ExportForVisualizationTest) {
    // Generate test audio and compute spectrogram
    auto testAudio = generateSineWave(1000.0f, 0.1f);
    auto result = processor_->computeSpectrogram(testAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogramData = result.value();

    // Test JSON export
    std::string json = processor_->exportForVisualization(spectrogramData, 50, 256);

    EXPECT_GT(json.length(), 0);

    // Basic JSON structure validation
    EXPECT_TRUE(json.find("\"magnitude_db\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"time_axis\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"frequency_axis\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"min_db\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"max_db\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"color_map\"") != std::string::npos);

    // Test with limits (should not crash)
    std::string limitedJson = processor_->exportForVisualization(spectrogramData, 10, 32);
    EXPECT_GT(limitedJson.length(), 0);

    // Test with no limits
    std::string unlimitedJson = processor_->exportForVisualization(spectrogramData, 0, 0);
    EXPECT_GT(unlimitedJson.length(), 0);
}

TEST_F(SpectrogramProcessorTest, ErrorHandlingTest) {
    // Test with empty audio
    std::vector<float> emptyAudio;
    auto result = processor_->computeSpectrogram(emptyAudio);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SpectrogramError::INVALID_INPUT);

    // Test processFrame with wrong size
    std::vector<float> wrongSizeFrame(512);  // Should be window_size (1024)
    auto frameResult = processor_->processFrame(wrongSizeFrame);
    EXPECT_FALSE(frameResult.has_value());
    EXPECT_EQ(frameResult.error(), SpectrogramError::INVALID_INPUT);

    // Test with audio too short for even one frame
    std::vector<float> tooShortAudio(100);
    auto shortResult = processor_->computeSpectrogram(tooShortAudio);
    EXPECT_FALSE(shortResult.has_value());
    EXPECT_EQ(shortResult.error(), SpectrogramError::INVALID_INPUT);
}

TEST_F(SpectrogramProcessorTest, EdgeCaseTest) {
    // Test with exactly one frame of audio
    auto oneFrameAudio =
        generateSineWave(1000.0f, static_cast<float>(config_.window_size) / config_.sample_rate);

    auto result = processor_->computeSpectrogram(oneFrameAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogramData = result.value();
    EXPECT_EQ(spectrogramData.time_bins, 1);
    EXPECT_EQ(spectrogramData.magnitude_db.size(), 1);
    EXPECT_EQ(spectrogramData.magnitude_db[0].size(), config_.window_size / 2 + 1);

    // Test with very low amplitude signal (near noise floor)
    auto lowAmpAudio = generateSineWave(1000.0f, 0.1f, 1e-6f);
    auto lowAmpResult = processor_->computeSpectrogram(lowAmpAudio);
    ASSERT_TRUE(lowAmpResult.has_value());

    // Should still produce valid data, even if all values are near the floor
    const auto& lowAmpData = lowAmpResult.value();
    EXPECT_GT(lowAmpData.magnitude_db.size(), 0);
    for (const auto& timeSlice : lowAmpData.magnitude_db) {
        for (float magnitude : timeSlice) {
            EXPECT_TRUE(std::isfinite(magnitude));
            EXPECT_GE(magnitude, config_.db_floor);
        }
    }
}

TEST_F(SpectrogramProcessorTest, ConfigAccessTest) {
    const auto& retrievedConfig = processor_->getConfig();

    EXPECT_EQ(retrievedConfig.window_size, config_.window_size);
    EXPECT_EQ(retrievedConfig.hop_size, config_.hop_size);
    EXPECT_EQ(retrievedConfig.sample_rate, config_.sample_rate);
    EXPECT_EQ(retrievedConfig.db_floor, config_.db_floor);
    EXPECT_EQ(retrievedConfig.db_ceiling, config_.db_ceiling);
    EXPECT_EQ(retrievedConfig.apply_window, config_.apply_window);
}

// Test class for utility methods (separate from main processor tests)
class SpectrogramUtilityTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // No setup needed for static utility tests
    }
};

TEST_F(SpectrogramUtilityTest, MagnitudeToDecibelsBoundaryTest) {
    // Test edge cases for magnitudeToDecibels static method
    std::vector<float> testMagnitudes = {
        0.0f,                                    // Zero
        1e-10f,                                  // Very small positive
        1e-6f,                                   // Small positive
        0.001f,                                  // Moderate
        1.0f,                                    // Unity
        1000.0f,                                 // Large
        std::numeric_limits<float>::infinity(),  // Infinity
        std::numeric_limits<float>::quiet_NaN()  // NaN
    };

    float floor_db = -80.0f;
    auto db_values = SpectrogramProcessor::magnitudeToDecibels(testMagnitudes, floor_db);

    ASSERT_EQ(db_values.size(), testMagnitudes.size());

    // Zero should map to floor
    EXPECT_EQ(db_values[0], floor_db);

    // Very small values should map to floor
    EXPECT_EQ(db_values[1], floor_db);

    // Unity should map to 0 dB
    EXPECT_FLOAT_EQ(db_values[4], 0.0f);

    // Large value test
    EXPECT_FLOAT_EQ(db_values[5], 20.0f * std::log10(1000.0f));

    // All finite values should be >= floor and finite
    for (size_t i = 0; i < db_values.size() - 2; ++i) {  // Exclude inf and nan
        EXPECT_TRUE(std::isfinite(db_values[i]));
        EXPECT_GE(db_values[i], floor_db);
    }

    // Infinity and NaN should be handled gracefully
    EXPECT_TRUE(std::isfinite(db_values[6]) || db_values[6] == floor_db);
    EXPECT_TRUE(std::isfinite(db_values[7]) || db_values[7] == floor_db);
}

TEST_F(SpectrogramUtilityTest, GenerateColorMapBoundaryTest) {
    // Test color map generation with edge cases
    SpectrogramData edgeData;

    // Test with single value
    edgeData.time_bins = 1;
    edgeData.magnitude_db.resize(1);
    edgeData.magnitude_db[0] = {-30.0f};
    edgeData.min_db = -30.0f;
    edgeData.max_db = -30.0f;  // Same min and max

    auto colorMap1 = SpectrogramProcessor::generateColorMap(edgeData);
    ASSERT_EQ(colorMap1.size(), 1);
    ASSERT_EQ(colorMap1[0].size(), 1);
    EXPECT_GE(colorMap1[0][0], 0.0f);
    EXPECT_LE(colorMap1[0][0], 1.0f);

    // Test with wide dynamic range
    edgeData.time_bins = 2;
    edgeData.magnitude_db.resize(2);
    edgeData.magnitude_db[0] = {-100.0f, 50.0f};
    edgeData.magnitude_db[1] = {-100.0f, 50.0f};
    edgeData.min_db = -100.0f;
    edgeData.max_db = 50.0f;

    auto colorMap2 = SpectrogramProcessor::generateColorMap(edgeData);
    ASSERT_EQ(colorMap2.size(), 2);
    ASSERT_EQ(colorMap2[0].size(), 2);

    // Min value should map to 0, max value should map to 1
    EXPECT_FLOAT_EQ(colorMap2[0][0], 0.0f);
    EXPECT_FLOAT_EQ(colorMap2[0][1], 1.0f);
    EXPECT_FLOAT_EQ(colorMap2[1][0], 0.0f);
    EXPECT_FLOAT_EQ(colorMap2[1][1], 1.0f);
}
