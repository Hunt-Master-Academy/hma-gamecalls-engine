#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <vector>

#include "huntmaster/core/HuntmasterEngine.h"

using namespace huntmaster;

class EndToEndTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // The PlatformEngineConfig struct has been refactored and no longer
        // contains direct members like 'sample_rate'. The engine now uses
        // reasonable internal defaults when given a default-constructed config.
        // This change aligns the test with the new engine architecture.
        huntmaster::PlatformEngineConfig config;
        engine_ = std::make_unique<huntmaster::HuntmasterEngine>(config);
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

    std::unique_ptr<huntmaster::HuntmasterEngine> engine_;
};

TEST_F(EndToEndTest, ProcessSimpleAudio) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(15);
    const auto start_time = std::chrono::steady_clock::now();

    // Generate a 440Hz test signal
    auto test_signal = generateTestSignal(440.0f, 0.5f, 44100.0f);

    // Start a session
    auto session_result = engine_->startSession(1);
    ASSERT_TRUE(session_result.has_value());

    // Check timeout during session start
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during session start";

    // Load a master call before processing audio
    auto load_result = engine_->loadMasterCall("data/master_calls/buck_grunt.wav");
    ASSERT_TRUE(load_result.has_value() ||
                load_result.error().status == EngineStatus::FILE_NOT_FOUND);

    // Process in chunks
    const size_t chunk_size = 512;
    for (size_t i = 0; i < test_signal.size(); i += chunk_size) {
        // Check timeout during processing
        ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
            << "Test timed out during audio processing";

        size_t actual_size = std::min(chunk_size, test_signal.size() - i);
        std::span<const float> chunk(test_signal.data() + i, actual_size);

        auto result = engine_->processChunk(chunk);
        if (!result.has_value()) {
            std::cout << "ProcessChunk failed at chunk " << i / chunk_size
                      << ": status=" << static_cast<int>(result.error().status) << ", message='"
                      << result.error().message << "'\n";
            ADD_FAILURE() << "ProcessChunk failed at chunk " << i / chunk_size
                          << ": status=" << static_cast<int>(result.error().status) << ", message='"
                          << result.error().message << "'";
        }
        ASSERT_TRUE(result.has_value())
            << "Chunk " << i / chunk_size << " failed: status="
            << (result.has_value() ? 0 : static_cast<int>(result.error().status)) << ", message='"
            << (result.has_value() ? "" : result.error().message) << "'";
    }

    // End session
    auto end_result = engine_->endSession(1);
    EXPECT_TRUE(end_result.has_value());
}

TEST_F(EndToEndTest, LoadMasterCallAndCompare) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(10);
    const auto start_time = std::chrono::steady_clock::now();

    // This test would require having test audio files
    // For unit testing, we might want to generate synthetic master calls

    // Start a session before loading master call
    auto session_result = engine_->startSession(1);
    ASSERT_TRUE(session_result.has_value());

    // Load a master call (would need test data)
    auto load_result = engine_->loadMasterCall("data/master_calls/buck_grunt.wav");
    if (!load_result.has_value()) {
        std::cout << "LoadMasterCall failed: status="
                  << static_cast<int>(load_result.error().status) << ", message='"
                  << load_result.error().message << "'\n";
        ADD_FAILURE() << "LoadMasterCall failed: status="
                      << static_cast<int>(load_result.error().status) << ", message='"
                      << load_result.error().message << "'";
    }

    // Check timeout during load operation
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during master call loading";

    // If the file exists, load_result should succeed; otherwise, expect FILE_NOT_FOUND
    if (load_result.has_value()) {
        std::cout << "Master call loaded successfully." << std::endl;
        SUCCEED() << "Master call loaded successfully.";
    } else {
        std::cout << "Master call failed to load: status="
                  << static_cast<int>(load_result.error().status) << ", message='"
                  << load_result.error().message << "'\n";
        EXPECT_EQ(load_result.error().status, EngineStatus::FILE_NOT_FOUND)
            << "Unexpected error status: " << static_cast<int>(load_result.error().status)
            << ", message='" << load_result.error().message << "'";
    }

    // For now, just verify the API works
    EXPECT_TRUE(engine_->isInitialized());
}

TEST_F(EndToEndTest, EngineInitializesSuccessfully) { ASSERT_TRUE(engine_->isInitialized()); }