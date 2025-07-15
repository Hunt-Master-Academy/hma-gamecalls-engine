#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

// Test fixture for VAD tests
class VADStateTransitionTest : public ::testing::Test {
   protected:
    // Generates a silent audio chunk
    std::vector<float> generateSilentChunk(size_t samples) {
        return std::vector<float>(samples, 0.0f);
    }

    // Generates an audio chunk with a specific signal level
    std::vector<float> generateSignalChunk(size_t samples, float level) {
        return std::vector<float>(samples, level);
    }
};

TEST_F(VADStateTransitionTest, InitialStateIsSilence) {
    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    VoiceActivityDetector vad(config);

    // Initially, the VAD should not be active
    ASSERT_FALSE(vad.isVoiceActive());

    auto silent_chunk = generateSilentChunk(160);  // 10ms at 16kHz
    auto result = vad.processWindow(silent_chunk);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().is_active);
    EXPECT_FALSE(vad.isVoiceActive());
}
