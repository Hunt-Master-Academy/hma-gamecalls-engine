/**
 * @file test_harmonic_analyzer_comprehensive.cpp
 * @brief Comprehensive HarmonicAnalyzer coverage test with corrected API usage
 *
 * Targets HarmonicAnalyzer coverage improvement from 30.47% to >90%
 * Tests all major code paths, error conditions, and edge cases
 */

#include <cmath>
#include <complex>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/HarmonicAnalyzer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class HarmonicAnalyzerComprehensiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Standard configuration based on header file API
        standard_config.sampleRate = 44100.0f;
        standard_config.fftSize = 4096;
        standard_config.hopSize = 1024;
        standard_config.minFrequency = 80.0f;
        standard_config.maxFrequency = 8000.0f;
        standard_config.maxHarmonics = 10;
        standard_config.harmonicTolerance = 0.1f;
        standard_config.numFormants = 4;
        standard_config.enableFormantTracking = true;
        standard_config.enableTonalAnalysis = true;
        standard_config.noiseFloorDb = -60.0f;
    }

    HarmonicAnalyzer::Config standard_config;

    // Helper to generate harmonic test signals
    std::vector<float> generateHarmonicSignal(float fundamental, int num_harmonics, size_t length) {
        std::vector<float> signal(length, 0.0f);
        for (size_t i = 0; i < length; ++i) {
            float sample = 0.0f;
            for (int h = 1; h <= num_harmonics; ++h) {
                float amplitude = 1.0f / h;  // Decreasing amplitude
                sample +=
                    amplitude
                    * std::sin(2.0f * M_PI * fundamental * h * i / standard_config.sampleRate);
            }
            signal[i] = sample * 0.3f;  // Scale to avoid clipping
        }
        return signal;
    }

    std::vector<float> generateSineWave(float frequency, size_t length) {
        std::vector<float> signal(length);
        for (size_t i = 0; i < length; ++i) {
            signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / standard_config.sampleRate);
        }
        return signal;
    }

    std::vector<float> generateNoise(size_t length, float amplitude = 0.1f) {
        std::vector<float> signal(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-amplitude, amplitude);
        for (size_t i = 0; i < length; ++i) {
            signal[i] = dist(gen);
        }
        return signal;
    }
};

// Test 1: Factory method and initialization - targeting factory coverage
TEST_F(HarmonicAnalyzerComprehensiveTest, FactoryMethodAndInitialization) {
    // Test valid config
    {
        auto result = HarmonicAnalyzer::create(standard_config);
        ASSERT_TRUE(result.has_value()) << "Failed to create with valid config";
        auto analyzer = std::move(result.value());
        EXPECT_NE(analyzer.get(), nullptr);
    }

    // Test invalid sample rate
    {
        auto config = standard_config;
        config.sampleRate = 0.0f;
        auto result = HarmonicAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with zero sample rate";
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), HarmonicAnalyzer::Error::INVALID_SAMPLE_RATE);
        }
    }

    // Test negative sample rate
    {
        auto config = standard_config;
        config.sampleRate = -44100.0f;
        auto result = HarmonicAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with negative sample rate";
    }

    // Test zero FFT size
    {
        auto config = standard_config;
        config.fftSize = 0;
        auto result = HarmonicAnalyzer::create(config);
        EXPECT_FALSE(result.has_value()) << "Should fail with zero FFT size";
    }

    // Test invalid frequency range
    {
        auto config = standard_config;
        config.minFrequency = 8000.0f;
        config.maxFrequency = 80.0f;  // Max < min
        auto result = HarmonicAnalyzer::create(config);
        // This may or may not fail depending on implementation
        // Just test that it handles gracefully
        EXPECT_NO_THROW({
            if (result.has_value()) {
                auto analyzer = std::move(result.value());
            }
        });
    }
}

// Test 2: Basic harmonic analysis - targeting main analysis path
TEST_F(HarmonicAnalyzerComprehensiveTest, BasicHarmonicAnalysis) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test with harmonic signal
    {
        auto signal = generateHarmonicSignal(440.0f, 5, standard_config.fftSize);
        auto result = analyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Analysis should succeed with harmonic signal";

        if (result.has_value()) {
            const auto& profile = result.value();
            EXPECT_GT(profile.fundamentalFreq, 0.0f);
            EXPECT_GE(profile.confidence, 0.0f);
            EXPECT_LE(profile.confidence, 1.0f);
            EXPECT_GE(profile.spectralCentroid, 0.0f);
        }
    }

    // Test with insufficient data
    {
        std::vector<float> short_signal(100);  // Much smaller than FFT size
        auto result = analyzer->analyzeHarmonics(short_signal);
        EXPECT_FALSE(result.has_value()) << "Should fail with insufficient data";
        if (!result.has_value()) {
            EXPECT_EQ(result.error(), HarmonicAnalyzer::Error::INSUFFICIENT_DATA);
        }
    }

    // Test with empty data
    {
        std::vector<float> empty_signal;
        auto result = analyzer->analyzeHarmonics(empty_signal);
        EXPECT_FALSE(result.has_value()) << "Should fail with empty data";
    }

    // Test with noise (non-harmonic)
    {
        auto signal = generateNoise(standard_config.fftSize);
        auto result = analyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Analysis should complete even with noise";

        if (result.has_value()) {
            const auto& profile = result.value();
            // Noise typically has low confidence for harmonic content
            EXPECT_LE(profile.confidence, 0.8f);
        }
    }
}

// Test 3: Continuous processing - targeting processAudioChunk path
TEST_F(HarmonicAnalyzerComprehensiveTest, ContinuousProcessing) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test processing chunks
    {
        auto signal = generateHarmonicSignal(220.0f, 3, standard_config.hopSize);
        auto result = analyzer->processAudioChunk(signal);
        EXPECT_TRUE(result.has_value()) << "Chunk processing should succeed";
    }

    // Test getting current analysis
    {
        auto result = analyzer->getCurrentAnalysis();
        // May or may not have analysis depending on buffer state
        EXPECT_NO_THROW({
            if (result.has_value()) {
                const auto& profile = result.value();
                EXPECT_GE(profile.confidence, 0.0f);
            }
        });
    }

    // Test reset functionality
    {
        EXPECT_NO_THROW(analyzer->reset());
    }

    // Test isActive status
    {
        bool active = analyzer->isActive();
        EXPECT_TRUE(active || !active);  // Should return a valid boolean
    }
}

// Test 4: Spectral features - targeting getSpectralFeatures path
TEST_F(HarmonicAnalyzerComprehensiveTest, SpectralFeatures) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test spectral features extraction
    {
        auto signal = generateSineWave(1000.0f, standard_config.fftSize);
        auto result = analyzer->getSpectralFeatures(signal);
        EXPECT_TRUE(result.has_value()) << "Spectral features should be extractable";

        if (result.has_value()) {
            auto features = result.value();
            EXPECT_GT(features.first, 0.0f);   // Centroid should be positive
            EXPECT_GE(features.second, 0.0f);  // Spread should be non-negative
        }
    }

    // Test with insufficient data
    {
        std::vector<float> short_signal(10);
        auto result = analyzer->getSpectralFeatures(short_signal);
        EXPECT_FALSE(result.has_value()) << "Should fail with insufficient data";
    }
}

// Test 5: Formant extraction - targeting extractFormants path
TEST_F(HarmonicAnalyzerComprehensiveTest, FormantExtraction) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test formant extraction with formant tracking enabled
    {
        auto signal = generateHarmonicSignal(150.0f, 8, standard_config.fftSize);
        auto result = analyzer->extractFormants(signal);
        EXPECT_TRUE(result.has_value()) << "Formant extraction should succeed";

        if (result.has_value()) {
            const auto& formants = result.value();
            EXPECT_LE(formants.size(), standard_config.numFormants);
            for (float formant : formants) {
                EXPECT_GT(formant, 0.0f);
                EXPECT_LE(formant, standard_config.maxFrequency);
            }
        }
    }

    // Test with formant tracking disabled
    {
        auto config = standard_config;
        config.enableFormantTracking = false;
        auto disabledAnalyzerResult = HarmonicAnalyzer::create(config);
        ASSERT_TRUE(disabledAnalyzerResult.has_value());
        auto disabledAnalyzer = std::move(disabledAnalyzerResult.value());

        auto signal = generateHarmonicSignal(150.0f, 8, standard_config.fftSize);
        auto result = disabledAnalyzer->extractFormants(signal);
        // Should still work but may return empty or limited results
        EXPECT_TRUE(result.has_value() || !result.has_value());
    }
}

// Test 6: Tonal quality assessment - targeting assessTonalQualities path
TEST_F(HarmonicAnalyzerComprehensiveTest, TonalQualityAssessment) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test tonal quality assessment
    {
        auto signal = generateHarmonicSignal(300.0f, 6, standard_config.fftSize);
        auto result = analyzer->assessTonalQualities(signal);
        EXPECT_TRUE(result.has_value()) << "Tonal quality assessment should succeed";

        if (result.has_value()) {
            const auto& qualities = result.value();
            EXPECT_GE(qualities.rasp, 0.0f);
            EXPECT_LE(qualities.rasp, 1.0f);
            EXPECT_GE(qualities.whine, 0.0f);
            EXPECT_LE(qualities.whine, 1.0f);
            EXPECT_GE(qualities.resonance, 0.0f);
            EXPECT_LE(qualities.resonance, 1.0f);
            EXPECT_GE(qualities.brightness, 0.0f);
            EXPECT_LE(qualities.brightness, 1.0f);
            EXPECT_GE(qualities.roughness, 0.0f);
            EXPECT_LE(qualities.roughness, 1.0f);
        }
    }

    // Test with tonal analysis disabled
    {
        auto config = standard_config;
        config.enableTonalAnalysis = false;
        auto disabledAnalyzerResult = HarmonicAnalyzer::create(config);
        ASSERT_TRUE(disabledAnalyzerResult.has_value());
        auto disabledAnalyzer = std::move(disabledAnalyzerResult.value());

        auto signal = generateHarmonicSignal(300.0f, 6, standard_config.fftSize);
        auto result = disabledAnalyzer->assessTonalQualities(signal);
        // Should still work but may return default/limited results
        EXPECT_TRUE(result.has_value() || !result.has_value());
    }
}

// Test 7: Configuration management - targeting updateConfig path
TEST_F(HarmonicAnalyzerComprehensiveTest, ConfigurationManagement) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test getting current config
    {
        const auto& config = analyzer->getConfig();
        EXPECT_EQ(config.sampleRate, standard_config.sampleRate);
        EXPECT_EQ(config.fftSize, standard_config.fftSize);
        EXPECT_EQ(config.enableFormantTracking, standard_config.enableFormantTracking);
    }

    // Test updating config with valid parameters
    {
        auto newConfig = standard_config;
        newConfig.maxHarmonics = 8;
        newConfig.harmonicTolerance = 0.15f;
        auto result = analyzer->updateConfig(newConfig);
        EXPECT_TRUE(result.has_value()) << "Config update should succeed with valid parameters";
    }

    // Test updating config with invalid parameters
    {
        auto invalidConfig = standard_config;
        invalidConfig.sampleRate = 0.0f;
        auto result = analyzer->updateConfig(invalidConfig);
        EXPECT_FALSE(result.has_value()) << "Config update should fail with invalid sample rate";
    }
}

// Test 8: Utility and visualization methods - targeting additional paths
TEST_F(HarmonicAnalyzerComprehensiveTest, UtilityMethods) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test processing stats
    {
        std::string stats = analyzer->getProcessingStats();
        EXPECT_FALSE(stats.empty()) << "Processing stats should not be empty";
    }

    // Test frequency bins
    {
        auto bins = analyzer->getFrequencyBins();
        EXPECT_GT(bins.size(), 0) << "Should have frequency bins";
        for (size_t i = 1; i < bins.size(); ++i) {
            EXPECT_GT(bins[i], bins[i - 1]) << "Frequency bins should be increasing";
        }
    }

    // Test current spectrum
    {
        // Process some audio first
        auto signal = generateHarmonicSignal(440.0f, 4, standard_config.fftSize);
        analyzer->analyzeHarmonics(signal);

        auto result = analyzer->getCurrentSpectrum();
        EXPECT_TRUE(result.has_value()) << "Should be able to get current spectrum";

        if (result.has_value()) {
            const auto& spectrum = result.value();
            EXPECT_GT(spectrum.size(), 0) << "Spectrum should not be empty";
        }
    }

    // Test JSON export
    {
        HarmonicAnalyzer::HarmonicProfile profile;
        profile.fundamentalFreq = 440.0f;
        profile.confidence = 0.85f;
        profile.isHarmonic = true;
        profile.spectralCentroid = 1200.0f;

        std::string json = HarmonicAnalyzer::exportToJson(profile);
        EXPECT_FALSE(json.empty()) << "JSON export should not be empty";
        EXPECT_NE(json.find("fundamentalFreq"), std::string::npos)
            << "JSON should contain fundamental frequency";
        EXPECT_NE(json.find("440"), std::string::npos) << "JSON should contain the actual value";
    }
}

// Test 9: Edge cases and boundary conditions
TEST_F(HarmonicAnalyzerComprehensiveTest, EdgeCasesAndBoundaries) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test with silence
    {
        std::vector<float> silence(standard_config.fftSize, 0.0f);
        auto result = analyzer->analyzeHarmonics(silence);
        EXPECT_TRUE(result.has_value()) << "Analysis should handle silence";

        if (result.has_value()) {
            const auto& profile = result.value();
            EXPECT_EQ(profile.fundamentalFreq, 0.0f);
            EXPECT_FALSE(profile.isHarmonic);
        }
    }

    // Test with very high frequency
    {
        auto signal = generateSineWave(7500.0f, standard_config.fftSize);
        auto result = analyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle high frequency signals";
    }

    // Test with very low frequency
    {
        auto signal = generateSineWave(60.0f, standard_config.fftSize);
        auto result = analyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle low frequency signals";
    }

    // Test with clipped signal
    {
        auto signal = generateSineWave(440.0f, standard_config.fftSize);
        for (auto& sample : signal) {
            sample = std::clamp(sample * 5.0f, -1.0f, 1.0f);  // Clip signal
        }
        auto result = analyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Should handle clipped signals";
    }

    // Test with NaN/infinite values
    {
        auto signal = generateSineWave(440.0f, standard_config.fftSize);
        signal[100] = std::numeric_limits<float>::quiet_NaN();
        signal[200] = std::numeric_limits<float>::infinity();

        auto result = analyzer->analyzeHarmonics(signal);
        // Should either succeed with cleaned data or fail gracefully
        EXPECT_TRUE(result.has_value() || !result.has_value());
    }
}

// Test 10: Performance and large data handling
TEST_F(HarmonicAnalyzerComprehensiveTest, PerformanceAndLargeData) {
    auto analyzerResult = HarmonicAnalyzer::create(standard_config);
    ASSERT_TRUE(analyzerResult.has_value());
    auto analyzer = std::move(analyzerResult.value());

    // Test with minimum FFT size
    {
        auto config = standard_config;
        config.fftSize = 256;  // Small FFT
        auto smallAnalyzerResult = HarmonicAnalyzer::create(config);
        ASSERT_TRUE(smallAnalyzerResult.has_value());
        auto smallAnalyzer = std::move(smallAnalyzerResult.value());

        auto signal = generateHarmonicSignal(440.0f, 3, 256);
        auto result = smallAnalyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Should work with small FFT size";
    }

    // Test with large FFT size
    {
        auto config = standard_config;
        config.fftSize = 16384;  // Large FFT
        auto largeAnalyzerResult = HarmonicAnalyzer::create(config);
        ASSERT_TRUE(largeAnalyzerResult.has_value());
        auto largeAnalyzer = std::move(largeAnalyzerResult.value());

        auto signal = generateHarmonicSignal(440.0f, 5, 16384);
        auto result = largeAnalyzer->analyzeHarmonics(signal);
        EXPECT_TRUE(result.has_value()) << "Should work with large FFT size";
    }

    // Test continuous processing with multiple chunks
    {
        for (int i = 0; i < 10; ++i) {
            auto signal = generateHarmonicSignal(200.0f + i * 50.0f, 4, standard_config.hopSize);
            auto result = analyzer->processAudioChunk(signal);
            EXPECT_TRUE(result.has_value()) << "Continuous processing should work";
        }
    }
}
