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

// Test factory method and configuration
TEST_F(AudioBufferPoolTest, FactoryMethodTest) {
    AudioBufferPool::Config config;
    config.pool_size = 8;
    config.buffer_size = 2048;
    config.alignment = 32;

    auto result = AudioBufferPool::create(config);
    ASSERT_TRUE(result.has_value());

    auto& created_pool = result.value();
    auto stats = created_pool->getStats();
    EXPECT_EQ(stats.total_buffers, 8);
    EXPECT_EQ(stats.available_buffers, 8);

    // Test acquiring from factory-created pool
    auto handle = created_pool->acquire();
    ASSERT_TRUE(handle.has_value());
    EXPECT_EQ(handle.value().size(), 2048 / sizeof(float));
}

// Test BufferHandle methods
TEST_F(AudioBufferPoolTest, BufferHandleMethods) {
    auto handle_result = pool->acquire();
    ASSERT_TRUE(handle_result.has_value());
    auto& handle = handle_result.value();

    // Test valid() and operator bool()
    EXPECT_TRUE(handle.valid());
    EXPECT_TRUE(static_cast<bool>(handle));

    // Test size
    EXPECT_EQ(handle.size(), 1024 / sizeof(float));

    // Test data() methods
    auto float_span = handle.data();
    EXPECT_EQ(float_span.size(), 1024 / sizeof(float));

    const auto& const_handle = handle;
    auto const_span = const_handle.data();
    EXPECT_EQ(const_span.size(), 1024 / sizeof(float));

    // Test bytes() methods
    auto byte_span = handle.bytes();
    EXPECT_EQ(byte_span.size(), 1024);

    auto const_byte_span = const_handle.bytes();
    EXPECT_EQ(const_byte_span.size(), 1024);

    // Test iterator support
    auto begin_it = handle.begin();
    auto end_it = handle.end();
    EXPECT_EQ(std::distance(begin_it, end_it), static_cast<ptrdiff_t>(handle.size()));

    // Test const iterators
    auto const_begin = const_handle.begin();
    auto const_end = const_handle.end();
    EXPECT_EQ(std::distance(const_begin, const_end), static_cast<ptrdiff_t>(handle.size()));

    // Test range-based for loop capability
    float test_value = 42.0f;
    for (auto& sample : handle) {
        sample = test_value;
    }

    // Verify values were written
    for (const auto& sample : const_handle) {
        EXPECT_EQ(sample, test_value);
    }
}

// Test move semantics
TEST_F(AudioBufferPoolTest, MoveSemantics) {
    auto handle1 = pool->acquire();
    ASSERT_TRUE(handle1.has_value());
    auto& handle = handle1.value();

    // Test move constructor
    auto moved_handle = std::move(handle);
    EXPECT_TRUE(moved_handle.valid());
    EXPECT_FALSE(handle.valid());  // Moved-from handle should be invalid

    // Test move assignment
    auto handle2 = pool->acquire();
    ASSERT_TRUE(handle2.has_value());
    auto& handle2_ref = handle2.value();

    moved_handle = std::move(handle2_ref);
    EXPECT_TRUE(moved_handle.valid());
    EXPECT_FALSE(handle2_ref.valid());
}

// Test statistics and reset
TEST_F(AudioBufferPoolTest, StatisticsAndReset) {
    // Check initial stats
    auto initial_stats = pool->getStats();
    EXPECT_EQ(initial_stats.total_buffers, 4);
    EXPECT_EQ(initial_stats.available_buffers, 4);
    EXPECT_EQ(initial_stats.peak_usage, 0);
    EXPECT_EQ(initial_stats.total_allocations, 0);
    EXPECT_EQ(initial_stats.failed_allocations, 0);

    // Acquire some buffers
    std::vector<AudioBufferPool::BufferHandle> handles;
    for (int i = 0; i < 3; ++i) {
        auto result = pool->acquire();
        ASSERT_TRUE(result.has_value());
        handles.push_back(std::move(result.value()));
    }

    // Check stats after acquisitions
    auto after_acquire_stats = pool->getStats();
    EXPECT_EQ(after_acquire_stats.available_buffers, 1);
    EXPECT_EQ(after_acquire_stats.peak_usage, 3);
    EXPECT_EQ(after_acquire_stats.total_allocations, 3);

    // Release all handles
    handles.clear();

    // Check stats after release
    auto after_release_stats = pool->getStats();
    EXPECT_EQ(after_release_stats.available_buffers, 4);
    EXPECT_EQ(after_release_stats.peak_usage, 3);  // Peak should remain
    EXPECT_EQ(after_release_stats.total_allocations, 3);

    // Test resetStats
    pool->resetStats();
    auto reset_stats = pool->getStats();
    EXPECT_EQ(reset_stats.peak_usage, 0);
    EXPECT_EQ(reset_stats.total_allocations, 0);
    EXPECT_EQ(reset_stats.failed_allocations, 0);
}

// Test available() method
TEST_F(AudioBufferPoolTest, AvailableMethod) {
    EXPECT_EQ(pool->available(), 4);

    auto handle1 = pool->acquire();
    ASSERT_TRUE(handle1.has_value());
    EXPECT_EQ(pool->available(), 3);

    auto handle2 = pool->acquire();
    ASSERT_TRUE(handle2.has_value());
    EXPECT_EQ(pool->available(), 2);

    // Release one by destroying the handle
    {
        auto tempHandle = std::move(handle1.value());
        // handle destroyed when tempHandle goes out of scope
    }
    EXPECT_EQ(pool->available(), 3);
}

// Test timeout behavior
TEST_F(AudioBufferPoolTest, TimeoutBehavior) {
    // Exhaust the pool
    std::vector<AudioBufferPool::BufferHandle> handles;
    for (int i = 0; i < 4; ++i) {
        auto result = pool->acquire();
        ASSERT_TRUE(result.has_value());
        handles.push_back(std::move(result.value()));
    }

    // Try to acquire with zero timeout - should fail immediately
    auto start_time = std::chrono::steady_clock::now();
    auto result = pool->tryAcquireFor(std::chrono::milliseconds(0));
    auto end_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::POOL_EXHAUSTED);

    // Should return quickly (within 10ms tolerance)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_LT(duration.count(), 10);

    // Test with small timeout
    start_time = std::chrono::steady_clock::now();
    result = pool->tryAcquireFor(std::chrono::milliseconds(50));
    end_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::POOL_EXHAUSTED);

    // Should wait approximately the timeout period (with tolerance)
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    EXPECT_GE(duration.count(), 45);   // At least 45ms
    EXPECT_LT(duration.count(), 100);  // But less than 100ms
}

// Test error conditions with factory method
TEST_F(AudioBufferPoolTest, FactoryErrorConditions) {
    // Test with invalid configuration
    AudioBufferPool::Config invalid_config;
    invalid_config.pool_size = 0;  // Invalid pool size

    auto result = AudioBufferPool::create(invalid_config);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::INVALID_CONFIGURATION);

    // Test with invalid buffer size
    invalid_config.pool_size = 4;
    invalid_config.buffer_size = 0;  // Invalid buffer size

    result = AudioBufferPool::create(invalid_config);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::INVALID_CONFIGURATION);
}
