/**
 * @file IOOptimizationDemo.cpp
 * @brief Demonstration of advanced I/O optimization features
 *
 * This tool demonstrates how to use the advanced I/O optimization features
 * to improve file recording and playback performance in the Huntmaster Audio Engine.
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dr_wav.h"
#include "huntmaster/core/AdvancedIOOptimizer.h"
#include "huntmaster/core/OptimizedAudioIO.h"
#include "huntmaster/core/OptimizedAudioRecorder.hh"

namespace huntmaster {

class IOOptimizationDemo {
  private:
    std::unique_ptr<io::MasterIOOptimizer> optimizer_;
    std::unique_ptr<io::StorageAnalyzer> storageAnalyzer_;

  public:
    IOOptimizationDemo() {
        // Initialize with balanced profile for interactive audio work
        io::MasterIOOptimizer::OptimizationProfile profile;
        profile.workloadType =
            io::MasterIOOptimizer::OptimizationProfile::WorkloadType::INTERACTIVE_PLAYBACK;
        profile.maxLatency = std::chrono::microseconds(5000);  // 5ms max latency
        profile.minThroughputMBps = 100.0;                     // 100 MB/s minimum
        profile.enableCompression = false;                     // Prioritize latency over storage
        profile.enableCaching = true;
        profile.enablePrefetch = true;
        profile.enableNUMAOptimization = true;

        optimizer_ = std::make_unique<io::MasterIOOptimizer>(profile);
        if (!optimizer_->initialize()) {
            std::cerr << "Failed to initialize Master IO Optimizer!" << std::endl;
            throw std::runtime_error("Optimizer initialization failed.");
        }
    }

    void demonstrateStorageAnalysis(const std::string& path) {
        std::cout << "\n=== Storage Analysis Demo ===" << std::endl;
        std::cout << "Analyzing storage for path: " << path << std::endl;

        // Analyze storage characteristics
        auto characteristics = io::StorageAnalyzer::analyzeStorage(path);

        std::cout << "\nDetected Storage Characteristics:" << std::endl;
        std::cout << "  Device Type: " << storageTypeToString(characteristics.deviceType)
                  << std::endl;
        std::cout << "  Optimal Block Size: " << characteristics.optimalBlockSize << " bytes"
                  << std::endl;
        std::cout << "  Sequential Throughput: " << std::fixed << std::setprecision(1)
                  << characteristics.sequentialThroughputMBps << " MB/s" << std::endl;
        std::cout << "  Random Throughput: " << characteristics.randomThroughputMBps << " MB/s"
                  << std::endl;
        std::cout << "  Average Latency: " << characteristics.averageLatencyUs << " μs"
                  << std::endl;
        std::cout << "  Max Concurrent Ops: " << characteristics.maxConcurrentOps << std::endl;
        std::cout << "  Direct I/O Support: " << (characteristics.supportsDirectIO ? "Yes" : "No")
                  << std::endl;

        // Get optimization recommendations
        auto suggestions = io::StorageAnalyzer::getStorageOptimizations(characteristics);

        std::cout << "\nOptimization Recommendations:" << std::endl;
        std::cout << "  Recommended Buffer Size: " << (suggestions.recommendedBufferSize / 1024)
                  << " KB" << std::endl;
        std::cout << "  Recommended Cache Size: "
                  << (suggestions.recommendedCacheSize / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "  Recommended Thread Count: " << suggestions.recommendedThreadCount
                  << std::endl;
        std::cout << "  Enable Compression: " << (suggestions.enableCompression ? "Yes" : "No")
                  << std::endl;
        std::cout << "  Enable Prefetch: " << (suggestions.enablePrefetch ? "Yes" : "No")
                  << std::endl;

        // Perform benchmark if requested
        std::cout << "\nPerforming storage benchmark..." << std::endl;
        auto benchmarkResults = io::StorageAnalyzer::benchmarkStorage(path, 50);  // 50MB test

        std::cout << "Benchmark Results:" << std::endl;
        std::cout << "  Measured Sequential Throughput: " << std::fixed << std::setprecision(1)
                  << benchmarkResults.sequentialThroughputMBps << " MB/s" << std::endl;
        std::cout << "  Measured Average Latency: " << benchmarkResults.averageLatencyUs << " μs"
                  << std::endl;
    }

    void demonstrateNUMAOptimization() {
        std::cout << "\n=== NUMA Optimization Demo ===" << std::endl;

        io::NUMAAudioAllocator allocator;
        auto topology = allocator.getTopology();

        std::cout << "NUMA System: " << (topology.isNUMASystem ? "Yes" : "No") << std::endl;
        std::cout << "Current NUMA Node: " << topology.currentNode << std::endl;
        std::cout << "Total NUMA Nodes: " << topology.nodes.size() << std::endl;

        for (const auto& node : topology.nodes) {
            std::cout << "  Node " << node.nodeId << ":" << std::endl;
            std::cout << "    CPUs: ";
            for (size_t i = 0; i < node.cpuIds.size(); ++i) {
                std::cout << node.cpuIds[i];
                if (i < node.cpuIds.size() - 1)
                    std::cout << ", ";
            }
            std::cout << std::endl;
            std::cout << "    Memory: " << node.memoryMB << " MB" << std::endl;
            std::cout << "    Bandwidth: " << std::fixed << std::setprecision(1)
                      << node.memoryBandwidthGBps << " GB/s" << std::endl;
        }

        // Demonstrate NUMA-aware allocation
        std::cout << "\nAllocating NUMA-optimized audio buffers:" << std::endl;

        const size_t bufferSize = 48000 * 2;  // 1 second stereo at 48kHz
        auto buffer1 = allocator.allocateBuffer(bufferSize);
        auto buffer2 = allocator.allocateBuffer(bufferSize, 0);  // Force node 0

        if (buffer1 && buffer2) {
            std::cout << "  Successfully allocated two " << (bufferSize * sizeof(float) / 1024)
                      << " KB audio buffers" << std::endl;
            std::cout << "  Buffers are aligned and NUMA-optimized" << std::endl;
        }
    }

    void demonstrateAdaptiveBuffering() {
        std::cout << "\n=== Adaptive Buffer Management Demo ===" << std::endl;

        io::AdaptiveBufferManager::BufferConfig config;
        config.initialSizeBytes = 64 * 1024;                         // Start with 64KB
        config.minSizeBytes = 16 * 1024;                             // Minimum 16KB
        config.maxSizeBytes = 1024 * 1024;                           // Maximum 1MB
        config.adaptationInterval = std::chrono::milliseconds(500);  // Adapt every 500ms

        io::AdaptiveBufferManager bufferManager(config);

        std::cout << "Initial buffer configuration:" << std::endl;
        std::cout << "  Initial size: " << (config.initialSizeBytes / 1024) << " KB" << std::endl;
        std::cout << "  Size range: " << (config.minSizeBytes / 1024) << " - "
                  << (config.maxSizeBytes / 1024) << " KB" << std::endl;

        // Simulate different utilization patterns
        std::cout << "\nSimulating varying buffer utilization patterns..." << std::endl;

        for (int phase = 0; phase < 3; ++phase) {
            std::cout << "\nPhase " << (phase + 1) << ": ";

            double utilizationPattern;
            switch (phase) {
                case 0:
                    std::cout << "Light usage (30% utilization)" << std::endl;
                    utilizationPattern = 0.3;
                    break;
                case 1:
                    std::cout << "Heavy usage (85% utilization)" << std::endl;
                    utilizationPattern = 0.85;
                    break;
                case 2:
                    std::cout << "Moderate usage (60% utilization)" << std::endl;
                    utilizationPattern = 0.6;
                    break;
            }

            // Simulate 10 buffer allocations with this pattern
            for (int i = 0; i < 10; ++i) {
                size_t actualSamples;
                auto buffer = bufferManager.getBuffer(16384, actualSamples);  // Ask for 16K samples

                if (buffer) {
                    // Simulate processing with the utilization pattern
                    size_t usedSamples = static_cast<size_t>(actualSamples * utilizationPattern);
                    auto processingTime =
                        std::chrono::nanoseconds(100000 + (i * 10000));  // Varying processing time

                    bufferManager.recordUtilization(
                        usedSamples * sizeof(float), actualSamples * sizeof(float), processingTime);
                }

                // Allow time for adaptation
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            auto stats = bufferManager.getStats();
            std::cout << "  Current optimal size: " << (stats.currentOptimalSize / 1024) << " KB"
                      << std::endl;
            std::cout << "  Average utilization: " << std::fixed << std::setprecision(1)
                      << (stats.averageUtilization * 100) << "%" << std::endl;
            std::cout << "  Total adaptations: " << stats.adaptationCount << std::endl;
        }
    }

    void demonstrateAdvancedAsyncIO(const std::string& testFile) {
        std::cout << "\n=== Advanced Async I/O Demo ===" << std::endl;

        io::AdvancedAsyncIO::Config config;
        config.preferredEngine = io::AdvancedAsyncIO::Engine::AUTO_DETECT;
        config.queueDepth = 64;
        config.enableBatching = true;
        config.batchSize = 8;

        io::AdvancedAsyncIO asyncIO(config);

        if (!asyncIO.initialize()) {
            std::cerr << "Failed to initialize advanced async I/O" << std::endl;
            return;
        }

        std::cout << "Active I/O engine: " << engineTypeToString(asyncIO.getActiveEngine())
                  << std::endl;

        // Create test data
        const size_t testDataSize = 1024 * 1024;  // 1MB
        std::vector<char> testData(testDataSize, 'A');

        // Write test
        std::cout << "\nPerforming async write test..." << std::endl;

        int fd = open(testFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            std::cerr << "Failed to create test file" << std::endl;
            return;
        }

        std::atomic<int> completedWrites{0};
        std::atomic<size_t> totalBytesWritten{0};
        const int numWrites = 10;

        auto writeStart = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numWrites; ++i) {
            asyncIO.writeAsync(
                fd,
                testData.data(),
                testData.size(),
                i * testDataSize,
                [&](bool success, size_t bytesTransferred, std::chrono::nanoseconds latency) {
                    if (success) {
                        totalBytesWritten += bytesTransferred;
                    }
                    completedWrites++;
                });
        }

        // Wait for completion
        while (completedWrites.load() < numWrites) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto writeEnd = std::chrono::high_resolution_clock::now();
        auto writeDuration =
            std::chrono::duration_cast<std::chrono::microseconds>(writeEnd - writeStart);

        close(fd);

        double writeThroughputMBps =
            (totalBytesWritten.load() / (1024.0 * 1024.0)) / (writeDuration.count() / 1000000.0);

        std::cout << "Write Results:" << std::endl;
        std::cout << "  Total bytes written: " << totalBytesWritten.load() << std::endl;
        std::cout << "  Write throughput: " << std::fixed << std::setprecision(1)
                  << writeThroughputMBps << " MB/s" << std::endl;
        std::cout << "  Total write time: " << writeDuration.count() << " μs" << std::endl;

        // Get detailed metrics
        auto metrics = asyncIO.getMetrics();
        std::cout << "\nDetailed I/O Metrics:" << std::endl;
        std::cout << "  Min latency: " << metrics.minLatency.count() << " ns" << std::endl;
        std::cout << "  Max latency: " << metrics.maxLatency.count() << " ns" << std::endl;
        std::cout << "  P95 latency: " << metrics.p95Latency.count() << " ns" << std::endl;
        std::cout << "  P99 latency: " << metrics.p99Latency.count() << " ns" << std::endl;

        asyncIO.shutdown();

        // Clean up
        std::filesystem::remove(testFile);
    }

    void demonstrateSystemOptimization(const std::string& audioPath) {
        std::cout << "\n=== System-Wide I/O Optimization Demo ===" << std::endl;

        // The optimizer is now initialized in the constructor
        // if (!optimizer_->initialize()) {
        //     std::cerr << "Failed to initialize master I/O optimizer" << std::endl;
        //     return;
        // }

        // Optimize for the given audio path
        auto optimizedHandle = optimizer_->optimizeForPath(audioPath);

        if (optimizedHandle) {
            std::cout << "Successfully optimized I/O for path: " << audioPath << std::endl;

            const auto& storage = optimizedHandle->storageInfo;
            const auto& suggestions = optimizedHandle->suggestions;

            std::cout << "\nOptimized Configuration:" << std::endl;
            std::cout << "  Storage type: " << storageTypeToString(storage.deviceType) << std::endl;
            std::cout << "  Buffer size: " << (suggestions.recommendedBufferSize / 1024) << " KB"
                      << std::endl;
            std::cout << "  Cache size: " << (suggestions.recommendedCacheSize / (1024 * 1024))
                      << " MB" << std::endl;
            std::cout << "  Thread count: " << suggestions.recommendedThreadCount << std::endl;
            std::cout << "  Compression: "
                      << (suggestions.enableCompression ? "Enabled" : "Disabled") << std::endl;
            std::cout << "  Prefetch: " << (suggestions.enablePrefetch ? "Enabled" : "Disabled")
                      << std::endl;
        }

        // Auto-tune system parameters
        std::cout << "\nPerforming system auto-tuning..." << std::endl;
        if (optimizer_->autoTune()) {
            std::cout << "Auto-tuning completed successfully" << std::endl;
        } else {
            std::cout << "Auto-tuning encountered issues" << std::endl;
        }

        // Get system performance report
        auto systemReport = optimizer_->getSystemReport();

        std::cout << "\nSystem Performance Report:" << std::endl;
        std::cout << "  Overall health score: " << std::fixed << std::setprecision(2)
                  << (systemReport.overallHealthScore * 100) << "%" << std::endl;

        if (!systemReport.performanceWarnings.empty()) {
            std::cout << "\nPerformance Warnings:" << std::endl;
            for (const auto& warning : systemReport.performanceWarnings) {
                std::cout << "  • " << warning << std::endl;
            }
        }

        if (!systemReport.optimizationRecommendations.empty()) {
            std::cout << "\nOptimization Recommendations:" << std::endl;
            for (const auto& recommendation : systemReport.optimizationRecommendations) {
                std::cout << "  • " << recommendation << std::endl;
            }
        }
    }

  private:
    std::string storageTypeToString(io::StorageCharacteristics::DeviceType type) {
        switch (type) {
            case io::StorageCharacteristics::DeviceType::HDD:
                return "HDD";
            case io::StorageCharacteristics::DeviceType::SSD_SATA:
                return "SATA SSD";
            case io::StorageCharacteristics::DeviceType::SSD_NVME:
                return "NVMe SSD";
            case io::StorageCharacteristics::DeviceType::NETWORK_STORAGE:
                return "Network Storage";
            case io::StorageCharacteristics::DeviceType::MEMORY_DISK:
                return "Memory Disk";
            default:
                return "Unknown";
        }
    }

    std::string engineTypeToString(io::AdvancedAsyncIO::Engine engine) {
        switch (engine) {
            case io::AdvancedAsyncIO::Engine::THREAD_POOL:
                return "Thread Pool";
            case io::AdvancedAsyncIO::Engine::IO_URING:
                return "io_uring";
            case io::AdvancedAsyncIO::Engine::IOCP:
                return "IOCP";
            case io::AdvancedAsyncIO::Engine::EPOLL:
                return "epoll";
            default:
                return "Auto-Detect";
        }
    }
};

}  // namespace huntmaster

int main(int argc, char* argv[]) {
    std::cout << "Huntmaster Audio Engine - Advanced I/O Optimization Demo" << std::endl;
    std::cout << "========================================================" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_for_io_tests>" << std::endl;
        return 1;
    }

    std::string testPath = argv[1];

    try {
        huntmaster::IOOptimizationDemo demo;

        demo.demonstrateStorageAnalysis(testPath);
        demo.demonstrateNUMAOptimization();
        demo.demonstrateAdaptiveBuffering();
        demo.demonstrateAdvancedAsyncIO(testPath + "/async_test_file.dat");
        demo.demonstrateSystemOptimization(testPath + "/sample_audio.wav");

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
