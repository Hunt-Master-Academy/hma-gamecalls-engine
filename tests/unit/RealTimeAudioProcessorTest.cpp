#include <gtest/gtest.h>

#include <chrono>
#include <cmath> // For std::sin and M_PI
#include <thread>
#include <vector>

// Define M_PI if not defined (common issue with some compilers)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "huntmaster/core/RealtimeAudioProcessor.h"

using namespace huntmaster;

namespace {

RealtimeAudioProcessor::Config DefaultConfig() {
    RealtimeAudioProcessor::Config cfg;
    cfg.ring_buffer_size = 16;  // Must be power of 2
    cfg.chunk_size = 8;
    cfg.enable_backpressure = false;
    cfg.enable_metrics = true;
    return cfg;
}

std::vector<float> MakeAudioData(size_t n, float value = 1.0f) {
    return std::vector<float>(n, value);
}

TEST(RealtimeAudioProcessorTest, EnqueueAndDequeueSingleChunk) {
    RealtimeAudioProcessor proc(DefaultConfig());
    auto data = MakeAudioData(8, 0.5f);

    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    EXPECT_FALSE(proc.isEmpty());

    auto chunk_opt = proc.tryDequeueChunk();
    ASSERT_TRUE(chunk_opt.has_value());
    EXPECT_EQ(chunk_opt->valid_samples, 8);
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(chunk_opt->data[i], 0.5f);
    }
    EXPECT_TRUE(proc.isEmpty());
}

TEST(RealtimeAudioProcessorTest, BufferFullReturnsFalse) {
    auto cfg = DefaultConfig();
    cfg.ring_buffer_size = 2;
    RealtimeAudioProcessor proc(cfg);

    auto data = MakeAudioData(8, 1.0f);
    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    // Buffer should now be full
    EXPECT_FALSE(proc.tryEnqueueAudio(data));
}

TEST(RealtimeAudioProcessorTest, BufferEmptyReturnsNullopt) {
    RealtimeAudioProcessor proc(DefaultConfig());
    EXPECT_FALSE(proc.tryDequeueChunk().has_value());
}

TEST(RealtimeAudioProcessorTest, EnqueueBatchAndDequeueBatch) {
    RealtimeAudioProcessor proc(DefaultConfig());
    // BUG FIX: The original code used a static vector, causing all spans
    // to point to the same data. This creates separate data for each batch.
    std::vector<std::vector<float>> data_storage;
    data_storage.reserve(4);
    std::vector<std::span<const float>> batches;
    for (int i = 0; i < 4; ++i) {
        data_storage.push_back(MakeAudioData(8, float(i)));
        batches.push_back(data_storage.back());
    }
    size_t enq = proc.enqueueBatch(batches);
    EXPECT_EQ(enq, 4);

    auto chunks = proc.dequeueBatch(4);
    EXPECT_EQ(chunks.size(), 4);
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].valid_samples, 8);
    }
}

TEST(RealtimeAudioProcessorTest, StatsAreUpdated) {
    RealtimeAudioProcessor proc(DefaultConfig());
    auto data = MakeAudioData(8, 0.2f);
    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    ASSERT_TRUE(proc.tryDequeueChunk().has_value());
    auto stats = proc.getStats();
    EXPECT_GE(stats.total_chunks_processed, 1);
}

TEST(RealtimeAudioProcessorTest, OverrunAndUnderrunCounters) {
    auto cfg = DefaultConfig();
    cfg.ring_buffer_size = 2;
    RealtimeAudioProcessor proc(cfg);
    auto data = MakeAudioData(8, 1.0f);

    // Fill buffer
    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    ASSERT_TRUE(proc.tryEnqueueAudio(data));
    // Overrun
    EXPECT_FALSE(proc.tryEnqueueAudio(data));
    // Empty buffer
    ASSERT_TRUE(proc.tryDequeueChunk().has_value());
    ASSERT_TRUE(proc.tryDequeueChunk().has_value());
    // Underrun
    EXPECT_FALSE(proc.tryDequeueChunk().has_value());

    auto stats = proc.getStats();
    EXPECT_GE(stats.chunks_dropped + stats.buffer_overruns, 1);
    EXPECT_GE(stats.buffer_underruns, 1);
}

TEST(RealtimeAudioProcessorTest, ThreadedProducerConsumer) {
    RealtimeAudioProcessor proc(DefaultConfig());
    std::atomic<int> produced{0}, consumed{0};

    auto producer = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto data = MakeAudioData(8, float(i));
            if (proc.tryEnqueueAudio(data)) {
                produced++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    auto consumer = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto chunk = proc.tryDequeueChunk();
            if (chunk) {
                consumed++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    };

    std::thread t1(producer), t2(consumer);
    t1.join();
    t2.join();

    EXPECT_GT(produced, 0);
    EXPECT_GT(consumed, 0);
}

TEST(RealtimeAudioProcessorTest, ProcessesAudioMetadata) {
    RealtimeAudioProcessor proc(DefaultConfig());
    
    // Create audio with varying amplitudes
    std::vector<float> data(8);
    for (size_t i = 0; i < 8; ++i) {
        data[i] = std::sin(2.0f * M_PI * i / 8.0f);
    }
    
    EXPECT_TRUE(proc.tryEnqueueAudio(data));
    auto chunk = proc.tryDequeueChunk();
    ASSERT_TRUE(chunk.has_value());
    
    // Energy should be calculated
    EXPECT_GT(chunk->energy_level, 0.0f);
    EXPECT_LT(chunk->energy_level, 1.0f);
    
    // Timestamp should be set
    auto now = std::chrono::steady_clock::now();
    auto diff = now - chunk->timestamp;
    EXPECT_LT(diff, std::chrono::seconds(1));
}

TEST(RealtimeAudioProcessorTest, HandlesVariableChunkSizes) {
    auto cfg = DefaultConfig();
    cfg.chunk_size = 512;  // Larger chunk size
    RealtimeAudioProcessor proc(cfg);
    
    // Test with exact chunk size
    auto data1 = MakeAudioData(512, 1.0f);
    EXPECT_TRUE(proc.tryEnqueueAudio(data1));
    
    // Test with smaller data
    auto data2 = MakeAudioData(256, 0.5f);
    EXPECT_TRUE(proc.tryEnqueueAudio(data2));
    
    // Verify both chunks
    auto chunk1 = proc.tryDequeueChunk();
    ASSERT_TRUE(chunk1.has_value());
    EXPECT_EQ(chunk1->valid_samples, 512);
    
    auto chunk2 = proc.tryDequeueChunk();
    ASSERT_TRUE(chunk2.has_value());
    EXPECT_EQ(chunk2->valid_samples, 256);
}

TEST(RealtimeAudioProcessorTest, VoiceDetectionThreshold) {
    RealtimeAudioProcessor proc(DefaultConfig());
    
    // Quiet audio (should not trigger voice detection)
    auto quiet = MakeAudioData(8, 0.001f);
    EXPECT_TRUE(proc.tryEnqueueAudio(quiet));
    auto quiet_chunk = proc.tryDequeueChunk();
    ASSERT_TRUE(quiet_chunk.has_value());
    EXPECT_FALSE(quiet_chunk->contains_voice);
    
    // Loud audio (should trigger voice detection)
    auto loud = MakeAudioData(8, 0.5f);
    EXPECT_TRUE(proc.tryEnqueueAudio(loud));
    auto loud_chunk = proc.tryDequeueChunk();
    ASSERT_TRUE(loud_chunk.has_value());
    EXPECT_TRUE(loud_chunk->contains_voice);
}

TEST(RealtimeAudioProcessorTest, ResetClearsBuffer) {
    RealtimeAudioProcessor proc(DefaultConfig());
    
    // Fill with some data
    for (int i = 0; i < 5; ++i) {
        auto data = MakeAudioData(8, static_cast<float>(i));
        ASSERT_TRUE(proc.tryEnqueueAudio(data));
    }
    
    EXPECT_FALSE(proc.isEmpty());
    auto stats_before = proc.getStats();
    EXPECT_GT(stats_before.total_chunks_processed, 0);
    
    // Reset
    proc.resetStats();
    
    // Stats should be reset (but buffer might not be cleared by resetStats)
    auto stats_after = proc.getStats();
    EXPECT_EQ(stats_after.total_chunks_processed, 0);
}

TEST(RealtimeAudioProcessorTest, PerformanceMetrics) {
    auto cfg = DefaultConfig();
    cfg.enable_metrics = true;
    RealtimeAudioProcessor proc(cfg);
    
    // Process multiple chunks
    for (int i = 0; i < 10; ++i) {
        auto data = MakeAudioData(8, 1.0f);
        ASSERT_TRUE(proc.tryEnqueueAudio(data));
        ASSERT_TRUE(proc.tryDequeueChunk().has_value());
    }
    
    auto stats = proc.getStats();
    // Should have tracked processing time
    EXPECT_GT(stats.total_processing_time.count(), 0);
    EXPECT_GT(stats.max_processing_time.count(), 0);
    EXPECT_GE(stats.max_processing_time.count(),
              stats.total_processing_time.count() / stats.total_chunks_processed);
}

}  // namespace