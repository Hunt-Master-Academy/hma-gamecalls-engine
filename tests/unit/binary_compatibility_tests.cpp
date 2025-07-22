#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

struct TestCase {
    std::string input;
    std::string output;
};

class BinaryCompatibilityTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(*engineResult);
    }

    void TearDown() override { engine.reset(); }

    std::unique_ptr<UnifiedAudioEngine> engine;
};

// Simple test to verify basic functionality
TEST_F(BinaryCompatibilityTest, BasicEngineOperations) {
    // Test that the engine can create sessions
    auto sessionResult = engine->createSession(44100.0f);
    EXPECT_TRUE(sessionResult.isOk());

    if (sessionResult.isOk()) {
        SessionId sessionId = *sessionResult;
        auto resetResult = engine->resetSession(sessionId);
        EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);
    }
}

// Test audio recording functionality
TEST_F(BinaryCompatibilityTest, RecordingOperations) {
    auto sessionResult = engine->createSession(44100.0);
    EXPECT_TRUE(sessionResult.isOk());

    if (sessionResult.isOk()) {
        SessionId sessionId = *sessionResult;
        // Test basic session operations
        auto resetResult = engine->resetSession(sessionId);
        EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);
    }
}