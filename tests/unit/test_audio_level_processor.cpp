#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

#include "huntmaster/core/AudioLevelProcessor.h"

namespace huntmaster {

class AudioLevelProcessorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        config_.sampleRate = 44100.0f;
        config_.updateRateMs = 50.0f;
        config_.historySize = 10;
        // Use very fast attack/release times for testing (almost no smoothing)
        config_.rmsAttackTimeMs = 0.001f;
        config_.rmsReleaseTimeMs = 0.001f;
        config_.peakAttackTimeMs = 0.001f;
        config_.peakReleaseTimeMs = 0.001f;
        processor_ = std::make_unique<AudioLevelProcessor>(config_);
    }

    void TearDown() override { processor_.reset(); }

    AudioLevelProcessor::Config config_;
    std::unique_ptr<AudioLevelProcessor> processor_;
};

TEST_F(AudioLevelProcessorTest, InitializationTest) {
    EXPECT_TRUE(processor_->isInitialized());

    // Test invalid configuration
    AudioLevelProcessor::Config invalidConfig;
    invalidConfig.sampleRate = -1.0f;  // Invalid

    AudioLevelProcessor invalidProcessor(invalidConfig);
    EXPECT_FALSE(invalidProcessor.isInitialized());
}

TEST_F(AudioLevelProcessorTest, SilenceProcessingTest) {
    // Create silent audio (all zeros)
    std::vector<float> silentAudio(1024, 0.0f);

    auto result = processor_->processAudio(silentAudio, 1);

    ASSERT_TRUE(result.has_value());

    auto measurement = *result;
    EXPECT_EQ(measurement.rmsLinear, 0.0f);
    EXPECT_EQ(measurement.peakLinear, 0.0f);
    EXPECT_EQ(measurement.rmsDb, config_.dbFloor);  // Should be at silence floor
    EXPECT_EQ(measurement.peakDb, config_.dbFloor);
}

TEST_F(AudioLevelProcessorTest, SineWaveProcessingTest) {
    const float frequency = 440.0f;  // A4 note
    const float amplitude = 0.5f;    // Half amplitude
    const size_t numSamples = 1024;

    // Generate sine wave
    std::vector<float> sineWave(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        const float t = static_cast<float>(i) / config_.sampleRate;
        sineWave[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
    }

    auto result = processor_->processAudio(sineWave, 1);

    ASSERT_TRUE(result.has_value());

    auto measurement = *result;

    // For sine wave, RMS should be amplitude / sqrt(2)
    const float expectedRms = amplitude / std::sqrt(2.0f);
    const float tolerance = 0.05f;  // 5% tolerance

    EXPECT_NEAR(measurement.rmsLinear, expectedRms, tolerance);
    EXPECT_NEAR(measurement.peakLinear, amplitude, tolerance);

    // dB values should be reasonable
    EXPECT_GT(measurement.rmsDb, config_.dbFloor);
    EXPECT_GT(measurement.peakDb, config_.dbFloor);
    EXPECT_LT(measurement.rmsDb, config_.dbCeiling);
    EXPECT_LT(measurement.peakDb, config_.dbCeiling);
}

TEST_F(AudioLevelProcessorTest, MultiChannelProcessingTest) {
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

    auto result = processor_->processAudio(stereoAudio, numChannels);

    ASSERT_TRUE(result.has_value());

    auto measurement = *result;

    // Should process the average of both channels
    const float expectedAverage = (0.5f + 0.3f) / 2.0f;
    const float tolerance = 0.05f;

    EXPECT_NEAR(measurement.rmsLinear, expectedAverage, tolerance);
    EXPECT_NEAR(measurement.peakLinear, 0.5f, tolerance);  // Peak should be the max
}

TEST_F(AudioLevelProcessorTest, LevelHistoryTest) {
    const size_t numChunks = 5;
    const size_t chunkSize = 512;

    // Process multiple audio chunks
    for (size_t chunk = 0; chunk < numChunks; ++chunk) {
        std::vector<float> audio(chunkSize, static_cast<float>(chunk) * 0.1f);
        processor_->processAudio(audio, 1);

        // Small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Get history
    auto history = processor_->getLevelHistory();

    EXPECT_EQ(history.size(), numChunks);

    // History should be in reverse chronological order (newest first)
    for (size_t i = 1; i < history.size(); ++i) {
        EXPECT_GE(history[i - 1].timestamp, history[i].timestamp);
    }
}

TEST_F(AudioLevelProcessorTest, JsonExportTest) {
    // Process some audio
    std::vector<float> audio(512, 0.5f);
    auto result = processor_->processAudio(audio, 1);
    ASSERT_TRUE(result.has_value());

    // Test current level JSON export
    std::string json = processor_->exportToJson();

    // Should contain expected fields
    EXPECT_NE(json.find("\"rms\""), std::string::npos);
    EXPECT_NE(json.find("\"peak\""), std::string::npos);
    EXPECT_NE(json.find("\"rmsLinear\""), std::string::npos);
    EXPECT_NE(json.find("\"peakLinear\""), std::string::npos);
    EXPECT_NE(json.find("\"timestamp\""), std::string::npos);

    // Should be valid JSON format
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');

    // Test history JSON export
    std::string historyJson = processor_->exportHistoryToJson(5);
    EXPECT_EQ(historyJson.front(), '[');
    EXPECT_EQ(historyJson.back(), ']');
}

TEST_F(AudioLevelProcessorTest, ConfigUpdateTest) {
    // Update configuration
    AudioLevelProcessor::Config newConfig = config_;
    newConfig.historySize = 20;
    newConfig.dbFloor = -80.0f;

    bool success = processor_->updateConfig(newConfig);
    EXPECT_TRUE(success);

    auto retrievedConfig = processor_->getConfig();
    EXPECT_EQ(retrievedConfig.historySize, 20);
    EXPECT_EQ(retrievedConfig.dbFloor, -80.0f);

    // Test invalid config update
    AudioLevelProcessor::Config invalidConfig = config_;
    invalidConfig.sampleRate = -1.0f;

    success = processor_->updateConfig(invalidConfig);
    EXPECT_FALSE(success);
}

TEST_F(AudioLevelProcessorTest, ResetTest) {
    // Process some audio first
    std::vector<float> audio(512, 0.5f);
    processor_->processAudio(audio, 1);

    // Verify we have non-zero levels
    auto levelBefore = processor_->getCurrentLevel();
    EXPECT_GT(levelBefore.rmsLinear, 0.0f);

    // Reset processor
    processor_->reset();

    // Verify levels are reset
    auto levelAfter = processor_->getCurrentLevel();
    EXPECT_EQ(levelAfter.rmsLinear, 0.0f);
    EXPECT_EQ(levelAfter.peakLinear, 0.0f);

    // Verify history is cleared
    auto history = processor_->getLevelHistory();
    EXPECT_EQ(history.size(), 0);
}

TEST_F(AudioLevelProcessorTest, ErrorHandlingTest) {
    // Test empty audio data
    std::vector<float> emptyAudio;
    auto result = processor_->processAudio(emptyAudio, 1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AudioLevelProcessor::Error::INVALID_AUDIO_DATA);

    // Test invalid number of channels
    std::vector<float> audio(512, 0.5f);
    result = processor_->processAudio(audio, 0);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AudioLevelProcessor::Error::INVALID_AUDIO_DATA);

    result = processor_->processAudio(audio, 10);  // Too many channels
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AudioLevelProcessor::Error::INVALID_AUDIO_DATA);
}

// Test utility functions
TEST(AudioLevelUtilityTest, LinearToDbConversionTest) {
    // Test known conversions
    EXPECT_FLOAT_EQ(linearToDb(1.0f), 0.0f);                      // Full scale = 0 dB
    EXPECT_FLOAT_EQ(linearToDb(0.5f), 20.0f * std::log10(0.5f));  // Half amplitude
    EXPECT_FLOAT_EQ(linearToDb(0.0f), -60.0f);                    // Silence = floor

    // Test clamping
    EXPECT_FLOAT_EQ(linearToDb(2.0f, -60.0f, 6.0f), 6.0f);     // Clamp to ceiling
    EXPECT_FLOAT_EQ(linearToDb(-1.0f, -60.0f, 6.0f), -60.0f);  // Negative = floor
}

TEST(AudioLevelUtilityTest, DbToLinearConversionTest) {
    // Test known conversions
    EXPECT_FLOAT_EQ(dbToLinear(0.0f), 1.0f);      // 0 dB = full scale
    EXPECT_FLOAT_EQ(dbToLinear(-20.0f), 0.1f);    // -20 dB = 0.1 linear
    EXPECT_NEAR(dbToLinear(-6.0f), 0.5f, 0.01f);  // -6 dB ≈ 0.5 linear
}

}  // namespace huntmaster
