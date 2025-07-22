#include <chrono>
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/VoiceActivityDetector.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;
using namespace std::chrono_literals;

TEST(EnergyComparisonTest, ComprehensiveEnergyAndStateValidation) {
    std::cout << "=== Starting EnergyComparisonTest ===" << std::endl;

    // Create VAD with specific configuration
    const float energy_threshold = 0.01f;
    const auto window_duration = 20ms;     // 20ms
    const auto min_sound_duration = 40ms;  // 40ms (requires 2 frames)
    const auto post_buffer = 50ms;         // 50ms

    std::cout << "Config details:" << std::endl;
    std::cout << "  energy_threshold=" << energy_threshold << std::endl;
    std::cout << "  window_duration=" << window_duration.count() << "ms" << std::endl;
    std::cout << "  min_sound_duration=" << min_sound_duration.count() << "ms" << std::endl;
    std::cout << "  post_buffer=" << post_buffer.count() << "ms" << std::endl;
    std::cout << "  Expected frames to activate: "
              << static_cast<int>(min_sound_duration.count() / window_duration.count())
              << std::endl;

    // Create VAD with config
    VoiceActivityDetector::Config config;
    config.energy_threshold = energy_threshold;
    config.window_duration = window_duration;
    config.min_sound_duration = min_sound_duration;
    config.post_buffer = post_buffer;

    VoiceActivityDetector vad(config);

    // Create test signals
    const int window_size =
        static_cast<int>(8000.0f * window_duration.count() / 1000.0f);  // Convert ms to samples
    std::vector<float> voice_signal(window_size);
    std::vector<float> silence_signal(window_size, 0.0f);

    // Generate 1kHz sine wave with amplitude 0.2
    const float amplitude = 0.2f;
    const float frequency = 1000.0f;
    for (int i = 0; i < window_size; ++i) {
        voice_signal[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / 8000.0f);
    }

    // Expected energy calculation
    // For sine wave: RMS = amplitude/√2, Energy = RMS²
    float rms_amplitude = amplitude / std::sqrt(2.0f);            // 0.2/√2 ≈ 0.141
    float expected_voice_energy = rms_amplitude * rms_amplitude;  // ≈ 0.02
    float expected_silence_energy = 0.0f;

    std::cout << "Expected energy for voice: " << expected_voice_energy << std::endl;
    std::cout << "Expected energy for silence: " << expected_silence_energy << std::endl;
    std::cout << "Configured threshold: " << energy_threshold << std::endl;
    std::cout << "Voice should be above threshold: " << (expected_voice_energy > energy_threshold)
              << std::endl;

    std::cout << "=== Processing Multiple Frames ===" << std::endl;

    // Frame 1: Voice signal - should detect voice but NOT be active yet
    std::cout << "--- Frame 1: Voice Signal ---" << std::endl;
    std::cout << "Processing " << voice_signal.size() << " samples with amplitude " << amplitude
              << std::endl;
    auto result1 = vad.processWindow(voice_signal);
    ASSERT_TRUE(result1.has_value());
    std::cout << "Frame 1 result:" << std::endl;
    std::cout << "  energy: " << result1.value().energy_level << std::endl;
    std::cout << "  is_active: " << result1.value().is_active << std::endl;
    std::cout << "  energy > threshold: " << (result1.value().energy_level > energy_threshold)
              << std::endl;

    // Frame 1 assertions
    EXPECT_FLOAT_EQ(result1.value().energy_level, expected_voice_energy)
        << "Frame 1 energy calculation";
    EXPECT_FALSE(result1.value().is_active)
        << "Frame 1 should NOT be active (still in VOICE_CANDIDATE state)";

    // Frame 2: Voice signal - should now be active (40ms reached)
    std::cout << "\n--- Frame 2: Voice Signal ---" << std::endl;
    auto result2 = vad.processWindow(voice_signal);
    ASSERT_TRUE(result2.has_value());
    std::cout << "Frame 2 result:" << std::endl;
    std::cout << "  energy: " << result2.value().energy_level << std::endl;
    std::cout << "  is_active: " << result2.value().is_active << std::endl;

    // Frame 2 assertions - Should be active now (40ms reached)
    EXPECT_FLOAT_EQ(result2.value().energy_level, expected_voice_energy)
        << "Frame 2 energy calculation";
    EXPECT_TRUE(result2.value().is_active)
        << "Frame 2 SHOULD be active (40ms reached with 2 frames of 20ms each)";

    // Frame 3: Voice signal - should remain active
    std::cout << "\n--- Frame 3: Voice Signal ---" << std::endl;
    auto result3 = vad.processWindow(voice_signal);
    ASSERT_TRUE(result3.has_value());
    std::cout << "Frame 3 result:" << std::endl;
    std::cout << "  energy: " << result3.value().energy_level << std::endl;
    std::cout << "  is_active: " << result3.value().is_active << std::endl;

    // Frame 3 assertions - Should remain active
    EXPECT_FLOAT_EQ(result3.value().energy_level, expected_voice_energy)
        << "Frame 3 energy calculation";
    EXPECT_TRUE(result3.value().is_active)
        << "Frame 3 should remain active (in VOICE_ACTIVE state)";

    // Frame 4: Silence - should remain active due to hangover
    std::cout << "\n--- Frame 4: Silence Signal ---" << std::endl;
    auto result4 = vad.processWindow(silence_signal);
    ASSERT_TRUE(result4.has_value());
    std::cout << "Frame 4 result:" << std::endl;
    std::cout << "  energy: " << result4.value().energy_level << std::endl;
    std::cout << "  is_active: " << result4.value().is_active << std::endl;

    // Frame 4 assertions
    EXPECT_FLOAT_EQ(result4.value().energy_level, expected_silence_energy)
        << "Frame 4 energy calculation";
    EXPECT_TRUE(result4.value().is_active) << "Frame 4 should remain active (in HANGOVER state)";

    std::cout << "=== Test Complete ===" << std::endl;
}