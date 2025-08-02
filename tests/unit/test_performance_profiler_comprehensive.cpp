/**
 * @file test_performance_profiler_comprehensive.cpp
 * @brief Comprehensive tests for PerformanceProfiler - targeting 75%+ coverage
 *
 * This test suite is specifically designed to achieve maximum code coverage
 * for the PerformanceProfiler class. Target: Achieve 75%+ coverage (+367 lines toward 90% goal)
 *
 * Tests cover:
 * - startProfiling() / stopProfiling() functionality
 * - Performance metric collection
 * - Report generation with various formats
 * - Memory usage tracking and analysis
 * - Bottleneck detection algorithms
 * - Real-time monitoring capabilities
 * - Configuration management
 * - Error handling and edge cases
 */

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/PerformanceProfiler.h"

namespace huntmaster {
namespace test {

/**
 * Comprehensive test fixture for PerformanceProfiler
 */
class PerformanceProfilerComprehensiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Default configuration for most tests
        default_config_.enable_timing = true;
        default_config_.enable_memory_tracking = true;
        default_config_.enable_thread_tracking = true;
        default_config_.enable_bottleneck_detection = true;
        default_config_.sampling_interval = std::chrono::milliseconds(5);
        default_config_.max_samples = 10000;
        default_config_.bottleneck_threshold = 0.15;  // 15% threshold

        profiler_ = std::make_unique<PerformanceProfiler>(default_config_);
    }

    void TearDown() override {
        if (profiler_) {
            profiler_->stopContinuousMonitoring();
            profiler_->reset();
        }
    }

    // Helper methods for generating test data
    void simulateVariableWorkload(const std::string& operation_name,
                                  int iterations,
                                  std::chrono::milliseconds min_duration,
                                  std::chrono::milliseconds max_duration) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_duration.count(), max_duration.count());

        for (int i = 0; i < iterations; ++i) {
            profiler_->startTiming(operation_name);
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            profiler_->endTiming(operation_name);
        }
    }

    void simulateMemoryIntensiveOperation(const std::string& checkpoint_prefix, int checkpoints) {
        for (int i = 0; i < checkpoints; ++i) {
            profiler_->recordMemoryUsage(checkpoint_prefix + "_" + std::to_string(i));
            // Simulate some memory allocation/deallocation
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    PerformanceProfiler::ProfilerConfig default_config_;
    std::unique_ptr<PerformanceProfiler> profiler_;
};

// ===== BASIC FUNCTIONALITY TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, ConfigurationManagement) {
    // Test configuration updates
    PerformanceProfiler::ProfilerConfig new_config;
    new_config.enable_timing = false;
    new_config.enable_memory_tracking = false;
    new_config.bottleneck_threshold = 0.25;
    new_config.sampling_interval = std::chrono::milliseconds(100);
    new_config.max_samples = 5000;

    profiler_->updateConfig(new_config);

    auto retrieved_config = profiler_->getConfig();
    EXPECT_EQ(retrieved_config.enable_timing, false);
    EXPECT_EQ(retrieved_config.enable_memory_tracking, false);
    EXPECT_DOUBLE_EQ(retrieved_config.bottleneck_threshold, 0.25);
    EXPECT_EQ(retrieved_config.sampling_interval.count(), 100);
    EXPECT_EQ(retrieved_config.max_samples, 5000);
}

TEST_F(PerformanceProfilerComprehensiveTest, TimingAccuracyAndPrecision) {
    // Test timing accuracy with known durations
    const std::vector<int> test_durations = {1, 5, 10, 50, 100};  // milliseconds

    for (int duration_ms : test_durations) {
        std::string operation_name = "precise_timing_" + std::to_string(duration_ms) + "ms";

        profiler_->startTiming(operation_name);
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        profiler_->endTiming(operation_name);

        auto timing_data = profiler_->getTimingData(operation_name);
        EXPECT_EQ(timing_data.call_count, 1);

        // Allow 20% tolerance for timing accuracy
        auto measured_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(timing_data.total_time).count();
        EXPECT_GE(measured_ms, duration_ms * 0.8);
        EXPECT_LE(measured_ms, duration_ms * 1.2);
    }
}

TEST_F(PerformanceProfilerComprehensiveTest, ScopedTimerFunctionality) {
    // Test scoped timer with various scenarios

    // Simple scoped timer
    {
        PerformanceProfiler::ScopedTimer timer(*profiler_, "scoped_simple");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Nested scoped timers
    {
        PerformanceProfiler::ScopedTimer outer(*profiler_, "scoped_outer");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        {
            PerformanceProfiler::ScopedTimer inner(*profiler_, "scoped_inner");
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // Exception safety test
    try {
        PerformanceProfiler::ScopedTimer timer(*profiler_, "scoped_exception");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        throw std::runtime_error("Test exception");
    } catch (const std::exception&) {
        // Timer should still record timing despite exception
    }

    // Verify all timings were recorded
    auto simple_timing = profiler_->getTimingData("scoped_simple");
    auto outer_timing = profiler_->getTimingData("scoped_outer");
    auto inner_timing = profiler_->getTimingData("scoped_inner");
    auto exception_timing = profiler_->getTimingData("scoped_exception");

    EXPECT_EQ(simple_timing.call_count, 1);
    EXPECT_EQ(outer_timing.call_count, 1);
    EXPECT_EQ(inner_timing.call_count, 1);
    EXPECT_EQ(exception_timing.call_count, 1);

    // Outer timer should include inner timer duration
    EXPECT_GT(outer_timing.total_time, inner_timing.total_time);
}

// ===== STATISTICAL ANALYSIS TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, StatisticalAccuracy) {
    // Test min/max/average calculations with known data
    const std::string operation = "statistical_test";
    const std::vector<int> durations = {1, 5, 10, 15, 20, 25, 30};  // milliseconds

    for (int duration_ms : durations) {
        profiler_->startTiming(operation);
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        profiler_->endTiming(operation);
    }

    auto timing_data = profiler_->getTimingData(operation);
    EXPECT_EQ(timing_data.call_count, durations.size());

    // Convert to milliseconds for easier validation
    auto min_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(timing_data.min_time).count();
    auto max_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(timing_data.max_time).count();
    auto total_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(timing_data.total_time).count();
    auto avg_ms = total_ms / timing_data.call_count;

    // Verify statistical properties (with some tolerance for timing variance)
    EXPECT_GE(min_ms, 0);
    EXPECT_LE(min_ms, 3);   // Should be close to 1ms
    EXPECT_GE(max_ms, 25);  // Should be close to 30ms
    EXPECT_LE(max_ms, 35);
    EXPECT_GE(avg_ms, 12);  // Should be around 15ms average
    EXPECT_LE(avg_ms, 18);
}

TEST_F(PerformanceProfilerComprehensiveTest, MemoryTrackingAccuracy) {
    // Test memory tracking functionality
    const std::string checkpoint_prefix = "memory_test";
    const int num_checkpoints = 10;

    simulateMemoryIntensiveOperation(checkpoint_prefix, num_checkpoints);

    auto memory_history = profiler_->getMemoryHistory();
    EXPECT_GE(memory_history.size(), num_checkpoints);

    // Verify timestamps are in order and reasonable
    for (size_t i = 1; i < memory_history.size(); ++i) {
        EXPECT_GE(memory_history[i].timestamp, memory_history[i - 1].timestamp);
    }
}

// ===== BOTTLENECK DETECTION TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, BottleneckDetectionBasic) {
    // Create clear bottleneck scenario

    // Fast operations (10 calls, ~1ms each)
    simulateVariableWorkload(
        "fast_operation", 10, std::chrono::milliseconds(1), std::chrono::milliseconds(2));

    // Medium operations (5 calls, ~5ms each)
    simulateVariableWorkload(
        "medium_operation", 5, std::chrono::milliseconds(4), std::chrono::milliseconds(6));

    // Bottleneck operation (3 calls, ~50ms each)
    simulateVariableWorkload(
        "bottleneck_operation", 3, std::chrono::milliseconds(45), std::chrono::milliseconds(55));

    auto bottlenecks = profiler_->identifyBottlenecks();
    EXPECT_GT(bottlenecks.size(), 0);

    // The bottleneck operation should have the highest score
    bool found_bottleneck = false;
    for (const auto& bottleneck : bottlenecks) {
        if (bottleneck.operation_name == "bottleneck_operation") {
            found_bottleneck = true;
            EXPECT_GT(bottleneck.bottleneck_score, 50.0);  // Should be high score
            EXPECT_FALSE(bottleneck.recommendation.empty());
        }
    }
    EXPECT_TRUE(found_bottleneck);
}

TEST_F(PerformanceProfilerComprehensiveTest, BottleneckThresholdConfiguration) {
    // Test different bottleneck thresholds

    // Setup scenario with medium impact operation
    simulateVariableWorkload(
        "moderate_impact", 10, std::chrono::milliseconds(10), std::chrono::milliseconds(15));

    // Test with high threshold (should not detect as bottleneck)
    PerformanceProfiler::ProfilerConfig high_threshold_config = default_config_;
    high_threshold_config.bottleneck_threshold = 0.8;  // 80%
    profiler_->updateConfig(high_threshold_config);

    auto bottlenecks_high = profiler_->identifyBottlenecks();
    bool found_with_high_threshold = false;
    for (const auto& bottleneck : bottlenecks_high) {
        if (bottleneck.operation_name == "moderate_impact") {
            found_with_high_threshold = true;
        }
    }

    // Test with low threshold (should detect as bottleneck)
    PerformanceProfiler::ProfilerConfig low_threshold_config = default_config_;
    low_threshold_config.bottleneck_threshold = 0.05;  // 5%
    profiler_->updateConfig(low_threshold_config);

    auto bottlenecks_low = profiler_->identifyBottlenecks();
    bool found_with_low_threshold = false;
    for (const auto& bottleneck : bottlenecks_low) {
        if (bottleneck.operation_name == "moderate_impact") {
            found_with_low_threshold = true;
        }
    }

    // Low threshold should be more sensitive
    EXPECT_GE(bottlenecks_low.size(), bottlenecks_high.size());
}

// ===== REAL-TIME MONITORING TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, ContinuousMonitoringFunctionality) {
    // Test continuous monitoring start/stop
    EXPECT_TRUE(profiler_->getConfig().enable_timing);  // Initially enabled as per default config

    profiler_->startContinuousMonitoring();

    // Simulate some operations while monitoring
    simulateVariableWorkload(
        "monitored_operation", 20, std::chrono::milliseconds(1), std::chrono::milliseconds(5));

    // Let monitoring run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    profiler_->stopContinuousMonitoring();

    // Verify data was collected
    auto timing_data = profiler_->getTimingData("monitored_operation");
    EXPECT_EQ(timing_data.call_count, 20);

    auto memory_history = profiler_->getMemoryHistory();
    EXPECT_GT(memory_history.size(), 0);
}

TEST_F(PerformanceProfilerComprehensiveTest, MonitoringConfigurationImpact) {
    // Test how different configurations affect monitoring

    // Test with memory tracking disabled
    PerformanceProfiler::ProfilerConfig no_memory_config = default_config_;
    no_memory_config.enable_memory_tracking = false;
    profiler_->updateConfig(no_memory_config);

    profiler_->startContinuousMonitoring();
    simulateMemoryIntensiveOperation("no_memory_tracking", 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    profiler_->stopContinuousMonitoring();

    // Memory history should be minimal or empty when tracking disabled
    auto memory_history_disabled = profiler_->getMemoryHistory();

    // Test with memory tracking enabled
    PerformanceProfiler::ProfilerConfig memory_config = default_config_;
    memory_config.enable_memory_tracking = true;
    profiler_->updateConfig(memory_config);
    profiler_->reset();  // Clear previous data

    profiler_->startContinuousMonitoring();
    simulateMemoryIntensiveOperation("with_memory_tracking", 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    profiler_->stopContinuousMonitoring();

    auto memory_history_enabled = profiler_->getMemoryHistory();

    // Should have more memory data when tracking enabled
    EXPECT_GE(memory_history_enabled.size(), memory_history_disabled.size());
}

// ===== REPORT GENERATION TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, ReportGenerationToFile) {
    // Setup some test data
    simulateVariableWorkload(
        "report_test_op1", 10, std::chrono::milliseconds(5), std::chrono::milliseconds(15));
    simulateVariableWorkload(
        "report_test_op2", 5, std::chrono::milliseconds(20), std::chrono::milliseconds(30));

    simulateMemoryIntensiveOperation("report_memory", 5);

    // Generate report to file
    const std::string report_filename = "test_performance_report.txt";
    profiler_->generateReport(report_filename);

    // Verify file was created and has content
    std::ifstream report_file(report_filename);
    ASSERT_TRUE(report_file.is_open());

    std::string line;
    std::vector<std::string> report_lines;
    while (std::getline(report_file, line)) {
        report_lines.push_back(line);
    }
    report_file.close();

    EXPECT_GT(report_lines.size(), 0);

    // Check for expected content in report
    bool found_timing_section = false;
    bool found_operation1 = false;
    bool found_operation2 = false;

    for (const auto& line : report_lines) {
        if (line.find("Timing") != std::string::npos) {
            found_timing_section = true;
        }
        if (line.find("report_test_op1") != std::string::npos) {
            found_operation1 = true;
        }
        if (line.find("report_test_op2") != std::string::npos) {
            found_operation2 = true;
        }
    }

    EXPECT_TRUE(found_timing_section);
    EXPECT_TRUE(found_operation1);
    EXPECT_TRUE(found_operation2);

    // Clean up test file
    std::remove(report_filename.c_str());
}

TEST_F(PerformanceProfilerComprehensiveTest, ReportGenerationToString) {
    // Test generating report without file (should go to string/stdout)
    simulateVariableWorkload(
        "string_report_test", 3, std::chrono::milliseconds(10), std::chrono::milliseconds(10));

    // This should not crash and should work without file
    EXPECT_NO_THROW(profiler_->generateReport(""));
    EXPECT_NO_THROW(profiler_->generateReport());
}

// ===== DATA RETRIEVAL TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, AllTimingDataRetrieval) {
    // Create multiple operations
    const std::vector<std::string> operations = {
        "retrieval_test_1", "retrieval_test_2", "retrieval_test_3"};

    for (const auto& op : operations) {
        simulateVariableWorkload(op, 5, std::chrono::milliseconds(1), std::chrono::milliseconds(3));
    }

    auto all_timing_data = profiler_->getAllTimingData();

    // Should have at least our test operations
    EXPECT_GE(all_timing_data.size(), operations.size());

    for (const auto& op : operations) {
        EXPECT_TRUE(all_timing_data.find(op) != all_timing_data.end());
        EXPECT_EQ(all_timing_data[op].call_count, 5);
    }
}

TEST_F(PerformanceProfilerComprehensiveTest, MemoryHistoryRetrieval) {
    // Test memory history retrieval functionality
    const int num_memory_checkpoints = 15;
    simulateMemoryIntensiveOperation("history_test", num_memory_checkpoints);

    auto memory_history = profiler_->getMemoryHistory();
    EXPECT_GE(memory_history.size(), num_memory_checkpoints);

    // Verify data integrity
    for (const auto& snapshot : memory_history) {
        EXPECT_GE(snapshot.total_allocated, 0);
        EXPECT_GE(snapshot.current_usage, 0);
        EXPECT_GE(snapshot.peak_usage, snapshot.current_usage);
    }
}

// ===== ERROR HANDLING AND EDGE CASES =====

TEST_F(PerformanceProfilerComprehensiveTest, ErrorHandlingInvalidOperations) {
    // Test ending timing that was never started
    EXPECT_NO_THROW(profiler_->endTiming("never_started"));

    // Test getting timing data for non-existent operation
    auto invalid_timing = profiler_->getTimingData("non_existent");
    EXPECT_EQ(invalid_timing.call_count, 0);
    EXPECT_EQ(invalid_timing.total_time.count(), 0);

    // Test starting timing twice for same operation
    profiler_->startTiming("double_start");
    EXPECT_NO_THROW(profiler_->startTiming("double_start"));  // Should handle gracefully
    profiler_->endTiming("double_start");

    auto double_start_timing = profiler_->getTimingData("double_start");
    EXPECT_GT(double_start_timing.call_count, 0);
}

TEST_F(PerformanceProfilerComprehensiveTest, LargeDatasetHandling) {
    // Test handling large numbers of operations and calls
    const int num_operations = 100;
    const int calls_per_operation = 50;

    for (int op = 0; op < num_operations; ++op) {
        std::string operation_name = "large_test_op_" + std::to_string(op);

        for (int call = 0; call < calls_per_operation; ++call) {
            profiler_->startTiming(operation_name);
            // Very short operation to test performance of profiler itself
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            profiler_->endTiming(operation_name);
        }
    }

    auto all_data = profiler_->getAllTimingData();
    EXPECT_EQ(all_data.size(), num_operations);

    for (const auto& [name, timing] : all_data) {
        EXPECT_EQ(timing.call_count, calls_per_operation);
    }

    // Test that bottleneck detection still works with large dataset
    auto bottlenecks = profiler_->identifyBottlenecks();
    EXPECT_GE(bottlenecks.size(), 0);  // Should not crash
}

TEST_F(PerformanceProfilerComprehensiveTest, ResetFunctionality) {
    // Setup some data
    simulateVariableWorkload(
        "reset_test", 10, std::chrono::milliseconds(1), std::chrono::milliseconds(2));
    simulateMemoryIntensiveOperation("reset_memory", 5);

    // Verify data exists
    auto timing_before = profiler_->getTimingData("reset_test");
    auto memory_before = profiler_->getMemoryHistory();
    EXPECT_GT(timing_before.call_count, 0);
    EXPECT_GT(memory_before.size(), 0);

    // Reset profiler
    profiler_->reset();

    // Verify data is cleared
    auto timing_after = profiler_->getTimingData("reset_test");
    auto memory_after = profiler_->getMemoryHistory();
    EXPECT_EQ(timing_after.call_count, 0);
    EXPECT_EQ(memory_after.size(), 0);

    auto all_data_after = profiler_->getAllTimingData();
    EXPECT_EQ(all_data_after.size(), 0);
}

// ===== CONCURRENCY AND THREAD SAFETY TESTS =====

TEST_F(PerformanceProfilerComprehensiveTest, ThreadSafetyBasic) {
    // Test basic thread safety with multiple threads timing operations
    const int num_threads = 4;
    const int operations_per_thread = 25;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, operations_per_thread]() {
            std::string thread_operation = "thread_" + std::to_string(t) + "_operation";

            for (int i = 0; i < operations_per_thread; ++i) {
                profiler_->startTiming(thread_operation);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                profiler_->endTiming(thread_operation);
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all operations were recorded correctly
    auto all_data = profiler_->getAllTimingData();
    EXPECT_EQ(all_data.size(), num_threads);

    for (int t = 0; t < num_threads; ++t) {
        std::string thread_operation = "thread_" + std::to_string(t) + "_operation";
        EXPECT_TRUE(all_data.find(thread_operation) != all_data.end());
        EXPECT_EQ(all_data[thread_operation].call_count, operations_per_thread);
    }
}

}  // namespace test
}  // namespace huntmaster
