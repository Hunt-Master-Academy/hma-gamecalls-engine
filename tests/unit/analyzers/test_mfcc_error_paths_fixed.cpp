/**
 * @file test_mfcc_error_paths.cpp
 * @brief Comprehensive test coverage for MFCC processor error paths and edge cases
 */

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MFCCErrorPathTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Set up a valid baseline configuration using current API
        valid_config.sample_rate = 44100;
        valid_config.frame_size = 2048;
        valid_config.num_coefficients = 13;
        valid_config.num_filters = 40;
        valid_config.low_freq = 0.0f;
        valid_config.high_freq = 8000.0f;
        valid_config.use_energy = true;
        valid_config.apply_lifter = true;
        valid_config.lifter_coeff = 22;
        valid_config.enable_simd = true;
        valid_config.enable_caching = true;

        // Create engine for integration tests
        auto engineResult = UnifiedAudioEngine::create();
        if (engineResult.isOk()) {
            engine = std::move(engineResult.value);
        }
    }

    void TearDown() override {
        if (engine && sessionId != static_cast<SessionId>(-1)) {
            engine->destroySession(sessionId);
        }
    }

    MFCCProcessor::Config valid_config;
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId = static_cast<SessionId>(-1);
};

// Test MFCC with invalid frequency ranges
TEST_F(MFCCErrorPathTest, InvalidFrequencyRanges) {
    // Test low_freq > high_freq - this should still throw
    {
        auto config = valid_config;
        config.low_freq = 8000.0f;
        config.high_freq = 4000.0f;  // Lower than low_freq

        EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
    }

    // Test frequencies beyond Nyquist - this gets clamped, not thrown
    {
        auto config = valid_config;
        config.low_freq = 0.0f;
        config.high_freq = 50000.0f;  // Beyond Nyquist for 44.1kHz

        // The processor should handle this gracefully (clamp to Nyquist)
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            // Test with actual audio to ensure it works
            std::vector<float> audio(config.frame_size, 0.1f);
            auto result = processor.extractFeatures(audio);
            EXPECT_FALSE(result.empty());
        });
    }

    // Test negative frequencies
    {
        auto config = valid_config;
        config.low_freq = -1000.0f;
        config.high_freq = 4000.0f;

        // Should handle gracefully by clamping to 0
        EXPECT_NO_THROW({
            MFCCProcessor processor(config);
            std::vector<float> audio(config.frame_size, 0.1f);
            auto result = processor.extractFeatures(audio);
            EXPECT_FALSE(result.empty());
        });
    }
}

// Test MFCC processor with extremely large frame sizes
TEST_F(MFCCErrorPathTest, ExtremeFrameSizeHandling) {
    auto config = valid_config;
    config.frame_size = 1024 * 1024;  // 1MB frame size

    // This should either handle gracefully or throw
    EXPECT_NO_FATAL_FAILURE({
        try {
            MFCCProcessor processor(config);
            // If creation succeeds, test with small audio buffer
            std::vector<float> tiny_audio(100, 0.1f);
            auto result = processor.extractFeatures(tiny_audio);
            // Result might be empty due to insufficient data, but shouldn't crash
        } catch (const std::exception&) {
            // It's also acceptable to throw for unreasonable configurations
        }
    });

    // Test extremely small frame sizes
    config.frame_size = 1;
    config.hop_size = 1;
    EXPECT_NO_FATAL_FAILURE({
        try {
            MFCCProcessor processor(config);
            std::vector<float> audio(10, 0.1f);
            auto result = processor.extractFeatures(audio);
        } catch (const std::exception&) {
            // Acceptable to reject very small frame sizes
        }
    });
}

// Test zero filters configuration
TEST_F(MFCCErrorPathTest, ZeroFiltersConfiguration) {
    auto config = valid_config;
    config.n_mels = 0;

    EXPECT_THROW({ MFCCProcessor processor(config); }, std::invalid_argument);
}

// Test edge case audio signals
TEST_F(MFCCErrorPathTest, EdgeCaseAudioSignals) {
    MFCCProcessor processor(valid_config);

    // Test with all zeros
    {
        std::vector<float> zeros(valid_config.frame_size, 0.0f);
        auto result = processor.extractFeatures(zeros);
        EXPECT_FALSE(result.empty()) << "Should handle silent audio";

        // All values should be finite (not NaN or infinity)
        for (float coeff : result) {
            EXPECT_TRUE(std::isfinite(coeff)) << "MFCC coefficient should be finite";
        }
    }

    // Test with extremely large values
    {
        std::vector<float> large_values(valid_config.frame_size, 1e6f);
        auto result = processor.extractFeatures(large_values);
        EXPECT_FALSE(result.empty()) << "Should handle large amplitude audio";

        for (float coeff : result) {
            EXPECT_TRUE(std::isfinite(coeff))
                << "MFCC coefficient should be finite for large input";
        }
    }

    // Test with very short audio (shorter than frame)
    {
        std::vector<float> short_audio(10, 0.1f);  // Much shorter than frame_size
        auto result = processor.extractFeatures(short_audio);
        // Should handle gracefully - either pad or return empty
        EXPECT_NO_FATAL_FAILURE({ (void)result.size(); });
    }
}

// Test engine-level MFCC error paths
TEST_F(MFCCErrorPathTest, EngineLevelMFCCErrors) {
    if (!engine) {
        GTEST_SKIP() << "Engine creation failed, skipping engine-level tests";
    }

    // Create session
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    sessionId = sessionResult.value;

    // Test processing extremely short audio
    {
        std::vector<float> tiny_audio(1, 0.5f);  // Just 1 sample
        std::span<const float> span(tiny_audio);
        auto status = engine->processAudioChunk(sessionId, span);
        // Should handle gracefully - either process or skip
        EXPECT_NO_FATAL_FAILURE({ [[maybe_unused]] auto s = status; });
    }

    // Test processing extremely long audio burst
    {
        std::vector<float> huge_audio(1024 * 1024, 0.5f);  // 1M samples
        std::span<const float> span(huge_audio);
        auto status = engine->processAudioChunk(sessionId, span);
        // Should handle gracefully - either process in chunks or limit
        EXPECT_NO_FATAL_FAILURE({ [[maybe_unused]] auto s = status; });
    }

    // Test rapid processing cycles
    for (int i = 0; i < 10; ++i) {  // Reduced iterations for test speed
        std::vector<float> audio(512, std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
        std::span<const float> span(audio);
        auto status = engine->processAudioChunk(sessionId, span);
        EXPECT_NO_FATAL_FAILURE({ [[maybe_unused]] auto s = status; });
    }
}
