/**
 * @file PerformanceProfiler.cpp
 * @brief Implementation of advanced performance profiling and bottleneck detection
 */

#include "huntmaster/profiling/PerformanceProfiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "huntmaster/core/ComponentErrorHandler.h"
#include "huntmaster/core/DebugLogger.h"

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace huntmaster {
namespace profiling {

// ===== PerformanceProfiler Implementation =====

PerformanceProfiler::PerformanceProfiler(const PerformanceThresholds& thresholds)
    : thresholds_(thresholds) {}

void PerformanceProfiler::startProfiling(SessionId sessionId) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    if (sessionMetrics_.find(sessionId) == sessionMetrics_.end()) {
        auto metrics = std::make_unique<SessionPerformanceData>();
        metrics->sessionId = sessionId;
        metrics->startTime = std::chrono::steady_clock::now();
        sessionMetrics_[sessionId] = std::move(metrics);
    }
}

void PerformanceProfiler::stopProfiling(SessionId sessionId) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        it->second->endTime = std::chrono::steady_clock::now();

        // Store in historical data
        if (historicalData_.find(sessionId) == historicalData_.end()) {
            historicalData_[sessionId] = std::vector<SessionPerformanceData>();
        }
        historicalData_[sessionId].push_back(*it->second);

        // Keep only last 100 sessions for memory management
        if (historicalData_[sessionId].size() > 100) {
            historicalData_[sessionId].erase(historicalData_[sessionId].begin());
        }
    }
}

bool PerformanceProfiler::isProfilingActive(SessionId sessionId) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return sessionMetrics_.find(sessionId) != sessionMetrics_.end();
}

void PerformanceProfiler::recordComponentStart(SessionId sessionId, const std::string& component) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    componentStartTimes_[sessionId][component] = std::chrono::steady_clock::now();
}

void PerformanceProfiler::recordComponentEnd(SessionId sessionId, const std::string& component) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto sessionIt = componentStartTimes_.find(sessionId);
    if (sessionIt == componentStartTimes_.end()
        || sessionIt->second.find(component) == sessionIt->second.end()) {
        return;
    }

    auto startTime = sessionIt->second[component];
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    auto metricsIt = sessionMetrics_.find(sessionId);
    if (metricsIt != sessionMetrics_.end()) {
        ComponentMetrics* metrics = nullptr;

        if (component == "MFCC_Processing") {
            metrics = &metricsIt->second->mfccMetrics;
        } else if (component == "DTW_Comparison") {
            metrics = &metricsIt->second->dtwMetrics;
        } else if (component == "VAD_Processing") {
            metrics = &metricsIt->second->vadMetrics;
        } else if (component == "Realtime_Scoring") {
            metrics = &metricsIt->second->scoringMetrics;
        }

        if (metrics) {
            updateComponentMetrics(*metrics, duration);
        }
    }

    // Remove the start time entry
    sessionIt->second.erase(component);

    // Check for performance alerts if real-time monitoring is enabled
    if (realTimeMonitoringEnabled_) {
        checkAndGenerateAlerts(sessionId);
    }
}

void PerformanceProfiler::recordMemoryUsage(SessionId sessionId, size_t memoryBytes) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        auto& metrics = it->second;
        metrics->peakMemoryUsage = std::max(metrics->peakMemoryUsage, memoryBytes);

        // Update running average (simple exponential moving average)
        if (metrics->avgMemoryUsage == 0) {
            metrics->avgMemoryUsage = memoryBytes;
        } else {
            metrics->avgMemoryUsage = 0.9f * metrics->avgMemoryUsage + 0.1f * memoryBytes;
        }
    }
}

void PerformanceProfiler::recordChunkProcessed(SessionId sessionId,
                                               size_t sampleCount,
                                               float processingTimeMs) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        auto& metrics = it->second;
        metrics->totalChunksProcessed++;
        metrics->totalSamplesProcessed += sampleCount;

        // Update average processing time
        if (metrics->avgChunkProcessingTime == 0.0f) {
            metrics->avgChunkProcessingTime = processingTimeMs;
        } else {
            float alpha = 0.1f;  // Smoothing factor
            metrics->avgChunkProcessingTime =
                alpha * processingTimeMs + (1.0f - alpha) * metrics->avgChunkProcessingTime;
        }

        // Calculate real-time ratio (assuming 44.1kHz sample rate)
        float audioTimeMs = (sampleCount / 44100.0f) * 1000.0f;
        float currentRatio = processingTimeMs / audioTimeMs;

        if (metrics->realTimeRatio == 0.0f) {
            metrics->realTimeRatio = currentRatio;
        } else {
            metrics->realTimeRatio = 0.9f * metrics->realTimeRatio + 0.1f * currentRatio;
        }
    }
}

void PerformanceProfiler::recordSimilarityScore(SessionId sessionId, float score) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        auto& metrics = it->second;

        if (metrics->avgSimilarityScore == 0.0f) {
            metrics->avgSimilarityScore = score;
        } else {
            metrics->avgSimilarityScore = 0.95f * metrics->avgSimilarityScore + 0.05f * score;
        }
    }
}

void PerformanceProfiler::recordFeatureExtraction(SessionId sessionId, int featureCount) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        it->second->featureExtractionCount += featureCount;
    }
}

void PerformanceProfiler::recordVADActivity(SessionId sessionId, bool isActive) {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        auto& metrics = it->second;
        static thread_local int vadSampleCount = 0;
        static thread_local int vadActiveCount = 0;

        vadSampleCount++;
        if (isActive)
            vadActiveCount++;

        metrics->vadActivityRatio = static_cast<float>(vadActiveCount) / vadSampleCount;
    }
}

SessionPerformanceData PerformanceProfiler::getSessionMetrics(SessionId sessionId) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    auto it = sessionMetrics_.find(sessionId);
    if (it != sessionMetrics_.end()) {
        return *it->second;
    }

    return SessionPerformanceData{};  // Return empty metrics if not found
}

std::vector<PerformanceAlert> PerformanceProfiler::checkPerformanceAlerts(SessionId sessionId) {
    std::vector<PerformanceAlert> alerts;

    auto metrics = getSessionMetrics(sessionId);
    auto now = std::chrono::steady_clock::now();

    // Check real-time ratio
    if (metrics.realTimeRatio > thresholds_.maxRealTimeRatio) {
        PerformanceAlert alert;
        alert.type = PerformanceIssueType::HIGH_LATENCY;
        alert.sessionId = sessionId;
        alert.componentName = "Overall";
        alert.description = "Real-time processing ratio exceeded threshold: "
                            + std::to_string(metrics.realTimeRatio);
        alert.severity = std::min(1.0f, metrics.realTimeRatio / thresholds_.maxRealTimeRatio);
        alert.timestamp = now;
        alert.suggestions = {"Consider reducing MFCC frame size",
                             "Optimize DTW window ratio",
                             "Enable SIMD optimizations"};
        alerts.push_back(alert);
    }

    // Check memory usage
    if (metrics.peakMemoryUsage > thresholds_.maxMemoryUsage) {
        PerformanceAlert alert;
        alert.type = PerformanceIssueType::MEMORY_LEAK;
        alert.sessionId = sessionId;
        alert.componentName = "Memory";
        alert.description = "Memory usage exceeded threshold: "
                            + std::to_string(metrics.peakMemoryUsage / (1024 * 1024)) + " MB";
        alert.severity = static_cast<float>(metrics.peakMemoryUsage) / thresholds_.maxMemoryUsage;
        alert.timestamp = now;
        alert.suggestions = {"Check for memory leaks in buffer management",
                             "Implement buffer pooling",
                             "Reduce feature vector caching"};
        alerts.push_back(alert);
    }

    // Check chunk processing latency
    if (metrics.avgChunkProcessingTime > thresholds_.maxChunkLatency) {
        PerformanceAlert alert;
        alert.type = PerformanceIssueType::HIGH_LATENCY;
        alert.sessionId = sessionId;
        alert.componentName = "ChunkProcessing";
        alert.description = "Average chunk processing time exceeded threshold: "
                            + std::to_string(metrics.avgChunkProcessingTime) + " ms";
        alert.severity = metrics.avgChunkProcessingTime / thresholds_.maxChunkLatency;
        alert.timestamp = now;
        alerts.push_back(alert);
    }

    return alerts;
}

BottleneckAnalysis PerformanceProfiler::analyzeBottlenecks(SessionId sessionId) {
    auto metrics = getSessionMetrics(sessionId);

    BottleneckAnalysis analysis;
    analysis.bottleneckComponent = identifyPrimaryBottleneck(metrics);

    // Calculate impact percentage
    ComponentMetrics* primaryMetrics = nullptr;
    if (analysis.bottleneckComponent == "MFCC_Processing") {
        primaryMetrics = &const_cast<ComponentMetrics&>(metrics.mfccMetrics);
    } else if (analysis.bottleneckComponent == "DTW_Comparison") {
        primaryMetrics = &const_cast<ComponentMetrics&>(metrics.dtwMetrics);
    } else if (analysis.bottleneckComponent == "VAD_Processing") {
        primaryMetrics = &const_cast<ComponentMetrics&>(metrics.vadMetrics);
    } else if (analysis.bottleneckComponent == "Realtime_Scoring") {
        primaryMetrics = &const_cast<ComponentMetrics&>(metrics.scoringMetrics);
    }

    if (primaryMetrics) {
        analysis.impactPercentage = calculateBottleneckImpact(*primaryMetrics, metrics);
        analysis.optimizationStrategies =
            generateOptimizationStrategies(analysis.bottleneckComponent);
        analysis.expectedImprovement =
            std::min(50.0f, analysis.impactPercentage * 0.7f);  // Conservative estimate

        // Determine root cause based on component
        if (analysis.bottleneckComponent == "MFCC_Processing") {
            analysis.rootCause = "High computational cost of FFT and filter bank operations";
        } else if (analysis.bottleneckComponent == "DTW_Comparison") {
            analysis.rootCause = "Dynamic programming matrix computation complexity";
        } else if (analysis.bottleneckComponent == "VAD_Processing") {
            analysis.rootCause = "Per-frame energy and spectral feature calculations";
        } else {
            analysis.rootCause = "Multiple similarity metric computations";
        }
    }

    return analysis;
}

std::string PerformanceProfiler::generatePerformanceReport(SessionId sessionId) const {
    auto metrics = getSessionMetrics(sessionId);

    std::ostringstream report;
    report << std::fixed << std::setprecision(3);

    report << "=== Performance Report for Session " << sessionId << " ===\n\n";

    // Session overview
    auto sessionDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(metrics.endTime - metrics.startTime);

    report << "Session Duration: " << sessionDuration.count() << " ms\n";
    report << "Total Chunks Processed: " << metrics.totalChunksProcessed << "\n";
    report << "Total Samples Processed: " << metrics.totalSamplesProcessed << "\n";
    report << "Average Processing Time: " << metrics.avgChunkProcessingTime << " ms/chunk\n";
    report << "Real-time Ratio: " << metrics.realTimeRatio << " (< 1.0 is good)\n";
    report << "Peak Memory Usage: " << (metrics.peakMemoryUsage / (1024 * 1024)) << " MB\n";
    report << "Average Similarity Score: " << metrics.avgSimilarityScore << "\n\n";

    // Component-wise breakdown
    report << "=== Component Performance Breakdown ===\n\n";

    auto printComponentMetrics = [&](const ComponentMetrics& comp) {
        report << comp.componentName << ":\n";
        report << "  Total Time: " << formatDuration(comp.totalTime) << "\n";
        report << "  Average Time: " << formatDuration(comp.avgTime) << "\n";
        report << "  Min/Max Time: " << formatDuration(comp.minTime) << " / "
               << formatDuration(comp.maxTime) << "\n";
        report << "  Call Count: " << comp.callCount << "\n\n";
    };

    printComponentMetrics(metrics.mfccMetrics);
    printComponentMetrics(metrics.dtwMetrics);
    printComponentMetrics(metrics.vadMetrics);
    printComponentMetrics(metrics.scoringMetrics);

    // Performance status
    report << "=== Performance Assessment ===\n\n";

    if (metrics.realTimeRatio < 0.3f) {
        report << "Status: EXCELLENT - Processing significantly faster than real-time\n";
    } else if (metrics.realTimeRatio < 0.5f) {
        report << "Status: GOOD - Suitable for real-time applications\n";
    } else if (metrics.realTimeRatio < 1.0f) {
        report << "Status: ACCEPTABLE - May struggle with sustained real-time processing\n";
    } else {
        report << "Status: POOR - Cannot keep up with real-time audio\n";
    }

    return report.str();
}

std::vector<PerformanceProfiler::OptimizationSuggestion>
PerformanceProfiler::suggestOptimizations(SessionId sessionId) {
    std::vector<OptimizationSuggestion> suggestions;
    auto metrics = getSessionMetrics(sessionId);

    // MFCC optimization suggestions
    if (metrics.mfccMetrics.avgTime > std::chrono::microseconds(1000)) {  // > 1ms
        OptimizationSuggestion suggestion;
        suggestion.component = "MFCC_Processing";
        suggestion.parameter = "frame_size";
        suggestion.currentValue = "512";
        suggestion.suggestedValue = "256";
        suggestion.rationale = "Reducing frame size can significantly speed up FFT computation";
        suggestion.expectedImprovement = 25.0f;
        suggestions.push_back(suggestion);

        suggestion.parameter = "num_coefficients";
        suggestion.currentValue = "13";
        suggestion.suggestedValue = "10";
        suggestion.rationale = "Fewer coefficients reduce DCT computation cost";
        suggestion.expectedImprovement = 15.0f;
        suggestions.push_back(suggestion);
    }

    // DTW optimization suggestions
    if (metrics.dtwMetrics.avgTime > std::chrono::microseconds(2000)) {  // > 2ms
        OptimizationSuggestion suggestion;
        suggestion.component = "DTW_Comparison";
        suggestion.parameter = "window_ratio";
        suggestion.currentValue = "0.1";
        suggestion.suggestedValue = "0.05";
        suggestion.rationale = "Smaller window reduces dynamic programming matrix size";
        suggestion.expectedImprovement = 30.0f;
        suggestions.push_back(suggestion);
    }

    // Memory optimization suggestions
    if (metrics.peakMemoryUsage > 50 * 1024 * 1024) {  // > 50MB
        OptimizationSuggestion suggestion;
        suggestion.component = "Memory_Management";
        suggestion.parameter = "buffer_pooling";
        suggestion.currentValue = "disabled";
        suggestion.suggestedValue = "enabled";
        suggestion.rationale = "Buffer pooling reduces memory allocation overhead";
        suggestion.expectedImprovement = 10.0f;
        suggestions.push_back(suggestion);
    }

    return suggestions;
}

void PerformanceProfiler::applyAutomaticOptimizations(SessionId sessionId,
                                                      UnifiedAudioEngine* engine) {
    auto suggestions = suggestOptimizations(sessionId);

    for (const auto& suggestion : suggestions) {
        if (suggestion.component == "DTW_Comparison" && suggestion.parameter == "window_ratio") {
            float newRatio = std::stof(suggestion.suggestedValue);
            if (engine->configureDTW(sessionId, newRatio, true) != UnifiedAudioEngine::Status::OK) {
                // Handle error appropriately
            }
        }
        // Additional optimizations can be applied here as the engine API expands
    }
}

void PerformanceProfiler::setPerformanceThresholds(const PerformanceThresholds& thresholds) {
    thresholds_ = thresholds;
}

void PerformanceProfiler::enableRealTimeMonitoring(bool enable) {
    realTimeMonitoringEnabled_ = enable;
}

void PerformanceProfiler::setCallbackOnAlert(
    std::function<void(const PerformanceAlert&)> callback) {
    alertCallback_ = callback;
}

void PerformanceProfiler::exportToJson(SessionId sessionId, const std::string& filename) const {
    auto metrics = getSessionMetrics(sessionId);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << "{\n";
        file << "  \"sessionId\": " << sessionId << ",\n";
        file << "  \"realTimeRatio\": " << metrics.realTimeRatio << ",\n";
        file << "  \"avgChunkProcessingTime\": " << metrics.avgChunkProcessingTime << ",\n";
        file << "  \"peakMemoryUsage\": " << metrics.peakMemoryUsage << ",\n";
        file << "  \"totalChunksProcessed\": " << metrics.totalChunksProcessed << ",\n";
        file << "  \"avgSimilarityScore\": " << metrics.avgSimilarityScore << ",\n";
        file << "  \"components\": {\n";
        file << "    \"mfcc\": {\n";
        file << "      \"totalTime\": " << metrics.mfccMetrics.totalTime.count() << ",\n";
        file << "      \"avgTime\": " << metrics.mfccMetrics.avgTime.count() << ",\n";
        file << "      \"callCount\": " << metrics.mfccMetrics.callCount << "\n";
        file << "    },\n";
        file << "    \"dtw\": {\n";
        file << "      \"totalTime\": " << metrics.dtwMetrics.totalTime.count() << ",\n";
        file << "      \"avgTime\": " << metrics.dtwMetrics.avgTime.count() << ",\n";
        file << "      \"callCount\": " << metrics.dtwMetrics.callCount << "\n";
        file << "    }\n";
        file << "  }\n";
        file << "}\n";
    }
}

// ===== Internal Helper Methods =====

void PerformanceProfiler::updateComponentMetrics(ComponentMetrics& metrics,
                                                 std::chrono::microseconds duration) {
    metrics.totalTime += duration;
    metrics.callCount++;
    metrics.minTime = std::min(metrics.minTime, duration);
    metrics.maxTime = std::max(metrics.maxTime, duration);
    metrics.avgTime = metrics.totalTime / metrics.callCount;
}

void PerformanceProfiler::checkAndGenerateAlerts(SessionId sessionId) {
    auto alerts = checkPerformanceAlerts(sessionId);

    for (const auto& alert : alerts) {
        if (alertCallback_) {
            alertCallback_(alert);
        }
    }
}

std::string PerformanceProfiler::formatDuration(std::chrono::microseconds duration) const {
    if (duration.count() < 1000) {
        return std::to_string(duration.count()) + " μs";
    } else if (duration.count() < 1000000) {
        return std::to_string(duration.count() / 1000.0f) + " ms";
    } else {
        return std::to_string(duration.count() / 1000000.0f) + " s";
    }
}

size_t PerformanceProfiler::getCurrentMemoryUsage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#elif defined(__linux__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;  // Convert KB to bytes
    }
#endif
    return 0;
}

float PerformanceProfiler::getCurrentCpuUsage() const {
    // Simplified CPU usage - would need platform-specific implementation
    return 0.0f;
}

std::string
PerformanceProfiler::identifyPrimaryBottleneck(const SessionPerformanceData& data) const {
    std::vector<std::pair<std::string, std::chrono::microseconds>> components = {
        {"MFCC_Processing", data.mfccMetrics.totalTime},
        {"DTW_Comparison", data.dtwMetrics.totalTime},
        {"VAD_Processing", data.vadMetrics.totalTime},
        {"Realtime_Scoring", data.scoringMetrics.totalTime}};

    auto maxElement =
        std::max_element(components.begin(), components.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    return maxElement != components.end() ? maxElement->first : "Unknown";
}

float PerformanceProfiler::calculateBottleneckImpact(const ComponentMetrics& metrics,
                                                     const SessionPerformanceData& data) const {
    auto totalTime = data.mfccMetrics.totalTime + data.dtwMetrics.totalTime
                     + data.vadMetrics.totalTime + data.scoringMetrics.totalTime;

    if (totalTime.count() == 0)
        return 0.0f;

    return (static_cast<float>(metrics.totalTime.count()) / totalTime.count()) * 100.0f;
}

std::vector<std::string>
PerformanceProfiler::generateOptimizationStrategies(const std::string& bottleneck) const {
    if (bottleneck == "MFCC_Processing") {
        return {"Reduce MFCC frame size from 512 to 256 samples",
                "Decrease number of coefficients from 13 to 10",
                "Enable SIMD optimizations for FFT computation",
                "Implement pre-computed window functions",
                "Use approximated filter bank responses"};
    } else if (bottleneck == "DTW_Comparison") {
        return {"Reduce DTW window ratio from 10% to 5%",
                "Implement early termination for poor matches",
                "Use approximate distance calculations",
                "Limit maximum sequence length for comparison",
                "Enable parallel DTW computation"};
    } else if (bottleneck == "VAD_Processing") {
        return {"Reduce VAD frame size",
                "Use energy-only detection (disable spectral features)",
                "Implement fast silence detection",
                "Batch process multiple VAD frames",
                "Use adaptive thresholding"};
    } else {
        return {"Enable component-specific optimizations",
                "Implement parallel processing",
                "Optimize memory access patterns",
                "Use hardware acceleration where available"};
    }
}

// ===== AutoProfiler Implementation =====

AutoProfiler::AutoProfiler(PerformanceProfiler& profiler,
                           SessionId sessionId,
                           const std::string& component)
    : profiler_(profiler), sessionId_(sessionId), component_(component) {
    profiler_.recordComponentStart(sessionId_, component_);
}

AutoProfiler::~AutoProfiler() {
    profiler_.recordComponentEnd(sessionId_, component_);
}

// ===== PerformanceBenchmark Implementation =====

PerformanceBenchmark::PerformanceBenchmark(UnifiedAudioEngine* engine) : engine_(engine) {}

std::vector<PerformanceBenchmark::BenchmarkResult>
PerformanceBenchmark::runComprehensiveBenchmark(const BenchmarkConfig& config) {
    std::vector<BenchmarkResult> results;

    // Real-time processing benchmarks
    for (int duration : config.testDurations) {
        for (int chunkSize : config.chunkSizes) {
            for (float sampleRate : config.sampleRates) {
                auto result = benchmarkRealTimeProcessing(duration, chunkSize, sampleRate);
                result.testName = "RealTime_" + std::to_string(duration) + "s_"
                                  + std::to_string(chunkSize) + "_"
                                  + std::to_string((int)sampleRate);
                results.push_back(result);
            }
        }
    }

    // Memory usage benchmark
    if (config.enableMemoryProfiling) {
        auto memResult = benchmarkMemoryUsage(30);  // 30 second test
        memResult.testName = "MemoryUsage_30s";
        results.push_back(memResult);
    }

    // Latency benchmarks
    if (config.enableLatencyProfiling) {
        for (int chunkSize : config.chunkSizes) {
            auto latResult = benchmarkChunkLatency(chunkSize, 1000);
            latResult.testName = "Latency_" + std::to_string(chunkSize);
            results.push_back(latResult);
        }
    }

    return results;
}

PerformanceBenchmark::BenchmarkResult PerformanceBenchmark::benchmarkRealTimeProcessing(
    int durationSeconds, int chunkSize, float sampleRate) {
    BenchmarkResult result;

    // Generate test audio
    auto testAudio = generateTestAudio(durationSeconds, sampleRate);

    // Create session
    auto sessionResult = engine_->createSession(sampleRate);
    if (!sessionResult.isOk()) {
        result.performanceCategory = "Error";
        return result;
    }

    SessionId sessionId = *sessionResult;

    // Time the processing
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t initialMemory = this->getCurrentMemoryUsage();

    size_t totalProcessed = 0;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t remaining = testAudio.size() - i;
        size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);

        std::span<const float> chunk(testAudio.data() + i, toProcess);
        auto status = engine_->processAudioChunk(sessionId, chunk);

        if (status != UnifiedAudioEngine::Status::OK) {
            break;
        }

        totalProcessed += toProcess;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    size_t finalMemory = getCurrentMemoryUsage();

    // Calculate metrics
    auto processingTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    float audioTimeMs = (testAudio.size() / sampleRate) * 1000.0f;

    result.avgProcessingTime = processingTime.count();
    result.maxProcessingTime = processingTime.count();  // Single run, so avg = max
    result.realTimeRatio = processingTime.count() / audioTimeMs;
    result.peakMemoryUsage = finalMemory - initialMemory;
    result.passedRealTimeThreshold = result.realTimeRatio < 0.5f;

    // Get final similarity score
    auto scoreResult = engine_->getSimilarityScore(sessionId);
    result.avgSimilarityScore = scoreResult.isOk() ? *scoreResult : 0.0f;

    result.performanceCategory = categorizePerfomance(result);

    // Cleanup
    if (engine_->destroySession(sessionId) != UnifiedAudioEngine::Status::OK) {
        huntmaster::ComponentErrorHandler::UnifiedEngineErrors::logSessionError(
            std::to_string(sessionId), "Failed to destroy session during benchmark cleanup.");
    }

    return result;
}

std::vector<float> PerformanceBenchmark::generateTestAudio(int durationSeconds, float sampleRate) {
    int totalSamples = static_cast<int>(durationSeconds * sampleRate);
    std::vector<float> audio(totalSamples);

    // Generate a mix of frequencies to simulate real audio
    for (int i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        audio[i] = 0.3f * std::sin(2.0f * 3.14159f * 220.0f * t) +  // 220 Hz
                   0.2f * std::sin(2.0f * 3.14159f * 440.0f * t) +  // 440 Hz
                   0.1f * std::sin(2.0f * 3.14159f * 880.0f * t);   // 880 Hz
    }

    return audio;
}

std::string PerformanceBenchmark::categorizePerfomance(const BenchmarkResult& result) const {
    if (result.realTimeRatio < 0.2f) {
        return "Excellent";
    } else if (result.realTimeRatio < 0.5f) {
        return "Good";
    } else if (result.realTimeRatio < 1.0f) {
        return "Acceptable";
    } else {
        return "Poor";
    }
}

size_t PerformanceBenchmark::getCurrentMemoryUsage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0;
}

}  // namespace profiling
}  // namespace huntmaster
