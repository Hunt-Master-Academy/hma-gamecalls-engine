/**
 * @file OptimizedAudioIO.cpp
 * @brief Implementation of high-performance audio I/O optimizations
 */

#include "huntmaster/core/OptimizedAudioIO.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>

#include "dr_wav.h"

#ifdef _WIN32
#include <Windows.h>
#include <memoryapi.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace huntmaster {

// ============================================================================
// MemoryMappedAudioFile Implementation
// ============================================================================

class MemoryMappedAudioFile::Impl {
  public:
    Config config_;
    std::string filename_;
    void* mappedData_ = nullptr;
    size_t fileSize_ = 0;
    size_t sampleCount_ = 0;
    AudioFormat format_ = {};
    IOPerformanceMetrics metrics_ = {};
    bool isOpen_ = false;

    // Platform-specific handles
#ifdef _WIN32
    HANDLE fileHandle_ = INVALID_HANDLE_VALUE;
    HANDLE mappingHandle_ = INVALID_HANDLE_VALUE;
#else
    int fileDescriptor_ = -1;
#endif

    // Cache for frequently accessed data
    mutable std::unordered_map<size_t, std::vector<float>> cache_;
    mutable std::mutex cacheMutex_;
    mutable size_t cacheSize_ = 0;

    ~Impl() {
        close();
    }

    bool open(const std::string& filename) {
        close();  // Close any existing file

        filename_ = filename;

        // First, analyze the file with dr_wav to get format info
        if (!analyzeAudioFormat()) {
            return false;
        }

        // Open file for memory mapping
        if (!openForMapping()) {
            return false;
        }

        isOpen_ = true;
        return true;
    }

    void close() {
        if (!isOpen_)
            return;

#ifdef _WIN32
        if (mappedData_) {
            UnmapViewOfFile(mappedData_);
            mappedData_ = nullptr;
        }
        if (mappingHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(mappingHandle_);
            mappingHandle_ = INVALID_HANDLE_VALUE;
        }
        if (fileHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle_);
            fileHandle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (mappedData_ && mappedData_ != MAP_FAILED) {
            munmap(mappedData_, fileSize_);
            mappedData_ = nullptr;
        }
        if (fileDescriptor_ >= 0) {
            ::close(fileDescriptor_);
            fileDescriptor_ = -1;
        }
#endif

        // Clear cache
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_.clear();
        cacheSize_ = 0;

        isOpen_ = false;
    }

    const float* readSamples(size_t offset, size_t count) {
        if (!isOpen_ || offset + count > sampleCount_) {
            return nullptr;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Check cache first
        if (config_.enableCaching) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            auto it = cache_.find(offset);
            if (it != cache_.end() && it->second.size() >= count) {
                metrics_.cacheHits++;
                return it->second.data();
            }
            metrics_.cacheMisses++;
        }

        // Read from memory-mapped data
        const float* data = readFromMappedData(offset, count);

        // Cache the data if enabled
        if (config_.enableCaching && data && cacheSize_ < config_.maxCacheSize) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            if (cache_.find(offset) == cache_.end()) {
                std::vector<float> cached(data, data + count);
                cache_[offset] = std::move(cached);
                cacheSize_ += count * sizeof(float);

                // Evict oldest entries if cache is too large
                while (cacheSize_ > config_.maxCacheSize && !cache_.empty()) {
                    auto oldest = cache_.begin();
                    cacheSize_ -= oldest->second.size() * sizeof(float);
                    cache_.erase(oldest);
                }
            }
        }

        // Update metrics
        auto endTime = std::chrono::high_resolution_clock::now();
        metrics_.totalReadTime +=
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        metrics_.bytesRead += count * sizeof(float);
        metrics_.readOperations++;

        return data;
    }

  private:
    bool analyzeAudioFormat() {
        drwav wav;
        if (!drwav_init_file(&wav, filename_.c_str(), nullptr)) {
            std::cerr << "Failed to analyze audio format: " << filename_ << std::endl;
            return false;
        }

        format_.sampleRate = wav.sampleRate;
        format_.channels = wav.channels;
        format_.bitsPerSample = wav.bitsPerSample;
        format_.formatName = "WAV";

        sampleCount_ = wav.totalPCMFrameCount * wav.channels;

        drwav_uninit(&wav);
        return true;
    }

    bool openForMapping() {
#ifdef _WIN32
        fileHandle_ = CreateFileA(filename_.c_str(),
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  nullptr,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);

        if (fileHandle_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open file for mapping: " << filename_ << std::endl;
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(fileHandle_, &fileSize)) {
            std::cerr << "Failed to get file size: " << filename_ << std::endl;
            return false;
        }
        fileSize_ = static_cast<size_t>(fileSize.QuadPart);

        mappingHandle_ = CreateFileMappingA(fileHandle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mappingHandle_) {
            std::cerr << "Failed to create file mapping: " << filename_ << std::endl;
            return false;
        }

        mappedData_ = MapViewOfFile(mappingHandle_, FILE_MAP_READ, 0, 0, 0);
        if (!mappedData_) {
            std::cerr << "Failed to map view of file: " << filename_ << std::endl;
            return false;
        }
#else
        fileDescriptor_ = ::open(filename_.c_str(), O_RDONLY);
        if (fileDescriptor_ < 0) {
            std::cerr << "Failed to open file for mapping: " << filename_ << std::endl;
            return false;
        }

        struct stat fileStat;
        if (fstat(fileDescriptor_, &fileStat) < 0) {
            std::cerr << "Failed to get file size: " << filename_ << std::endl;
            return false;
        }
        fileSize_ = static_cast<size_t>(fileStat.st_size);

        mappedData_ = mmap(nullptr, fileSize_, PROT_READ, MAP_PRIVATE, fileDescriptor_, 0);
        if (mappedData_ == MAP_FAILED) {
            std::cerr << "Failed to memory map file: " << filename_ << std::endl;
            return false;
        }

        // Provide hints to the kernel about access pattern
        int advice = MADV_SEQUENTIAL;
        switch (config_.accessPattern) {
            case AccessPattern::RANDOM:
                advice = MADV_RANDOM;
                break;
            case AccessPattern::STREAMING:
                advice = MADV_WILLNEED;
                break;
            default:
                advice = MADV_SEQUENTIAL;
                break;
        }
        madvise(mappedData_, fileSize_, advice);
#endif

        return true;
    }

    const float* readFromMappedData(size_t offset, size_t count) {
        // This is a simplified version - in practice, you'd need to handle
        // the WAV header format and convert to float samples

        // For now, assume the mapped data contains raw float samples
        // after the WAV header (typically 44 bytes)
        size_t headerSize = 44;  // Simplified WAV header size
        size_t byteOffset = headerSize + (offset * sizeof(float));

        if (byteOffset + (count * sizeof(float)) > fileSize_) {
            return nullptr;
        }

        return reinterpret_cast<const float*>(static_cast<char*>(mappedData_) + byteOffset);
    }
};

MemoryMappedAudioFile::MemoryMappedAudioFile(const Config& config)
    : pImpl(std::make_unique<Impl>()) {
    pImpl->config_ = config;
}

MemoryMappedAudioFile::~MemoryMappedAudioFile() = default;

bool MemoryMappedAudioFile::open(const std::string& filename) {
    return pImpl->open(filename);
}

void MemoryMappedAudioFile::close() {
    pImpl->close();
}

const float* MemoryMappedAudioFile::readSamples(size_t offset, size_t count) {
    return pImpl->readSamples(offset, count);
}

size_t MemoryMappedAudioFile::getSampleCount() const {
    return pImpl->sampleCount_;
}

MemoryMappedAudioFile::AudioFormat MemoryMappedAudioFile::getFormat() const {
    return pImpl->format_;
}

IOPerformanceMetrics MemoryMappedAudioFile::getMetrics() const {
    return pImpl->metrics_;
}

void MemoryMappedAudioFile::prefetch(size_t offset, size_t count) {
    if (!pImpl->isOpen_)
        return;

    // Trigger read to cache the data
    readSamples(offset, count);
}

bool MemoryMappedAudioFile::isOpen() const {
    return pImpl->isOpen_;
}

// ============================================================================
// AsyncAudioWriter Implementation
// ============================================================================

class AsyncAudioWriter::Impl {
  public:
    Config config_;
    std::string filename_;
    drwav wav_;
    bool wavInitialized_ = false;

    // Async processing
    std::thread writerThread_;
    std::queue<std::pair<std::vector<float>, WriteCallback>> writeQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shouldStop_{false};
    std::atomic<bool> isActive_{false};

    IOPerformanceMetrics metrics_ = {};

    ~Impl() {
        stop(std::chrono::milliseconds(1000));
    }

    bool start(const std::string& filename,
               uint32_t sampleRate,
               uint16_t channels,
               uint16_t bitsPerSample) {
        if (isActive_) {
            return false;
        }

        filename_ = filename;

        // Initialize WAV file
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = bitsPerSample;

        if (!drwav_init_file_write(&wav_, filename_.c_str(), &format, nullptr)) {
            std::cerr << "Failed to initialize WAV file for writing: " << filename_ << std::endl;
            return false;
        }
        wavInitialized_ = true;

        // Start writer thread
        shouldStop_ = false;
        isActive_ = true;
        writerThread_ = std::thread(&Impl::writerThreadFunc, this);

        return true;
    }

    bool writeAsync(const float* data, size_t sampleCount, WriteCallback callback) {
        if (!isActive_) {
            return false;
        }

        // Check queue depth
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (writeQueue_.size() >= config_.maxQueuedWrites) {
                if (callback) {
                    callback(false, "Write queue full");
                }
                return false;
            }
        }

        // Copy data and queue for writing
        std::vector<float> dataCopy(data, data + sampleCount);

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            writeQueue_.emplace(std::move(dataCopy), callback);
        }

        queueCondition_.notify_one();
        return true;
    }

    bool stop(std::chrono::milliseconds timeout) {
        if (!isActive_) {
            return true;
        }

        shouldStop_ = true;
        queueCondition_.notify_all();

        if (writerThread_.joinable()) {
            if (timeout.count() > 0) {
                // Try to join with timeout (simplified - in practice you'd use platform-specific
                // APIs)
                writerThread_.join();
            } else {
                writerThread_.join();
            }
        }

        if (wavInitialized_) {
            drwav_uninit(&wav_);
            wavInitialized_ = false;
        }

        isActive_ = false;
        return true;
    }

    size_t getQueueDepth() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return writeQueue_.size();
    }

  private:
    void writerThreadFunc() {
        while (!shouldStop_ || !writeQueue_.empty()) {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Wait for data or stop signal
            queueCondition_.wait(lock, [this] { return !writeQueue_.empty() || shouldStop_; });

            if (writeQueue_.empty()) {
                continue;
            }

            // Get next write operation
            auto [data, callback] = std::move(writeQueue_.front());
            writeQueue_.pop();
            lock.unlock();

            // Perform write operation
            auto startTime = std::chrono::high_resolution_clock::now();

            bool success = false;
            std::string errorMessage;

            if (wavInitialized_) {
                drwav_uint64 samplesWritten =
                    drwav_write_pcm_frames(&wav_, data.size(), data.data());
                success = (samplesWritten == data.size());
                if (!success) {
                    errorMessage = "Failed to write audio samples";
                }
            } else {
                errorMessage = "WAV file not initialized";
            }

            // Update metrics
            auto endTime = std::chrono::high_resolution_clock::now();
            metrics_.totalWriteTime +=
                std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            metrics_.bytesWritten += data.size() * sizeof(float);
            metrics_.writeOperations++;

            // Call completion callback
            if (callback) {
                callback(success, errorMessage);
            }
        }
    }
};

AsyncAudioWriter::AsyncAudioWriter(const Config& config) : pImpl(std::make_unique<Impl>()) {
    pImpl->config_ = config;
}

AsyncAudioWriter::~AsyncAudioWriter() = default;

bool AsyncAudioWriter::start(const std::string& filename,
                             uint32_t sampleRate,
                             uint16_t channels,
                             uint16_t bitsPerSample) {
    return pImpl->start(filename, sampleRate, channels, bitsPerSample);
}

bool AsyncAudioWriter::writeAsync(const float* data, size_t sampleCount, WriteCallback callback) {
    return pImpl->writeAsync(data, sampleCount, callback);
}

bool AsyncAudioWriter::stop(std::chrono::milliseconds timeout) {
    return pImpl->stop(timeout);
}

size_t AsyncAudioWriter::getQueueDepth() const {
    return pImpl->getQueueDepth();
}

bool AsyncAudioWriter::isActive() const {
    return pImpl->isActive_;
}

IOPerformanceMetrics AsyncAudioWriter::getMetrics() const {
    return pImpl->metrics_;
}

// ============================================================================
// StreamingAudioBuffer Implementation
// ============================================================================

class StreamingAudioBuffer::Impl {
  public:
    Config config_;
    std::vector<float> buffer_;
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> readPos_{0};
    std::atomic<size_t> availableFrames_{0};
    uint16_t channels_ = 0;

    BufferCallback callback_;
    std::mutex callbackMutex_;

    std::atomic<size_t> overflowCount_{0};
    std::atomic<size_t> underflowCount_{0};

    bool initialize(uint16_t channels) {
        channels_ = channels;
        buffer_.resize(config_.bufferSizeFrames * channels);
        writePos_ = 0;
        readPos_ = 0;
        availableFrames_ = 0;
        return true;
    }

    size_t write(const float* data, size_t frameCount) {
        size_t framesWritten = 0;
        size_t currentWritePos = writePos_.load();

        for (size_t i = 0; i < frameCount; ++i) {
            size_t currentAvailable = availableFrames_.load();

            // Check for buffer overflow
            if (currentAvailable >= config_.bufferSizeFrames) {
                if (config_.enableOverflowProtection) {
                    overflowCount_++;
                    break;
                } else {
                    // Overwrite oldest data
                    readPos_ = (readPos_.load() + 1) % config_.bufferSizeFrames;
                }
            }

            // Write frame
            for (uint16_t ch = 0; ch < channels_; ++ch) {
                buffer_[currentWritePos * channels_ + ch] = data[i * channels_ + ch];
            }

            currentWritePos = (currentWritePos + 1) % config_.bufferSizeFrames;
            framesWritten++;
            availableFrames_++;
        }

        writePos_ = currentWritePos;

        // Check buffer levels and trigger callbacks
        checkBufferLevels();

        return framesWritten;
    }

    size_t read(float* data, size_t frameCount) {
        size_t framesRead = 0;
        size_t currentReadPos = readPos_.load();

        for (size_t i = 0; i < frameCount; ++i) {
            size_t currentAvailable = availableFrames_.load();

            // Check for buffer underflow
            if (currentAvailable == 0) {
                if (config_.enableUnderflowProtection) {
                    underflowCount_++;
                    // Fill remaining with silence
                    for (size_t j = i; j < frameCount; ++j) {
                        for (uint16_t ch = 0; ch < channels_; ++ch) {
                            data[j * channels_ + ch] = 0.0f;
                        }
                    }
                    break;
                }
            }

            // Read frame
            for (uint16_t ch = 0; ch < channels_; ++ch) {
                data[i * channels_ + ch] = buffer_[currentReadPos * channels_ + ch];
            }

            currentReadPos = (currentReadPos + 1) % config_.bufferSizeFrames;
            framesRead++;
            availableFrames_--;
        }

        readPos_ = currentReadPos;
        return framesRead;
    }

    size_t getAvailableFrames() const {
        return availableFrames_.load();
    }

    size_t getFreeSpace() const {
        return config_.bufferSizeFrames - availableFrames_.load();
    }

    void clear() {
        writePos_ = 0;
        readPos_ = 0;
        availableFrames_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

  private:
    void checkBufferLevels() {
        size_t available = availableFrames_.load();

        // Check for overflow condition
        if (available >= config_.highWatermarkFrames) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (callback_) {
                callback_(buffer_.data() + (readPos_.load() * channels_),
                          available,
                          true);  // isOverflow = true
            }
        }
        // Check for underflow condition
        else if (available <= config_.lowWatermarkFrames) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (callback_) {
                callback_(buffer_.data() + (readPos_.load() * channels_),
                          available,
                          false);  // isOverflow = false
            }
        }
    }
};

StreamingAudioBuffer::StreamingAudioBuffer(const Config& config) : pImpl(std::make_unique<Impl>()) {
    pImpl->config_ = config;
}

StreamingAudioBuffer::~StreamingAudioBuffer() = default;

bool StreamingAudioBuffer::initialize(uint16_t channels) {
    return pImpl->initialize(channels);
}

size_t StreamingAudioBuffer::write(const float* data, size_t frameCount) {
    return pImpl->write(data, frameCount);
}

size_t StreamingAudioBuffer::read(float* data, size_t frameCount) {
    return pImpl->read(data, frameCount);
}

size_t StreamingAudioBuffer::getAvailableFrames() const {
    return pImpl->getAvailableFrames();
}

size_t StreamingAudioBuffer::getFreeSpace() const {
    return pImpl->getFreeSpace();
}

void StreamingAudioBuffer::setBufferCallback(BufferCallback callback) {
    std::lock_guard<std::mutex> lock(pImpl->callbackMutex_);
    pImpl->callback_ = callback;
}

void StreamingAudioBuffer::clear() {
    pImpl->clear();
}

StreamingAudioBuffer::BufferHealth StreamingAudioBuffer::getHealth() const {
    BufferHealth health;
    health.fillRatio =
        static_cast<double>(pImpl->getAvailableFrames()) / pImpl->config_.bufferSizeFrames;
    health.overflowCount = pImpl->overflowCount_.load();
    health.underflowCount = pImpl->underflowCount_.load();
    health.isHealthy = (health.fillRatio > 0.2 && health.fillRatio < 0.8);
    return health;
}

}  // namespace huntmaster
