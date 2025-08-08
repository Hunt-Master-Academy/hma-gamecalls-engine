#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/enhanced/EnhancedAnalysisProcessor.h"

using namespace huntmaster;

class EnhancedAnalysisProcessorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Default configuration for testing
        config_.sampleRate = 44100.0f;
        config_.enablePitchTracking = true;
        config_.enableHarmonicAnalysis = true;
        config_.enableCadenceAnalysis = true;
        config_.enableVisualizationData = true;
        config_.realTimeMode = false;

        // Configure individual analyzers
        config_.pitchConfig.sampleRate = 44100.0f;
        config_.pitchConfig.windowSize = 1024;
        config_.pitchConfig.hopSize = 256;
        config_.pitchConfig.minFrequency = 80.0f;
        config_.pitchConfig.maxFrequency = 2000.0f;

        config_.harmonicConfig.sampleRate = 44100.0f;
        config_.harmonicConfig.fftSize = 2048;
        config_.harmonicConfig.hopSize = 256;

        config_.cadenceConfig.sampleRate = 44100.0f;
        config_.cadenceConfig.frameSize = 0.05f;
        config_.cadenceConfig.hopSize = 0.025f;

        // Create test audio signals
        generateTestSignals();
    }

    void generateTestSignals() {
        const size_t signalLength = 4410;  // 100ms at 44.1kHz

        // Pure tone at 440Hz (A4)
        pureTone_.resize(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            pureTone_[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }

        // Complex harmonic signal
        harmonicSignal_.resize(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            harmonicSignal_[i] = 0.4f * std::sin(2.0f * M_PI * 220.0f * t) +  // Fundamental
                                 0.3f * std::sin(2.0f * M_PI * 440.0f * t) +  // 2nd harmonic
                                 0.2f * std::sin(2.0f * M_PI * 660.0f * t);   // 3rd harmonic
        }

        // Rhythmic signal (simple beat pattern)
        rhythmicSignal_.resize(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            // Beat at 120 BPM (2 Hz)
            float envelope = (std::sin(2.0f * M_PI * 2.0f * t) > 0) ? 1.0f : 0.1f;
            rhythmicSignal_[i] = envelope * 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
        }

        // White noise
        whiteNoise_.resize(signalLength);
        std::srand(42);  // Fixed seed for reproducible tests
        for (size_t i = 0; i < signalLength; ++i) {
            whiteNoise_[i] = 0.1f * (2.0f * std::rand() / RAND_MAX - 1.0f);
        }
    }

    EnhancedAnalysisProcessor::Config config_;
    std::vector<float> pureTone_;
    std::vector<float> harmonicSignal_;
    std::vector<float> rhythmicSignal_;
    std::vector<float> whiteNoise_;
};

// Basic functionality tests
TEST_F(EnhancedAnalysisProcessorTest, CreateProcessor) {
    auto result = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(result.has_value()) << "Failed to create EnhancedAnalysisProcessor";

    auto processor = std::move(result.value());
    ASSERT_NE(processor, nullptr);
}

TEST_F(EnhancedAnalysisProcessorTest, CreateProcessorWithDisabledComponents) {
    // Test with only pitch tracking enabled
    config_.enableHarmonicAnalysis = false;
    config_.enableCadenceAnalysis = false;

    auto result = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(result.has_value());

    auto processor = std::move(result.value());
    ASSERT_NE(processor, nullptr);
}

TEST_F(EnhancedAnalysisProcessorTest, AnalyzePureTone) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->analyze(pureTone_);
    ASSERT_TRUE(result.has_value()) << "Analysis failed for pure tone";

    auto profile = result.value();
    EXPECT_TRUE(profile.isValid);
    EXPECT_GT(profile.overallConfidence, 0.0f);

    // Check pitch detection
    if (profile.pitchResult.has_value()) {
        EXPECT_NEAR(profile.pitchResult->frequency, 440.0f, 10.0f)
            << "Pitch detection should be close to 440Hz";
        EXPECT_GT(profile.pitchResult->confidence, 0.5f)
            << "Pitch confidence should be high for pure tone";
    }

    // Check combined features
    EXPECT_NEAR(profile.combinedFeatures.fundamentalFrequency, 440.0f, 20.0f);
    EXPECT_GT(profile.combinedFeatures.pitchStability, 0.3f);
}

TEST_F(EnhancedAnalysisProcessorTest, AnalyzeHarmonicSignal) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->analyze(harmonicSignal_);
    ASSERT_TRUE(result.has_value()) << "Analysis failed for harmonic signal";

    auto profile = result.value();
    EXPECT_TRUE(profile.isValid);

    // Check harmonic analysis
    if (profile.harmonicProfile.has_value()) {
        EXPECT_GT(profile.harmonicProfile->harmonicToNoiseRatio, 1.0f)
            << "HNR should be reasonable for harmonic signal";
        EXPECT_GT(profile.harmonicProfile->confidence, 0.1f);
        EXPECT_FALSE(profile.harmonicProfile->harmonicFreqs.empty())
            << "Should detect multiple harmonics";
    }

    // Check combined features
    EXPECT_GT(profile.combinedFeatures.harmonicToNoiseRatio, 1.0f);
    EXPECT_FALSE(profile.combinedFeatures.harmonicRatios.empty());
}

TEST_F(EnhancedAnalysisProcessorTest, AnalyzeRhythmicSignal) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->analyze(rhythmicSignal_);
    ASSERT_TRUE(result.has_value()) << "Analysis failed for rhythmic signal";

    auto profile = result.value();
    EXPECT_TRUE(profile.isValid);

    // Check cadence analysis
    if (profile.cadenceProfile.has_value()) {
        // Tempo detection might not work with synthetic signals
        EXPECT_GE(profile.cadenceProfile->estimatedTempo, 0.0f);
        EXPECT_GT(profile.cadenceProfile->confidence, 0.0f);
    }

    // Check combined features
    EXPECT_GE(profile.combinedFeatures.estimatedTempo, 0.0f);
}

TEST_F(EnhancedAnalysisProcessorTest, AnalyzeWhiteNoise) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->analyze(whiteNoise_);
    ASSERT_TRUE(result.has_value()) << "Analysis should handle white noise";

    auto profile = result.value();
    EXPECT_TRUE(profile.isValid);

    // White noise should have low pitch confidence
    if (profile.pitchResult.has_value()) {
        EXPECT_LT(profile.pitchResult->confidence, 0.5f)
            << "Pitch confidence should be low for white noise";
    }

    // White noise should have low HNR
    if (profile.harmonicProfile.has_value()) {
        EXPECT_LT(profile.harmonicProfile->harmonicToNoiseRatio, 10.0f)
            << "HNR should be low for white noise";
    }
}

// ML Feature Extraction Tests
TEST_F(EnhancedAnalysisProcessorTest, ExtractMLFeatures) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->extractMLFeatures(harmonicSignal_);
    ASSERT_TRUE(result.has_value()) << "ML feature extraction failed";

    auto features = result.value();

    // Check that features are populated
    EXPECT_GT(features.fundamentalFrequency, 0.0f);
    EXPECT_GE(features.spectralCentroid, 0.0f);
    EXPECT_GE(features.harmonicToNoiseRatio, 0.0f);
    EXPECT_GE(features.brightness, 0.0f);
    EXPECT_GE(features.roughness, 0.0f);
    EXPECT_GE(features.resonance, 0.0f);
    EXPECT_GE(features.pitchStability, 0.0f);
    EXPECT_GE(features.rhythmComplexity, 0.0f);
}

// Visualization Data Tests
TEST_F(EnhancedAnalysisProcessorTest, GenerateVisualizationData) {
    config_.enableVisualizationData = true;
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto analysisResult = processor->analyze(harmonicSignal_);
    ASSERT_TRUE(analysisResult.has_value());
    auto profile = analysisResult.value();

    auto vizResult = processor->generateVisualizationData(profile);
    ASSERT_TRUE(vizResult.has_value()) << "Visualization data generation failed";

    auto vizData = vizResult.value();

    // Check that visualization data is populated
    EXPECT_FALSE(vizData.pitchTrack.empty()) << "Pitch track should not be empty";
    EXPECT_FALSE(vizData.harmonicSpectrum.empty()) << "Harmonic spectrum should not be empty";
}

// Performance Tests
TEST_F(EnhancedAnalysisProcessorTest, ProcessChunk) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->processChunk(pureTone_);
    EXPECT_TRUE(result.has_value()) << "Chunk processing failed";

    // Should be able to get current analysis after processing
    auto currentResult = processor->getCurrentAnalysis();
    EXPECT_TRUE(currentResult.has_value()) << "Should have current analysis after processing chunk";
}

TEST_F(EnhancedAnalysisProcessorTest, GetPerformanceStats) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    // Process some audio
    processor->analyze(pureTone_);
    processor->analyze(harmonicSignal_);

    std::string stats = processor->getPerformanceStats();
    EXPECT_FALSE(stats.empty()) << "Performance stats should not be empty";
    EXPECT_NE(stats.find("Processed Frames"), std::string::npos)
        << "Stats should contain frame count";
    EXPECT_NE(stats.find("Processing Time"), std::string::npos)
        << "Stats should contain timing information";
}

// Real-time Configuration Tests
TEST_F(EnhancedAnalysisProcessorTest, RealTimeConfiguration) {
    config_.realTimeMode = true;
    config_.pitchConfig.windowSize = 512;  // Smaller for real-time
    config_.harmonicConfig.fftSize = 512;

    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = processor->analyze(pureTone_);
    auto end = std::chrono::high_resolution_clock::now();

    ASSERT_TRUE(result.has_value());

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 500)
        << "Processing should complete in reasonable time (<500ms for test signal)";
}

// Adaptive Configuration Tests
TEST_F(EnhancedAnalysisProcessorTest, AdaptiveConfiguration) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    // Analyze signal to get profile
    auto result = processor->analyze(harmonicSignal_);
    ASSERT_TRUE(result.has_value());
    auto profile = result.value();

    // Test adaptation
    EXPECT_NO_THROW(processor->adaptToAudioContent(profile))
        << "Adaptive configuration should not throw";
}

// Error Handling Tests
TEST_F(EnhancedAnalysisProcessorTest, HandleEmptyAudio) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    std::vector<float> emptyAudio;
    auto result = processor->analyze(emptyAudio);
    EXPECT_FALSE(result.has_value()) << "Should fail on empty audio";
    EXPECT_EQ(result.error(), EnhancedAnalysisProcessor::Error::INVALID_AUDIO_DATA);
}

TEST_F(EnhancedAnalysisProcessorTest, GetCurrentAnalysisWithoutProcessing) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->getCurrentAnalysis();
    EXPECT_FALSE(result.has_value()) << "Should fail when no audio has been processed";
    EXPECT_EQ(result.error(), EnhancedAnalysisProcessor::Error::INSUFFICIENT_DATA);
}

// Reset Functionality Tests
TEST_F(EnhancedAnalysisProcessorTest, ResetProcessor) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    // Process some audio
    processor->analyze(pureTone_);
    auto beforeReset = processor->getCurrentAnalysis();
    EXPECT_TRUE(beforeReset.has_value());

    // Reset and verify
    EXPECT_NO_THROW(processor->reset()) << "Reset should not throw";

    auto afterReset = processor->getCurrentAnalysis();
    EXPECT_FALSE(afterReset.has_value()) << "Should have no current analysis after reset";
}

// JSON Export Tests
TEST_F(EnhancedAnalysisProcessorTest, ExportToJson) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    auto result = processor->analyze(harmonicSignal_);
    ASSERT_TRUE(result.has_value());
    auto profile = result.value();

    std::string json = EnhancedAnalysisProcessor::exportToJson(profile);
    EXPECT_FALSE(json.empty()) << "JSON export should not be empty";
    EXPECT_NE(json.find("timestamp"), std::string::npos) << "JSON should contain timestamp";
    EXPECT_NE(json.find("combinedFeatures"), std::string::npos)
        << "JSON should contain combined features";
}

// Adaptive Configuration Manager Tests
TEST_F(EnhancedAnalysisProcessorTest, AdaptiveConfigManagerDetectCharacteristics) {
    auto processorResult = EnhancedAnalysisProcessor::create(config_);
    ASSERT_TRUE(processorResult.has_value());
    auto processor = std::move(processorResult.value());

    // Analyze vocal-like signal (harmonic signal in vocal range)
    auto result = processor->analyze(harmonicSignal_);
    ASSERT_TRUE(result.has_value());
    auto profile = result.value();

    auto characteristics = AdaptiveConfigManager::detectCharacteristics(profile);

    // Should detect some characteristics
    EXPECT_GE(characteristics.dominantFrequency, 0.0f);
    EXPECT_GE(characteristics.harmonicity, 0.0f);
    EXPECT_LE(characteristics.harmonicity, 1.0f);
}

TEST_F(EnhancedAnalysisProcessorTest, AdaptiveConfigManagerRealTimeConfig) {
    auto rtConfig = AdaptiveConfigManager::getRealTimeConfig(44100.0f);

    EXPECT_TRUE(rtConfig.realTimeMode);
    EXPECT_EQ(rtConfig.sampleRate, 44100.0f);
    EXPECT_LE(rtConfig.pitchConfig.windowSize, 1024)
        << "Real-time config should use smaller windows";
}

TEST_F(EnhancedAnalysisProcessorTest, AdaptiveConfigManagerHighQualityConfig) {
    auto hqConfig = AdaptiveConfigManager::getHighQualityConfig(44100.0f);

    EXPECT_TRUE(hqConfig.highQualityMode);
    EXPECT_EQ(hqConfig.sampleRate, 44100.0f);
    EXPECT_GE(hqConfig.harmonicConfig.fftSize, 4096) << "High-quality config should use larger FFT";
}
