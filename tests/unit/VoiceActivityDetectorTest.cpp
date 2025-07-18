#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;

// Use a test fixture for better structure and to avoid code duplication.
class VoiceActivityDetectorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // This configuration is used by default for all tests in this fixture.
        config_.energy_threshold = 0.01f;
        config_.window_duration = std::chrono::milliseconds(20);
        config_.sample_rate = 16000;
        config_.pre_buffer = std::chrono::milliseconds(40);
        config_.post_buffer = std::chrono::milliseconds(40); // Hangover for 2 frames
        config_.min_sound_duration = std::chrono::milliseconds(40); // Requires 2 frames to activate
        vad_ = std::make_unique<VoiceActivityDetector>(config_);

    }

    std::vector<float> MakeAudio(size_t n, float value) { return std::vector<float>(n, value); }

    VoiceActivityDetector::Config config_;
    std::unique_ptr<VoiceActivityDetector> vad_;
};

TEST_F(VoiceActivityDetectorTest, SilenceIsNotActive) {
    auto silence = MakeAudio(320, 0.0f);  // 20ms at 16kHz
    auto result = vad_->processWindow(silence);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->is_active);
    EXPECT_LT(result->energy_level, 0.01f);
}

TEST_F(VoiceActivityDetectorTest, VoiceIsDetectedAfterMinDuration) {
    auto voice = MakeAudio(320, 0.2f);  // 20ms at 16kHz, above threshold

    // Frame 1: The VAD detects energy and transitions from SILENCE to VOICE_CANDIDATE.
    // The result's is_active flag will be false because the state is not yet VOICE_ACTIVE.
    auto result1 = vad_->processWindow(voice);
    ASSERT_TRUE(result1.has_value());
    EXPECT_FALSE(result1->is_active) << "VAD should be in CANDIDATE state, not ACTIVE, after one frame.";
    EXPECT_FALSE(vad_->isVoiceActive()) << "isVoiceActive() should be false in CANDIDATE state.";

    // Frame 2: The VAD sees another voice frame, meets min_sound_duration (40ms),
    // and transitions from VOICE_CANDIDATE to VOICE_ACTIVE.
    auto result2 = vad_->processWindow(voice);
    ASSERT_TRUE(result2.has_value());
    EXPECT_TRUE(result2->is_active) << "VAD should transition to ACTIVE on the second frame.";
    EXPECT_TRUE(vad_->isVoiceActive()) << "isVoiceActive() should be true after meeting duration.";
}

TEST_F(VoiceActivityDetectorTest, PreAndPostBuffering) {
    auto silence = MakeAudio(320, 0.0f);
    auto voice = MakeAudio(320, 0.2f);

    // Silence first
    ASSERT_TRUE(vad_->processWindow(silence).has_value())
        << "Initial silence processing failed";

    // Voice onset - requires 2 frames (40ms) to become active
    ASSERT_TRUE(vad_->processWindow(voice).has_value())
        << "First voice frame processing failed";
    EXPECT_FALSE(vad_->isVoiceActive()) << "Should not be active after one voice frame.";
    ASSERT_TRUE(vad_->processWindow(voice).has_value())
        << "Second voice frame processing failed";
    EXPECT_TRUE(vad_->isVoiceActive()) << "Should be active after two voice frames.";

    // Voice offset (back to silence)
    ASSERT_TRUE(vad_->processWindow(silence).has_value())
        << "Post-buffer window 1 processing failed";
    EXPECT_TRUE(vad_->isVoiceActive()) << "Should be in HANGOVER state after first silent frame.";
    ASSERT_TRUE(vad_->processWindow(silence).has_value())
        << "Post-buffer window 2 processing failed";
    EXPECT_TRUE(vad_->isVoiceActive()) << "Should still be in HANGOVER state after second silent frame (post_buffer is 40ms).";

    // Post-buffer period (40ms) has now elapsed. The next silent window should transition to inactive.
    ASSERT_TRUE(vad_->processWindow(silence).has_value()) << "Final silence processing failed";
    EXPECT_FALSE(vad_->isVoiceActive());
}

TEST_F(VoiceActivityDetectorTest, ResetRestoresInitialState) {
    auto voice = MakeAudio(320, 0.2f);
    // Process a window to change the state. We assert the call succeeds
    // to be explicit and to address the [[nodiscard]] warning.
    ASSERT_TRUE(vad_->processWindow(voice).has_value());
    vad_->reset();
    EXPECT_FALSE(vad_->isVoiceActive());
    EXPECT_EQ(vad_->getActiveDuration().count(), 0);
}

// Empty audio should return an error
TEST_F(VoiceActivityDetectorTest, InvalidInputReturnsError) {
    std::vector<float> empty;
    auto result = vad_->processWindow(empty);
    EXPECT_FALSE(result.has_value());
}