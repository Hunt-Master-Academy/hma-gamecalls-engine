#include "huntmaster/core/OptimizedAudioRecorder.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "dr_wav.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/OptimizedAudioIO.h"

namespace huntmaster {

// OptimizedAudioRecorder Implementation
class OptimizedAudioRecorder::Impl {
  public:
    Config config;
    std::unique_ptr<StreamingAudioBuffer> streamingBuffer;
    std::unique_ptr<AsyncAudioWriter> asyncWriter;

    ma_device device;
    ma_device_config deviceConfig;
    std::atomic<bool> recording{false};
    std::atomic<float> currentLevel{0.0f};

    // Memory-based recording
    std::vector<float> memoryBuffer;
    std::atomic<size_t> memoryBufferUsed{0};
    mutable std::mutex memoryMutex;
    std::atomic<bool> memoryOverflow{false};

    // File-based recording
    std::vector<float> recordedData;
    mutable std::mutex dataMutex;

    Impl() {
        deviceConfig = ma_device_config_init(ma_device_type_capture);
    }

    ~Impl() {
        if (recording) {
            recording = false;
            ma_device_uninit(&device);
        }
    }

    static void
    dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        if (!impl || !impl->recording)
            return;

        const float* input = static_cast<const float*>(pInput);
        const size_t sampleCount = frameCount * impl->config.channels;

        // Calculate level
        float maxLevel = 0.0f;
        for (ma_uint32 i = 0; i < sampleCount; ++i) {
            float absValue = std::abs(input[i]);
            if (absValue > maxLevel)
                maxLevel = absValue;
        }
        impl->currentLevel = maxLevel;

        // Handle different recording modes
        switch (impl->config.recordingMode) {
            case RecordingMode::MEMORY_BASED:
                impl->handleMemoryRecording(input, sampleCount);
                break;

            case RecordingMode::FILE_BASED:
                impl->handleFileRecording(input, frameCount);
                break;

            case RecordingMode::HYBRID:
                impl->handleMemoryRecording(input, sampleCount);
                impl->handleFileRecording(input, frameCount);
                break;
        }
    }

    void handleMemoryRecording(const float* input, size_t sampleCount) {
        size_t currentUsed = memoryBufferUsed.load();

        // Check if we have space
        if (currentUsed + sampleCount > memoryBuffer.size()) {
            if (config.enableMemoryGrowth) {
                // Grow buffer if needed
                std::lock_guard<std::mutex> lock(memoryMutex);
                size_t newSize = memoryBuffer.size() + config.memoryGrowthIncrement;
                try {
                    memoryBuffer.resize(newSize);
                } catch (const std::bad_alloc&) {
                    memoryOverflow = true;
                    return;
                }
            } else {
                memoryOverflow = true;
                return;
            }
        }

        // Standard memory copy
        std::lock_guard<std::mutex> lock(memoryMutex);
        std::memcpy(&memoryBuffer[currentUsed], input, sampleCount * sizeof(float));
        memoryBufferUsed.store(currentUsed + sampleCount);
    }

    void handleFileRecording(const float* input, ma_uint32 frameCount) {
        // Process with streaming buffer for real-time performance
        if (streamingBuffer) {
            (void)streamingBuffer->write(input, frameCount);

            // Process chunks when buffer reaches optimal size
            if (streamingBuffer->getAvailableFrames() >= config.chunkSize) {
                std::vector<float> chunkData(config.chunkSize * config.channels);
                size_t framesRead = streamingBuffer->read(chunkData.data(), config.chunkSize);

                if (framesRead > 0) {
                    // Store chunk
                    std::lock_guard<std::mutex> lock(dataMutex);
                    recordedData.insert(recordedData.end(),
                                        chunkData.begin(),
                                        chunkData.begin() + framesRead * config.channels);

                    // Use advanced async I/O if available
                    if (asyncWriter && asyncWriter->isActive()) {
                        // Queue for async write; ignore callback in engine path
                        (void)asyncWriter->writeAsync(chunkData.data(),
                                                      framesRead * config.channels);
                    }
                }
            }
        }
    }
};

OptimizedAudioRecorder::OptimizedAudioRecorder(const Config& config)
    : pImpl(std::make_unique<Impl>()) {
    pImpl->config = config;

    // Initialize memory buffer if memory-based recording is enabled
    if (config.recordingMode == RecordingMode::MEMORY_BASED
        || config.recordingMode == RecordingMode::HYBRID) {
        pImpl->memoryBuffer.reserve(config.memoryBufferMaxFrames);
        pImpl->memoryBuffer.resize(config.memoryBufferMaxFrames);
        pImpl->memoryBufferUsed = 0;
        pImpl->memoryOverflow = false;
    }
}

OptimizedAudioRecorder::~OptimizedAudioRecorder() = default;

// Initialize device-agnostic resources in constructor; device is started in start().

bool OptimizedAudioRecorder::start() {
    if (pImpl->recording) {
        return false;
    }

    // Configure miniaudio device from current config
    pImpl->deviceConfig.capture.format = ma_format_f32;
    pImpl->deviceConfig.capture.channels = pImpl->config.channels;
    pImpl->deviceConfig.sampleRate = pImpl->config.sampleRate;
    pImpl->deviceConfig.dataCallback = Impl::dataCallback;
    pImpl->deviceConfig.pUserData = pImpl.get();

    // Initialize streaming buffer with optimal size for file-based recording
    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        size_t bufferFrames = pImpl->config.sampleRate * pImpl->config.bufferDurationMs / 1000;
        StreamingAudioBuffer::Config sbCfg{};
        sbCfg.bufferSizeFrames = bufferFrames;
        pImpl->streamingBuffer = std::make_unique<StreamingAudioBuffer>(sbCfg);
        pImpl->streamingBuffer->initialize(pImpl->config.channels);

        // Initialize async writer if output file is specified
        if (!pImpl->config.outputFile.empty()) {
            AsyncAudioWriter::Config wcfg{};
            pImpl->asyncWriter = std::make_unique<AsyncAudioWriter>(wcfg);
            pImpl->asyncWriter->start(
                pImpl->config.outputFile, pImpl->config.sampleRate, pImpl->config.channels, 32);
        }

        // No additional processing pipeline at this stage
    }

    // Initialize device
    ma_result result = ma_device_init(nullptr, &pImpl->deviceConfig, &pImpl->device);
    if (result != MA_SUCCESS) {
        return false;
    }

    // Clear buffers based on recording mode
    if (pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        pImpl->memoryBufferUsed = 0;
        pImpl->memoryOverflow = false;
    }

    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        pImpl->recordedData.clear();
    }

    // Start device
    result = ma_device_start(&pImpl->device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&pImpl->device);
        return false;
    }

    pImpl->recording = true;

    const char* modeStr = "";
    switch (pImpl->config.recordingMode) {
        case RecordingMode::MEMORY_BASED:
            modeStr = "memory-based";
            break;
        case RecordingMode::FILE_BASED:
            modeStr = "file-based";
            break;
        case RecordingMode::HYBRID:
            modeStr = "hybrid";
            break;
    }

    std::cout << "Optimized recording started (" << modeStr << ") at " << pImpl->config.sampleRate
              << " Hz" << std::endl;
    return true;
}

void OptimizedAudioRecorder::stop() {
    if (!pImpl->recording)
        return;

    pImpl->recording = false;

    // Process any remaining data in streaming buffer for file-based recording
    if ((pImpl->config.recordingMode == RecordingMode::FILE_BASED
         || pImpl->config.recordingMode == RecordingMode::HYBRID)
        && pImpl->streamingBuffer && pImpl->streamingBuffer->getAvailableFrames() > 0) {
        std::vector<float> remainingData(pImpl->streamingBuffer->getAvailableFrames()
                                         * pImpl->config.channels);
        size_t framesRead = pImpl->streamingBuffer->read(
            remainingData.data(), pImpl->streamingBuffer->getAvailableFrames());

        if (framesRead > 0) {
            std::lock_guard<std::mutex> lock(pImpl->dataMutex);
            pImpl->recordedData.insert(pImpl->recordedData.end(),
                                       remainingData.begin(),
                                       remainingData.begin() + framesRead * pImpl->config.channels);
        }
    }

    // Finalize async writer
    if (pImpl->asyncWriter && pImpl->asyncWriter->isActive()) {
        pImpl->asyncWriter->stop();
    }

    ma_device_uninit(&pImpl->device);

    size_t totalSamples = 0;
    if (pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        totalSamples = pImpl->memoryBufferUsed.load();
    } else {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        totalSamples = pImpl->recordedData.size();
    }

    std::cout << "Optimized recording stopped. Data size: " << totalSamples << " samples"
              << std::endl;
}

bool OptimizedAudioRecorder::isRecording() const {
    return pImpl->recording;
}

void OptimizedAudioRecorder::setOutputFile(const std::string& filename) {
    pImpl->config.outputFile = filename;
}

void OptimizedAudioRecorder::setRecordingMode(RecordingMode mode) {
    if (!pImpl->recording) {
        pImpl->config.recordingMode = mode;
    }
}

OptimizedAudioRecorder::RecordingMode OptimizedAudioRecorder::getRecordingMode() const {
    return pImpl->config.recordingMode;
}

std::vector<float> OptimizedAudioRecorder::getRecordedData() const {
    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED) {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        return pImpl->recordedData;
    } else {
        std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
        size_t used = pImpl->memoryBufferUsed.load();
        return std::vector<float>(pImpl->memoryBuffer.begin(), pImpl->memoryBuffer.begin() + used);
    }
}

const float* OptimizedAudioRecorder::getRecordedDataPtr(size_t& size) const {
    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED) {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        size = pImpl->recordedData.size();
        return pImpl->recordedData.data();
    } else {
        std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
        size = pImpl->memoryBufferUsed.load();
        return pImpl->memoryBuffer.data();
    }
}

size_t OptimizedAudioRecorder::copyRecordedData(float* buffer, size_t maxSamples) const {
    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED) {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        size_t toCopy = std::min(maxSamples, pImpl->recordedData.size());
        std::memcpy(buffer, pImpl->recordedData.data(), toCopy * sizeof(float));
        return toCopy;
    } else {
        std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
        size_t used = pImpl->memoryBufferUsed.load();
        size_t toCopy = std::min(maxSamples, used);
        std::memcpy(buffer, pImpl->memoryBuffer.data(), toCopy * sizeof(float));
        return toCopy;
    }
}

bool OptimizedAudioRecorder::saveMemoryToFile(const std::string& filename,
                                              const std::string& format) const {
    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED) {
        return false;  // No memory data to save
    }

    std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
    size_t used = pImpl->memoryBufferUsed.load();

    if (used == 0) {
        return false;
    }

    // Compression pipeline not integrated in this build

    // Save as WAV file using dr_wav
    drwav wav;
    drwav_data_format wav_format;
    wav_format.container = drwav_container_riff;
    wav_format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    wav_format.channels = pImpl->config.channels;
    wav_format.sampleRate = pImpl->config.sampleRate;
    wav_format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, filename.c_str(), &wav_format, nullptr)) {
        return false;
    }

    drwav_uint64 framesWritten =
        drwav_write_pcm_frames(&wav, used / pImpl->config.channels, pImpl->memoryBuffer.data());
    drwav_uninit(&wav);

    return framesWritten == (used / pImpl->config.channels);
}

void OptimizedAudioRecorder::clearMemoryBuffer() {
    if (pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
        pImpl->memoryBufferUsed = 0;
        pImpl->memoryOverflow = false;
    }
}

OptimizedAudioRecorder::MemoryBufferInfo OptimizedAudioRecorder::getMemoryBufferInfo() const {
    MemoryBufferInfo info;

    if (pImpl->config.recordingMode == RecordingMode::FILE_BASED) {
        // No memory buffer
        info.totalCapacityFrames = 0;
        info.usedFrames = 0;
        info.freeFrames = 0;
        info.usagePercentage = 0.0;
        info.memorySizeBytes = 0;
        info.isGrowthEnabled = false;
        info.hasOverflowed = false;
        return info;
    }

    std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
    size_t used = pImpl->memoryBufferUsed.load();
    size_t capacity = pImpl->memoryBuffer.size();

    info.totalCapacityFrames = capacity / pImpl->config.channels;
    info.usedFrames = used / pImpl->config.channels;
    info.freeFrames = (capacity - used) / pImpl->config.channels;
    info.usagePercentage = capacity > 0 ? (double(used) / capacity) * 100.0 : 0.0;
    info.memorySizeBytes = capacity * sizeof(float);
    info.isGrowthEnabled = pImpl->config.enableMemoryGrowth;
    info.hasOverflowed = pImpl->memoryOverflow.load();

    return info;
}

float OptimizedAudioRecorder::getCurrentLevel() const {
    return pImpl->currentLevel;
}

double OptimizedAudioRecorder::getDuration() const {
    size_t totalSamples = 0;

    if (pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        totalSamples = pImpl->memoryBufferUsed.load();
    } else {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        totalSamples = pImpl->recordedData.size();
    }

    return static_cast<double>(totalSamples) / (pImpl->config.sampleRate * pImpl->config.channels);
}

size_t OptimizedAudioRecorder::getSampleCount() const {
    if (pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
        || pImpl->config.recordingMode == RecordingMode::HYBRID) {
        return pImpl->memoryBufferUsed.load();
    } else {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        return pImpl->recordedData.size();
    }
}

// saveToFile functionality is not part of the public header; memory save is provided via
// saveMemoryToFile.

// Player/Batch components intentionally omitted in this build

#ifdef HUNTMASTER_TEST_HOOKS
void OptimizedAudioRecorder::testFeedMemorySamples(const float* samples, size_t sampleCount) {
    if (!(pImpl->config.recordingMode == RecordingMode::MEMORY_BASED
          || pImpl->config.recordingMode == RecordingMode::HYBRID)) {
        return;
    }
    std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
    size_t used = pImpl->memoryBufferUsed.load();
    if (used + sampleCount > pImpl->memoryBuffer.size()) {
        if (pImpl->config.enableMemoryGrowth) {
            size_t growBy = std::max(sampleCount, pImpl->config.memoryGrowthIncrement);
            pImpl->memoryBuffer.resize(pImpl->memoryBuffer.size() + growBy);
        } else {
            pImpl->memoryOverflow = true;
            sampleCount = std::min(sampleCount, pImpl->memoryBuffer.size() - used);
        }
    }
    std::memcpy(pImpl->memoryBuffer.data() + used, samples, sampleCount * sizeof(float));
    pImpl->memoryBufferUsed.store(used + sampleCount);
}

void OptimizedAudioRecorder::testFeedFileSamples(const float* interleaved, size_t frames) {
    if (!(pImpl->config.recordingMode == RecordingMode::FILE_BASED
          || pImpl->config.recordingMode == RecordingMode::HYBRID)) {
        return;
    }
    // Lazy init of streamingBuffer to make test independent from start()
    if (!pImpl->streamingBuffer) {
        size_t bufferFrames =
            std::max<size_t>(pImpl->config.chunkSize,
                             pImpl->config.sampleRate * pImpl->config.bufferDurationMs / 1000);
        StreamingAudioBuffer::Config sbCfg{};
        sbCfg.bufferSizeFrames = bufferFrames;
        pImpl->streamingBuffer = std::make_unique<StreamingAudioBuffer>(sbCfg);
        pImpl->streamingBuffer->initialize(pImpl->config.channels);
    }

    (void)pImpl->streamingBuffer->write(interleaved, frames);

    // If enough frames are available, mimic the normal chunking path
    while (pImpl->streamingBuffer->getAvailableFrames() >= pImpl->config.chunkSize) {
        std::vector<float> chunk(pImpl->config.chunkSize * pImpl->config.channels);
        size_t readFrames = pImpl->streamingBuffer->read(chunk.data(), pImpl->config.chunkSize);
        if (readFrames == 0)
            break;
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        pImpl->recordedData.insert(pImpl->recordedData.end(),
                                   chunk.begin(),
                                   chunk.begin() + readFrames * pImpl->config.channels);
    }
}

void OptimizedAudioRecorder::testForceFlushFileBuffer() {
    if (!(pImpl->config.recordingMode == RecordingMode::FILE_BASED
          || pImpl->config.recordingMode == RecordingMode::HYBRID)) {
        return;
    }
    if (!pImpl->streamingBuffer)
        return;
    size_t avail = pImpl->streamingBuffer->getAvailableFrames();
    if (avail == 0)
        return;
    std::vector<float> remaining(avail * pImpl->config.channels);
    size_t readFrames = pImpl->streamingBuffer->read(remaining.data(), avail);
    if (readFrames > 0) {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        pImpl->recordedData.insert(pImpl->recordedData.end(),
                                   remaining.begin(),
                                   remaining.begin() + readFrames * pImpl->config.channels);
    }
}

size_t OptimizedAudioRecorder::testGetFileRecordedSamples() const {
    if (!(pImpl->config.recordingMode == RecordingMode::FILE_BASED
          || pImpl->config.recordingMode == RecordingMode::HYBRID)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    return pImpl->recordedData.size();
}
#endif

}  // namespace huntmaster
