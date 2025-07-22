#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class VADCandidateTransitionTest : public ::testing::Test {
  protected:
    std::vector<float> generateSignalChunk(size_t samples, float level) {
        return std::vector<float>(samples, level);
    }
    std::vector<float> generateSilentChunk(size_t samples) {
        return std::vector<float>(samples, 0.0f);
    }
};

TEST_F(VADCandidateTransitionTest, SilenceToCandidate) {
    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    VoiceActivityDetector vad(config);

    auto signal_chunk = generateSignalChunk(160, 0.2f);

    // A single frame of signal should move state to VOICE_CANDIDATE
    auto result = vad.processWindow(signal_chunk);

    ASSERT_TRUE(result.has_value());
    // Not active yet because min_sound_duration is not met
    EXPECT_FALSE(result.value().is_active);

    // If we now process silence, it should immediately go back to SILENCE state.
    // This implies the previous state was CANDIDATE, not ACTIVE (which would go to HANGOVER).
    auto silent_chunk = generateSilentChunk(160);
    result = vad.processWindow(silent_chunk);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().is_active);
}