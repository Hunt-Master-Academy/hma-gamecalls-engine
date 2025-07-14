/**
 * @file test_unified_engine.cpp
 * @brief Unit tests for the UnifiedAudioEngine to verify session management and API consistency
 *
 * This test suite verifies that the new UnifiedAudioEngine correctly implements:
 * - Session-based audio processing with isolation
 * - Per-session master call management
 * - Thread-safe concurrent session handling
 * - Consistent Result<T> error handling
 * - Migration from legacy singleton patterns
 */

#include <gtest/gtest.h>
#include <huntmaster/core/UnifiedAudioEngine.h>

#include <chrono>
#include <thread>

using namespace huntmaster;

class UnifiedEngineTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Create engine instance for testing
        engine = std::make_unique<UnifiedAudioEngine>();
    }

    void TearDown() override {
        // Clean up any sessions that might be hanging around
        engine.reset();
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
};

/**
 * Test basic session creation and destruction
 */
TEST_F(UnifiedEngineTest, SessionCreationAndDestruction) {
    // Create a session
    auto result = engine->createSession();
    ASSERT_TRUE(result.isSuccess()) << "Failed to create session: " << result.getMessage();

    SessionId sessionId = result.getValue();
    EXPECT_GT(sessionId, 0) << "Session ID should be positive";

    // Verify session exists
    EXPECT_TRUE(engine->hasSession(sessionId));

    // Destroy session
    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_TRUE(destroyResult.isSuccess())
        << "Failed to destroy session: " << destroyResult.getMessage();

    // Verify session no longer exists
    EXPECT_FALSE(engine->hasSession(sessionId));
}

/**
 * Test per-session master call loading
 */
TEST_F(UnifiedEngineTest, PerSessionMasterCallLoading) {
    // Create two sessions
    auto session1Result = engine->createSession();
    auto session2Result = engine->createSession();

    ASSERT_TRUE(session1Result.isSuccess());
    ASSERT_TRUE(session2Result.isSuccess());

    SessionId session1 = session1Result.getValue();
    SessionId session2 = session2Result.getValue();

    // Load different master calls for each session
    std::string masterCall1 = "data/master_calls/buck_grunt_master.mfc";
    std::string masterCall2 = "data/master_calls/doe_grunt.mfc";

    auto load1Result = engine->loadMasterCall(session1, masterCall1);
    auto load2Result = engine->loadMasterCall(session2, masterCall2);

    // Note: These might fail if the files don't exist, but the API should work
    // The important thing is that we get consistent Result<T> responses
    EXPECT_TRUE(load1Result.isSuccess() ||
                load1Result.getStatus() == ProcessingStatus::FILE_NOT_FOUND);
    EXPECT_TRUE(load2Result.isSuccess() ||
                load2Result.getStatus() == ProcessingStatus::FILE_NOT_FOUND);

    // Clean up
    engine->destroySession(session1);
    engine->destroySession(session2);
}

/**
 * Test concurrent session operations for thread safety
 */
TEST_F(UnifiedEngineTest, ConcurrentSessionOperations) {
    const int numThreads = 4;
    const int sessionsPerThread = 3;

    std::vector<std::thread> threads;
    std::vector<SessionId> allSessions;
    std::mutex sessionsMutex;

    // Create sessions concurrently
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, &allSessions, &sessionsMutex, sessionsPerThread]() {
            for (int s = 0; s < sessionsPerThread; ++s) {
                auto result = engine->createSession();
                if (result.isSuccess()) {
                    std::lock_guard<std::mutex> lock(sessionsMutex);
                    allSessions.push_back(result.getValue());
                }

                // Small delay to encourage race conditions if they exist
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all sessions were created successfully
    EXPECT_EQ(allSessions.size(), numThreads * sessionsPerThread);

    // Verify all sessions exist and have unique IDs
    std::set<SessionId> uniqueSessions(allSessions.begin(), allSessions.end());
    EXPECT_EQ(uniqueSessions.size(), allSessions.size()) << "Session IDs should be unique";

    for (SessionId sessionId : allSessions) {
        EXPECT_TRUE(engine->hasSession(sessionId));
    }

    // Clean up all sessions
    for (SessionId sessionId : allSessions) {
        auto result = engine->destroySession(sessionId);
        EXPECT_TRUE(result.isSuccess());
    }
}

/**
 * Test session isolation - operations on one session shouldn't affect another
 */
TEST_F(UnifiedEngineTest, SessionIsolation) {
    // Create two sessions
    auto session1Result = engine->createSession();
    auto session2Result = engine->createSession();

    ASSERT_TRUE(session1Result.isSuccess());
    ASSERT_TRUE(session2Result.isSuccess());

    SessionId session1 = session1Result.getValue();
    SessionId session2 = session2Result.getValue();

    // Start processing on session 1
    auto startResult1 = engine->startProcessing(session1);
    EXPECT_TRUE(startResult1.isSuccess());

    // Session 2 should be unaffected
    EXPECT_FALSE(engine->isProcessing(session2));

    // Start processing on session 2
    auto startResult2 = engine->startProcessing(session2);
    EXPECT_TRUE(startResult2.isSuccess());

    // Both sessions should now be processing independently
    EXPECT_TRUE(engine->isProcessing(session1));
    EXPECT_TRUE(engine->isProcessing(session2));

    // Stop processing on session 1
    auto stopResult1 = engine->stopProcessing(session1);
    EXPECT_TRUE(stopResult1.isSuccess());

    // Session 2 should still be processing
    EXPECT_FALSE(engine->isProcessing(session1));
    EXPECT_TRUE(engine->isProcessing(session2));

    // Clean up
    engine->stopProcessing(session2);
    engine->destroySession(session1);
    engine->destroySession(session2);
}

/**
 * Test error handling with invalid session IDs
 */
TEST_F(UnifiedEngineTest, InvalidSessionHandling) {
    SessionId invalidSession = 99999;

    // All operations with invalid session should fail gracefully
    EXPECT_FALSE(engine->hasSession(invalidSession));

    auto destroyResult = engine->destroySession(invalidSession);
    EXPECT_FALSE(destroyResult.isSuccess());
    EXPECT_EQ(destroyResult.getStatus(), ProcessingStatus::INVALID_SESSION);

    auto loadResult = engine->loadMasterCall(invalidSession, "dummy.mfc");
    EXPECT_FALSE(loadResult.isSuccess());
    EXPECT_EQ(loadResult.getStatus(), ProcessingStatus::INVALID_SESSION);

    auto startResult = engine->startProcessing(invalidSession);
    EXPECT_FALSE(startResult.isSuccess());
    EXPECT_EQ(startResult.getStatus(), ProcessingStatus::INVALID_SESSION);

    EXPECT_FALSE(engine->isProcessing(invalidSession));
}

/**
 * Test Result<T> pattern consistency across all operations
 */
TEST_F(UnifiedEngineTest, ResultPatternConsistency) {
    // Create session
    auto createResult = engine->createSession();
    ASSERT_TRUE(createResult.isSuccess());

    SessionId sessionId = createResult.getValue();

    // All operations should return Result<T> with consistent behavior
    auto loadResult = engine->loadMasterCall(sessionId, "nonexistent.mfc");
    EXPECT_FALSE(loadResult.isSuccess());
    EXPECT_FALSE(loadResult.getMessage().empty());
    EXPECT_NE(loadResult.getStatus(), ProcessingStatus::SUCCESS);

    auto startResult = engine->startProcessing(sessionId);
    // This should succeed even without a master call loaded
    EXPECT_TRUE(startResult.isSuccess() ||
                startResult.getStatus() == ProcessingStatus::NO_MASTER_CALL);

    // Clean up
    engine->destroySession(sessionId);
}

/**
 * Integration test demonstrating migration from legacy engines
 */
TEST_F(UnifiedEngineTest, LegacyMigrationPattern) {
    // This test demonstrates how to migrate from singleton patterns to session-based

    // OLD PATTERN (what we're replacing):
    // HuntmasterAudioEngine::getInstance().initializeFromConfig(config);
    // HuntmasterAudioEngine::getInstance().loadMasterCall("master.mfc");
    // HuntmasterAudioEngine::getInstance().startProcessing();

    // NEW PATTERN (UnifiedAudioEngine):
    auto sessionResult = engine->createSession();
    ASSERT_TRUE(sessionResult.isSuccess());
    SessionId session = sessionResult.getValue();

    // Load master call for this specific session
    auto loadResult = engine->loadMasterCall(session, "data/master_calls/buck_grunt_master.mfc");
    // Note: File might not exist in test environment
    EXPECT_TRUE(loadResult.isSuccess() ||
                loadResult.getStatus() == ProcessingStatus::FILE_NOT_FOUND);

    // Start processing for this specific session
    auto startResult = engine->startProcessing(session);
    EXPECT_TRUE(startResult.isSuccess() ||
                startResult.getStatus() == ProcessingStatus::NO_MASTER_CALL);

    // Multiple sessions can coexist (this was impossible with singleton)
    auto session2Result = engine->createSession();
    ASSERT_TRUE(session2Result.isSuccess());
    SessionId session2 = session2Result.getValue();

    auto startResult2 = engine->startProcessing(session2);
    EXPECT_TRUE(startResult2.isSuccess() ||
                startResult2.getStatus() == ProcessingStatus::NO_MASTER_CALL);

    // Both sessions can run concurrently
    // (This demonstrates the key advantage over singleton pattern)

    // Clean up
    engine->stopProcessing(session);
    engine->stopProcessing(session2);
    engine->destroySession(session);
    engine->destroySession(session2);
}
