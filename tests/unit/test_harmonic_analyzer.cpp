// Test for HarmonicAnalyzer - Complex spectral analysis component (0% coverage)
#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/HarmonicAnalyzer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class HarmonicAnalyzerTest : public testing::Test {
  protected:
    void SetUp() override {
        // Default configuration for testing
        defaultConfig_ = HarmonicAnalyzer::Config{.sampleRate = 44100.0f,
                                                  .fftSize = 4096,
                                                  .hopSize = 1024,
                                                  .minFrequency = 80.0f,
                                                  .maxFrequency = 8000.0f,
                                                  .maxHarmonics = 10,
                                                  .harmonicTolerance = 0.1f,
                                                  .numFormants = 4,
                                                  .enableFormantTracking = true,
                                                  .enableTonalAnalysis = true,
                                                  .noiseFloorDb = -60.0f};
    }

    // Generate synthetic sine wave for testing
    std::vector<float>
    generateSineWave(float frequency, float amplitude, size_t samples, float sampleRate) {
        std::vector<float> audio(samples);
        for (size_t i = 0; i < samples; ++i) {
            audio[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / sampleRate);
        }
        return audio;
    }

    // Generate complex harmonic signal with multiple harmonics
    std::vector<float> generateHarmonicSignal(float fundamental,
                                              size_t samples,
                                              float sampleRate,
                                              const std::vector<float>& harmonicAmplitudes = {
                                                  1.0f, 0.5f, 0.3f, 0.2f}) {
        std::vector<float> audio(samples, 0.0f);
        for (size_t h = 0; h < harmonicAmplitudes.size(); ++h) {
            float frequency = fundamental * (h + 1);
            float amplitude = harmonicAmplitudes[h];
            for (size_t i = 0; i < samples; ++i) {
                audio[i] += amplitude * std::sin(2.0f * M_PI * frequency * i / sampleRate);
            }
        }
        return audio;
    }

    // Generate noisy signal for testing edge cases
    std::vector<float> generateNoise(size_t samples, float amplitude = 0.1f) {
        std::vector<float> audio(samples);
        for (size_t i = 0; i < samples; ++i) {
            audio[i] = amplitude * (2.0f * (rand() / float(RAND_MAX)) - 1.0f);
        }
        return audio;
    }

    HarmonicAnalyzer::Config defaultConfig_;
};

// Test 1: Factory Creation and Basic Initialization
TEST_F(HarmonicAnalyzerTest, FactoryCreate_ValidConfiguration_Success) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);

    ASSERT_TRUE(analyzerResult.has_value()) << "Factory should create analyzer with valid config";
    auto& analyzer = analyzerResult.value();
    EXPECT_NE(analyzer, nullptr);

    // Verify initial state
    EXPECT_FALSE(analyzer->isActive()) << "Analyzer should not be active initially";

    // Verify configuration
    const auto& config = analyzer->getConfig();
    EXPECT_EQ(config.sampleRate, defaultConfig_.sampleRate);
    EXPECT_EQ(config.fftSize, defaultConfig_.fftSize);
    EXPECT_EQ(config.hopSize, defaultConfig_.hopSize);
    EXPECT_EQ(config.maxHarmonics, defaultConfig_.maxHarmonics);
}

// Test 2: Invalid Configuration Handling
TEST_F(HarmonicAnalyzerTest, FactoryCreate_InvalidConfigurations_Errors) {
    // Test zero sample rate
    auto invalidConfig1 = defaultConfig_;
    invalidConfig1.sampleRate = 0.0f;
    auto result1 = HarmonicAnalyzer::create(invalidConfig1);
    ASSERT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error(), HarmonicAnalyzer::Error::INVALID_SAMPLE_RATE);

    // Test invalid FFT size
    auto invalidConfig2 = defaultConfig_;
    invalidConfig2.fftSize = 0;
    auto result2 = HarmonicAnalyzer::create(invalidConfig2);
    ASSERT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), HarmonicAnalyzer::Error::INVALID_FFT_SIZE);

    // Test hop size larger than FFT size
    auto invalidConfig3 = defaultConfig_;
    invalidConfig3.hopSize = invalidConfig3.fftSize + 1;
    auto result3 = HarmonicAnalyzer::create(invalidConfig3);
    ASSERT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error(), HarmonicAnalyzer::Error::INVALID_FFT_SIZE);
}

// Test 3: Harmonic Analysis with Pure Sine Wave
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_PureSineWave_DetectsFundamental) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate pure sine wave at 440 Hz (A4)
    constexpr float testFreq = 440.0f;
    auto audio =
        generateSineWave(testFreq, 0.8f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);

    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value()) << "Analysis should succeed with sufficient audio data";

    auto profile = profileResult.value();

    // Check fundamental frequency detection (allow 5% tolerance due to FFT resolution)
    EXPECT_GT(profile.fundamentalFreq, testFreq * 0.95f);
    EXPECT_LT(profile.fundamentalFreq, testFreq * 1.05f);

    // Should detect as harmonic signal
    EXPECT_TRUE(profile.isHarmonic) << "Pure sine wave should be detected as harmonic";
    EXPECT_GT(profile.confidence, 0.5f) << "Confidence should be high for pure tone";

    // Spectral features should be reasonable
    EXPECT_GT(profile.spectralCentroid, 0.0f);
    EXPECT_GT(profile.spectralSpread, 0.0f);
    EXPECT_GT(profile.spectralRolloff, profile.fundamentalFreq);

    // Should be active after analysis
    EXPECT_TRUE(analyzer->isActive());
}

// Test 4: Complex Harmonic Signal Analysis
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_ComplexHarmonicSignal_DetectsHarmonics) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate complex harmonic signal with fundamental at 220 Hz
    constexpr float fundamental = 220.0f;
    std::vector<float> harmonicAmps = {1.0f, 0.6f, 0.4f, 0.25f, 0.15f};  // 5 harmonics
    auto audio = generateHarmonicSignal(
        fundamental, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate, harmonicAmps);

    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value());

    auto profile = profileResult.value();

    // Check fundamental frequency
    EXPECT_GT(profile.fundamentalFreq, fundamental * 0.9f);
    EXPECT_LT(profile.fundamentalFreq, fundamental * 1.1f);

    // Should detect harmonics
    EXPECT_TRUE(profile.isHarmonic);
    EXPECT_FALSE(profile.harmonicFreqs.empty());
    EXPECT_FALSE(profile.harmonicAmps.empty());
    EXPECT_FALSE(profile.harmonicRatios.empty());

    // Check that at least some harmonics are detected
    EXPECT_GE(profile.harmonicFreqs.size(), 2);  // At least fundamental + 1 harmonic

    // Verify harmonic frequencies are multiples of fundamental (with tolerance)
    for (size_t i = 0; i < std::min(profile.harmonicFreqs.size(), size_t(3)); ++i) {
        float expectedFreq = fundamental * (i + 1);
        EXPECT_GT(profile.harmonicFreqs[i], expectedFreq * 0.85f);
        EXPECT_LT(profile.harmonicFreqs[i], expectedFreq * 1.15f);
    }
}

// Test 5: Noise Signal Analysis
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_NoiseSignal_LowConfidence) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate random noise
    auto audio = generateNoise(defaultConfig_.fftSize * 2, 0.5f);

    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value());

    auto profile = profileResult.value();

    // Noise should not be detected as harmonic or have low confidence
    EXPECT_LT(profile.confidence, 0.3f) << "Noise should have low harmonic confidence";

    // Spectral features should still be computed
    EXPECT_GE(profile.spectralCentroid, 0.0f);
    EXPECT_GE(profile.spectralSpread, 0.0f);
}

// Test 6: Insufficient Data Error Handling
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_InsufficientData_ReturnsError) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate audio smaller than FFT size
    std::vector<float> shortAudio(defaultConfig_.fftSize / 2, 0.5f);

    auto result = analyzer->analyzeHarmonics(std::span<const float>(shortAudio));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), HarmonicAnalyzer::Error::INSUFFICIENT_DATA);
}

// Test 7: Streaming Audio Processing
TEST_F(HarmonicAnalyzerTest, ProcessAudioChunk_StreamingAnalysis_Success) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate test signal
    constexpr float testFreq = 330.0f;
    auto audio =
        generateSineWave(testFreq, 0.7f, defaultConfig_.fftSize * 3, defaultConfig_.sampleRate);

    // Process in chunks
    size_t chunkSize = defaultConfig_.hopSize;
    for (size_t i = 0; i + chunkSize <= audio.size(); i += chunkSize) {
        std::span<const float> chunk(audio.data() + i, chunkSize);
        auto result = analyzer->processAudioChunk(chunk);
        EXPECT_TRUE(result.has_value()) << "Chunk processing should succeed";
    }

    // Get current analysis after streaming
    auto analysisResult = analyzer->getCurrentAnalysis();
    EXPECT_TRUE(analysisResult.has_value()) << "Should have current analysis after streaming";

    if (analysisResult.has_value()) {
        auto profile = analysisResult.value();
        // Should detect the test frequency
        EXPECT_GT(profile.fundamentalFreq, testFreq * 0.9f);
        EXPECT_LT(profile.fundamentalFreq, testFreq * 1.1f);
    }
}

// Test 8: Configuration Update
TEST_F(HarmonicAnalyzerTest, UpdateConfig_NewParameters_ConfigurationChanged) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Update configuration
    auto newConfig = defaultConfig_;
    newConfig.maxHarmonics = 20;
    newConfig.harmonicTolerance = 0.05f;
    newConfig.enableFormantTracking = false;

    auto updateResult = analyzer->updateConfig(newConfig);
    EXPECT_TRUE(updateResult.has_value()) << "Config update should succeed";

    // Verify configuration was updated
    const auto& currentConfig = analyzer->getConfig();
    EXPECT_EQ(currentConfig.maxHarmonics, 20);
    EXPECT_FLOAT_EQ(currentConfig.harmonicTolerance, 0.05f);
    EXPECT_FALSE(currentConfig.enableFormantTracking);
}

// Test 9: Spectral Features Extraction
TEST_F(HarmonicAnalyzerTest, GetSpectralFeatures_BasicAnalysis_ReturnsFeatures) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate test signal
    auto audio =
        generateSineWave(1000.0f, 0.6f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);

    auto featuresResult = analyzer->getSpectralFeatures(std::span<const float>(audio));
    ASSERT_TRUE(featuresResult.has_value()) << "Spectral features extraction should succeed";

    auto features = featuresResult.value();

    // Validate spectral centroid and spread
    EXPECT_GT(features.first, 0.0f) << "Spectral centroid should be positive";
    EXPECT_GT(features.second, 0.0f) << "Spectral spread should be positive";

    // For a 1000 Hz sine wave, centroid should be near 1000 Hz
    EXPECT_GT(features.first, 800.0f);
    EXPECT_LT(features.first, 1200.0f);
}

// Test 10: Formant Extraction
TEST_F(HarmonicAnalyzerTest, ExtractFormants_VoiceLikeSignal_DetectsFormants) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate voice-like signal with multiple frequency peaks (simulated formants)
    std::vector<float> audio(defaultConfig_.fftSize * 2, 0.0f);

    // Add formant-like peaks at typical voice frequencies
    std::vector<float> formantFreqs = {800.0f, 1200.0f, 2400.0f, 3200.0f};
    for (float freq : formantFreqs) {
        auto component = generateSineWave(freq, 0.3f, audio.size(), defaultConfig_.sampleRate);
        for (size_t i = 0; i < audio.size(); ++i) {
            audio[i] += component[i];
        }
    }

    auto formantsResult = analyzer->extractFormants(std::span<const float>(audio));
    ASSERT_TRUE(formantsResult.has_value()) << "Formant extraction should succeed";

    auto formants = formantsResult.value();

    // Should detect some formants
    EXPECT_FALSE(formants.empty()) << "Should detect at least one formant";
    EXPECT_LE(formants.size(), defaultConfig_.numFormants)
        << "Should not exceed max formants config";

    // Formants should be in reasonable frequency range
    for (float formant : formants) {
        EXPECT_GT(formant, defaultConfig_.minFrequency);
        EXPECT_LT(formant, defaultConfig_.maxFrequency);
    }
}

// Test 11: Tonal Quality Assessment
TEST_F(HarmonicAnalyzerTest, AssessTonalQualities_VariousSignals_ReturnsQualities) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Test with harmonic signal (should have high resonance, low roughness)
    auto harmonicAudio =
        generateHarmonicSignal(440.0f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);

    auto qualitiesResult = analyzer->assessTonalQualities(std::span<const float>(harmonicAudio));
    ASSERT_TRUE(qualitiesResult.has_value()) << "Tonal quality assessment should succeed";

    auto qualities = qualitiesResult.value();

    // Validate quality metrics are in valid range [0, 1]
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

    // For harmonic signal, resonance should be higher than roughness
    EXPECT_GT(qualities.resonance, qualities.roughness);
}

// Test 12: Reset Functionality
TEST_F(HarmonicAnalyzerTest, Reset_AfterAnalysis_ClearsState) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Perform analysis to set state
    auto audio =
        generateSineWave(500.0f, 0.5f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);
    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value());

    // Verify analyzer is active
    EXPECT_TRUE(analyzer->isActive());

    // Reset analyzer
    analyzer->reset();

    // Verify state is cleared
    EXPECT_FALSE(analyzer->isActive()) << "Analyzer should not be active after reset";
}

// Test 13: JSON Export Functionality
TEST_F(HarmonicAnalyzerTest, ExportToJson_ValidProfile_ProducesJson) {
    // Create a test profile with sample data
    HarmonicAnalyzer::HarmonicProfile profile;
    profile.fundamentalFreq = 440.0f;
    profile.spectralCentroid = 1200.0f;
    profile.spectralSpread = 800.0f;
    profile.confidence = 0.85f;
    profile.isHarmonic = true;
    profile.harmonicFreqs = {440.0f, 880.0f, 1320.0f};
    profile.harmonicAmps = {1.0f, 0.5f, 0.3f};
    profile.qualities.resonance = 0.7f;
    profile.qualities.brightness = 0.6f;

    auto jsonString = HarmonicAnalyzer::exportToJson(profile);

    // Verify JSON is not empty and contains expected fields
    EXPECT_FALSE(jsonString.empty()) << "JSON export should not be empty";
    EXPECT_NE(jsonString.find("fundamentalFreq"), std::string::npos);
    EXPECT_NE(jsonString.find("spectralCentroid"), std::string::npos);
    EXPECT_NE(jsonString.find("confidence"), std::string::npos);
    EXPECT_NE(jsonString.find("isHarmonic"), std::string::npos);
}

// Test 14: Frequency Bins Access
TEST_F(HarmonicAnalyzerTest, GetFrequencyBins_AfterInitialization_ReturnsValidBins) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    auto frequencyBins = analyzer->getFrequencyBins();

    // Should have frequency bins
    EXPECT_FALSE(frequencyBins.empty()) << "Should have frequency bins";

    // Number of bins should be related to FFT size
    EXPECT_LE(frequencyBins.size(), defaultConfig_.fftSize / 2 + 1);

    // Bins should be in ascending order
    for (size_t i = 1; i < frequencyBins.size(); ++i) {
        EXPECT_GT(frequencyBins[i], frequencyBins[i - 1]) << "Frequency bins should be ascending";
    }

    // First bin should be 0 Hz, last should be near Nyquist
    EXPECT_FLOAT_EQ(frequencyBins[0], 0.0f);
    EXPECT_LT(frequencyBins.back(), defaultConfig_.sampleRate / 2.0f + 100.0f);
}

// Test 15: Current Spectrum Access
TEST_F(HarmonicAnalyzerTest, GetCurrentSpectrum_AfterAnalysis_ReturnsSpectrum) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Perform analysis first
    auto audio =
        generateSineWave(750.0f, 0.7f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);
    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value());

    // Get current spectrum
    auto spectrumResult = analyzer->getCurrentSpectrum();
    ASSERT_TRUE(spectrumResult.has_value()) << "Should have current spectrum after analysis";

    auto spectrum = spectrumResult.value();

    // Spectrum should have reasonable size
    EXPECT_FALSE(spectrum.empty()) << "Spectrum should not be empty";
    EXPECT_LE(spectrum.size(), defaultConfig_.fftSize / 2 + 1);

    // All magnitude values should be non-negative
    for (float magnitude : spectrum) {
        EXPECT_GE(magnitude, 0.0f) << "Spectrum magnitudes should be non-negative";
    }
}

// Test 16: Processing Statistics
TEST_F(HarmonicAnalyzerTest, GetProcessingStats_AfterOperations_ReturnsStats) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Perform some operations
    auto audio =
        generateSineWave(600.0f, 0.6f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);
    analyzer->analyzeHarmonics(std::span<const float>(audio));

    auto stats = analyzer->getProcessingStats();

    // Stats should not be empty
    EXPECT_FALSE(stats.empty()) << "Processing stats should not be empty";

    // Should contain some expected keywords
    EXPECT_NE(stats.find("Analysis"), std::string::npos) << "Stats should mention Analysis";
}

// Test 17: Edge Case - Very High Frequency
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_HighFrequency_HandlesGracefully) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate high frequency signal near Nyquist
    float highFreq = defaultConfig_.sampleRate * 0.4f;  // 40% of Nyquist
    auto audio =
        generateSineWave(highFreq, 0.5f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);

    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value()) << "Should handle high frequency signals";

    auto profile = profileResult.value();

    if (profile.isHarmonic) {
        // If detected, frequency should be in reasonable range
        EXPECT_GT(profile.fundamentalFreq, highFreq * 0.8f);
        EXPECT_LT(profile.fundamentalFreq, defaultConfig_.maxFrequency);
    }
}

// Test 18: Edge Case - Very Low Frequency
TEST_F(HarmonicAnalyzerTest, AnalyzeHarmonics_LowFrequency_HandlesGracefully) {
    auto analyzerResult = HarmonicAnalyzer::create(defaultConfig_);
    ASSERT_TRUE(analyzerResult.has_value());
    auto& analyzer = analyzerResult.value();

    // Generate low frequency signal
    float lowFreq = defaultConfig_.minFrequency + 10.0f;  // Just above minimum
    auto audio =
        generateSineWave(lowFreq, 0.5f, defaultConfig_.fftSize * 2, defaultConfig_.sampleRate);

    auto profileResult = analyzer->analyzeHarmonics(std::span<const float>(audio));
    ASSERT_TRUE(profileResult.has_value()) << "Should handle low frequency signals";

    auto profile = profileResult.value();

    if (profile.isHarmonic) {
        // If detected, frequency should be in reasonable range
        EXPECT_GT(profile.fundamentalFreq, defaultConfig_.minFrequency * 0.9f);
        EXPECT_LT(profile.fundamentalFreq, lowFreq * 1.2f);
    }
}
