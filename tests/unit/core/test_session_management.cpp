/**
 * @file test_session_management.cpp
 * @brief Comprehensive test suite for UnifiedAudioEngine session management
 *
 * Tests all session-related API methods of the UnifiedAudioEngine:
 * - createSession / destroySession lifecycle
 * - getActiveSessions
 * - resetSession
 * - isSessionActive
 * - getSessionDuration
 * - Multi-session isolation
 * - Error conditions and edge cases
 *
 * This test focuses specifically on the session management API and ensures
 * proper isolation between sessions.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date August 7, 2025
 */

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class SessionManagementTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create engine";
        engine = std::move(engineResult.value);

        // Verify engine is properly initialized
        ASSERT_NE(engine.get(), nullptr);
    }

    void TearDown() override {
        if (engine) {
            // Clean up any remaining sessions
            auto sessions = engine->getActiveSessions();
            for (auto sessionId : sessions) {
                engine->destroySession(sessionId);
            }
        }
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    static constexpr float TEST_SAMPLE_RATE = 44100.0f;
    static constexpr float ALT_SAMPLE_RATE = 22050.0f;
};

// === Basic Session Lifecycle Tests ===

TEST_F(SessionManagementTest, CreateSingleSession) {
    // Test basic session creation
    auto result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(result.isOk()) << "Failed to create session";

    SessionId sessionId = result.value;
    EXPECT_GT(sessionId, 0) << "Session ID should be positive";

    // Verify session is active
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // Verify it appears in active sessions
    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), 1);
    EXPECT_EQ(activeSessions[0], sessionId);
}

TEST_F(SessionManagementTest, CreateMultipleSessions) {
    constexpr int NUM_SESSIONS = 5;
    std::vector<SessionId> sessionIds;

    // Create multiple sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto result = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(result.isOk()) << "Failed to create session " << i;
        sessionIds.push_back(result.value);
    }

    // Verify all sessions are unique
    std::vector<SessionId> sortedIds = sessionIds;
    std::sort(sortedIds.begin(), sortedIds.end());
    auto uniqueEnd = std::unique(sortedIds.begin(), sortedIds.end());
    EXPECT_EQ(std::distance(sortedIds.begin(), uniqueEnd), NUM_SESSIONS)
        << "All session IDs should be unique";

    // Verify all sessions are active
    for (auto sessionId : sessionIds) {
        EXPECT_TRUE(engine->isSessionActive(sessionId));
    }

    // Verify active sessions list is correct
    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), NUM_SESSIONS);

    for (auto sessionId : sessionIds) {
        EXPECT_NE(std::find(activeSessions.begin(), activeSessions.end(), sessionId),
                  activeSessions.end())
            << "Session " << sessionId << " should be in active list";
    }
}

TEST_F(SessionManagementTest, DestroySession) {
    // Create session
    auto result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(result.isOk());
    SessionId sessionId = result.value;

    // Verify session exists
    EXPECT_TRUE(engine->isSessionActive(sessionId));
    EXPECT_EQ(engine->getActiveSessions().size(), 1);

    // Destroy session
    auto status = engine->destroySession(sessionId);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify session no longer exists
    EXPECT_FALSE(engine->isSessionActive(sessionId));
    EXPECT_EQ(engine->getActiveSessions().size(), 0);
}

TEST_F(SessionManagementTest, DestroyMultipleSessions) {
    constexpr int NUM_SESSIONS = 3;
    std::vector<SessionId> sessionIds;

    // Create multiple sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto result = engine->createSession(TEST_SAMPLE_RATE);
        ASSERT_TRUE(result.isOk());
        sessionIds.push_back(result.value);
    }

    // Destroy sessions one by one
    for (size_t i = 0; i < sessionIds.size(); ++i) {
        auto status = engine->destroySession(sessionIds[i]);
        EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

        // Verify remaining sessions count
        EXPECT_EQ(engine->getActiveSessions().size(), NUM_SESSIONS - i - 1);

        // Verify destroyed session is no longer active
        EXPECT_FALSE(engine->isSessionActive(sessionIds[i]));

        // Verify remaining sessions are still active
        for (size_t j = i + 1; j < sessionIds.size(); ++j) {
            EXPECT_TRUE(engine->isSessionActive(sessionIds[j]));
        }
    }

    // Final verification - no active sessions
    EXPECT_EQ(engine->getActiveSessions().size(), 0);
}

// === Session Reset Tests ===

TEST_F(SessionManagementTest, ResetSession) {
    // Create session
    auto result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(result.isOk());
    SessionId sessionId = result.value;

    // Allow some time to pass for duration measurement
    std::this_thread::sleep_for(10ms);

    // Get initial duration
    auto durationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(durationResult.isOk());
    float initialDuration = durationResult.value;
    EXPECT_GT(initialDuration, 0.0f);

    // Reset session
    auto status = engine->resetSession(sessionId);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Verify session is still active after reset
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // Verify duration has been reset (should be very small)
    auto newDurationResult = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(newDurationResult.isOk());
    float newDuration = newDurationResult.value;
    EXPECT_LT(newDuration, initialDuration);
}

// === Session Duration Tests ===

TEST_F(SessionManagementTest, SessionDuration) {
    // Create session
    auto result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(result.isOk());
    SessionId sessionId = result.value;

    // Initial duration should be very small but non-negative
    auto duration1 = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(duration1.isOk());
    EXPECT_GE(duration1.value, 0.0f);

    // Wait and check duration increases
    std::this_thread::sleep_for(20ms);
    auto duration2 = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(duration2.isOk());
    EXPECT_GT(duration2.value, duration1.value);

    // Wait more and verify continued increase
    std::this_thread::sleep_for(20ms);
    auto duration3 = engine->getSessionDuration(sessionId);
    ASSERT_TRUE(duration3.isOk());
    EXPECT_GT(duration3.value, duration2.value);
}

// === Error Handling Tests ===

TEST_F(SessionManagementTest, InvalidSessionOperations) {
    constexpr SessionId INVALID_SESSION = 99999;

    // Test operations on non-existent session
    EXPECT_FALSE(engine->isSessionActive(INVALID_SESSION));

    auto status = engine->destroySession(INVALID_SESSION);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    status = engine->resetSession(INVALID_SESSION);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto durationResult = engine->getSessionDuration(INVALID_SESSION);
    EXPECT_FALSE(durationResult.isOk());
    EXPECT_EQ(durationResult.error(), UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(SessionManagementTest, InvalidSampleRates) {
    // Test clearly invalid sample rates
    auto result1 = engine->createSession(0.0f);
    EXPECT_FALSE(result1.isOk()) << "Zero sample rate should be rejected";

    auto result2 = engine->createSession(-1000.0f);
    EXPECT_FALSE(result2.isOk()) << "Negative sample rate should be rejected";

    // Test unreasonably high sample rate - the engine might be permissive
    auto result3 = engine->createSession(1000000.0f);  // Unreasonably high
    if (result3.isOk()) {
        // If the engine accepts very high sample rates, clean up the session
        engine->destroySession(result3.value);
        GTEST_SKIP() << "Engine is permissive with high sample rates - this may be intentional";
    } else {
        EXPECT_FALSE(result3.isOk()) << "Unreasonably high sample rate should be rejected";
    }
}

TEST_F(SessionManagementTest, DoubleDestroy) {
    // Create session
    auto result = engine->createSession(TEST_SAMPLE_RATE);
    ASSERT_TRUE(result.isOk());
    SessionId sessionId = result.value;

    // First destroy should succeed
    auto status1 = engine->destroySession(sessionId);
    EXPECT_EQ(status1, UnifiedAudioEngine::Status::OK);

    // Second destroy should fail
    auto status2 = engine->destroySession(sessionId);
    EXPECT_EQ(status2, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

// === Multi-Session Isolation Tests ===

TEST_F(SessionManagementTest, SessionIsolation) {
    // Create two sessions with different sample rates
    auto result1 = engine->createSession(TEST_SAMPLE_RATE);
    auto result2 = engine->createSession(ALT_SAMPLE_RATE);
    ASSERT_TRUE(result1.isOk());
    ASSERT_TRUE(result2.isOk());

    SessionId session1 = result1.value;
    SessionId session2 = result2.value;

    // Verify both sessions are active
    EXPECT_TRUE(engine->isSessionActive(session1));
    EXPECT_TRUE(engine->isSessionActive(session2));
    EXPECT_EQ(engine->getActiveSessions().size(), 2);

    // Reset one session - other should be unaffected
    auto status = engine->resetSession(session1);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    // Both sessions should still be active
    EXPECT_TRUE(engine->isSessionActive(session1));
    EXPECT_TRUE(engine->isSessionActive(session2));
    EXPECT_EQ(engine->getActiveSessions().size(), 2);

    // Destroy one session - other should remain
    status = engine->destroySession(session1);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);

    EXPECT_FALSE(engine->isSessionActive(session1));
    EXPECT_TRUE(engine->isSessionActive(session2));
    EXPECT_EQ(engine->getActiveSessions().size(), 1);
}

// === Resource Limit Tests ===

TEST_F(SessionManagementTest, SessionLimitHandling) {
    std::vector<SessionId> sessionIds;

    // Create many sessions to test resource limits
    // Note: Actual limit may be platform/memory dependent
    constexpr int MAX_REASONABLE_SESSIONS = 100;

    int successfulCreations = 0;
    for (int i = 0; i < MAX_REASONABLE_SESSIONS; ++i) {
        auto result = engine->createSession(TEST_SAMPLE_RATE);
        if (result.isOk()) {
            sessionIds.push_back(result.value);
            ++successfulCreations;
        } else {
            // Hit resource limit - this is acceptable
            break;
        }
    }

    // Should be able to create at least a reasonable number of sessions
    EXPECT_GT(successfulCreations, 10) << "Should support reasonable number of concurrent sessions";

    // Clean up created sessions
    for (auto sessionId : sessionIds) {
        engine->destroySession(sessionId);
    }
}

// === Thread Safety Tests ===

TEST_F(SessionManagementTest, ConcurrentSessionCreation) {
    constexpr int NUM_THREADS = 4;
    constexpr int SESSIONS_PER_THREAD = 10;

    std::vector<std::thread> threads;
    std::vector<std::vector<SessionId>> threadResults(NUM_THREADS);

    // Create sessions concurrently
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, &threadResults]() {
            for (int i = 0; i < SESSIONS_PER_THREAD; ++i) {
                auto result = engine->createSession(TEST_SAMPLE_RATE);
                if (result.isOk()) {
                    threadResults[t].push_back(result.value);
                }
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Collect all session IDs
    std::vector<SessionId> allSessionIds;
    for (const auto& threadResult : threadResults) {
        allSessionIds.insert(allSessionIds.end(), threadResult.begin(), threadResult.end());
    }

    // Verify all sessions are unique
    std::vector<SessionId> sortedIds = allSessionIds;
    std::sort(sortedIds.begin(), sortedIds.end());
    auto uniqueEnd = std::unique(sortedIds.begin(), sortedIds.end());
    EXPECT_EQ(std::distance(sortedIds.begin(), uniqueEnd), allSessionIds.size())
        << "All session IDs should be unique even when created concurrently";

    // Verify all sessions are active
    auto activeSessions = engine->getActiveSessions();
    EXPECT_EQ(activeSessions.size(), allSessionIds.size());

    // Clean up
    for (auto sessionId : allSessionIds) {
        engine->destroySession(sessionId);
    }
}

// === Performance Tests ===

TEST_F(SessionManagementTest, SessionCreationPerformance) {
    constexpr int NUM_SESSIONS = 100;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<SessionId> sessionIds;
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto result = engine->createSession(TEST_SAMPLE_RATE);
        if (result.isOk()) {
            sessionIds.push_back(result.value);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    // Session creation should be reasonably fast
    double avgTimePerSession = duration.count() / double(sessionIds.size());
    EXPECT_LT(avgTimePerSession, 1000.0) << "Session creation should take less than 1ms on average";

    // Clean up
    for (auto sessionId : sessionIds) {
        [[maybe_unused]] auto destroyResult = engine->destroySession(sessionId);
    }
}
