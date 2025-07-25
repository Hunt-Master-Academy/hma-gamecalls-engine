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
#include <malloc.h>  // For _aligned_malloc and _aligned_free
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

CircularAudioBuffer::CircularAudioBuffer()
    : initialized_(false), lastStatsUpdate_(std::chrono::steady_clock::now()),
      lastOperationTime_(std::chrono::high_resolution_clock::now()),
      lastHealthCheck_(std::chrono::steady_clock::now()) {
    config_ = createDefaultConfig();

    statistics_.startTime = std::chrono::steady_clock::now();
    statistics_.lastUpdate = statistics_.startTime;

    lastError_ = {};

    latencyHistory_.reserve(1000);
    throughputHistory_.reserve(1000);
    errorHistory_.reserve(100);

    std::cout << "CircularAudioBuffer constructed with default configuration" << std::endl;
}

CircularAudioBuffer::CircularAudioBuffer(const CircularBufferConfig& config)
    : CircularAudioBuffer() {
    initialize(config);
}

CircularAudioBuffer::~CircularAudioBuffer() {
    clearCallbacks();
    cleanupBuffer();

    std::cout << "CircularAudioBuffer destructed" << std::endl;
}

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
    other.initialized_ = false;
    other.buffer_.reset();
    other.bufferSize_ = 0;
}

CircularAudioBuffer& CircularAudioBuffer::operator=(CircularAudioBuffer&& other) noexcept {
    if (this != &other) {
        cleanupBuffer();

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

        other.initialized_ = false;
        other.buffer_.reset();
        other.bufferSize_ = 0;
    }
    return *this;
}

bool CircularAudioBuffer::initialize(const CircularBufferConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);

    try {
        std::string validationError;
        if (!validateConfiguration(config, validationError)) {
            handleError(-1, "Invalid buffer configuration: " + validationError);
            return false;
        }

        if (initialized_) {
            cleanupBuffer();
        }

        config_ = config;
        bufferSize_ = config.bufferSize;
        numChannels_ = config.numChannels;
        sampleRate_ = config.sampleRate;

        if (!initializeBuffer()) {
            handleError(-2, "Failed to initialize buffer storage");
            return false;
        }

        writePointer_ = 0;
        readPointer_ = 0;
        availableData_ = 0;
        sequenceNumber_ = 0;
        timestamp_ = 0;

        writeInProgress_ = false;
        readInProgress_ = false;

        statistics_ = {};
        statistics_.startTime = std::chrono::steady_clock::now();
        statistics_.lastUpdate = statistics_.startTime;
        statistics_.isHealthy = true;
        statistics_.healthScore = 1.0f;

        lastError_ = {};
        errorHistory_.clear();

        latencyHistory_.clear();
        throughputHistory_.clear();
        currentThroughput_ = 0.0f;
        healthScore_ = 1.0f;
        isHealthy_ = true;

        lastStatsUpdate_ = std::chrono::steady_clock::now();
        lastOperationTime_ = std::chrono::high_resolution_clock::now();
        lastHealthCheck_ = std::chrono::steady_clock::now();

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

    // Validate new configuration
    std::string validationError;
    if (!validateConfiguration(config, validationError)) {
        handleError(-11, "Invalid configuration update: " + validationError);
        return false;
    }

    bool needsReinitialization = false;

    if (config.bufferSize != config_.bufferSize || config.numChannels != config_.numChannels
        || config.sampleRate != config_.sampleRate) {
        needsReinitialization = true;
    }

    if (needsReinitialization) {
        initialized_ = false;
        return initialize(config);
    } else {
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

    size_t available = getAvailableForWrite();
    size_t toWrite = std::min(sampleCount, available);

    if (toWrite > 0) {
        written = writeInternal(data, toWrite, false);
    }

    return written;
}

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

    size_t available = getAvailableForRead();
    size_t toRead = std::min(sampleCount, available);

    if (toRead > 0) {
        read = readInternal(data, toRead, false);
    }

    return read;
}

size_t CircularAudioBuffer::peek(float* data, size_t sampleCount, size_t offset) const {
    if (!initialized_ || !data || sampleCount == 0) {
        return 0;
    }

    try {
        size_t available = getAvailableForRead();
        if (offset >= available) {
            return 0;  // Offset beyond available data
        }

        size_t actualCount = std::min(sampleCount, available - offset);
        if (actualCount == 0) {
            return 0;
        }

        size_t readPos =
            (readPointer_.load(std::memory_order_acquire) + offset) % bufferSize_.load();

        if (readPos + actualCount <= bufferSize_) {
            std::memcpy(data, buffer_.get() + readPos, actualCount * sizeof(float));
        } else {
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
        size_t available = getAvailableForRead();
        size_t toSkip = std::min(sampleCount, available);

        if (toSkip > 0) {
            advanceReadPointer(toSkip);

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
        std::lock_guard<std::mutex> writeLock(writeMutex_);
        std::lock_guard<std::mutex> readLock(readMutex_);

        writePointer_ = 0;
        readPointer_ = 0;
        availableData_ = 0;

        if (buffer_) {
            std::memset(buffer_.get(), 0, bufferSize_ * sizeof(float));
        }

        sequenceNumber_ = 0;

        std::cout << "CircularAudioBuffer cleared" << std::endl;

    } catch (const std::exception& e) {
        handleError(-30, "Buffer clear operation failed", e.what());
    }
}

void CircularAudioBuffer::reset() {
    clear();

    resetStatistics();
    clearErrors();
}

bool CircularAudioBuffer::flush() {
    if (!initialized_) {
        return false;
    }

    try {
        while (writeInProgress_.load(std::memory_order_acquire)
               || readInProgress_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

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

        if (newSize < config_.minBufferSize || newSize > config_.maxBufferSize) {
            handleError(-32, "Invalid buffer size for resize: " + std::to_string(newSize));
            return false;
        }

#ifdef _WIN32
        std::unique_ptr<float, AlignedDeleter> newBuffer(
            static_cast<float*>(_aligned_malloc(newSize * sizeof(float), 32)));
#else
        float* alignedPtr = nullptr;
        posix_memalign(reinterpret_cast<void**>(&alignedPtr), 32, newSize * sizeof(float));
        std::unique_ptr<float, AlignedDeleter> newBuffer(alignedPtr);
#endif
        if (!newBuffer) {
            handleError(-33, "Failed to allocate new buffer for resize");
            return false;
        }

        size_t dataToCopy = std::min(getAvailableForRead(), newSize - 1);
        if (dataToCopy > 0) {
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

        buffer_.reset(newBuffer.release());
        bufferSize_ = newSize;
        config_.bufferSize = newSize;

        readPointer_ = 0;
        writePointer_ = dataToCopy;
        availableData_ = dataToCopy;

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

bool CircularAudioBuffer::initializeBuffer() {
    try {
        size_t totalSize = bufferSize_ * numChannels_;

#ifdef _WIN32
        buffer_.reset(static_cast<float*>(_aligned_malloc(totalSize * sizeof(float), 32)));
#elif defined(__linux__) || defined(__APPLE__)
        float* alignedPtr = nullptr;
        if (posix_memalign(reinterpret_cast<void**>(&alignedPtr), 32, totalSize * sizeof(float))
            == 0) {
            buffer_.reset(alignedPtr);
        }
#else
        // Fallback for other platforms
        buffer_.reset(static_cast<float*>(malloc(totalSize * sizeof(float))));
#endif

        if (!buffer_) {
            return false;
        }

        std::memset(buffer_.get(), 0, totalSize * sizeof(float));

        return true;

    } catch (const std::exception& e) {
        handleError(-50, "Buffer initialization failed", e.what());
        return false;
    }
}

void CircularAudioBuffer::cleanupBuffer() {
    if (buffer_) {
        buffer_.reset();
    }

    bufferSize_ = 0;
    writePointer_ = 0;
    readPointer_ = 0;
    availableData_ = 0;
    sequenceNumber_ = 0;
    timestamp_ = 0;
}

bool CircularAudioBuffer::validateConfiguration(const CircularBufferConfig& config,
                                                std::string& error) const {
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

    if (config.numChannels == 0 || config.numChannels > 32) {
        error = "Invalid number of channels: " + std::to_string(config.numChannels);
        return false;
    }

    if (config.sampleRate < 8000 || config.sampleRate > 192000) {
        error = "Invalid sample rate: " + std::to_string(config.sampleRate);
        return false;
    }

    if (config.overflowThreshold <= 0.0f || config.overflowThreshold > 1.0f) {
        error = "Invalid overflow threshold: " + std::to_string(config.overflowThreshold);
        return false;
    }

    if (config.underflowThreshold < 0.0f || config.underflowThreshold >= config.overflowThreshold) {
        error = "Invalid underflow threshold: " + std::to_string(config.underflowThreshold);
        return false;
    }

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
        writeInProgress_ = true;

        size_t totalWritten = 0;
        size_t remaining = sampleCount;

        while (remaining > 0 && (blocking || getAvailableForWrite() > 0)) {
            size_t available = getAvailableForWrite();
            if (available == 0) {
                if (!blocking) {
                    break;
                }

                if (config_.enableOverflowProtection) {
                    if (overflowCallback_) {
                        overflowCallback_(remaining, available);
                    }
                    statistics_.overflowCount.fetch_add(1, std::memory_order_relaxed);
                }

                std::this_thread::yield();
                continue;
            }

            size_t toWrite = std::min(remaining, available);
            size_t writePos = writePointer_.load(std::memory_order_acquire);
            size_t bufferSize = bufferSize_.load(std::memory_order_acquire);

            if (writePos + toWrite <= bufferSize) {
#ifdef __SSE2__
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
                        buffer_.get()[writePos + i] = data[totalWritten + i];
                    }
                } else
#endif
                {
                    std::memcpy(
                        buffer_.get() + writePos, data + totalWritten, toWrite * sizeof(float));
                }
            } else {
                size_t firstPart = bufferSize - writePos;
                size_t secondPart = toWrite - firstPart;

                std::memcpy(
                    buffer_.get() + writePos, data + totalWritten, firstPart * sizeof(float));
                std::memcpy(
                    buffer_.get(), data + totalWritten + firstPart, secondPart * sizeof(float));
            }

            advanceWritePointer(toWrite);

            totalWritten += toWrite;
            remaining -= toWrite;

            statistics_.totalWrites.fetch_add(1, std::memory_order_relaxed);
            statistics_.totalSamples.fetch_add(toWrite, std::memory_order_relaxed);
        }

        writeInProgress_ = false;

        auto endTime = std::chrono::high_resolution_clock::now();
        float latency =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
            / 1000.0f;
        recordWriteLatency(latency);

        sequenceNumber_.fetch_add(1, std::memory_order_relaxed);

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
        readInProgress_ = true;

        size_t totalRead = 0;
        size_t remaining = sampleCount;

        while (remaining > 0 && (blocking || getAvailableForRead() > 0)) {
            size_t available = getAvailableForRead();
            if (available == 0) {
                if (!blocking) {
                    break;
                }

                if (config_.enableUnderflowProtection) {
                    if (underflowCallback_) {
                        underflowCallback_(remaining, available);
                    }
                    statistics_.underflowCount.fetch_add(1, std::memory_order_relaxed);
                }

                std::this_thread::yield();
                continue;
            }

            size_t toRead = std::min(remaining, available);
            size_t readPos = readPointer_.load(std::memory_order_acquire);
            size_t bufferSize = bufferSize_.load(std::memory_order_acquire);

            if (readPos + toRead <= bufferSize) {
#ifdef __SSE2__
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
                        data[totalRead + i] = buffer_.get()[readPos + i];
                    }
                } else
#endif
                {
                    std::memcpy(data + totalRead, buffer_.get() + readPos, toRead * sizeof(float));
                }
            } else {
                size_t firstPart = bufferSize - readPos;
                size_t secondPart = toRead - firstPart;

                std::memcpy(data + totalRead, buffer_.get() + readPos, firstPart * sizeof(float));
                std::memcpy(
                    data + totalRead + firstPart, buffer_.get(), secondPart * sizeof(float));
            }

            advanceReadPointer(toRead);

            totalRead += toRead;
            remaining -= toRead;

            statistics_.totalReads.fetch_add(1, std::memory_order_relaxed);
        }

        readInProgress_ = false;

        auto endTime = std::chrono::high_resolution_clock::now();
        float latency =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
            / 1000.0f;
        recordReadLatency(latency);

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

void CircularAudioBuffer::recordWriteLatency(float latency) {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

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

    CircularBufferStatistics stats;
    stats.totalWrites = statistics_.totalWrites.load();
    stats.totalReads = statistics_.totalReads.load();
    stats.totalSamples = statistics_.totalSamples.load();
    stats.overflowCount = statistics_.overflowCount.load();
    stats.underflowCount = statistics_.underflowCount.load();
    stats.retryCount = statistics_.retryCount.load();
    stats.errorCount = statistics_.errorCount.load();
    stats.consecutiveErrors = statistics_.consecutiveErrors.load();
    stats.averageWriteLatency = statistics_.averageWriteLatency.load();
    stats.averageReadLatency = statistics_.averageReadLatency.load();
    stats.maxWriteLatency = statistics_.maxWriteLatency.load();
    stats.maxReadLatency = statistics_.maxReadLatency.load();
    stats.throughput = statistics_.throughput.load();
    stats.healthScore = statistics_.healthScore.load();
    stats.isHealthy = statistics_.isHealthy.load();
    stats.startTime = statistics_.startTime;
    stats.lastUpdate = statistics_.lastUpdate;

    auto currentTime = std::chrono::steady_clock::now();
    auto timeDiff =
        std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - stats.startTime);

    // Calculate current buffer state
    stats.currentLevel = getCurrentLevel();
    stats.fillRatio = getFillRatio();

    // Calculate throughput
    if (timeDiff.count() > 0) {
        float seconds = timeDiff.count() / 1000.0f;
        stats.throughput = stats.totalSamples.load() / seconds;
    }

    // Calculate error rate
    size_t totalOps = stats.totalWrites.load() + stats.totalReads.load();
    if (totalOps > 0) {
        stats.errorRate = static_cast<float>(stats.errorCount.load()) / totalOps;
    }

    return stats;
}

void CircularAudioBuffer::resetStatistics() {
    std::lock_guard<std::mutex> lock(statisticsMutex_);

    statistics_ = CircularBufferStatistics();
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

void CircularAudioBuffer::handleError(int code,
                                      const std::string& message,
                                      const std::string& details) const {
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

void CircularAudioBuffer::updateHealthScore() const {
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

}  // namespace core
}  // namespace huntmaster
