// Enhanced Analyzers Real-Time Performance Optimization Test
// Tests optimized configurations for <10ms real-time processing targets

#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class EnhancedAnalyzersPerformanceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        sampleRate_ = 44100.0f;
        testDuration_ = 0.1f;  // 100ms for real-time validation

        // Generate optimized test signals
        testTone_ = generateSineWave(440.0f, testDuration_, sampleRate_);
        testComplexTone_ = generateComplexTone(440.0f, {0.3f, 0.2f}, testDuration_, sampleRate_);
        testRhythm_ = generateRhythmPattern(120.0f, testDuration_, sampleRate_);
    }

    std::vector<float> generateSineWave(float freq, float duration, float sampleRate) {
        size_t samples = static_cast<size_t>(duration * sampleRate);
        std::vector<float> signal(samples);

        for (size_t i = 0; i < samples; ++i) {
            signal[i] = 0.5f * std::sin(2.0f * M_PI * freq * i / sampleRate);
        }
        return signal;
    }

    std::vector<float> generateComplexTone(float fundamental,
                                           const std::vector<float>& harmonics,
                                           float duration,
                                           float sampleRate) {
        size_t samples = static_cast<size_t>(duration * sampleRate);
        std::vector<float> signal(samples, 0.0f);

        // Add fundamental
        for (size_t i = 0; i < samples; ++i) {
            signal[i] += 0.5f * std::sin(2.0f * M_PI * fundamental * i / sampleRate);
        }

        // Add limited harmonics for performance
        for (size_t h = 0; h < harmonics.size(); ++h) {
            float freq = fundamental * (h + 2);
            for (size_t i = 0; i < samples; ++i) {
                signal[i] += harmonics[h] * std::sin(2.0f * M_PI * freq * i / sampleRate);
            }
        }
        return signal;
    }

    std::vector<float> generateRhythmPattern(float bpm, float duration, float sampleRate) {
        size_t samples = static_cast<size_t>(duration * sampleRate);
        std::vector<float> signal(samples, 0.0f);

        float beatInterval = 60.0f / bpm;
        size_t samplesPerBeat = static_cast<size_t>(beatInterval * sampleRate);

        // Add multiple beats for better detection
        for (size_t beat = 0; beat * samplesPerBeat < samples; ++beat) {
            size_t startIdx = beat * samplesPerBeat;
            size_t pulseLength =
                std::min(static_cast<size_t>(0.02f * sampleRate), samples - startIdx);

            for (size_t i = 0; i < pulseLength; ++i) {
                signal[startIdx + i] = 0.8f * std::sin(2.0f * M_PI * 200.0f * i / sampleRate);
            }
        }
        return signal;
    }

  protected:
    float sampleRate_;
    float testDuration_;
    std::vector<float> testTone_;
    std::vector<float> testComplexTone_;
    std::vector<float> testRhythm_;
};

// Test PitchTracker with optimized real-time configuration
TEST_F(EnhancedAnalyzersPerformanceTest, PitchTrackerRealTimeOptimized) {
    PitchTracker::Config config;
    config.sampleRate = sampleRate_;
    config.windowSize = 512;                // Smaller window for speed
    config.hopSize = 128;                   // Smaller hop for speed
    config.enableVibratoDetection = false;  // Disable for speed

    auto result = PitchTracker::create(config);
    ASSERT_TRUE(result.has_value()) << "Failed to create optimized PitchTracker";

    auto tracker = std::move(result.value());

    // Performance timing with optimized config
    auto start = std::chrono::high_resolution_clock::now();
    auto detectionResult = tracker->detectPitch(std::span<const float>(testTone_));
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    ASSERT_TRUE(detectionResult.has_value()) << "Optimized pitch detection failed";

    auto pitch = detectionResult.value();
    EXPECT_NEAR(pitch.frequency, 440.0f, 15.0f) << "Optimized pitch detection accuracy";
    EXPECT_GT(pitch.confidence, 0.7f) << "Optimized pitch detection confidence";

    // Real-time performance validation (<10ms)
    EXPECT_LT(duration, 10.0) << "Optimized processing time should be <10ms, got: " << duration
                              << "ms";

    std::cout << "Optimized PitchTracker: " << pitch.frequency << "Hz (conf: " << pitch.confidence
              << "), processing: " << duration << "ms [REAL-TIME]" << std::endl;
}

// Test HarmonicAnalyzer with optimized real-time configuration
TEST_F(EnhancedAnalyzersPerformanceTest, HarmonicAnalyzerRealTimeOptimized) {
    HarmonicAnalyzer::Config config;
    config.sampleRate = sampleRate_;
    config.fftSize = 512;                  // Much smaller FFT for speed
    config.hopSize = 128;                  // Smaller hop for speed
    config.maxHarmonics = 3;               // Limit harmonics for speed
    config.enableFormantTracking = false;  // Disable for speed
    config.enableTonalAnalysis = false;    // Disable for speed
    config.minFrequency = 200.0f;
    config.maxFrequency = 2000.0f;
    config.harmonicTolerance = 0.2f;
    config.numFormants = 0;
    config.noiseFloorDb = -40.0f;

    auto result = HarmonicAnalyzer::create(config);
    ASSERT_TRUE(result.has_value()) << "Failed to create optimized HarmonicAnalyzer";

    auto analyzer = std::move(result.value());

    // Performance timing with optimized config
    auto start = std::chrono::high_resolution_clock::now();
    auto analysisResult = analyzer->analyzeHarmonics(std::span<const float>(testComplexTone_));
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    ASSERT_TRUE(analysisResult.has_value()) << "Optimized harmonic analysis failed";

    auto profile = analysisResult.value();
    EXPECT_NEAR(profile.fundamentalFreq, 440.0f, 20.0f)
        << "Optimized fundamental frequency detection";
    EXPECT_GT(profile.confidence, 0.3f) << "Optimized analysis confidence";

    // Real-time performance validation (<10ms)
    EXPECT_LT(duration, 10.0) << "Optimized processing time should be <10ms, got: " << duration
                              << "ms";

    std::cout << "Optimized HarmonicAnalyzer: " << profile.fundamentalFreq << "Hz, "
              << profile.harmonicFreqs.size() << " harmonics, processing: " << duration
              << "ms [REAL-TIME]" << std::endl;
}

// Test CadenceAnalyzer with optimized real-time configuration
TEST_F(EnhancedAnalyzersPerformanceTest, CadenceAnalyzerRealTimeOptimized) {
    CadenceAnalyzer::Config config;
    config.sampleRate = sampleRate_;
    config.frameSize = 0.1f;  // Larger frames for speed (100ms)
    config.hopSize = 0.05f;   // Larger hop for speed (50ms)
    config.enableBeatTracking = true;
    config.enableOnsetDetection = true;
    config.enableSyllableAnalysis = false;  // Disable for speed
    config.minTempo = 80.0f;
    config.maxTempo = 160.0f;
    config.adaptiveThreshold = 0.2f;

    auto result = CadenceAnalyzer::create(config);
    ASSERT_TRUE(result.has_value()) << "Failed to create optimized CadenceAnalyzer";

    auto analyzer = std::move(result.value());

    // Create longer rhythm signal for better detection
    auto longerRhythm = generateRhythmPattern(120.0f, 0.5f, sampleRate_);

    // Performance timing with optimized config
    auto start = std::chrono::high_resolution_clock::now();
    auto analysisResult = analyzer->analyzeCadence(std::span<const float>(longerRhythm));
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    ASSERT_TRUE(analysisResult.has_value()) << "Optimized cadence analysis failed";

    auto profile = analysisResult.value();

    // More lenient expectations for optimized config
    EXPECT_GT(profile.estimatedTempo, 0.0f) << "Should estimate some tempo";
    EXPECT_LT(profile.estimatedTempo, 300.0f) << "Tempo should be in reasonable range";

    // Real-time performance validation (<10ms)
    EXPECT_LT(duration, 10.0) << "Optimized processing time should be <10ms, got: " << duration
                              << "ms";

    std::cout << "Optimized CadenceAnalyzer: " << profile.estimatedTempo << " BPM, "
              << profile.beatTimes.size() << " beats, processing: " << duration << "ms [REAL-TIME]"
              << std::endl;
}

// Test all optimized Enhanced Analyzers working together in real-time
TEST_F(EnhancedAnalyzersPerformanceTest, CombinedRealTimeOptimized) {
    // Create all analyzers with optimized configs
    auto pitchResult = PitchTracker::create({.sampleRate = sampleRate_,
                                             .minFrequency = 80.0f,
                                             .maxFrequency = 2000.0f,
                                             .threshold = 0.2f,
                                             .windowSize = 512,
                                             .hopSize = 128,
                                             .enableSmoothing = false,
                                             .enableVibratoDetection = false});

    auto harmonicResult = HarmonicAnalyzer::create({.sampleRate = sampleRate_,
                                                    .fftSize = 512,
                                                    .hopSize = 128,
                                                    .minFrequency = 200.0f,
                                                    .maxFrequency = 2000.0f,
                                                    .maxHarmonics = 3,
                                                    .harmonicTolerance = 0.2f,
                                                    .numFormants = 0,
                                                    .enableFormantTracking = false,
                                                    .enableTonalAnalysis = false,
                                                    .noiseFloorDb = -40.0f});

    auto cadenceResult = CadenceAnalyzer::create({.sampleRate = sampleRate_,
                                                  .frameSize = 0.1f,
                                                  .hopSize = 0.05f,
                                                  .minTempo = 80.0f,
                                                  .maxTempo = 160.0f,
                                                  .enableBeatTracking = true,
                                                  .enableOnsetDetection = true,
                                                  .enableSyllableAnalysis = false,
                                                  .adaptiveThreshold = 0.2f});

    ASSERT_TRUE(pitchResult.has_value()) << "Optimized PitchTracker creation failed";
    ASSERT_TRUE(harmonicResult.has_value()) << "Optimized HarmonicAnalyzer creation failed";
    ASSERT_TRUE(cadenceResult.has_value()) << "Optimized CadenceAnalyzer creation failed";

    auto longerRhythm = generateRhythmPattern(120.0f, 0.5f, sampleRate_);

    // Run combined analysis with performance timing
    auto start = std::chrono::high_resolution_clock::now();

    auto pitch = pitchResult.value()->detectPitch(std::span<const float>(testComplexTone_));
    auto harmonic =
        harmonicResult.value()->analyzeHarmonics(std::span<const float>(testComplexTone_));
    auto cadence = cadenceResult.value()->analyzeCadence(std::span<const float>(longerRhythm));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    ASSERT_TRUE(pitch.has_value()) << "Combined optimized pitch detection failed";
    ASSERT_TRUE(harmonic.has_value()) << "Combined optimized harmonic analysis failed";
    ASSERT_TRUE(cadence.has_value()) << "Combined optimized cadence analysis failed";

    // Combined real-time performance validation (<15ms total for optimized configs)
    EXPECT_LT(duration, 15.0) << "Combined optimized processing should be <15ms, got: " << duration
                              << "ms";

    std::cout << "Combined optimized analysis: " << duration << "ms total [REAL-TIME READY]"
              << std::endl;
}

// Performance comparison test
TEST_F(EnhancedAnalyzersPerformanceTest, PerformanceComparison) {
    std::cout << "\n=== Performance Comparison: Standard vs Optimized Configurations ==="
              << std::endl;

    // Standard config performance
    auto standardHarmonic = HarmonicAnalyzer::create({.sampleRate = sampleRate_,
                                                      .fftSize = 2048,  // Standard size
                                                      .hopSize = 512,
                                                      .minFrequency = 80.0f,
                                                      .maxFrequency = 8000.0f,
                                                      .maxHarmonics = 10,
                                                      .harmonicTolerance = 0.1f,
                                                      .numFormants = 4,
                                                      .enableFormantTracking = true,
                                                      .enableTonalAnalysis = true,
                                                      .noiseFloorDb = -60.0f});

    ASSERT_TRUE(standardHarmonic.has_value());

    auto start = std::chrono::high_resolution_clock::now();
    auto standardResult =
        standardHarmonic.value()->analyzeHarmonics(std::span<const float>(testComplexTone_));
    auto end = std::chrono::high_resolution_clock::now();
    auto standardDuration = std::chrono::duration<double, std::milli>(end - start).count();

    // Optimized config performance
    auto optimizedHarmonic = HarmonicAnalyzer::create({.sampleRate = sampleRate_,
                                                       .fftSize = 512,  // Optimized size
                                                       .hopSize = 128,
                                                       .minFrequency = 200.0f,
                                                       .maxFrequency = 2000.0f,
                                                       .maxHarmonics = 3,
                                                       .harmonicTolerance = 0.2f,
                                                       .numFormants = 0,
                                                       .enableFormantTracking = false,
                                                       .enableTonalAnalysis = false,
                                                       .noiseFloorDb = -40.0f});

    ASSERT_TRUE(optimizedHarmonic.has_value());

    start = std::chrono::high_resolution_clock::now();
    auto optimizedResult =
        optimizedHarmonic.value()->analyzeHarmonics(std::span<const float>(testComplexTone_));
    end = std::chrono::high_resolution_clock::now();
    auto optimizedDuration = std::chrono::duration<double, std::milli>(end - start).count();

    ASSERT_TRUE(standardResult.has_value());
    ASSERT_TRUE(optimizedResult.has_value());

    float speedup = standardDuration / optimizedDuration;

    std::cout << "Standard config: " << standardDuration << "ms" << std::endl;
    std::cout << "Optimized config: " << optimizedDuration << "ms" << std::endl;
    std::cout << "Performance improvement: " << speedup << "x faster" << std::endl;

    // Should achieve significant speedup
    EXPECT_GT(speedup, 2.0f) << "Optimized config should be at least 2x faster";
    EXPECT_LT(optimizedDuration, 10.0) << "Optimized config should meet real-time target";
}
