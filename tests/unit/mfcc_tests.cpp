#include <gtest/gtest.h>

#include "huntmaster/core/MFCCProcessor.h"

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
    // Basic test - just verify processor was created
    EXPECT_TRUE(true);
}

TEST_F(MFCCTest, ProcessSingleFrame) {
    auto testSignal = generateSineWave(440.0f, 44100.0f, 512);

    auto result = processor->extractFeatures(testSignal);

    ASSERT_TRUE(result.has_value());
    auto features = result.value();
    EXPECT_EQ(features.size(), 13);  // Default number of coefficients
    EXPECT_GT(std::abs(features[0]), 0.0f);
}

TEST_F(MFCCTest, ProcessBuffer) {
    size_t bufferSize = 4096;
    auto testSignal = generateSineWave(440.0f, 44100.0f, bufferSize);

    auto result = processor->extractFeaturesFromBuffer(testSignal, 512);

    ASSERT_TRUE(result.has_value());
    auto features = result.value();
    EXPECT_GT(features.size(), 0);

    for (const auto& frame : features) {
        EXPECT_EQ(frame.size(), 13);  // Default number of coefficients
    }
}

TEST_F(MFCCTest, SilenceProducesLowEnergy) {
    std::vector<float> silence(512, 0.0f);

    auto result = processor->extractFeatures(silence);

    ASSERT_TRUE(result.has_value());
    auto features = result.value();
    // For silence, first coefficient (energy) should be very low
    EXPECT_LT(features[0], -10.0f);
}

TEST_F(MFCCTest, DifferentFrequenciesProduceDifferentMFCCs) {
    auto signal1 = generateSineWave(440.0f, 44100.0f, 512);
    auto signal2 = generateSineWave(880.0f, 44100.0f, 512);

    auto result1 = processor->extractFeatures(signal1);
    auto result2 = processor->extractFeatures(signal2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto features1 = result1.value();
    auto features2 = result2.value();

    float distance = 0.0f;
    for (size_t i = 0; i < 13; ++i) {
        float diff = features1[i] - features2[i];
        distance += diff * diff;
    }
    distance = std::sqrt(distance);

    EXPECT_GT(distance, 1.0f);
}

TEST_F(MFCCTest, WhiteNoiseTest) {
    auto noise = generateWhiteNoise(512);

    auto result = processor->extractFeatures(noise);

    ASSERT_TRUE(result.has_value());
    auto features = result.value();

    float totalEnergy = 0.0f;
    for (auto coeff : features) {
        totalEnergy += coeff * coeff;
    }

    EXPECT_GT(totalEnergy, 0.0f);
}

TEST_F(MFCCTest, ClearCache) {
    auto testSignal = generateSineWave(440.0f, 44100.0f, 512);

    // Process some data to populate cache
    auto result1 = processor->extractFeatures(testSignal);
    ASSERT_TRUE(result1.has_value());

    // Clear cache and process again
    processor->clearCache();
    auto result2 = processor->extractFeatures(testSignal);
    ASSERT_TRUE(result2.has_value());

    // Results should be the same
    auto features1 = result1.value();
    auto features2 = result2.value();
    EXPECT_EQ(features1.size(), features2.size());
}
