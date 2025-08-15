/**
 * @file OptimizedAudioIO.h
 * @brief High-performance audio I/O optimizations for recording and playback
 *
 * This file provides optimized I/O operations for audio recording and playback,
 * including memory-mapped files, asynchronous operations, buffered streaming,
 * and chunked processing for improved performance.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "DebugLogger.h"

namespace huntmaster {

/**
 * @brief Performance metrics for I/O operations
 */
struct IOPerformanceMetrics {
    std::chrono::microseconds totalReadTime{0};
    std::chrono::microseconds totalWriteTime{0};
    size_t bytesRead{0};
    size_t bytesWritten{0};
    size_t readOperations{0};
    size_t writeOperations{0};
    size_t cacheHits{0};
    size_t cacheMisses{0};

    double getReadThroughputMBps() const {
        if (totalReadTime.count() == 0)
            return 0.0;
        return (static_cast<double>(bytesRead) / (1024.0 * 1024.0))
               / (static_cast<double>(totalReadTime.count()) / 1000000.0);
    }

    double getWriteThroughputMBps() const {
        if (totalWriteTime.count() == 0)
            return 0.0;
        return (static_cast<double>(bytesWritten) / (1024.0 * 1024.0))
               / (static_cast<double>(totalWriteTime.count()) / 1000000.0);
    }

    double getCacheHitRatio() const {
        size_t totalAccesses = cacheHits + cacheMisses;
        return totalAccesses > 0 ? static_cast<double>(cacheHits) / totalAccesses : 0.0;
    }
};

/**
 * @brief Memory-mapped file reader for high-performance audio file access
 */
class MemoryMappedAudioFile {
  public:
    enum class AccessPattern {
        SEQUENTIAL,  // Sequential read access
        RANDOM,      // Random access pattern
        STREAMING    // Streaming with prefetch
    };

    struct Config {
        AccessPattern accessPattern;
        size_t prefetchSizeBytes;
        bool enableCaching;
        size_t maxCacheSize;

        Config()
            : accessPattern(AccessPattern::SEQUENTIAL),
              prefetchSizeBytes(1024 * 1024)  // 1MB prefetch
              ,
              enableCaching(true), maxCacheSize(64 * 1024 * 1024)  // 64MB cache
        {}
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit MemoryMappedAudioFile(const Config& config = Config());
    ~MemoryMappedAudioFile();

    /**
     * @brief Open audio file for memory-mapped access
     */
    bool open(const std::string& filename);

    /**
     * @brief Close the memory-mapped file
     */
    void close();

    /**
     * @brief Read audio samples with zero-copy access
     */
    const float* readSamples(size_t offset, size_t count);

    /**
     * @brief Get file size in samples
     */
    size_t getSampleCount() const;

    /**
     * @brief Get audio format information
     */
    struct AudioFormat {
        uint32_t sampleRate;
        uint16_t channels;
        uint16_t bitsPerSample;
        std::string formatName;
    };

    AudioFormat getFormat() const;

    /**
     * @brief Get performance metrics
     */
    IOPerformanceMetrics getMetrics() const;

    /**
     * @brief Prefetch data for future access
     */
    void prefetch(size_t offset, size_t count);

    /**
     * @brief Check if file is currently open
     */
    bool isOpen() const;
};

/**
 * @brief Asynchronous audio writer for non-blocking file operations
 */
class AsyncAudioWriter {
  public:
    enum class CompressionLevel { NONE = 0, FAST = 1, BALANCED = 5, BEST = 9 };

    struct Config {
        size_t bufferSizeBytes;
        size_t maxQueuedWrites;
        CompressionLevel compression;
        bool enableChecksums;
        std::string tempDirectory;

        Config()
            : bufferSizeBytes(2 * 1024 * 1024)  // 2MB buffer
              ,
              maxQueuedWrites(16), compression(CompressionLevel::NONE), enableChecksums(false),
              tempDirectory("/tmp") {}
    };

    using WriteCallback = std::function<void(bool success, const std::string& error)>;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit AsyncAudioWriter(const Config& config = Config());
    ~AsyncAudioWriter();

    /**
     * @brief Start asynchronous writer
     */
    bool start(const std::string& filename,
               uint32_t sampleRate,
               uint16_t channels,
               uint16_t bitsPerSample = 32);

    /**
     * @brief Queue audio data for writing
     */
    bool writeAsync(const float* data, size_t sampleCount, WriteCallback callback = nullptr);

    /**
     * @brief Flush all pending writes and stop
     */
    bool stop(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    /**
     * @brief Get current queue depth
     */
    size_t getQueueDepth() const;

    /**
     * @brief Check if writer is active
     */
    bool isActive() const;

    /**
     * @brief Get performance metrics
     */
    IOPerformanceMetrics getMetrics() const;
};

/**
 * @brief Streaming audio buffer for real-time recording/playback
 */
class StreamingAudioBuffer {
  public:
    struct Config {
        size_t bufferSizeFrames;
        size_t lowWatermarkFrames;
        size_t highWatermarkFrames;
        bool enableOverflowProtection;
        bool enableUnderflowProtection;

        Config()
            : bufferSizeFrames(8192), lowWatermarkFrames(2048), highWatermarkFrames(6144),
              enableOverflowProtection(true), enableUnderflowProtection(true) {}
    };

    using BufferCallback =
        std::function<void(const float* data, size_t frameCount, bool isOverflow)>;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit StreamingAudioBuffer(const Config& config = Config());
    ~StreamingAudioBuffer();

    /**
     * @brief Initialize streaming buffer
     */
    bool initialize(uint16_t channels);

    /**
     * @brief Write audio frames to buffer
     */
    size_t write(const float* data, size_t frameCount);

    /**
     * @brief Read audio frames from buffer
     */
    size_t read(float* data, size_t frameCount);

    /**
     * @brief Get available frames for reading
     */
    size_t getAvailableFrames() const;

    /**
     * @brief Get free space for writing
     */
    size_t getFreeSpace() const;

    /**
     * @brief Set buffer level callbacks
     */
    void setBufferCallback(BufferCallback callback);

    /**
     * @brief Clear buffer contents
     */
    void clear();

    /**
     * @brief Get buffer health metrics
     */
    struct BufferHealth {
        double fillRatio;
        size_t overflowCount;
        size_t underflowCount;
        bool isHealthy;
    };

    BufferHealth getHealth() const;
};

/**
 * @brief Chunked audio processor for large file operations
 */
class ChunkedAudioProcessor {
  public:
    struct Config {
        size_t chunkSizeFrames;
        size_t overlapFrames;
        size_t maxParallelChunks;
        bool enableProgressCallback;

        Config()
            : chunkSizeFrames(16384), overlapFrames(1024), maxParallelChunks(4),
              enableProgressCallback(true) {}
    };

    using ChunkProcessor =
        std::function<bool(const float* data, size_t frameCount, size_t chunkIndex)>;
    using ProgressCallback =
        std::function<void(size_t processedFrames, size_t totalFrames, double percentage)>;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit ChunkedAudioProcessor(const Config& config = Config());
    ~ChunkedAudioProcessor();

    /**
     * @brief Process audio file in chunks
     */
    bool processFile(const std::string& filename,
                     ChunkProcessor processor,
                     ProgressCallback progressCallback = nullptr);

    /**
     * @brief Process audio buffer in chunks
     */
    bool processBuffer(const float* data,
                       size_t frameCount,
                       uint16_t channels,
                       ChunkProcessor processor,
                       ProgressCallback progressCallback = nullptr);

    /**
     * @brief Cancel ongoing processing
     */
    void cancel();

    /**
     * @brief Check if processing is active
     */
    bool isProcessing() const;

    /**
     * @brief Get processing metrics
     */
    IOPerformanceMetrics getMetrics() const;
};

/**
 * @brief Audio format converter with optimized performance
 */
class OptimizedFormatConverter {
  public:
    enum class Quality {
        FAST,         // Fast conversion with minimal quality
        BALANCED,     // Balanced speed/quality
        HIGH_QUALITY  // High quality conversion
    };

    struct Config {
        Quality conversionQuality;
        bool enableDithering;
        bool enableNormalization;
        size_t maxThreads;

        Config()
            : conversionQuality(Quality::BALANCED), enableDithering(true),
              enableNormalization(false), maxThreads(std::thread::hardware_concurrency()) {}
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit OptimizedFormatConverter(const Config& config = Config());
    ~OptimizedFormatConverter();

    /**
     * @brief Convert audio format
     */
    bool convert(const float* inputData,
                 size_t inputFrames,
                 uint16_t inputChannels,
                 uint32_t inputSampleRate,
                 std::vector<float>& outputData,
                 uint16_t outputChannels,
                 uint32_t outputSampleRate);

    /**
     * @brief Convert bit depth
     */
    bool convertBitDepth(const void* inputData,
                         void* outputData,
                         size_t frameCount,
                         uint16_t channels,
                         uint16_t inputBitsPerSample,
                         uint16_t outputBitsPerSample);

    /**
     * @brief Resample audio data
     */
    bool resample(const float* inputData,
                  size_t inputFrames,
                  uint32_t inputSampleRate,
                  std::vector<float>& outputData,
                  uint32_t outputSampleRate,
                  uint16_t channels);

    /**
     * @brief Get conversion metrics
     */
    IOPerformanceMetrics getMetrics() const;
};

/**
 * @brief I/O performance monitor and optimizer
 */
class IOPerformanceMonitor {
  public:
    struct Thresholds {
        double minReadThroughputMBps;
        double minWriteThroughputMBps;
        double maxLatencyMs;
        double minCacheHitRatio;

        Thresholds()
            : minReadThroughputMBps(50.0), minWriteThroughputMBps(30.0), maxLatencyMs(10.0),
              minCacheHitRatio(0.8) {}
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit IOPerformanceMonitor(const Thresholds& thresholds = Thresholds());
    ~IOPerformanceMonitor();

    /**
     * @brief Register I/O operation for monitoring
     */
    void recordOperation(const std::string& operation, const IOPerformanceMetrics& metrics);

    /**
     * @brief Get overall performance report
     */
    struct PerformanceReport {
        IOPerformanceMetrics aggregatedMetrics;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        bool isPerformanceAcceptable;
    };

    PerformanceReport getReport() const;

    /**
     * @brief Auto-tune I/O parameters based on performance
     */
    struct OptimizationSuggestions {
        size_t recommendedBufferSize;
        size_t recommendedCacheSize;
        size_t recommendedThreadCount;
        bool enableCompression;
        bool enablePrefetch;
    };

    OptimizationSuggestions getOptimizationSuggestions() const;

    /**
     * @brief Reset monitoring statistics
     */
    void reset();
};

}  // namespace huntmaster
