#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace huntmaster {

/**
 * High-precision performance measurement and bottleneck detection system
 */
class PerformanceProfiler {
  public:
    struct TimingData {
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
        std::chrono::nanoseconds duration;
        size_t call_count;
        std::chrono::nanoseconds total_time;
        std::chrono::nanoseconds min_time;
        std::chrono::nanoseconds max_time;
        std::thread::id thread_id;

        TimingData()
            : duration(0), call_count(0), total_time(0), min_time(std::chrono::nanoseconds::max()),
              max_time(0) {}
    };

    struct MemorySnapshot {
        size_t total_allocated;
        size_t peak_usage;
        size_t current_usage;
        std::chrono::high_resolution_clock::time_point timestamp;

        MemorySnapshot() : total_allocated(0), peak_usage(0), current_usage(0) {}
    };

    struct BottleneckAnalysis {
        std::string operation_name;
        std::chrono::nanoseconds average_duration;
        double cpu_utilization;
        size_t memory_delta;
        double bottleneck_score;  // 0-100, higher = more problematic
        std::string recommendation;

        BottleneckAnalysis()
            : average_duration(0), cpu_utilization(0.0), memory_delta(0), bottleneck_score(0.0) {}
    };

    struct ProfilerConfig {
        bool enable_timing;
        bool enable_memory_tracking;
        bool enable_thread_tracking;
        bool enable_bottleneck_detection;
        std::chrono::milliseconds sampling_interval;
        size_t max_samples;
        double bottleneck_threshold;  // 10% of total time

        ProfilerConfig()
            : enable_timing(true), enable_memory_tracking(true), enable_thread_tracking(true),
              enable_bottleneck_detection(true), sampling_interval(1), max_samples(10000),
              bottleneck_threshold(0.1) {}
    };

    explicit PerformanceProfiler(const ProfilerConfig& config = ProfilerConfig());
    ~PerformanceProfiler() = default;

    // Core profiling methods
    void startTiming(const std::string& operation_name);
    void endTiming(const std::string& operation_name);
    void recordMemoryUsage(const std::string& checkpoint_name);

    // RAII timing helper
    class ScopedTimer {
      public:
        ScopedTimer(PerformanceProfiler& profiler, const std::string& operation_name);
        ~ScopedTimer();

      private:
        PerformanceProfiler& profiler_;
        std::string operation_name_;
    };

    // Analysis and reporting
    std::vector<BottleneckAnalysis> identifyBottlenecks() const;
    void generateReport(const std::string& output_file = "") const;
    void reset();

    // Real-time monitoring
    void startContinuousMonitoring();
    void stopContinuousMonitoring();

    // Statistics
    TimingData getTimingData(const std::string& operation_name) const;
    std::map<std::string, TimingData> getAllTimingData() const;
    std::vector<MemorySnapshot> getMemoryHistory() const;

    // Configuration
    void updateConfig(const ProfilerConfig& config);
    ProfilerConfig getConfig() const;

  private:
    ProfilerConfig config_;
    mutable std::mutex data_mutex_;

    // Timing data storage
    std::unordered_map<std::string, TimingData> timing_data_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> active_timings_;

    // Memory tracking
    std::vector<MemorySnapshot> memory_history_;

    // Continuous monitoring
    std::unique_ptr<std::thread> monitoring_thread_;
    std::atomic<bool> monitoring_active_{false};

    // Helper methods
    void updateTimingStatistics(const std::string& operation_name,
                                std::chrono::nanoseconds duration);
    size_t getCurrentMemoryUsage() const;
    void monitoringLoop();
    double calculateBottleneckScore(const TimingData& timing,
                                    std::chrono::nanoseconds total_runtime) const;
    std::string generateRecommendation(const BottleneckAnalysis& analysis) const;
};

// Convenient macro for scoped timing
#define PROFILE_SCOPE(profiler, name) PerformanceProfiler::ScopedTimer _prof_timer(profiler, name)

#define PROFILE_FUNCTION(profiler) PROFILE_SCOPE(profiler, __FUNCTION__)

}  // namespace huntmaster
