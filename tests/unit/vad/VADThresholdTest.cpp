#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace huntmaster::test;
using namespace std::chrono_literals;

class VADThresholdTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();
    }

    void TearDown() override {
        TestFixtureBase::TearDown();
    }

    std::vector<float> generateSignalChunk(size_t samples, float level) {
        return std::vector<float>(samples, level);
    }
};

TEST_F(VADThresholdTest, CheckThresholdValues) {
    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    VoiceActivityDetector vad(config);

    auto signal_chunk = generateSignalChunk(160, 0.2f);  // 10ms at 16kHz

    // Process one frame and check the threshold behavior
    auto result = vad.processWindow(signal_chunk);
    ASSERT_TRUE(result.has_value());

    // The energy should be 0.04 (0.2 * 0.2 = 0.04)
    EXPECT_NEAR(result.value().energy_level, 0.04f, 1e-6f);

    // Print the actual values for debugging
    std::cout << "Energy: " << result.value().energy_level << std::endl;
    std::cout << "Configured threshold: " << config.energy_threshold << std::endl;

    // The energy should be greater than the threshold
    EXPECT_GT(result.value().energy_level, config.energy_threshold);
}

// TODO: Expand VAD threshold testing with additional test cases:
// [ ] Test multiple threshold values (low, medium, high sensitivity)
// [ ] Test boundary conditions (exactly at threshold, just above/below)
// [ ] Test varying signal levels over time
// [ ] Test different window sizes and their effect on threshold detection
// [ ] Test threshold adaptation for different noise floors
// [ ] Test silence detection (signals below threshold)
// [ ] Test transition behavior when crossing threshold
// [ ] Test with real-world audio patterns (speech, music, ambient noise)
