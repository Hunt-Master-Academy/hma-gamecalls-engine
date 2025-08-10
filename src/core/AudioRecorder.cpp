#include "huntmaster/core/AudioRecorder.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include "huntmaster/core/ComponentErrorHandler.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace huntmaster {

class AudioRecorder::Impl {
  public:
    Config config;
    ma_device device;
    ma_device_config deviceConfig;
    std::vector<float> recordedData;
    std::atomic<bool> recording{false};
    std::atomic<float> currentLevel{0.0f};
    mutable std::mutex dataMutex;

    static void data_callback(ma_device* pDevice,
                              [[maybe_unused]] void* pOutput,
                              const void* pInput,
                              ma_uint32 frameCount) {
        Impl* impl = static_cast<Impl*>(pDevice->pUserData);
        if (impl->recording) {
            std::lock_guard<std::mutex> lock(impl->dataMutex);
            const float* input = static_cast<const float*>(pInput);
            size_t samplesToAppend = frameCount * impl->config.channels;

            if (impl->config.enableCircularBuffer && impl->config.maxMemoryBufferSize > 0) {
                size_t currentSize = impl->recordedData.size();
                size_t capacity = impl->config.maxMemoryBufferSize;
                if (currentSize + samplesToAppend > capacity) {
                    size_t overflow = (currentSize + samplesToAppend) - capacity;
                    impl->recordedData.erase(impl->recordedData.begin(),
                                             impl->recordedData.begin() + overflow);
                }
            }
            impl->recordedData.insert(impl->recordedData.end(), input, input + samplesToAppend);

            float maxLevel = 0.0f;
            for (ma_uint32 i = 0; i < samplesToAppend; ++i) {
                if (std::abs(input[i]) > maxLevel) {
                    maxLevel = std::abs(input[i]);
                }
            }
            impl->currentLevel.store(maxLevel);
        }
    }
};

AudioRecorder::AudioRecorder() : pImpl(std::make_unique<Impl>()) {}

AudioRecorder::~AudioRecorder() {
    if (pImpl && pImpl->recording) {
        stopRecording();
    }
}

bool AudioRecorder::startRecording(const Config& config) {
    if (pImpl->recording) {
        ComponentErrorHandler::AudioEngineErrors::logRecordingStartFailure(
            "Recording is already in progress.");
        return false;
    }

    pImpl->config = config;
    pImpl->recordedData.clear();
    pImpl->currentLevel = 0.0f;

    pImpl->deviceConfig = ma_device_config_init(ma_device_type_capture);
    pImpl->deviceConfig.capture.format = ma_format_f32;
    pImpl->deviceConfig.capture.channels = pImpl->config.channels;
    pImpl->deviceConfig.sampleRate = pImpl->config.sampleRate;
    pImpl->deviceConfig.dataCallback = Impl::data_callback;
    pImpl->deviceConfig.pUserData = pImpl.get();

    if (ma_device_init(NULL, &pImpl->deviceConfig, &pImpl->device) != MA_SUCCESS) {
        ComponentErrorHandler::AudioEngineErrors::logDeviceInitFailure(
            "capture_device", "Failed to initialize capture device.");
        return false;
    }

    if (ma_device_start(&pImpl->device) != MA_SUCCESS) {
        ma_device_uninit(&pImpl->device);
        ComponentErrorHandler::AudioEngineErrors::logRecordingStartFailure(
            "Failed to start capture device.");
        return false;
    }

    pImpl->recording = true;
    return true;
}

void AudioRecorder::stopRecording() {
    if (!pImpl->recording) {
        return;
    }

    pImpl->recording = false;
    ma_device_uninit(&pImpl->device);
}

bool AudioRecorder::isRecording() const {
    return pImpl->recording;
}

std::vector<float> AudioRecorder::getRecordedData() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    return pImpl->recordedData;
}

bool AudioRecorder::saveToWav(const std::string& filename) const {
    return saveMemoryBufferToWav(filename, false);
}

bool AudioRecorder::saveToWavTrimmed(const std::string& filename) const {
    return saveMemoryBufferToWav(filename, true);
}

float AudioRecorder::getCurrentLevel() const {
    return pImpl->currentLevel.load();
}

double AudioRecorder::getDuration() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    if (pImpl->config.sampleRate == 0) {
        return 0.0;
    }
    return static_cast<double>(pImpl->recordedData.size())
           / (pImpl->config.channels * pImpl->config.sampleRate);
}

AudioRecorder::RecordingMode AudioRecorder::getRecordingMode() const {
    return pImpl->config.recordingMode;
}

size_t AudioRecorder::getRecordedDataSize() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    return pImpl->recordedData.size();
}

size_t AudioRecorder::copyRecordedData(float* buffer, size_t maxSamples) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    size_t samplesToCopy = std::min(maxSamples, pImpl->recordedData.size());
    if (samplesToCopy > 0) {
        memcpy(buffer, pImpl->recordedData.data(), samplesToCopy * sizeof(float));
    }
    return samplesToCopy;
}

bool AudioRecorder::clearMemoryBuffer() {
    if (pImpl->recording) {
        ComponentErrorHandler::AudioEngineErrors::logProcessingError(
            "clearMemoryBuffer", "Cannot clear buffer while recording.");
        return false;
    }
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    pImpl->recordedData.clear();
    pImpl->recordedData.shrink_to_fit();
    return true;
}

bool AudioRecorder::isMemoryBufferNearCapacity(float thresholdPercent) const {
    if (pImpl->config.maxMemoryBufferSize == 0) {
        return false;  // unlimited
    }
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    return (pImpl->recordedData.size() >= pImpl->config.maxMemoryBufferSize * thresholdPercent);
}

AudioRecorder::MemoryBufferStats AudioRecorder::getMemoryBufferStats() const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    MemoryBufferStats stats;
    stats.currentSamples = pImpl->recordedData.size();
    stats.maxSamples = pImpl->config.maxMemoryBufferSize;
    stats.bytesUsed = stats.currentSamples * sizeof(float);
    stats.isCircular = pImpl->config.enableCircularBuffer;
    if (stats.maxSamples > 0) {
        stats.utilizationPercent =
            static_cast<float>(stats.currentSamples) / stats.maxSamples * 100.0f;
    } else {
        stats.utilizationPercent = 0.0f;
    }
    return stats;
}

bool AudioRecorder::saveMemoryBufferToWav(const std::string& filename, bool applyTrimming) const {
    std::lock_guard<std::mutex> lock(pImpl->dataMutex);
    if (pImpl->recordedData.empty()) {
        ComponentErrorHandler::AudioEngineErrors::logInvalidAudioData(0,
                                                                      "No data in buffer to save.");
        return false;
    }

    std::vector<float> dataToSave = pImpl->recordedData;
    if (applyTrimming) {
        // Basic trimming: remove leading/trailing silence
        // A more advanced implementation would use a VAD
        const float silenceThreshold = 0.01f;
        auto first_not_silent = std::find_if(dataToSave.begin(), dataToSave.end(), [=](float s) {
            return std::abs(s) > silenceThreshold;
        });
        auto last_not_silent = std::find_if(dataToSave.rbegin(), dataToSave.rend(), [=](float s) {
            return std::abs(s) > silenceThreshold;
        });

        if (first_not_silent != dataToSave.end()) {
            dataToSave.erase(dataToSave.begin(), first_not_silent);
            dataToSave.erase(last_not_silent.base(), dataToSave.end());
        } else {
            // All silent, save nothing
            return true;
        }
    }

    if (dataToSave.empty()) {
        return true;  // Nothing to save after trimming
    }

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = pImpl->config.channels;
    format.sampleRate = pImpl->config.sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL)) {
        ComponentErrorHandler::IOErrors::logFileWriteError(
            filename, dataToSave.size() * sizeof(float), "Failed to initialize WAV file writer");
        return false;
    }

    drwav_uint64 framesWritten =
        drwav_write_pcm_frames(&wav, dataToSave.size() / pImpl->config.channels, dataToSave.data());
    drwav_uninit(&wav);

    if (framesWritten != dataToSave.size() / pImpl->config.channels) {
        ComponentErrorHandler::IOErrors::logFileWriteError(
            filename, dataToSave.size() * sizeof(float), "Failed to write all frames to WAV file");
        return false;
    }

    return true;
}

}  // namespace huntmaster
