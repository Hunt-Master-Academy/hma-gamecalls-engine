/**
 * @file PerformanceProfiler.h
 * @brief Advanced performance profiling and bottleneck detection for UnifiedAudioEngine
 *
 * This tool provides comprehensive performance analysis capabilities for identifying
 * and resolving bottlenecks in the Huntmaster audio processing pipeline.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

namespace huntmaster {
namespace profiling {

/**
 * @struct ComponentMetrics
 * @brief Performance metrics for individual audio processing components
 */
struct ComponentMetrics {
    std::string componentName;
    std::chrono::microseconds totalTime{0};
    std::chrono::microseconds minTime{std::chrono::microseconds::max()};
    std::chrono::microseconds maxTime{0};
    std::chrono::microseconds avgTime{0};
    uint64_t callCount{0};
    size_t memoryUsage{0};
    float cpuUsage{0.0f};
};

/**
 * @struct SessionPerformanceData
 * @brief Comprehensive performance data for a single session
 */
struct SessionPerformanceData {
    SessionId sessionId;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    // Component-specific metrics
    ComponentMetrics mfccMetrics{"MFCC_Processing"};
    ComponentMetrics dtwMetrics{"DTW_Comparison"};
    ComponentMetrics vadMetrics{"VAD_Processing"};
    ComponentMetrics scoringMetrics{"Realtime_Scoring"};

    // Overall session metrics
    uint64_t totalChunksProcessed{0};
    uint64_t totalSamplesProcessed{0};
    float avgChunkProcessingTime{0.0f};
    float realTimeRatio{0.0f};  // < 1.0 is good (processing faster than real-time)
    size_t peakMemoryUsage{0};
    size_t avgMemoryUsage{0};

    // Quality metrics
    float avgSimilarityScore{0.0f};
    int featureExtractionCount{0};
    float vadActivityRatio{0.0f};  // Percentage of audio with detected activity
};

/**
 * @struct PerformanceThresholds
 * @brief Configurable performance thresholds for alerts and optimization
 */
struct PerformanceThresholds {
    float maxRealTimeRatio{0.5f};              // Maximum acceptable real-time ratio
    float maxChunkLatency{10.0f};              // Maximum chunk processing time (ms)
    size_t maxMemoryUsage{100 * 1024 * 1024};  // Maximum memory usage (bytes)
    float minSimilarityScore{0.01f};           // Minimum expected similarity score
    float maxCpuUsage{25.0f};                  // Maximum CPU usage percentage
};

/**
 * @enum PerformanceIssueType
 * @brief Types of performance issues that can be detected
 */
enum class PerformanceIssueType {
    HIGH_LATENCY,
    MEMORY_LEAK,
    CPU_OVERLOAD,
    POOR_QUALITY,
    BOTTLENECK_DETECTED,
    REGRESSION_DETECTED
};

/**
 * @struct PerformanceAlert
 * @brief Alert generated when performance issues are detected
 */
struct PerformanceAlert {
    PerformanceIssueType type;
    SessionId sessionId;
    std::string componentName;
    std::string description;
    float severity{0.0f};  // 0.0 = info, 1.0 = critical
    std::chrono::steady_clock::time_point timestamp;

    // Suggested optimizations
    std::vector<std::string> suggestions;
};

/**
 * @struct BottleneckAnalysis
 * @brief Detailed analysis of performance bottlenecks
 */
struct BottleneckAnalysis {
    std::string bottleneckComponent;
    float impactPercentage{0.0f};  // Percentage of total processing time
    std::string rootCause;
    std::vector<std::string> optimizationStrategies;
    float expectedImprovement{0.0f};  // Expected performance improvement percentage
};

/**
 * @class PerformanceProfiler
 * @brief Main performance profiling and analysis tool
 */
class PerformanceProfiler {
  public:
    /**
     * @brief Constructor with configurable thresholds
     * @param thresholds Performance thresholds for alerts
     */
    explicit PerformanceProfiler(const PerformanceThresholds& thresholds = {});

    ~PerformanceProfiler() = default;

    // Core profiling control
    void startProfiling(SessionId sessionId);
    void stopProfiling(SessionId sessionId);
    bool isProfilingActive(SessionId sessionId) const;

    // Performance measurement
    void recordComponentStart(SessionId sessionId, const std::string& component);
    void recordComponentEnd(SessionId sessionId, const std::string& component);
    void recordMemoryUsage(SessionId sessionId, size_t memoryBytes);
    void recordChunkProcessed(SessionId sessionId, size_t sampleCount, float processingTimeMs);
    void recordSimilarityScore(SessionId sessionId, float score);
    void recordFeatureExtraction(SessionId sessionId, int featureCount);
    void recordVADActivity(SessionId sessionId, bool isActive);

    // Analysis and reporting
    SessionPerformanceData getSessionMetrics(SessionId sessionId) const;
    std::vector<PerformanceAlert> checkPerformanceAlerts(SessionId sessionId);
    BottleneckAnalysis analyzeBottlenecks(SessionId sessionId);
    std::string generatePerformanceReport(SessionId sessionId) const;

    // Optimization suggestions
    struct OptimizationSuggestion {
        std::string component;
        std::string parameter;
        std::string currentValue;
        std::string suggestedValue;
        std::string rationale;
        float expectedImprovement;
    };

    std::vector<OptimizationSuggestion> suggestOptimizations(SessionId sessionId);
    void applyAutomaticOptimizations(SessionId sessionId, UnifiedAudioEngine* engine);

    // Configuration
    void setPerformanceThresholds(const PerformanceThresholds& thresholds);
    void enableRealTimeMonitoring(bool enable);
    void setCallbackOnAlert(std::function<void(const PerformanceAlert&)> callback);

    // Export functionality
    void exportToJson(SessionId sessionId, const std::string& filename) const;
    void exportToCsv(SessionId sessionId, const std::string& filename) const;
    void exportDetailedProfile(SessionId sessionId, const std::string& filename) const;

    // Historical analysis
    struct PerformanceTrend {
        std::vector<float> realTimeRatios;
        std::vector<float> avgLatencies;
        std::vector<size_t> memoryUsages;
        std::vector<float> qualityScores;
        std::chrono::steady_clock::time_point trendStart;
        std::chrono::steady_clock::time_point trendEnd;
    };

    PerformanceTrend analyzePerformanceTrend(SessionId sessionId,
                                             std::chrono::minutes duration) const;
    bool detectPerformanceRegression(SessionId sessionId, float threshold = 0.1f) const;

    // System-wide analysis
    struct SystemPerformanceOverview {
        uint32_t totalActiveSessions;
        float avgSystemLoad;
        size_t totalMemoryUsage;
        std::vector<SessionId> highLatencySessions;
        std::vector<SessionId> memoryIntensiveSessions;
        std::string overallHealthStatus;
    };

    SystemPerformanceOverview getSystemOverview() const;

  private:
    // Internal data structures
    mutable std::mutex metricsMutex_;
    std::unordered_map<SessionId, std::unique_ptr<SessionPerformanceData>> sessionMetrics_;
    std::unordered_map<SessionId,
                       std::unordered_map<std::string, std::chrono::steady_clock::time_point>>
        componentStartTimes_;

    PerformanceThresholds thresholds_;
    std::atomic<bool> realTimeMonitoringEnabled_{false};
    std::function<void(const PerformanceAlert&)> alertCallback_;

    // Historical data for trend analysis
    std::unordered_map<SessionId, std::vector<SessionPerformanceData>> historicalData_;

    // Internal helper methods
    void updateComponentMetrics(ComponentMetrics& metrics, std::chrono::microseconds duration);
    void checkAndGenerateAlerts(SessionId sessionId);
    std::string formatDuration(std::chrono::microseconds duration) const;
    size_t getCurrentMemoryUsage() const;
    float getCurrentCpuUsage() const;

    // Bottleneck detection algorithms
    std::string identifyPrimaryBottleneck(const SessionPerformanceData& data) const;
    float calculateBottleneckImpact(const ComponentMetrics& metrics,
                                    const SessionPerformanceData& data) const;
    std::vector<std::string> generateOptimizationStrategies(const std::string& bottleneck) const;
};

/**
 * @class AutoProfiler
 * @brief RAII-style automatic profiling for specific components
 */
class AutoProfiler {
  public:
    AutoProfiler(PerformanceProfiler& profiler, SessionId sessionId, const std::string& component);
    ~AutoProfiler();

    // Non-copyable, non-moveable
    AutoProfiler(const AutoProfiler&) = delete;
    AutoProfiler& operator=(const AutoProfiler&) = delete;
    AutoProfiler(AutoProfiler&&) = delete;
    AutoProfiler& operator=(AutoProfiler&&) = delete;

  private:
    PerformanceProfiler& profiler_;
    SessionId sessionId_;
    std::string component_;
};

/**
 * @def PROFILE_COMPONENT
 * @brief Macro for easy component profiling
 *
 * Usage: PROFILE_COMPONENT(profiler, sessionId, "ComponentName");
 */
#define PROFILE_COMPONENT(profiler, sessionId, componentName) \
    AutoProfiler _auto_profiler(profiler, sessionId, componentName)

/**
 * @class PerformanceBenchmark
 * @brief Comprehensive benchmarking tools for UnifiedAudioEngine
 */
class PerformanceBenchmark {
  public:
    struct BenchmarkConfig {
        std::vector<int> testDurations{1, 5, 10, 30};       // seconds
        std::vector<int> chunkSizes{256, 512, 1024, 2048};  // samples
        std::vector<float> sampleRates{22050.0f, 44100.0f, 48000.0f};
        int numRuns{5};
        bool enableMemoryProfiling{true};
        bool enableLatencyProfiling{true};
        bool enableQualityValidation{true};
    };

    struct BenchmarkResult {
        std::string testName;
        float avgProcessingTime{0.0f};
        float maxProcessingTime{0.0f};
        float realTimeRatio{0.0f};
        size_t peakMemoryUsage{0};
        float avgSimilarityScore{0.0f};
        bool passedRealTimeThreshold{false};
        std::string performanceCategory;  // "Excellent", "Good", "Acceptable", "Poor"
    };

    explicit PerformanceBenchmark(UnifiedAudioEngine* engine);

    // Benchmark execution
    std::vector<BenchmarkResult> runComprehensiveBenchmark(const BenchmarkConfig& config = {});
    BenchmarkResult
    benchmarkRealTimeProcessing(int durationSeconds, int chunkSize, float sampleRate);
    BenchmarkResult benchmarkMemoryUsage(int durationSeconds);
    BenchmarkResult benchmarkChunkLatency(int chunkSize, int numIterations = 1000);
    BenchmarkResult benchmarkScalability(int numConcurrentSessions);

    // Specialized benchmarks
    BenchmarkResult benchmarkMFCCPerformance();
    BenchmarkResult benchmarkDTWPerformance();
    BenchmarkResult benchmarkVADPerformance();

    // Comparison and validation
    bool compareWithBaseline(const std::vector<BenchmarkResult>& results,
                             const std::string& baselineFile);
    void saveBaseline(const std::vector<BenchmarkResult>& results, const std::string& filename);

    // Report generation
    std::string generateBenchmarkReport(const std::vector<BenchmarkResult>& results) const;
    void exportBenchmarkResults(const std::vector<BenchmarkResult>& results,
                                const std::string& filename) const;

  private:
    UnifiedAudioEngine* engine_;
    std::vector<float> generateTestAudio(int durationSeconds, float sampleRate);
    std::string categorizePerfomance(const BenchmarkResult& result) const;
};

}  // namespace profiling
}  // namespace huntmaster
