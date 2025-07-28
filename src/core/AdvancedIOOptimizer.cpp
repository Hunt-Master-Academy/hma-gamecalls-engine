/**
 * @file AdvancedIOOptimizer.cpp
 * @brief Implementation of advanced I/O optimization features
 */

#include "huntmaster/core/AdvancedIOOptimizer.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include <unordered_map>

// Platform-specific includes
#ifdef __linux__
#include <linux/magic.h>
#include <sched.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/uio.h>
#include <unistd.h>
#ifdef HAVE_NUMA
#include <numa.h>
#include <numaif.h>
#endif
#ifdef HAVE_IO_URING
#include <liburing.h>
#endif
#endif

#ifdef _WIN32
#include <Windows.h>
#include <winioctl.h>
#endif

namespace huntmaster {
namespace io {

// ============================================================================
// StorageAnalyzer Implementation
// ============================================================================

StorageCharacteristics StorageAnalyzer::analyzeStorage(const std::string& path) {
    StorageCharacteristics characteristics;

    try {
        std::filesystem::path fsPath(path);
        if (!std::filesystem::exists(fsPath.parent_path())) {
            return characteristics;
        }

#ifdef __linux__
        struct statfs fs_info;
        if (statfs(fsPath.parent_path().c_str(), &fs_info) == 0) {
            // Detect file system type and storage characteristics
            switch (fs_info.f_type) {
                case EXT4_SUPER_MAGIC:
                case XFS_SUPER_MAGIC:
                    // Traditional file systems - likely HDD or SSD
                    characteristics.deviceType = detectLinuxStorageType(fsPath);
                    break;
                case TMPFS_MAGIC:
                    characteristics.deviceType = StorageCharacteristics::DeviceType::MEMORY_DISK;
                    characteristics.sequentialThroughputMBps = 10000.0;  // Very fast
                    characteristics.randomThroughputMBps = 8000.0;
                    characteristics.averageLatencyUs = 0.1;
                    break;
                default:
                    characteristics.deviceType = StorageCharacteristics::DeviceType::UNKNOWN;
            }

            characteristics.optimalBlockSize = fs_info.f_bsize;
        }
#endif

#ifdef _WIN32
        // Windows storage detection
        DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
        if (GetDiskFreeSpace(fsPath.parent_path().wstring().c_str(),
                             &sectorsPerCluster,
                             &bytesPerSector,
                             &freeClusters,
                             &totalClusters)) {
            characteristics.optimalBlockSize = sectorsPerCluster * bytesPerSector;
        }

        // Detect SSD vs HDD on Windows
        characteristics.deviceType = detectWindowsStorageType(fsPath);
#endif

        // Set reasonable defaults based on detected type
        switch (characteristics.deviceType) {
            case StorageCharacteristics::DeviceType::SSD_NVME:
                characteristics.sequentialThroughputMBps = 3500.0;
                characteristics.randomThroughputMBps = 650.0;
                characteristics.averageLatencyUs = 15.0;
                characteristics.maxConcurrentOps = 128;
                characteristics.supportsDirectIO = true;
                characteristics.supportsTrim = true;
                break;

            case StorageCharacteristics::DeviceType::SSD_SATA:
                characteristics.sequentialThroughputMBps = 550.0;
                characteristics.randomThroughputMBps = 90.0;
                characteristics.averageLatencyUs = 50.0;
                characteristics.maxConcurrentOps = 64;
                characteristics.supportsDirectIO = true;
                characteristics.supportsTrim = true;
                break;

            case StorageCharacteristics::DeviceType::HDD:
                characteristics.sequentialThroughputMBps = 150.0;
                characteristics.randomThroughputMBps = 2.0;
                characteristics.averageLatencyUs = 8000.0;
                characteristics.maxConcurrentOps = 4;
                characteristics.supportsDirectIO = true;
                characteristics.supportsTrim = false;
                break;

            default:
                // Conservative defaults
                characteristics.sequentialThroughputMBps = 100.0;
                characteristics.randomThroughputMBps = 10.0;
                characteristics.averageLatencyUs = 1000.0;
                characteristics.maxConcurrentOps = 16;
        }

    } catch (const std::exception& e) {
        std::cerr << "StorageAnalyzer error: " << e.what() << std::endl;
    }

    return characteristics;
}

StorageCharacteristics StorageAnalyzer::benchmarkStorage(const std::string& path,
                                                         size_t testSizeMB) {
    auto characteristics = analyzeStorage(path);

    // Perform actual benchmark
    const size_t testSize = testSizeMB * 1024 * 1024;
    const size_t blockSize = 64 * 1024;  // 64KB blocks

    try {
        std::string testFile = path + "/benchmark_test.tmp";

        // Sequential write benchmark
        auto start = std::chrono::high_resolution_clock::now();
        {
            std::ofstream file(testFile, std::ios::binary);
            std::vector<char> buffer(blockSize, 'A');

            for (size_t written = 0; written < testSize; written += blockSize) {
                file.write(buffer.data(), blockSize);
            }
            file.flush();
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto writeDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double writeSpeedMBps =
            (testSize / (1024.0 * 1024.0)) / (writeDuration.count() / 1000000.0);

        // Sequential read benchmark
        start = std::chrono::high_resolution_clock::now();
        {
            std::ifstream file(testFile, std::ios::binary);
            std::vector<char> buffer(blockSize);

            while (file.read(buffer.data(), blockSize)) {
                // Consume data to prevent optimization
                volatile char c = buffer[0];
                (void)c;
            }
        }
        end = std::chrono::high_resolution_clock::now();

        auto readDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double readSpeedMBps = (testSize / (1024.0 * 1024.0)) / (readDuration.count() / 1000000.0);

        // Update characteristics with actual measurements
        characteristics.sequentialThroughputMBps = std::min(readSpeedMBps, writeSpeedMBps);
        characteristics.averageLatencyUs = std::max(writeDuration.count() / (testSize / blockSize),
                                                    readDuration.count() / (testSize / blockSize));

        // Clean up
        std::filesystem::remove(testFile);

    } catch (const std::exception& e) {
        std::cerr << "Storage benchmark error: " << e.what() << std::endl;
    }

    return characteristics;
}

IOPerformanceMonitor::OptimizationSuggestions
StorageAnalyzer::getStorageOptimizations(const StorageCharacteristics& storage) {
    IOPerformanceMonitor::OptimizationSuggestions suggestions;

    switch (storage.deviceType) {
        case StorageCharacteristics::DeviceType::SSD_NVME:
            suggestions.recommendedBufferSize = 1024 * 1024;       // 1MB for NVMe
            suggestions.recommendedCacheSize = 128 * 1024 * 1024;  // 128MB cache
            suggestions.recommendedThreadCount = std::min(32u, std::thread::hardware_concurrency());
            suggestions.enableCompression = false;  // NVMe is fast, don't compress
            suggestions.enablePrefetch = true;
            break;

        case StorageCharacteristics::DeviceType::SSD_SATA:
            suggestions.recommendedBufferSize = 512 * 1024;       // 512KB for SATA SSD
            suggestions.recommendedCacheSize = 64 * 1024 * 1024;  // 64MB cache
            suggestions.recommendedThreadCount = std::min(16u, std::thread::hardware_concurrency());
            suggestions.enableCompression = false;
            suggestions.enablePrefetch = true;
            break;

        case StorageCharacteristics::DeviceType::HDD:
            suggestions.recommendedBufferSize = 2 * 1024 * 1024;   // 2MB for HDD (sequential)
            suggestions.recommendedCacheSize = 256 * 1024 * 1024;  // 256MB cache
            suggestions.recommendedThreadCount = 2;                // Low concurrency for HDD
            suggestions.enableCompression = true;                  // Save disk space
            suggestions.enablePrefetch = true;
            break;

        case StorageCharacteristics::DeviceType::MEMORY_DISK:
            suggestions.recommendedBufferSize = 64 * 1024;        // 64KB for RAM disk
            suggestions.recommendedCacheSize = 32 * 1024 * 1024;  // 32MB cache
            suggestions.recommendedThreadCount = std::thread::hardware_concurrency();
            suggestions.enableCompression = false;
            suggestions.enablePrefetch = false;  // No need for RAM disk
            break;

        default:
            // Conservative defaults
            suggestions.recommendedBufferSize = 256 * 1024;
            suggestions.recommendedCacheSize = 64 * 1024 * 1024;
            suggestions.recommendedThreadCount = 4;
            suggestions.enableCompression = false;
            suggestions.enablePrefetch = true;
    }

    return suggestions;
}

#ifdef __linux__
// Helper function stubs for missing implementations
static std::string getBlockDevice(const std::filesystem::path& path) {
    // Stub implementation
    return "";
}

StorageCharacteristics::DeviceType
StorageAnalyzer::detectLinuxStorageType(const std::filesystem::path& path) {
    try {
        // Try to determine the underlying block device
        std::string devicePath = getBlockDevice(path);
        if (devicePath.empty()) {
            return StorageCharacteristics::DeviceType::UNKNOWN;
        }  // Check if it's an NVMe device
        if (devicePath.find("nvme") != std::string::npos) {
            return StorageCharacteristics::DeviceType::SSD_NVME;
        }

        // Check for SSD vs HDD by looking at rotational characteristic
        std::string rotationalFile = "/sys/block/"
                                     + std::filesystem::path(devicePath).filename().string()
                                     + "/queue/rotational";

        std::ifstream rotFile(rotationalFile);
        if (rotFile.is_open()) {
            int rotational;
            rotFile >> rotational;
            return rotational == 0 ? StorageCharacteristics::DeviceType::SSD_SATA
                                   : StorageCharacteristics::DeviceType::HDD;
        }

    } catch (const std::exception& e) {
        std::cerr << "Linux storage detection error: " << e.what() << std::endl;
    }

    return StorageCharacteristics::DeviceType::UNKNOWN;
}
#endif

// ============================================================================
// NUMAAudioAllocator Implementation
// ============================================================================

class NUMAAudioAllocator::Impl {
  public:
    NUMATopology topology_;
    mutable std::mutex allocationMutex_;
    std::unordered_map<void*, uint32_t> allocationMap_;

    void detectTopology() {
        topology_ = {};

#ifdef __linux__
#ifdef HAVE_NUMA
        if (numa_available() >= 0) {
            topology_.isNUMASystem = true;

            int maxNode = numa_max_node();
            topology_.nodes.reserve(maxNode + 1);

            for (int nodeId = 0; nodeId <= maxNode; ++nodeId) {
                if (numa_bitmask_isbitset(numa_nodes_ptr, nodeId)) {
                    NUMATopology::Node node;
                    node.nodeId = nodeId;

                    // Get CPUs for this node
                    struct bitmask* cpus = numa_allocate_cpumask();
                    if (numa_node_to_cpus(nodeId, cpus) == 0) {
                        for (unsigned int cpu = 0; cpu < cpus->size; ++cpu) {
                            if (numa_bitmask_isbitset(cpus, cpu)) {
                                node.cpuIds.push_back(cpu);
                            }
                        }
                    }
                    numa_free_cpumask(cpus);

                    // Get memory info
                    long long nodeMemory = numa_node_size64(nodeId, nullptr);
                    if (nodeMemory > 0) {
                        node.memoryMB = nodeMemory / (1024 * 1024);
                    }

                    // Estimate memory bandwidth (simplified)
                    node.memoryBandwidthGBps = 25.0;  // Typical DDR4 bandwidth per channel

                    topology_.nodes.push_back(node);
                }
            }

            // Detect current node
            topology_.currentNode = numa_node_of_cpu(sched_getcpu());
        }
#endif  // HAVE_NUMA
#endif  // __linux__

        // Fallback for non-NUMA systems
        if (topology_.nodes.empty()) {
            topology_.isNUMASystem = false;
            NUMATopology::Node node;
            node.nodeId = 0;

            // Add all CPUs to single node
            unsigned int numCpus = std::thread::hardware_concurrency();
            for (unsigned int cpu = 0; cpu < numCpus; ++cpu) {
                node.cpuIds.push_back(cpu);
            }

            // Estimate total system memory
            node.memoryMB = getSystemMemoryMB();
            node.memoryBandwidthGBps = 50.0;  // Reasonable default

            topology_.nodes.push_back(node);
            topology_.currentNode = 0;
        }
    }

  private:
    size_t getSystemMemoryMB() const {
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.substr(0, 9) == "MemTotal:") {
                std::istringstream iss(line);
                std::string label, unit;
                size_t value;
                iss >> label >> value >> unit;
                return value / 1024;  // Convert KB to MB
            }
        }
#endif
        return 8192;  // 8GB default
    }
};

NUMAAudioAllocator::NUMAAudioAllocator() : pImpl_(std::make_unique<Impl>()) {
    pImpl_->detectTopology();
}

NUMAAudioAllocator::~NUMAAudioAllocator() = default;

std::unique_ptr<float[], std::function<void(float*)>>
NUMAAudioAllocator::allocateBuffer(size_t sampleCount, uint32_t targetNode) {
    std::lock_guard<std::mutex> lock(pImpl_->allocationMutex_);

    size_t bytes = sampleCount * sizeof(float);
    float* buffer = nullptr;

#ifdef HAVE_NUMA
    if (pImpl_->topology_.isNUMASystem && targetNode < pImpl_->topology_.nodes.size()) {
        buffer = static_cast<float*>(numa_alloc_onnode(bytes, targetNode));
    } else {
        if (pImpl_->topology_.isNUMASystem) {
            buffer = static_cast<float*>(numa_alloc(bytes));
        } else {
            buffer = static_cast<float*>(std::aligned_alloc(64, bytes));
        }
    }
#else
    // Fallback for systems without NUMA
    buffer = static_cast<float*>(std::aligned_alloc(64, bytes));
#endif

    if (!buffer) {
        return nullptr;
    }

    // Track allocation
    pImpl_->allocationMap_[buffer] = targetNode;

    // Return with custom deleter
    return std::unique_ptr<float[], std::function<void(float*)>>(buffer, [this, bytes](float* ptr) {
        std::lock_guard<std::mutex> lock(pImpl_->allocationMutex_);
        pImpl_->allocationMap_.erase(ptr);
#ifdef HAVE_NUMA
        if (pImpl_->topology_.isNUMASystem) {
            numa_free(ptr, 0);  // Size is tracked internally by numa
        } else {
            std::free(ptr);
        }
#else
            std::free(ptr);
#endif
    });
}

uint32_t NUMAAudioAllocator::getOptimalNode() const {
#ifdef __linux__
#ifdef HAVE_NUMA
    if (pImpl_->topology_.isNUMASystem) {
        return numa_node_of_cpu(sched_getcpu());
    }
#endif
#endif
    return 0;
}

// ============================================================================
// AdaptiveBufferManager Implementation
// ============================================================================

class AdaptiveBufferManager::Impl {
  public:
    BufferConfig config_;
    NUMAAudioAllocator allocator_;

    // Performance tracking
    std::vector<double> utilizationHistory_;
    std::vector<std::chrono::nanoseconds> processingTimeHistory_;
    size_t currentOptimalSize_;
    std::atomic<size_t> totalBuffersAllocated_{0};
    std::atomic<size_t> adaptationCount_{0};

    // Adaptation control
    std::chrono::steady_clock::time_point lastAdaptation_;
    mutable std::mutex adaptationMutex_;

    Impl(const BufferConfig& config)
        : config_(config), currentOptimalSize_(config.initialSizeBytes) {
        lastAdaptation_ = std::chrono::steady_clock::now();
        utilizationHistory_.reserve(100);
        processingTimeHistory_.reserve(100);
    }

    void adaptBufferSize() {
        std::lock_guard<std::mutex> lock(adaptationMutex_);

        auto now = std::chrono::steady_clock::now();
        if (now - lastAdaptation_ < config_.adaptationInterval) {
            return;  // Too soon to adapt
        }

        if (utilizationHistory_.empty()) {
            return;  // No data to adapt on
        }

        // Calculate average utilization over recent history
        double avgUtilization =
            std::accumulate(utilizationHistory_.begin(), utilizationHistory_.end(), 0.0)
            / utilizationHistory_.size();

        size_t newSize = currentOptimalSize_;

        if (avgUtilization > config_.growthThreshold) {
            // Buffer is being used heavily, grow it
            newSize = std::min(static_cast<size_t>(currentOptimalSize_ * config_.growthFactor),
                               config_.maxSizeBytes);
        } else if (avgUtilization < config_.shrinkThreshold) {
            // Buffer is underutilized, shrink it
            newSize = std::max(static_cast<size_t>(currentOptimalSize_ / config_.growthFactor),
                               config_.minSizeBytes);
        }

        if (newSize != currentOptimalSize_) {
            currentOptimalSize_ = newSize;
            adaptationCount_.fetch_add(1);

            // Clear old history to start fresh measurement
            utilizationHistory_.clear();
            processingTimeHistory_.clear();
        }

        lastAdaptation_ = now;
    }
};

AdaptiveBufferManager::AdaptiveBufferManager(const BufferConfig& config)
    : pImpl_(std::make_unique<Impl>(config)) {}

AdaptiveBufferManager::~AdaptiveBufferManager() = default;

std::unique_ptr<float[], std::function<void(float*)>>
AdaptiveBufferManager::getBuffer(size_t minSamples, size_t& actualSamples) {
    pImpl_->adaptBufferSize();

    // Calculate buffer size in samples
    size_t minBytes = minSamples * sizeof(float);
    size_t optimalBytes = std::max(minBytes, pImpl_->currentOptimalSize_);

    actualSamples = optimalBytes / sizeof(float);

    pImpl_->totalBuffersAllocated_.fetch_add(1);

    return pImpl_->allocator_.allocateBuffer(actualSamples);
}

void AdaptiveBufferManager::recordUtilization(size_t actualUsed,
                                              size_t bufferSize,
                                              std::chrono::nanoseconds processingTime) {
    std::lock_guard<std::mutex> lock(pImpl_->adaptationMutex_);

    double utilization = static_cast<double>(actualUsed) / bufferSize;

    pImpl_->utilizationHistory_.push_back(utilization);
    pImpl_->processingTimeHistory_.push_back(processingTime);

    // Keep history size manageable
    if (pImpl_->utilizationHistory_.size() > 100) {
        pImpl_->utilizationHistory_.erase(pImpl_->utilizationHistory_.begin());
        pImpl_->processingTimeHistory_.erase(pImpl_->processingTimeHistory_.begin());
    }
}

AdaptiveBufferManager::BufferStats AdaptiveBufferManager::getStats() const {
    std::lock_guard<std::mutex> lock(pImpl_->adaptationMutex_);

    BufferStats stats;
    stats.currentOptimalSize = pImpl_->currentOptimalSize_;
    stats.totalBuffersAllocated = pImpl_->totalBuffersAllocated_.load();
    stats.adaptationCount = pImpl_->adaptationCount_.load();

    if (!pImpl_->utilizationHistory_.empty()) {
        stats.averageUtilization = std::accumulate(pImpl_->utilizationHistory_.begin(),
                                                   pImpl_->utilizationHistory_.end(),
                                                   0.0)
                                   / pImpl_->utilizationHistory_.size();
    } else {
        stats.averageUtilization = 0.0;
    }

    return stats;
}

void AdaptiveBufferManager::recalculateOptimalSize() {
    pImpl_->adaptBufferSize();
}

// ============================================================================
// Simple thread pool implementation for non-Linux systems
// ============================================================================

class ThreadPoolAsyncIO {
  private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopping_{false};

  public:
    explicit ThreadPoolAsyncIO(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (!stopping_) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

                        if (stopping_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    ~ThreadPoolAsyncIO() {
        stopping_ = true;
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename F>
    void enqueue(F&& task) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }

    size_t getPendingTaskCount() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return tasks_.size();
    }
};

// ============================================================================
// AdvancedAsyncIO Implementation
// ============================================================================

class AdvancedAsyncIO::Impl {
  public:
    Config config_;
    Engine activeEngine_ = Engine::THREAD_POOL;
    AdvancedIOMetrics metrics_ = {};
    std::atomic<bool> initialized_{false};

    // Thread pool implementation (fallback)
    std::unique_ptr<ThreadPoolAsyncIO> threadPool_;

    // Metrics tracking
    mutable std::mutex metricsMutex_;
    std::vector<std::chrono::nanoseconds> latencyHistory_;

    Impl(const Config& config) : config_(config) {}

    bool initialize() {
        if (initialized_)
            return true;

        // Detect best available engine
        activeEngine_ = detectBestEngine();

        switch (activeEngine_) {
            case Engine::THREAD_POOL:
                return initializeThreadPool();

#ifdef __linux__
#ifdef HAVE_IO_URING
            case Engine::IO_URING:
                return initializeIOUring();
#endif
#endif

            default:
                return initializeThreadPool();  // Fallback
        }
    }

    void shutdown() {
        if (!initialized_)
            return;

        threadPool_.reset();
        initialized_ = false;
    }

    void recordLatency(std::chrono::nanoseconds latency) {
        std::lock_guard<std::mutex> lock(metricsMutex_);

        latencyHistory_.push_back(latency);

        // Update min/max
        if (latency < metrics_.minLatency) {
            metrics_.minLatency = latency;
        }
        if (latency > metrics_.maxLatency) {
            metrics_.maxLatency = latency;
        }

        // Keep history manageable
        if (latencyHistory_.size() > 1000) {
            latencyHistory_.erase(latencyHistory_.begin());
        }

        // Calculate percentiles periodically
        if (latencyHistory_.size() % 100 == 0) {
            calculatePercentiles();
        }
    }

  private:
    Engine detectBestEngine() const {
        if (config_.preferredEngine != Engine::AUTO_DETECT) {
            return config_.preferredEngine;
        }

#ifdef __linux__
#ifdef HAVE_IO_URING
        // Check if io_uring is available
        struct io_uring ring;
        if (io_uring_queue_init(1, &ring, 0) == 0) {
            io_uring_queue_exit(&ring);
            return Engine::IO_URING;
        }
#endif
#endif

        return Engine::THREAD_POOL;
    }

    bool initializeThreadPool() {
        size_t numThreads = config_.workerThreads;
        if (numThreads == 0) {
            numThreads = std::min(8u, std::thread::hardware_concurrency());
        }

        try {
            threadPool_ = std::make_unique<ThreadPoolAsyncIO>(numThreads);
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize thread pool: " << e.what() << std::endl;
            return false;
        }
    }

#ifdef __linux__
#ifdef HAVE_IO_URING
    bool initializeIOUring() {
        // io_uring implementation would go here
        // For now, fall back to thread pool
        return initializeThreadPool();
    }
#endif
#endif

    void calculatePercentiles() {
        if (latencyHistory_.empty())
            return;

        std::vector<std::chrono::nanoseconds> sorted = latencyHistory_;
        std::sort(sorted.begin(), sorted.end());

        size_t p50_idx = sorted.size() * 50 / 100;
        size_t p95_idx = sorted.size() * 95 / 100;
        size_t p99_idx = sorted.size() * 99 / 100;

        metrics_.p50Latency = sorted[p50_idx];
        metrics_.p95Latency = sorted[p95_idx];
        metrics_.p99Latency = sorted[p99_idx];
    }
};

AdvancedAsyncIO::AdvancedAsyncIO() : pImpl_(std::make_unique<Impl>(Config{})) {}

AdvancedAsyncIO::AdvancedAsyncIO(const Config& config) : pImpl_(std::make_unique<Impl>(config)) {}

AdvancedAsyncIO::~AdvancedAsyncIO() {
    if (pImpl_) {
        pImpl_->shutdown();
    }
}

bool AdvancedAsyncIO::initialize() {
    return pImpl_->initialize();
}

void AdvancedAsyncIO::shutdown() {
    pImpl_->shutdown();
}

bool AdvancedAsyncIO::readAsync(
    int fileDescriptor, void* buffer, size_t size, off_t offset, CompletionCallback callback) {
    if (!pImpl_->initialized_)
        return false;

    // Enqueue read operation
    pImpl_->threadPool_->enqueue([=, this]() {
        auto start = std::chrono::high_resolution_clock::now();

        // Perform actual read
        ssize_t result = pread(fileDescriptor, buffer, size, offset);

        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        pImpl_->recordLatency(latency);

        bool success = (result >= 0);
        size_t bytesTransferred = success ? static_cast<size_t>(result) : 0;

        if (callback) {
            callback(success, bytesTransferred, latency);
        }
    });

    return true;
}

bool AdvancedAsyncIO::writeAsync(int fileDescriptor,
                                 const void* buffer,
                                 size_t size,
                                 off_t offset,
                                 CompletionCallback callback) {
    if (!pImpl_->initialized_)
        return false;

    // Enqueue write operation
    pImpl_->threadPool_->enqueue([=, this]() {
        auto start = std::chrono::high_resolution_clock::now();

        // Perform actual write
        ssize_t result = pwrite(fileDescriptor, buffer, size, offset);

        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        pImpl_->recordLatency(latency);

        bool success = (result >= 0);
        size_t bytesTransferred = success ? static_cast<size_t>(result) : 0;

        if (callback) {
            callback(success, bytesTransferred, latency);
        }
    });

    return true;
}

bool AdvancedAsyncIO::vectoredIO(int fileDescriptor,
                                 const std::vector<iovec>& vectors,
                                 off_t offset,
                                 bool isWrite,
                                 CompletionCallback callback) {
    if (!pImpl_->initialized_)
        return false;

    // Enqueue vectored I/O operation
    pImpl_->threadPool_->enqueue([=, this]() {
        auto start = std::chrono::high_resolution_clock::now();

        ssize_t result;
        if (isWrite) {
            result = pwritev(fileDescriptor, vectors.data(), vectors.size(), offset);
        } else {
            result = preadv(fileDescriptor, vectors.data(), vectors.size(), offset);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        pImpl_->recordLatency(latency);

        bool success = (result >= 0);
        size_t bytesTransferred = success ? static_cast<size_t>(result) : 0;

        if (callback) {
            callback(success, bytesTransferred, latency);
        }
    });

    return true;
}

AdvancedIOMetrics AdvancedAsyncIO::getMetrics() const {
    std::lock_guard<std::mutex> lock(pImpl_->metricsMutex_);
    return pImpl_->metrics_;
}

AdvancedAsyncIO::Engine AdvancedAsyncIO::getActiveEngine() const {
    return pImpl_->activeEngine_;
}

// ============================================================================
// CompressionPipeline Implementation (Stub)
// ============================================================================

class CompressionPipeline::Impl {
  public:
    explicit Impl(const Config& config) : config_(config) {}
    Config config_;
};

CompressionPipeline::CompressionPipeline() : pImpl_(std::make_unique<Impl>(Config{})) {}

CompressionPipeline::CompressionPipeline(const Config& config)
    : pImpl_(std::make_unique<Impl>(config)) {}

CompressionPipeline::~CompressionPipeline() = default;

CompressionPipeline::CompressionResult CompressionPipeline::compress(const float* audioData,
                                                                     size_t sampleCount,
                                                                     uint16_t channels,
                                                                     uint32_t sampleRate) {
    // Stub implementation
    CompressionResult result;
    result.compressedData.resize(sampleCount * sizeof(float));
    std::memcpy(result.compressedData.data(), audioData, sampleCount * sizeof(float));
    result.compressionTime = std::chrono::microseconds(100);
    result.success = true;
    return result;
}

CompressionPipeline::DecompressionResult
CompressionPipeline::decompress(const uint8_t* compressedData, size_t dataSize) {
    // Stub implementation
    DecompressionResult result;
    result.audioData.resize(dataSize / sizeof(float));
    std::memcpy(result.audioData.data(), compressedData, dataSize);
    result.channels = 1;
    result.sampleRate = 44100;
    result.decompressionTime = std::chrono::microseconds(100);
    result.success = true;
    return result;
}

// ============================================================================
// MasterIOOptimizer Implementation (Stub)
// ============================================================================

class MasterIOOptimizer::Impl {
  public:
    explicit Impl(const OptimizationProfile& profile) : profile_(profile) {}
    OptimizationProfile profile_;
};

MasterIOOptimizer::MasterIOOptimizer() : pImpl_(std::make_unique<Impl>(OptimizationProfile{})) {}

MasterIOOptimizer::MasterIOOptimizer(const OptimizationProfile& profile)
    : pImpl_(std::make_unique<Impl>(profile)) {}

MasterIOOptimizer::~MasterIOOptimizer() = default;

bool MasterIOOptimizer::initialize() {
    return true;  // Stub implementation
}

std::unique_ptr<MasterIOOptimizer::OptimizedIOHandle>
MasterIOOptimizer::optimizeForPath(const std::string& path) {
    return nullptr;  // Stub implementation
}

MasterIOOptimizer::SystemIOReport MasterIOOptimizer::getSystemReport() const {
    SystemIOReport report;
    report.overallHealthScore = 0.8;  // Stub value
    return report;
}

bool MasterIOOptimizer::autoTune() {
    return true;  // Stub implementation
}

bool MasterIOOptimizer::exportSettings(const std::string& filename) const {
    return true;  // Stub implementation
}

bool MasterIOOptimizer::importSettings(const std::string& filename) {
    return true;  // Stub implementation
}

}  // namespace io
}  // namespace huntmaster
