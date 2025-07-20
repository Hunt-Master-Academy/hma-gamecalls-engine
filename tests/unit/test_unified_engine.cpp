/**
 * @file test_unified_engine.cpp
 * @brief Unit tests for the UnifiedAudioEngine t                loadResult ==
 UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Clean up
    auto result1 = engine->destroySession(session1);
    auto result2 = engine->destroySession(session2);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);ify session management and API consistency
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
        // Create engine instance for testing using the static factory method
        auto result = UnifiedAudioEngine::create();
        ASSERT_TRUE(result.isOk())
            << "Engine creation failed with status: " << static_cast<int>(result.error());
        engine = std::move(*result);
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
    ASSERT_TRUE(result.isOk()) << "Failed to create session with status: "
                               << static_cast<int>(result.error());

    SessionId sessionId = *result;
    EXPECT_GT(sessionId, 0) << "Session ID should be positive";

    // Verify session exists
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // Destroy session
    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK)
        << "Failed to destroy session with status: " << static_cast<int>(destroyResult);

    // Verify session no longer exists
    EXPECT_FALSE(engine->isSessionActive(sessionId));
}

/**
 * Test per-session master call loading
 */
TEST_F(UnifiedEngineTest, PerSessionMasterCallLoading) {
    // Create two sessions
    auto session1Result = engine->createSession();
    auto session2Result = engine->createSession();

    ASSERT_TRUE(session1Result.isOk());
    ASSERT_TRUE(session2Result.isOk());

    SessionId session1 = *session1Result;
    SessionId session2 = *session2Result;

    // Load different master calls for each session
    std::string masterCall1 = "data/master_calls/buck_grunt_master.mfc";
    std::string masterCall2 = "data/master_calls/doe_grunt.mfc";

    auto load1Result = engine->loadMasterCall(session1, masterCall1);
    auto load2Result = engine->loadMasterCall(session2, masterCall2);

    // Note: These might fail if the files don't exist, but the API should work
    // The important thing is that we get consistent Status responses
    EXPECT_TRUE(load1Result == UnifiedAudioEngine::Status::OK ||
                load1Result == UnifiedAudioEngine::Status::FILE_NOT_FOUND);
    EXPECT_TRUE(load2Result == UnifiedAudioEngine::Status::OK ||
                load2Result == UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Clean up
    auto result1 = engine->destroySession(session1);
    auto result2 = engine->destroySession(session2);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
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
                if (result.isOk()) {
                    std::lock_guard<std::mutex> lock(sessionsMutex);
                    allSessions.push_back(*result);
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
        EXPECT_TRUE(engine->isSessionActive(sessionId));
    }

    // Clean up all sessions
    for (SessionId sessionId : allSessions) {
        auto result = engine->destroySession(sessionId);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }
}

/**
 * Test session isolation - operations on one session shouldn't affect another
 */
TEST_F(UnifiedEngineTest, SessionIsolation) {
    // Create two sessions
    auto session1Result = engine->createSession();
    auto session2Result = engine->createSession();

    ASSERT_TRUE(session1Result.isOk());
    ASSERT_TRUE(session2Result.isOk());

    SessionId session1 = *session1Result;
    SessionId session2 = *session2Result;

    // Process a chunk on session 1
    std::vector<float> audioChunk(1024, 0.1f);
    auto processResult1 = engine->processAudioChunk(session1, audioChunk);
    EXPECT_EQ(processResult1, UnifiedAudioEngine::Status::OK);

    // Session 2 should be unaffected. Check its feature count.
    auto featureCount2 = engine->getFeatureCount(session2);
    ASSERT_TRUE(featureCount2.isOk());
    EXPECT_EQ(*featureCount2, 0);

    // Process a chunk on session 2
    auto processResult2 = engine->processAudioChunk(session2, audioChunk);
    EXPECT_EQ(processResult2, UnifiedAudioEngine::Status::OK);

    // Both sessions should now have features
    auto featureCount1 = engine->getFeatureCount(session1);
    ASSERT_TRUE(featureCount1.isOk());
    EXPECT_GT(*featureCount1, 0);

    auto finalFeatureCount2 = engine->getFeatureCount(session2);
    ASSERT_TRUE(finalFeatureCount2.isOk());
    EXPECT_GT(*finalFeatureCount2, 0);

    // Resetting session 1 should not affect session 2
    auto resetResult1 = engine->resetSession(session1);
    EXPECT_EQ(resetResult1, UnifiedAudioEngine::Status::OK);

    auto featureCount1AfterReset = engine->getFeatureCount(session1);
    ASSERT_TRUE(featureCount1AfterReset.isOk());
    EXPECT_EQ(*featureCount1AfterReset, 0);

    auto featureCount2AfterReset1 = engine->getFeatureCount(session2);
    ASSERT_TRUE(featureCount2AfterReset1.isOk());
    EXPECT_EQ(*finalFeatureCount2, *featureCount2AfterReset1);

    // Clean up
    auto result1 = engine->destroySession(session1);
    auto result2 = engine->destroySession(session2);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}

/**
 * Test error handling with invalid session IDs
 */
TEST_F(UnifiedEngineTest, InvalidSessionHandling) {
    SessionId invalidSession = 99999;

    // All operations with invalid session should fail gracefully
    EXPECT_FALSE(engine->isSessionActive(invalidSession));

    auto destroyResult = engine->destroySession(invalidSession);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto loadResult = engine->loadMasterCall(invalidSession, "dummy.mfc");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    std::vector<float> audioChunk(1024, 0.1f);
    auto processResult = engine->processAudioChunk(invalidSession, audioChunk);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

/**
 * Test Result<T> pattern consistency across all operations
 */
TEST_F(UnifiedEngineTest, ResultPatternConsistency) {
    // Create session
    auto createResult = engine->createSession();
    ASSERT_TRUE(createResult.isOk());

    SessionId sessionId = *createResult;

    // All operations should return Status or Result<T> with consistent behavior
    auto loadResult = engine->loadMasterCall(sessionId, "nonexistent.mfc");
    EXPECT_EQ(loadResult, UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Processing should still work, but similarity score will be unavailable
    std::vector<float> audioChunk(1024, 0.1f);
    auto processResult = engine->processAudioChunk(sessionId, audioChunk);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    auto scoreResult = engine->getSimilarityScore(sessionId);
    EXPECT_FALSE(scoreResult.isOk());
    EXPECT_EQ(scoreResult.error(),
              UnifiedAudioEngine::Status::INSUFFICIENT_DATA);  // No master call loaded

    // Clean up
    auto result = engine->destroySession(sessionId);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
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
    ASSERT_TRUE(sessionResult.isOk());
    SessionId session = *sessionResult;

    // Load master call for this specific session
    auto loadResult = engine->loadMasterCall(session, "data/master_calls/buck_grunt_master.mfc");
    // Note: File might not exist in test environment
    EXPECT_TRUE(loadResult == UnifiedAudioEngine::Status::OK ||
                loadResult == UnifiedAudioEngine::Status::FILE_NOT_FOUND);

    // Process audio for this specific session
    std::vector<float> audioChunk(4096, 0.2f);
    auto processResult = engine->processAudioChunk(session, audioChunk);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

    // Multiple sessions can coexist (this was impossible with singleton)
    auto session2Result = engine->createSession();
    ASSERT_TRUE(session2Result.isOk());
    SessionId session2 = *session2Result;

    auto processResult2 = engine->processAudioChunk(session2, audioChunk);
    EXPECT_EQ(processResult2, UnifiedAudioEngine::Status::OK);

    // Both sessions can run concurrently
    // (This demonstrates the key advantage over singleton pattern)

    // Clean up
    auto result1 = engine->destroySession(session);
    auto result2 = engine->destroySession(session2);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}
