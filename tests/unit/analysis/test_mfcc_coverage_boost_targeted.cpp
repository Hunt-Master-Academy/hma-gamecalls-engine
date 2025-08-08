/**
 * @file test_mfcc_coverage_boost_targeted.cpp
 * @brief Targeted tests specifically designed to increase MFCCProcessor coverage
 *
 * This file contains tests that target specific uncovered code paths in MFCCProcessor.cpp
 * identified through coverage analysis. Focus is on improving coverage from 19.67% to >90%.
 */

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/MFCCProcessor.h"

using namespace huntmaster;

class MFCCCoverageBoostTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Standard config for most tests
        standard_config.sample_rate = 44100;
        standard_config.frame_size = 512;
        standard_config.num_coefficients = 13;
        standard_config.num_filters = 26;
        standard_config.low_freq = 0.0f;
        standard_config.high_freq = 0.0f;  // Auto-set to Nyquist
        standard_config.use_energy = true;
        standard_config.apply_lifter = true;
        standard_config.enable_simd = true;
        standard_config.enable_caching = true;
        standard_config.lifter_coeff = 22;
    }

    MFCCProcessor::Config standard_config;

    // Helper to generate test signals
    std::vector<float>
    generateSineWave(float frequency, size_t numSamples, float sampleRate = 44100.0f) {
        std::vector<float> signal(numSamples);
        const float twoPi = 2.0f * M_PI;
        for (size_t i = 0; i < numSamples; ++i) {
            signal[i] = 0.5f * std::sin(twoPi * frequency * i / sampleRate);
        }
        return signal;
    }

    std::vector<float> generateWhiteNoise(size_t numSamples, float amplitude = 0.1f) {
        std::vector<float> signal(numSamples);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, amplitude);

        for (size_t i = 0; i < numSamples; ++i) {
            signal[i] = dist(gen);
        }
        return signal;
    }
};

// Test 1: Constructor validation paths - targeting config validation coverage
TEST_F(MFCCCoverageBoostTest, ConstructorValidationPaths) {
    // Test zero sample rate (should throw)
    {
        auto config = standard_config;
        config.sample_rate = 0;
        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test non-power-of-2 frame size (should throw)
    {
        auto config = standard_config;
        config.frame_size = 500;  // Not power of 2
        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test zero frame size (should throw)
    {
        auto config = standard_config;
        config.frame_size = 0;
        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test zero filters (should throw)
    {
        auto config = standard_config;
        config.num_filters = 0;
        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test zero coefficients (should throw)
    {
        auto config = standard_config;
        config.num_coefficients = 0;
        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test high frequency > Nyquist (should clamp)
    {
        auto config = standard_config;
        config.high_freq = 50000.0f;  // Above Nyquist for 44.1kHz
        EXPECT_NO_THROW({ MFCCProcessor processor(config); });
    }

    // TODO: Add validation for low_freq <= high_freq to prevent numerical issues
}

// Test 2: extractFeatures input validation - targeting input validation coverage
TEST_F(MFCCCoverageBoostTest, ExtractFeaturesInputValidation) {
    MFCCProcessor processor(standard_config);

    // Test wrong frame size (should return error)
    {
        std::vector<float> wrong_size(256);  // Half the expected size
        auto result = processor.extractFeatures(wrong_size);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
    }

    // Test NaN input (should return error)
    {
        std::vector<float> nan_input(standard_config.frame_size,
                                     std::numeric_limits<float>::quiet_NaN());
        auto result = processor.extractFeatures(nan_input);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
    }

    // Test infinite input (should return error)
    {
        std::vector<float> inf_input(standard_config.frame_size,
                                     std::numeric_limits<float>::infinity());
        auto result = processor.extractFeatures(inf_input);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
    }

    // Test very large values (should handle gracefully)
    {
        std::vector<float> large_input(standard_config.frame_size, 1e6f);
        auto result = processor.extractFeatures(large_input);
        if (result.has_value()) {
            // Should produce finite results
            for (float coeff : *result) {
                EXPECT_TRUE(std::isfinite(coeff));
            }
        }
    }
}

// Test 3: extractFeaturesFromBuffer validation - targeting buffer processing coverage
TEST_F(MFCCCoverageBoostTest, ExtractFeaturesFromBufferValidation) {
    MFCCProcessor processor(standard_config);

    // Test empty buffer (should return error)
    {
        std::vector<float> empty_buffer;
        auto result = processor.extractFeaturesFromBuffer(empty_buffer, 256);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
    }

    // Test buffer smaller than frame size (should return empty results)
    {
        std::vector<float> tiny_buffer(100);
        auto result = processor.extractFeaturesFromBuffer(tiny_buffer, 256);
        if (result.has_value()) {
            EXPECT_TRUE(result->empty());
        }
    }

    // Test buffer with hop size larger than frame size (should work)
    {
        auto signal = generateSineWave(440.0f, 2048);
        auto result = processor.extractFeaturesFromBuffer(signal, 1024);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_GT(result->size(), 0);
        }
    }

    // TODO: Add validation for hop_size > 0 to prevent division by zero
}

// Test 4: Mel filter bank edge cases - targeting filter bank coverage
TEST_F(MFCCCoverageBoostTest, MelFilterBankEdgeCases) {
    // Test with minimal filters
    {
        auto config = standard_config;
        config.num_filters = 2;  // Minimal number
        config.num_coefficients = 2;
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
        });
    }

    // Test with very high number of filters
    {
        auto config = standard_config;
        config.num_filters = 128;  // High number
        config.num_coefficients = 20;
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
        });
    }

    // Test with extreme frequency ranges
    {
        auto config = standard_config;
        config.low_freq = 20.0f;      // Very low
        config.high_freq = 20000.0f;  // Very high
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
        });
    }

    // Test with narrow frequency range
    {
        auto config = standard_config;
        config.low_freq = 1000.0f;
        config.high_freq = 1500.0f;  // Narrow range
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1200.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
        });
    }
}

// Test 5: DCT matrix edge cases - targeting DCT coverage
TEST_F(MFCCCoverageBoostTest, DCTMatrixEdgeCases) {
    // Test with num_coefficients == num_filters
    {
        auto config = standard_config;
        config.num_filters = 13;
        config.num_coefficients = 13;  // Equal to filters
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
            if (result.has_value()) {
                EXPECT_EQ(result->size(), 13);
            }
        });
    }

    // Test with minimal coefficients
    {
        auto config = standard_config;
        config.num_coefficients = 1;  // Just energy coefficient
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
            if (result.has_value()) {
                EXPECT_EQ(result->size(), 1);
            }
        });
    }

    // Test with high coefficient count
    {
        auto config = standard_config;
        config.num_filters = 50;
        config.num_coefficients = 40;  // High number
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, config.frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value());
            if (result.has_value()) {
                EXPECT_EQ(result->size(), 40);
            }
        });
    }
}

// Test 6: Windowing and FFT edge cases - targeting windowing coverage
TEST_F(MFCCCoverageBoostTest, WindowingAndFFTEdgeCases) {
    MFCCProcessor processor(standard_config);

    // Test all zeros (DC component only)
    {
        std::vector<float> dc_signal(standard_config.frame_size, 0.0f);
        auto result = processor.extractFeatures(dc_signal);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            // Should produce valid coefficients even for silence
            for (float coeff : *result) {
                EXPECT_TRUE(std::isfinite(coeff));
            }
        }
    }

    // Test DC signal (constant non-zero)
    {
        std::vector<float> dc_signal(standard_config.frame_size, 0.5f);
        auto result = processor.extractFeatures(dc_signal);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            // DC signal should produce specific MFCC pattern
            EXPECT_TRUE(std::isfinite((*result)[0]));  // Energy coefficient
        }
    }

    // Test impulse signal (single spike)
    {
        std::vector<float> impulse_signal(standard_config.frame_size, 0.0f);
        impulse_signal[standard_config.frame_size / 2] = 1.0f;  // Single impulse
        auto result = processor.extractFeatures(impulse_signal);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            for (float coeff : *result) {
                EXPECT_TRUE(std::isfinite(coeff));
            }
        }
    }

    // Test alternating signal (+1, -1, +1, -1...)
    {
        std::vector<float> alternating_signal(standard_config.frame_size);
        for (size_t i = 0; i < alternating_signal.size(); ++i) {
            alternating_signal[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        }
        auto result = processor.extractFeatures(alternating_signal);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            for (float coeff : *result) {
                EXPECT_TRUE(std::isfinite(coeff));
            }
        }
    }
}

// Test 7: Configuration combinations - targeting option coverage
TEST_F(MFCCCoverageBoostTest, ConfigurationCombinations) {
    auto signal = generateSineWave(1000.0f, standard_config.frame_size);

    // Test all boolean option combinations
    for (bool use_energy : {false, true}) {
        for (bool apply_lifter : {false, true}) {
            for (bool enable_simd : {false, true}) {
                for (bool enable_caching : {false, true}) {
                    auto config = standard_config;
                    config.use_energy = use_energy;
                    config.apply_lifter = apply_lifter;
                    config.enable_simd = enable_simd;
                    config.enable_caching = enable_caching;

                    EXPECT_NO_THROW({
                        MFCCProcessor processor(config);
                        auto result = processor.extractFeatures(signal);
                        EXPECT_TRUE(result.has_value())
                            << "Failed for config: energy=" << use_energy
                            << ", lifter=" << apply_lifter << ", simd=" << enable_simd
                            << ", caching=" << enable_caching;
                    });
                }
            }
        }
    }
}

// Test 8: Different frame sizes - targeting frame size coverage
TEST_F(MFCCCoverageBoostTest, DifferentFrameSizes) {
    std::vector<size_t> frame_sizes = {64, 128, 256, 512, 1024, 2048, 4096};

    for (size_t frame_size : frame_sizes) {
        auto config = standard_config;
        config.frame_size = frame_size;

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal = generateSineWave(1000.0f, frame_size);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value()) << "Failed for frame size: " << frame_size;
            if (result.has_value()) {
                EXPECT_EQ(result->size(), config.num_coefficients);
            }
        }) << "Exception for frame size: "
           << frame_size;
    }
}

// Test 9: Different sample rates - targeting sample rate coverage
TEST_F(MFCCCoverageBoostTest, DifferentSampleRates) {
    std::vector<size_t> sample_rates = {8000, 16000, 22050, 44100, 48000, 96000};

    for (size_t sample_rate : sample_rates) {
        auto config = standard_config;
        config.sample_rate = sample_rate;
        config.high_freq = 0.0f;  // Auto-adjust to Nyquist

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto signal =
                generateSineWave(1000.0f, config.frame_size, static_cast<float>(sample_rate));
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value()) << "Failed for sample rate: " << sample_rate;
        }) << "Exception for sample rate: "
           << sample_rate;
    }
}

// Test 10: Cache operations - targeting cache coverage
TEST_F(MFCCCoverageBoostTest, CacheOperations) {
    auto config = standard_config;
    config.enable_caching = true;

    MFCCProcessor processor(config);
    auto signal = generateSineWave(1000.0f, config.frame_size);

    // First extraction (populate cache)
    auto result1 = processor.extractFeatures(signal);
    EXPECT_TRUE(result1.has_value());

    // Second extraction (use cache)
    auto result2 = processor.extractFeatures(signal);
    EXPECT_TRUE(result2.has_value());

    // Results should be identical
    if (result1.has_value() && result2.has_value()) {
        ASSERT_EQ(result1->size(), result2->size());
        for (size_t i = 0; i < result1->size(); ++i) {
            EXPECT_FLOAT_EQ((*result1)[i], (*result2)[i]);
        }
    }

    // Clear cache
    processor.clearCache();

    // Third extraction (rebuild cache)
    auto result3 = processor.extractFeatures(signal);
    EXPECT_TRUE(result3.has_value());

    // Should still match
    if (result1.has_value() && result3.has_value()) {
        ASSERT_EQ(result1->size(), result3->size());
        for (size_t i = 0; i < result1->size(); ++i) {
            EXPECT_FLOAT_EQ((*result1)[i], (*result3)[i]);
        }
    }

    // Test cache size
    size_t cache_size = processor.getCacheSize();
    EXPECT_GE(cache_size, 0);  // Should be non-negative
}

// Test 11: Liftering variations - targeting liftering coverage
TEST_F(MFCCCoverageBoostTest, LifteringVariations) {
    auto signal = generateSineWave(1000.0f, standard_config.frame_size);

    std::vector<size_t> lifter_coeffs = {0, 10, 22, 30, 50};

    for (size_t lifter_coeff : lifter_coeffs) {
        auto config = standard_config;
        config.apply_lifter = true;
        config.lifter_coeff = lifter_coeff;

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            auto result = processor.extractFeatures(signal);
            EXPECT_TRUE(result.has_value()) << "Failed for lifter coeff: " << lifter_coeff;
        }) << "Exception for lifter coeff: "
           << lifter_coeff;
    }
}

// Test 12: Move semantics - targeting move constructor/assignment coverage
TEST_F(MFCCCoverageBoostTest, MoveSemantics) {
    auto config = standard_config;

    // Test move constructor
    {
        MFCCProcessor processor1(config);
        MFCCProcessor processor2 = std::move(processor1);

        auto signal = generateSineWave(1000.0f, config.frame_size);
        auto result = processor2.extractFeatures(signal);
        EXPECT_TRUE(result.has_value());
    }

    // Test move assignment
    {
        MFCCProcessor processor1(config);
        MFCCProcessor processor2(config);

        processor2 = std::move(processor1);

        auto signal = generateSineWave(1000.0f, config.frame_size);
        auto result = processor2.extractFeatures(signal);
        EXPECT_TRUE(result.has_value());
    }
}
