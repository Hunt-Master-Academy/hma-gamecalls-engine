#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

namespace huntmaster {
namespace test {

class PerformanceProfilerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        PerformanceProfiler::ProfilerConfig config;
        config.enable_timing = true;
        config.enable_memory_tracking = true;
        config.bottleneck_threshold = 0.1;  // 10% threshold
        config.sampling_interval = std::chrono::milliseconds(10);
        config.max_samples = 1000;

        profiler_ = std::make_unique<PerformanceProfiler>(config);
    }

    void TearDown() override {
        profiler_->stopContinuousMonitoring();
        profiler_.reset();
    }

    std::unique_ptr<PerformanceProfiler> profiler_;
};

TEST_F(PerformanceProfilerTest, BasicTimingMeasurement) {
    // Test basic timing functionality
    profiler_->startTiming("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    profiler_->endTiming("test_operation");

    auto timing_data = profiler_->getTimingData("test_operation");
    EXPECT_EQ(timing_data.call_count, 1);
    EXPECT_GT(timing_data.total_time.count(), 0);
    EXPECT_GT(timing_data.min_time.count(), 0);
    EXPECT_GT(timing_data.max_time.count(), 0);
}

TEST_F(PerformanceProfilerTest, ScopedTimerRAII) {
    // Test RAII scoped timer
    {
        auto timer = profiler_->createScopedTimer("scoped_test");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }  // Timer ends here automatically

    auto timing_data = profiler_->getTimingData("scoped_test");
    EXPECT_EQ(timing_data.call_count, 1);
    EXPECT_GT(timing_data.total_time.count(), 0);
}

TEST_F(PerformanceProfilerTest, MultipleOperationTiming) {
    // Test multiple operations with different characteristics

    // Fast operation called many times
    for (int i = 0; i < 100; ++i) {
        profiler_->startTiming("fast_operation");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        profiler_->endTiming("fast_operation");
    }

    // Slow operation called few times
    for (int i = 0; i < 5; ++i) {
        profiler_->startTiming("slow_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        profiler_->endTiming("slow_operation");
    }

    auto fast_timing = profiler_->getTimingData("fast_operation");
    auto slow_timing = profiler_->getTimingData("slow_operation");

    EXPECT_EQ(fast_timing.call_count, 100);
    EXPECT_EQ(slow_timing.call_count, 5);
    EXPECT_LT(fast_timing.min_time, slow_timing.min_time);
}

TEST_F(PerformanceProfilerTest, BottleneckDetection) {
    // Create a clear bottleneck scenario

    // Normal operations
    for (int i = 0; i < 10; ++i) {
        profiler_->startTiming("normal_op");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        profiler_->endTiming("normal_op");
    }

    // Bottleneck operation
    for (int i = 0; i < 5; ++i) {
        profiler_->startTiming("bottleneck_op");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // Much slower
        profiler_->endTiming("bottleneck_op");
    }

    auto bottlenecks = profiler_->identifyBottlenecks();
    EXPECT_GT(bottlenecks.size(), 0);

    // The bottleneck operation should be identified
    bool found_bottleneck = false;
    for (const auto& bottleneck : bottlenecks) {
        if (bottleneck.operation_name == "bottleneck_op") {
            found_bottleneck = true;
            EXPECT_GT(bottleneck.bottleneck_score, 50.0);
            EXPECT_FALSE(bottleneck.recommendation.empty());
            break;
        }
    }
    EXPECT_TRUE(found_bottleneck);
}

TEST_F(PerformanceProfilerTest, MemoryTracking) {
    profiler_->recordMemoryUsage("start");

    // Simulate memory allocation
    std::vector<std::vector<char>> memory_hog;
    for (int i = 0; i < 10; ++i) {
        memory_hog.emplace_back(1024 * 1024);  // 1MB each
        profiler_->recordMemoryUsage("allocation_" + std::to_string(i));
    }

    auto memory_history = profiler_->getMemoryHistory();
    EXPECT_GT(memory_history.size(), 1);

    // Memory usage should generally increase
    EXPECT_GE(memory_history.back().current_usage, memory_history.front().current_usage);
    EXPECT_GE(memory_history.back().peak_usage, memory_history.back().current_usage);
}

TEST_F(PerformanceProfilerTest, ContinuousMonitoring) {
    profiler_->startContinuousMonitoring();

    // Let it run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    profiler_->stopContinuousMonitoring();

    auto memory_history = profiler_->getMemoryHistory();
    EXPECT_GT(memory_history.size(), 1);  // Should have collected samples
}

TEST_F(PerformanceProfilerTest, ReportGeneration) {
    // Create some test data
    profiler_->startTiming("report_test");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    profiler_->endTiming("report_test");

    profiler_->recordMemoryUsage("report_checkpoint");

    // Test console report (no file output)
    EXPECT_NO_THROW(profiler_->generateReport());

    // Test file report
    std::string report_file = "test_performance_report.txt";
    EXPECT_NO_THROW(profiler_->generateReport(report_file));

    // Verify file was created (basic check)
    std::ifstream file(report_file);
    EXPECT_TRUE(file.is_open());
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        EXPECT_FALSE(content.empty());
        EXPECT_NE(content.find("Performance Profiling Report"), std::string::npos);
        file.close();
        std::remove(report_file.c_str());  // Clean up
    }
}

TEST_F(PerformanceProfilerTest, ConfigurationUpdates) {
    // Test initial config
    auto initial_config = profiler_->getConfig();
    EXPECT_TRUE(initial_config.enable_timing);
    EXPECT_TRUE(initial_config.enable_memory_tracking);

    // Update config
    PerformanceProfiler::ProfilerConfig new_config;
    new_config.enable_timing = false;
    new_config.enable_memory_tracking = true;
    new_config.bottleneck_threshold = 0.05;

    profiler_->updateConfig(new_config);

    auto updated_config = profiler_->getConfig();
    EXPECT_FALSE(updated_config.enable_timing);
    EXPECT_TRUE(updated_config.enable_memory_tracking);
    EXPECT_DOUBLE_EQ(updated_config.bottleneck_threshold, 0.05);
}

// Integration test with UnifiedAudioEngine
class UnifiedAudioEnginePerformanceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        PerformanceProfiler::ProfilerConfig config;
        config.enable_timing = true;
        config.enable_memory_tracking = true;
        config.bottleneck_threshold = 0.05;
        config.sampling_interval = std::chrono::milliseconds(10);

        profiler_ = std::make_unique<PerformanceProfiler>(config);

        UnifiedAudioEngine::Config engine_config;
        engine_config.sample_rate = 16000;
        engine_config.max_recording_duration = 10.0;
        engine_config.silence_threshold = 0.01;

        engine_ = UnifiedAudioEngine::create(engine_config);
        ASSERT_TRUE(engine_.has_value());
    }

    void TearDown() override {
        profiler_->stopContinuousMonitoring();
        profiler_.reset();
    }

    std::unique_ptr<PerformanceProfiler> profiler_;
    std::optional<UnifiedAudioEngine> engine_;
};

TEST_F(UnifiedAudioEnginePerformanceTest, SessionManagementProfiling) {
    ASSERT_TRUE(engine_.has_value());

    // Profile session creation
    {
        auto timer = profiler_->createScopedTimer("session_creation");
        auto session_result = engine_->createSession();
        ASSERT_TRUE(session_result.has_value());
    }

    // Profile multiple sessions
    std::vector<UnifiedAudioEngine::SessionId> session_ids;
    for (int i = 0; i < 10; ++i) {
        auto timer = profiler_->createScopedTimer("batch_session_creation");
        auto session_result = engine_->createSession();
        ASSERT_TRUE(session_result.has_value());
        session_ids.push_back(session_result.value());
    }

    // Profile session cleanup
    for (const auto& session_id : session_ids) {
        auto timer = profiler_->createScopedTimer("session_cleanup");
        auto result = engine_->removeSession(session_id);
        EXPECT_TRUE(result.has_value());
    }

    // Analyze performance
    auto session_creation_timing = profiler_->getTimingData("session_creation");
    auto batch_creation_timing = profiler_->getTimingData("batch_session_creation");
    auto cleanup_timing = profiler_->getTimingData("session_cleanup");

    EXPECT_GT(session_creation_timing.call_count, 0);
    EXPECT_EQ(batch_creation_timing.call_count, 10);
    EXPECT_EQ(cleanup_timing.call_count, 10);

    // Generate performance report
    profiler_->generateReport("engine_session_performance.txt");
}

TEST_F(UnifiedAudioEnginePerformanceTest, AudioProcessingProfiling) {
    ASSERT_TRUE(engine_.has_value());

    auto session_result = engine_->createSession();
    ASSERT_TRUE(session_result.has_value());
    auto session_id = session_result.value();

    // Start continuous monitoring
    profiler_->startContinuousMonitoring();

    // Simulate audio processing workload
    std::vector<float> test_audio_chunk(1024, 0.5f);  // 1024 samples at 0.5 amplitude

    for (int i = 0; i < 100; ++i) {
        {
            auto timer = profiler_->createScopedTimer("audio_processing");
            auto result = engine_->processAudioChunk(session_id, test_audio_chunk);
            // Processing result depends on engine implementation
        }

        // Record memory usage periodically
        if (i % 10 == 0) {
            profiler_->recordMemoryUsage("processing_checkpoint_" + std::to_string(i));
        }
    }

    profiler_->stopContinuousMonitoring();

    // Analyze results
    auto processing_timing = profiler_->getTimingData("audio_processing");
    EXPECT_EQ(processing_timing.call_count, 100);

    auto bottlenecks = profiler_->identifyBottlenecks();

    // Generate comprehensive performance report
    profiler_->generateReport("engine_audio_processing_performance.txt");

    // Clean up
    engine_->removeSession(session_id);
}

}  // namespace test
}  // namespace huntmaster
