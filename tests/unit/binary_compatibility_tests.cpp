#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;

struct TestCase {
    std::string input;
    std::string output;
};

class BinaryCompatibilityTest : public ::testing::Test {
   protected:
    void SetUp() override { engine.initialize(); }

    void TearDown() override { engine.shutdown(); }

    HuntmasterAudioEngine& engine = HuntmasterAudioEngine::getInstance();
};

// Simple test to verify basic functionality
TEST_F(BinaryCompatibilityTest, BasicEngineOperations) {
    // Test that the engine can start and stop sessions
    auto result = engine.startRealtimeSession(44100.0f, 1024);
    EXPECT_TRUE(result.isOk());

    if (result.isOk()) {
        int sessionId = result.value;
        engine.endRealtimeSession(sessionId);
    }
}

// Test audio recording functionality
TEST_F(BinaryCompatibilityTest, RecordingOperations) {
    auto result = engine.startRecording(44100.0);
    EXPECT_TRUE(result.isOk());

    if (result.isOk()) {
        int recordingId = result.value;
        engine.stopRecording(recordingId);
    }
}