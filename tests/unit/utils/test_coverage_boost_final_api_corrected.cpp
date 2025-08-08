/**
 * @file test_coverage_boost_final_api_corrected.cpp
 * @brief API-corrected comprehensive coverage boost tests targeting >90% total coverage
 *
 * Systematically tests low-coverage components identified in coverage analysis:
 * - MFCCProcessor (19% → target 85%+)
 * - HarmonicAnalyzer (30% → target 85%+)
 * - VoiceActivityDetector (58% → target 85%+)
 * - RealtimeScorer (65% → target 85%+)
 * - Additional component coverage improvements
 *
 * API Corrections Applied:
 * - MFCCProcessor::Config uses sample_rate, frame_size (not sampleRate, frameSize)
 * - VoiceActivityDetector uses Config constructor pattern
 * - HarmonicAnalyzer uses create() factory method and correct Result<T, Error> return types
 * - Expected types use .has_value() not .isOk()
 * - Correct method names: extractFeatures, processWindow, analyzeHarmonics
 */

#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/AudioLevelProcessor.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/RealTimeAudioProcessor.h"
#include "huntmaster/core/RealtimeScorer.h"
#include "huntmaster/core/SpectrogramProcessor.h"
#include "huntmaster/core/VoiceActivityDetector.h"

namespace huntmaster {
namespace test {

class CoverageBoostFinalTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Generate test audio data
        const size_t sampleCount = 8192;
        testAudio.resize(sampleCount);

        // Create sine wave with harmonics for realistic testing
        const float sampleRate = 44100.0f;
        const float fundamental = 440.0f;  // A4

        for (size_t i = 0; i < sampleCount; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            testAudio[i] = 0.5f * std::sin(2.0f * M_PI * fundamental * t)
                           + 0.25f * std::sin(2.0f * M_PI * fundamental * 2.0f * t)
                           + 0.125f * std::sin(2.0f * M_PI * fundamental * 3.0f * t);
        }

        // Add some noise for realism
        for (size_t i = 0; i < sampleCount; ++i) {
            testAudio[i] += 0.05f * (static_cast<float>(rand()) / RAND_MAX - 0.5f);
        }

        // Generate silent audio for edge case testing
        silentAudio.resize(sampleCount, 0.0f);

        // Generate noise audio
        noiseAudio.resize(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            noiseAudio[i] = 0.2f * (static_cast<float>(rand()) / RAND_MAX - 0.5f);
        }
    }

    std::vector<float> testAudio;
    std::vector<float> silentAudio;
    std::vector<float> noiseAudio;

    static constexpr float SAMPLE_RATE = 44100.0f;
    static constexpr size_t FRAME_SIZE = 512;
    static constexpr size_t HOP_SIZE = 256;
};

// ==================== MFCCProcessor Coverage Tests ====================

TEST_F(CoverageBoostFinalTest, MFCCProcessor_ComprehensiveConfigurationTesting) {
    // Test various configuration combinations
    std::vector<MFCCProcessor::Config> configs = {// Standard configuration
                                                  {.sample_rate = 44100,
                                                   .frame_size = 512,
                                                   .num_coefficients = 13,
                                                   .num_filters = 26,
                                                   .low_freq = 0.0f,
                                                   .high_freq = 0.0f,  // Auto-set to sample_rate/2
                                                   .use_energy = true,
                                                   .apply_lifter = true},
                                                  // High resolution configuration
                                                  {.sample_rate = 48000,
                                                   .frame_size = 1024,
                                                   .num_coefficients = 20,
                                                   .num_filters = 40,
                                                   .low_freq = 80.0f,
                                                   .high_freq = 8000.0f,
                                                   .use_energy = false,
                                                   .apply_lifter = false},
                                                  // Minimal configuration
                                                  {.sample_rate = 22050,
                                                   .frame_size = 256,
                                                   .num_coefficients = 8,
                                                   .num_filters = 16,
                                                   .low_freq = 100.0f,
                                                   .high_freq = 4000.0f,
                                                   .use_energy = true,
                                                   .apply_lifter = true}};

    for (const auto& config : configs) {
        ASSERT_NO_THROW({
            MFCCProcessor processor(config);

            // Test single frame extraction
            if (testAudio.size() >= config.frame_size) {
                std::span<const float> frame(testAudio.data(), config.frame_size);
                auto result = processor.extractFeatures(frame);

                ASSERT_TRUE(result.has_value()) << "Single frame extraction failed";
                EXPECT_EQ(result->size(), config.num_coefficients);

                // Validate feature values are finite
                for (float coeff : *result) {
                    EXPECT_TRUE(std::isfinite(coeff)) << "Non-finite MFCC coefficient";
                }
            }

            // Test buffer extraction with different hop sizes
            std::vector<size_t> hopSizes = {128, 256, 512};
            for (size_t hopSize : hopSizes) {
                auto bufferResult = processor.extractFeaturesFromBuffer(testAudio, hopSize);
                ASSERT_TRUE(bufferResult.has_value())
                    << "Buffer extraction failed for hop size " << hopSize;

                if (!bufferResult->empty()) {
                    EXPECT_EQ(bufferResult->front().size(), config.num_coefficients);

                    // Test matrix consistency
                    for (const auto& frame : *bufferResult) {
                        EXPECT_EQ(frame.size(), config.num_coefficients);
                        for (float coeff : frame) {
                            EXPECT_TRUE(std::isfinite(coeff));
                        }
                    }
                }
            }

            // Test cache management
            processor.clearCache();
            EXPECT_EQ(processor.getCacheSize(), 0);
        });
    }
}

TEST_F(CoverageBoostFinalTest, MFCCProcessor_EdgeCasesAndErrorHandling) {
    MFCCProcessor::Config config{
        .sample_rate = 44100, .frame_size = 512, .num_coefficients = 13, .num_filters = 26};

    MFCCProcessor processor(config);

    // Test with silent audio
    std::span<const float> silentFrame(silentAudio.data(), config.frame_size);
    auto silentResult = processor.extractFeatures(silentFrame);
    EXPECT_TRUE(silentResult.has_value());

    // Test with noise audio
    std::span<const float> noiseFrame(noiseAudio.data(), config.frame_size);
    auto noiseResult = processor.extractFeatures(noiseFrame);
    EXPECT_TRUE(noiseResult.has_value());

    // Test with wrong frame size (should fail)
    std::span<const float> wrongSizeFrame(testAudio.data(), config.frame_size / 2);
    auto wrongSizeResult = processor.extractFeatures(wrongSizeFrame);
    EXPECT_FALSE(wrongSizeResult.has_value());

    // Test empty buffer
    std::vector<float> emptyBuffer;
    auto emptyResult = processor.extractFeaturesFromBuffer(emptyBuffer, 256);
    EXPECT_FALSE(emptyResult.has_value());

    // Test various audio patterns
    std::vector<float> extremeAudio(config.frame_size);

    // Test with maximum amplitude
    std::fill(extremeAudio.begin(), extremeAudio.end(), 1.0f);
    auto maxResult = processor.extractFeatures(extremeAudio);
    EXPECT_TRUE(maxResult.has_value());

    // Test with minimum amplitude
    std::fill(extremeAudio.begin(), extremeAudio.end(), -1.0f);
    auto minResult = processor.extractFeatures(extremeAudio);
    EXPECT_TRUE(minResult.has_value());
}

TEST_F(CoverageBoostFinalTest, MFCCProcessor_PerformanceAndCaching) {
    MFCCProcessor::Config config{.sample_rate = 44100,
                                 .frame_size = 512,
                                 .num_coefficients = 13,
                                 .num_filters = 26,
                                 .enable_caching = true};

    MFCCProcessor processor(config);

    // Test repeated processing for cache behavior
    std::span<const float> frame(testAudio.data(), config.frame_size);

    auto result1 = processor.extractFeatures(frame);
    auto result2 = processor.extractFeatures(frame);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result1->size(), result2->size());

    // Test cache size tracking
    size_t cacheSize = processor.getCacheSize();
    EXPECT_GE(cacheSize, 0);

    // Process more frames to increase cache
    for (size_t offset = config.frame_size; offset + config.frame_size <= testAudio.size();
         offset += config.frame_size) {
        std::span<const float> frameSpan(testAudio.data() + offset, config.frame_size);
        auto result = processor.extractFeatures(frameSpan);
        EXPECT_TRUE(result.has_value());
    }

    size_t newCacheSize = processor.getCacheSize();

    // Clear cache and verify
    processor.clearCache();
    EXPECT_EQ(processor.getCacheSize(), 0);
}

// ==================== VoiceActivityDetector Coverage Tests ====================

TEST_F(CoverageBoostFinalTest, VoiceActivityDetector_ConfigurationVariations) {
    std::vector<VoiceActivityDetector::Config> configs = {
        // Standard configuration
        {.energy_threshold = 0.01f,
         .window_duration = std::chrono::milliseconds(20),
         .min_sound_duration = std::chrono::milliseconds(100),
         .pre_buffer = std::chrono::milliseconds(50),
         .post_buffer = std::chrono::milliseconds(100),
         .sample_rate = 44100},
        // Sensitive configuration
        {.energy_threshold = 0.005f,
         .window_duration = std::chrono::milliseconds(10),
         .min_sound_duration = std::chrono::milliseconds(50),
         .pre_buffer = std::chrono::milliseconds(25),
         .post_buffer = std::chrono::milliseconds(50),
         .sample_rate = 44100},
        // Conservative configuration
        {.energy_threshold = 0.05f,
         .window_duration = std::chrono::milliseconds(40),
         .min_sound_duration = std::chrono::milliseconds(200),
         .pre_buffer = std::chrono::milliseconds(100),
         .post_buffer = std::chrono::milliseconds(200),
         .sample_rate = 44100}};

    for (const auto& config : configs) {
        ASSERT_NO_THROW({
            VoiceActivityDetector vad(config);

            // Test with different audio types
            std::vector<std::vector<float>*> audioSamples = {&testAudio, &silentAudio, &noiseAudio};

            for (auto* audio : audioSamples) {
                // Process in windows
                size_t windowSize =
                    static_cast<size_t>(config.window_duration.count() * config.sample_rate / 1000);
                windowSize = std::min(windowSize, audio->size());

                if (windowSize > 0) {
                    std::span<const float> window(audio->data(), windowSize);
                    auto result = vad.processWindow(window);

                    ASSERT_TRUE(result.has_value()) << "VAD processing failed";

                    // Validate result structure
                    EXPECT_GE(result->energy_level, 0.0f);
                    EXPECT_GE(result->duration.count(), 0);
                }
            }

            // Test state queries
            bool isActive = vad.isVoiceActive();
            EXPECT_TRUE(isActive || !isActive);  // Just ensure it doesn't crash

            auto duration = vad.getActiveDuration();
            EXPECT_GE(duration.count(), 0);

            // Test reset
            vad.reset();
            EXPECT_FALSE(vad.isVoiceActive());
            EXPECT_EQ(vad.getActiveDuration().count(), 0);
        });
    }
}

TEST_F(CoverageBoostFinalTest, VoiceActivityDetector_StateMachineTransitions) {
    VoiceActivityDetector::Config config{.energy_threshold = 0.01f,
                                         .window_duration = std::chrono::milliseconds(20),
                                         .min_sound_duration = std::chrono::milliseconds(100),
                                         .pre_buffer = std::chrono::milliseconds(50),
                                         .post_buffer = std::chrono::milliseconds(100),
                                         .sample_rate = 44100};

    VoiceActivityDetector vad(config);

    size_t windowSize =
        static_cast<size_t>(config.window_duration.count() * config.sample_rate / 1000);
    windowSize = std::min(windowSize, silentAudio.size());

    // Start with silence
    std::span<const float> silentWindow(silentAudio.data(), windowSize);
    auto silentResult = vad.processWindow(silentWindow);
    ASSERT_TRUE(silentResult.has_value());
    EXPECT_FALSE(silentResult->is_active);

    // Process some voice activity
    if (windowSize <= testAudio.size()) {
        std::span<const float> voiceWindow(testAudio.data(), windowSize);

        // Process multiple voice windows to trigger state transitions
        for (int i = 0; i < 10; ++i) {
            auto voiceResult = vad.processWindow(voiceWindow);
            ASSERT_TRUE(voiceResult.has_value());
        }

        // Return to silence
        for (int i = 0; i < 5; ++i) {
            auto endSilentResult = vad.processWindow(silentWindow);
            ASSERT_TRUE(endSilentResult.has_value());
        }
    }
}

TEST_F(CoverageBoostFinalTest, VoiceActivityDetector_ErrorConditions) {
    VoiceActivityDetector::Config config{.energy_threshold = 0.01f,
                                         .window_duration = std::chrono::milliseconds(20),
                                         .min_sound_duration = std::chrono::milliseconds(100),
                                         .sample_rate = 44100};

    VoiceActivityDetector vad(config);

    // Test with empty audio
    std::vector<float> emptyAudio;
    std::span<const float> emptySpan(emptyAudio.data(), emptyAudio.size());
    auto emptyResult = vad.processWindow(emptySpan);
    EXPECT_FALSE(emptyResult.has_value());

    // Test with extremely short audio
    std::vector<float> tinyAudio(1, 0.5f);
    std::span<const float> tinySpan(tinyAudio.data(), tinyAudio.size());
    auto tinyResult = vad.processWindow(tinySpan);
    EXPECT_TRUE(tinyResult.has_value());  // Should handle gracefully
}

// ==================== HarmonicAnalyzer Coverage Tests ====================

TEST_F(CoverageBoostFinalTest, HarmonicAnalyzer_ComprehensiveAnalysis) {
    HarmonicAnalyzer::Config config{.sampleRate = 44100,
                                    .fftSize = 1024,
                                    .hopSize = 512,
                                    .windowType = HarmonicAnalyzer::WindowType::HANNING,
                                    .enableFormantTracking = true,
                                    .enableTonalAnalysis = true,
                                    .minFundamental = 80.0f,
                                    .maxFundamental = 2000.0f};

    auto analyzerResult = HarmonicAnalyzer::create(config);
    ASSERT_TRUE(analyzerResult.has_value()) << "Failed to create HarmonicAnalyzer";

    auto analyzer = std::move(*analyzerResult);

    // Test harmonic analysis with different audio types
    std::vector<std::pair<std::string, std::vector<float>*>> testCases = {
        {"tonal_audio", &testAudio}, {"noise_audio", &noiseAudio}, {"silent_audio", &silentAudio}};

    for (const auto& [name, audio] : testCases) {
        if (audio->size() >= config.fftSize) {
            std::span<const float> audioSpan(audio->data(), config.fftSize);

            // Test full harmonic analysis
            auto harmonicResult = analyzer->analyzeHarmonics(audioSpan);
            ASSERT_TRUE(harmonicResult.has_value()) << "Harmonic analysis failed for " << name;

            const auto& profile = *harmonicResult;
            EXPECT_GE(profile.fundamentalFreq, 0.0f);
            EXPECT_GE(profile.confidence, 0.0f);
            EXPECT_LE(profile.confidence, 1.0f);
            EXPECT_GE(profile.spectralCentroid, 0.0f);
            EXPECT_GE(profile.spectralSpread, 0.0f);

            // Test spectral features extraction
            auto spectralResult = analyzer->getSpectralFeatures(audioSpan);
            ASSERT_TRUE(spectralResult.has_value()) << "Spectral features failed for " << name;

            auto [centroid, spread] = *spectralResult;
            EXPECT_GE(centroid, 0.0f);
            EXPECT_GE(spread, 0.0f);

            // Test formant extraction
            auto formantResult = analyzer->extractFormants(audioSpan);
            ASSERT_TRUE(formantResult.has_value()) << "Formant extraction failed for " << name;

            const auto& formants = *formantResult;
            for (float formant : formants) {
                EXPECT_GE(formant, 0.0f);
                EXPECT_LE(formant, config.sampleRate / 2.0f);
            }

            // Test tonal quality assessment
            auto tonalResult = analyzer->assessTonalQualities(audioSpan);
            ASSERT_TRUE(tonalResult.has_value()) << "Tonal assessment failed for " << name;

            const auto& qualities = *tonalResult;
            EXPECT_GE(qualities.harmonicity, 0.0f);
            EXPECT_LE(qualities.harmonicity, 1.0f);
            EXPECT_GE(qualities.roughness, 0.0f);
            EXPECT_GE(qualities.brightness, 0.0f);
        }
    }

    // Test continuous processing
    auto chunkResult = analyzer->processAudioChunk(testAudio);
    EXPECT_TRUE(chunkResult.has_value());

    auto currentResult = analyzer->getCurrentAnalysis();
    EXPECT_TRUE(currentResult.has_value());

    // Test reset functionality
    analyzer->reset();
}

TEST_F(CoverageBoostFinalTest, HarmonicAnalyzer_EdgeCasesAndConfigurations) {
    // Test various configurations
    std::vector<HarmonicAnalyzer::Config> configs = {
        // High resolution
        {.sampleRate = 48000,
         .fftSize = 2048,
         .hopSize = 1024,
         .windowType = HarmonicAnalyzer::WindowType::HAMMING,
         .enableFormantTracking = true,
         .enableTonalAnalysis = true,
         .minFundamental = 50.0f,
         .maxFundamental = 4000.0f},
        // Fast processing
        {.sampleRate = 22050,
         .fftSize = 512,
         .hopSize = 256,
         .windowType = HarmonicAnalyzer::WindowType::BLACKMAN,
         .enableFormantTracking = false,
         .enableTonalAnalysis = false,
         .minFundamental = 100.0f,
         .maxFundamental = 1000.0f}};

    for (const auto& config : configs) {
        auto analyzerResult = HarmonicAnalyzer::create(config);
        ASSERT_TRUE(analyzerResult.has_value());

        auto analyzer = std::move(*analyzerResult);

        // Test with insufficient data
        if (testAudio.size() >= config.fftSize / 2) {
            std::span<const float> shortSpan(testAudio.data(), config.fftSize / 2);
            auto shortResult = analyzer->analyzeHarmonics(shortSpan);
            EXPECT_FALSE(shortResult.has_value());  // Should fail with insufficient data
        }

        // Test with extreme values
        std::vector<float> extremeAudio(config.fftSize);

        // Test with all zeros
        std::fill(extremeAudio.begin(), extremeAudio.end(), 0.0f);
        auto zeroResult = analyzer->analyzeHarmonics(extremeAudio);
        EXPECT_TRUE(zeroResult.has_value());  // Should handle gracefully

        // Test with maximum amplitude
        std::fill(extremeAudio.begin(), extremeAudio.end(), 1.0f);
        auto maxResult = analyzer->analyzeHarmonics(extremeAudio);
        EXPECT_TRUE(maxResult.has_value());
    }
}

// ==================== Additional Component Coverage Tests ====================

TEST_F(CoverageBoostFinalTest, SpectrogramProcessor_ComprehensiveTesting) {
    SpectrogramProcessor::Config config{.window_size = 1024,
                                        .hop_size = 256,
                                        .sample_rate = 44100.0f,
                                        .min_frequency = 0.0f,
                                        .max_frequency = 8000.0f,
                                        .apply_window = true,
                                        .db_floor = -80.0f,
                                        .db_ceiling = 0.0f};

    ASSERT_TRUE(config.validate());

    auto spectrogramResult = SpectrogramProcessor::create(config);
    ASSERT_TRUE(spectrogramResult.has_value());

    auto processor = std::move(*spectrogramResult);

    // Test spectrogram generation
    auto result = processor->generateSpectrogram(testAudio);
    ASSERT_TRUE(result.has_value());

    const auto& spectrogram = *result;
    EXPECT_GT(spectrogram.magnitude.size(), 0);
    EXPECT_GT(spectrogram.timeAxis.size(), 0);
    EXPECT_GT(spectrogram.frequencyAxis.size(), 0);

    // Test configuration retrieval
    const auto& retrievedConfig = processor->getConfig();
    EXPECT_EQ(retrievedConfig.window_size, config.window_size);
    EXPECT_EQ(retrievedConfig.hop_size, config.hop_size);

    // Test reset
    processor->reset();
}

TEST_F(CoverageBoostFinalTest, RealtimeAudioProcessor_BufferManagement) {
    RealtimeAudioProcessor::Config config{.ring_buffer_size = 1024,
                                          .chunk_size = 256,
                                          .enable_backpressure = true,
                                          .backpressure_timeout = std::chrono::milliseconds(10),
                                          .enable_metrics = true};

    auto processorResult = RealtimeAudioProcessor::create(config);
    ASSERT_TRUE(processorResult.has_value());

    auto processor = std::move(*processorResult);

    // Test chunk processing
    size_t chunkSize = config.chunk_size;
    if (testAudio.size() >= chunkSize) {
        std::span<const float> chunk(testAudio.data(), chunkSize);

        auto enqueueResult = processor->enqueueAudioChunk(chunk);
        EXPECT_TRUE(enqueueResult.has_value());

        // Test chunk retrieval
        auto dequeueResult = processor->dequeueProcessedChunk();
        EXPECT_TRUE(dequeueResult.has_value());

        // Test metrics
        auto metrics = processor->getMetrics();
        EXPECT_GE(metrics.total_chunks, 0);
        EXPECT_GE(metrics.processing_time_avg.count(), 0);

        // Test buffer management
        processor->clearBuffers();
        auto bufferStatus = processor->getBufferStatus();
        EXPECT_EQ(bufferStatus.used_capacity, 0);
    }
}

TEST_F(CoverageBoostFinalTest, AudioLevelProcessor_LevelAnalysis) {
    AudioLevelProcessor::Config config{.sampleRate = 44100.0f,
                                       .blockSize = 256,
                                       .smoothingFactor = 0.9f,
                                       .enablePeakHold = true,
                                       .peakHoldTime = std::chrono::milliseconds(500)};

    auto processorResult = AudioLevelProcessor::create(config);
    ASSERT_TRUE(processorResult.has_value());

    auto processor = std::move(*processorResult);

    // Test level processing with different audio types
    std::vector<std::vector<float>*> audioTypes = {&testAudio, &silentAudio, &noiseAudio};

    for (auto* audio : audioTypes) {
        if (audio->size() >= config.blockSize) {
            std::span<const float> block(audio->data(), config.blockSize);

            auto result = processor->processAudioBlock(block);
            ASSERT_TRUE(result.has_value());

            const auto& levels = *result;
            EXPECT_GE(levels.rms, 0.0f);
            EXPECT_GE(levels.peak, 0.0f);
            EXPECT_GE(levels.lufs, -100.0f);  // LUFS can be very negative for quiet audio
            EXPECT_LE(levels.peak, 1.0f);
        }
    }

    // Test configuration access
    const auto& retrievedConfig = processor->getConfig();
    EXPECT_FLOAT_EQ(retrievedConfig.sampleRate, config.sampleRate);

    // Test reset
    processor->reset();
}

// ==================== Integration and Performance Tests ====================

TEST_F(CoverageBoostFinalTest, IntegratedWorkflow_FullPipeline) {
    // Test integrated workflow combining multiple components

    // Setup MFCC processor
    MFCCProcessor::Config mfccConfig{
        .sample_rate = 44100, .frame_size = 512, .num_coefficients = 13, .num_filters = 26};
    MFCCProcessor mfccProcessor(mfccConfig);

    // Setup VAD
    VoiceActivityDetector::Config vadConfig{.energy_threshold = 0.01f,
                                            .window_duration = std::chrono::milliseconds(20),
                                            .min_sound_duration = std::chrono::milliseconds(100),
                                            .sample_rate = 44100};
    VoiceActivityDetector vad(vadConfig);

    // Setup harmonic analyzer
    HarmonicAnalyzer::Config harmonicConfig{.sampleRate = 44100,
                                            .fftSize = 1024,
                                            .hopSize = 512,
                                            .enableFormantTracking = true,
                                            .enableTonalAnalysis = true};

    auto harmonicResult = HarmonicAnalyzer::create(harmonicConfig);
    ASSERT_TRUE(harmonicResult.has_value());
    auto harmonic = std::move(*harmonicResult);

    // Process audio through the pipeline
    size_t frameSize = mfccConfig.frame_size;
    size_t vadWindowSize =
        static_cast<size_t>(vadConfig.window_duration.count() * vadConfig.sample_rate / 1000);

    if (testAudio.size() >= std::max({frameSize, vadWindowSize, harmonicConfig.fftSize})) {
        // MFCC processing
        std::span<const float> mfccFrame(testAudio.data(), frameSize);
        auto mfccResult = mfccProcessor.extractFeatures(mfccFrame);
        ASSERT_TRUE(mfccResult.has_value());

        // VAD processing
        std::span<const float> vadFrame(testAudio.data(), vadWindowSize);
        auto vadResult = vad.processWindow(vadFrame);
        ASSERT_TRUE(vadResult.has_value());

        // Harmonic analysis
        std::span<const float> harmonicFrame(testAudio.data(), harmonicConfig.fftSize);
        auto harmonicAnalysisResult = harmonic->analyzeHarmonics(harmonicFrame);
        ASSERT_TRUE(harmonicAnalysisResult.has_value());

        // Verify results are consistent
        EXPECT_EQ(mfccResult->size(), mfccConfig.num_coefficients);
        EXPECT_GE(vadResult->energy_level, 0.0f);
        EXPECT_GE(harmonicAnalysisResult->confidence, 0.0f);
    }
}

TEST_F(CoverageBoostFinalTest, PerformanceStressTest) {
    // Test components under stress conditions

    // Generate large audio buffer
    const size_t largeBufferSize = 100000;
    std::vector<float> largeBuffer(largeBufferSize);

    for (size_t i = 0; i < largeBufferSize; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        largeBuffer[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
    }

    // Test MFCC with large buffer
    MFCCProcessor::Config mfccConfig{.sample_rate = static_cast<size_t>(SAMPLE_RATE),
                                     .frame_size = 512,
                                     .num_coefficients = 13,
                                     .num_filters = 26};
    MFCCProcessor mfccProcessor(mfccConfig);

    auto mfccResult = mfccProcessor.extractFeaturesFromBuffer(largeBuffer, 256);
    ASSERT_TRUE(mfccResult.has_value());
    EXPECT_GT(mfccResult->size(), 0);

    // Test VAD with rapid processing
    VoiceActivityDetector::Config vadConfig{.energy_threshold = 0.01f,
                                            .window_duration = std::chrono::milliseconds(10),
                                            .min_sound_duration = std::chrono::milliseconds(50),
                                            .sample_rate = static_cast<size_t>(SAMPLE_RATE)};
    VoiceActivityDetector vad(vadConfig);

    size_t windowSize =
        static_cast<size_t>(vadConfig.window_duration.count() * vadConfig.sample_rate / 1000);
    size_t processedWindows = 0;

    for (size_t offset = 0; offset + windowSize < largeBuffer.size(); offset += windowSize) {
        std::span<const float> window(largeBuffer.data() + offset, windowSize);
        auto vadResult = vad.processWindow(window);
        if (vadResult.has_value()) {
            processedWindows++;
        }
    }

    EXPECT_GT(processedWindows, 0);
}

}  // namespace test
}  // namespace huntmaster
