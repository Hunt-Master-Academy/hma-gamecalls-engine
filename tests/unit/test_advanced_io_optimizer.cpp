/**
 * @file test_advanced_io_optimizer.cpp
 * @brief Comprehensive tests for advanced I/O optimization features
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "huntmaster/core/AdvancedIOOptimizer.h"

using namespace huntmaster::io;
using namespace testing;

class AdvancedIOOptimizerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create temporary directory for tests
        testDir_ = std::filesystem::temp_directory_path() / "huntmaster_io_test";
        std::filesystem::create_directories(testDir_);

        // Create test files
        createTestFiles();
    }

    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
    }

    void createTestFiles() {
        // Create a small test file
        smallTestFile_ = testDir_ / "small_test.wav";
        createTestAudioFile(smallTestFile_, 1 * 1024 * 1024);  // 1MB

        // Create a large test file
        largeTestFile_ = testDir_ / "large_test.wav";
        createTestAudioFile(largeTestFile_, 50 * 1024 * 1024);  // 50MB
    }

    void createTestAudioFile(const std::filesystem::path& path, size_t sizeBytes) {
        std::ofstream file(path, std::ios::binary);

        // Write simple WAV header (44 bytes)
        const char wavHeader[44] = {
            'R',  'I',  'F',  'F',  // ChunkID
            0,    0,    0,    0,    // ChunkSize (will be filled)
            'W',  'A',  'V',  'E',  // Format
            'f',  'm',  't',  ' ',  // Subchunk1ID
            16,   0,    0,    0,    // Subchunk1Size
            1,    0,                // AudioFormat (PCM)
            2,    0,                // NumChannels (stereo)
            0x44, 0xAC, 0,    0,    // SampleRate (44100)
            0x10, 0xB1, 0x02, 0,    // ByteRate
            4,    0,                // BlockAlign
            16,   0,                // BitsPerSample
            'd',  'a',  't',  'a',  // Subchunk2ID
            0,    0,    0,    0     // Subchunk2Size (will be filled)
        };

        file.write(wavHeader, 44);

        // Fill rest with random audio data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);

        for (size_t i = 44; i < sizeBytes; ++i) {
            uint8_t byte = dis(gen);
            file.write(reinterpret_cast<const char*>(&byte), 1);
        }
    }

    std::filesystem::path testDir_;
    std::filesystem::path smallTestFile_;
    std::filesystem::path largeTestFile_;
};

// ============================================================================
// StorageAnalyzer Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, StorageAnalyzerDetectsCharacteristics) {
    auto characteristics = StorageAnalyzer::analyzeStorage(testDir_.string());

    // Should detect some storage type
    EXPECT_NE(characteristics.deviceType, StorageCharacteristics::DeviceType::UNKNOWN);

    // Should have reasonable values
    EXPECT_GT(characteristics.optimalBlockSize, 0);
    EXPECT_GT(characteristics.sequentialThroughputMBps, 0.0);
    EXPECT_GT(characteristics.maxConcurrentOps, 0);
}

TEST_F(AdvancedIOOptimizerTest, StorageAnalyzerBenchmarkWorks) {
    auto characteristics =
        StorageAnalyzer::benchmarkStorage(testDir_.string(), 10);  // 10MB benchmark

    // Benchmark should produce meaningful results
    EXPECT_GT(characteristics.sequentialThroughputMBps, 1.0);  // At least 1 MB/s
    EXPECT_GT(characteristics.averageLatencyUs, 0.0);
}

TEST_F(AdvancedIOOptimizerTest, StorageOptimizationsAreReasonable) {
    auto characteristics = StorageAnalyzer::analyzeStorage(testDir_.string());
    auto suggestions = StorageAnalyzer::getStorageOptimizations(characteristics);

    // Buffer size should be reasonable
    EXPECT_GE(suggestions.recommendedBufferSize, 4 * 1024);          // At least 4KB
    EXPECT_LE(suggestions.recommendedBufferSize, 16 * 1024 * 1024);  // At most 16MB

    // Cache size should be reasonable
    EXPECT_GE(suggestions.recommendedCacheSize, 1 * 1024 * 1024);     // At least 1MB
    EXPECT_LE(suggestions.recommendedCacheSize, 1024 * 1024 * 1024);  // At most 1GB

    // Thread count should be reasonable
    EXPECT_GE(suggestions.recommendedThreadCount, 1u);
    EXPECT_LE(suggestions.recommendedThreadCount, std::thread::hardware_concurrency() * 2);
}

// ============================================================================
// NUMAAudioAllocator Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, NUMAAudioAllocatorBasicFunctionality) {
    NUMAAudioAllocator allocator;

    // Should detect topology
    auto topology = allocator.getTopology();
    EXPECT_GT(topology.nodes.size(), 0);

    // Should be able to get optimal node
    uint32_t optimalNode = allocator.getOptimalNode();
    EXPECT_GE(optimalNode, 0);
    EXPECT_LT(optimalNode, topology.nodes.size());
}

TEST_F(AdvancedIOOptimizerTest, NUMAAudioAllocatorBufferAllocation) {
    NUMAAudioAllocator allocator;

    const size_t bufferSize = 48000 * 2;  // 1 second stereo at 48kHz
    auto buffer = allocator.allocateBuffer(bufferSize);

    ASSERT_NE(buffer, nullptr);

    // Should be able to write to the buffer
    buffer[0] = 1.0f;
    buffer[bufferSize - 1] = -1.0f;

    EXPECT_EQ(buffer[0], 1.0f);
    EXPECT_EQ(buffer[bufferSize - 1], -1.0f);
}

TEST_F(AdvancedIOOptimizerTest, NUMAAudioAllocatorMultipleAllocations) {
    NUMAAudioAllocator allocator;

    std::vector<std::unique_ptr<float[], std::function<void(float*)>>> buffers;

    // Allocate multiple buffers
    for (int i = 0; i < 10; ++i) {
        size_t bufferSize = 1024 * (i + 1);
        auto buffer = allocator.allocateBuffer(bufferSize);
        ASSERT_NE(buffer, nullptr);
        buffers.push_back(std::move(buffer));
    }

    // All buffers should be valid and independent
    for (size_t i = 0; i < buffers.size(); ++i) {
        buffers[i][0] = static_cast<float>(i);
    }

    for (size_t i = 0; i < buffers.size(); ++i) {
        EXPECT_EQ(buffers[i][0], static_cast<float>(i));
    }
}

// ============================================================================
// AdaptiveBufferManager Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, AdaptiveBufferManagerInitialization) {
    AdaptiveBufferManager::BufferConfig config;
    config.initialSizeBytes = 64 * 1024;
    config.minSizeBytes = 16 * 1024;
    config.maxSizeBytes = 1024 * 1024;

    AdaptiveBufferManager manager(config);

    auto stats = manager.getStats();
    EXPECT_EQ(stats.currentOptimalSize, config.initialSizeBytes);
    EXPECT_EQ(stats.totalBuffersAllocated, 0);
    EXPECT_EQ(stats.adaptationCount, 0);
}

TEST_F(AdvancedIOOptimizerTest, AdaptiveBufferManagerBufferAllocation) {
    AdaptiveBufferManager manager;

    size_t actualSamples;
    auto buffer = manager.getBuffer(16384, actualSamples);  // Request 64KB

    ASSERT_NE(buffer, nullptr);
    EXPECT_GE(actualSamples, 16384);

    auto stats = manager.getStats();
    EXPECT_EQ(stats.totalBuffersAllocated, 1);
}

TEST_F(AdvancedIOOptimizerTest, AdaptiveBufferManagerAdaptation) {
    AdaptiveBufferManager::BufferConfig config;
    config.adaptationInterval = std::chrono::milliseconds(10);  // Very fast adaptation for testing
    config.growthThreshold = 0.8;
    config.shrinkThreshold = 0.3;

    AdaptiveBufferManager manager(config);

    // Record high utilization to trigger growth
    for (int i = 0; i < 10; ++i) {
        manager.recordUtilization(
            8192, 10240, std::chrono::nanoseconds(100000));  // 80% utilization
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    auto statsAfterGrowth = manager.getStats();
    EXPECT_GT(statsAfterGrowth.currentOptimalSize, config.initialSizeBytes);
    EXPECT_GT(statsAfterGrowth.adaptationCount, 0);
    EXPECT_NEAR(statsAfterGrowth.averageUtilization, 0.8, 0.1);

    // Record low utilization to trigger shrinking
    for (int i = 0; i < 10; ++i) {
        manager.recordUtilization(2048, 10240, std::chrono::nanoseconds(50000));  // 20% utilization
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    auto statsAfterShrink = manager.getStats();
    EXPECT_LT(statsAfterShrink.currentOptimalSize, statsAfterGrowth.currentOptimalSize);
    EXPECT_GT(statsAfterShrink.adaptationCount, statsAfterGrowth.adaptationCount);
}

// ============================================================================
// AdvancedAsyncIO Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, AdvancedAsyncIOInitialization) {
    AdvancedAsyncIO::Config config;
    config.queueDepth = 32;
    config.enableBatching = true;

    AdvancedAsyncIO asyncIO(config);

    EXPECT_TRUE(asyncIO.initialize());
    EXPECT_NE(asyncIO.getActiveEngine(), AdvancedAsyncIO::Engine::AUTO_DETECT);

    asyncIO.shutdown();
}

TEST_F(AdvancedIOOptimizerTest, AdvancedAsyncIOReadWrite) {
    AdvancedAsyncIO asyncIO;
    ASSERT_TRUE(asyncIO.initialize());

    // Create test file
    std::string testFile = (testDir_ / "async_test.tmp").string();

    // Test data
    const size_t dataSize = 4096;
    std::vector<char> writeData(dataSize, 'T');
    std::vector<char> readData(dataSize, 0);

    // Open file for writing
    int fd = open(testFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_GE(fd, 0);

    // Test async write
    std::atomic<bool> writeCompleted{false};
    std::atomic<bool> writeSuccess{false};

    bool writeSubmitted = asyncIO.writeAsync(
        fd,
        writeData.data(),
        writeData.size(),
        0,
        [&](bool success, size_t bytesTransferred, std::chrono::nanoseconds latency) {
            writeSuccess = success && (bytesTransferred == dataSize);
            writeCompleted = true;
        });

    EXPECT_TRUE(writeSubmitted);

    // Wait for write completion
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!writeCompleted && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(writeCompleted);
    EXPECT_TRUE(writeSuccess);

    // Test async read
    std::atomic<bool> readCompleted{false};
    std::atomic<bool> readSuccess{false};

    bool readSubmitted = asyncIO.readAsync(
        fd,
        readData.data(),
        readData.size(),
        0,
        [&](bool success, size_t bytesTransferred, std::chrono::nanoseconds latency) {
            readSuccess = success && (bytesTransferred == dataSize);
            readCompleted = true;
        });

    EXPECT_TRUE(readSubmitted);

    // Wait for read completion
    timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!readCompleted && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(readCompleted);
    EXPECT_TRUE(readSuccess);

    // Verify data
    EXPECT_EQ(writeData, readData);

    close(fd);
    asyncIO.shutdown();
    std::filesystem::remove(testFile);
}

TEST_F(AdvancedIOOptimizerTest, AdvancedAsyncIOMetrics) {
    AdvancedAsyncIO asyncIO;
    ASSERT_TRUE(asyncIO.initialize());

    // Perform some I/O operations to generate metrics
    std::string testFile = (testDir_ / "metrics_test.tmp").string();
    int fd = open(testFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_GE(fd, 0);

    std::vector<char> testData(1024, 'M');
    std::atomic<int> completedOps{0};

    // Submit multiple operations
    for (int i = 0; i < 10; ++i) {
        asyncIO.writeAsync(fd,
                           testData.data(),
                           testData.size(),
                           i * testData.size(),
                           [&](bool success,
                               size_t bytesTransferred,
                               std::chrono::nanoseconds latency) { completedOps++; });
    }

    // Wait for completion
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (completedOps.load() < 10 && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedOps.load(), 10);

    // Check metrics
    auto metrics = asyncIO.getMetrics();
    EXPECT_GT(metrics.maxLatency.count(), 0);
    EXPECT_LT(metrics.minLatency, metrics.maxLatency);

    close(fd);
    asyncIO.shutdown();
    std::filesystem::remove(testFile);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, SystemIntegrationTest) {
    // Test that all components work together
    MasterIOOptimizer::OptimizationProfile profile;
    profile.workloadType =
        MasterIOOptimizer::OptimizationProfile::WorkloadType::INTERACTIVE_PLAYBACK;
    profile.maxLatency = std::chrono::microseconds(10000);
    profile.minThroughputMBps = 50.0;

    MasterIOOptimizer optimizer(profile);
    EXPECT_TRUE(optimizer.initialize());

    // Optimize for test directory
    auto optimizedHandle = optimizer.optimizeForPath(testDir_.string());
    EXPECT_NE(optimizedHandle, nullptr);

    if (optimizedHandle) {
        // Verify optimization components are created
        EXPECT_NE(optimizedHandle->asyncIO, nullptr);
        EXPECT_NE(optimizedHandle->bufferManager, nullptr);

        // Verify storage characteristics are detected
        EXPECT_NE(optimizedHandle->storageInfo.deviceType,
                  StorageCharacteristics::DeviceType::UNKNOWN);

        // Verify suggestions are reasonable
        EXPECT_GT(optimizedHandle->suggestions.recommendedBufferSize, 0);
        EXPECT_GT(optimizedHandle->suggestions.recommendedCacheSize, 0);
        EXPECT_GT(optimizedHandle->suggestions.recommendedThreadCount, 0);
    }

    // Get system report
    auto systemReport = optimizer.getSystemReport();
    EXPECT_GE(systemReport.overallHealthScore, 0.0);
    EXPECT_LE(systemReport.overallHealthScore, 1.0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, PerformanceComparisonTest) {
    // Compare optimized vs unoptimized I/O performance
    const size_t testDataSize = 10 * 1024 * 1024;  // 10MB
    std::vector<char> testData(testDataSize, 'P');

    // Test unoptimized I/O
    auto start = std::chrono::high_resolution_clock::now();
    {
        std::string testFile = (testDir_ / "unoptimized_test.tmp").string();
        std::ofstream file(testFile, std::ios::binary);
        file.write(testData.data(), testData.size());
        file.close();
        std::filesystem::remove(testFile);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto unoptimizedTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Test optimized I/O
    start = std::chrono::high_resolution_clock::now();
    {
        AdvancedAsyncIO asyncIO;
        ASSERT_TRUE(asyncIO.initialize());

        std::string testFile = (testDir_ / "optimized_test.tmp").string();
        int fd = open(testFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        ASSERT_GE(fd, 0);

        std::atomic<bool> completed{false};
        asyncIO.writeAsync(fd,
                           testData.data(),
                           testData.size(),
                           0,
                           [&](bool success,
                               size_t bytesTransferred,
                               std::chrono::nanoseconds latency) { completed = true; });

        // Wait for completion
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!completed && std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        close(fd);
        asyncIO.shutdown();
        std::filesystem::remove(testFile);
    }
    end = std::chrono::high_resolution_clock::now();
    auto optimizedTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Optimized should be at least as fast (allowing for test variability)
    EXPECT_LE(optimizedTime.count(), unoptimizedTime.count() * 2);

    std::cout << "Performance comparison:" << std::endl;
    std::cout << "  Unoptimized: " << unoptimizedTime.count() << " μs" << std::endl;
    std::cout << "  Optimized: " << optimizedTime.count() << " μs" << std::endl;
    std::cout << "  Improvement: " << std::fixed << std::setprecision(2)
              << (static_cast<double>(unoptimizedTime.count()) / optimizedTime.count()) << "x"
              << std::endl;
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(AdvancedIOOptimizerTest, ErrorHandlingTest) {
    // Test error handling for invalid paths
    auto characteristics = StorageAnalyzer::analyzeStorage("/nonexistent/path");
    EXPECT_EQ(characteristics.deviceType, StorageCharacteristics::DeviceType::UNKNOWN);

    // Test error handling for invalid allocations
    NUMAAudioAllocator allocator;
    auto buffer = allocator.allocateBuffer(SIZE_MAX);  // Huge allocation should fail gracefully
    EXPECT_EQ(buffer, nullptr);

    // Test error handling for invalid async I/O
    AdvancedAsyncIO asyncIO;
    ASSERT_TRUE(asyncIO.initialize());

    std::vector<char> testData(1024);
    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    // Try to write to invalid file descriptor
    bool submitted = asyncIO.writeAsync(
        -1,
        testData.data(),
        testData.size(),
        0,
        [&](bool operationSuccess, size_t bytesTransferred, std::chrono::nanoseconds latency) {
            success = operationSuccess;
            completed = true;
        });

    if (submitted) {
        // Wait for completion
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!completed && std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_TRUE(completed);
        EXPECT_FALSE(success);  // Should fail due to invalid fd
    }

    asyncIO.shutdown();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
