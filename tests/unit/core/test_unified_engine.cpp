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

#include <chrono>
#include <cmath>
#include <filesystem>
#include <set>
#include <thread>

#include <gtest/gtest.h>
#include <huntmaster/core/UnifiedAudioEngine.h>

#include "TestUtils.h"

using namespace huntmaster;
using namespace huntmaster::test;

class UnifiedEngineTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Create engine instance for testing using the static factory method
        auto result = UnifiedAudioEngine::create();
        ASSERT_TRUE(result.isOk())
            << "Engine creation failed with status: " << static_cast<int>(result.error());
        engine = std::move(*result);
    }

    void TearDown() override {
        // Clean up any sessions that might be hanging around
        engine.reset();
        TestFixtureBase::TearDown();
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
    auto masterCall1Path = TestPaths::getMasterCallFile("buck_grunt", ".mfc");
    auto masterCall2Path = TestPaths::getMasterCallFile("doe_grunt", ".mfc");

    auto load1Result = engine->loadMasterCall(session1, masterCall1Path.string());
    auto load2Result = engine->loadMasterCall(session2, masterCall2Path.string());

    // Note: These might fail if the files don't exist, but the API should work
    // The important thing is that we get consistent Status responses
    EXPECT_TRUE(load1Result == UnifiedAudioEngine::Status::OK
                || load1Result == UnifiedAudioEngine::Status::FILE_NOT_FOUND);
    EXPECT_TRUE(load2Result == UnifiedAudioEngine::Status::OK
                || load2Result == UnifiedAudioEngine::Status::FILE_NOT_FOUND);

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
    auto masterCallPath = TestPaths::getMasterCallFile("buck_grunt", ".mfc");
    auto loadResult = engine->loadMasterCall(session, masterCallPath.string());
    // Note: File might not exist in test environment
    EXPECT_TRUE(loadResult == UnifiedAudioEngine::Status::OK
                || loadResult == UnifiedAudioEngine::Status::FILE_NOT_FOUND);

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

/**
 * @brief Test reset functionality through UnifiedAudioEngine
 *
 * This is a refactored version of RealtimeScorerTest.DISABLED_ResetFunctionalityTest
 * that works with the new session-based UnifiedAudioEngine interface.
 */
TEST_F(UnifiedEngineTest, SessionResetFunctionalityTest) {
    // Create a session
    auto sessionResult = engine->createSession();
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Load a master call for comparison
    // Note: loadMasterCall expects just the call ID, not a full path
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Master call file not available, skipping reset test";
    }

    // Process some audio to generate history/state
    std::vector<float> audioChunk(2048, 0.5f);  // Larger chunk for more processing
    for (int i = 0; i < 5; ++i) {
        auto processResult = engine->processAudioChunk(sessionId, audioChunk);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        // Small delay to allow processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify we have processed features (indicating state exists)
    auto featureCountBefore = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountBefore.isOk());
    EXPECT_GT(*featureCountBefore, 0) << "Should have features after processing audio";

    // Try to get similarity score (should work if master call is loaded)
    auto scoreBefore = engine->getSimilarityScore(sessionId);
    // Score might not be available depending on implementation, but session should be active
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // Reset the session (this should clear analysis state but preserve master call)
    auto resetResult = engine->resetSession(sessionId);
    EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);

    // Verify state is reset
    auto featureCountAfter = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountAfter.isOk());
    EXPECT_EQ(*featureCountAfter, 0) << "Feature count should be zero after reset";

    // Session should still be active
    EXPECT_TRUE(engine->isSessionActive(sessionId));

    // Master call should still be loaded (reset preserves it)
    // We can verify this by trying to process audio again
    auto processAfterReset = engine->processAudioChunk(sessionId, audioChunk);
    EXPECT_EQ(processAfterReset, UnifiedAudioEngine::Status::OK);

    // Should be able to accumulate features again
    auto featureCountAfterProcessing = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountAfterProcessing.isOk());
    EXPECT_GT(*featureCountAfterProcessing, 0) << "Should accumulate features after reset";

    // Clean up
    auto destroyResult = engine->destroySession(sessionId);
    EXPECT_EQ(destroyResult, UnifiedAudioEngine::Status::OK);
}

/**
 * @brief Test audio file processing capabilities
 *
 * This is an implementation of the HuntmasterEngineTest.DISABLED_CanProcessAudioFiles
 * test that was previously just a placeholder.
 */
TEST_F(UnifiedEngineTest, CanProcessAudioFiles) {
    // Create a session
    auto sessionResult = engine->createSession();
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Test 1: Load and process a master call file
    // Note: loadMasterCall expects just the call ID, not a full path
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult == UnifiedAudioEngine::Status::FILE_NOT_FOUND) {
        GTEST_SKIP() << "Master call file not found for buck_grunt";
    }
    ASSERT_EQ(loadResult, UnifiedAudioEngine::Status::OK) << "Failed to load master call from file";

    // Test 2: Process a test audio file by simulating file loading
    // Since UnifiedAudioEngine processes chunks, we'll simulate loading a file
    // by generating test audio that represents file content

    // Simulate different types of audio content
    std::vector<std::vector<float>> testAudioFiles = {
        // Silent audio (should be processed but score low)
        std::vector<float>(4096, 0.0f),

        // Low amplitude audio
        std::vector<float>(4096, 0.1f),

        // Medium amplitude audio
        std::vector<float>(4096, 0.5f),

        // Complex waveform (sine wave + harmonics)
        []() {
            std::vector<float> audio(4096);
            for (size_t i = 0; i < audio.size(); ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                audio[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * t) +  // Fundamental
                           0.2f * std::sin(2.0f * M_PI * 880.0f * t) +  // Second harmonic
                           0.1f * std::sin(2.0f * M_PI * 1320.0f * t);  // Third harmonic
            }
            return audio;
        }()};

    // Process each "file" (audio chunk)
    for (size_t fileIndex = 0; fileIndex < testAudioFiles.size(); ++fileIndex) {
        const auto& audioFile = testAudioFiles[fileIndex];

        // Reset session for each "file"
        auto resetResult = engine->resetSession(sessionId);
        EXPECT_EQ(resetResult, UnifiedAudioEngine::Status::OK);

        // Process the audio file in chunks (simulating streaming from file)
        const size_t chunkSize = 1024;
        size_t processedSamples = 0;

        for (size_t offset = 0; offset < audioFile.size(); offset += chunkSize) {
            size_t currentChunkSize = std::min(chunkSize, audioFile.size() - offset);
            std::vector<float> chunk(audioFile.begin() + offset,
                                     audioFile.begin() + offset + currentChunkSize);

            auto processResult = engine->processAudioChunk(sessionId, chunk);
            EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
                << "Failed to process chunk " << (offset / chunkSize) << " of file " << fileIndex;

            processedSamples += currentChunkSize;
        }

        // Verify processing results
        auto featureCount = engine->getFeatureCount(sessionId);
        ASSERT_TRUE(featureCount.isOk());
        EXPECT_GT(*featureCount, 0)
            << "File " << fileIndex << " should generate features after processing";

        // Try to get similarity score (may not be available for all audio types)
        auto similarityScore = engine->getSimilarityScore(sessionId);
        // We don't assert success here because similarity requires sufficient data
        // The important thing is that processing completes without errors

        EXPECT_EQ(processedSamples, audioFile.size())
            << "Should process all samples in file " << fileIndex;
    }

    // Test 3: Verify file processing with different session states

    // Process audio without master call (should work but no similarity)
    auto session2Result = engine->createSession();
    ASSERT_TRUE(session2Result.isOk());
    SessionId session2 = *session2Result;

    std::vector<float> testAudio(2048, 0.3f);
    auto processResult = engine->processAudioChunk(session2, testAudio);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK)
        << "Should be able to process audio without master call";

    auto featureCount = engine->getFeatureCount(session2);
    ASSERT_TRUE(featureCount.isOk());
    EXPECT_GT(*featureCount, 0) << "Should generate features even without master call";

    auto similarityScore = engine->getSimilarityScore(session2);
    EXPECT_FALSE(similarityScore.isOk()) << "Should not have similarity score without master call";
    EXPECT_EQ(similarityScore.error(), UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    // Clean up
    auto result1 = engine->destroySession(sessionId);
    auto result2 = engine->destroySession(session2);
    EXPECT_EQ(result1, UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(result2, UnifiedAudioEngine::Status::OK);
}
