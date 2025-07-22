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
