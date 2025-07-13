#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;

// Memory growth threshold for leak detection (in MB)
constexpr size_t MEMORY_GROWTH_THRESHOLD_MB = 50;

// Platform-specific memory usage functions
#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
size_t getCurrentMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
    return pmc.WorkingSetSize / (1024 * 1024);  // Convert to MB
}
#else
#if defined(__linux__)
#include <unistd.h>

#include <fstream>
size_t getCurrentMemoryUsage() {
    long rss = 0L;
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        statm >> rss;  // Read RSS value from /proc/self/statm
        statm.close();
        return rss * sysconf(_SC_PAGESIZE) / (1024 * 1024);  // Convert to MB
    }
    return 0;
}
#else
// Default implementation for other platforms
size_t getCurrentMemoryUsage() {
    return 0;  // Not implemented for this platform
}
#endif
#endif

// Generate test audio data
std::vector<float> generateTestAudio(int durationSeconds, int sampleRate) {
    int totalSamples = durationSeconds * sampleRate;
    std::vector<float> audio(totalSamples);

    // Use a fixed seed for reproducibility
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> noiseDist(-0.05f, 0.05f);

    // Generate a mix of frequencies to simulate real audio
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        audio[i] = 0.3f * sin(2.0f * 3.14159f * 220.0f * t) +  // 220 Hz
                   0.2f * sin(2.0f * 3.14159f * 440.0f * t) +  // 440 Hz
                   0.1f * sin(2.0f * 3.14159f * 880.0f * t) +  // 880 Hz
                   noiseDist(rng);                             // Small amount of noise
    }

    return audio;
}

// Performance tests converted to Google Test format
TEST(PerformanceTest, RealtimeProcessingCapability) {
    std::cout << "=== Huntmaster Performance Testing ===" << std::endl;
    std::cout << "Testing real-time processing capability and memory usage\n" << std::endl;

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Test 1: Real-time Processing Performance
    std::cout << "Test 1: Real-time Processing Performance" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Test different audio durations
    std::vector<int> testDurations = {1, 5, 10};  // Reduced for faster testing
    const int sampleRate = 44100;
    const int chunkSize = 512;  // Typical real-time chunk size

    // Load a master call for comparison
    auto loadResult = engine.loadMasterCall("buck_grunt");
    EXPECT_EQ(loadResult, HuntmasterAudioEngine::EngineStatus::OK);

    bool allTestsPassed = true;

    for (int duration : testDurations) {
        std::cout << "\nTesting " << duration << " second audio processing..." << std::endl;

        // Generate test audio
        std::vector<float> audio = generateTestAudio(duration, sampleRate);

        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();

        int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), chunkSize);
        EXPECT_GE(sessionId, 0);

        // Process in chunks to simulate real-time
        for (size_t i = 0; i < audio.size(); i += chunkSize) {
            size_t remaining = audio.size() - i;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);

            auto chunkResult = engine.processAudioChunk(sessionId, audio.data() + i, toProcess);
            EXPECT_EQ(chunkResult, HuntmasterAudioEngine::EngineStatus::OK);
        }

        float score = engine.getSimilarityScore(sessionId);
        engine.endRealtimeSession(sessionId);

        auto end = std::chrono::high_resolution_clock::now();
        auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        float realTimeRatio = static_cast<float>(processingTime.count()) / (duration * 1000.0f);

        std::cout << "  Processing time: " << processingTime.count() << " ms" << std::endl;
        std::cout << "  Real-time ratio: " << realTimeRatio << "x" << std::endl;
        std::cout << "  Score: " << score << std::endl;

        // Should process faster than real-time for real-time applications
        bool testPassed = realTimeRatio < 1.0f;  // Should be faster than real-time
        if (!testPassed) {
            allTestsPassed = false;
        }

        std::cout << "  Status: " << (testPassed ? "PASS ✓" : "FAIL ✗") << std::endl;
        EXPECT_LT(realTimeRatio, 1.0f) << "Processing should be faster than real-time";
    }

    engine.shutdown();

    // Google Test assertions
    EXPECT_TRUE(allTestsPassed) << "Real-time processing tests failed";
}

TEST(PerformanceTest, MemoryUsageMonitoring) {
    std::cout << "\n=== Memory Usage Monitoring Test ===" << std::endl;

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load a master call for testing
    auto loadResult = engine.loadMasterCall("buck_grunt");
    EXPECT_EQ(loadResult, HuntmasterAudioEngine::EngineStatus::OK);

    const int sampleRate = 44100;
    const int chunkSize = 512;

    size_t initialMemory = getCurrentMemoryUsage();
    std::cout << "Initial memory: " << initialMemory << " MB" << std::endl;

    // Process multiple sessions to check for memory leaks
    for (int i = 0; i < 20; ++i) {  // Reduced iterations for faster testing
        std::vector<float> audio = generateTestAudio(1, sampleRate);  // 1 second audio

        int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), chunkSize);

        for (size_t j = 0; j < audio.size(); j += chunkSize) {
            size_t remaining = audio.size() - j;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
            auto result = engine.processAudioChunk(sessionId, audio.data() + j, toProcess);
            // Note: We don't assert here since this is a memory test, not functionality test
            (void)result;  // Suppress unused variable warning
        }

        engine.endRealtimeSession(sessionId);

        if (i % 5 == 4) {
            size_t currentMemory = getCurrentMemoryUsage();
            std::cout << "After " << (i + 1) << " iterations: " << currentMemory << " MB"
                      << std::endl;
        }
    }

    size_t finalMemory = getCurrentMemoryUsage();
    size_t memoryGrowth = finalMemory > initialMemory ? finalMemory - initialMemory : 0;

    std::cout << "Final memory: " << finalMemory << " MB" << std::endl;
    std::cout << "Memory growth: " << memoryGrowth << " MB" << std::endl;

    bool memoryTestPassed = memoryGrowth < MEMORY_GROWTH_THRESHOLD_MB;
    std::cout << "Memory test: " << (memoryTestPassed ? "PASS ✓" : "FAIL ✗") << std::endl;

    engine.shutdown();

    // Google Test assertions
    EXPECT_TRUE(memoryTestPassed) << "Memory growth exceeded acceptable threshold";
}

TEST(PerformanceTest, ChunkProcessingLatency) {
    std::cout << "\n=== Chunk Processing Latency Test ===" << std::endl;

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Load a master call for testing
    auto loadResult = engine.loadMasterCall("buck_grunt");
    EXPECT_EQ(loadResult, HuntmasterAudioEngine::EngineStatus::OK);

    std::vector<int> chunkSizes = {256, 512, 1024, 2048};
    std::vector<float> latencies;

    for (int size : chunkSizes) {
        std::vector<float> testChunk(size, 0.5f);
        int sessionId = engine.startRealtimeSession(44100.0f, size);

        auto start = std::chrono::high_resolution_clock::now();
        auto chunkResult = engine.processAudioChunk(sessionId, testChunk.data(), testChunk.size());
        auto end = std::chrono::high_resolution_clock::now();

        EXPECT_EQ(chunkResult, HuntmasterAudioEngine::EngineStatus::OK);

        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float latencyMs = latency.count() / 1000.0f;
        latencies.push_back(latencyMs);

        engine.endRealtimeSession(sessionId);

        std::cout << "Chunk size " << size << ": " << latencyMs << " ms" << std::endl;
    }

    float avgLatency = 0.0f;
    for (float lat : latencies) {
        avgLatency += lat;
    }
    avgLatency /= latencies.size();

    bool latencyPassed = avgLatency < 10.0;  // Should process in < 10ms
    std::cout << "Average latency: " << avgLatency << " ms" << std::endl;
    std::cout << "Latency test: " << (latencyPassed ? "PASS ✓" : "FAIL ✗") << std::endl;

    engine.shutdown();

    // Google Test assertions
    EXPECT_TRUE(latencyPassed) << "Chunk processing latency exceeded acceptable threshold";
}
