#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class UnifiedEngineDebugTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;
    std::string testLogFile = "debug_test_log.txt";

    void SetUp() override {
        DebugLogger::getInstance().enableFileLogging(testLogFile);
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::DEBUG);

        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            [[maybe_unused]] auto status = engine->destroySession(sessionId);
        }
        std::remove(testLogFile.c_str());
    }

    std::string readLogFile() {
        std::ifstream logStream(testLogFile);
        if (!logStream)
            return "";
        return std::string((std::istreambuf_iterator<char>(logStream)),
                           std::istreambuf_iterator<char>());
    }
};

TEST_F(UnifiedEngineDebugTest, SessionCreationAndDestructionLogging) {
    // Session is created in SetUp and destroyed in TearDown.
    // We just need to check if the log contains the relevant messages.
    TearDown();  // Manually call TearDown to trigger destruction logging before the test ends.

    std::string logContents = readLogFile();
    EXPECT_NE(logContents.find("Session created successfully"), std::string::npos);
    EXPECT_NE(logContents.find("Destroying session"), std::string::npos);
}

TEST_F(UnifiedEngineDebugTest, AudioProcessingLogging) {
    std::vector<float> audio(1024, 0.1f);
    [[maybe_unused]] auto status = engine->processAudioChunk(sessionId, audio);

    std::string logContents = readLogFile();
    // Look for the actual log message from UnifiedAudioEngine
    EXPECT_NE(logContents.find("Processing audio chunk"), std::string::npos);
}

TEST_F(UnifiedEngineDebugTest, MasterCallLogging) {
    [[maybe_unused]] auto status =
        engine->loadMasterCall(sessionId, "non_existent_call");  // This will fail, but should log

    std::string logContents = readLogFile();
    EXPECT_NE(logContents.find("Attempting to load master call"), std::string::npos);
    EXPECT_NE(logContents.find("Failed to load master call"), std::string::npos);
}
