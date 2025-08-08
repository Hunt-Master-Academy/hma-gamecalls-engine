#include <cmath>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/UnifiedAudioEngine.h"
#include "huntmaster/enhanced/EnhancedAnalysisProcessor.h"

using namespace huntmaster;
using SessionId = uint32_t;

class EnhancedAnalysisProcessorIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create unified audio engine
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        audioEngine_ = std::move(engineResult.value);

        // Create session
        auto sessionResult = audioEngine_->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId_ = sessionResult.value;

        // Create enhanced analysis processor
        EnhancedAnalysisProcessor::Config processorConfig;
        processorConfig.sampleRate = 44100.0f;
        processorConfig.enablePitchTracking = true;
        processorConfig.enableHarmonicAnalysis = true;
        processorConfig.enableCadenceAnalysis = true;
        processorConfig.realTimeMode = true;

        auto processorResult = EnhancedAnalysisProcessor::create(processorConfig);
        ASSERT_TRUE(processorResult.has_value()) << "Failed to create EnhancedAnalysisProcessor";
        analysisProcessor_ = std::move(processorResult.value());

        // Generate test signals
        generateTestAudio();
    }

    void generateTestAudio() {
        const size_t signalLength = 44100;  // 1 second at 44.1kHz

        // Wildlife call simulation: frequency modulated tone
        wildlifeCall_.resize(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            // Frequency modulation from 300Hz to 800Hz over 1 second
            float freq = 300.0f + 500.0f * t;
            // Amplitude envelope
            float envelope = std::sin(M_PI * t) * 0.5f;  // Bell curve
            wildlifeCall_[i] = envelope * std::sin(2.0f * M_PI * freq * t);
        }

        // Human vocal simulation: harmonic series
        humanVocal_.resize(signalLength);
        for (size_t i = 0; i < signalLength; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float fundamental = 150.0f;  // Low male voice
            humanVocal_[i] = 0.5f * std::sin(2.0f * M_PI * fundamental * t)
                             + 0.3f * std::sin(2.0f * M_PI * fundamental * 2.0f * t)
                             + 0.2f * std::sin(2.0f * M_PI * fundamental * 3.0f * t)
                             + 0.1f * std::sin(2.0f * M_PI * fundamental * 4.0f * t);
        }

        // Mixed environment: wildlife call + background noise
        mixedEnvironment_.resize(signalLength);
        std::srand(42);  // Fixed seed for reproducible tests
        for (size_t i = 0; i < signalLength; ++i) {
            float noise = 0.05f * (2.0f * std::rand() / RAND_MAX - 1.0f);
            mixedEnvironment_[i] = 0.7f * wildlifeCall_[i] + noise;
        }
    }

    void TearDown() override {
        if (audioEngine_ && sessionId_ != INVALID_SESSION_ID) {
            [[maybe_unused]] auto status = audioEngine_->destroySession(sessionId_);
        }
    }

    std::unique_ptr<UnifiedAudioEngine> audioEngine_;
    std::unique_ptr<EnhancedAnalysisProcessor> analysisProcessor_;
    SessionId sessionId_ = INVALID_SESSION_ID;
    std::vector<float> wildlifeCall_;
    std::vector<float> humanVocal_;
    std::vector<float> mixedEnvironment_;
};

// Integration with UnifiedAudioEngine
TEST_F(EnhancedAnalysisProcessorIntegrationTest, IntegrateWithAudioEngine) {
    // Process audio through the engine first
    auto engineResult = audioEngine_->processAudioChunk(sessionId_, wildlifeCall_);
    ASSERT_EQ(engineResult, UnifiedAudioEngine::Status::OK) << "Audio engine processing failed";

    // Then analyze with enhanced processor
    auto analysisResult = analysisProcessor_->analyze(wildlifeCall_);
    ASSERT_TRUE(analysisResult.has_value()) << "Enhanced analysis failed";

    auto profile = analysisResult.value();
    EXPECT_TRUE(profile.isValid);
    EXPECT_GT(profile.overallConfidence, 0.0f);
}

// Wildlife Call Classification Simulation
TEST_F(EnhancedAnalysisProcessorIntegrationTest, WildlifeCallAnalysis) {
    auto result = analysisProcessor_->analyze(wildlifeCall_);
    ASSERT_TRUE(result.has_value());

    auto profile = result.value();

    // Extract ML features for classification
    auto mlResult = analysisProcessor_->extractMLFeatures(wildlifeCall_);
    ASSERT_TRUE(mlResult.has_value());
    auto features = mlResult.value();

    // Validate features for wildlife call characteristics
    EXPECT_GT(features.fundamentalFrequency, 200.0f) << "Wildlife calls typically >200Hz";
    EXPECT_LT(features.fundamentalFrequency, 1000.0f) << "Wildlife calls typically <1000Hz";

    // Wildlife calls often have frequency modulation
    EXPECT_FALSE(features.pitchContour.empty()) << "Should capture pitch contour";

    // Check for harmonic content
    EXPECT_GT(features.harmonicToNoiseRatio, 5.0f) << "Wildlife calls should be harmonic";

    // Temporal characteristics
    EXPECT_GT(features.estimatedTempo, 0.0f) << "Should detect temporal patterns";
}

// Human Vocal Detection
TEST_F(EnhancedAnalysisProcessorIntegrationTest, HumanVocalAnalysis) {
    auto result = analysisProcessor_->analyze(humanVocal_);
    ASSERT_TRUE(result.has_value());

    auto profile = result.value();

    // Human vocal characteristics
    if (profile.pitchResult.has_value()) {
        float freq = profile.pitchResult->frequency;
        EXPECT_GE(freq, 80.0f) << "Human vocal range starts around 80Hz";
        EXPECT_LE(freq, 1000.0f) << "Human vocal range ends around 1000Hz";
        EXPECT_GT(profile.pitchResult->confidence, 0.5f) << "Human voice should have clear pitch";
    }

    // Human voice should have strong harmonic structure
    if (profile.harmonicProfile.has_value()) {
        EXPECT_GT(profile.harmonicProfile->harmonicToNoiseRatio, 15.0f)
            << "Human voice should have high HNR";
        EXPECT_GE(profile.harmonicProfile->harmonicFreqs.size(), 3)
            << "Should detect multiple harmonics";
    }

    // Adaptive configuration should detect vocal content
    auto characteristics = AdaptiveConfigManager::detectCharacteristics(profile);
    EXPECT_TRUE(characteristics.isVocal) << "Should detect vocal characteristics";
    EXPECT_TRUE(characteristics.isTonal) << "Human voice is tonal";
}

// Mixed Environment Processing
TEST_F(EnhancedAnalysisProcessorIntegrationTest, MixedEnvironmentAnalysis) {
    auto result = analysisProcessor_->analyze(mixedEnvironment_);
    ASSERT_TRUE(result.has_value());

    auto profile = result.value();
    EXPECT_TRUE(profile.isValid);

    // Should still detect the wildlife call despite noise
    if (profile.pitchResult.has_value()) {
        EXPECT_GT(profile.pitchResult->confidence, 0.3f)
            << "Should detect signal even with background noise";
    }

    // Harmonic content should be reduced but present
    if (profile.harmonicProfile.has_value()) {
        EXPECT_GT(profile.harmonicProfile->harmonicToNoiseRatio, 2.0f)
            << "Should maintain some harmonic content";
        EXPECT_LT(profile.harmonicProfile->harmonicToNoiseRatio, 15.0f)
            << "HNR should be reduced due to noise";
    }
}

// Real-time Performance Validation
TEST_F(EnhancedAnalysisProcessorIntegrationTest, RealTimePerformance) {
    // Test with small chunks for real-time simulation
    const size_t chunkSize = 1024;  // ~23ms at 44.1kHz
    std::vector<double> processingTimes;

    for (size_t i = 0; i < wildlifeCall_.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, wildlifeCall_.size());
        std::span<const float> chunk(wildlifeCall_.data() + i, endIdx - i);

        auto start = std::chrono::high_resolution_clock::now();
        auto result = analysisProcessor_->processChunk(chunk);
        auto end = std::chrono::high_resolution_clock::now();

        ASSERT_TRUE(result.has_value()) << "Real-time chunk processing failed";

        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        processingTimes.push_back(duration);

        // Real-time constraint: processing time < audio duration
        double audioDuration = static_cast<double>(chunk.size()) * 1000.0 / 44100.0;
        EXPECT_LT(duration, audioDuration * 2.0) << "Processing should be less than 2x real-time";
    }

    // Calculate statistics
    double avgTime = std::accumulate(processingTimes.begin(), processingTimes.end(), 0.0)
                     / processingTimes.size();
    double maxTime = *std::max_element(processingTimes.begin(), processingTimes.end());

    EXPECT_LT(avgTime, 20.0) << "Average processing time should be <20ms";
    EXPECT_LT(maxTime, 50.0) << "Max processing time should be <50ms";
}

// Adaptive Configuration in Real Environment
TEST_F(EnhancedAnalysisProcessorIntegrationTest, AdaptiveConfigurationIntegration) {
    // Process different types of audio and check adaptation
    std::vector<std::pair<std::vector<float>*, std::string>> testCases = {
        {&wildlifeCall_, "wildlife"}, {&humanVocal_, "human"}, {&mixedEnvironment_, "mixed"}};

    for (auto& testCase : testCases) {
        auto result = analysisProcessor_->analyze(*testCase.first);
        ASSERT_TRUE(result.has_value()) << "Analysis failed for " << testCase.second;

        auto profile = result.value();

        // Test adaptive configuration
        EXPECT_NO_THROW(analysisProcessor_->adaptToAudioContent(profile))
            << "Adaptation failed for " << testCase.second;

        // Verify characteristics detection
        auto characteristics = AdaptiveConfigManager::detectCharacteristics(profile);
        EXPECT_GE(characteristics.dominantFrequency, 0.0f)
            << "Should detect dominant frequency for " << testCase.second;
    }
}

// Visualization Data Generation
TEST_F(EnhancedAnalysisProcessorIntegrationTest, VisualizationDataGeneration) {
    auto result = analysisProcessor_->analyze(wildlifeCall_);
    ASSERT_TRUE(result.has_value());

    auto profile = result.value();
    auto vizResult = analysisProcessor_->generateVisualizationData(profile);
    ASSERT_TRUE(vizResult.has_value()) << "Visualization data generation failed";

    auto vizData = vizResult.value();

    // Verify visualization data
    EXPECT_FALSE(vizData.pitchTrack.empty()) << "Should generate pitch track data";
    EXPECT_FALSE(vizData.harmonicSpectrum.empty()) << "Should generate harmonic spectrum data";

    // Data should be reasonable
    for (float pitch : vizData.pitchTrack) {
        EXPECT_GE(pitch, 0.0f) << "Pitch values should be non-negative";
        EXPECT_LE(pitch, 8000.0f) << "Pitch values should be reasonable";
    }

    for (float harmonic : vizData.harmonicSpectrum) {
        EXPECT_GE(harmonic, 0.0f) << "Harmonic frequencies should be non-negative";
    }
}

// JSON Export and Data Persistence
TEST_F(EnhancedAnalysisProcessorIntegrationTest, JsonExportIntegration) {
    auto result = analysisProcessor_->analyze(wildlifeCall_);
    ASSERT_TRUE(result.has_value());

    auto profile = result.value();
    std::string json = EnhancedAnalysisProcessor::exportToJson(profile);

    EXPECT_FALSE(json.empty()) << "JSON export should not be empty";

    // Verify JSON contains expected fields
    EXPECT_NE(json.find("timestamp"), std::string::npos);
    EXPECT_NE(json.find("duration"), std::string::npos);
    EXPECT_NE(json.find("overallConfidence"), std::string::npos);
    EXPECT_NE(json.find("combinedFeatures"), std::string::npos);

    // Check for analysis results
    if (profile.pitchResult.has_value()) {
        EXPECT_NE(json.find("pitch"), std::string::npos);
    }

    if (profile.harmonicProfile.has_value()) {
        EXPECT_NE(json.find("harmonic"), std::string::npos);
    }

    if (profile.cadenceProfile.has_value()) {
        EXPECT_NE(json.find("cadence"), std::string::npos);
    }
}

// Memory and Resource Management
TEST_F(EnhancedAnalysisProcessorIntegrationTest, ResourceManagement) {
    // Process multiple chunks to test memory stability
    const size_t numIterations = 100;
    const size_t chunkSize = 1024;

    for (size_t iter = 0; iter < numIterations; ++iter) {
        size_t startIdx = (iter * chunkSize) % wildlifeCall_.size();
        size_t endIdx = std::min(startIdx + chunkSize, wildlifeCall_.size());

        if (startIdx >= endIdx)
            continue;

        std::span<const float> chunk(wildlifeCall_.data() + startIdx, endIdx - startIdx);

        auto result = analysisProcessor_->processChunk(chunk);
        ASSERT_TRUE(result.has_value()) << "Processing failed at iteration " << iter;
    }

    // Get performance stats to verify stability
    std::string stats = analysisProcessor_->getPerformanceStats();
    EXPECT_FALSE(stats.empty()) << "Performance stats should be available";
    EXPECT_NE(stats.find("100"), std::string::npos) << "Should have processed 100 frames";
}

// Error Recovery and Robustness
TEST_F(EnhancedAnalysisProcessorIntegrationTest, ErrorRecoveryIntegration) {
    // Test with problematic audio data
    std::vector<float> problematicAudio(1024, 0.0f);  // All zeros

    auto result = analysisProcessor_->analyze(problematicAudio);
    // Should handle gracefully even if analysis quality is poor
    EXPECT_TRUE(result.has_value()) << "Should handle zero audio gracefully";

    if (result.has_value()) {
        auto profile = result.value();
        EXPECT_TRUE(profile.isValid) << "Profile should still be valid";
        // Confidence may be low, which is expected
    }

    // Test recovery with good audio
    auto recoveryResult = analysisProcessor_->analyze(wildlifeCall_);
    EXPECT_TRUE(recoveryResult.has_value()) << "Should recover and process good audio";

    if (recoveryResult.has_value()) {
        auto profile = recoveryResult.value();
        EXPECT_GT(profile.overallConfidence, 0.0f) << "Should have reasonable confidence";
    }
}
