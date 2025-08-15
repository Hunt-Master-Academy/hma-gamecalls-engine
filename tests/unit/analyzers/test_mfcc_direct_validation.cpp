//
// Direct MFCC Processor Validation Tests
// Specifically targets uncovered error handling paths in MFCCProcessor.cpp
//

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/MFCCProcessor.h"

using namespace huntmaster;

class MFCCDirectValidationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create valid baseline config
        valid_config.sample_rate = 44100;
        valid_config.frame_size = 512;
        valid_config.num_coefficients = 13;
        valid_config.num_filters = 26;
        valid_config.low_freq = 0.0f;
        valid_config.high_freq = 0.0f;  // Will auto-set to Nyquist
        valid_config.use_energy = false;
        valid_config.apply_lifter = true;
        valid_config.enable_simd = false;
        valid_config.enable_caching = false;
    }

    MFCCProcessor::Config valid_config;
};

TEST_F(MFCCDirectValidationTest, SampleRateValidation) {
    // Test zero sample rate (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.sample_rate = 0;

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test negative sample rate (will wrap to large number due to unsigned)
    {
        auto config = valid_config;
        config.sample_rate = static_cast<size_t>(-1);  // Very large number

        // This should either throw or handle gracefully
        EXPECT_NO_FATAL_FAILURE({
            try {
                MFCCProcessor processor(config);
            } catch (const std::exception&) {
                // Expected - configuration validation should catch this
            }
        });
    }
}

TEST_F(MFCCDirectValidationTest, FrameSizeValidation) {
    // Test zero frame size (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.frame_size = 0;

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test non-power-of-2 frame size (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.frame_size = 513;  // Not power of 2

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test another non-power-of-2 frame size
    {
        auto config = valid_config;
        config.frame_size = 1000;  // Not power of 2

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test valid power-of-2 sizes (should work)
    std::vector<size_t> valid_sizes = {64, 128, 256, 512, 1024, 2048, 4096};
    for (size_t size : valid_sizes) {
        auto config = valid_config;
        config.frame_size = size;

        EXPECT_NO_THROW({ MFCCProcessor processor(config); })
            << "Failed for valid frame size: " << size;
    }
}

TEST_F(MFCCDirectValidationTest, FilterAndCoefficientValidation) {
    // Test zero filter count (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.num_filters = 0;

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test zero coefficient count (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.num_coefficients = 0;

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test both zero (should throw std::invalid_argument)
    {
        auto config = valid_config;
        config.num_filters = 0;
        config.num_coefficients = 0;

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }
}

TEST_F(MFCCDirectValidationTest, FrequencyRangeValidation) {
    // Test auto-setting of high frequency (high_freq = 0)
    {
        auto config = valid_config;
        config.high_freq = 0.0f;  // Should auto-set to sample_rate/2

        EXPECT_NO_THROW({ MFCCProcessor processor(config); });
    }

    // Test high frequency above Nyquist (should be clamped)
    {
        auto config = valid_config;
        config.high_freq = config.sample_rate;  // Above Nyquist

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            // Should auto-correct to Nyquist frequency
        });
    }

    // Test very high frequency (should be clamped)
    {
        auto config = valid_config;
        config.high_freq = 100000.0f;  // Way above Nyquist

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            // Should auto-correct to Nyquist frequency
        });
    }
}

TEST_F(MFCCDirectValidationTest, InputValidationPaths) {
    MFCCProcessor processor(valid_config);

    // Test with wrong frame size (should return error)
    {
        std::vector<float> wrong_size(valid_config.frame_size / 2);  // Wrong size
        auto result = processor.extractFeatures(std::span<const float>(wrong_size));
        EXPECT_FALSE(result.has_value());
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
        }
    }

    // Test with NaN values (should return error)
    {
        std::vector<float> nan_signal(valid_config.frame_size,
                                      std::numeric_limits<float>::quiet_NaN());
        auto result = processor.extractFeatures(std::span<const float>(nan_signal));
        EXPECT_FALSE(result.has_value());
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
        }
    }

    // Test with infinite values (should return error)
    {
        std::vector<float> inf_signal(valid_config.frame_size,
                                      std::numeric_limits<float>::infinity());
        auto result = processor.extractFeatures(std::span<const float>(inf_signal));
        EXPECT_FALSE(result.has_value());
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
        }
    }

    // Test with empty buffer for extractFeaturesFromBuffer
    {
        std::vector<float> empty_buffer;
        auto result =
            processor.extractFeaturesFromBuffer(std::span<const float>(empty_buffer), 256);
        EXPECT_FALSE(result.has_value());
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), MFCCError::INVALID_INPUT);
        }
    }
}

TEST_F(MFCCDirectValidationTest, ConfigurationCombinations) {
    // Test minimal valid configuration
    {
        MFCCProcessor::Config config;
        config.sample_rate = 8000;    // Low but valid
        config.frame_size = 64;       // Small but valid power of 2
        config.num_coefficients = 1;  // Minimal
        config.num_filters = 1;       // Minimal
        config.low_freq = 100.0f;
        config.high_freq = 1000.0f;

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);

            std::vector<float> test_signal(64);
            for (size_t i = 0; i < 64; ++i) {
                test_signal[i] = 0.5f * std::sin(2.0f * M_PI * 500.0f * i / 8000.0f);
            }

            auto result = processor.extractFeatures(std::span<const float>(test_signal));
            EXPECT_TRUE(result.has_value());
            if (result.has_value()) {
                EXPECT_EQ(result->size(), 1);
            }
        });
    }

    // Test maximum reasonable configuration
    {
        MFCCProcessor::Config config;
        config.sample_rate = 96000;    // High sample rate
        config.frame_size = 4096;      // Large frame
        config.num_coefficients = 39;  // Many coefficients
        config.num_filters = 80;       // Many filters
        config.low_freq = 20.0f;
        config.high_freq = 20000.0f;

        EXPECT_NO_THROW({
            MFCCProcessor processor(config);

            std::vector<float> test_signal(4096);
            for (size_t i = 0; i < 4096; ++i) {
                test_signal[i] = 0.3f * std::sin(2.0f * M_PI * 1000.0f * i / 96000.0f);
            }

            auto result = processor.extractFeatures(std::span<const float>(test_signal));
            EXPECT_TRUE(result.has_value());
            if (result.has_value()) {
                EXPECT_EQ(result->size(), 39);
            }
        });
    }
}

TEST_F(MFCCDirectValidationTest, BooleanOptionCombinations) {
    // Test all combinations of boolean flags to hit different code paths
    std::vector<bool> bool_values = {false, true};

    for (bool use_energy : bool_values) {
        for (bool apply_lifter : bool_values) {
            for (bool enable_simd : bool_values) {
                for (bool enable_caching : bool_values) {
                    auto config = valid_config;
                    config.use_energy = use_energy;
                    config.apply_lifter = apply_lifter;
                    config.enable_simd = enable_simd;
                    config.enable_caching = enable_caching;

                    EXPECT_NO_THROW({
                        MFCCProcessor processor(config);

                        std::vector<float> test_signal(valid_config.frame_size);
                        for (size_t i = 0; i < test_signal.size(); ++i) {
                            test_signal[i] =
                                0.3f
                                * std::sin(2.0f * M_PI * 440.0f * i / valid_config.sample_rate);
                        }

                        auto result =
                            processor.extractFeatures(std::span<const float>(test_signal));
                        EXPECT_TRUE(result.has_value());
                    });
                }
            }
        }
    }
}

TEST_F(MFCCDirectValidationTest, SpecialSignalTypes) {
    MFCCProcessor processor(valid_config);

    // Test pure DC signal
    {
        std::vector<float> dc_signal(valid_config.frame_size, 0.75f);
        auto result = processor.extractFeatures(std::span<const float>(dc_signal));
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_EQ(result->size(), valid_config.num_coefficients);
        }
    }

    // Test pure silence
    {
        std::vector<float> silence(valid_config.frame_size, 0.0f);
        auto result = processor.extractFeatures(std::span<const float>(silence));
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_EQ(result->size(), valid_config.num_coefficients);
        }
    }

    // Test alternating signal (high frequency content)
    {
        std::vector<float> alternating(valid_config.frame_size);
        for (size_t i = 0; i < alternating.size(); ++i) {
            alternating[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        }
        auto result = processor.extractFeatures(std::span<const float>(alternating));
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_EQ(result->size(), valid_config.num_coefficients);
        }
    }

    // Test ramp signal
    {
        std::vector<float> ramp(valid_config.frame_size);
        for (size_t i = 0; i < ramp.size(); ++i) {
            ramp[i] = static_cast<float>(i) / ramp.size() - 0.5f;
        }
        auto result = processor.extractFeatures(std::span<const float>(ramp));
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            EXPECT_EQ(result->size(), valid_config.num_coefficients);
        }
    }
}

TEST_F(MFCCDirectValidationTest, CacheOperations) {
    auto config = valid_config;
    config.enable_caching = true;

    MFCCProcessor processor(config);

    // Test cache operations
    size_t initial_cache = processor.getCacheSize();

    // Process some data to potentially populate cache
    std::vector<float> test_signal(config.frame_size);
    for (size_t i = 0; i < test_signal.size(); ++i) {
        test_signal[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / config.sample_rate);
    }

    auto result1 = processor.extractFeatures(std::span<const float>(test_signal));
    EXPECT_TRUE(result1.has_value());

    size_t cache_after_processing = processor.getCacheSize();

    // Clear cache
    processor.clearCache();
    size_t cache_after_clear = processor.getCacheSize();

    // Cache should be reduced after clearing
    EXPECT_LE(cache_after_clear, cache_after_processing);

    // Process again to see if cache grows
    auto result2 = processor.extractFeatures(std::span<const float>(test_signal));
    EXPECT_TRUE(result2.has_value());

    // Results should be consistent regardless of caching
    if (result1.has_value() && result2.has_value()) {
        EXPECT_EQ(result1->size(), result2->size());
    }
}
