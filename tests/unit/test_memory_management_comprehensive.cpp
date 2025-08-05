/**
 * @file test_memory_management_comprehensive.cpp
 * @brief Comprehensive memory management tests for UnifiedAudioEngine
 */

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MemoryManagementTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);
    }

    void TearDown() override {
        // Clean up any remaining sessions
        auto sessions = engine->getActiveSessions();
        for (auto sessionId : sessions) {
            engine->destroySession(sessionId);
        }
    }

    // Helper to get approximate memory usage (platform-specific)
    size_t getApproximateMemoryUsage() {
#ifdef _WIN32
        // Windows implementation would go here
        return 0;
#else
        // Unix-like implementation would go here
        return 0;
#endif
    }
};

TEST_F(MemoryManagementTest, SessionLifecycleMemoryTest) {
    size_t initialMemory = getApproximateMemoryUsage();
    std::vector<SessionId> sessions;

    // Create multiple sessions
    const int numSessions = 20;
    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessions.push_back(*sessionResult);
    }

    size_t afterCreationMemory = getApproximateMemoryUsage();

    // Process some audio in each session
    std::vector<float> testAudio(4096, 0.1f);
    for (auto sessionId : sessions) {
        auto result = engine->processAudioChunk(sessionId, testAudio);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }

    size_t afterProcessingMemory = getApproximateMemoryUsage();

    // Destroy all sessions
    for (auto sessionId : sessions) {
        auto result = engine->destroySession(sessionId);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }

    size_t finalMemory = getApproximateMemoryUsage();

    // Memory should be properly released (allowing for some variance)
    // This is a basic smoke test - actual values depend on platform
    std::cout << "Memory usage - Initial: " << initialMemory
              << ", After creation: " << afterCreationMemory
              << ", After processing: " << afterProcessingMemory << ", Final: " << finalMemory
              << std::endl;
}

TEST_F(MemoryManagementTest, LargeBufferHandling) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Test with progressively larger buffers
    std::vector<size_t> bufferSizes = {
        1024,     // 1KB
        10240,    // 10KB
        102400,   // 100KB
        1024000,  // 1MB
        10240000  // 10MB
    };

    for (size_t bufferSize : bufferSizes) {
        std::vector<float> largeBuffer(bufferSize, 0.1f);

        auto result = engine->processAudioChunk(sessionId, largeBuffer);

        // Should either succeed or fail gracefully
        EXPECT_TRUE(result == UnifiedAudioEngine::Status::OK
                    || result == UnifiedAudioEngine::Status::OUT_OF_MEMORY
                    || result == UnifiedAudioEngine::Status::PROCESSING_ERROR)
            << "Failed with buffer size: " << bufferSize;

        // If it failed due to memory, don't try larger buffers
        if (result == UnifiedAudioEngine::Status::OUT_OF_MEMORY) {
            std::cout << "Memory limit reached at buffer size: " << bufferSize << std::endl;
            break;
        }
    }

    engine->destroySession(sessionId);
}

TEST_F(MemoryManagementTest, ConcurrentMemoryAccess) {
    const int numThreads = 8;
    const int operationsPerThread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> errorCount{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, operationsPerThread, &successCount, &errorCount, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                // Create session
                auto sessionResult = engine->createSession(44100.0f);
                if (!sessionResult.isOk()) {
                    errorCount++;
                    continue;
                }
                SessionId sessionId = *sessionResult;

                // Process some audio
                std::vector<float> audio(1024 + (t * 100), 0.1f + (t * 0.01f));
                auto processResult = engine->processAudioChunk(sessionId, audio);

                if (processResult == UnifiedAudioEngine::Status::OK) {
                    successCount++;
                } else {
                    errorCount++;
                }

                // Destroy session
                engine->destroySession(sessionId);

                // Small delay to allow some interleaving
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have reasonable success rate
    int totalOperations = numThreads * operationsPerThread;
    double successRate = double(successCount.load()) / totalOperations;

    std::cout << "Concurrent operations - Success: " << successCount.load()
              << ", Errors: " << errorCount.load() << ", Success rate: " << (successRate * 100)
              << "%" << std::endl;

    EXPECT_GT(successRate, 0.8);  // At least 80% success rate
}

TEST_F(MemoryManagementTest, MemoryLeakDetection) {
    // This is a basic test - real leak detection would need specialized tools
    size_t initialMemory = getApproximateMemoryUsage();

    // Perform many operations that should not leak memory
    for (int cycle = 0; cycle < 10; ++cycle) {
        std::vector<SessionId> sessions;

        // Create sessions
        for (int i = 0; i < 10; ++i) {
            auto sessionResult = engine->createSession(44100.0f);
            if (sessionResult.isOk()) {
                sessions.push_back(*sessionResult);
            }
        }

        // Process audio in each session
        std::vector<float> audio(2048, 0.1f);
        for (auto sessionId : sessions) {
            engine->processAudioChunk(sessionId, audio);
        }

        // Reset sessions
        for (auto sessionId : sessions) {
            engine->resetSession(sessionId);
        }

        // Destroy sessions
        for (auto sessionId : sessions) {
            engine->destroySession(sessionId);
        }
    }

    size_t finalMemory = getApproximateMemoryUsage();

    std::cout << "Leak test - Initial memory: " << initialMemory
              << ", Final memory: " << finalMemory << std::endl;

    // This is just a basic smoke test
    // Real leak detection would be done with valgrind, AddressSanitizer, etc.
}

TEST_F(MemoryManagementTest, BufferReuseEfficiency) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Test that processing multiple buffers of same size is efficient
    const size_t bufferSize = 4096;
    const int numIterations = 100;

    std::vector<float> buffer1(bufferSize, 0.1f);
    std::vector<float> buffer2(bufferSize, 0.2f);
    std::vector<float> buffer3(bufferSize, 0.3f);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        // Rotate between different buffers to test reuse efficiency
        std::vector<float>* currentBuffer;
        switch (i % 3) {
            case 0:
                currentBuffer = &buffer1;
                break;
            case 1:
                currentBuffer = &buffer2;
                break;
            case 2:
                currentBuffer = &buffer3;
                break;
        }

        auto result = engine->processAudioChunk(sessionId, *currentBuffer);
        EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    double avgTimePerBuffer = double(duration.count()) / numIterations;
    std::cout << "Buffer reuse test - Average time per buffer: " << avgTimePerBuffer
              << " microseconds" << std::endl;

    // Basic performance expectation
    EXPECT_LT(avgTimePerBuffer, 10000);  // Less than 10ms per buffer on average

    engine->destroySession(sessionId);
}

TEST_F(MemoryManagementTest, FragmentationResistance) {
    // Test that the engine handles fragmented memory allocation patterns well
    std::vector<SessionId> sessions;
    const int numSessions = 50;

    // Create many sessions
    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        if (sessionResult.isOk()) {
            sessions.push_back(*sessionResult);
        }
    }

    // Destroy every other session to create fragmentation
    for (size_t i = 1; i < sessions.size(); i += 2) {
        engine->destroySession(sessions[i]);
        sessions[i] = 0;  // Mark as destroyed
    }

    // Try to create new sessions in the fragmented space
    std::vector<SessionId> newSessions;
    for (int i = 0; i < numSessions / 2; ++i) {
        auto sessionResult = engine->createSession(48000.0f);  // Different sample rate
        if (sessionResult.isOk()) {
            newSessions.push_back(*sessionResult);
        }
    }

    // Should be able to create some new sessions despite fragmentation
    EXPECT_GT(newSessions.size(), 0);

    // Clean up remaining sessions
    for (auto sessionId : sessions) {
        if (sessionId != 0) {
            engine->destroySession(sessionId);
        }
    }
    for (auto sessionId : newSessions) {
        engine->destroySession(sessionId);
    }
}

TEST_F(MemoryManagementTest, ZeroSizeBufferHandling) {
    auto sessionResult = engine->createSession(44100.0f);
    ASSERT_TRUE(sessionResult.isOk());
    SessionId sessionId = *sessionResult;

    // Test with zero-size buffer
    std::vector<float> emptyBuffer;
    auto result = engine->processAudioChunk(sessionId, emptyBuffer);
    EXPECT_EQ(result, UnifiedAudioEngine::Status::OK);  // Empty buffers are handled gracefully

    // Test with single sample
    std::vector<float> singleSample = {0.5f};
    auto result2 = engine->processAudioChunk(sessionId, singleSample);
    // Should either succeed or handle gracefully
    EXPECT_TRUE(result2 == UnifiedAudioEngine::Status::OK
                || result2 == UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    engine->destroySession(sessionId);
}
