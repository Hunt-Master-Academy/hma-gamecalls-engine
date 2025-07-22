#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class VADActiveTransitionTest : public ::testing::Test {
  protected:
    std::vector<float> generateSignalChunk(size_t samples, float level) {
        return std::vector<float>(samples, level);
    }
};

TEST_F(VADActiveTransitionTest, CandidateToActive) {
    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;  // Requires 3 consecutive frames
    config.post_buffer = 50ms;

    VoiceActivityDetector vad(config);

    auto signal_chunk = generateSignalChunk(160, 0.2f);  // 10ms at 16kHz

    // Process two frames. State should still be CANDIDATE.
    auto result1 = vad.processWindow(signal_chunk);
    ASSERT_TRUE(result1.has_value());
    EXPECT_FALSE(result1.value().is_active) << "Should not be active after 1 frame";

    auto result2 = vad.processWindow(signal_chunk);
    ASSERT_TRUE(result2.has_value());
    EXPECT_FALSE(result2.value().is_active) << "Should not be active after 2 frames";

    // Process a third frame. This meets the 30ms min_sound_duration.
    auto result3 = vad.processWindow(signal_chunk);
    ASSERT_TRUE(result3.has_value());
    EXPECT_TRUE(result3.value().is_active) << "Should be active after 3 frames";
    EXPECT_TRUE(vad.isVoiceActive());
}