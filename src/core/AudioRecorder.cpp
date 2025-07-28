#include "huntmaster/core/AudioRecorder.h"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>

#include "../libs/dr_wav.h"
#include "../libs/miniaudio.h"
#include "huntmaster/core/OptimizedAudioIO.h"
#include "huntmaster/core/ErrorLogger.h"
#include "huntmaster/core/ComponentErrorHandler.h"

namespace huntmaster {

class AudioRecorder::Impl {
  public:
    // Configuration for Voice Activity Detection (VAD) and trimming
    struct TrimConfig {
        float silenceThreshold = 0.01f;         // Amplitude threshold for silence detection
        float energyThreshold = 0.0001f;        // Energy threshold for silence detection
        float windowDurationMs = 10.0f;         // Window size for VAD in milliseconds
        float requiredSoundDurationMs = 20.0f;  // Min consecutive sound duration to mark start/end
        float hangoverDurationMs = 100.0f;      // Extra duration to keep after sound ends
        float fadeDurationMs = 5.0f;            // Duration for fade in/out to prevent clicks
    };

    TrimConfig trimConfig;

    Config config;
    ma_device device;
    ma_device_config deviceConfig;
    std::vector<float> recordedData;  // Primary memory buffer
    std::atomic<bool> recording{false};
    std::atomic<float> currentLevel{0.0f};
    mutable std::mutex dataMutex;

    // Memory management for different recording modes
    std::unique_ptr<std::vector<float>> circularBuffer;
    size_t circularBufferWritePos = 0;
    size_t totalSamplesRecorded = 0;
    std::atomic<size_t> memoryBufferSize{0};  // Thread-safe buffer size tracking

    // File-based recording components
    std::unique_ptr<AsyncAudioWriter> asyncWriter;
    std::unique_ptr<StreamingAudioBuffer> streamingBuffer;
    std::string currentOutputFilename;

    // Recording mode state
    RecordingMode currentRecordingMode = RecordingMode::MEMORY_BASED;

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
        switch (impl->currentRecordingMode) {
            case RecordingMode::MEMORY_BASED:
                impl->handleMemoryBasedRecording(input, sampleCount);
                break;

            case RecordingMode::FILE_BASED:
                impl->handleFileBasedRecording(input, sampleCount);
                break;

            case RecordingMode::HYBRID:
                impl->handleHybridRecording(input, sampleCount);
                break;
        }

        // Update total samples counter
        impl->totalSamplesRecorded += sampleCount;
        impl->memoryBufferSize.store(impl->recordedData.size());
    }

    void handleMemoryBasedRecording(const float* input, size_t sampleCount) {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (config.enableCircularBuffer && config.maxMemoryBufferSize > 0) {
            // Circular buffer mode - overwrite old data when full
            if (!circularBuffer) {
                try {
                    circularBuffer = std::make_unique<std::vector<float>>(config.maxMemoryBufferSize);
                } catch (const std::bad_alloc& e) {
                    ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                        config.maxMemoryBufferSize * sizeof(float));
                    return;
                }
            }

            for (size_t i = 0; i < sampleCount; ++i) {
                (*circularBuffer)[circularBufferWritePos] = input[i];
                circularBufferWritePos = (circularBufferWritePos + 1) % config.maxMemoryBufferSize;
            }

            // Copy current state to main buffer for compatibility
            try {
                recordedData.assign(circularBuffer->begin(), circularBuffer->end());
            } catch (const std::bad_alloc& e) {
                ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                    circularBuffer->size() * sizeof(float));
                return;
            }
        } else {
            // Linear buffer mode
            if (config.maxMemoryBufferSize > 0
                && recordedData.size() + sampleCount > config.maxMemoryBufferSize) {
                // Log buffer overflow warning
                ComponentErrorHandler::AudioEngineErrors::logBufferOverflow(
                    config.maxMemoryBufferSize, recordedData.size() + sampleCount);
                
                // Truncate if we exceed memory limit
                size_t availableSpace = config.maxMemoryBufferSize - recordedData.size();
                if (availableSpace > 0) {
                    try {
                        recordedData.insert(recordedData.end(), input, input + availableSpace);
                    } catch (const std::bad_alloc& e) {
                        ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                            availableSpace * sizeof(float));
                    }
                }
            } else {
                // Unlimited or within limits
                try {
                    recordedData.insert(recordedData.end(), input, input + sampleCount);
                } catch (const std::bad_alloc& e) {
                    ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                        sampleCount * sizeof(float));
                }
            }
        }
    }

    void handleFileBasedRecording(const float* input, size_t sampleCount) {
        // Use optimized I/O if available
        if (config.enableOptimizedIO && streamingBuffer) {
            try {
                size_t framesWritten = streamingBuffer->write(input, sampleCount / config.channels);

                // Async write to file if configured
                if (asyncWriter && asyncWriter->isActive()) {
                    asyncWriter->writeAsync(
                        input, sampleCount, [](bool success, const std::string& error) {
                            if (!success) {
                                ComponentErrorHandler::AudioEngineErrors::logProcessingError(
                                    "async_file_write", error);
                            }
                        });
                }
            } catch (const std::exception& e) {
                ComponentErrorHandler::AudioEngineErrors::logProcessingError(
                    "streaming_buffer_write", e.what());
            }
        } else {
            // Store minimal data for level monitoring and basic access
            std::lock_guard<std::mutex> lock(dataMutex);
            try {
                recordedData.insert(recordedData.end(), input, input + sampleCount);
            } catch (const std::bad_alloc& e) {
                ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                    sampleCount * sizeof(float));
            }
        }
    }

    void handleHybridRecording(const float* input, size_t sampleCount) {
        // Handle memory storage
        handleMemoryBasedRecording(input, sampleCount);

        // Handle file storage (without locking again)
        if (config.enableOptimizedIO && streamingBuffer) {
            try {
                size_t framesWritten = streamingBuffer->write(input, sampleCount / config.channels);

                if (asyncWriter && asyncWriter->isActive()) {
                    asyncWriter->writeAsync(
                        input, sampleCount, [](bool success, const std::string& error) {
                            if (!success) {
                                ComponentErrorHandler::AudioEngineErrors::logProcessingError(
                                    "hybrid_async_write", error);
                            }
                        });
                }
            } catch (const std::exception& e) {
                ComponentErrorHandler::AudioEngineErrors::logProcessingError(
                    "hybrid_file_write", e.what());
            }
        }
    }
};
};

AudioRecorder::AudioRecorder() : pImpl(std::make_unique<Impl>()) {}
AudioRecorder::~AudioRecorder() {
    stopRecording();
}

bool AudioRecorder::startRecording(const Config& config) {
    if (pImpl->recording) {
        ComponentErrorHandler::AudioEngineErrors::logRecordingStartFailure(
            "Recording already in progress");
        return false;
    }

    // Validate configuration
    if (config.sampleRate <= 0 || config.channels <= 0) {
        ComponentErrorHandler::AudioEngineErrors::logConfigurationError(
            "invalid_audio_params", 
            "sample_rate=" + std::to_string(config.sampleRate) + 
            ", channels=" + std::to_string(config.channels));
        return false;
    }

    // Store configuration and initialize state
    pImpl->config = config;
    pImpl->currentRecordingMode = config.recordingMode;
    pImpl->recordedData.clear();
    pImpl->currentLevel = 0.0f;
    pImpl->totalSamplesRecorded = 0;
    pImpl->circularBufferWritePos = 0;
    pImpl->memoryBufferSize.store(0);
    pImpl->currentOutputFilename = config.outputFilename;

    // Initialize circular buffer if needed
    if (config.enableCircularBuffer && config.maxMemoryBufferSize > 0) {
        try {
            pImpl->circularBuffer = std::make_unique<std::vector<float>>(config.maxMemoryBufferSize);
            LOG_INFO(Component::AUDIO_ENGINE, 
                    "Initialized circular buffer with " + std::to_string(config.maxMemoryBufferSize) + " samples");
        } catch (const std::bad_alloc& e) {
            ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(
                config.maxMemoryBufferSize * sizeof(float));
            return false;
        }
    }

    // Set up audio device
    pImpl->deviceConfig = ma_device_config_init(ma_device_type_capture);
    pImpl->deviceConfig.capture.format = ma_format_f32;
    pImpl->deviceConfig.capture.channels = config.channels;
    pImpl->deviceConfig.sampleRate = config.sampleRate;
    pImpl->deviceConfig.dataCallback = Impl::dataCallback;
    pImpl->deviceConfig.pUserData = pImpl.get();

    ma_result result = ma_device_init(NULL, &pImpl->deviceConfig, &pImpl->device);
    if (result != MA_SUCCESS) {
        std::string errorMsg = "Failed to initialize capture device: " + std::string(ma_result_description(result));
        ComponentErrorHandler::AudioEngineErrors::logDeviceInitFailure("default_capture", errorMsg);
        return false;
    }

    // Initialize recording mode-specific components
    if (config.recordingMode == RecordingMode::FILE_BASED
        || config.recordingMode == RecordingMode::HYBRID) {
        if (config.enableOptimizedIO) {
            // Initialize streaming buffer for file operations
            size_t bufferFrames = config.sampleRate * config.channels;  // 1 second buffer
            pImpl->streamingBuffer = std::make_unique<StreamingAudioBuffer>(
                bufferFrames, config.channels, config.sampleRate);

            // Initialize async writer for file output if filename is provided
            if (!config.outputFilename.empty()) {
                // Initialize AsyncAudioWriter for direct file streaming
                // This would need to be implemented with the specific file format
                std::cout << "File-based recording will stream to: " << config.outputFilename
                          << std::endl;
            }

            std::cout << "Optimized I/O enabled for "
                      << (config.recordingMode == RecordingMode::FILE_BASED ? "file-based"
                                                                            : "hybrid")
                      << " recording mode" << std::endl;
        }
    }

    if (ma_device_start(&pImpl->device) != MA_SUCCESS) {
        std::cerr << "Failed to start capture device." << std::endl;
        ma_device_uninit(&pImpl->device);
        return false;
    }

    pImpl->recording = true;
    std::cout << "Recording started at " << config.sampleRate << " Hz" << std::endl;
    return true;
}

void AudioRecorder::stopRecording() {
    if (!pImpl->recording)
        return;

    pImpl->recording = false;

    // Cleanup optimized I/O resources
    if (pImpl->enableOptimizedIO) {
        // Finalize async writer if it was used
        if (pImpl->asyncWriter && pImpl->asyncWriter->isActive()) {
            pImpl->asyncWriter->finalize();
        }

        // Process any remaining data in streaming buffer
        if (pImpl->streamingBuffer && pImpl->streamingBuffer->getAvailableFrames() > 0) {
            std::vector<float> remainingData(pImpl->streamingBuffer->getAvailableFrames()
                                             * pImpl->config.channels);
            size_t framesRead = pImpl->streamingBuffer->read(
                remainingData.data(), pImpl->streamingBuffer->getAvailableFrames());

            if (framesRead > 0) {
                std::lock_guard<std::mutex> lock(pImpl->dataMutex);
                pImpl->recordedData.insert(pImpl->recordedData.end(),
                                           remainingData.begin(),
                                           remainingData.begin()
                                               + framesRead * pImpl->config.channels);
            }
        }

        // Clear optimized I/O resources
        pImpl->asyncWriter.reset();
        pImpl->streamingBuffer.reset();

        std::cout << "Optimized I/O resources cleaned up" << std::endl;
    }

    ma_device_uninit(&pImpl->device);
    std::cout << "Recording stopped. Captured " << getDuration() << " seconds." << std::endl;
}

bool AudioRecorder::isRecording() const {
    return pImpl->recording;
}

std::vector<float> AudioRecorder::getRecordedData() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    return pImpl->recordedData;
}
bool AudioRecorder::saveToWavTrimmed(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);

    if (pImpl->recordedData.empty()) {
        std::cerr << "No audio data to save!" << std::endl;
        return false;
    }

    // Voice Activity Detection parameters
    const float silenceThreshold = pImpl->trimConfig.silenceThreshold;
    const float energyThreshold = pImpl->trimConfig.energyThreshold;
    const int windowSize =
        static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.windowDurationMs / 1000.0f);

    // Find audio boundaries
    size_t audioStart = 0;
    size_t audioEnd = pImpl->recordedData.size();

    // Find start
    int consecutiveSoundFrames = 0;
    int requiredFrames = static_cast<int>(pImpl->config.sampleRate
                                          * pImpl->trimConfig.requiredSoundDurationMs / 1000.0f);

    for (size_t i = 0; i < pImpl->recordedData.size(); i += windowSize / 2) {
        float energy = 0.0f;
        float peak = 0.0f;
        int count = 0;

        for (size_t j = i;
             j < std::min(i + static_cast<size_t>(windowSize), pImpl->recordedData.size());
             ++j) {
            energy += pImpl->recordedData[j] * pImpl->recordedData[j];
            peak = std::max(peak, std::abs(pImpl->recordedData[j]));
            count++;
        }

        if (count > 0) {
            energy /= count;
        }

        if (energy > energyThreshold || peak > silenceThreshold) {
            consecutiveSoundFrames += windowSize / 2;
            if (consecutiveSoundFrames >= requiredFrames) {
                audioStart = std::max(0, static_cast<int>(i) - windowSize);
                break;
            }
        } else {
            consecutiveSoundFrames = 0;
        }
    }

    // Find end (search backwards)
    int hangoverSamples =
        static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.hangoverDurationMs / 1000.0f);

    for (int i = static_cast<int>(pImpl->recordedData.size()) - windowSize; i >= 0;
         i -= windowSize / 2) {
        float energy = 0.0f;
        float peak = 0.0f;
        int count = 0;

        for (int j = i; j < std::min(i + windowSize, static_cast<int>(pImpl->recordedData.size()));
             ++j) {
            energy += pImpl->recordedData[j] * pImpl->recordedData[j];
            peak = std::max(peak, std::abs(pImpl->recordedData[j]));
            count++;
        }

        if (count > 0) {
            energy /= count;
        }

        if (energy > energyThreshold || peak > silenceThreshold) {
            audioEnd = std::min(pImpl->recordedData.size(),
                                static_cast<size_t>(i + windowSize + hangoverSamples));
            break;
        } else {
            // If we encounter silence, reset consecutive sound frames for the end detection
            // This ensures we find the *last* significant audio segment
            // (This part of the logic was missing in the original backward loop)
            // For end detection, we don't need consecutive frames, just the first significant one
            // from the end
        }
    }

    // Ensure valid range
    if (audioStart >= audioEnd) {
        std::cerr << "Warning: No significant audio detected, saving full recording" << std::endl;
        audioStart = 0;
        audioEnd = pImpl->recordedData.size();
    }

    // Create trimmed data
    std::vector<float> trimmedData(pImpl->recordedData.begin() + audioStart,
                                   pImpl->recordedData.begin() + audioEnd);

    // Apply fade in/out to avoid clicks
    int fadeLength =
        static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.fadeDurationMs / 1000.0f);

    // Ensure fadeLength does not exceed half of trimmedData.size() to prevent overlap
    fadeLength = std::min(fadeLength, static_cast<int>(trimmedData.size() / 2));

    for (int i = 0; i < fadeLength; ++i) {
        float factor = static_cast<float>(i) / fadeLength;
        trimmedData[i] *= factor;
    }
    for (int i = 0; i < fadeLength; ++i) {
        float factor = static_cast<float>(i) / fadeLength;
        trimmedData[trimmedData.size() - 1 - i] *=
            (1.0f - factor);  // Use (1.0f - factor) for fade out
    }

    // Save trimmed audio
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = pImpl->config.channels;
    format.sampleRate = pImpl->config.sampleRate;
    format.bitsPerSample = 32;

    // RAII for drwav to ensure uninit is called
    struct DrWavGuard {
        drwav* wav;
        DrWavGuard(drwav* w) : wav(w) {}
        ~DrWavGuard() {
            if (wav) {
                drwav_uninit(wav);
            }
        }
        DrWavGuard(const DrWavGuard&) = delete;
        DrWavGuard& operator=(const DrWavGuard&) = delete;
    };

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL)) {
        std::cerr << "Failed to create WAV file: " << filename << std::endl;
        return false;
    }
    DrWavGuard wavGuard(&wav);

    drwav_uint64 expectedFrames = trimmedData.size() / pImpl->config.channels;
    drwav_uint64 samplesWritten = drwav_write_pcm_frames(&wav, expectedFrames, trimmedData.data());

    float originalDuration = static_cast<float>(pImpl->recordedData.size())
                             / (pImpl->config.sampleRate * pImpl->config.channels);
    float trimmedDuration = static_cast<float>(trimmedData.size())
                            / (pImpl->config.sampleRate * pImpl->config.channels);

    std::cout << "Saved trimmed audio to " << filename << std::endl;
    std::cout << "  Original: " << std::fixed << std::setprecision(2) << originalDuration << "s"
              << std::endl;
    std::cout << "  Trimmed:  " << std::fixed << std::setprecision(2) << trimmedDuration << "s"
              << std::endl;
    std::cout << "  Removed:  " << std::fixed << std::setprecision(2)
              << (originalDuration - trimmedDuration) << "s of silence" << std::endl;

    return samplesWritten == expectedFrames;
}
bool AudioRecorder::saveToWav(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);

    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = pImpl->config.channels;
    format.sampleRate = pImpl->config.sampleRate;
    format.bitsPerSample = 32;

    // RAII for drwav to ensure uninit is called
    struct DrWavGuard {
        drwav* wav;
        DrWavGuard(drwav* w) : wav(w) {}
        ~DrWavGuard() {
            if (wav) {
                drwav_uninit(wav);
            }
        }
        DrWavGuard(const DrWavGuard&) = delete;
        DrWavGuard& operator=(const DrWavGuard&) = delete;
    };

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL)) {
        std::cerr << "Failed to create WAV file: " << filename << std::endl;
        return false;
    }
    DrWavGuard wavGuard(&wav);

    drwav_uint64 expectedFrames = pImpl->recordedData.size() / pImpl->config.channels;
    drwav_uint64 samplesWritten =
        drwav_write_pcm_frames(&wav, expectedFrames, pImpl->recordedData.data());

    std::cout << "Saved " << samplesWritten << " frames to " << filename << std::endl;
    return samplesWritten == expectedFrames;
}

float AudioRecorder::getCurrentLevel() const {
    return pImpl->currentLevel;
}

double AudioRecorder::getDuration() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    if (pImpl->config.sampleRate == 0 || pImpl->config.channels == 0)
        return 0.0;
    return static_cast<double>(pImpl->recordedData.size())
           / (pImpl->config.sampleRate * pImpl->config.channels);
}

// New memory-based recording methods
AudioRecorder::RecordingMode AudioRecorder::getRecordingMode() const {
    return pImpl->currentRecordingMode;
}

size_t AudioRecorder::getRecordedDataSize() const {
    return pImpl->memoryBufferSize.load();
}

size_t AudioRecorder::copyRecordedData(float* buffer, size_t maxSamples) const {
    if (!buffer || maxSamples == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    size_t samplesToCopy = std::min(maxSamples, pImpl->recordedData.size());

    if (samplesToCopy > 0) {
        std::memcpy(buffer, pImpl->recordedData.data(), samplesToCopy * sizeof(float));
    }

    return samplesToCopy;
}

bool AudioRecorder::clearMemoryBuffer() {
    if (pImpl->recording) {
        // Don't clear buffer while recording is active
        return false;
    }

    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    pImpl->recordedData.clear();
    pImpl->totalSamplesRecorded = 0;
    pImpl->circularBufferWritePos = 0;
    pImpl->memoryBufferSize.store(0);

    if (pImpl->circularBuffer) {
        std::fill(pImpl->circularBuffer->begin(), pImpl->circularBuffer->end(), 0.0f);
    }

    return true;
}

bool AudioRecorder::isMemoryBufferNearCapacity(float thresholdPercent) const {
    if (pImpl->config.maxMemoryBufferSize == 0) {
        return false;  // Unlimited buffer
    }

    size_t currentSize = pImpl->memoryBufferSize.load();
    float utilization =
        static_cast<float>(currentSize) / static_cast<float>(pImpl->config.maxMemoryBufferSize);

    return utilization >= thresholdPercent;
}

AudioRecorder::MemoryBufferStats AudioRecorder::getMemoryBufferStats() const {
    MemoryBufferStats stats;

    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    stats.currentSamples = pImpl->recordedData.size();
    stats.maxSamples = pImpl->config.maxMemoryBufferSize;
    stats.bytesUsed = stats.currentSamples * sizeof(float);
    stats.isCircular = pImpl->config.enableCircularBuffer;

    if (stats.maxSamples > 0) {
        stats.utilizationPercent = static_cast<float>(stats.currentSamples)
                                   / static_cast<float>(stats.maxSamples) * 100.0f;
    } else {
        stats.utilizationPercent = 0.0f;  // Unlimited buffer
    }

    return stats;
}

bool AudioRecorder::saveMemoryBufferToWav(const std::string& filename, bool applyTrimming) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);

    if (pImpl->recordedData.empty()) {
        std::cerr << "No audio data in memory buffer to save!" << std::endl;
        return false;
    }

    // Create a temporary copy to avoid modifying the original data
    std::vector<float> dataToSave = pImpl->recordedData;

    // Apply trimming if requested
    if (applyTrimming) {
        // Use the same trimming logic as saveToWavTrimmed but on our copy
        const float silenceThreshold = pImpl->trimConfig.silenceThreshold;
        const float energyThreshold = pImpl->trimConfig.energyThreshold;
        const int windowSize = static_cast<int>(pImpl->config.sampleRate
                                                * pImpl->trimConfig.windowDurationMs / 1000.0f);

        size_t audioStart = 0;
        size_t audioEnd = dataToSave.size();

        // Find start of audio (same logic as in saveToWavTrimmed)
        int consecutiveSoundFrames = 0;
        int requiredFrames = static_cast<int>(
            pImpl->config.sampleRate * pImpl->trimConfig.requiredSoundDurationMs / 1000.0f);

        for (size_t i = 0; i < dataToSave.size(); i += windowSize / 2) {
            float energy = 0.0f;
            float peak = 0.0f;
            int count = 0;

            for (int j = i; j < std::min(i + windowSize, static_cast<int>(dataToSave.size()));
                 ++j) {
                energy += dataToSave[j] * dataToSave[j];
                peak = std::max(peak, std::abs(dataToSave[j]));
                count++;
            }

            if (count > 0) {
                energy /= count;
            }

            if (energy > energyThreshold || peak > silenceThreshold) {
                consecutiveSoundFrames += windowSize / 2;
                if (consecutiveSoundFrames >= requiredFrames) {
                    audioStart = std::max(0, static_cast<int>(i) - windowSize);
                    break;
                }
            } else {
                consecutiveSoundFrames = 0;
            }
        }

        // Find end of audio (search backwards)
        int hangoverSamples = static_cast<int>(pImpl->config.sampleRate
                                               * pImpl->trimConfig.hangoverDurationMs / 1000.0f);

        for (int i = static_cast<int>(dataToSave.size()) - windowSize; i >= 0;
             i -= windowSize / 2) {
            float energy = 0.0f;
            float peak = 0.0f;
            int count = 0;

            for (int j = i; j < std::min(i + windowSize, static_cast<int>(dataToSave.size()));
                 ++j) {
                energy += dataToSave[j] * dataToSave[j];
                peak = std::max(peak, std::abs(dataToSave[j]));
                count++;
            }

            if (count > 0) {
                energy /= count;
            }

            if (energy > energyThreshold || peak > silenceThreshold) {
                audioEnd = std::min(dataToSave.size(),
                                    static_cast<size_t>(i + windowSize + hangoverSamples));
                break;
            }
        }

        // Ensure valid range
        if (audioStart >= audioEnd) {
            std::cerr << "Warning: No significant audio detected, saving full buffer" << std::endl;
            audioStart = 0;
            audioEnd = dataToSave.size();
        }

        // Apply trimming
        dataToSave =
            std::vector<float>(dataToSave.begin() + audioStart, dataToSave.begin() + audioEnd);

        // Apply fade in/out to avoid clicks
        int fadeLength =
            static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.fadeDurationMs / 1000.0f);
        fadeLength = std::min(fadeLength, static_cast<int>(dataToSave.size()) / 2);

        // Fade in
        for (int i = 0; i < fadeLength; ++i) {
            float factor = static_cast<float>(i) / fadeLength;
            dataToSave[i] *= factor;
        }

        // Fade out
        for (int i = 0; i < fadeLength; ++i) {
            float factor = static_cast<float>(fadeLength - i) / fadeLength;
            dataToSave[dataToSave.size() - 1 - i] *= factor;
        }
    }

    // Save to WAV file
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = pImpl->config.channels;
    format.sampleRate = pImpl->config.sampleRate;
    format.bitsPerSample = 32;

    struct DrWavGuard {
        drwav* wav;
        DrWavGuard(drwav* w) : wav(w) {}
        ~DrWavGuard() {
            drwav_uninit(wav);
        }
    };

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL)) {
        std::cerr << "Failed to create WAV file: " << filename << std::endl;
        return false;
    }
    DrWavGuard wavGuard(&wav);

    drwav_uint64 expectedFrames = dataToSave.size() / pImpl->config.channels;
    drwav_uint64 samplesWritten = drwav_write_pcm_frames(&wav, expectedFrames, dataToSave.data());

    std::cout << "Saved memory buffer (" << dataToSave.size() << " samples) to " << filename
              << std::endl;
    return samplesWritten == expectedFrames;
}

}  // namespace huntmaster
