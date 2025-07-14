#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

#include "huntmaster/core/WaveformGenerator.h"

namespace huntmaster {

class WaveformGeneratorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        config_.sampleRate = 44100.0f;
        config_.maxSamples = 4096;
        config_.downsampleRatio = 16;
        config_.updateRateMs = 50.0f;
        config_.enablePeakHold = true;
        config_.enableRmsOverlay = true;
        generator_ = std::make_unique<WaveformGenerator>(config_);
    }

    void TearDown() override { generator_.reset(); }

    WaveformGenerator::Config config_;
    std::unique_ptr<WaveformGenerator> generator_;
};

TEST_F(WaveformGeneratorTest, InitializationTest) {
    EXPECT_TRUE(generator_->isInitialized());

    auto config = generator_->getConfig();
    EXPECT_EQ(config.sampleRate, 44100.0f);
    EXPECT_EQ(config.downsampleRatio, 16);
    EXPECT_TRUE(config.enablePeakHold);
    EXPECT_TRUE(config.enableRmsOverlay);

    // Test invalid configuration
    WaveformGenerator::Config invalidConfig;
    invalidConfig.sampleRate = -1.0f;  // Invalid

    WaveformGenerator invalidGenerator(invalidConfig);
    EXPECT_FALSE(invalidGenerator.isInitialized());
}

TEST_F(WaveformGeneratorTest, SilenceProcessingTest) {
    // Create silent audio (all zeros)
    std::vector<float> silentAudio(1024, 0.0f);

    auto result = generator_->processAudio(silentAudio, 1);

    ASSERT_TRUE(result.isOk());

    auto waveformData = *result;
    EXPECT_GE(waveformData.samples.size(), 0);  // Should have some samples (downsampled)
    EXPECT_EQ(waveformData.maxAmplitude, 0.0f);
    EXPECT_EQ(waveformData.rmsAmplitude, 0.0f);

    // All downsampled samples should be zero
    for (float sample : waveformData.samples) {
        EXPECT_EQ(sample, 0.0f);
    }

    // Peak envelope should be zero
    if (waveformData.peaks.size() > 0) {
        for (float peak : waveformData.peaks) {
            EXPECT_EQ(peak, 0.0f);
        }
    }
}

TEST_F(WaveformGeneratorTest, SineWaveProcessingTest) {
    const float frequency = 440.0f;  // A4 note
    const float amplitude = 0.5f;    // Half amplitude
    const size_t numSamples = 2048;  // Enough samples for multiple downsample windows

    // Generate sine wave
    std::vector<float> sineWave(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        const float t = static_cast<float>(i) / config_.sampleRate;
        sineWave[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
    }

    auto result = generator_->processAudio(sineWave, 1);

    ASSERT_TRUE(result.isOk());

    auto waveformData = *result;

    // Should have downsampled data
    const size_t expectedDownsampledSize = numSamples / config_.downsampleRatio;
    EXPECT_GT(waveformData.samples.size(), 0);
    EXPECT_LE(waveformData.samples.size(), expectedDownsampledSize + 1);  // Allow for rounding

    // Max amplitude should be close to input amplitude
    const float tolerance = 0.1f;
    EXPECT_NEAR(waveformData.maxAmplitude, amplitude, tolerance);

    // RMS amplitude should be reasonable (for sine wave: amplitude / sqrt(2))
    const float expectedRms = amplitude / std::sqrt(2.0f);
    EXPECT_NEAR(waveformData.rmsAmplitude, expectedRms, tolerance);

    // Peak envelope should have reasonable values
    if (!waveformData.peaks.empty()) {
        for (float peak : waveformData.peaks) {
            EXPECT_GE(peak, 0.0f);
            EXPECT_LE(peak, amplitude + tolerance);
        }
    }

    // RMS envelope should have reasonable values
    if (!waveformData.rmsEnvelope.empty()) {
        for (float rms : waveformData.rmsEnvelope) {
            EXPECT_GE(rms, 0.0f);
            EXPECT_LE(rms, amplitude + tolerance);
        }
    }
}

TEST_F(WaveformGeneratorTest, MultiChannelProcessingTest) {
    const size_t numSamples = 1024;
    const int numChannels = 2;

    // Create stereo audio (interleaved)
    std::vector<float> stereoAudio(numSamples * numChannels);
    for (size_t i = 0; i < numSamples; ++i) {
        // Left channel: 0.5 amplitude
        stereoAudio[i * 2] = 0.5f;
        // Right channel: 0.3 amplitude
        stereoAudio[i * 2 + 1] = 0.3f;
    }

    auto result = generator_->processAudio(stereoAudio, numChannels);

    ASSERT_TRUE(result.isOk());

    auto waveformData = *result;

    // Should process the average of both channels
    const float expectedMax = 0.5f;  // Peak should be the maximum
    const float tolerance = 0.05f;

    EXPECT_NEAR(waveformData.maxAmplitude, expectedMax, tolerance);

    // Should have downsampled data
    EXPECT_GT(waveformData.samples.size(), 0);
}

TEST_F(WaveformGeneratorTest, BufferManagementTest) {
    const size_t chunkSize = 512;
    const size_t numChunks = 20;  // More chunks than buffer can hold

    // Process multiple chunks to test circular buffer behavior
    for (size_t chunk = 0; chunk < numChunks; ++chunk) {
        std::vector<float> audio(chunkSize, static_cast<float>(chunk) * 0.1f);
        auto result = generator_->processAudio(audio, 1);
        ASSERT_TRUE(result.isOk());
    }

    // Check buffer stats
    auto [used, capacity] = generator_->getBufferStats();
    EXPECT_LE(used, capacity);  // Should not exceed capacity

    // Get complete waveform
    auto completeWaveform = generator_->getCompleteWaveform();
    EXPECT_GT(completeWaveform.samples.size(), 0);
    EXPECT_LE(completeWaveform.samples.size(), capacity);
}

TEST_F(WaveformGeneratorTest, JsonExportTest) {
    // Process some audio
    std::vector<float> audio(1024, 0.5f);
    auto result = generator_->processAudio(audio, 1);
    ASSERT_TRUE(result.isOk());

    // Test JSON export
    std::string json = generator_->exportToJson(true);

    // Should contain expected fields
    EXPECT_NE(json.find("\"maxAmplitude\""), std::string::npos);
    EXPECT_NE(json.find("\"rmsAmplitude\""), std::string::npos);
    EXPECT_NE(json.find("\"sampleCount\""), std::string::npos);
    EXPECT_NE(json.find("\"sampleRate\""), std::string::npos);
    EXPECT_NE(json.find("\"downsampleRatio\""), std::string::npos);
    EXPECT_NE(json.find("\"timestamp\""), std::string::npos);
    EXPECT_NE(json.find("\"samples\""), std::string::npos);

    // Should be valid JSON format
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');

    // Test export without raw samples
    std::string jsonNoSamples = generator_->exportToJson(false);
    EXPECT_EQ(jsonNoSamples.find("\"samples\""), std::string::npos);
}

TEST_F(WaveformGeneratorTest, DisplayExportTest) {
    // Process some audio
    const size_t audioSize = 2048;
    std::vector<float> audio(audioSize);

    // Generate varying amplitude audio
    for (size_t i = 0; i < audioSize; ++i) {
        audio[i] = std::sin(2.0f * M_PI * i / 100.0f) * 0.5f;
    }

    auto result = generator_->processAudio(audio, 1);
    ASSERT_TRUE(result.isOk());

    // Test display export for different display widths
    const std::vector<size_t> displayWidths = {100, 256, 512, 800};

    for (size_t width : displayWidths) {
        std::string displayJson = generator_->exportForDisplay(width, true);

        // Should contain display-specific fields
        EXPECT_NE(displayJson.find("\"displayWidth\":" + std::to_string(width)), std::string::npos);
        EXPECT_NE(displayJson.find("\"samplesPerPixel\""), std::string::npos);
        EXPECT_NE(displayJson.find("\"samples\""), std::string::npos);

        // Should be valid JSON format
        EXPECT_EQ(displayJson.front(), '{');
        EXPECT_EQ(displayJson.back(), '}');
    }
}

TEST_F(WaveformGeneratorTest, ZoomLevelTest) {
    // Process some initial audio
    std::vector<float> audio(1024, 0.5f);
    generator_->processAudio(audio, 1);

    // Test different zoom levels
    const std::vector<float> zoomLevels = {0.5f, 1.0f, 2.0f, 4.0f};

    for (float zoom : zoomLevels) {
        generator_->setZoomLevel(zoom);

        // Process more audio with new zoom level
        auto result = generator_->processAudio(audio, 1);
        ASSERT_TRUE(result.isOk());

        // Higher zoom should generally produce more detailed data
        // (though this depends on the specific implementation)
        auto waveformData = *result;
        EXPECT_GT(waveformData.samples.size(), 0);
    }
}

TEST_F(WaveformGeneratorTest, WaveformRangeTest) {
    // Process some audio with time-varying content
    const size_t totalSamples = 4096;
    std::vector<float> audio(totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        // Varying amplitude over time
        const float timeRatio = static_cast<float>(i) / totalSamples;
        audio[i] = timeRatio * 0.5f;  // Linearly increasing amplitude
    }

    auto result = generator_->processAudio(audio, 1);
    ASSERT_TRUE(result.isOk());

    // Test getting specific time ranges
    const float totalTimeMs = totalSamples * 1000.0f / config_.sampleRate;
    const float halfTimeMs = totalTimeMs / 2.0f;

    auto firstHalf = generator_->getWaveformRange(0.0f, halfTimeMs);
    auto secondHalf = generator_->getWaveformRange(halfTimeMs, halfTimeMs);

    EXPECT_GT(firstHalf.samples.size(), 0);
    EXPECT_GT(secondHalf.samples.size(), 0);

    // Second half should generally have higher amplitude (due to our test signal)
    if (!firstHalf.samples.empty() && !secondHalf.samples.empty()) {
        float firstHalfAvg = 0.0f;
        float secondHalfAvg = 0.0f;

        for (float sample : firstHalf.samples) {
            firstHalfAvg += std::abs(sample);
        }
        firstHalfAvg /= firstHalf.samples.size();

        for (float sample : secondHalf.samples) {
            secondHalfAvg += std::abs(sample);
        }
        secondHalfAvg /= secondHalf.samples.size();

        EXPECT_GT(secondHalfAvg, firstHalfAvg);
    }
}

TEST_F(WaveformGeneratorTest, ResetTest) {
    // Process some audio first
    std::vector<float> audio(1024, 0.5f);
    generator_->processAudio(audio, 1);

    // Verify we have data
    auto waveformBefore = generator_->getCompleteWaveform();
    EXPECT_GT(waveformBefore.samples.size(), 0);

    // Reset generator
    generator_->reset();

    // Verify data is cleared
    auto waveformAfter = generator_->getCompleteWaveform();
    EXPECT_EQ(waveformAfter.samples.size(), 0);
    EXPECT_EQ(waveformAfter.maxAmplitude, 0.0f);
    EXPECT_EQ(waveformAfter.rmsAmplitude, 0.0f);

    // Verify buffer stats are reset
    auto [used, capacity] = generator_->getBufferStats();
    EXPECT_EQ(used, 0);
}

TEST_F(WaveformGeneratorTest, ConfigUpdateTest) {
    // Update configuration
    WaveformGenerator::Config newConfig = config_;
    newConfig.downsampleRatio = 32;  // Different ratio
    newConfig.enablePeakHold = false;

    bool success = generator_->updateConfig(newConfig);
    EXPECT_TRUE(success);

    auto retrievedConfig = generator_->getConfig();
    EXPECT_EQ(retrievedConfig.downsampleRatio, 32);
    EXPECT_FALSE(retrievedConfig.enablePeakHold);

    // Test invalid config update
    WaveformGenerator::Config invalidConfig = config_;
    invalidConfig.sampleRate = -1.0f;

    success = generator_->updateConfig(invalidConfig);
    EXPECT_FALSE(success);
}

TEST_F(WaveformGeneratorTest, ErrorHandlingTest) {
    // Test empty audio data
    std::vector<float> emptyAudio;
    auto result = generator_->processAudio(emptyAudio, 1);
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error(), WaveformGenerator::Error::INVALID_AUDIO_DATA);

    // Test invalid number of channels
    std::vector<float> audio(512, 0.5f);
    result = generator_->processAudio(audio, 0);
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error(), WaveformGenerator::Error::INVALID_AUDIO_DATA);

    result = generator_->processAudio(audio, 10);  // Too many channels
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error(), WaveformGenerator::Error::INVALID_AUDIO_DATA);
}

// Test utility functions
TEST(WaveformUtilityTest, DownsampleRatioCalculationTest) {
    // Test optimal downsampling calculation
    const float sampleRate = 44100.0f;

    // Test case 1: More samples than pixels
    size_t ratio1 = calculateOptimalDownsampleRatio(44100, 800, sampleRate);
    EXPECT_GT(ratio1, 1);
    EXPECT_EQ(ratio1, 44100 / 800);

    // Test case 2: Fewer samples than pixels
    size_t ratio2 = calculateOptimalDownsampleRatio(400, 800, sampleRate);
    EXPECT_EQ(ratio2, 1);  // Should be minimum

    // Test case 3: Edge cases
    size_t ratio3 = calculateOptimalDownsampleRatio(0, 800, sampleRate);
    EXPECT_EQ(ratio3, 1);

    size_t ratio4 = calculateOptimalDownsampleRatio(1000, 0, sampleRate);
    EXPECT_EQ(ratio4, 1);
}

TEST(WaveformUtilityTest, PeakEnvelopeGenerationTest) {
    // Create test signal with known peaks
    std::vector<float> signal = {0.1f, 0.8f, 0.2f, -0.9f, 0.3f, 0.7f, -0.5f};

    auto envelope = generatePeakEnvelope(signal, 3);

    EXPECT_EQ(envelope.size(), signal.size());

    // Check that peak values are reasonable
    for (size_t i = 0; i < envelope.size(); ++i) {
        EXPECT_GE(envelope[i], 0.0f);
        EXPECT_GE(envelope[i], std::abs(signal[i]));  // Peak should be >= sample amplitude
    }
}

TEST(WaveformUtilityTest, RmsEnvelopeGenerationTest) {
    // Create test signal
    std::vector<float> signal = {0.5f, -0.5f, 0.8f, -0.8f, 0.3f, -0.3f};

    auto envelope = generateRmsEnvelope(signal, 3);

    EXPECT_EQ(envelope.size(), signal.size());

    // Check that RMS values are reasonable
    for (size_t i = 0; i < envelope.size(); ++i) {
        EXPECT_GE(envelope[i], 0.0f);  // RMS is always non-negative
        EXPECT_LE(envelope[i], 1.0f);  // Should be reasonable for our test signal
    }
}

}  // namespace huntmaster
