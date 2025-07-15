#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

TEST(EnergyComparisonTest, DebugEnergyComparison) {
    std::cout << "=== Starting EnergyComparisonTest ===" << std::endl;

    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    std::cout << "Config: energy_threshold=" << config.energy_threshold << std::endl;

    VoiceActivityDetector vad(config);

    std::vector<float> signal_chunk(160, 0.2f);  // Should give energy = 0.2*0.2 = 0.04

    std::cout << "Expected energy: " << (0.2f * 0.2f) << std::endl;
    std::cout << "Configured threshold: " << config.energy_threshold << std::endl;
    std::cout << "Should be active: " << (0.04f > 0.01f) << std::endl;

    std::cout << "--- Calling processWindow ---" << std::endl;
    std::cout << "VoiceActivityDetector address: " << &vad << std::endl;
    std::cout << "About to call processWindow with " << signal_chunk.size() << " samples"
              << std::endl;
    std::cout.flush();

    auto result = vad.processWindow(signal_chunk);

    std::cout << "--- processWindow finished ---" << std::endl;
    std::cout << "Result has_value: " << result.has_value() << std::endl;
    std::cout.flush();

    ASSERT_TRUE(result.has_value());

    float actual_energy = result.value().energy_level;
    bool is_active = result.value().is_active;

    std::cout << "Actual energy: " << actual_energy << std::endl;
    std::cout << "Is active: " << is_active << std::endl;
    std::cout << "Manual comparison: " << actual_energy << " > " << config.energy_threshold << " = "
              << (actual_energy > config.energy_threshold) << std::endl;

    // The energy should be 0.04 and should be greater than 0.01
    EXPECT_FLOAT_EQ(actual_energy, 0.04f);
    EXPECT_GT(actual_energy, config.energy_threshold);

    // This should pass but it doesn't - this is the bug!
    EXPECT_TRUE(is_active) << "Energy " << actual_energy << " should be greater than threshold "
                           << config.energy_threshold;
}
