#include "huntmaster/core/PerformanceProfiler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace huntmaster {

PerformanceProfiler::PerformanceProfiler(const ProfilerConfig& config) : config_(config) {
    if (config_.enable_memory_tracking) {
        recordMemoryUsage("profiler_start");
    }
}

void PerformanceProfiler::startTiming(const std::string& operation_name) {
    if (!config_.enable_timing)
        return;

    std::lock_guard<std::mutex> lock(data_mutex_);
    active_timings_[operation_name] = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endTiming(const std::string& operation_name) {
    if (!config_.enable_timing)
        return;

    auto end_time = std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = active_timings_.find(operation_name);
    if (it != active_timings_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - it->second);
        updateTimingStatistics(operation_name, duration);
        active_timings_.erase(it);
    }
}

void PerformanceProfiler::recordMemoryUsage([[maybe_unused]] const std::string& checkpoint_name) {
    if (!config_.enable_memory_tracking)
        return;

    std::lock_guard<std::mutex> lock(data_mutex_);
    MemorySnapshot snapshot;
    snapshot.current_usage = getCurrentMemoryUsage();
    snapshot.timestamp = std::chrono::high_resolution_clock::now();

    if (!memory_history_.empty()) {
        snapshot.peak_usage = std::max(memory_history_.back().peak_usage, snapshot.current_usage);
    } else {
        snapshot.peak_usage = snapshot.current_usage;
    }

    memory_history_.push_back(snapshot);

    // Limit memory history size (checkpoint_name currently unused in minimal implementation)
    if (memory_history_.size() > config_.max_samples) {
        memory_history_.erase(memory_history_.begin());
    }
}

PerformanceProfiler::ScopedTimer::ScopedTimer(PerformanceProfiler& profiler,
                                              const std::string& operation_name)
    : profiler_(profiler), operation_name_(operation_name) {
    profiler_.startTiming(operation_name_);
}

PerformanceProfiler::ScopedTimer::~ScopedTimer() {
    profiler_.endTiming(operation_name_);
}

std::vector<PerformanceProfiler::BottleneckAnalysis>
PerformanceProfiler::identifyBottlenecks() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<BottleneckAnalysis> bottlenecks;

    if (timing_data_.empty())
        return bottlenecks;

    // Calculate total runtime across all operations
    std::chrono::nanoseconds total_runtime{0};
    for (const auto& [name, timing] : timing_data_) {
        total_runtime += timing.total_time;
    }

    if (total_runtime.count() == 0)
        return bottlenecks;

    // Analyze each operation for bottlenecks
    for (const auto& [name, timing] : timing_data_) {
        BottleneckAnalysis analysis;
        analysis.operation_name = name;
        analysis.average_duration =
            timing.call_count > 0
                ? std::chrono::nanoseconds(timing.total_time.count() / timing.call_count)
                : std::chrono::nanoseconds{0};

        // Calculate bottleneck score based on time percentage and variance
        double time_percentage =
            static_cast<double>(timing.total_time.count()) / total_runtime.count();
        analysis.bottleneck_score = calculateBottleneckScore(timing, total_runtime);

        // Only include operations that exceed the bottleneck threshold
        if (time_percentage >= config_.bottleneck_threshold || analysis.bottleneck_score > 50.0) {
            analysis.recommendation = generateRecommendation(analysis);
            bottlenecks.push_back(analysis);
        }
    }

    // Sort by bottleneck score (highest first)
    std::sort(bottlenecks.begin(),
              bottlenecks.end(),
              [](const BottleneckAnalysis& a, const BottleneckAnalysis& b) {
                  return a.bottleneck_score > b.bottleneck_score;
              });

    return bottlenecks;
}

void PerformanceProfiler::generateReport(const std::string& output_file) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    std::ostringstream report;
    report << "=== Performance Profiling Report ===\n\n";

    // Timing Analysis
    report << "--- Timing Analysis ---\n";
    report << std::left << std::setw(30) << "Operation" << std::setw(15) << "Call Count"
           << std::setw(15) << "Total Time" << std::setw(15) << "Avg Time" << std::setw(15)
           << "Min Time" << std::setw(15) << "Max Time" << "\n";
    report << std::string(120, '-') << "\n";

    for (const auto& [name, timing] : timing_data_) {
        auto avg_time =
            timing.call_count > 0
                ? std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::nanoseconds(timing.total_time.count() / timing.call_count))
                : std::chrono::microseconds{0};

        report << std::left << std::setw(30) << name << std::setw(15) << timing.call_count
               << std::setw(15)
               << std::chrono::duration_cast<std::chrono::milliseconds>(timing.total_time).count()
               << "ms" << std::setw(15) << avg_time.count() << "μs" << std::setw(15)
               << std::chrono::duration_cast<std::chrono::microseconds>(timing.min_time).count()
               << "μs" << std::setw(15)
               << std::chrono::duration_cast<std::chrono::microseconds>(timing.max_time).count()
               << "μs" << "\n";
    }

    // Bottleneck Analysis
    report << "\n--- Bottleneck Analysis ---\n";
    auto bottlenecks = identifyBottlenecks();

    if (bottlenecks.empty()) {
        report << "No significant bottlenecks detected.\n";
    } else {
        report << std::left << std::setw(30) << "Operation" << std::setw(15) << "Avg Duration"
               << std::setw(10) << "Score" << std::setw(50) << "Recommendation" << "\n";
        report << std::string(105, '-') << "\n";

        for (const auto& bottleneck : bottlenecks) {
            report << std::left << std::setw(30) << bottleneck.operation_name << std::setw(15)
                   << std::chrono::duration_cast<std::chrono::microseconds>(
                          bottleneck.average_duration)
                          .count()
                   << "μs" << std::setw(10) << std::fixed << std::setprecision(1)
                   << bottleneck.bottleneck_score << std::setw(50) << bottleneck.recommendation
                   << "\n";
        }
    }

    // Memory Analysis
    if (config_.enable_memory_tracking && !memory_history_.empty()) {
        report << "\n--- Memory Analysis ---\n";
        report << "Peak Memory Usage: " << memory_history_.back().peak_usage / 1024 << " KB\n";
        report << "Current Memory Usage: " << memory_history_.back().current_usage / 1024
               << " KB\n";
        report << "Memory Samples: " << memory_history_.size() << "\n";
    }

    report << "\n=== End Report ===\n";

    if (output_file.empty()) {
        std::cout << report.str();
    } else {
        std::ofstream file(output_file);
        if (file.is_open()) {
            file << report.str();
            file.close();
            std::cout << "Performance report written to: " << output_file << std::endl;
        }
    }
}

void PerformanceProfiler::reset() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    timing_data_.clear();
    active_timings_.clear();
    memory_history_.clear();
}

void PerformanceProfiler::startContinuousMonitoring() {
    if (monitoring_active_.load())
        return;

    monitoring_active_.store(true);
    monitoring_thread_ = std::make_unique<std::thread>(&PerformanceProfiler::monitoringLoop, this);
}

void PerformanceProfiler::stopContinuousMonitoring() {
    monitoring_active_.store(false);
    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
        monitoring_thread_.reset();
    }
}

PerformanceProfiler::TimingData
PerformanceProfiler::getTimingData(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = timing_data_.find(operation_name);
    return (it != timing_data_.end()) ? it->second : TimingData{};
}

std::map<std::string, PerformanceProfiler::TimingData>
PerformanceProfiler::getAllTimingData() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return std::map<std::string, TimingData>(timing_data_.begin(), timing_data_.end());
}

std::vector<PerformanceProfiler::MemorySnapshot> PerformanceProfiler::getMemoryHistory() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return memory_history_;
}

void PerformanceProfiler::updateConfig(const ProfilerConfig& config) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    config_ = config;
}

PerformanceProfiler::ProfilerConfig PerformanceProfiler::getConfig() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return config_;
}

void PerformanceProfiler::updateTimingStatistics(const std::string& operation_name,
                                                 std::chrono::nanoseconds duration) {
    auto& timing = timing_data_[operation_name];
    timing.call_count++;
    timing.total_time += duration;
    timing.min_time = std::min(timing.min_time, duration);
    timing.max_time = std::max(timing.max_time, duration);
    timing.thread_id = std::this_thread::get_id();
}

size_t PerformanceProfiler::getCurrentMemoryUsage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;  // Convert KB to bytes on Linux
    }
    return 0;
#endif
}

void PerformanceProfiler::monitoringLoop() {
    while (monitoring_active_.load()) {
        recordMemoryUsage("continuous_monitoring");
        std::this_thread::sleep_for(config_.sampling_interval);
    }
}

double PerformanceProfiler::calculateBottleneckScore(const TimingData& timing,
                                                     std::chrono::nanoseconds total_runtime) const {
    if (total_runtime.count() == 0 || timing.call_count == 0)
        return 0.0;

    // Base score from time percentage (0-50)
    double time_percentage = static_cast<double>(timing.total_time.count()) / total_runtime.count();
    double base_score = std::min(50.0, time_percentage * 500.0);

    // Variance penalty (0-30) - high variance indicates inconsistent performance
    if (timing.call_count > 1) {
        auto avg_time = timing.total_time.count() / timing.call_count;
        double max_deviation =
            std::max(abs(static_cast<long long>(timing.max_time.count()) - avg_time),
                     abs(static_cast<long long>(timing.min_time.count()) - avg_time));
        double variance_score = std::min(30.0, (max_deviation / avg_time) * 30.0);
        base_score += variance_score;
    }

    // Frequency penalty (0-20) - very frequent operations that are slow
    if (timing.call_count > 100) {
        double frequency_score = std::min(20.0, (timing.call_count / 1000.0) * 20.0);
        base_score += frequency_score;
    }

    return std::min(100.0, base_score);
}

std::string PerformanceProfiler::generateRecommendation(const BottleneckAnalysis& analysis) const {
    if (analysis.bottleneck_score > 80.0) {
        return "Critical bottleneck - requires immediate optimization";
    } else if (analysis.bottleneck_score > 60.0) {
        return "Significant bottleneck - consider optimization";
    } else if (analysis.bottleneck_score > 40.0) {
        return "Moderate bottleneck - monitor and optimize if needed";
    } else {
        return "Minor bottleneck - low priority for optimization";
    }
}

}  // namespace huntmaster
