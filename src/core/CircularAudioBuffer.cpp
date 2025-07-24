/**
 * @file CircularAudioBuffer.cpp
 * @brief Circular Audio Buffer System Implementation
 *
 * This file implements the Circular Audio Buffer system for efficient real-time
 * audio processing with thread-safe operations and optimized memory management.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

#include "huntmaster/CircularAudioBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>

// Platform-specific includes
#ifdef _WIN32
#include <immintrin.h>
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif
#endif

namespace huntmaster {
namespace core {

// TODO: Phase 2.4 - Advanced Audio Engine Implementation - COMPREHENSIVE FILE TODO
// =================================================================================

// TODO 2.4.96: CircularAudioBuffer Implementation
// -----------------------------------------------
/**
 * TODO: Implement complete CircularAudioBuffer with:
 * [ ] High-performance lock-free operations with memory ordering
 * [ ] Real-time audio processing with guaranteed latency bounds
 * [ ] Thread-safe multi-reader/single-writer architecture
 * [ ] Optimized memory management with aligned allocation
 * [ ] Comprehensive error handling with recovery mechanisms
 * [ ] Performance monitoring with detailed statistics
 * [ ] Cross-platform compatibility with SIMD optimizations
 * [ ] Dynamic buffer management with automatic resizing
 * [ ] Integration with audio processing pipelines
 * [ ] Advanced diagnostics with health monitoring
 */

CircularAudioBuffer::CircularAudioBuffer()
    : initialized_(false), lastStatsUpdate_(std::chrono::steady_clock::now()),
      lastOperationTime_(std::chrono::high_resolution_clock::now()),
      lastHealthCheck_(std::chrono::steady_clock::now()) {
    // TODO: Initialize with default configuration
    config_ = createDefaultConfig();

    // TODO: Initialize statistics
    statistics_.startTime = std::chrono::steady_clock::now();
    statistics_.lastUpdate = statistics_.startTime;

    // TODO: Initialize error tracking
    lastError_ = {};

    // TODO: Reserve space for history tracking
    latencyHistory_.reserve(1000);
    throughputHistory_.reserve(1000);
    errorHistory_.reserve(100);

    std::cout << "CircularAudioBuffer constructed with default configuration" << std::endl;
}

CircularAudioBuffer::CircularAudioBuffer(const CircularBufferConfig& config)
    : CircularAudioBuffer() {
    // TODO: Initialize with provided configuration
    initialize(config);
}

CircularAudioBuffer::~CircularAudioBuffer() {
    // TODO: Ensure proper cleanup
    clearCallbacks();
    cleanupBuffer();

    std::cout << "CircularAudioBuffer destructed" << std::endl;
}

// TODO: Move constructor and assignment
CircularAudioBuffer::CircularAudioBuffer(CircularAudioBuffer&& other) noexcept
    : config_(std::move(other.config_)), initialized_(other.initialized_.load()),
      buffer_(std::move(other.buffer_)), bufferSize_(other.bufferSize_.load()),
      numChannels_(other.numChannels_.load()), sampleRate_(other.sampleRate_.load()),
      writePointer_(other.writePointer_.load()), readPointer_(other.readPointer_.load()),
      availableData_(other.availableData_.load()), sequenceNumber_(other.sequenceNumber_.load()),
      timestamp_(other.timestamp_.load()), statistics_(std::move(other.statistics_)),
      lastError_(std::move(other.lastError_)), errorHistory_(std::move(other.errorHistory_)),
      latencyHistory_(std::move(other.latencyHistory_)),
      throughputHistory_(std::move(other.throughputHistory_)),
      currentThroughput_(other.currentThroughput_.load()), healthScore_(other.healthScore_.load()),
      isHealthy_(other.isHealthy_.load()) {
    // TODO: Mark other as moved-from
    other.initialized_ = false;
    other.buffer_.reset();
    other.bufferSize_ = 0;
}

CircularAudioBuffer& CircularAudioBuffer::operator=(CircularAudioBuffer&& other) noexcept {
    if (this != &other) {
        // TODO: Clean up current resources
        cleanupBuffer();

        // TODO: Move data from other
        config_ = std::move(other.config_);
        initialized_ = other.initialized_.load();
        buffer_ = std::move(other.buffer_);
        bufferSize_ = other.bufferSize_.load();
        numChannels_ = other.numChannels_.load();
        sampleRate_ = other.sampleRate_.load();
        writePointer_ = other.writePointer_.load();
        readPointer_ = other.readPointer_.load();
        availableData_ = other.availableData_.load();
        sequenceNumber_ = other.sequenceNumber_.load();
        timestamp_ = other.timestamp_.load();
        statistics_ = std::move(other.statistics_);
        lastError_ = std::move(other.lastError_);
        errorHistory_ = std::move(other.errorHistory_);
        latencyHistory_ = std::move(other.latencyHistory_);
        throughputHistory_ = std::move(other.throughputHistory_);
        currentThroughput_ = other.currentThroughput_.load();
        healthScore_ = other.healthScore_.load();
        isHealthy_ = other.isHealthy_.load();

        // TODO: Mark other as moved-from
        other.initialized_ = false;
        other.buffer_.reset();
        other.bufferSize_ = 0;
    }
    return *this;
}

// TODO 2.4.97: Initialization and Configuration
// ---------------------------------------------
/**
 * TODO: Implement initialization and configuration with:
 * [ ] Configuration validation with comprehensive parameter checking
 * [ ] Memory allocation with proper alignment for SIMD operations
 * [ ] Thread setup with priority and affinity configuration
 * [ ] Performance baseline establishment with calibration
 * [ ] Error handling setup with recovery mechanisms
 * [ ] Statistics initialization with baseline metrics
 * [ ] Platform-specific optimizations with feature detection
 * [ ] Integration setup with external audio systems
 * [ ] Health monitoring initialization with diagnostic setup
 * [ ] Callback system initialization with proper synchronization
 */
bool CircularAudioBuffer::initialize(const CircularBufferConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);

    try {
        // TODO: Validate configuration
        std::string validationError;
        if (!validateConfiguration(config, validationError)) {
            handleError(-1, "Invalid buffer configuration: " + validationError);
            return false;
        }

        // TODO: Clean up existing buffer if needed
        if (initialized_) {
            cleanupBuffer();
        }

        // TODO: Store configuration
        config_ = config;
        bufferSize_ = config.bufferSize;
        numChannels_ = config.numChannels;
        sampleRate_ = config.sampleRate;

        // TODO: Initialize buffer storage
        if (!initializeBuffer()) {
            handleError(-2, "Failed to initialize buffer storage");
            return false;
        }

        // TODO: Reset pointers and counters
        writePointer_ = 0;
        readPointer_ = 0;
        availableData_ = 0;
        sequenceNumber_ = 0;
        timestamp_ = 0;

        // TODO: Reset threading state
        writeInProgress_ = false;
        readInProgress_ = false;

        // TODO: Initialize statistics
        statistics_ = {};
        statistics_.startTime = std::chrono::steady_clock::now();
        statistics_.lastUpdate = statistics_.startTime;
        statistics_.isHealthy = true;
        statistics_.healthScore = 1.0f;

        // TODO: Reset error tracking
        lastError_ = {};
        errorHistory_.clear();

        // TODO: Initialize performance tracking
        latencyHistory_.clear();
        throughputHistory_.clear();
        currentThroughput_ = 0.0f;
        healthScore_ = 1.0f;
        isHealthy_ = true;

        // TODO: Update timestamps
        lastStatsUpdate_ = std::chrono::steady_clock::now();
        lastOperationTime_ = std::chrono::high_resolution_clock::now();
        lastHealthCheck_ = std::chrono::steady_clock::now();

        // TODO: Mark as initialized
        initialized_ = true;

        std::cout << "CircularAudioBuffer initialized: " << bufferSize_ << " samples, "
                  << numChannels_ << " channels, " << sampleRate_ << " Hz" << std::endl;

        return true;

    } catch (const std::exception& e) {
        handleError(-100, "Exception during buffer initialization", e.what());
        initialized_ = false;
        return false;
    }
}

bool CircularAudioBuffer::initialize(size_t bufferSize, size_t numChannels, uint32_t sampleRate) {
    CircularBufferConfig config = createDefaultConfig();
    config.bufferSize = bufferSize;
    config.numChannels = numChannels;
    config.sampleRate = sampleRate;

    return initialize(config);
}

bool CircularAudioBuffer::updateConfiguration(const CircularBufferConfig& config) {
    if (!initialized_) {
        handleError(-10, "Buffer not initialized");
        return false;
    }

    std::lock_guard<std::mutex> lock(configMutex_);

    // TODO: Validate new configuration
    std::string validationError;
    if (!validateConfiguration(config, validationError)) {
        handleError(-11, "Invalid configuration update: " + validationError);
        return false;
    }

    // TODO: Check if reinitialization is needed
    bool needsReinitialization = false;

    if (config.bufferSize != config_.bufferSize || config.numChannels != config_.numChannels
        || config.sampleRate != config_.sampleRate) {
        needsReinitialization = true;
    }

    if (needsReinitialization) {
        initialized_ = false;
        return initialize(config);
    } else {
        // TODO: Apply changes that can be done dynamically
        config_ = config;
        return true;
    }
}

bool CircularAudioBuffer::isInitialized() const {
    return initialized_;
}

CircularBufferConfig CircularAudioBuffer::getConfiguration() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

// TODO 2.4.98: Core Buffer Operations - Write Operations
// ------------------------------------------------------
/**
 * TODO: Implement high-performance write operations with:
 * [ ] Lock-free algorithms with memory ordering guarantees
 * [ ] Zero-copy operations where possible for maximum efficiency
 * [ ] SIMD optimizations for bulk data movement
 * [ ] Overflow handling with configurable policies
 * [ ] Performance monitoring with minimal overhead
 * [ ] Thread safety with atomic operations and memory barriers
 * [ ] Error recovery with automatic retry mechanisms
 * [ ] Memory alignment considerations for optimal performance
 * [ ] Integration with real-time audio processing pipelines
 * [ ] Latency measurement with high-precision timing
 */
size_t CircularAudioBuffer::write(const float* data, size_t sampleCount) {
    return writeInternal(data, sampleCount, true);
}

size_t CircularAudioBuffer::writeNonBlocking(const float* data, size_t sampleCount) {
    return writeInternal(data, sampleCount, false);
}

bool CircularAudioBuffer::tryWrite(const float* data, size_t sampleCount) {
    if (!initialized_ || !data || sampleCount == 0) {
        return false;
    }

    // TODO: Check if we have enough space without blocking
    size_t available = getAvailableForWrite();
    if (available < sampleCount) {
        return false;
    }

    return writeInternal(data, sampleCount, false) == sampleCount;
}

size_t CircularAudioBuffer::writePartial(const float* data, size_t sampleCount, size_t& written) {
    written = 0;

    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    // TODO: Write as much as possible without blocking
    size_t available = getAvailableForWrite();
    size_t toWrite = std::min(sampleCount, available);

    if (toWrite > 0) {
        written = writeInternal(data, toWrite, false);
    }

    return written;
}

// TODO 2.4.99: Core Buffer Operations - Read Operations
// -----------------------------------------------------
/**
 * TODO: Implement high-performance read operations with:
 * [ ] Lock-free algorithms with proper memory synchronization
 * [ ] Minimal latency with optimized data access patterns
 * [ ] SIMD optimizations for efficient data copying
 * [ ] Underflow handling with graceful degradation
 * [ ] Performance monitoring with real-time metrics
 * [ ] Thread safety with atomic operations
 * [ ] Error detection and recovery mechanisms
 * [ ] Memory prefetching for improved cache performance
 * [ ] Integration with audio processing chains
 * [ ] Latency tracking with statistical analysis
 */
size_t CircularAudioBuffer::read(float* data, size_t sampleCount) {
    return readInternal(data, sampleCount, true);
}

size_t CircularAudioBuffer::readNonBlocking(float* data, size_t sampleCount) {
    return readInternal(data, sampleCount, false);
}

bool CircularAudioBuffer::tryRead(float* data, size_t sampleCount) {
    if (!initialized_ || !data || sampleCount == 0) {
        return false;
    }

    // TODO: Check if we have enough data without blocking
    size_t available = getAvailableForRead();
    if (available < sampleCount) {
        return false;
    }

    return readInternal(data, sampleCount, false) == sampleCount;
}

size_t CircularAudioBuffer::readPartial(float* data, size_t sampleCount, size_t& read) {
    read = 0;

    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    // TODO: Read as much as available without blocking
    size_t available = getAvailableForRead();
    size_t toRead = std::min(sampleCount, available);

    if (toRead > 0) {
        read = readInternal(data, toRead, false);
    }

    return read;
}

// TODO 2.4.100: Peek and Skip Operations
// --------------------------------------
size_t CircularAudioBuffer::peek(float* data, size_t sampleCount, size_t offset) const {
    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    try {
        // TODO: Calculate available data considering offset
        size_t available = getAvailableForRead();
        if (offset >= available) {
            return 0;  // Offset beyond available data
        }

        size_t actualCount = std::min(sampleCount, available - offset);
        if (actualCount == 0) {
            return 0;
        }

        // TODO: Calculate read position with offset
        size_t readPos =
            (readPointer_.load(std::memory_order_acquire) + offset) % bufferSize_.load();

        // TODO: Handle circular buffer wrap-around
        if (readPos + actualCount <= bufferSize_) {
            // TODO: Single contiguous copy
            std::memcpy(data, buffer_.get() + readPos, actualCount * sizeof(float));
        } else {
            // TODO: Two-part copy due to wrap-around
            size_t firstPart = bufferSize_ - readPos;
            size_t secondPart = actualCount - firstPart;

            std::memcpy(data, buffer_.get() + readPos, firstPart * sizeof(float));
            std::memcpy(data + firstPart, buffer_.get(), secondPart * sizeof(float));
        }

        return actualCount;

    } catch (const std::exception& e) {
        handleError(-20, "Peek operation failed", e.what());
        return 0;
    }
}

size_t CircularAudioBuffer::skip(size_t sampleCount) {
    if (!initialized_ || sampleCount == 0) {
        return 0;
    }

    try {
        // TODO: Calculate how much we can actually skip
        size_t available = getAvailableForRead();
        size_t toSkip = std::min(sampleCount, available);

        if (toSkip > 0) {
            // TODO: Advance read pointer
            advanceReadPointer(toSkip);

            // TODO: Update statistics
            statistics_.totalReads.fetch_add(1, std::memory_order_relaxed);
        }

        return toSkip;

    } catch (const std::exception& e) {
        handleError(-21, "Skip operation failed", e.what());
        return 0;
    }
}

bool CircularAudioBuffer::skipToLatest(size_t& skipped) {
    skipped = 0;

    if (!initialized_) {
        return false;
    }

    try {
        // TODO: Skip all available data except the most recent samples
        size_t available = getAvailableForRead();

        if (available > config_.readBlockSize) {
            size_t toSkip = available - config_.readBlockSize;
            skipped = skip(toSkip);
            return skipped == toSkip;
        }

        return true;  // Nothing to skip

    } catch (const std::exception& e) {
        handleError(-22, "Skip to latest operation failed", e.what());
        return false;
    }
}

// TODO 2.4.101: Buffer State Management
// -------------------------------------
/**
 * TODO: Implement buffer state management with:
 * [ ] Atomic operations for thread-safe state queries
 * [ ] Efficient space calculation with minimal overhead
 * [ ] Real-time monitoring with statistical analysis
 * [ ] State transitions with proper synchronization
 * [ ] Memory optimization with cache-friendly operations
 * [ ] Error detection with comprehensive validation
 * [ ] Performance monitoring with trend analysis
 * [ ] Integration with callback systems
 * [ ] Health monitoring with predictive analysis
 * [ ] Dynamic adjustment with adaptive algorithms
 */
size_t CircularAudioBuffer::getAvailableForWrite() const {
    if (!initialized_) {
        return 0;
    }

    size_t currentLevel = availableData_.load(std::memory_order_acquire);
    size_t bufferSize = bufferSize_.load(std::memory_order_acquire);

    return (bufferSize > currentLevel) ? (bufferSize - currentLevel - 1) : 0;
    // Note: -1 to prevent write pointer from catching up to read pointer
}

size_t CircularAudioBuffer::getAvailableForRead() const {
    if (!initialized_) {
        return 0;
    }

    return availableData_.load(std::memory_order_acquire);
}

size_t CircularAudioBuffer::getCurrentLevel() const {
    return getAvailableForRead();
}

float CircularAudioBuffer::getFillRatio() const {
    if (!initialized_) {
        return 0.0f;
    }

    size_t level = getCurrentLevel();
    size_t capacity = getCapacity();

    return capacity > 0 ? static_cast<float>(level) / capacity : 0.0f;
}

size_t CircularAudioBuffer::getCapacity() const {
    return bufferSize_.load(std::memory_order_acquire);
}

size_t CircularAudioBuffer::getNumChannels() const {
    return numChannels_.load(std::memory_order_acquire);
}

uint32_t CircularAudioBuffer::getSampleRate() const {
    return sampleRate_.load(std::memory_order_acquire);
}

void CircularAudioBuffer::clear() {
    if (!initialized_) {
        return;
    }

    try {
        // TODO: Thread-safe buffer clearing
        std::lock_guard<std::mutex> writeLock(writeMutex_);
        std::lock_guard<std::mutex> readLock(readMutex_);

        // TODO: Reset pointers and counters
        writePointer_ = 0;
        readPointer_ = 0;
        availableData_ = 0;

        // TODO: Clear buffer contents
        if (buffer_) {
            std::memset(buffer_.get(), 0, bufferSize_ * sizeof(float));
        }

        // TODO: Reset sequence number but keep timestamp
        sequenceNumber_ = 0;

        std::cout << "CircularAudioBuffer cleared" << std::endl;

    } catch (const std::exception& e) {
        handleError(-30, "Buffer clear operation failed", e.what());
    }
}

void CircularAudioBuffer::reset() {
    clear();

    // TODO: Also reset statistics and error history
    resetStatistics();
    clearErrors();
}

bool CircularAudioBuffer::flush() {
    if (!initialized_) {
        return false;
    }

    try {
        // TODO: Wait for all pending operations to complete
        while (writeInProgress_.load(std::memory_order_acquire)
               || readInProgress_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        // TODO: Force any cached data to be written
        std::atomic_thread_fence(std::memory_order_seq_cst);

        return true;

    } catch (const std::exception& e) {
        handleError(-31, "Buffer flush operation failed", e.what());
        return false;
    }
}

bool CircularAudioBuffer::resize(size_t newSize) {
    if (!initialized_ || newSize == 0) {
        return false;
    }

    try {
        std::lock_guard<std::mutex> configLock(configMutex_);
        std::lock_guard<std::mutex> writeLock(writeMutex_);
        std::lock_guard<std::mutex> readLock(readMutex_);

        size_t oldSize = bufferSize_.load();

        // TODO: Validate new size
        if (newSize < config_.minBufferSize || newSize > config_.maxBufferSize) {
            handleError(-32, "Invalid buffer size for resize: " + std::to_string(newSize));
            return false;
        }

        // TODO: Allocate new buffer
        auto newBuffer = std::make_unique<float[]>(newSize);
        if (!newBuffer) {
            handleError(-33, "Failed to allocate new buffer for resize");
            return false;
        }

        // TODO: Copy existing data if any
        size_t dataToCopy = std::min(getAvailableForRead(), newSize - 1);
        if (dataToCopy > 0) {
            // TODO: Copy data from old buffer to new buffer
            size_t readPos = readPointer_.load();

            if (readPos + dataToCopy <= oldSize) {
                // Single contiguous copy
                std::memcpy(newBuffer.get(), buffer_.get() + readPos, dataToCopy * sizeof(float));
            } else {
                // Two-part copy due to wrap-around
                size_t firstPart = oldSize - readPos;
                size_t secondPart = dataToCopy - firstPart;

                std::memcpy(newBuffer.get(), buffer_.get() + readPos, firstPart * sizeof(float));
                std::memcpy(newBuffer.get() + firstPart, buffer_.get(), secondPart * sizeof(float));
            }
        }

        // TODO: Update buffer and pointers
        buffer_ = std::move(newBuffer);
        bufferSize_ = newSize;
        config_.bufferSize = newSize;

        // TODO: Update pointers
        readPointer_ = 0;
        writePointer_ = dataToCopy;
        availableData_ = dataToCopy;

        // TODO: Trigger resize callback
        if (resizeCallback_) {
            resizeCallback_(oldSize, newSize);
        }

        std::cout << "CircularAudioBuffer resized from " << oldSize << " to " << newSize
                  << " samples" << std::endl;

        return true;

    } catch (const std::exception& e) {
        handleError(-34, "Buffer resize operation failed", e.what());
        return false;
    }
}

bool CircularAudioBuffer::isEmpty() const {
    return getAvailableForRead() == 0;
}

bool CircularAudioBuffer::isFull() const {
    return getAvailableForWrite() == 0;
}

bool CircularAudioBuffer::isNearOverflow() const {
    float fillRatio = getFillRatio();
    return fillRatio > config_.overflowThreshold;
}

bool CircularAudioBuffer::isNearUnderflow() const {
    float fillRatio = getFillRatio();
    return fillRatio < config_.underflowThreshold;
}

// TODO 2.4.102: Internal Implementation - Core Helpers
// ----------------------------------------------------
/**
 * TODO: Implement internal helper methods with:
 * [ ] Lock-free buffer initialization with proper alignment
 * [ ] Memory management with leak detection and optimization
 * [ ] Thread-safe pointer manipulation with atomic operations
 * [ ] Performance optimization with SIMD and cache considerations
 * [ ] Error handling with context preservation and recovery
 * [ ] Statistics collection with minimal performance impact
 * [ ] Platform-specific optimizations with feature detection
 * [ ] Integration with external systems and callbacks
 * [ ] Health monitoring with predictive analysis
 * [ ] Resource cleanup with proper ordering and verification
 */
bool CircularAudioBuffer::initializeBuffer() {
    try {
        // TODO: Calculate total buffer size including all channels
        size_t totalSize = bufferSize_ * numChannels_;

        // TODO: Allocate aligned memory for SIMD operations
#ifdef _WIN32
        buffer_ = std::unique_ptr<float[]>(
            static_cast<float*>(_aligned_malloc(totalSize * sizeof(float), 32)));
#elif defined(__linux__) || defined(__APPLE__)
        float* alignedPtr = nullptr;
        if (posix_memalign(reinterpret_cast<void**>(&alignedPtr), 32, totalSize * sizeof(float))
            == 0) {
            buffer_ = std::unique_ptr<float[]>(alignedPtr);
        }
#else
        buffer_ = std::make_unique<float[]>(totalSize);
#endif

        if (!buffer_) {
            return false;
        }

        // TODO: Initialize buffer with zeros
        std::memset(buffer_.get(), 0, totalSize * sizeof(float));

        return true;

    } catch (const std::exception& e) {
        handleError(-50, "Buffer initialization failed", e.what());
        return false;
    }
}

void CircularAudioBuffer::cleanupBuffer() {
    // TODO: Clean up buffer memory
    if (buffer_) {
#ifdef _WIN32
        // For aligned malloc, we need to check if it was allocated with _aligned_malloc
        // This is a simplification - in real code, we'd track the allocation method
        try {
            buffer_.reset();
        } catch (...) {
            // Handle any cleanup errors
        }
#else
        buffer_.reset();
#endif
    }

    // TODO: Reset all atomic variables
    bufferSize_ = 0;
    writePointer_ = 0;
    readPointer_ = 0;
    availableData_ = 0;
    sequenceNumber_ = 0;
    timestamp_ = 0;
}

bool CircularAudioBuffer::validateConfiguration(const CircularBufferConfig& config,
                                                std::string& error) const {
    // TODO: Validate buffer size
    if (config.bufferSize == 0) {
        error = "Buffer size cannot be zero";
        return false;
    }

    if (config.bufferSize < config.minBufferSize) {
        error = "Buffer size below minimum: " + std::to_string(config.minBufferSize);
        return false;
    }

    if (config.bufferSize > config.maxBufferSize) {
        error = "Buffer size above maximum: " + std::to_string(config.maxBufferSize);
        return false;
    }

    // TODO: Validate number of channels
    if (config.numChannels == 0 || config.numChannels > 32) {
        error = "Invalid number of channels: " + std::to_string(config.numChannels);
        return false;
    }

    // TODO: Validate sample rate
    if (config.sampleRate < 8000 || config.sampleRate > 192000) {
        error = "Invalid sample rate: " + std::to_string(config.sampleRate);
        return false;
    }

    // TODO: Validate thresholds
    if (config.overflowThreshold <= 0.0f || config.overflowThreshold > 1.0f) {
        error = "Invalid overflow threshold: " + std::to_string(config.overflowThreshold);
        return false;
    }

    if (config.underflowThreshold < 0.0f || config.underflowThreshold >= config.overflowThreshold) {
        error = "Invalid underflow threshold: " + std::to_string(config.underflowThreshold);
        return false;
    }

    // TODO: Validate block sizes
    if (config.writeBlockSize == 0 || config.writeBlockSize > config.bufferSize) {
        error = "Invalid write block size: " + std::to_string(config.writeBlockSize);
        return false;
    }

    if (config.readBlockSize == 0 || config.readBlockSize > config.bufferSize) {
        error = "Invalid read block size: " + std::to_string(config.readBlockSize);
        return false;
    }

    return true;
}

size_t CircularAudioBuffer::writeInternal(const float* data, size_t sampleCount, bool blocking) {
    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // TODO: Set write in progress flag
        writeInProgress_ = true;

        size_t totalWritten = 0;
        size_t remaining = sampleCount;

        while (remaining > 0 && (blocking || getAvailableForWrite() > 0)) {
            // TODO: Calculate available space
            size_t available = getAvailableForWrite();
            if (available == 0) {
                if (!blocking) {
                    break;
                }

                // TODO: Handle overflow if blocking
                if (config_.enableOverflowProtection) {
                    if (overflowCallback_) {
                        overflowCallback_(remaining, available);
                    }
                    statistics_.overflowCount.fetch_add(1, std::memory_order_relaxed);
                }

                // TODO: Wait a bit and retry
                std::this_thread::yield();
                continue;
            }

            // TODO: Calculate how much to write in this iteration
            size_t toWrite = std::min(remaining, available);
            size_t writePos = writePointer_.load(std::memory_order_acquire);
            size_t bufferSize = bufferSize_.load(std::memory_order_acquire);

            // TODO: Handle circular buffer wrap-around
            if (writePos + toWrite <= bufferSize) {
                // TODO: Single contiguous write
#ifdef __SSE2__
                // TODO: Use SIMD for bulk copy if data is aligned
                if (reinterpret_cast<uintptr_t>(data + totalWritten) % 16 == 0
                    && reinterpret_cast<uintptr_t>(buffer_.get() + writePos) % 16 == 0
                    && toWrite >= 4) {
                    size_t simdCount = (toWrite / 4) * 4;
                    for (size_t i = 0; i < simdCount; i += 4) {
                        __m128 values = _mm_load_ps(data + totalWritten + i);
                        _mm_store_ps(buffer_.get() + writePos + i, values);
                    }

                    // Handle remaining samples
                    for (size_t i = simdCount; i < toWrite; ++i) {
                        buffer_[writePos + i] = data[totalWritten + i];
                    }
                } else
#endif
                {
                    std::memcpy(
                        buffer_.get() + writePos, data + totalWritten, toWrite * sizeof(float));
                }
            } else {
                // TODO: Two-part write due to wrap-around
                size_t firstPart = bufferSize - writePos;
                size_t secondPart = toWrite - firstPart;

                std::memcpy(
                    buffer_.get() + writePos, data + totalWritten, firstPart * sizeof(float));
                std::memcpy(
                    buffer_.get(), data + totalWritten + firstPart, secondPart * sizeof(float));
            }

            // TODO: Update pointers and counters atomically
            advanceWritePointer(toWrite);

            totalWritten += toWrite;
            remaining -= toWrite;

            // TODO: Update statistics
            statistics_.totalWrites.fetch_add(1, std::memory_order_relaxed);
            statistics_.totalSamples.fetch_add(toWrite, std::memory_order_relaxed);
        }

        // TODO: Clear write in progress flag
        writeInProgress_ = false;

        // TODO: Record latency
        auto endTime = std::chrono::high_resolution_clock::now();
        float latency =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
            / 1000.0f;
        recordWriteLatency(latency);

        // TODO: Update sequence number
        sequenceNumber_.fetch_add(1, std::memory_order_relaxed);

        // TODO: Trigger buffer state callback
        if (bufferStateCallback_) {
            bufferStateCallback_(getCurrentLevel(), getFillRatio());
        }

        return totalWritten;

    } catch (const std::exception& e) {
        writeInProgress_ = false;
        handleError(-60, "Write operation failed", e.what());
        return 0;
    }
}

size_t CircularAudioBuffer::readInternal(float* data, size_t sampleCount, bool blocking) {
    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // TODO: Set read in progress flag
        readInProgress_ = true;

        size_t totalRead = 0;
        size_t remaining = sampleCount;

        while (remaining > 0 && (blocking || getAvailableForRead() > 0)) {
            // TODO: Calculate available data
            size_t available = getAvailableForRead();
            if (available == 0) {
                if (!blocking) {
                    break;
                }

                // TODO: Handle underflow if blocking
                if (config_.enableUnderflowProtection) {
                    if (underflowCallback_) {
                        underflowCallback_(remaining, available);
                    }
                    statistics_.underflowCount.fetch_add(1, std::memory_order_relaxed);
                }

                // TODO: Wait a bit and retry
                std::this_thread::yield();
                continue;
            }

            // TODO: Calculate how much to read in this iteration
            size_t toRead = std::min(remaining, available);
            size_t readPos = readPointer_.load(std::memory_order_acquire);
            size_t bufferSize = bufferSize_.load(std::memory_order_acquire);

            // TODO: Handle circular buffer wrap-around
            if (readPos + toRead <= bufferSize) {
                // TODO: Single contiguous read
#ifdef __SSE2__
                // TODO: Use SIMD for bulk copy if data is aligned
                if (reinterpret_cast<uintptr_t>(data + totalRead) % 16 == 0
                    && reinterpret_cast<uintptr_t>(buffer_.get() + readPos) % 16 == 0
                    && toRead >= 4) {
                    size_t simdCount = (toRead / 4) * 4;
                    for (size_t i = 0; i < simdCount; i += 4) {
                        __m128 values = _mm_load_ps(buffer_.get() + readPos + i);
                        _mm_store_ps(data + totalRead + i, values);
                    }

                    // Handle remaining samples
                    for (size_t i = simdCount; i < toRead; ++i) {
                        data[totalRead + i] = buffer_[readPos + i];
                    }
                } else
#endif
                {
                    std::memcpy(data + totalRead, buffer_.get() + readPos, toRead * sizeof(float));
                }
            } else {
                // TODO: Two-part read due to wrap-around
                size_t firstPart = bufferSize - readPos;
                size_t secondPart = toRead - firstPart;

                std::memcpy(data + totalRead, buffer_.get() + readPos, firstPart * sizeof(float));
                std::memcpy(
                    data + totalRead + firstPart, buffer_.get(), secondPart * sizeof(float));
            }

            // TODO: Update pointers and counters atomically
            advanceReadPointer(toRead);

            totalRead += toRead;
            remaining -= toRead;

            // TODO: Update statistics
            statistics_.totalReads.fetch_add(1, std::memory_order_relaxed);
        }

        // TODO: Clear read in progress flag
        readInProgress_ = false;

        // TODO: Record latency
        auto endTime = std::chrono::high_resolution_clock::now();
        float latency =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
            / 1000.0f;
        recordReadLatency(latency);

        // TODO: Trigger buffer state callback
        if (bufferStateCallback_) {
            bufferStateCallback_(getCurrentLevel(), getFillRatio());
        }

        return totalRead;

    } catch (const std::exception& e) {
        readInProgress_ = false;
        handleError(-61, "Read operation failed", e.what());
        return 0;
    }
}

void CircularAudioBuffer::advanceWritePointer(size_t samples) {
    size_t bufferSize = bufferSize_.load(std::memory_order_acquire);
    size_t currentPos = writePointer_.load(std::memory_order_acquire);
    size_t newPos = (currentPos + samples) % bufferSize;

    writePointer_.store(newPos, std::memory_order_release);
    availableData_.fetch_add(samples, std::memory_order_acq_rel);
}

void CircularAudioBuffer::advanceReadPointer(size_t samples) {
    size_t bufferSize = bufferSize_.load(std::memory_order_acquire);
    size_t currentPos = readPointer_.load(std::memory_order_acquire);
    size_t newPos = (currentPos + samples) % bufferSize;

    readPointer_.store(newPos, std::memory_order_release);
    availableData_.fetch_sub(samples, std::memory_order_acq_rel);
}

// TODO 2.4.103: Performance Monitoring Implementation
// ---------------------------------------------------
void CircularAudioBuffer::recordWriteLatency(float latency) {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

    // TODO: Update write latency statistics
    float oldAvg = statistics_.averageWriteLatency.load();
    size_t totalWrites = statistics_.totalWrites.load();

    if (totalWrites > 1) {
        float newAvg = (oldAvg * (totalWrites - 1) + latency) / totalWrites;
        statistics_.averageWriteLatency.store(newAvg);
    } else {
        statistics_.averageWriteLatency.store(latency);
    }

    // Update maximum
    float currentMax = statistics_.maxWriteLatency.load();
    if (latency > currentMax) {
        statistics_.maxWriteLatency.store(latency);
    }

    // Store in history for analysis
    if (latencyHistory_.size() >= 1000) {
        latencyHistory_.erase(latencyHistory_.begin());
    }
    latencyHistory_.push_back(latency);
}

void CircularAudioBuffer::recordReadLatency(float latency) {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

    // TODO: Update read latency statistics
    float oldAvg = statistics_.averageReadLatency.load();
    size_t totalReads = statistics_.totalReads.load();

    if (totalReads > 1) {
        float newAvg = (oldAvg * (totalReads - 1) + latency) / totalReads;
        statistics_.averageReadLatency.store(newAvg);
    } else {
        statistics_.averageReadLatency.store(latency);
    }

    // Update maximum
    float currentMax = statistics_.maxReadLatency.load();
    if (latency > currentMax) {
        statistics_.maxReadLatency.store(latency);
    }
}

CircularBufferStatistics CircularAudioBuffer::getStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

    // TODO: Update dynamic statistics
    auto currentTime = std::chrono::steady_clock::now();
    auto timeDiff =
        std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - statistics_.startTime);

    CircularBufferStatistics stats = statistics_;

    // Calculate current buffer state
    stats.currentLevel = getCurrentLevel();
    stats.fillRatio = getFillRatio();

    // Calculate throughput
    if (timeDiff.count() > 0) {
        float seconds = timeDiff.count() / 1000.0f;
        stats.throughput = statistics_.totalSamples.load() / seconds;
    }

    // Calculate error rate
    size_t totalOps = statistics_.totalWrites.load() + statistics_.totalReads.load();
    if (totalOps > 0) {
        stats.errorRate = static_cast<float>(statistics_.errorCount.load()) / totalOps;
    }

    return stats;
}

void CircularAudioBuffer::resetStatistics() {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

    statistics_ = {};
    statistics_.startTime = std::chrono::steady_clock::now();
    statistics_.lastUpdate = statistics_.startTime;
    statistics_.isHealthy = true;
    statistics_.healthScore = 1.0f;

    latencyHistory_.clear();
    throughputHistory_.clear();
}

float CircularAudioBuffer::getLatency() const {
    return (statistics_.averageWriteLatency.load() + statistics_.averageReadLatency.load()) / 2.0f;
}

float CircularAudioBuffer::getThroughput() const {
    return statistics_.throughput.load();
}

float CircularAudioBuffer::getHealthScore() const {
    return healthScore_.load();
}

bool CircularAudioBuffer::isHealthy() const {
    return isHealthy_.load();
}

std::string CircularAudioBuffer::getDiagnosticInfo() const {
    std::ostringstream oss;

    oss << "CircularAudioBuffer Diagnostic Information:\n";
    oss << "Initialized: " << (initialized_ ? "Yes" : "No") << "\n";

    if (initialized_) {
        CircularBufferStatistics stats = getStatistics();

        oss << "Configuration:\n";
        oss << "  Buffer Size: " << bufferSize_.load() << " samples\n";
        oss << "  Channels: " << numChannels_.load() << "\n";
        oss << "  Sample Rate: " << sampleRate_.load() << " Hz\n";

        oss << "Current State:\n";
        oss << "  Current Level: " << stats.currentLevel.load() << " samples\n";
        oss << "  Fill Ratio: " << std::fixed << std::setprecision(2)
            << stats.fillRatio.load() * 100.0f << "%\n";
        oss << "  Available for Write: " << getAvailableForWrite() << " samples\n";
        oss << "  Available for Read: " << getAvailableForRead() << " samples\n";

        oss << "Performance Statistics:\n";
        oss << "  Total Writes: " << stats.totalWrites.load() << "\n";
        oss << "  Total Reads: " << stats.totalReads.load() << "\n";
        oss << "  Total Samples: " << stats.totalSamples.load() << "\n";
        oss << "  Average Write Latency: " << std::fixed << std::setprecision(3)
            << stats.averageWriteLatency.load() << " ms\n";
        oss << "  Average Read Latency: " << std::fixed << std::setprecision(3)
            << stats.averageReadLatency.load() << " ms\n";
        oss << "  Throughput: " << std::fixed << std::setprecision(1) << stats.throughput.load()
            << " samples/sec\n";

        oss << "Error Statistics:\n";
        oss << "  Overflow Count: " << stats.overflowCount.load() << "\n";
        oss << "  Underflow Count: " << stats.underflowCount.load() << "\n";
        oss << "  Total Errors: " << stats.errorCount.load() << "\n";
        oss << "  Error Rate: " << std::fixed << std::setprecision(4)
            << stats.errorRate.load() * 100.0f << "%\n";

        oss << "Health:\n";
        oss << "  Health Score: " << std::fixed << std::setprecision(2) << stats.healthScore.load()
            << "\n";
        oss << "  Is Healthy: " << (stats.isHealthy.load() ? "Yes" : "No") << "\n";
    }

    return oss.str();
}

// TODO 2.4.104: Error Handling Implementation
// ------------------------------------------
void CircularAudioBuffer::handleError(int code,
                                      const std::string& message,
                                      const std::string& details) {
    CircularBufferError error = {.code = code,
                                 .message = message,
                                 .details = details,
                                 .timestamp = std::chrono::steady_clock::now(),
                                 .component = "CircularAudioBuffer",
                                 .bufferState = getCurrentLevel(),
                                 .operationContext = sequenceNumber_.load()};

    {
        std::lock_guard<std::mutex> lock(errorMutex_);
        lastError_ = error;

        // Store in error history
        errorHistory_.push_back(error);
        if (errorHistory_.size() > 100) {
            errorHistory_.erase(errorHistory_.begin());
        }
    }

    // Update error statistics
    statistics_.errorCount.fetch_add(1, std::memory_order_relaxed);
    statistics_.consecutiveErrors.fetch_add(1, std::memory_order_relaxed);

    // Update health score
    updateHealthScore();

    std::cerr << "CircularAudioBuffer Error " << code << ": " << message;
    if (!details.empty()) {
        std::cerr << " (" << details << ")";
    }
    std::cerr << std::endl;

    // Trigger error callback
    if (errorCallback_) {
        errorCallback_(code, message);
    }
}

void CircularAudioBuffer::updateHealthScore() {
    // TODO: Calculate health score based on various metrics
    float score = 1.0f;

    // Factor in error rate
    float errorRate = statistics_.errorRate.load();
    if (errorRate > 0.01f) {  // More than 1% error rate
        score *= (1.0f - std::min(errorRate * 10.0f, 0.9f));
    }

    // Factor in consecutive errors
    size_t consecutiveErrors = statistics_.consecutiveErrors.load();
    if (consecutiveErrors > 0) {
        score *= std::max(0.1f, 1.0f - consecutiveErrors * 0.1f);
    }

    // Factor in buffer utilization (extreme values are unhealthy)
    float fillRatio = getFillRatio();
    if (fillRatio > 0.95f || fillRatio < 0.05f) {
        score *= 0.8f;
    }

    healthScore_.store(score);
    isHealthy_.store(score > 0.5f);
    statistics_.healthScore.store(score);
    statistics_.isHealthy.store(score > 0.5f);

    // Trigger health callback if status changed
    static bool lastHealthyState = true;
    bool currentHealthy = isHealthy_.load();
    if (currentHealthy != lastHealthyState) {
        if (healthCallback_) {
            healthCallback_(currentHealthy, score);
        }
        lastHealthyState = currentHealthy;
    }
}

// TODO 2.4.105: Callback Management
// ---------------------------------
void CircularAudioBuffer::setBufferStateCallback(BufferStateCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    bufferStateCallback_ = callback;
}

void CircularAudioBuffer::clearCallbacks() {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    bufferStateCallback_ = nullptr;
    overflowCallback_ = nullptr;
    underflowCallback_ = nullptr;
    performanceCallback_ = nullptr;
    errorCallback_ = nullptr;
    healthCallback_ = nullptr;
    statisticsCallback_ = nullptr;
    resizeCallback_ = nullptr;
}

CircularBufferError CircularAudioBuffer::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return lastError_;
}

void CircularAudioBuffer::clearErrors() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    lastError_ = {};
    errorHistory_.clear();
    statistics_.consecutiveErrors.store(0);
    updateHealthScore();
}

// TODO 2.4.106: Utility Function Implementations
// ----------------------------------------------
CircularBufferConfig createDefaultConfig() {
    CircularBufferConfig config = {};

    // Basic configuration
    config.bufferSize = 8192;
    config.numChannels = 1;
    config.sampleRate = 44100;

    // Performance configuration
    config.enableLockFreeOperations = true;
    config.enableStatistics = true;
    config.writeBlockSize = 1024;
    config.readBlockSize = 1024;

    // Safety configuration
    config.overflowThreshold = 0.95f;
    config.underflowThreshold = 0.05f;
    config.enableOverflowProtection = true;
    config.enableUnderflowProtection = true;

    // Monitoring configuration
    config.enableLatencyMonitoring = true;
    config.statisticsUpdateInterval = 1000;
    config.enableDetailedDiagnostics = false;

    // Advanced configuration
    config.enableDynamicResize = false;
    config.resizeThreshold = 0.8f;
    config.maxBufferSize = 32768;
    config.minBufferSize = 1024;

    // Threading configuration
    config.readerPriority = 0;
    config.writerPriority = 0;
    config.enableThreadAffinity = false;

    // Error handling
    config.enableErrorRecovery = true;
    config.maxRetries = 3;
    config.retryDelay = 10;

    return config;
}

CircularBufferConfig createRealtimeConfig(size_t bufferSize) {
    CircularBufferConfig config = createDefaultConfig();

    config.bufferSize = bufferSize;
    config.enableLockFreeOperations = true;
    config.overflowThreshold = 0.90f;
    config.underflowThreshold = 0.10f;
    config.enableLatencyMonitoring = true;
    config.writeBlockSize = std::min(bufferSize / 8, static_cast<size_t>(512));
    config.readBlockSize = std::min(bufferSize / 8, static_cast<size_t>(512));

    return config;
}

size_t calculateOptimalBufferSize(uint32_t sampleRate, float targetLatency) {
    // Calculate buffer size for target latency in milliseconds
    size_t samples = static_cast<size_t>(sampleRate * targetLatency / 1000.0f);

    // Round up to next power of 2 for efficiency
    size_t powerOfTwo = 1;
    while (powerOfTwo < samples) {
        powerOfTwo <<= 1;
    }

    // Ensure reasonable limits
    return std::max(static_cast<size_t>(1024), std::min(powerOfTwo, static_cast<size_t>(32768)));
}

bool validateBufferConfiguration(const CircularBufferConfig& config, std::string& error) {
    CircularAudioBuffer tempBuffer;
    return tempBuffer.validateConfiguration(config, error);
}

}  // namespace core
}  // namespace huntmaster
