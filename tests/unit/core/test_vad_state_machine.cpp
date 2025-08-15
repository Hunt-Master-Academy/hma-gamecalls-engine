#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;

namespace {
std::vector<float> makeWindow(size_t samples, float amplitude) {
    std::vector<float> w(samples, 0.0f);
    if (amplitude != 0.0f) {
        for (size_t i = 0; i < samples; ++i) {
            // Simple tone-like signal within [-1,1]
            w[i] = amplitude * ((i % 2) ? 1.0f : -1.0f);
        }
    }
    return w;
}
}  // namespace

TEST(VoiceActivityDetectorTest, Transitions_Silence_Candidate_Active_Hangover_Silence) {
    VoiceActivityDetector::Config cfg;
    cfg.sample_rate = 44100;
    cfg.window_duration = std::chrono::milliseconds(20);     // 20ms windows
    cfg.min_sound_duration = std::chrono::milliseconds(60);  // need ~3 windows of activity
    cfg.pre_buffer = std::chrono::milliseconds(10);
    cfg.post_buffer = std::chrono::milliseconds(20);  // short hangover for quick transition
    cfg.energy_threshold = 0.02f;

    VoiceActivityDetector vad(cfg);
    const size_t samplesPerWin =
        static_cast<size_t>(cfg.sample_rate * cfg.window_duration.count() / 1000);

    // Start in silence
    EXPECT_FALSE(vad.isVoiceActive());

    // Feed one silence window (should remain not active)
    auto silence = makeWindow(samplesPerWin, 0.0f);
    auto r0 = vad.processWindow(silence);
    ASSERT_TRUE(r0.has_value());
    EXPECT_FALSE(r0->is_active);
    EXPECT_FALSE(vad.isVoiceActive());

    // Feed candidate activity windows (3 windows above threshold)
    auto voice = makeWindow(samplesPerWin, 0.2f);
    (void)vad.processWindow(voice);
    EXPECT_FALSE(vad.isVoiceActive());  // still in candidate region
    (void)vad.processWindow(voice);
    EXPECT_FALSE(vad.isVoiceActive());  // still candidate
    auto r3 = vad.processWindow(voice);
    ASSERT_TRUE(r3.has_value());
    EXPECT_TRUE(vad.isVoiceActive());  // should enter ACTIVE after min_sound_duration

    // Now feed silence; should remain active briefly due to hangover/post_buffer
    auto r4 = vad.processWindow(silence);
    ASSERT_TRUE(r4.has_value());
    EXPECT_TRUE(vad.isVoiceActive());  // in HANGOVER, still reports active

    // Another silence window should allow hangover to elapse and return to SILENCE
    auto r5 = vad.processWindow(silence);
    ASSERT_TRUE(r5.has_value());
    EXPECT_FALSE(vad.isVoiceActive());

    // Reset returns to initial state
    vad.reset();
    EXPECT_FALSE(vad.isVoiceActive());
}

// TODO: Expand VAD state machine testing with comprehensive coverage:
// [ ] Test each state transition with precise timing verification
// [ ] Test edge cases for timing boundaries (exactly at duration thresholds)
// [ ] Test different sample rates and their effect on state timing
// [ ] Test configuration parameter validation and edge values
// [ ] Test concurrent access patterns (if thread-safety is required)
// [ ] Test memory usage and performance under sustained operation
// [ ] Test integration with different audio input patterns
// [ ] Test error conditions and recovery scenarios
// [ ] Test state machine behavior with malformed input data
// [ ] Test configuration changes during operation (if supported)
