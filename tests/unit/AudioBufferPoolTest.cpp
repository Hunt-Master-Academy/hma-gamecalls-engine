#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "core/AudioBufferPool.h"

using namespace huntmaster;

class AudioBufferPoolTest : public ::testing::Test {
   protected:
    void SetUp() override { pool = std::make_unique<AudioBufferPool>(4, 1024); }

    std::unique_ptr<AudioBufferPool> pool;
};

TEST_F(AudioBufferPoolTest, BasicAcquireRelease) {
    auto handle_result = pool->acquire();
    ASSERT_TRUE(handle_result.has_value());

    auto& handle = handle_result.value();
    EXPECT_EQ(handle.size(), 1024 / sizeof(float));

    // Write some data
    std::fill(handle.begin(), handle.end(), 1.0f);

    // Handle automatically releases when it goes out of scope
}

TEST_F(AudioBufferPoolTest, ExhaustPool) {
    std::vector<AudioBufferPool::BufferHandle> handles;

    // Acquire all buffers
    for (int i = 0; i < 4; ++i) {
        auto result = pool->acquire();
        ASSERT_TRUE(result.has_value());
        handles.push_back(std::move(result.value()));
    }

    // Pool should be exhausted
    auto result = pool->acquire();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), BufferPoolError::POOL_EXHAUSTED);

    // Release one
    handles.pop_back();

    // Should be able to acquire again
    result = pool->acquire();
    EXPECT_TRUE(result.has_value());
}

TEST_F(AudioBufferPoolTest, ThreadSafety) {
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::atomic<int> success_count{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &success_count]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                auto result = pool->tryAcquireFor(std::chrono::milliseconds(10));
                if (result.has_value()) {
                    success_count++;
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    // Buffer automatically released
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should have processed many operations successfully
    EXPECT_GT(success_count.load(), 0);

    // Pool should still be functional
    auto stats = pool->getStats();
    EXPECT_EQ(stats.total_buffers, 4);
}