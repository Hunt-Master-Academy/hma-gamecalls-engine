#include <gtest/gtest.h>

#include <iostream>
#include <vector>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;

class StateMachineTest : public ::testing::Test {
   protected:
    void SetUp() override {
        VoiceActivityDetector::Config config;
        config.energy_threshold = 0.01f;
        config.min_sound_duration =
            std::chrono::milliseconds(40);  // 40ms = 2 frames at 160 samples/20ms
        config.window_duration = std::chrono::milliseconds(20);

        vad_ = std::make_unique<VoiceActivityDetector>(config);

        // Create signal that should trigger voice activity
        signal_chunk_.resize(160);
        for (int i = 0; i < 160; ++i) {
            signal_chunk_[i] = 0.2f;  // Should give energy = 0.04
        }
    }

    std::unique_ptr<VoiceActivityDetector> vad_;
    std::vector<float> signal_chunk_;
};

TEST_F(StateMachineTest, MultipleFramesToActivate) {
    std::cout << "=== Testing VAD State Machine with Multiple Frames ===" << std::endl;

    // Process first frame - should go to VOICE_CANDIDATE
    std::cout << "Frame 1:" << std::endl;
    auto result1 = vad_->processWindow(signal_chunk_);
    ASSERT_TRUE(result1.has_value());
    std::cout << "  Energy: " << result1.value().energy_level << std::endl;
    std::cout << "  Is active: " << result1.value().is_active << std::endl;

    // Process second frame - should stay in VOICE_CANDIDATE
    std::cout << "Frame 2:" << std::endl;
    auto result2 = vad_->processWindow(signal_chunk_);
    ASSERT_TRUE(result2.has_value());
    std::cout << "  Energy: " << result2.value().energy_level << std::endl;
    std::cout << "  Is active: " << result2.value().is_active << std::endl;

    // Process third frame - should transition to VOICE_ACTIVE
    std::cout << "Frame 3:" << std::endl;
    auto result3 = vad_->processWindow(signal_chunk_);
    ASSERT_TRUE(result3.has_value());
    std::cout << "  Energy: " << result3.value().energy_level << std::endl;
    std::cout << "  Is active: " << result3.value().is_active << std::endl;

    // The third frame should finally show is_active = true
    EXPECT_TRUE(result3.value().is_active) << "After 3 frames (60ms), VAD should be active";
}
