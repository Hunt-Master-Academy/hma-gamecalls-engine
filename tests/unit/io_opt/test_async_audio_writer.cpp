// AsyncAudioWriter deterministic start/stop test (engine-free)
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/OptimizedAudioIO.h"

using namespace huntmaster;

namespace {
std::string make_temp_wav_path() {
    using clock = std::chrono::high_resolution_clock;
    auto now = clock::now().time_since_epoch().count();
    return std::string("/tmp/hm_async_writer_") + std::to_string(now) + ".wav";
}
}  // namespace

TEST(AsyncAudioWriter, StartWriteStopMetricsAndFile) {
    AsyncAudioWriter::Config cfg;
    cfg.maxQueuedWrites = 8;  // keep small for deterministic drain
    AsyncAudioWriter writer(cfg);

    const std::string path = make_temp_wav_path();
    ASSERT_TRUE(writer.start(path, /*sampleRate*/ 44100, /*channels*/ 1, /*bitsPerSample*/ 32));
    EXPECT_TRUE(writer.isActive());
    EXPECT_EQ(writer.getQueueDepth(), 0u);

    // Prepare a small chunk of audio (frames == samples since 1 channel here)
    std::vector<float> data(2048, 0.25f);
    std::atomic<bool> callbackOk{false};
    ASSERT_TRUE(writer.writeAsync(data.data(), data.size(), [&](bool success, const std::string&) {
        callbackOk.store(success, std::memory_order_relaxed);
    }));

    // Wait briefly for the write queue to drain and callback to run
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (writer.getQueueDepth() == 0 && callbackOk.load(std::memory_order_relaxed)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Stop the writer (flush any remaining)
    EXPECT_TRUE(writer.stop(std::chrono::milliseconds(2000)));
    EXPECT_FALSE(writer.isActive());

    // Validate metrics
    IOPerformanceMetrics m = writer.getMetrics();
    EXPECT_GE(m.writeOperations, 1u);
    EXPECT_GT(m.bytesWritten, 0u);

    // Validate file exists and has content
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(f.good());
    auto size = static_cast<long>(f.tellg());
    // WAV header is typically 44 bytes; ensure larger than header
    EXPECT_GT(size, 44);
    f.close();

    // Cleanup
    (void)std::remove(path.c_str());
}

// TODO: Expand AsyncAudioWriter testing with comprehensive scenarios:
// [ ] Test multiple concurrent writes with different data sizes
// [ ] Test queue overflow behavior and backpressure handling
// [ ] Test different audio formats (sample rates, channels, bit depths)
// [ ] Test error conditions (disk full, permission denied, invalid paths)
// [ ] Test performance under high-throughput sustained writing
// [ ] Test callback timing and error reporting accuracy
// [ ] Test graceful shutdown with pending writes in queue
// [ ] Test integration with real-time audio streaming scenarios
// [ ] Test memory usage patterns with large audio buffers
// [ ] Test cross-platform file system compatibility
