#include <gtest/gtest.h>
#include "huntmaster_engine/MFCCProcessor.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <numeric>

using namespace huntmaster;

class MFCCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create processor with an explicit config object.
        MFCCProcessor::Config config;
        processor = std::make_unique<MFCCProcessor>(config);
    }
    
    // Generate a sine wave for testing
    std::vector<float> generateSineWave(float frequency, float sampleRate, size_t numSamples) {
        std::vector<float> samples(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = std::sin(2.0f * M_PI * frequency * static_cast<float>(i) / sampleRate);
        }
        return samples;
    }
    
    // Generate white noise for testing
    std::vector<float> generateWhiteNoise(size_t numSamples) {
        std::vector<float> samples(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = (rand() / float(RAND_MAX)) * 2.0f - 1.0f;
        }
        return samples;
    }
    
    std::unique_ptr<MFCCProcessor> processor;
};

TEST_F(MFCCTest, CanCreateProcessor) {
    ASSERT_NE(processor, nullptr);
    auto config = processor->getConfig();
    EXPECT_EQ(config.sampleRate, 44100.0f);
    EXPECT_EQ(config.frameSize, 2048);
}

TEST_F(MFCCTest, ProcessSingleFrame) {
    auto testSignal = generateSineWave(440.0f, 44100.0f, 2048);
    
    auto frame = processor->processFrame(testSignal.data());
    
    EXPECT_EQ(frame.coefficients.size(), MFCCProcessor::DEFAULT_NUM_COEFFS);
    EXPECT_GT(std::abs(frame.coefficients[0]), 0.0f);
    EXPECT_GT(frame.energy, 0.0f);
}

TEST_F(MFCCTest, ProcessBuffer) {
    size_t bufferSize = 4096;
    auto testSignal = generateSineWave(440.0f, 44100.0f, bufferSize);
    
    auto frames = processor->processBuffer(testSignal.data(), bufferSize);
    
    size_t expectedFrames = (bufferSize - 2048) / 512 + 1;
    EXPECT_EQ(frames.size(), expectedFrames);
    
    for (size_t i = 0; i < frames.size(); ++i) {
        EXPECT_EQ(frames[i].frameIndex, i);
    }
}

TEST_F(MFCCTest, SilenceProducesLowEnergy) {
    std::vector<float> silence(2048, 0.0f);
    
    auto frame = processor->processFrame(silence.data());
    
    EXPECT_LT(frame.energy, -20.0f);
}

TEST_F(MFCCTest, DifferentFrequenciesProduceDifferentMFCCs) {
    auto signal1 = generateSineWave(440.0f, 44100.0f, 2048);
    auto signal2 = generateSineWave(880.0f, 44100.0f, 2048);
    
    auto frame1 = processor->processFrame(signal1.data());
    auto frame2 = processor->processFrame(signal2.data());
    
    float distance = 0.0f;
    for (size_t i = 0; i < MFCCProcessor::DEFAULT_NUM_COEFFS; ++i) {
        float diff = frame1.coefficients[i] - frame2.coefficients[i];
        distance += diff * diff;
    }
    distance = std::sqrt(distance);
    
    EXPECT_GT(distance, 1.0f);
}

TEST_F(MFCCTest, WhiteNoiseTest) {
    auto noise = generateWhiteNoise(2048);
    
    auto frame = processor->processFrame(noise.data());
    
    float totalEnergy = 0.0f;
    for (auto coeff : frame.coefficients) {
        totalEnergy += coeff * coeff;
    }
    
    EXPECT_GT(totalEnergy, 0.0f);
}

TEST_F(MFCCTest, ResetClearsFrameCounter) {
    auto testSignal = generateSineWave(440.0f, 44100.0f, 2048);
    
    auto frame1 = processor->processFrame(testSignal.data());
    EXPECT_EQ(frame1.frameIndex, 0);
    
    auto frame2 = processor->processFrame(testSignal.data());
    EXPECT_EQ(frame2.frameIndex, 1);
    
    processor->reset();
    auto frame3 = processor->processFrame(testSignal.data());
    EXPECT_EQ(frame3.frameIndex, 0);
}
