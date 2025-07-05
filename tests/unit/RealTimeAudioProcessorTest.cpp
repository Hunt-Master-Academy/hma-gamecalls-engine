#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "../../include/huntmaster/core/RealtimeAudioProcessor.h"

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

    EXPECT_TRUE(proc.tryEnqueueAudio(data));
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
    EXPECT_TRUE(proc.tryEnqueueAudio(data));
    EXPECT_TRUE(proc.tryEnqueueAudio(data));
    // Buffer should now be full
    EXPECT_FALSE(proc.tryEnqueueAudio(data));
}

TEST(RealtimeAudioProcessorTest, BufferEmptyReturnsNullopt) {
    RealtimeAudioProcessor proc(DefaultConfig());
    EXPECT_FALSE(proc.tryDequeueChunk().has_value());
}

TEST(RealtimeAudioProcessorTest, EnqueueBatchAndDequeueBatch) {
    RealtimeAudioProcessor proc(DefaultConfig());
    std::vector<std::span<const float>> batches;
    for (int i = 0; i < 4; ++i) {
        static std::vector<float> data(8, float(i));
        batches.push_back(data);
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
    proc.tryEnqueueAudio(data);
    proc.tryDequeueChunk();
    auto stats = proc.getStats();
    EXPECT_GE(stats.total_chunks_processed, 1);
}

TEST(RealtimeAudioProcessorTest, OverrunAndUnderrunCounters) {
    auto cfg = DefaultConfig();
    cfg.ring_buffer_size = 2;
    RealtimeAudioProcessor proc(cfg);
    auto data = MakeAudioData(8, 1.0f);

    // Fill buffer
    EXPECT_TRUE(proc.tryEnqueueAudio(data));
    EXPECT_TRUE(proc.tryEnqueueAudio(data));
    // Overrun
    EXPECT_FALSE(proc.tryEnqueueAudio(data));
    // Empty buffer
    EXPECT_TRUE(proc.tryDequeueChunk().has_value());
    EXPECT_TRUE(proc.tryDequeueChunk().has_value());
    // Underrun
    EXPECT_FALSE(proc.tryDequeueChunk().has_value());

    auto stats = proc.getStats();
    EXPECT_GE(stats.chunks_dropped + stats.chunks_overrun, 1);
    EXPECT_GE(stats.chunks_underrun, 1);
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

}  // namespace