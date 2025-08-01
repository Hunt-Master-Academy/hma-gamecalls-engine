/**
 * @file CircularAudioBuffer.h
 * @brief Circular Audio Buffer System Header
 *
 * This file defines the Circular Audio Buffer system for efficient real-time
 * audio processing with thread-safe operations and optimized memory management.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <stdlib.h>  // For free
#ifdef _WIN32
#include <malloc.h>  // For _aligned_free
#endif

namespace huntmaster {
namespace core {

// Forward declarations
class AudioBuffer;
struct AudioConfig;

// Custom deleter for aligned memory
struct AlignedDeleter {
    void operator()(float* ptr) const {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
};

// TODO: Phase 2.4 - Advanced Audio Engine Header - COMPREHENSIVE FILE TODO
// ========================================================================

// TODO 2.4.83: CircularAudioBuffer Core System
// --------------------------------------------
/**
 * TODO: Define CircularAudioBuffer core system with:
 * [ ] Thread-safe circular buffer implementation with lock-free operations
 * [ ] Real-time audio processing support with minimal latency
 * [ ] Multi-reader/single-writer architecture with efficient synchronization
 * [ ] Configurable buffer sizes with dynamic resizing capabilities
 * [ ] Memory-efficient storage with optimized allocation patterns
 * [ ] Overflow/underflow handling with graceful degradation
 * [ ] Performance monitoring with statistics and diagnostics
 * [ ] Integration with streaming audio processors
 * [ ] Support for various audio formats and sample rates
 * [ ] Zero-copy operations where possible for maximum efficiency
 */

/**
 * @brief Configuration structure for CircularAudioBuffer
 */
struct CircularBufferConfig {
    // Buffer configuration
    size_t bufferSize = 8192;     ///< Buffer size in samples
    size_t numChannels = 1;       ///< Number of audio channels
    uint32_t sampleRate = 44100;  ///< Sample rate in Hz

    // Performance configuration
    bool enableLockFreeOperations = true;  ///< Enable lock-free operations where possible
    bool enableStatistics = true;          ///< Enable performance statistics
    size_t writeBlockSize = 1024;          ///< Preferred write block size
    size_t readBlockSize = 1024;           ///< Preferred read block size

    // Safety configuration
    float overflowThreshold = 0.95f;        ///< Buffer overflow threshold (95%)
    float underflowThreshold = 0.05f;       ///< Buffer underflow threshold (5%)
    bool enableOverflowProtection = true;   ///< Enable overflow protection
    bool enableUnderflowProtection = true;  ///< Enable underflow protection

    // Monitoring configuration
    bool enableLatencyMonitoring = true;     ///< Enable latency monitoring
    size_t statisticsUpdateInterval = 1000;  ///< Statistics update interval (ms)
    bool enableDetailedDiagnostics = false;  ///< Enable detailed diagnostics

    // Advanced configuration
    bool enableDynamicResize = false;  ///< Enable dynamic buffer resizing
    float resizeThreshold = 0.8f;      ///< Threshold for dynamic resize
    size_t maxBufferSize = 32768;      ///< Maximum buffer size
    size_t minBufferSize = 1024;       ///< Minimum buffer size

    // Threading configuration
    int readerPriority = 0;             ///< Reader thread priority
    int writerPriority = 0;             ///< Writer thread priority
    bool enableThreadAffinity = false;  ///< Enable thread affinity

    // Error handling
    bool enableErrorRecovery = true;  ///< Enable automatic error recovery
    size_t maxRetries = 3;            ///< Maximum retry attempts
    size_t retryDelay = 10;           ///< Retry delay in milliseconds
};

/**
 * @brief Statistics structure for CircularAudioBuffer performance monitoring
 */
struct CircularBufferStatistics {
    // Buffer state
    std::atomic<size_t> totalWrites{0};   ///< Total number of write operations
    std::atomic<size_t> totalReads{0};    ///< Total number of read operations
    std::atomic<size_t> totalSamples{0};  ///< Total samples processed
    std::atomic<size_t> currentLevel{0};  ///< Current buffer level
    std::atomic<float> fillRatio{0.0f};   ///< Current fill ratio (0-1)

    // Performance metrics
    std::atomic<float> averageWriteLatency{0.0f};  ///< Average write latency (ms)
    std::atomic<float> averageReadLatency{0.0f};   ///< Average read latency (ms)
    std::atomic<float> maxWriteLatency{0.0f};      ///< Maximum write latency (ms)
    std::atomic<float> maxReadLatency{0.0f};       ///< Maximum read latency (ms)
    std::atomic<float> throughput{0.0f};           ///< Throughput (samples/sec)

    // Error tracking
    std::atomic<size_t> overflowCount{0};   ///< Number of overflow events
    std::atomic<size_t> underflowCount{0};  ///< Number of underflow events
    std::atomic<size_t> retryCount{0};      ///< Number of retry attempts
    std::atomic<size_t> errorCount{0};      ///< Total error count
    std::atomic<float> errorRate{0.0f};     ///< Error rate (errors/operations)

    // Timing
    std::chrono::steady_clock::time_point startTime;   ///< Start time
    std::chrono::steady_clock::time_point lastUpdate;  ///< Last statistics update
    std::atomic<uint64_t> totalOperationTime{0};       ///< Total operation time (μs)

    // Health indicators
    std::atomic<bool> isHealthy{true};         ///< Overall health status
    std::atomic<float> healthScore{1.0f};      ///< Health score (0-1)
    std::atomic<size_t> consecutiveErrors{0};  ///< Consecutive error count

    CircularBufferStatistics() = default;

    CircularBufferStatistics(const CircularBufferStatistics& other) {
        totalWrites = other.totalWrites.load();
        totalReads = other.totalReads.load();
        totalSamples = other.totalSamples.load();
        currentLevel = other.currentLevel.load();
        fillRatio = other.fillRatio.load();
        averageWriteLatency = other.averageWriteLatency.load();
        averageReadLatency = other.averageReadLatency.load();
        maxWriteLatency = other.maxWriteLatency.load();
        maxReadLatency = other.maxReadLatency.load();
        throughput = other.throughput.load();
        overflowCount = other.overflowCount.load();
        underflowCount = other.underflowCount.load();
        retryCount = other.retryCount.load();
        errorCount = other.errorCount.load();
        errorRate = other.errorRate.load();
        totalOperationTime = other.totalOperationTime.load();
        isHealthy = other.isHealthy.load();
        healthScore = other.healthScore.load();
        consecutiveErrors = other.consecutiveErrors.load();
        startTime = other.startTime;
        lastUpdate = other.lastUpdate;
    }

    CircularBufferStatistics& operator=(const CircularBufferStatistics& other) {
        if (this == &other) {
            return *this;
        }
        totalWrites = other.totalWrites.load();
        totalReads = other.totalReads.load();
        totalSamples = other.totalSamples.load();
        currentLevel = other.currentLevel.load();
        fillRatio = other.fillRatio.load();
        averageWriteLatency = other.averageWriteLatency.load();
        averageReadLatency = other.averageReadLatency.load();
        maxWriteLatency = other.maxWriteLatency.load();
        maxReadLatency = other.maxReadLatency.load();
        throughput = other.throughput.load();
        overflowCount = other.overflowCount.load();
        underflowCount = other.underflowCount.load();
        retryCount = other.retryCount.load();
        errorCount = other.errorCount.load();
        errorRate = other.errorRate.load();
        totalOperationTime = other.totalOperationTime.load();
        isHealthy = other.isHealthy.load();
        healthScore = other.healthScore.load();
        consecutiveErrors = other.consecutiveErrors.load();
        startTime = other.startTime;
        lastUpdate = other.lastUpdate;
        return *this;
    }
};

/**
 * @brief Error information structure for CircularAudioBuffer
 */
struct CircularBufferError {
    int code = 0;                                     ///< Error code
    std::string message;                              ///< Error message
    std::string details;                              ///< Detailed error information
    std::chrono::steady_clock::time_point timestamp;  ///< Error timestamp
    std::string component;                            ///< Component that generated the error
    size_t bufferState = 0;                           ///< Buffer state at error time
    size_t operationContext = 0;                      ///< Operation context
};

// TODO 2.4.84: Callback Type Definitions
// --------------------------------------
/**
 * TODO: Define comprehensive callback system with:
 * [ ] Buffer state change callbacks with detailed information
 * [ ] Overflow/underflow event callbacks with recovery actions
 * [ ] Performance monitoring callbacks with real-time metrics
 * [ ] Error handling callbacks with recovery suggestions
 * [ ] Statistics update callbacks with trend analysis
 * [ ] Health monitoring callbacks with diagnostic information
 * [ ] Threading callbacks with priority management
 * [ ] Memory management callbacks with allocation tracking
 * [ ] Integration callbacks with external systems
 * [ ] Custom event callbacks with user-defined triggers
 */
using BufferStateCallback = std::function<void(size_t currentLevel, float fillRatio)>;
using OverflowCallback = std::function<void(size_t attemptedWrite, size_t availableSpace)>;
using UnderflowCallback = std::function<void(size_t attemptedRead, size_t availableData)>;
using PerformanceCallback = std::function<void(const CircularBufferStatistics& stats)>;
using ErrorCallback = std::function<void(int errorCode, const std::string& message)>;
using HealthCallback = std::function<void(bool isHealthy, float healthScore)>;
using StatisticsCallback = std::function<void(const CircularBufferStatistics& stats)>;
using ResizeCallback = std::function<void(size_t oldSize, size_t newSize)>;

// TODO 2.4.85: CircularAudioBuffer Main Class
// -------------------------------------------
/**
 * @brief High-performance circular buffer for real-time audio processing
 *
 * TODO: Implement comprehensive CircularAudioBuffer with:
 * [ ] Thread-safe operations with lock-free algorithms where possible
 * [ ] Real-time performance guarantees with bounded latency
 * [ ] Multi-channel audio support with interleaved or planar formats
 * [ ] Dynamic buffer management with automatic resizing
 * [ ] Comprehensive error handling with recovery mechanisms
 * [ ] Performance monitoring with detailed statistics
 * [ ] Memory optimization with efficient allocation strategies
 * [ ] Integration with audio processing pipelines
 * [ ] Cross-platform compatibility with platform-specific optimizations
 * [ ] Advanced diagnostics with health monitoring and reporting
 */
class CircularAudioBuffer {
  public:
    // TODO: Constructor and destructor
    CircularAudioBuffer();
    explicit CircularAudioBuffer(const CircularBufferConfig& config);
    ~CircularAudioBuffer();

    // Non-copyable but movable
    CircularAudioBuffer(const CircularAudioBuffer&) = delete;
    CircularAudioBuffer& operator=(const CircularAudioBuffer&) = delete;
    CircularAudioBuffer(CircularAudioBuffer&&) noexcept;
    CircularAudioBuffer& operator=(CircularAudioBuffer&&) noexcept;

    // TODO 2.4.86: Initialization and Configuration
    // ---------------------------------------------
    /**
     * TODO: Implement initialization and configuration with:
     * [ ] Buffer initialization with memory allocation and setup
     * [ ] Configuration validation with parameter checking
     * [ ] Thread setup with priority and affinity configuration
     * [ ] Performance baseline establishment with calibration
     * [ ] Error handling initialization with recovery setup
     * [ ] Statistics initialization with baseline metrics
     * [ ] Integration setup with external components
     * [ ] Memory optimization with alignment and cache considerations
     * [ ] Platform-specific optimization detection and configuration
     * [ ] Health monitoring initialization with diagnostic setup
     */
    bool initialize(const CircularBufferConfig& config);
    bool initialize(size_t bufferSize, size_t numChannels = 1, uint32_t sampleRate = 44100);
    bool updateConfiguration(const CircularBufferConfig& config);
    bool isInitialized() const;
    CircularBufferConfig getConfiguration() const;

    // TODO 2.4.87: Core Buffer Operations
    // ----------------------------------
    /**
     * TODO: Implement core buffer operations with:
     * [ ] High-performance write operations with zero-copy where possible
     * [ ] Efficient read operations with minimal latency
     * [ ] Atomic operations for thread safety without locks
     * [ ] Bulk operations for improved throughput
     * [ ] Partial operations with continuation support
     * [ ] Overflow/underflow handling with graceful degradation
     * [ ] Memory barriers and synchronization for correctness
     * [ ] SIMD optimizations for data movement
     * [ ] Error detection and recovery mechanisms
     * [ ] Performance monitoring with real-time metrics
     */

    // Write operations
    size_t write(const float* data, size_t sampleCount);
    size_t write(const AudioBuffer& buffer);
    size_t writeNonBlocking(const float* data, size_t sampleCount);
    bool tryWrite(const float* data, size_t sampleCount);
    size_t writePartial(const float* data, size_t sampleCount, size_t& written);

    // Read operations
    size_t read(float* data, size_t sampleCount);
    size_t read(AudioBuffer& buffer);
    size_t readNonBlocking(float* data, size_t sampleCount);
    bool tryRead(float* data, size_t sampleCount);
    size_t readPartial(float* data, size_t sampleCount, size_t& read);

    // Peek operations (read without consuming)
    size_t peek(float* data, size_t sampleCount, size_t offset = 0) const;
    size_t peek(AudioBuffer& buffer, size_t offset = 0) const;

    // Skip operations
    size_t skip(size_t sampleCount);
    bool skipToLatest(size_t& skipped);

    // TODO 2.4.88: Buffer State Management
    // -----------------------------------
    /**
     * TODO: Implement buffer state management with:
     * [ ] Real-time buffer level monitoring with atomic operations
     * [ ] Available space calculation with optimization
     * [ ] Fill ratio monitoring with trend analysis
     * [ ] Buffer reset with proper synchronization
     * [ ] Flush operations with data preservation options
     * [ ] Resize operations with data migration
     * [ ] State queries with minimal overhead
     * [ ] Capacity management with dynamic adjustment
     * [ ] Memory usage optimization with efficient allocation
     * [ ] Thread-safe state transitions with proper synchronization
     */

    // Buffer state queries
    size_t getAvailableForWrite() const;
    size_t getAvailableForRead() const;
    size_t getCurrentLevel() const;
    float getFillRatio() const;
    size_t getCapacity() const;
    size_t getNumChannels() const;
    uint32_t getSampleRate() const;

    // Buffer state management
    void clear();
    void reset();
    bool flush();
    bool resize(size_t newSize);
    bool isEmpty() const;
    bool isFull() const;
    bool isNearOverflow() const;
    bool isNearUnderflow() const;

    // TODO 2.4.89: Performance Monitoring
    // ----------------------------------
    /**
     * TODO: Implement performance monitoring with:
     * [ ] Real-time latency measurement with high precision
     * [ ] Throughput calculation with sliding window averaging
     * [ ] Error rate tracking with trend analysis
     * [ ] Health scoring with multiple metrics combination
     * [ ] Statistics collection with minimal overhead
     * [ ] Performance profiling with detailed breakdowns
     * [ ] Memory usage monitoring with leak detection
     * [ ] Thread contention analysis with bottleneck identification
     * [ ] Historical data tracking with trend analysis
     * [ ] Diagnostic information generation with comprehensive reporting
     */
    CircularBufferStatistics getStatistics() const;
    void resetStatistics();
    float getLatency() const;
    float getThroughput() const;
    float getHealthScore() const;
    bool isHealthy() const;
    std::string getDiagnosticInfo() const;

    // TODO 2.4.90: Advanced Operations
    // -------------------------------
    /**
     * TODO: Implement advanced operations with:
     * [ ] Multi-channel interleaved/planar format support
     * [ ] Sample rate conversion with high-quality resampling
     * [ ] Format conversion with dithering and noise shaping
     * [ ] Zero-crossing detection for glitch-free operations
     * [ ] Fade-in/fade-out for smooth transitions
     * [ ] Synchronization with external clocks and triggers
     * [ ] Timestamping with high-resolution timestamps
     * [ ] Priority-based operations with QoS management
     * [ ] Batch processing with optimized bulk operations
     * [ ] Integration with SIMD instructions for performance
     */

    // Multi-channel operations
    size_t writeInterleaved(const float* data, size_t frameCount, size_t numChannels);
    size_t readInterleaved(float* data, size_t frameCount, size_t numChannels);
    size_t writePlanar(const float* const* channelData, size_t frameCount, size_t numChannels);
    size_t readPlanar(float** channelData, size_t frameCount, size_t numChannels);

    // Advanced processing
    bool synchronize();
    void setTimestamp(uint64_t timestamp);
    uint64_t getTimestamp() const;
    bool fadeIn(size_t sampleCount, float startGain = 0.0f, float endGain = 1.0f);
    bool fadeOut(size_t sampleCount, float startGain = 1.0f, float endGain = 0.0f);

    // TODO 2.4.91: Callback Management
    // -------------------------------
    /**
     * TODO: Implement callback management with:
     * [ ] Thread-safe callback registration and removal
     * [ ] Callback priority management with ordered execution
     * [ ] Conditional callbacks with trigger conditions
     * [ ] Batch callback execution with efficient processing
     * [ ] Error handling in callbacks with isolation
     * [ ] Performance monitoring of callback execution
     * [ ] Memory management for callback storage
     * [ ] Integration with event systems
     * [ ] Custom callback data with type safety
     * [ ] Callback thread safety with proper synchronization
     */
    void setBufferStateCallback(BufferStateCallback callback);
    void setOverflowCallback(OverflowCallback callback);
    void setUnderflowCallback(UnderflowCallback callback);
    void setPerformanceCallback(PerformanceCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setHealthCallback(HealthCallback callback);
    void setStatisticsCallback(StatisticsCallback callback);
    void setResizeCallback(ResizeCallback callback);
    void clearCallbacks();

    // TODO 2.4.92: Error Handling and Diagnostics
    // ------------------------------------------
    /**
     * TODO: Implement error handling and diagnostics with:
     * [ ] Comprehensive error detection with classification
     * [ ] Error recovery mechanisms with automatic retry
     * [ ] Error logging with detailed context information
     * [ ] Health monitoring with predictive analysis
     * [ ] Diagnostic data collection with performance impact
     * [ ] Error rate analysis with trend detection
     * [ ] Recovery strategy selection with adaptive algorithms
     * [ ] System integration for error reporting
     * [ ] User notification with appropriate messaging
     * [ ] Debug information generation with detailed analysis
     */
    CircularBufferError getLastError() const;
    void clearErrors();
    bool hasErrors() const;
    size_t getErrorCount() const;
    float getErrorRate() const;
    std::vector<CircularBufferError> getErrorHistory() const;

    // Public validation utility
    bool validateConfiguration(const CircularBufferConfig& config, std::string& error) const;

  private:
    // TODO 2.4.93: Internal Implementation
    // -----------------------------------
    /**
     * TODO: Implement internal methods with:
     * [ ] Lock-free algorithms with memory ordering guarantees
     * [ ] Memory management with aligned allocation
     * [ ] SIMD optimizations with platform detection
     * [ ] Cache optimization with data structure alignment
     * [ ] Thread synchronization with minimal contention
     * [ ] Performance profiling with low overhead
     * [ ] Error propagation with context preservation
     * [ ] Resource cleanup with proper order
     * [ ] Platform-specific optimizations with feature detection
     * [ ] Integration points with external systems
     */

    // Core implementation
    bool initializeBuffer();
    void cleanupBuffer();
    void updateStatistics();
    void handleError(int code, const std::string& message, const std::string& details = "") const;

    // Buffer operations helpers
    size_t writeInternal(const float* data, size_t sampleCount, bool blocking);
    size_t readInternal(float* data, size_t sampleCount, bool blocking);
    void advanceWritePointer(size_t samples);
    void advanceReadPointer(size_t samples);

    // Performance monitoring helpers
    void recordWriteLatency(float latency);
    void recordReadLatency(float latency);
    void updateHealthScore() const;
    void checkBufferHealth();

    // Threading helpers
    void lockWrite();
    void unlockWrite();
    void lockRead();
    void unlockRead();
    bool tryLockWrite();
    bool tryLockRead();

    // TODO 2.4.94: Member Variables
    // ----------------------------
    /**
     * TODO: Define member variables with:
     * [ ] Buffer storage with aligned memory allocation
     * [ ] Atomic pointers and counters for thread safety
     * [ ] Configuration storage with thread-safe access
     * [ ] Statistics tracking with minimal performance impact
     * [ ] Error tracking with comprehensive information
     * [ ] Callback storage with thread-safe management
     * [ ] Threading primitives with appropriate types
     * [ ] Performance monitoring with high-resolution timing
     * [ ] Memory management with leak detection
     * [ ] Platform-specific optimizations with feature flags
     */

    // Configuration and state
    CircularBufferConfig config_;
    std::atomic<bool> initialized_{false};
    mutable std::mutex configMutex_;

    // Buffer storage
    std::unique_ptr<float, AlignedDeleter> buffer_;
    std::atomic<size_t> bufferSize_{0};
    std::atomic<size_t> numChannels_{1};
    std::atomic<uint32_t> sampleRate_{44100};

    // Pointers and counters (lock-free)
    std::atomic<size_t> writePointer_{0};
    std::atomic<size_t> readPointer_{0};
    std::atomic<size_t> availableData_{0};
    std::atomic<uint64_t> sequenceNumber_{0};
    std::atomic<uint64_t> timestamp_{0};

    // Threading
    mutable std::mutex writeMutex_;
    mutable std::mutex readMutex_;
    std::atomic<bool> writeInProgress_{false};
    std::atomic<bool> readInProgress_{false};

    // Statistics and monitoring
    mutable CircularBufferStatistics statistics_;
    mutable std::mutex statisticsMutex_;
    std::chrono::steady_clock::time_point lastStatsUpdate_;
    std::vector<float> latencyHistory_;

    // Error handling
    mutable CircularBufferError lastError_;
    mutable std::vector<CircularBufferError> errorHistory_;
    mutable std::mutex errorMutex_;

    // Callbacks
    BufferStateCallback bufferStateCallback_;
    OverflowCallback overflowCallback_;
    UnderflowCallback underflowCallback_;
    PerformanceCallback performanceCallback_;
    ErrorCallback errorCallback_;
    HealthCallback healthCallback_;
    StatisticsCallback statisticsCallback_;
    ResizeCallback resizeCallback_;
    mutable std::mutex callbackMutex_;

    // Performance monitoring
    std::chrono::high_resolution_clock::time_point lastOperationTime_;
    std::vector<float> throughputHistory_;
    std::atomic<float> currentThroughput_{0.0f};

    // Health monitoring
    mutable std::atomic<float> healthScore_{1.0f};
    mutable std::atomic<bool> isHealthy_{true};
    std::chrono::steady_clock::time_point lastHealthCheck_;
};

// TODO 2.4.95: Utility Functions
// ------------------------------
/**
 * TODO: Implement utility functions with:
 * [ ] Buffer size calculation with optimal sizing algorithms
 * [ ] Performance benchmarking with standardized tests
 * [ ] Configuration templates with common use cases
 * [ ] Migration helpers with data preservation
 * [ ] Testing utilities with comprehensive validation
 * [ ] Debugging helpers with detailed information
 * [ ] Integration utilities with common frameworks
 * [ ] Performance profiling with detailed analysis
 * [ ] Memory usage analysis with optimization suggestions
 * [ ] Platform-specific utilities with feature detection
 */

// Configuration helpers
CircularBufferConfig createDefaultConfig();
CircularBufferConfig createRealtimeConfig(size_t bufferSize = 4096);
CircularBufferConfig createHighThroughputConfig(size_t bufferSize = 16384);
CircularBufferConfig createLowLatencyConfig(size_t bufferSize = 1024);

// Performance utilities
size_t calculateOptimalBufferSize(uint32_t sampleRate, float targetLatency);
size_t getRecommendedBufferSize(const std::string& useCase);
bool benchmarkPerformance(CircularAudioBuffer& buffer, size_t iterations = 1000);

// Validation utilities
bool validateBufferConfiguration(const CircularBufferConfig& config, std::string& error);
bool testBufferIntegrity(CircularAudioBuffer& buffer);
std::string formatDiagnosticInfo(const CircularBufferStatistics& stats);

}  // namespace core
}  // namespace huntmaster
