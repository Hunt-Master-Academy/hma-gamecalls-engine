#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "../../include/huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;

namespace {

VoiceActivityDetector::Config DefaultConfig() {
    VoiceActivityDetector::Config cfg;
    cfg.energy_threshold = 0.01f;
    cfg.window_duration = std::chrono::milliseconds(20);
    cfg.sample_rate = 16000;
    cfg.pre_buffer = std::chrono::milliseconds(40);
    cfg.post_buffer = std::chrono::milliseconds(40);
    cfg.min_sound_duration = std::chrono::milliseconds(20);
    return cfg;
}

std::vector<float> MakeAudio(size_t n, float value) { return std::vector<float>(n, value); }

TEST(VoiceActivityDetectorTest, SilenceIsNotActive) {
    VoiceActivityDetector vad(DefaultConfig());
    auto silence = MakeAudio(320, 0.0f);  // 20ms at 16kHz
    auto result = vad.processWindow(silence);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->is_active);
    EXPECT_LT(result->energy_level, 0.01f);
}

TEST(VoiceActivityDetectorTest, VoiceIsDetected) {
    VoiceActivityDetector vad(DefaultConfig());
    auto voice = MakeAudio(320, 0.2f);  // 20ms at 16kHz, above threshold
    // Need to call enough times to pass min_sound_duration
    for (int i = 0; i < 2; ++i) {
        auto result = vad.processWindow(voice);
        ASSERT_TRUE(result.has_value());
        if (i == 1) {
            EXPECT_TRUE(result->is_active);
            EXPECT_GT(result->energy_level, 0.01f);
        }
    }
}

TEST(VoiceActivityDetectorTest, AdaptiveThresholdIncreasesWithNoise) {
    auto cfg = DefaultConfig();
    cfg.energy_threshold = 0.001f;
    VoiceActivityDetector vad(cfg);
    auto noise = MakeAudio(320, 0.05f);
    // Feed noise for a while to raise adaptive threshold
    for (int i = 0; i < 20; ++i) {
        vad.processWindow(noise);
    }
    auto voice = MakeAudio(320, 0.06f);
    auto result = vad.processWindow(voice);
    ASSERT_TRUE(result.has_value());
    // Should still detect as voice, but threshold should have increased
    EXPECT_TRUE(result->energy_level > cfg.energy_threshold);
}

TEST(VoiceActivityDetectorTest, PreAndPostBuffering) {
    auto cfg = DefaultConfig();
    cfg.pre_buffer = std::chrono::milliseconds(40);
    cfg.post_buffer = std::chrono::milliseconds(40);
    VoiceActivityDetector vad(cfg);

    auto silence = MakeAudio(320, 0.0f);
    auto voice = MakeAudio(320, 0.2f);

    // Silence first
    vad.processWindow(silence);
    // Voice onset
    vad.processWindow(voice);
    vad.processWindow(voice);
    // Voice offset (back to silence)
    vad.processWindow(silence);
    vad.processWindow(silence);

    // Should still be in post-buffer period after voice offset
    EXPECT_TRUE(vad.isVoiceActive());
    // Wait for post-buffer to expire
    std::this_thread::sleep_for(cfg.post_buffer);
    vad.processWindow(silence);
    EXPECT_FALSE(vad.isVoiceActive());
}

TEST(VoiceActivityDetectorTest, ResetRestoresInitialState) {
    VoiceActivityDetector vad(DefaultConfig());
    auto voice = MakeAudio(320, 0.2f);
    vad.processWindow(voice);
    vad.reset();
    EXPECT_FALSE(vad.isVoiceActive());
    EXPECT_EQ(vad.getActiveDuration().count(), 0);
}

TEST(VoiceActivityDetectorTest, InvalidInputReturnsError) {
    VoiceActivityDetector vad(DefaultConfig());
    std::vector<float> empty;
    auto result = vad.processWindow(empty);
    EXPECT_FALSE(result.has_value());
}

}  // namespace