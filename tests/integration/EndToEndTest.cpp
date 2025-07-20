#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using SessionId = uint32_t;

class EndToEndTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine_ = std::move(engineResult.value);
    }

    // Generate a simple test signal
    std::vector<float> generateTestSignal(float frequency, float duration, float sample_rate) {
        size_t num_samples = static_cast<size_t>(duration * sample_rate);
        std::vector<float> signal(num_samples);

        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;
            signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
        }

        return signal;
    }

    std::unique_ptr<UnifiedAudioEngine> engine_;
};

TEST_F(EndToEndTest, ProcessSimpleAudio) {
    // Generate a 440Hz test signal
    auto test_signal = generateTestSignal(440.0f, 0.5f, 44100.0f);

    // Start a realtime session
    auto sessionResult = engine_->startRealtimeSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to start realtime session";
    uint32_t sessionId = sessionResult.value;

    // Process in chunks
    const size_t chunk_size = 512;
    for (size_t i = 0; i < test_signal.size(); i += chunk_size) {
        size_t actual_size = std::min(chunk_size, test_signal.size() - i);

        std::span<const float> chunk(test_signal.data() + i, actual_size);
        auto result = engine_->processAudioChunk(sessionId, chunk);
        ASSERT_EQ(result, UnifiedAudioEngine::Status::OK) << "Failed to process audio chunk";
    }

    // End session
    auto endResult = engine_->endRealtimeSession(sessionId);
    EXPECT_EQ(endResult, UnifiedAudioEngine::Status::OK) << "Failed to end session";
}

TEST_F(EndToEndTest, LoadMasterCallAndCompare) {
    // Start a session first
    auto sessionResult = engine_->startRealtimeSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to start realtime session";
    uint32_t sessionId = sessionResult.value;

    // Try to load a master call that doesn't exist (this should fail gracefully)
    auto loadResult = engine_->loadMasterCall(sessionId, "nonexistent_call");
    // We expect this to fail since this file definitely doesn't exist
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::FILE_NOT_FOUND)
        << "Expected file not found error for missing test data";

    // Clean up session
    auto endResult = engine_->endRealtimeSession(sessionId);
    EXPECT_EQ(endResult, UnifiedAudioEngine::Status::OK) << "Failed to end session";
}

TEST_F(EndToEndTest, EngineInitializesSuccessfully) {
    // For UnifiedAudioEngine, we verify successful creation in SetUp()
    // If we got here, the engine was created successfully
    ASSERT_TRUE(engine_ != nullptr) << "Engine should be initialized";

    // Test that we can create a session as a basic functionality check
    auto sessionResult = engine_->startRealtimeSession(44100.0f);
    EXPECT_TRUE(sessionResult.isOk()) << "Should be able to create a realtime session";

    if (sessionResult.isOk()) {
        auto endResult = engine_->endRealtimeSession(sessionResult.value);
        EXPECT_EQ(endResult, UnifiedAudioEngine::Status::OK) << "Should be able to end session";
    }
}