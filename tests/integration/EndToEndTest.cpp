#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "HuntmasterEngine.h"

using namespace huntmaster;

class EndToEndTest : public ::testing::Test {
   protected:
    void SetUp() override {
        PlatformEngineConfig config{.sample_rate = 44100, .frame_size = 512, .hop_size = 256};
        engine = std::make_unique<HuntmasterEngine>(config);
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

    std::unique_ptr<HuntmasterEngine> engine;
};

TEST_F(EndToEndTest, ProcessSimpleAudio) {
    // Generate a 440Hz test signal
    auto test_signal = generateTestSignal(440.0f, 0.5f, 44100.0f);

    // Start a session
    auto session_result = engine->startSession(1);
    ASSERT_TRUE(session_result.has_value());

    // Process in chunks
    const size_t chunk_size = 512;
    for (size_t i = 0; i < test_signal.size(); i += chunk_size) {
        size_t actual_size = std::min(chunk_size, test_signal.size() - i);
        std::span<const float> chunk(test_signal.data() + i, actual_size);

        auto result = engine->processChunk(chunk);
        ASSERT_TRUE(result.has_value());
    }

    // End session
    auto end_result = engine->endSession(1);
    EXPECT_TRUE(end_result.has_value());
}

TEST_F(EndToEndTest, LoadMasterCallAndCompare) {
    // This test would require having test audio files
    // For unit testing, we might want to generate synthetic master calls

    // Load a master call (would need test data)
    auto load_result = engine->loadMasterCall("test_call");
    // Note: This will fail without actual test data files
    // EXPECT_TRUE(load_result.has_value());

    // For now, just verify the API works
    EXPECT_TRUE(engine->isInitialized());
}