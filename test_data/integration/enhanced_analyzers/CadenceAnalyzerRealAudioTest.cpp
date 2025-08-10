// Real audio test for CadenceAnalyzer using a turkey 'Cluck_and_Purr' normalized WAV sample

#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/CadenceAnalyzer.h"

// miniaudio single-header decoder (MP3/OGG/WAV/FLAC)
#include "miniaudio.h"

using namespace huntmaster;

namespace {

struct DecodedAudio {
    std::vector<float> samples;
    float sampleRate = 0.0f;
};

DecodedAudio loadAudioMonoF32(const std::string& path, float desiredSampleRate = 44100.0f) {
    DecodedAudio out;

    ma_decoder_config config =
        ma_decoder_config_init(ma_format_f32, 1, (ma_uint32)desiredSampleRate);
    ma_decoder decoder;
    if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS) {
        return out;  // empty
    }

    ma_uint64 frameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS
        || frameCount == 0) {
        ma_decoder_uninit(&decoder);
        return out;
    }

    out.sampleRate = (float)decoder.outputSampleRate;
    const ma_uint32 channels = decoder.outputChannels;  // should be 1 per config

    std::vector<float> buffer;
    buffer.resize((size_t)frameCount * channels);

    ma_uint64 framesRead = 0;
    if (ma_decoder_read_pcm_frames(&decoder, buffer.data(), frameCount, &framesRead)
        != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return out;
    }
    ma_decoder_uninit(&decoder);

    buffer.resize((size_t)framesRead * channels);
    out.samples = std::move(buffer);
    return out;
}

// Trim to a centered slice to make onset detection more robust and execution fast
std::vector<float> centerSlice(const std::vector<float>& in, size_t sliceSamples) {
    if (in.size() <= sliceSamples)
        return in;
    size_t start = (in.size() - sliceSamples) / 2;
    return std::vector<float>(in.begin() + start, in.begin() + start + sliceSamples);
}

}  // namespace

class CadenceAnalyzerRealAudioTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Use normalized derivative (44.1kHz mono float WAV) from processed_calls
        audioPath_ = "data/processed_calls/normalized/turkey/Cluck_and_Purr.wav";
        decoded_ = loadAudioMonoF32(audioPath_, 44100.0f);

        // Keep analysis window small and stable (e.g., ~0.75s)
        if (!decoded_.samples.empty()) {
            size_t targetSamples = (size_t)(0.75f * decoded_.sampleRate);
            clip_ = centerSlice(decoded_.samples, targetSamples);
        }
    }

    std::string audioPath_;
    DecodedAudio decoded_;
    std::vector<float> clip_;
};

TEST_F(CadenceAnalyzerRealAudioTest, DetectsOnsetsAndTempoOnRealAudio) {
    if (decoded_.samples.empty() || clip_.empty()) {
        GTEST_SKIP() << "Real audio file not available or failed to decode: " << audioPath_;
    }

    CadenceAnalyzer::Config cfg;
    cfg.sampleRate = decoded_.sampleRate;  // Use decoded rate (expected 44100)
    cfg.frameSize = 0.025f;                // 25ms
    cfg.hopSize = 0.010f;                  // 10ms
    cfg.minTempo = 40.0f;                  // Plausible range
    cfg.maxTempo = 300.0f;
    cfg.onsetThreshold = 0.08f;  // Slightly lower threshold for real audio
    cfg.autocorrelationLags = 1024;
    cfg.enableBeatTracking = true;
    cfg.enableOnsetDetection = true;
    cfg.enableSyllableAnalysis = false;  // Keep fast
    cfg.adaptiveThreshold = 0.1f;

    auto aResult = CadenceAnalyzer::create(cfg);
    ASSERT_TRUE(aResult.has_value()) << "Failed to create CadenceAnalyzer for real audio";
    auto analyzer = std::move(aResult.value());

    auto result = analyzer->analyzeCadence(std::span<const float>(clip_));
    ASSERT_TRUE(result.has_value()) << "Cadence analysis failed on real audio";
    auto profile = result.value();

    // Functional expectations (lenient): should detect some rhythmic structure
    EXPECT_GT(profile.sequence.numCalls, 0u) << "Should detect at least one onset";
    EXPECT_GE(profile.estimatedTempo, 0.0f);
    EXPECT_LE(profile.estimatedTempo, 400.0f);

    // Log stats for visibility in CI
    std::cout << analyzer->getProcessingStats() << std::endl;
}
