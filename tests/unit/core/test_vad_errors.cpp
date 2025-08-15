// VAD error and auxiliary behavior tests
#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;

namespace {
std::vector<float> makeNonEmpty(size_t n, float v = 0.01f) {
    return std::vector<float>(n, v);
}
}  // namespace

TEST(VoiceActivityDetectorErrors, EmptyInputReturnsInvalid) {
    VoiceActivityDetector::Config cfg;
    cfg.sample_rate = 44100;
    cfg.window_duration = std::chrono::milliseconds(20);
    VoiceActivityDetector vad(cfg);

    std::vector<float> empty;
    auto r = vad.processWindow(empty);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), VADError::INVALID_INPUT);
}

TEST(VoiceActivityDetectorErrors, MovedFromDetectorReportsNotInitialized) {
    VoiceActivityDetector::Config cfg;
    cfg.sample_rate = 44100;
    cfg.window_duration = std::chrono::milliseconds(20);
    VoiceActivityDetector vad1(cfg);
    VoiceActivityDetector vad2(std::move(vad1));

    auto buf = makeNonEmpty(
        static_cast<size_t>(cfg.sample_rate * cfg.window_duration.count() / 1000), 0.02f);
    // Using moved-from instance should yield NOT_INITIALIZED
    auto r1 = vad1.processWindow(buf);
    ASSERT_FALSE(r1.has_value());
    EXPECT_EQ(r1.error(), VADError::NOT_INITIALIZED);

    // New owner should process fine (no specific state asserted here)
    auto r2 = vad2.processWindow(buf);
    ASSERT_TRUE(r2.has_value());
}

TEST(VoiceActivityDetectorErrors, InactiveDurationIsZero) {
    VoiceActivityDetector::Config cfg;
    cfg.sample_rate = 44100;
    cfg.window_duration = std::chrono::milliseconds(20);
    VoiceActivityDetector vad(cfg);

    EXPECT_FALSE(vad.isVoiceActive());
    EXPECT_EQ(vad.getActiveDuration().count(), 0);

    // Feed a silence window and verify still inactive with zero duration
    std::vector<float> silence(
        static_cast<size_t>(cfg.sample_rate * cfg.window_duration.count() / 1000), 0.0f);
    auto r = vad.processWindow(silence);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(vad.isVoiceActive());
    EXPECT_EQ(vad.getActiveDuration().count(), 0);
}
