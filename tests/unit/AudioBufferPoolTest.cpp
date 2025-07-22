#include <latch>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/AudioBufferPool.h"

using namespace huntmaster;

class AudioBufferPoolTest : public ::testing::Test {
  protected:
    void SetUp() override {
        pool = std::make_unique<AudioBufferPool>(4, 1024);
    }

    std::unique_ptr<AudioBufferPool> pool;
};

TEST_F(AudioBufferPoolTest, BasicAcquireRelease) {
    // 1. Arrange: Check the initial state of the pool.
    auto initial_stats = pool->getStats();
    ASSERT_EQ(initial_stats.available_buffers, 4);
    ASSERT_EQ(initial_stats.peak_usage, 0);

    // 2. Act: Acquire a buffer.
    auto handle_result = pool->acquire();
    ASSERT_TRUE(handle_result.has_value());

    // 3. Assert: Check the state after acquisition.
    auto after_acquire_stats = pool->getStats();
    EXPECT_EQ(after_acquire_stats.available_buffers, 3);
    EXPECT_EQ(after_acquire_stats.peak_usage, 1);

    auto& handle = handle_result.value();
    EXPECT_EQ(handle.size(), 1024 / sizeof(float));
    std::fill(handle.begin(), handle.end(), 1.0f);

    // 4. Act: Explicitly release the handle.
    handle = AudioBufferPool::BufferHandle{};

    // 5. Assert: Check the final state after release.
    auto final_stats = pool->getStats();
    EXPECT_EQ(final_stats.available_buffers, 4);
}

TEST_F(AudioBufferPoolTest, ExhaustPool) {
    const int pool_size = 4;
    std::vector<AudioBufferPool::BufferHandle> handles;
    handles.reserve(pool_size);

    // Acquire all buffers
    for (int i = 0; i < pool_size; ++i) {
        auto result = pool->acquire();
        ASSERT_TRUE(result.has_value());
        handles.push_back(std::move(result.value()));
        // Check stats after each acquisition
        EXPECT_EQ(pool->getStats().available_buffers, pool_size - (i + 1));
    }

    // Pool should be exhausted
    auto result = pool->tryAcquireFor(std::chrono::milliseconds(0));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::POOL_EXHAUSTED);
    EXPECT_EQ(pool->getStats().failed_allocations, 1);

    // Release one
    handles.pop_back();
    EXPECT_EQ(pool->getStats().available_buffers, 1);

    // Should be able to acquire again
    result = pool->acquire();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(pool->getStats().available_buffers, 0);
}

TEST_F(AudioBufferPoolTest, ThreadSafety) {
    const int num_threads = 4;
    const int operations_per_thread = 50;
    std::atomic<int> success_count{0};
    std::latch start_gate(num_threads + 1);  // Gate to start all threads at once
    std::latch end_gate(num_threads);        // Gate to wait for all threads to finish

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            start_gate.arrive_and_wait();  // Wait for the signal to start
            for (int i = 0; i < operations_per_thread; ++i) {
                auto result = pool->tryAcquireFor(std::chrono::milliseconds(10));
                if (result.has_value()) {
                    success_count++;
                }
            }
            end_gate.count_down();  // Signal that this thread is done
        });
    }

    start_gate.arrive_and_wait();  // Open the gate, starting all threads
    end_gate.wait();               // Wait for all threads to complete

    for (auto& t : threads) {
        t.join();
    }

    // With 4 threads and a pool of 4, all operations should succeed.
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);

    // Pool should still be functional
    auto stats = pool->getStats();
    EXPECT_EQ(stats.total_buffers, 4);
}