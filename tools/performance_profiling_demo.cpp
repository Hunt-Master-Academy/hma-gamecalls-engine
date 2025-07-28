/**
 * @file performance_profiling_demo.cpp
 * @brief Demonstration of performance profiling and bottleneck resolution tools
 *
 * This tool shows how to integrate the PerformanceProfiler with the UnifiedAudioEngine
 * to identify and resolve performance bottlenecks in real-world audio processing scenarios.
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"
#include "huntmaster/profiling/PerformanceProfiler.h"

using namespace huntmaster;
using namespace huntmaster::profiling;

/**
 * @brief Generate test audio with various complexity levels
 */
std::vector<float> generateComplexTestAudio(int durationSeconds,
                                            float sampleRate,
                                            const std::string& complexity = "medium") {
    int totalSamples = static_cast<int>(durationSeconds * sampleRate);
    std::vector<float> audio(totalSamples);

    if (complexity == "simple") {
        // Simple sine wave
        for (int i = 0; i < totalSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            audio[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * t);
        }
    } else if (complexity == "medium") {
        // Multiple harmonics
        for (int i = 0; i < totalSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            audio[i] = 0.3f * std::sin(2.0f * 3.14159f * 220.0f * t)
                       + 0.2f * std::sin(2.0f * 3.14159f * 440.0f * t)
                       + 0.1f * std::sin(2.0f * 3.14159f * 880.0f * t);
        }
    } else if (complexity == "complex") {
        // Complex waveform with noise and varying frequency
        for (int i = 0; i < totalSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float freq = 440.0f + 100.0f * std::sin(2.0f * 3.14159f * 2.0f * t);  // FM
            audio[i] = 0.3f * std::sin(2.0f * 3.14159f * freq * t)
                       + 0.1f * std::sin(2.0f * 3.14159f * freq * 2.0f * t)
                       + 0.05f * (rand() / static_cast<float>(RAND_MAX) - 0.5f);  // Noise
        }
    }

    return audio;
}

/**
 * @brief Demonstrate basic performance profiling
 */
void demonstrateBasicProfiling() {
    std::cout << "\n=== Basic Performance Profiling Demo ===\n" << std::endl;

    // Create engine and profiler
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return;
    }
    auto engine = std::move(*engineResult);

    PerformanceProfiler profiler;

    // Create session
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session!" << std::endl;
        return;
    }
    SessionId sessionId = *sessionResult;

    // Start profiling
    profiler.startProfiling(sessionId);

    // Load a master call for comparison
    auto loadResult = engine->loadMasterCall(sessionId, "test_sine_440");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        std::cout << "Warning: Could not load master call, continuing with basic profiling..."
                  << std::endl;
    }

    // Generate and process test audio with profiling
    auto testAudio = generateComplexTestAudio(5, 44100.0f, "medium");
    const size_t chunkSize = 512;

    std::cout << "Processing " << testAudio.size() << " samples in chunks of " << chunkSize << "..."
              << std::endl;

    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t remaining = testAudio.size() - i;
        size_t toProcess = std::min(chunkSize, remaining);

        // Profile the chunk processing with automatic timing
        {
            PROFILE_COMPONENT(profiler, sessionId, "MFCC_Processing");
            std::span<const float> chunk(testAudio.data() + i, toProcess);

            auto startTime = std::chrono::high_resolution_clock::now();
            auto status = engine->processAudioChunk(sessionId, chunk);
            auto endTime = std::chrono::high_resolution_clock::now();

            if (status != UnifiedAudioEngine::Status::OK) {
                std::cerr << "Processing failed at chunk " << (i / chunkSize) << std::endl;
                break;
            }

            // Record detailed metrics
            auto processingTime =
                std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            profiler.recordChunkProcessed(sessionId, toProcess, processingTime.count() / 1000.0f);
        }

        // Simulate memory usage recording (in real implementation, this would be automatic)
        static size_t simulatedMemory = 50 * 1024 * 1024;  // 50MB base
        simulatedMemory += (i / chunkSize) * 1024;         // Growth simulation
        profiler.recordMemoryUsage(sessionId, simulatedMemory);

        // Record similarity score if available
        auto scoreResult = engine->getSimilarityScore(sessionId);
        if (scoreResult.isOk()) {
            profiler.recordSimilarityScore(sessionId, *scoreResult);
        }
    }

    // Stop profiling and generate report
    profiler.stopProfiling(sessionId);

    // Get and display metrics
    auto metrics = profiler.getSessionMetrics(sessionId);
    std::cout << "\n--- Session Metrics ---" << std::endl;
    std::cout << "Chunks Processed: " << metrics.totalChunksProcessed << std::endl;
    std::cout << "Average Processing Time: " << metrics.avgChunkProcessingTime << " ms/chunk"
              << std::endl;
    std::cout << "Real-time Ratio: " << metrics.realTimeRatio << " (< 1.0 is good)" << std::endl;
    std::cout << "Peak Memory Usage: " << (metrics.peakMemoryUsage / (1024 * 1024)) << " MB"
              << std::endl;

    // Check for performance alerts
    auto alerts = profiler.checkPerformanceAlerts(sessionId);
    if (!alerts.empty()) {
        std::cout << "\n--- Performance Alerts ---" << std::endl;
        for (const auto& alert : alerts) {
            std::cout << "ALERT: " << alert.description << " (Severity: " << alert.severity << ")"
                      << std::endl;
            for (const auto& suggestion : alert.suggestions) {
                std::cout << "  - " << suggestion << std::endl;
            }
        }
    }

    // Generate full report
    std::cout << "\n" << profiler.generatePerformanceReport(sessionId) << std::endl;

    // Export to JSON for further analysis
    profiler.exportToJson(sessionId, "performance_report.json");
    std::cout << "Performance data exported to: performance_report.json" << std::endl;

    // Cleanup
    engine->destroySession(sessionId);
}

/**
 * @brief Demonstrate bottleneck analysis and optimization
 */
void demonstrateBottleneckAnalysis() {
    std::cout << "\n=== Bottleneck Analysis Demo ===\n" << std::endl;

    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return;
    }
    auto engine = std::move(*engineResult);

    PerformanceProfiler profiler;

    // Set strict performance thresholds for demo
    PerformanceThresholds strictThresholds;
    strictThresholds.maxRealTimeRatio = 0.3f;            // Very strict
    strictThresholds.maxChunkLatency = 5.0f;             // 5ms max
    strictThresholds.maxMemoryUsage = 75 * 1024 * 1024;  // 75MB max
    profiler.setPerformanceThresholds(strictThresholds);

    // Enable real-time monitoring with alert callback
    profiler.enableRealTimeMonitoring(true);
    profiler.setCallbackOnAlert([](const PerformanceAlert& alert) {
        std::cout << "[REAL-TIME ALERT] " << alert.description << std::endl;
    });

    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session!" << std::endl;
        return;
    }
    SessionId sessionId = *sessionResult;

    profiler.startProfiling(sessionId);

    // Process complex audio that will stress the system
    auto complexAudio = generateComplexTestAudio(10, 44100.0f, "complex");
    const size_t chunkSize = 1024;  // Larger chunks for more processing

    std::cout << "Processing complex audio to trigger bottleneck analysis..." << std::endl;

    for (size_t i = 0; i < complexAudio.size(); i += chunkSize) {
        size_t remaining = complexAudio.size() - i;
        size_t toProcess = std::min(chunkSize, remaining);

        // Simulate different component processing times
        {
            PROFILE_COMPONENT(profiler, sessionId, "MFCC_Processing");
            std::this_thread::sleep_for(std::chrono::microseconds(1500));  // Simulate slow MFCC
        }

        {
            PROFILE_COMPONENT(profiler, sessionId, "DTW_Comparison");
            std::this_thread::sleep_for(std::chrono::microseconds(3000));  // Simulate very slow DTW
        }

        {
            PROFILE_COMPONENT(profiler, sessionId, "VAD_Processing");
            std::this_thread::sleep_for(std::chrono::microseconds(500));  // Fast VAD
        }

        // Actual audio processing
        std::span<const float> chunk(complexAudio.data() + i, toProcess);
        auto status = engine->processAudioChunk(sessionId, chunk);

        if (status != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Processing failed!" << std::endl;
            break;
        }

        // Record metrics
        profiler.recordChunkProcessed(sessionId, toProcess, 6.0f);  // 6ms total (over threshold)
        profiler.recordMemoryUsage(sessionId, 80 * 1024 * 1024);    // Over memory threshold
    }

    profiler.stopProfiling(sessionId);

    // Perform bottleneck analysis
    std::cout << "\n--- Bottleneck Analysis ---" << std::endl;
    auto analysis = profiler.analyzeBottlenecks(sessionId);

    std::cout << "Primary Bottleneck: " << analysis.bottleneckComponent << std::endl;
    std::cout << "Impact: " << analysis.impactPercentage << "% of total processing time"
              << std::endl;
    std::cout << "Root Cause: " << analysis.rootCause << std::endl;
    std::cout << "Expected Improvement: " << analysis.expectedImprovement << "%" << std::endl;

    std::cout << "\nOptimization Strategies:" << std::endl;
    for (const auto& strategy : analysis.optimizationStrategies) {
        std::cout << "  - " << strategy << std::endl;
    }

    // Get specific optimization suggestions
    std::cout << "\n--- Optimization Suggestions ---" << std::endl;
    auto suggestions = profiler.suggestOptimizations(sessionId);
    for (const auto& suggestion : suggestions) {
        std::cout << "Component: " << suggestion.component << std::endl;
        std::cout << "  Parameter: " << suggestion.parameter << std::endl;
        std::cout << "  Current: " << suggestion.currentValue
                  << " -> Suggested: " << suggestion.suggestedValue << std::endl;
        std::cout << "  Rationale: " << suggestion.rationale << std::endl;
        std::cout << "  Expected Improvement: " << suggestion.expectedImprovement << "%"
                  << std::endl;
        std::cout << std::endl;
    }

    // Apply automatic optimizations
    std::cout << "Applying automatic optimizations..." << std::endl;
    profiler.applyAutomaticOptimizations(sessionId, engine.get());

    engine->destroySession(sessionId);
}

/**
 * @brief Demonstrate comprehensive benchmarking
 */
void demonstrateComprehensiveBenchmarking() {
    std::cout << "\n=== Comprehensive Benchmarking Demo ===\n" << std::endl;

    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return;
    }
    auto engine = std::move(*engineResult);

    PerformanceBenchmark benchmark(engine.get());

    // Configure benchmark parameters
    PerformanceBenchmark::BenchmarkConfig config;
    config.testDurations = {1, 5, 10};     // seconds
    config.chunkSizes = {256, 512, 1024};  // samples
    config.sampleRates = {44100.0f};       // Hz
    config.numRuns = 3;
    config.enableMemoryProfiling = true;
    config.enableLatencyProfiling = true;
    config.enableQualityValidation = true;

    std::cout << "Running comprehensive benchmark suite..." << std::endl;
    std::cout << "This may take a few minutes..." << std::endl;

    auto results = benchmark.runComprehensiveBenchmark(config);

    std::cout << "\n--- Benchmark Results ---" << std::endl;
    for (const auto& result : results) {
        std::cout << "\nTest: " << result.testName << std::endl;
        std::cout << "  Performance Category: " << result.performanceCategory << std::endl;
        std::cout << "  Real-time Ratio: " << std::fixed << std::setprecision(3)
                  << result.realTimeRatio << std::endl;
        std::cout << "  Avg Processing Time: " << result.avgProcessingTime << " ms" << std::endl;
        std::cout << "  Peak Memory Usage: " << (result.peakMemoryUsage / (1024 * 1024)) << " MB"
                  << std::endl;
        std::cout << "  Passed Real-time Threshold: "
                  << (result.passedRealTimeThreshold ? "YES" : "NO") << std::endl;

        if (result.avgSimilarityScore > 0.0f) {
            std::cout << "  Avg Similarity Score: " << result.avgSimilarityScore << std::endl;
        }
    }

    // Generate comprehensive report
    auto report = benchmark.generateBenchmarkReport(results);
    std::cout << "\n" << report << std::endl;

    // Export results
    benchmark.exportBenchmarkResults(results, "comprehensive_benchmark_results.json");
    std::cout << "Benchmark results exported to: comprehensive_benchmark_results.json" << std::endl;
}

/**
 * @brief Demonstrate performance monitoring during real-world scenario
 */
void demonstrateRealWorldMonitoring() {
    std::cout << "\n=== Real-World Performance Monitoring Demo ===\n" << std::endl;

    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return;
    }
    auto engine = std::move(*engineResult);

    PerformanceProfiler profiler;

    // Simulate multiple concurrent sessions
    std::vector<SessionId> sessions;
    const int numSessions = 3;

    std::cout << "Creating " << numSessions << " concurrent sessions..." << std::endl;

    for (int i = 0; i < numSessions; ++i) {
        auto sessionResult = engine->createSession(44100.0f);
        if (sessionResult.isOk()) {
            SessionId sessionId = *sessionResult;
            sessions.push_back(sessionId);
            profiler.startProfiling(sessionId);
            std::cout << "  Session " << sessionId << " created" << std::endl;
        }
    }

    // Simulate concurrent processing
    std::cout << "\nSimulating concurrent real-world processing..." << std::endl;

    for (int iteration = 0; iteration < 5; ++iteration) {
        std::cout << "Iteration " << (iteration + 1) << "/5..." << std::endl;

        for (SessionId sessionId : sessions) {
            // Generate different audio complexity for each session
            std::string complexity = (sessionId % 3 == 0)   ? "simple"
                                     : (sessionId % 3 == 1) ? "medium"
                                                            : "complex";
            auto audio = generateComplexTestAudio(2, 44100.0f, complexity);

            // Process in chunks with profiling
            const size_t chunkSize = 512;
            for (size_t i = 0; i < audio.size(); i += chunkSize) {
                size_t remaining = audio.size() - i;
                size_t toProcess = std::min(chunkSize, remaining);

                auto startTime = std::chrono::high_resolution_clock::now();

                std::span<const float> chunk(audio.data() + i, toProcess);
                auto status = engine->processAudioChunk(sessionId, chunk);

                auto endTime = std::chrono::high_resolution_clock::now();
                auto processingTime =
                    std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

                if (status == UnifiedAudioEngine::Status::OK) {
                    profiler.recordChunkProcessed(
                        sessionId, toProcess, processingTime.count() / 1000.0f);
                }

                // Simulate varying memory usage
                size_t memUsage = 60 * 1024 * 1024 + (sessionId * 10 * 1024 * 1024)
                                  + (iteration * 5 * 1024 * 1024);
                profiler.recordMemoryUsage(sessionId, memUsage);
            }
        }

        // Brief pause between iterations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop profiling and analyze results
    std::cout << "\n--- Session Performance Summary ---" << std::endl;

    for (SessionId sessionId : sessions) {
        profiler.stopProfiling(sessionId);

        auto metrics = profiler.getSessionMetrics(sessionId);
        std::cout << "\nSession " << sessionId << ":" << std::endl;
        std::cout << "  Chunks Processed: " << metrics.totalChunksProcessed << std::endl;
        std::cout << "  Real-time Ratio: " << std::fixed << std::setprecision(3)
                  << metrics.realTimeRatio << std::endl;
        std::cout << "  Avg Processing Time: " << metrics.avgChunkProcessingTime << " ms"
                  << std::endl;
        std::cout << "  Peak Memory: " << (metrics.peakMemoryUsage / (1024 * 1024)) << " MB"
                  << std::endl;

        // Check for alerts
        auto alerts = profiler.checkPerformanceAlerts(sessionId);
        if (!alerts.empty()) {
            std::cout << "  Alerts: " << alerts.size() << " performance issues detected"
                      << std::endl;
        }

        // Export individual session data
        std::string filename = "session_" + std::to_string(sessionId) + "_performance.json";
        profiler.exportToJson(sessionId, filename);

        engine->destroySession(sessionId);
    }
}

int main() {
    std::cout << "Huntmaster Performance Profiling and Bottleneck Resolution Demo" << std::endl;
    std::cout << "===============================================================" << std::endl;

    try {
        // Run demonstration scenarios
        demonstrateBasicProfiling();
        demonstrateBottleneckAnalysis();
        demonstrateComprehensiveBenchmarking();
        demonstrateRealWorldMonitoring();

        std::cout << "\n=== Demo Complete ===\n" << std::endl;
        std::cout << "Performance profiling tools demonstrated successfully!" << std::endl;
        std::cout << "Check the generated JSON files for detailed performance data." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
