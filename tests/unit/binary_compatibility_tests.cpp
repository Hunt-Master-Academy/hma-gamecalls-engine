
#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::UnifiedAudioEngine;

struct TestCase {
    std::string input;
    std::string output;
};

class BinaryCompatibilityTest : public ::testing::Test {
   protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    UnifiedAudioEngine::SessionId sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;

    void SetUp() override {
        auto result = UnifiedAudioEngine::create();
        ASSERT_TRUE(result.isOk()) << "Failed to create UnifiedAudioEngine instance";
        engine = std::move(result.value);
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    }

    void TearDown() override {
        if (engine && sessionId != UnifiedAudioEngine::INVALID_SESSION_ID) {
            engine->destroySession(sessionId);
            sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
        }
        engine.reset();
    }
};

// Simple test to verify basic functionality
TEST_F(BinaryCompatibilityTest, BasicEngineOperations) {
    // Test that the engine can create and destroy sessions
    auto sessionResult = engine->createSession(44100.0f);
    EXPECT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    if (sessionResult.isOk()) {
        auto destroyResult = engine->destroySession(sessionId);
        EXPECT_TRUE(destroyResult.isOk()) << "Failed to destroy session";
        sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
    }
}

// Test audio recording functionality
TEST_F(BinaryCompatibilityTest, RecordingOperations) {
    // Set a timeout for this test to prevent hanging
    const auto timeout = std::chrono::seconds(10);
    const auto start_time = std::chrono::steady_clock::now();

    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
    sessionId = sessionResult.value;

    // If UnifiedAudioEngine supports recording API, use it here
    auto startRecResult = engine->startRecording(sessionId);
    EXPECT_TRUE(startRecResult.isOk()) << "Failed to start recording";

    // Check timeout during recording operations
    ASSERT_LT(std::chrono::steady_clock::now() - start_time, timeout)
        << "Test timed out during recording operations";

    if (startRecResult.isOk()) {
        auto stopRecResult = engine->stopRecording(sessionId);
        EXPECT_TRUE(stopRecResult.isOk()) << "Failed to stop recording";
    }

    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_TRUE(destroyResult.isOk()) << "Failed to destroy session";
    sessionId = UnifiedAudioEngine::INVALID_SESSION_ID;
}