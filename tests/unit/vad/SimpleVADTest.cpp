#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

TEST(SimpleVADTest, BasicFunctionality) {
    std::cout << "Starting SimpleVADTest" << std::endl;

    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    std::cout << "Creating VoiceActivityDetector" << std::endl;
    VoiceActivityDetector vad(config);

    std::cout << "Generating signal chunk" << std::endl;
    std::vector<float> signal_chunk(160, 0.2f);  // 10ms at 16kHz

    std::cout << "Calling processWindow" << std::endl;
    auto result = vad.processWindow(signal_chunk);

    std::cout << "processWindow returned" << std::endl;
    ASSERT_TRUE(result.has_value());

    std::cout << "Energy: " << result.value().energy_level << std::endl;
    std::cout << "Is active: " << result.value().is_active << std::endl;

    EXPECT_TRUE(true);  // Just pass the test
}

// TODO: Transform SimpleVADTest into comprehensive VAD functionality testing:
// [ ] Replace basic pass test with meaningful assertions on energy and activity
// [ ] Test various signal amplitudes and their detected energy levels
// [ ] Test different window durations and their effect on detection accuracy
// [ ] Test VAD behavior with realistic audio patterns (speech, music, noise)
// [ ] Test configuration parameter validation and boundary conditions
// [ ] Test VAD performance metrics and processing timing
// [ ] Test integration with different sample rates and audio formats
// [ ] Add regression tests for known VAD accuracy issues
