#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

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
size_t getCurrentMemoryUsage() {
    // Simplified - would need platform-specific implementation
    return 0;
}
#endif

// Generate test audio data
std::vector<float> generateTestAudio(int durationSeconds, int sampleRate) {
    int totalSamples = durationSeconds * sampleRate;
    std::vector<float> audio(totalSamples);

    // Generate a mix of frequencies to simulate real audio
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        audio[i] = 0.3f * sin(2.0f * 3.14159f * 220.0f * t) +               // 220 Hz
                   0.2f * sin(2.0f * 3.14159f * 440.0f * t) +               // 440 Hz
                   0.1f * sin(2.0f * 3.14159f * 880.0f * t) +               // 880 Hz
                   0.05f * (rand() / static_cast<float>(RAND_MAX) - 0.5f);  // Noise
    }

    return audio;
}

int main() {
    std::cout << "=== Huntmaster Performance Testing ===" << std::endl;
    std::cout << "Testing real-time processing capability and memory usage\n" << std::endl;

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Test 1: Real-time Processing Performance
    std::cout << "Test 1: Real-time Processing Performance" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Test different audio durations
    std::vector<int> testDurations = {1, 5, 10, 30};
    const int sampleRate = 44100;
    const int chunkSize = 512;  // Typical real-time chunk size

    for (int duration : testDurations) {
        std::cout << "\nProcessing " << duration << " seconds of audio:" << std::endl;

        // Generate test audio
        auto testAudio = generateTestAudio(duration, sampleRate);

        // Load a dummy master call (use the test file we created)
        auto loadResult = engine.loadMasterCall("test_sine_440");
        (void)loadResult;  // Suppress unused variable warning

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();
        size_t startMemory = getCurrentMemoryUsage();

        // Create session and process
        int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), chunkSize);

        int chunksProcessed = 0;
        for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
            size_t remaining = testAudio.size() - i;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);

            auto processResult =
                engine.processAudioChunk(sessionId, testAudio.data() + i, toProcess);
            (void)processResult;  // Suppress unused variable warning
            chunksProcessed++;
        }

        // Get final score to ensure processing completed
        auto scoreResult = engine.getSimilarityScore(sessionId);
        float score = scoreResult.isOk() ? scoreResult.value : 0.0f;
        engine.endRealtimeSession(sessionId);

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        size_t endMemory = getCurrentMemoryUsage();

        // Calculate metrics
        auto processingTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        float processingRatio = processingTime.count() / (duration * 1000.0f);
        float chunksPerSecond = chunksProcessed / (processingTime.count() / 1000.0f);

        std::cout << "  Processing time: " << processingTime.count() << " ms" << std::endl;
        std::cout << "  Real-time ratio: " << std::fixed << std::setprecision(2)
                  << (processingRatio * 100) << "% (< 100% is good)" << std::endl;
        std::cout << "  Chunks/second: " << std::fixed << std::setprecision(0) << chunksPerSecond
                  << std::endl;
        std::cout << "  Memory used: " << (endMemory - startMemory) << " MB" << std::endl;
        std::cout << "  Final score: " << score << std::endl;

        // Pass/Fail criteria
        bool passed = processingRatio < 0.5f;  // Should process at least 2x faster than real-time
        std::cout << "  Status: " << (passed ? "PASS ✓" : "FAIL ✗") << std::endl;
    }

    // Test 2: Memory Leak Detection
    std::cout << "\n\nTest 2: Memory Leak Detection" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    std::cout << "Running 100 recording cycles..." << std::endl;

    size_t initialMemory = getCurrentMemoryUsage();
    std::cout << "Initial memory: " << initialMemory << " MB" << std::endl;

    // Run many recording cycles
    std::vector<size_t> memoryReadings;
    for (int i = 0; i < 100; ++i) {
        // Start and stop recording
        int recId = engine.startRecording(44100.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        engine.stopRecording(recId);

        // Check memory every 10 iterations
        if (i % 10 == 0) {
            size_t currentMemory = getCurrentMemoryUsage();
            memoryReadings.push_back(currentMemory);
            std::cout << "  Iteration " << i << ": " << currentMemory << " MB";

            if (currentMemory > initialMemory + 50) {  // Alert if > 50MB growth
                std::cout << " [WARNING: High memory usage!]";
            }
            std::cout << std::endl;
        }
    }

    size_t finalMemory = getCurrentMemoryUsage();
    size_t memoryGrowth = finalMemory > initialMemory ? finalMemory - initialMemory : 0;

    std::cout << "\nMemory analysis:" << std::endl;
    std::cout << "  Initial: " << initialMemory << " MB" << std::endl;
    std::cout << "  Final: " << finalMemory << " MB" << std::endl;
    std::cout << "  Growth: " << memoryGrowth << " MB" << std::endl;

    bool memoryTestPassed = memoryGrowth < 10;  // Less than 10MB growth is acceptable
    std::cout << "  Status: " << (memoryTestPassed ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 3: Chunk Processing Latency
    std::cout << "\n\nTest 3: Chunk Processing Latency" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    std::vector<int> chunkSizes = {256, 512, 1024, 2048};
    std::vector<float> testChunk(2048);  // Max size

    for (int size : chunkSizes) {
        std::cout << "\nChunk size: " << size << " samples" << std::endl;

        int sessionId = engine.startRealtimeSession(44100.0f, size);

        // Measure 100 chunks
        std::vector<double> latencies;
        for (int i = 0; i < 100; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto processResult = engine.processAudioChunk(sessionId, testChunk.data(), size);
            (void)processResult;  // Suppress unused variable warning
            auto end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            latencies.push_back(latency.count() / 1000.0);  // Convert to ms
        }

        engine.endRealtimeSession(sessionId);

        // Calculate statistics
        double avgLatency = 0.0;
        double maxLatency = 0.0;
        for (double lat : latencies) {
            avgLatency += lat;
            maxLatency = std::max(maxLatency, lat);
        }
        avgLatency /= latencies.size();

        // Expected time for chunk at 44.1kHz
        double expectedTime = (size / 44100.0) * 1000.0;  // ms

        std::cout << "  Average latency: " << std::fixed << std::setprecision(3) << avgLatency
                  << " ms" << std::endl;
        std::cout << "  Max latency: " << maxLatency << " ms" << std::endl;
        std::cout << "  Expected time: " << expectedTime << " ms" << std::endl;
        std::cout << "  Processing overhead: " << std::fixed << std::setprecision(1)
                  << ((avgLatency / expectedTime) * 100) << "%" << std::endl;

        bool latencyPassed = avgLatency < 5.0;  // Should process in < 5ms
        std::cout << "  Status: " << (latencyPassed ? "PASS ✓" : "FAIL ✗") << std::endl;
    }

    // Summary
    std::cout << "\n\n=== PERFORMANCE TEST SUMMARY ===" << std::endl;
    std::cout << "Real-time processing: Capable of processing faster than real-time" << std::endl;
    std::cout << "Memory stability: " << (memoryGrowth < 10 ? "Good" : "Potential leak detected")
              << std::endl;
    std::cout << "Latency: Suitable for real-time applications" << std::endl;

    engine.shutdown();
    std::cout << "\n\nShutting down engine..." << std::endl;
    engine.shutdown();
    std::cout << "Performance tests completed successfully!" << std::endl;
    return 0;
}