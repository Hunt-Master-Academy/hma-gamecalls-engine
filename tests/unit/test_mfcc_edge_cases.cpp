/**
 * @file test_mfcc_edge_cases.cpp
 * @brief Comprehensive tests for MFCC edge cases and robustness
 */

#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MFCCEdgeCasesTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            engine->destroySession(sessionId);
        }
    }

    std::vector<float> generateDCSignal(size_t numSamples, float dcValue = 0.5f) {
        return std::vector<float>(numSamples, dcValue);
    }

    std::vector<float> generateImpulse(size_t numSamples, size_t impulsePosition = 0) {
        std::vector<float> signal(numSamples, 0.0f);
        if (impulsePosition < numSamples) {
            signal[impulsePosition] = 1.0f;
        }
        return signal;
    }

    std::vector<float> generateSquareWave(size_t numSamples, float frequency, float sampleRate) {
        std::vector<float> signal(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            float phase = 2.0f * M_PI * frequency * i / sampleRate;
            signal[i] = (std::sin(phase) >= 0.0f) ? 0.5f : -0.5f;
        }
        return signal;
    }

    std::vector<float> generateSawtoothWave(size_t numSamples, float frequency, float sampleRate) {
        std::vector<float> signal(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            float phase = frequency * i / sampleRate;
            signal[i] = 0.5f * (phase - std::floor(phase)) - 0.25f;
        }
        return signal;
    }
};

TEST_F(MFCCEdgeCasesTest, PureSilenceProcessing) {
    std::vector<float> silence(8820, 0.0f);  // 0.2 seconds of silence

    auto result = engine->processAudioChunk(sessionId, silence);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());

    // Should extract some features even from silence
    EXPECT_GE(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, DCSignalProcessing) {
    auto dcSignal = generateDCSignal(8820, 0.3f);

    auto result = engine->processAudioChunk(sessionId, dcSignal);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GE(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, ImpulseResponseProcessing) {
    auto impulse = generateImpulse(8820, 1000);  // Impulse at sample 1000

    auto result = engine->processAudioChunk(sessionId, impulse);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GE(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, SquareWaveProcessing) {
    auto squareWave = generateSquareWave(8820, 440.0f, 44100.0f);

    auto result = engine->processAudioChunk(sessionId, squareWave);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, SawtoothWaveProcessing) {
    auto sawtoothWave = generateSawtoothWave(8820, 440.0f, 44100.0f);

    auto result = engine->processAudioChunk(sessionId, sawtoothWave);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, VeryLowFrequencySignal) {
    std::vector<float> lowFreqSignal(44100);  // 1 second at 44.1kHz

    // Generate 1 Hz sine wave
    for (size_t i = 0; i < lowFreqSignal.size(); ++i) {
        lowFreqSignal[i] = 0.5f * std::sin(2.0f * M_PI * 1.0f * i / 44100.0f);
    }

    auto result = engine->processAudioChunk(sessionId, lowFreqSignal);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, VeryHighFrequencySignal) {
    std::vector<float> highFreqSignal(8820);  // 0.2 seconds

    // Generate 20kHz sine wave (near Nyquist frequency)
    for (size_t i = 0; i < highFreqSignal.size(); ++i) {
        highFreqSignal[i] = 0.1f * std::sin(2.0f * M_PI * 20000.0f * i / 44100.0f);
    }

    auto result = engine->processAudioChunk(sessionId, highFreqSignal);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GE(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, ClippedAudioProcessing) {
    std::vector<float> clippedAudio(8820);

    // Generate sine wave and clip it
    for (size_t i = 0; i < clippedAudio.size(); ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        clippedAudio[i] = std::clamp(sample * 2.0f, -1.0f, 1.0f);  // Amplify and clip
    }

    auto result = engine->processAudioChunk(sessionId, clippedAudio);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, MultiToneSignal) {
    std::vector<float> multiTone(8820);

    // Generate multiple sine waves at different frequencies
    const std::vector<float> frequencies = {200.0f, 440.0f, 880.0f, 1760.0f};

    for (size_t i = 0; i < multiTone.size(); ++i) {
        multiTone[i] = 0.0f;
        for (float freq : frequencies) {
            multiTone[i] += 0.1f * std::sin(2.0f * M_PI * freq * i / 44100.0f);
        }
    }

    auto result = engine->processAudioChunk(sessionId, multiTone);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, ChirpSignal) {
    std::vector<float> chirp(8820);

    // Generate a chirp (frequency sweep) from 100Hz to 4000Hz
    float startFreq = 100.0f;
    float endFreq = 4000.0f;
    float duration = static_cast<float>(chirp.size()) / 44100.0f;

    for (size_t i = 0; i < chirp.size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        float freq = startFreq + (endFreq - startFreq) * t / duration;
        chirp[i] = 0.3f * std::sin(2.0f * M_PI * freq * t);
    }

    auto result = engine->processAudioChunk(sessionId, chirp);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, WhiteNoiseProcessing) {
    std::vector<float> whiteNoise(8820);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.2f, 0.2f);

    for (size_t i = 0; i < whiteNoise.size(); ++i) {
        whiteNoise[i] = dis(gen);
    }

    auto result = engine->processAudioChunk(sessionId, whiteNoise);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, PinkNoiseProcessing) {
    std::vector<float> pinkNoise(8820);

    // Simple approximation of pink noise using multiple sine waves
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> phaseDis(0.0f, 2.0f * M_PI);

    for (size_t i = 0; i < pinkNoise.size(); ++i) {
        pinkNoise[i] = 0.0f;
        for (int octave = 1; octave <= 10; ++octave) {
            float freq = 55.0f * std::pow(2.0f, octave);  // A1, A2, A3, etc.
            float amplitude = 1.0f / std::sqrt(freq);     // 1/f falloff
            float phase = phaseDis(gen);
            pinkNoise[i] += amplitude * 0.05f * std::sin(2.0f * M_PI * freq * i / 44100.0f + phase);
        }
    }

    auto result = engine->processAudioChunk(sessionId, pinkNoise);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, VeryShortAudioBuffer) {
    std::vector<float> shortAudio(64);  // Very short buffer

    // Fill with sine wave
    for (size_t i = 0; i < shortAudio.size(); ++i) {
        shortAudio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    auto result = engine->processAudioChunk(sessionId, shortAudio);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount.isOk());
    // May or may not extract features from very short buffer
    EXPECT_GE(*featureCount, 0);
}

TEST_F(MFCCEdgeCasesTest, PowerOfTwoBufferSizes) {
    std::vector<size_t> bufferSizes = {128, 256, 512, 1024, 2048, 4096};

    for (size_t bufferSize : bufferSizes) {
        engine->resetSession(sessionId);

        std::vector<float> audio(bufferSize);
        for (size_t i = 0; i < audio.size(); ++i) {
            audio[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }

        auto result = engine->processAudioChunk(sessionId, audio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK)
            << "Failed for buffer size: " << bufferSize;

        auto featureCount = engine->getFeatureCount(sessionId);
        EXPECT_TRUE(featureCount.isOk())
            << "Failed to get feature count for buffer size: " << bufferSize;
        EXPECT_GE(*featureCount, 0);
    }
}

TEST_F(MFCCEdgeCasesTest, NonPowerOfTwoBufferSizes) {
    std::vector<size_t> bufferSizes = {100, 333, 777, 1000, 1500, 3333};

    for (size_t bufferSize : bufferSizes) {
        engine->resetSession(sessionId);

        std::vector<float> audio(bufferSize);
        for (size_t i = 0; i < audio.size(); ++i) {
            audio[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }

        auto result = engine->processAudioChunk(sessionId, audio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK)
            << "Failed for buffer size: " << bufferSize;

        auto featureCount = engine->getFeatureCount(sessionId);
        EXPECT_TRUE(featureCount.isOk())
            << "Failed to get feature count for buffer size: " << bufferSize;
        EXPECT_GE(*featureCount, 0);
    }
}

TEST_F(MFCCEdgeCasesTest, ConsistencyAcrossMultipleRuns) {
    std::vector<float> testAudio(4410);  // 0.1 seconds

    // Generate deterministic audio
    for (size_t i = 0; i < testAudio.size(); ++i) {
        testAudio[i] = 0.4f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    std::vector<int> featureCounts;

    // Process the same audio multiple times
    for (int run = 0; run < 5; ++run) {
        engine->resetSession(sessionId);

        auto result = engine->processAudioChunk(sessionId, testAudio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);

        auto featureCount = engine->getFeatureCount(sessionId);
        EXPECT_TRUE(featureCount.isOk());
        featureCounts.push_back(*featureCount);
    }

    // All feature counts should be identical
    for (size_t i = 1; i < featureCounts.size(); ++i) {
        EXPECT_EQ(featureCounts[0], featureCounts[i])
            << "MFCC feature extraction should be deterministic";
    }
}

TEST_F(MFCCEdgeCasesTest, TemporalConsistency) {
    // Test that processing audio in different chunk sizes gives similar results
    std::vector<float> longAudio(17640);  // 0.4 seconds

    // Generate test audio
    for (size_t i = 0; i < longAudio.size(); ++i) {
        longAudio[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }

    // Process as single chunk
    engine->resetSession(sessionId);
    auto result1 = engine->processAudioChunk(sessionId, longAudio);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    auto featureCount1 = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount1.isOk());

    // Process as multiple smaller chunks
    engine->resetSession(sessionId);
    const size_t chunkSize = 4410;  // 0.1 second chunks
    for (size_t i = 0; i < longAudio.size(); i += chunkSize) {
        size_t currentChunkSize = std::min(chunkSize, longAudio.size() - i);
        std::vector<float> chunk(longAudio.begin() + i, longAudio.begin() + i + currentChunkSize);

        auto chunkResult = engine->processAudioChunk(sessionId, chunk);
        EXPECT_EQ(chunkResult, UnifiedAudioEngine::Status::OK);
    }

    auto featureCount2 = engine->getFeatureCount(sessionId);
    EXPECT_TRUE(featureCount2.isOk());

    // Feature counts should be similar (within some tolerance)
    EXPECT_NEAR(*featureCount1, *featureCount2, 3)
        << "Feature counts should be similar regardless of chunking";
}
