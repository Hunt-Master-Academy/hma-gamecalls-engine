#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/AdvancedIOOptimizer.h"

#ifdef __linux__
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

using namespace huntmaster::io;

namespace {

std::string makeTempDir() {
    auto base = std::filesystem::temp_directory_path() / "hm_ioopt_tests";
    std::filesystem::create_directories(base);
    return base.string();
}

}  // namespace

TEST(StorageAnalyzerTest, AnalyzeStorageBasic) {
    std::string tmpDir = makeTempDir();
    std::string testPath = tmpDir + "/dummy.dat";
    // We only need the parent to exist; file not required for analyzeStorage.
    StorageAnalyzer analyzer;
    auto characteristics = analyzer.analyzeStorage(testPath);
    // Should always return a non-zero optimal block size if underlying FS queried, or default 0.
    // Just assert that defaults got populated sensibly for throughput numbers.
    EXPECT_GT(characteristics.sequentialThroughputMBps, 0.0);
    EXPECT_GT(characteristics.randomThroughputMBps, 0.0);
    // Optimization suggestions should produce sane values
    auto suggestions = StorageAnalyzer::getStorageOptimizations(characteristics);
    EXPECT_GT(suggestions.recommendedBufferSize, 0u);
    EXPECT_GE(suggestions.recommendedThreadCount, 1u);
}

TEST(StorageAnalyzerTest, OptimizationProfilesDistinct) {
    StorageCharacteristics nvme;
    nvme.deviceType = StorageCharacteristics::DeviceType::SSD_NVME;
    auto nvmeSug = StorageAnalyzer::getStorageOptimizations(nvme);

    StorageCharacteristics hdd;
    hdd.deviceType = StorageCharacteristics::DeviceType::HDD;
    auto hddSug = StorageAnalyzer::getStorageOptimizations(hdd);

    EXPECT_GT(nvmeSug.recommendedThreadCount, hddSug.recommendedThreadCount);
    EXPECT_LT(nvmeSug.recommendedBufferSize, hddSug.recommendedBufferSize);
    EXPECT_TRUE(hddSug.enableCompression);
    EXPECT_FALSE(nvmeSug.enableCompression);
}

TEST(NUMAAudioAllocatorTest, BasicAllocation) {
    NUMAAudioAllocator allocator;
    size_t samples = 256;
    auto buf = allocator.allocateBuffer(samples);
    ASSERT_TRUE(buf != nullptr);
    for (size_t i = 0; i < samples; ++i) {
        buf[i] = static_cast<float>(i);
    }
    // Simple integrity check
    for (size_t i = 0; i < samples; ++i) {
        ASSERT_FLOAT_EQ(buf[i], static_cast<float>(i));
    }
}

TEST(AdaptiveBufferManagerTest, GrowthAndShrink) {
    AdaptiveBufferManager::BufferConfig cfg;
    cfg.initialSizeBytes = 4096;
    cfg.minSizeBytes = 1024;
    cfg.maxSizeBytes = 16384;
    cfg.growthFactor = 2.0;
    cfg.growthThreshold = 0.80;
    cfg.shrinkThreshold = 0.20;
    cfg.adaptationInterval = std::chrono::milliseconds(0);

    AdaptiveBufferManager mgr(cfg);

    // Record high utilization samples ( > 0.8 )
    for (int i = 0; i < 5; ++i) {
        mgr.recordUtilization(90, 100, std::chrono::microseconds(10));
    }
    mgr.recalculateOptimalSize();
    auto statsHigh = mgr.getStats();
    EXPECT_GT(statsHigh.currentOptimalSize, cfg.initialSizeBytes);

    // Record low utilization (< 0.2)
    for (int i = 0; i < 5; ++i) {
        mgr.recordUtilization(5, 100, std::chrono::microseconds(10));
    }
    mgr.recalculateOptimalSize();
    auto statsLow = mgr.getStats();
    EXPECT_LT(statsLow.currentOptimalSize, statsHigh.currentOptimalSize);
    EXPECT_GE(statsLow.currentOptimalSize, cfg.minSizeBytes);
}

TEST(AdaptiveBufferManagerTest, BufferAcquisitionMatchesMinSamples) {
    AdaptiveBufferManager::BufferConfig cfg;
    cfg.initialSizeBytes = 2048;
    cfg.minSizeBytes = 1024;
    cfg.maxSizeBytes = 8192;
    cfg.growthFactor = 2.0;
    cfg.growthThreshold = 0.9;
    cfg.shrinkThreshold = 0.1;
    cfg.adaptationInterval = std::chrono::milliseconds(0);

    AdaptiveBufferManager mgr(cfg);
    size_t actualSamples = 0;
    size_t requestSamples = 300;  // samples
    auto buffer = mgr.getBuffer(requestSamples, actualSamples);
    ASSERT_TRUE(buffer != nullptr);
    // currentOptimalSize_ = initialSizeBytes -> 2048 bytes -> 2048 / 4 = 512 samples
    EXPECT_GE(actualSamples, requestSamples);
    EXPECT_EQ(actualSamples, cfg.initialSizeBytes / sizeof(float));
}

#ifdef __linux__
TEST(AdvancedAsyncIOTest, AsyncReadWrite) {
    std::string tmpDir = makeTempDir();
    std::string filePath = tmpDir + "/async_io.bin";

    // Create file and open
    int fd = ::open(filePath.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    ASSERT_GE(fd, 0);

    AdvancedAsyncIO::Config cfg;
    cfg.workerThreads = 2;
    cfg.preferredEngine = AdvancedAsyncIO::Engine::AUTO_DETECT;
    AdvancedAsyncIO aio(cfg);
    ASSERT_TRUE(aio.initialize());

    const char writeData[] = "HelloAdvancedAsyncIO";
    size_t dataSize = sizeof(writeData);

    std::mutex m;
    std::condition_variable cv;
    bool writeDone = false;
    bool writeSuccess = false;
    size_t writeBytes = 0;

    ASSERT_TRUE(aio.writeAsync(
        fd, writeData, dataSize, 0, [&](bool success, size_t bytes, std::chrono::nanoseconds) {
            std::lock_guard<std::mutex> lk(m);
            writeSuccess = success;
            writeBytes = bytes;
            writeDone = true;
            cv.notify_one();
        }));

    {
        std::unique_lock<std::mutex> lk(m);
        ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&] { return writeDone; }));
    }

    EXPECT_TRUE(writeSuccess);
    EXPECT_EQ(writeBytes, dataSize);

    // Prepare read
    std::vector<char> readBuf(dataSize, 0);
    bool readDone = false;
    bool readSuccess = false;
    size_t readBytes = 0;

    ASSERT_TRUE(aio.readAsync(
        fd, readBuf.data(), dataSize, 0, [&](bool success, size_t bytes, std::chrono::nanoseconds) {
            std::lock_guard<std::mutex> lk(m);
            readSuccess = success;
            readBytes = bytes;
            readDone = true;
            cv.notify_one();
        }));

    {
        std::unique_lock<std::mutex> lk(m);
        ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&] { return readDone; }));
    }

    EXPECT_TRUE(readSuccess);
    EXPECT_EQ(readBytes, dataSize);
    EXPECT_EQ(std::memcmp(readBuf.data(), writeData, dataSize), 0);

    auto metrics = aio.getMetrics();
    // After operations, minLatency should be <= maxLatency (unless uninitialized defaults)
    EXPECT_LE(metrics.minLatency.count(), metrics.maxLatency.count());

    aio.shutdown();
    ::close(fd);
}

TEST(AdvancedAsyncIOTest, VectoredIOReadWrite) {
    std::string tmpDir = makeTempDir();
    std::string filePath = tmpDir + "/async_vectored.bin";
    int fd = ::open(filePath.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    ASSERT_GE(fd, 0);

    AdvancedAsyncIO aio;
    ASSERT_TRUE(aio.initialize());

    std::string part1 = "VECTOR_";
    std::string part2 = "WRITE_TEST";
    std::vector<char> buf1(part1.begin(), part1.end());
    std::vector<char> buf2(part2.begin(), part2.end());

    std::vector<iovec> writeVec(2);
    writeVec[0].iov_base = buf1.data();
    writeVec[0].iov_len = buf1.size();
    writeVec[1].iov_base = buf2.data();
    writeVec[1].iov_len = buf2.size();

    std::mutex m;
    std::condition_variable cv;
    bool writeDone = false;
    bool writeSuccess = false;
    size_t writeBytes = 0;

    ASSERT_TRUE(aio.vectoredIO(
        fd, writeVec, 0, true, [&](bool success, size_t bytes, std::chrono::nanoseconds) {
            std::lock_guard<std::mutex> lk(m);
            writeSuccess = success;
            writeBytes = bytes;
            writeDone = true;
            cv.notify_one();
        }));

    {
        std::unique_lock<std::mutex> lk(m);
        ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&] { return writeDone; }));
    }
    EXPECT_TRUE(writeSuccess);
    EXPECT_EQ(writeBytes, part1.size() + part2.size());

    // Read vectored
    std::vector<char> r1(buf1.size(), 0), r2(buf2.size(), 0);
    std::vector<iovec> readVec(2);
    readVec[0].iov_base = r1.data();
    readVec[0].iov_len = r1.size();
    readVec[1].iov_base = r2.data();
    readVec[1].iov_len = r2.size();

    bool readDone = false;
    bool readSuccess = false;
    size_t readBytes = 0;

    ASSERT_TRUE(aio.vectoredIO(
        fd, readVec, 0, false, [&](bool success, size_t bytes, std::chrono::nanoseconds) {
            std::lock_guard<std::mutex> lk(m);
            readSuccess = success;
            readBytes = bytes;
            readDone = true;
            cv.notify_one();
        }));

    {
        std::unique_lock<std::mutex> lk(m);
        ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&] { return readDone; }));
    }
    EXPECT_TRUE(readSuccess);
    EXPECT_EQ(readBytes, part1.size() + part2.size());
    EXPECT_EQ(std::string(r1.begin(), r1.end()), part1);
    EXPECT_EQ(std::string(r2.begin(), r2.end()), part2);

    aio.shutdown();
    ::close(fd);
}
#endif  // __linux__

TEST(CompressionPipelineTest, RoundTripStub) {
    CompressionPipeline::Config cfg;
    CompressionPipeline pipeline(cfg);

    std::vector<float> audio(256);
    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] = std::sin(static_cast<float>(i) * 0.01f);
    }

    auto comp = pipeline.compress(audio.data(), audio.size(), 1, 44100);
    ASSERT_TRUE(comp.success);
    ASSERT_EQ(comp.compressedData.size(), audio.size() * sizeof(float));

    auto decomp = pipeline.decompress(comp.compressedData.data(), comp.compressedData.size());
    ASSERT_TRUE(decomp.success);
    ASSERT_EQ(decomp.audioData.size(), audio.size());
    for (size_t i = 0; i < audio.size(); ++i) {
        ASSERT_FLOAT_EQ(decomp.audioData[i], audio[i]);
    }
    EXPECT_EQ(decomp.channels, 1);
    EXPECT_EQ(decomp.sampleRate, 44100u);
}

TEST(MasterIOOptimizerTest, BasicLifecycle) {
    MasterIOOptimizer optimizer;
    EXPECT_TRUE(optimizer.initialize());
    auto report = optimizer.getSystemReport();
    EXPECT_GE(report.overallHealthScore, 0.0);
    EXPECT_LE(report.overallHealthScore, 1.0);
    EXPECT_TRUE(optimizer.autoTune());
    EXPECT_TRUE(optimizer.exportSettings("ignored.cfg"));
    EXPECT_TRUE(optimizer.importSettings("ignored.cfg"));
}
