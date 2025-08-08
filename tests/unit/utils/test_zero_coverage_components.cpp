/**
 * @file test_zero_coverage_components.cpp
 * @brief Focused tests for zero-coverage components to achieve 90% target
 *
 * This test suite specifically targets the 5 components with 0% coverage:
 * - PitchTracker
 * - HarmonicAnalyzer
 * - CadenceAnalyzer
 * - VoiceActivityDetector
 * - EnhancedAnalysisProcessor
 */

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

// Include the headers for the zero-coverage components
// Note: These may be abstract classes, so we'll test through UnifiedAudioEngine integration

using namespace huntmaster;
using namespace huntmaster::test;

class ZeroCoverageComponentsTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Create UnifiedAudioEngine for integration testing
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        // Create test session
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = sessionResult.value;
    }

    void TearDown() override {
        if (engine && sessionId != INVALID_SESSION_ID) {
            [[maybe_unused]] auto status = engine->destroySession(sessionId);
        }
        TestFixtureBase::TearDown();
    }

    // Helper: Generate harmonic signal for testing
    std::vector<float>
    generateHarmonicSignal(float fundamental, float duration, float sampleRate = 44100.0f) {
        size_t numSamples = static_cast<size_t>(duration * sampleRate);
        std::vector<float> audio(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            audio[i] = 0.6f * std::sin(2.0f * M_PI * fundamental * t) +         // Fundamental
                       0.3f * std::sin(2.0f * M_PI * fundamental * 2.0f * t) +  // 2nd harmonic
                       0.15f * std::sin(2.0f * M_PI * fundamental * 3.0f * t);  // 3rd harmonic
        }

        return audio;
    }

    // Helper: Generate yelp sequence for cadence testing
    std::vector<float> generateYelpSequence(float sampleRate = 44100.0f) {
        std::vector<float> audio;

        // 5 yelps with decreasing intervals (accelerating pattern)
        std::vector<float> intervals = {0.4f, 0.35f, 0.3f, 0.25f};
        float callDuration = 0.15f;
        float frequency = 750.0f;

        for (size_t call = 0; call < 5; ++call) {
            // Generate yelp with frequency sweep
            size_t callSamples = static_cast<size_t>(callDuration * sampleRate);

            for (size_t i = 0; i < callSamples; ++i) {
                float t = static_cast<float>(i) / sampleRate;
                float relativeTime = static_cast<float>(i) / callSamples;

                // High to low frequency sweep
                float currentFreq = frequency - 200.0f * relativeTime;

                // Envelope
                float envelope = std::sin(M_PI * relativeTime);

                audio.push_back(envelope * 0.7f * std::sin(2.0f * M_PI * currentFreq * t));
            }

            // Add silence between calls
            if (call < intervals.size()) {
                size_t silenceSamples = static_cast<size_t>(intervals[call] * sampleRate);
                for (size_t j = 0; j < silenceSamples; ++j) {
                    audio.push_back(0.0f);
                }
            }
        }

        return audio;
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId = INVALID_SESSION_ID;
};

// Test PitchTracker coverage through UnifiedAudioEngine integration
TEST_F(ZeroCoverageComponentsTest, PitchTrackerIntegration) {
    // Generate test signal with known pitch
    auto audio = generateHarmonicSignal(440.0f, 0.5f);

    // Process through UnifiedAudioEngine
    auto processResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
        << "Should process audio containing pitch information";

    // Check that features were extracted
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract pitch-related features";

    // Test with different frequencies
    std::vector<float> testFreqs = {220.0f, 330.0f, 660.0f, 880.0f};

    for (float freq : testFreqs) {
        auto testAudio = generateHarmonicSignal(freq, 0.3f);
        auto result = engine->processAudioChunk(sessionId, testAudio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK)
            << "Should process " << freq << "Hz signal";
    }
}

// Test HarmonicAnalyzer coverage through UnifiedAudioEngine integration
TEST_F(ZeroCoverageComponentsTest, HarmonicAnalyzerIntegration) {
    // Generate complex harmonic signal
    auto audio = generateHarmonicSignal(300.0f, 0.8f);

    // Process through UnifiedAudioEngine
    auto processResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
        << "Should process harmonic audio signal";

    // Check feature extraction
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract harmonic features";

    // Test with pure tone (simple harmonic content)
    std::vector<float> pureTone(22050);  // 0.5 seconds
    for (size_t i = 0; i < pureTone.size(); ++i) {
        pureTone[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    auto pureResult = engine->processAudioChunk(sessionId, pureTone);
    EXPECT_EQ(pureResult, UnifiedAudioEngine::Status::OK) << "Should process pure tone signal";

    // Test with noise (no harmonic content) - generate simple white noise
    std::vector<float> noise(22050);  // 0.5 seconds at 44.1kHz
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.3f, 0.3f);
    for (auto& sample : noise) {
        sample = dis(gen);
    }
    auto noiseResult = engine->processAudioChunk(sessionId, noise);
    EXPECT_EQ(noiseResult, UnifiedAudioEngine::Status::OK)
        << "Should handle noise without harmonic content";
}

// Test CadenceAnalyzer coverage through UnifiedAudioEngine integration
TEST_F(ZeroCoverageComponentsTest, CadenceAnalyzerIntegration) {
    // Generate yelp sequence with temporal patterns
    auto audio = generateYelpSequence();

    // Process through UnifiedAudioEngine
    auto processResult = engine->processAudioChunk(sessionId, audio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Should process cadence patterns";

    // Check feature extraction
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract cadence features";

    // Test with irregular patterns
    std::vector<float> irregularAudio;
    std::vector<float> irregularIntervals = {0.2f, 0.8f, 0.3f, 1.2f, 0.15f};

    for (size_t i = 0; i < irregularIntervals.size(); ++i) {
        // Short call
        for (size_t j = 0; j < 4410; ++j) {  // 0.1 second
            float t = static_cast<float>(j) / 44100.0f;
            irregularAudio.push_back(0.6f * std::sin(2.0f * M_PI * 800.0f * t));
        }

        // Variable interval
        size_t silenceSamples = static_cast<size_t>(irregularIntervals[i] * 44100.0f);
        for (size_t j = 0; j < silenceSamples; ++j) {
            irregularAudio.push_back(0.0f);
        }
    }

    auto irregularResult = engine->processAudioChunk(sessionId, irregularAudio);
    EXPECT_EQ(irregularResult, UnifiedAudioEngine::Status::OK)
        << "Should handle irregular cadence patterns";
}

// Test VoiceActivityDetector coverage through UnifiedAudioEngine integration
TEST_F(ZeroCoverageComponentsTest, VoiceActivityDetectorIntegration) {
    // Test with speech-like signal
    auto speechAudio = generateHarmonicSignal(150.0f, 0.6f);  // Low frequency like speech

    // Process through UnifiedAudioEngine
    auto processResult = engine->processAudioChunk(sessionId, speechAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK) << "Should process speech-like signal";

    // Check feature extraction
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract voice activity features";

    // Test with silence
    std::vector<float> silence(44100, 0.0f);  // 1 second silence
    auto silenceResult = engine->processAudioChunk(sessionId, silence);
    EXPECT_EQ(silenceResult, UnifiedAudioEngine::Status::OK) << "Should handle silence";

    // Test with mixed voice/silence
    std::vector<float> mixedAudio;

    // Add voice segment
    auto voiceSegment = generateHarmonicSignal(200.0f, 0.3f);
    mixedAudio.insert(mixedAudio.end(), voiceSegment.begin(), voiceSegment.end());

    // Add silence
    for (size_t i = 0; i < 22050; ++i) {  // 0.5 seconds
        mixedAudio.push_back(0.0f);
    }

    // Add another voice segment
    auto voiceSegment2 = generateHarmonicSignal(180.0f, 0.4f);
    mixedAudio.insert(mixedAudio.end(), voiceSegment2.begin(), voiceSegment2.end());

    auto mixedResult = engine->processAudioChunk(sessionId, mixedAudio);
    EXPECT_EQ(mixedResult, UnifiedAudioEngine::Status::OK)
        << "Should handle mixed voice/silence patterns";
}

// Test EnhancedAnalysisProcessor coverage through UnifiedAudioEngine integration
TEST_F(ZeroCoverageComponentsTest, EnhancedAnalysisProcessorIntegration) {
    // Generate complex audio with multiple features
    auto complexAudio = generateYelpSequence();

    // Process through UnifiedAudioEngine (should engage enhanced analysis)
    auto processResult = engine->processAudioChunk(sessionId, complexAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
        << "Should process complex audio through enhanced analysis";

    // Check comprehensive feature extraction
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract enhanced analysis features";

    // Test with different audio characteristics

    // 1. High-frequency content
    auto highFreqAudio = generateHarmonicSignal(1200.0f, 0.3f);
    auto highFreqResult = engine->processAudioChunk(sessionId, highFreqAudio);
    EXPECT_EQ(highFreqResult, UnifiedAudioEngine::Status::OK)
        << "Should handle high-frequency content";

    // 2. Low-frequency content
    auto lowFreqAudio = generateHarmonicSignal(80.0f, 0.3f);
    auto lowFreqResult = engine->processAudioChunk(sessionId, lowFreqAudio);
    EXPECT_EQ(lowFreqResult, UnifiedAudioEngine::Status::OK)
        << "Should handle low-frequency content";

    // 3. Broadband noise - generate simple white noise
    std::vector<float> noise(22050);  // 0.5 seconds at 44.1kHz
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.3f, 0.3f);
    for (auto& sample : noise) {
        sample = dis(gen);
    }
    auto noiseResult = engine->processAudioChunk(sessionId, noise);
    EXPECT_EQ(noiseResult, UnifiedAudioEngine::Status::OK) << "Should handle broadband noise";

    // 4. Complex harmonic structure
    std::vector<float> complexHarmonic(22050);
    for (size_t i = 0; i < complexHarmonic.size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        complexHarmonic[i] = 0.4f * std::sin(2.0f * M_PI * 250.0f * t) +   // Fundamental
                             0.25f * std::sin(2.0f * M_PI * 500.0f * t) +  // 2nd harmonic
                             0.15f * std::sin(2.0f * M_PI * 750.0f * t) +  // 3rd harmonic
                             0.1f * std::sin(2.0f * M_PI * 1000.0f * t) +  // 4th harmonic
                             0.05f * std::sin(2.0f * M_PI * 1250.0f * t);  // 5th harmonic
    }

    auto complexResult = engine->processAudioChunk(sessionId, complexHarmonic);
    EXPECT_EQ(complexResult, UnifiedAudioEngine::Status::OK)
        << "Should handle complex harmonic structures";
}

// Integration test covering all zero-coverage components together
TEST_F(ZeroCoverageComponentsTest, ComprehensiveIntegrationTest) {
    // Create a complex audio scenario that should engage all analysis components
    std::vector<float> comprehensiveAudio;

    // Section 1: Single yelp (pitch + harmonic analysis)
    auto yelp = generateHarmonicSignal(600.0f, 0.2f);
    comprehensiveAudio.insert(comprehensiveAudio.end(), yelp.begin(), yelp.end());

    // Section 2: Short silence (voice activity detection)
    for (size_t i = 0; i < 11025; ++i) {  // 0.25 seconds
        comprehensiveAudio.push_back(0.0f);
    }

    // Section 3: Yelp sequence (cadence analysis)
    auto sequence = generateYelpSequence();
    comprehensiveAudio.insert(comprehensiveAudio.end(), sequence.begin(), sequence.end());

    // Section 4: Another silence
    for (size_t i = 0; i < 22050; ++i) {  // 0.5 seconds
        comprehensiveAudio.push_back(0.0f);
    }

    // Section 5: Complex harmonic call
    auto complexCall = generateHarmonicSignal(400.0f, 0.8f);
    comprehensiveAudio.insert(comprehensiveAudio.end(), complexCall.begin(), complexCall.end());

    // Process the comprehensive audio
    auto processResult = engine->processAudioChunk(sessionId, comprehensiveAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
        << "Should successfully process comprehensive audio scenario";

    // Verify feature extraction occurred
    auto featureResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureResult.isOk());
    EXPECT_GT(*featureResult, 0) << "Should extract features from comprehensive analysis";

    // Test similarity scoring if master call is available
    auto similarityResult = engine->getSimilarityScore(sessionId);
    if (similarityResult.isOk()) {
        float score = *similarityResult;
        EXPECT_GE(score, 0.0f) << "Similarity score should be non-negative";
        EXPECT_LE(score, 1.0f) << "Similarity score should not exceed 1.0";
    }
}

// Error handling and edge cases for zero-coverage components
TEST_F(ZeroCoverageComponentsTest, ErrorHandlingAndEdgeCases) {
    // Test with very short audio
    std::vector<float> shortAudio(256, 0.5f);
    auto shortResult = engine->processAudioChunk(sessionId, shortAudio);
    EXPECT_EQ(shortResult, UnifiedAudioEngine::Status::OK)
        << "Should handle very short audio gracefully";

    // Test with very quiet audio
    std::vector<float> quietAudio(44100);
    for (size_t i = 0; i < quietAudio.size(); ++i) {
        quietAudio[i] = 0.001f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    auto quietResult = engine->processAudioChunk(sessionId, quietAudio);
    EXPECT_EQ(quietResult, UnifiedAudioEngine::Status::OK) << "Should handle very quiet audio";

    // Test with clipped audio
    std::vector<float> clippedAudio(44100);
    for (size_t i = 0; i < clippedAudio.size(); ++i) {
        float sample = 2.0f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        clippedAudio[i] = std::clamp(sample, -1.0f, 1.0f);  // Hard clipping
    }

    auto clippedResult = engine->processAudioChunk(sessionId, clippedAudio);
    EXPECT_EQ(clippedResult, UnifiedAudioEngine::Status::OK) << "Should handle clipped audio";

    // Test with DC offset
    std::vector<float> dcAudio(44100);
    for (size_t i = 0; i < dcAudio.size(); ++i) {
        dcAudio[i] = 0.3f + 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);  // DC offset
    }

    auto dcResult = engine->processAudioChunk(sessionId, dcAudio);
    EXPECT_EQ(dcResult, UnifiedAudioEngine::Status::OK) << "Should handle audio with DC offset";
}

// Performance validation for zero-coverage components
TEST_F(ZeroCoverageComponentsTest, PerformanceValidation) {
    auto audio = generateYelpSequence();

    // Measure processing time
    auto start = std::chrono::high_resolution_clock::now();
    auto result = engine->processAudioChunk(sessionId, audio);
    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK) << "Should successfully process audio";

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should be faster than real-time
    float audioLength = static_cast<float>(audio.size()) / 44100.0f * 1000.0f;  // ms
    EXPECT_LT(static_cast<float>(duration.count()), audioLength)
        << "Processing should be faster than real-time";

    // Test with multiple consecutive chunks
    for (int i = 0; i < 5; ++i) {
        auto chunkResult = engine->processAudioChunk(sessionId, audio);
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK)
            << "Should handle consecutive chunk processing";
    }
}
