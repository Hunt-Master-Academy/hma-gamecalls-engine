#include <gtest/gtest.h>

#include <iostream>
#include <random>

#include "huntmaster/core/HuntmasterEngine.h"
#include "huntmaster/core/WaveformGenerator.h"

namespace huntmaster {

class EdgeCaseTest : public ::testing::Test {
   protected:
    HuntmasterEngine engine;
    WaveformGenerator::Config config;
    std::unique_ptr<WaveformGenerator> generator;

    void SetUp() override {
        config.sampleRate = 44100;
        config.maxSamples = 4096;
        config.downsampleRatio = 16;
        generator = std::make_unique<WaveformGenerator>(config);
    }
};

// Zero-length buffer
TEST_F(EdgeCaseTest, ZeroLengthBuffer) {
    std::vector<float> emptyAudio;
    auto result = generator->processAudio(emptyAudio, 1);
    std::cout << "[ZeroLengthBuffer] has_value: " << result.has_value() << std::endl;
    if (!result.has_value()) {
        std::cout << "[ZeroLengthBuffer] error: " << static_cast<int>(result.error()) << std::endl;
    }
    EXPECT_FALSE(result.has_value());
}

// Single-sample buffer
TEST_F(EdgeCaseTest, SingleSampleBuffer) {
    WaveformGenerator::Config singleSampleConfig = config;
    singleSampleConfig.downsampleRatio = 1;
    auto singleSampleGen = std::make_unique<WaveformGenerator>(singleSampleConfig);

    std::vector<float> audio(1, 0.7f);
    auto result = singleSampleGen->processAudio(audio, 1);
    std::cout << "[SingleSampleBuffer] has_value: " << result.has_value() << std::endl;
    auto waveform = singleSampleGen->getCompleteWaveform();
    std::cout << "[SingleSampleBuffer] samples: " << waveform.samples.size()
              << ", maxAmplitude: " << waveform.maxAmplitude << std::endl;
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(waveform.samples.size(), 1);
}

// Maximum buffer size
TEST_F(EdgeCaseTest, MaxBufferSize) {
    std::vector<float> audio(config.maxSamples, 1.0f);
    auto result = generator->processAudio(audio, 1);
    std::cout << "[MaxBufferSize] has_value: " << result.has_value() << std::endl;
    auto waveform = generator->getCompleteWaveform();
    std::cout << "[MaxBufferSize] samples: " << waveform.samples.size()
              << ", maxAmplitude: " << waveform.maxAmplitude << std::endl;
    EXPECT_TRUE(result.has_value());
    EXPECT_LE(waveform.samples.size(), config.maxSamples);
}

// Non-standard buffer size
TEST_F(EdgeCaseTest, NonStandardBufferSize) {
    std::vector<float> audio(513, 0.3f);
    auto result = generator->processAudio(audio, 1);
    std::cout << "[NonStandardBufferSize] has_value: " << result.has_value() << std::endl;
    auto waveform = generator->getCompleteWaveform();
    std::cout << "[NonStandardBufferSize] samples: " << waveform.samples.size()
              << ", maxAmplitude: " << waveform.maxAmplitude << std::endl;
    EXPECT_TRUE(result.has_value());
}

// Multi-channel audio (unsupported)
TEST_F(EdgeCaseTest, MultiChannelAudio) {
    std::vector<float> audio(1024, 0.5f);
    auto result = generator->processAudio(audio, 3);  // 3 channels, should error
    std::cout << "[MultiChannelAudio] has_value: " << result.has_value() << std::endl;
    if (!result.has_value()) {
        std::cout << "[MultiChannelAudio] error: " << static_cast<int>(result.error()) << std::endl;
    }
    EXPECT_FALSE(result.has_value());
}

// All silence
TEST_F(EdgeCaseTest, AllSilence) {
    std::vector<float> audio(1024, 0.0f);
    auto result = generator->processAudio(audio, 1);
    auto waveform = generator->getCompleteWaveform();
    std::cout << "[AllSilence] maxAmplitude: " << waveform.maxAmplitude
              << ", rmsAmplitude: " << waveform.rmsAmplitude << std::endl;
    ASSERT_TRUE(!waveform.samples.empty() || waveform.maxAmplitude == 0.0f);
    EXPECT_EQ(waveform.maxAmplitude, 0.0f);
    EXPECT_EQ(waveform.rmsAmplitude, 0.0f);
}

// All clipped
TEST_F(EdgeCaseTest, AllClipped) {
    std::vector<float> audio(1024, 1.0f);
    auto result = generator->processAudio(audio, 1);
    auto waveform = generator->getCompleteWaveform();
    EXPECT_NEAR(waveform.maxAmplitude, 1.0f, 1e-5);
    std::cout << "[AllClipped] maxAmplitude: " << waveform.maxAmplitude
              << ", rmsAmplitude: " << waveform.rmsAmplitude << std::endl;
    EXPECT_NEAR(waveform.maxAmplitude, 1.0f, 1e-5);
}

// Random noise
TEST_F(EdgeCaseTest, RandomNoise) {
    std::vector<float> audio(1024);
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (auto& sample : audio) sample = dist(rng);
    auto result = generator->processAudio(audio, 1);
    auto waveform = generator->getCompleteWaveform();
    std::cout << "[RandomNoise] maxAmplitude: " << waveform.maxAmplitude
              << ", rmsAmplitude: " << waveform.rmsAmplitude << std::endl;
    EXPECT_GT(waveform.maxAmplitude, 0.0f);
}

// Impulse
TEST_F(EdgeCaseTest, Impulse) {
    WaveformGenerator::Config impulseConfig = config;
    impulseConfig.downsampleRatio = 1;
    auto impulseGen = std::make_unique<WaveformGenerator>(impulseConfig);

    std::vector<float> audio(1024, 0.0f);
    audio[512] = 1.0f;
    auto result = impulseGen->processAudio(audio, 1);
    auto waveform = impulseGen->getCompleteWaveform();
    std::cout << "[Impulse] maxAmplitude: " << waveform.maxAmplitude << std::endl;
    EXPECT_EQ(waveform.maxAmplitude, 1.0f);
}

// Invalid config
TEST_F(EdgeCaseTest, InvalidConfig) {
    WaveformGenerator::Config badConfig;
    badConfig.sampleRate = -1.0f;
    WaveformGenerator badGen(badConfig);
    std::vector<float> audio(512, 0.1f);
    auto result = badGen.processAudio(audio, 1);
    std::cout << "[InvalidConfig] has_value: " << result.has_value() << std::endl;
    if (!result.has_value()) {
        std::cout << "[InvalidConfig] error: " << static_cast<int>(result.error()) << std::endl;
    }
    EXPECT_FALSE(result.has_value());
}

}  // namespace huntmaster