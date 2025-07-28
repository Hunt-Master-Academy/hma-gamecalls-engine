#include "huntmaster/core/OptimizedAudioRecorder.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "../libs/dr_wav.h"
#include "huntmaster/core/AdvancedIOOptimizer.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/OptimizedAudioIO.h"

namespace huntmaster {

// OptimizedAudioRecorder Implementation
class OptimizedAudioRecorder::Impl {
  public:
    Config config;
    std::unique_ptr<StreamingAudioBuffer> streamingBuffer;
    std::unique_ptr<AsyncAudioWriter> asyncWriter;
    std::unique_ptr<ChunkedAudioProcessor> chunkProcessor;

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

    // Advanced I/O optimizer integration
    std::unique_ptr<MasterIOOptimizer> ioOptimizer;
    MasterIOOptimizer::OptimizedHandle optimizedHandle;

    Impl() {
        deviceConfig = ma_device_config_init(ma_device_type_capture);

        // Initialize I/O optimizer with real-time recording profile
        MasterIOOptimizer::OptimizationProfile profile;
        profile.workloadType =
            MasterIOOptimizer::OptimizationProfile::WorkloadType::REAL_TIME_RECORDING;
        profile.maxLatency = std::chrono::microseconds(2000);  // 2ms max latency
        profile.enableCompression = false;                     // No compression for real-time

        ioOptimizer = std::make_unique<MasterIOOptimizer>(profile);
        if (ioOptimizer->initialize()) {
            optimizedHandle = ioOptimizer->optimizeForPath("../data/recordings/");
        }
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

        // Use advanced I/O optimizer for memory operations if available
        if (optimizedHandle && optimizedHandle->bufferManager) {
            // Get optimized buffer
            size_t actualSamples;
            auto optimizedBuffer =
                optimizedHandle->bufferManager->getBuffer(sampleCount, actualSamples);

            if (optimizedBuffer && actualSamples >= sampleCount) {
                // Copy to optimized buffer first
                std::memcpy(optimizedBuffer, input, sampleCount * sizeof(float));

                // Then copy to memory buffer with NUMA-aware allocation
                std::lock_guard<std::mutex> lock(memoryMutex);
                std::memcpy(
                    &memoryBuffer[currentUsed], optimizedBuffer, sampleCount * sizeof(float));
                memoryBufferUsed.store(currentUsed + sampleCount);

                // Record buffer utilization for adaptation
                optimizedHandle->bufferManager->recordUtilization(
                    sampleCount * sizeof(float),
                    actualSamples * sizeof(float),
                    std::chrono::nanoseconds(100)  // Minimal processing time for memory copy
                );
                return;
            }
        }

        // Fallback to standard memory copy
        std::lock_guard<std::mutex> lock(memoryMutex);
        std::memcpy(&memoryBuffer[currentUsed], input, sampleCount * sizeof(float));
        memoryBufferUsed.store(currentUsed + sampleCount);
    }

    void handleFileRecording(const float* input, ma_uint32 frameCount) {
        // Process with streaming buffer for real-time performance
        if (streamingBuffer) {
            size_t framesWritten = streamingBuffer->write(input, frameCount);

            // Process chunks when buffer reaches optimal size
            if (streamingBuffer->getAvailableFrames() >= config.chunkSize) {
                std::vector<float> chunkData(config.chunkSize * config.channels);
                size_t framesRead = streamingBuffer->read(chunkData.data(), config.chunkSize);

                if (framesRead > 0) {
                    // Process chunk if processor is available
                    if (chunkProcessor) {
                        auto processedChunk = chunkProcessor->processChunk(chunkData, framesRead);
                        if (processedChunk) {
                            std::lock_guard<std::mutex> lock(dataMutex);
                            recordedData.insert(
                                recordedData.end(), processedChunk->begin(), processedChunk->end());
                        }
                    } else {
                        // Store unprocessed chunk
                        std::lock_guard<std::mutex> lock(dataMutex);
                        recordedData.insert(recordedData.end(),
                                            chunkData.begin(),
                                            chunkData.begin() + framesRead * config.channels);
                    }

                    // Use advanced async I/O if available
                    if (optimizedHandle && optimizedHandle->asyncIO) {
                        // Use optimized async write
                        optimizedHandle->asyncIO->writeAsync(
                            -1,  // File descriptor managed by AsyncAudioWriter
                            chunkData.data(),
                            framesRead * config.channels * sizeof(float),
                            0,  // Offset managed by writer
                            [](bool success,
                               size_t bytesTransferred,
                               std::chrono::nanoseconds latency) {
                                if (!success) {
                                    std::cerr << "Optimized async write failed" << std::endl;
                                }
                            });
                    } else if (asyncWriter && asyncWriter->isActive()) {
                        // Fallback to regular async write
                        asyncWriter->writeAsync(chunkData.data(),
                                                framesRead * config.channels,
                                                [](bool success, const std::string& error) {
                                                    if (!success) {
                                                        std::cerr << "Async write failed: " << error
                                                                  << std::endl;
                                                    }
                                                });
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

bool OptimizedAudioRecorder::configure(const Config& config) {
    if (pImpl->recording) {
        return false;  // Cannot configure while recording
    }

    pImpl->config = config;

    // Configure memory buffer for memory-based recording
    if (config.recordingMode == RecordingMode::MEMORY_BASED
        || config.recordingMode == RecordingMode::HYBRID) {
        std::lock_guard<std::mutex> lock(pImpl->memoryMutex);
        try {
            pImpl->memoryBuffer.clear();
            pImpl->memoryBuffer.reserve(config.memoryBufferMaxFrames);
            pImpl->memoryBuffer.resize(config.memoryBufferMaxFrames);
            pImpl->memoryBufferUsed = 0;
            pImpl->memoryOverflow = false;
        } catch (const std::bad_alloc&) {
            return false;
        }
    }

    // Configure miniaudio device
    pImpl->deviceConfig.capture.format = ma_format_f32;
    pImpl->deviceConfig.capture.channels = config.channels;
    pImpl->deviceConfig.sampleRate = config.sampleRate;
    pImpl->deviceConfig.dataCallback = Impl::dataCallback;
    pImpl->deviceConfig.pUserData = pImpl.get();

    // Initialize streaming buffer with optimal size for file-based recording
    if (config.recordingMode == RecordingMode::FILE_BASED
        || config.recordingMode == RecordingMode::HYBRID) {
        size_t bufferFrames = config.sampleRate * config.bufferDurationMs / 1000;
        pImpl->streamingBuffer = std::make_unique<StreamingAudioBuffer>(
            bufferFrames, config.channels, config.sampleRate);

        // Initialize async writer if output file is specified
        if (!config.outputFile.empty()) {
            pImpl->asyncWriter = std::make_unique<AsyncAudioWriter>(
                config.outputFile, config.sampleRate, config.channels);
            if (!pImpl->asyncWriter->initialize()) {
                return false;
            }
        }

        // Initialize chunk processor if processing is enabled
        if (config.enableChunkProcessing) {
            pImpl->chunkProcessor = std::make_unique<ChunkedAudioProcessor>(
                config.chunkSize, config.channels, config.sampleRate);
        }
    }

    return true;
}

bool OptimizedAudioRecorder::start() {
    if (pImpl->recording) {
        return false;
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
ma_result result = ma_device_init(nullptr, &pImpl->deviceConfig, &pImpl->device);
if (result != MA_SUCCESS) {
    LOG_ERROR(Component::AUDIO_ENGINE, "Failed to initialize audio device: " + std::string(ma_result_description(result)));
    return Error("Failed to initialize device: " + std::string(ma_result_description(result)));
}

// Start device
result = ma_device_start(&pImpl->device);
if (result != MA_SUCCESS) {
    LOG_ERROR(Component::AUDIO_ENGINE, "Failed to start audio device: " + std::string(ma_result_description(result)));
    ma_device_uninit(&pImpl->device);
    return Error("Failed to start device: " + std::string(ma_result_description(result)));
}

pImpl->recording = true;
std::cout << "Optimized recording started at " << pImpl->config.sampleRate << " Hz" << std::endl;
return {};
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
        pImpl->asyncWriter->finalize();
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

    // Use advanced I/O optimizer for saving if available
    if (pImpl->optimizedHandle && pImpl->optimizedHandle->compression) {
        // Save with optimized compression if enabled
        if (pImpl->config.enableMemoryCompression) {
            // Use intelligent compression
            std::vector<uint8_t> compressedData;
            if (pImpl->optimizedHandle->compression->compressAudio(
                    pImpl->memoryBuffer.data(), used, compressedData)) {
                // Save compressed data (would need custom format support)
                std::cout << "Compressed " << used << " samples to " << compressedData.size()
                          << " bytes" << std::endl;
            }
        }
    }

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

Result<void> OptimizedAudioRecorder::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);

    if (pImpl->recordedData.empty()) {
        LOG_ERROR(Component::AUDIO_ENGINE, "saveToFile failed: No audio data to save for file: " + filename);
        return Error("No audio data to save");
    }

    // Use dr_wav for saving
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = pImpl->config.channels;
    format.sampleRate = pImpl->config.sampleRate;
    format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) {
        LOG_ERROR(Component::AUDIO_ENGINE, "Failed to initialize WAV file for writing: " + filename);
        return Error("Failed to initialize WAV file for writing");
    }

    drwav_uint64 framesWritten =
        drwav_write_pcm_frames(&wav, getRecordedFrames(), pImpl->recordedData.data());
    drwav_uninit(&wav);

    if (framesWritten != getRecordedFrames()) {
        LOG_ERROR(Component::AUDIO_ENGINE, "Failed to write all frames to file: " + filename + 
                 " (wrote " + std::to_string(framesWritten) + " of " + std::to_string(getRecordedFrames()) + " frames)");
        return Error("Failed to write all frames to file");
    }

    return {};
}

// OptimizedAudioPlayer Implementation
class OptimizedAudioPlayer::Impl {
  public:
    PlayerConfig config;
    std::unique_ptr<MemoryMappedAudioFile> memoryMappedFile;
    std::unique_ptr<StreamingAudioBuffer> streamingBuffer;

    ma_device device;
    ma_device_config deviceConfig;
    std::atomic<bool> playing{false};
    std::atomic<size_t> currentFrame{0};

    std::vector<float> audioData;

    Impl() {
        deviceConfig = ma_device_config_init(ma_device_type_playback);
    }

    ~Impl() {
        if (playing) {
            playing = false;
            ma_device_uninit(&device);
        }
    }

    static void
    dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        if (!impl || !impl->playing)
            return;

        float* output = static_cast<float*>(pOutput);
        size_t currentFrame = impl->currentFrame.load();
        size_t totalFrames = impl->audioData.size() / impl->config.channels;

        if (currentFrame >= totalFrames) {
            impl->playing = false;
            return;
        }

        size_t framesToCopy = std::min(static_cast<size_t>(frameCount), totalFrames - currentFrame);
        size_t samplesToCopy = framesToCopy * impl->config.channels;

        std::memcpy(output,
                    &impl->audioData[currentFrame * impl->config.channels],
                    samplesToCopy * sizeof(float));

        // Zero out remaining samples if needed
        if (framesToCopy < frameCount) {
            std::memset(&output[samplesToCopy],
                        0,
                        (frameCount * impl->config.channels - samplesToCopy) * sizeof(float));
        }

        impl->currentFrame.fetch_add(framesToCopy);
    }
};

OptimizedAudioPlayer::OptimizedAudioPlayer() : pImpl(std::make_unique<Impl>()) {}

OptimizedAudioPlayer::~OptimizedAudioPlayer() = default;

Result<void> OptimizedAudioPlayer::loadFile(const std::string& filename) {
    if (pImpl->playing) {
        return Error("Cannot load file while playing");
    }

    // Try to use memory-mapped file for large files
    pImpl->memoryMappedFile = std::make_unique<MemoryMappedAudioFile>();
    auto mapResult = pImpl->memoryMappedFile->mapFile(filename);

    if (mapResult) {
        // Successfully memory-mapped
        auto audioInfo = pImpl->memoryMappedFile->getAudioInfo();
        pImpl->config.sampleRate = audioInfo.sampleRate;
        pImpl->config.channels = audioInfo.channels;

        // Copy data for playback (in a real implementation, we might stream directly)
        pImpl->audioData = pImpl->memoryMappedFile->getAudioData();

        std::cout << "File loaded using memory mapping: " << filename << std::endl;
    } else {
        // Fallback to regular file loading using dr_wav
        drwav wav;
        if (!drwav_init_file(&wav, filename.c_str(), nullptr)) {
            return Error("Failed to open audio file: " + filename);
        }

        pImpl->config.sampleRate = wav.sampleRate;
        pImpl->config.channels = wav.channels;

        pImpl->audioData.resize(wav.totalPCMFrameCount * wav.channels);
        drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, pImpl->audioData.data());
        drwav_uninit(&wav);

        std::cout << "File loaded using standard method: " << filename << std::endl;
    }

    // Configure device
    pImpl->deviceConfig.playback.format = ma_format_f32;
    pImpl->deviceConfig.playback.channels = pImpl->config.channels;
    pImpl->deviceConfig.sampleRate = pImpl->config.sampleRate;
    pImpl->deviceConfig.dataCallback = Impl::dataCallback;
    pImpl->deviceConfig.pUserData = pImpl.get();

    return {};
}

Result<void> OptimizedAudioPlayer::play() {
    if (pImpl->playing) {
        return Error("Already playing");
    }

    if (pImpl->audioData.empty()) {
        return Error("No audio data loaded");
    }

    // Initialize device
    ma_result result = ma_device_init(nullptr, &pImpl->deviceConfig, &pImpl->device);
    if (result != MA_SUCCESS) {
        return Error("Failed to initialize playback device: "
                     + std::string(ma_result_description(result)));
    }

    // Start playback
    result = ma_device_start(&pImpl->device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&pImpl->device);
        return Error("Failed to start playback: " + std::string(ma_result_description(result)));
    }

    pImpl->playing = true;
    pImpl->currentFrame = 0;

    std::cout << "Optimized playback started" << std::endl;
    return {};
}

void OptimizedAudioPlayer::stop() {
    if (!pImpl->playing)
        return;

    pImpl->playing = false;
    ma_device_uninit(&pImpl->device);

    std::cout << "Optimized playback stopped" << std::endl;
}

void OptimizedAudioPlayer::pause() {
    if (pImpl->playing) {
        pImpl->playing = false;
    }
}

void OptimizedAudioPlayer::resume() {
    if (!pImpl->playing && !pImpl->audioData.empty()) {
        pImpl->playing = true;
    }
}

bool OptimizedAudioPlayer::isPlaying() const {
    return pImpl->playing;
}

double OptimizedAudioPlayer::getCurrentPosition() const {
    return static_cast<double>(pImpl->currentFrame.load()) / pImpl->config.sampleRate;
}

double OptimizedAudioPlayer::getDuration() const {
    return static_cast<double>(pImpl->audioData.size() / pImpl->config.channels)
           / pImpl->config.sampleRate;
}

void OptimizedAudioPlayer::setPosition(double seconds) {
    size_t frame = static_cast<size_t>(seconds * pImpl->config.sampleRate);
    size_t totalFrames = pImpl->audioData.size() / pImpl->config.channels;
    pImpl->currentFrame = std::min(frame, totalFrames);
}

// BatchAudioProcessor Implementation
class BatchAudioProcessor::Impl {
  public:
    ProcessorConfig config;
    std::unique_ptr<ChunkedAudioProcessor> chunkProcessor;

    Impl(const ProcessorConfig& cfg) : config(cfg) {
        chunkProcessor = std::make_unique<ChunkedAudioProcessor>(
            config.chunkSize, config.channels, config.sampleRate);
    }
};

BatchAudioProcessor::BatchAudioProcessor(const ProcessorConfig& config)
    : pImpl(std::make_unique<Impl>(config)) {}

BatchAudioProcessor::~BatchAudioProcessor() = default;

Result<std::vector<float>> BatchAudioProcessor::processFile(const std::string& inputFile,
                                                            const std::string& outputFile) {
    // Load input file using memory mapping for efficiency
    MemoryMappedAudioFile mappedFile;
    auto mapResult = mappedFile.mapFile(inputFile);

    std::vector<float> audioData;
    uint32_t sampleRate, channels;

    if (mapResult) {
        auto audioInfo = mappedFile.getAudioInfo();
        audioData = mappedFile.getAudioData();
        sampleRate = audioInfo.sampleRate;
        channels = audioInfo.channels;
    } else {
        // Fallback to regular loading
        drwav wav;
        if (!drwav_init_file(&wav, inputFile.c_str(), nullptr)) {
            return Error("Failed to open input file: " + inputFile);
        }

        sampleRate = wav.sampleRate;
        channels = wav.channels;
        audioData.resize(wav.totalPCMFrameCount * wav.channels);
        drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, audioData.data());
        drwav_uninit(&wav);
    }

    // Process in chunks
    std::vector<float> processedData;
    size_t totalFrames = audioData.size() / channels;

    for (size_t frameOffset = 0; frameOffset < totalFrames;
         frameOffset += pImpl->config.chunkSize) {
        size_t framesToProcess = std::min(pImpl->config.chunkSize, totalFrames - frameOffset);
        size_t sampleOffset = frameOffset * channels;
        size_t samplesToProcess = framesToProcess * channels;

        std::vector<float> chunk(audioData.begin() + sampleOffset,
                                 audioData.begin() + sampleOffset + samplesToProcess);

        auto processedChunk = pImpl->chunkProcessor->processChunk(chunk, framesToProcess);
        if (processedChunk) {
            processedData.insert(
                processedData.end(), processedChunk->begin(), processedChunk->end());
        }
    }

    // Save processed data if output file is specified
    if (!outputFile.empty()) {
        drwav wav;
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 32;

        if (!drwav_init_file_write(&wav, outputFile.c_str(), &format, nullptr)) {
            return Error("Failed to initialize output file: " + outputFile);
        }

        drwav_uint64 framesWritten =
            drwav_write_pcm_frames(&wav, processedData.size() / channels, processedData.data());
        drwav_uninit(&wav);

        if (framesWritten != processedData.size() / channels) {
            return Error("Failed to write all frames to output file");
        }
    }

    return processedData;
}

Result<void> BatchAudioProcessor::processMultipleFiles(const std::vector<std::string>& inputFiles,
                                                       const std::string& outputDirectory) {
    for (const auto& inputFile : inputFiles) {
        // Extract filename without path and extension
        size_t lastSlash = inputFile.find_last_of("/\\");
        size_t lastDot = inputFile.find_last_of('.');
        std::string baseName = inputFile.substr(lastSlash + 1, lastDot - lastSlash - 1);

        std::string outputFile = outputDirectory + "/" + baseName + "_processed.wav";

        auto result = processFile(inputFile, outputFile);
        if (!result) {
            return Error("Failed to process file " + inputFile + ": " + result.error());
        }

        std::cout << "Processed: " << inputFile << " -> " << outputFile << std::endl;
    }

    return {};
}

}  // namespace huntmaster
