/**
 * @file AdvancedIOOptimizer.h
 * @brief Advanced I/O optimization features for the Huntmaster Audio Engine
 *
 * This file provides next-generation I/O optimization features including:
 * - Adaptive buffer management with system performance monitoring
 * - Storage device detection and optimization
 * - Advanced async I/O with io_uring support
 * - NUMA-aware memory allocation
 * - Intelligent compression pipelines
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date January 2025
 */

#pragma once

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// System headers for I/O and NUMA operations
#ifdef __linux__
#include <sys/uio.h>  // For iovec
#ifdef HAVE_NUMA
#include <numa.h>  // For NUMA support
#endif
#endif

#include "OptimizedAudioIO.h"

// Compatibility layer for non-Linux systems
#ifndef __linux__
struct iovec {
    void* iov_base;  // Base address
    size_t iov_len;  // Number of bytes
};
#endif

namespace huntmaster {
namespace io {

/**
 * @brief System storage characteristics for optimization
 */
struct StorageCharacteristics {
    enum class DeviceType {
        UNKNOWN,
        HDD,              // Traditional spinning disk
        SSD_SATA,         // SATA SSD
        SSD_NVME,         // NVMe SSD
        NETWORK_STORAGE,  // Network-attached storage
        MEMORY_DISK       // RAM disk or tmpfs
    };

    DeviceType deviceType = DeviceType::UNKNOWN;
    size_t optimalBlockSize = 4096;  // Optimal I/O block size
    size_t maxConcurrentOps = 32;    // Max concurrent operations
    double sequentialThroughputMBps = 0.0;
    double randomThroughputMBps = 0.0;
    double averageLatencyUs = 0.0;
    bool supportsDirectIO = false;
    bool supportsTrim = false;
};

/**
 * @brief NUMA topology information for memory optimization
 */
struct NUMATopology {
    struct Node {
        uint32_t nodeId;
        std::vector<uint32_t> cpuIds;
        size_t memoryMB;
        double memoryBandwidthGBps;
    };

    std::vector<Node> nodes;
    bool isNUMASystem = false;
    uint32_t currentNode = 0;
};

/**
 * @brief Advanced I/O performance metrics with detailed analytics
 */
struct AdvancedIOMetrics {
    // Basic metrics
    IOPerformanceMetrics basic;

    // Advanced timing metrics
    std::chrono::nanoseconds minLatency{std::chrono::nanoseconds::max()};
    std::chrono::nanoseconds maxLatency{0};
    std::chrono::nanoseconds p50Latency{0};  // 50th percentile
    std::chrono::nanoseconds p95Latency{0};  // 95th percentile
    std::chrono::nanoseconds p99Latency{0};  // 99th percentile

    // Queue depth metrics
    double avgQueueDepth = 0.0;
    size_t maxQueueDepth = 0;

    // Efficiency metrics
    double cpuEfficiency = 0.0;      // CPU cycles per byte
    double memoryEfficiency = 0.0;   // Memory bandwidth utilization
    double storageEfficiency = 0.0;  // Storage bandwidth utilization

    // Error tracking
    size_t retryCount = 0;
    size_t errorCount = 0;
    std::vector<std::string> errorTypes;
};

/**
 * @brief Storage device detector and analyzer
 */
class StorageAnalyzer {
  public:
    /**
     * @brief Detect storage characteristics for a given path
     */
    static StorageCharacteristics analyzeStorage(const std::string& path);

    /**
     * @brief Benchmark storage performance
     */
    static StorageCharacteristics benchmarkStorage(const std::string& path,
                                                   size_t testSizeMB = 100);

    /**
     * @brief Get optimal I/O parameters for detected storage
     */
    static IOPerformanceMonitor::OptimizationSuggestions
    getStorageOptimizations(const StorageCharacteristics& storage);

  private:
    /**
     * @brief Detect Linux-specific storage type
     */
    static StorageCharacteristics::DeviceType
    detectLinuxStorageType(const std::filesystem::path& path);
};

/**
 * @brief NUMA-aware memory allocator for audio buffers
 */
class NUMAAudioAllocator {
  private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
    NUMATopology topology_;
    mutable std::mutex allocationMutex_;

  public:
    NUMAAudioAllocator();
    ~NUMAAudioAllocator();

    /**
     * @brief Allocate NUMA-optimized audio buffer
     */
    std::unique_ptr<float[], std::function<void(float*)>>
    allocateBuffer(size_t sampleCount, uint32_t preferredNode = UINT32_MAX);

    /**
     * @brief Get optimal NUMA node for current thread
     */
    uint32_t getOptimalNode() const;

    /**
     * @brief Get system NUMA topology
     */
    const NUMATopology& getTopology() const {
        return topology_;
    }

  private:
    void detectTopology();
};

/**
 * @brief Adaptive buffer manager with performance monitoring
 */
class AdaptiveBufferManager {
  public:
    struct BufferConfig {
        size_t initialSizeBytes;
        size_t minSizeBytes;
        size_t maxSizeBytes;
        double growthFactor;
        double shrinkThreshold;
        double growthThreshold;
        std::chrono::milliseconds adaptationInterval;

        BufferConfig()
            : initialSizeBytes(64 * 1024)  // 64KB initial
              ,
              minSizeBytes(4 * 1024)  // 4KB minimum
              ,
              maxSizeBytes(16 * 1024 * 1024)  // 16MB maximum
              ,
              growthFactor(1.5)  // Growth multiplier
              ,
              shrinkThreshold(0.3)  // Shrink if utilization < 30%
              ,
              growthThreshold(0.8)  // Grow if utilization > 80%
              ,
              adaptationInterval(1000)  // 1 second
        {}
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    explicit AdaptiveBufferManager(const BufferConfig& config = BufferConfig());
    ~AdaptiveBufferManager();

    /**
     * @brief Get optimally-sized buffer for current conditions
     */
    std::unique_ptr<float[], std::function<void(float*)>> getBuffer(size_t minSamples,
                                                                    size_t& actualSamples);

    /**
     * @brief Record buffer utilization for adaptation
     */
    void recordUtilization(size_t actualUsed,
                           size_t bufferSize,
                           std::chrono::nanoseconds processingTime);

    /**
     * @brief Get current buffer statistics
     */
    struct BufferStats {
        size_t currentOptimalSize;
        double averageUtilization;
        size_t totalBuffersAllocated;
        size_t adaptationCount;
    };

    BufferStats getStats() const;

    /**
     * @brief Force buffer size recalculation
     */
    void recalculateOptimalSize();
};

/**
 * @brief Advanced async I/O engine with io_uring support (Linux)
 */
class AdvancedAsyncIO {
  public:
    enum class Engine {
        AUTO_DETECT,  // Automatically choose best available
        THREAD_POOL,  // Traditional thread pool
        IO_URING,     // Linux io_uring (if available)
        IOCP,         // Windows I/O Completion Ports
        EPOLL         // Linux epoll-based
    };

    struct Config {
        Engine preferredEngine = Engine::AUTO_DETECT;
        size_t queueDepth = 128;
        size_t workerThreads = 0;  // 0 = auto-detect
        bool enableBatching = true;
        size_t batchSize = 16;
        std::chrono::microseconds batchTimeout{100};

        Config() = default;
    };

    using CompletionCallback = std::function<void(
        bool success, size_t bytesTransferred, std::chrono::nanoseconds latency)>;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    AdvancedAsyncIO();
    explicit AdvancedAsyncIO(const Config& config);
    ~AdvancedAsyncIO();

    /**
     * @brief Initialize the async I/O engine
     */
    bool initialize();

    /**
     * @brief Shutdown the async I/O engine
     */
    void shutdown();

    /**
     * @brief Submit async read operation
     */
    bool readAsync(
        int fileDescriptor, void* buffer, size_t size, off_t offset, CompletionCallback callback);

    /**
     * @brief Submit async write operation
     */
    bool writeAsync(int fileDescriptor,
                    const void* buffer,
                    size_t size,
                    off_t offset,
                    CompletionCallback callback);

    /**
     * @brief Submit vectored I/O operation
     */
    bool vectoredIO(int fileDescriptor,
                    const std::vector<iovec>& vectors,
                    off_t offset,
                    bool isWrite,
                    CompletionCallback callback);

    /**
     * @brief Get engine metrics
     */
    AdvancedIOMetrics getMetrics() const;

    /**
     * @brief Get currently active engine type
     */
    Engine getActiveEngine() const;
};

/**
 * @brief Intelligent compression pipeline for audio data
 */
class CompressionPipeline {
  public:
    enum class Algorithm {
        NONE,
        LZ4,            // Fast compression
        ZSTD_FAST,      // Zstandard fast mode
        ZSTD_BALANCED,  // Zstandard balanced
        ZSTD_BEST,      // Zstandard best compression
        FLAC_FAST,      // FLAC fast encoding
        FLAC_BEST       // FLAC best compression
    };

    struct Config {
        Algorithm algorithm = Algorithm::ZSTD_FAST;
        size_t blockSizeBytes = 64 * 1024;  // 64KB blocks
        size_t compressionLevel = 3;        // Algorithm-specific level
        bool enableParallelCompression = true;
        size_t maxParallelBlocks = 4;

        Config() = default;
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    CompressionPipeline();
    explicit CompressionPipeline(const Config& config);
    ~CompressionPipeline();

    /**
     * @brief Compress audio data
     */
    struct CompressionResult {
        std::vector<uint8_t> compressedData;
        double compressionRatio;
        std::chrono::microseconds compressionTime;
        bool success;
    };

    CompressionResult
    compress(const float* audioData, size_t sampleCount, uint16_t channels, uint32_t sampleRate);

    /**
     * @brief Decompress audio data
     */
    struct DecompressionResult {
        std::vector<float> audioData;
        uint16_t channels;
        uint32_t sampleRate;
        std::chrono::microseconds decompressionTime;
        bool success;
    };

    DecompressionResult decompress(const uint8_t* compressedData, size_t dataSize);

    /**
     * @brief Estimate compression ratio for given audio characteristics
     */
    double
    estimateCompressionRatio(uint16_t channels, uint32_t sampleRate, double durationSeconds) const;

    /**
     * @brief Get compression statistics
     */
    struct CompressionStats {
        size_t totalBytesInput;
        size_t totalBytesOutput;
        double averageCompressionRatio;
        std::chrono::microseconds totalCompressionTime;
        std::chrono::microseconds totalDecompressionTime;
    };

    CompressionStats getStats() const;
};

/**
 * @brief Master I/O optimizer that coordinates all optimization strategies
 */
class MasterIOOptimizer {
  public:
    struct OptimizationProfile {
        // Workload characteristics
        enum class WorkloadType {
            REAL_TIME_RECORDING,   // Low latency priority
            BATCH_PROCESSING,      // Throughput priority
            INTERACTIVE_PLAYBACK,  // Balanced latency/throughput
            ARCHIVE_STORAGE        // Compression priority
        };

        WorkloadType workloadType = WorkloadType::INTERACTIVE_PLAYBACK;

        // Performance targets
        std::chrono::microseconds maxLatency{10000};  // 10ms max
        double minThroughputMBps = 50.0;
        double maxCPUUsage = 0.8;     // 80% max CPU
        double maxMemoryUsage = 0.7;  // 70% max memory

        // Feature enablement
        bool enableCompression = false;
        bool enableCaching = true;
        bool enablePrefetch = true;
        bool enableNUMAOptimization = true;

        OptimizationProfile() = default;
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;

  public:
    MasterIOOptimizer();
    explicit MasterIOOptimizer(const OptimizationProfile& profile);
    ~MasterIOOptimizer();

    /**
     * @brief Initialize all optimization subsystems
     */
    bool initialize();

    /**
     * @brief Optimize I/O for a specific file path
     */
    struct OptimizedIOHandle {
        std::unique_ptr<AdvancedAsyncIO> asyncIO;
        std::unique_ptr<AdaptiveBufferManager> bufferManager;
        std::unique_ptr<CompressionPipeline> compression;
        StorageCharacteristics storageInfo;
        IOPerformanceMonitor::OptimizationSuggestions suggestions;
    };

    std::unique_ptr<OptimizedIOHandle> optimizeForPath(const std::string& path);

    /**
     * @brief Get system-wide I/O performance report
     */
    struct SystemIOReport {
        AdvancedIOMetrics aggregatedMetrics;
        std::vector<std::string> performanceWarnings;
        std::vector<std::string> optimizationRecommendations;
        double overallHealthScore;  // 0.0 - 1.0
    };

    SystemIOReport getSystemReport() const;

    /**
     * @brief Auto-tune all parameters based on current performance
     */
    bool autoTune();

    /**
     * @brief Export optimization settings for reuse
     */
    bool exportSettings(const std::string& filename) const;

    /**
     * @brief Import optimization settings
     */
    bool importSettings(const std::string& filename);
};

}  // namespace io
}  // namespace huntmaster
