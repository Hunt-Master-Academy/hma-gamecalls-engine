#include "../include/huntmaster_engine/AudioRecorder.h"
#include "../libs/miniaudio.h"
#include <mutex>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include "../libs/dr_wav.h"

namespace huntmaster
{

    class AudioRecorder::Impl
    {
    public:
        // Configuration for Voice Activity Detection (VAD) and trimming
        struct TrimConfig
        {
            float silenceThreshold = 0.01f;        // Amplitude threshold for silence detection
            float energyThreshold = 0.0001f;       // Energy threshold for silence detection
            float windowDurationMs = 10.0f;        // Window size for VAD in milliseconds
            float requiredSoundDurationMs = 20.0f; // Min consecutive sound duration to mark start/end
            float hangoverDurationMs = 100.0f;     // Extra duration to keep after sound ends
            float fadeDurationMs = 5.0f;           // Duration for fade in/out to prevent clicks
        };

        TrimConfig trimConfig;

        Config config;
        ma_device device;
        ma_device_config deviceConfig;
        std::vector<float> recordedData;
        std::atomic<bool> recording{false};
        std::atomic<float> currentLevel{0.0f};
        mutable std::mutex dataMutex;

        static void dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
        {
            auto *impl = static_cast<Impl *>(pDevice->pUserData);
            if (!impl || !impl->recording)
                return;

            const float *input = static_cast<const float *>(pInput);

            // Calculate level
            float maxLevel = 0.0f;
            for (ma_uint32 i = 0; i < frameCount * impl->config.channels; ++i)
            {
                float absValue = std::abs(input[i]);
                if (absValue > maxLevel)
                    maxLevel = absValue;
            }
            impl->currentLevel = maxLevel;

            // Store data
            {
                std::lock_guard<std::mutex> lock(impl->dataMutex);
                impl->recordedData.insert(impl->recordedData.end(),
                                          input,
                                          input + frameCount * impl->config.channels);
            }
        }
    };

    AudioRecorder::AudioRecorder() : pImpl(std::make_unique<Impl>()) {}
    AudioRecorder::~AudioRecorder()
    {
        stopRecording();
    }

    bool AudioRecorder::startRecording(const Config &config)
    {
        if (pImpl->recording)
            return false;

        pImpl->config = config;
        pImpl->recordedData.clear();
        pImpl->currentLevel = 0.0f;

        pImpl->deviceConfig = ma_device_config_init(ma_device_type_capture);
        pImpl->deviceConfig.capture.format = ma_format_f32;
        pImpl->deviceConfig.capture.channels = config.channels;
        pImpl->deviceConfig.sampleRate = config.sampleRate;
        pImpl->deviceConfig.dataCallback = Impl::dataCallback;
        pImpl->deviceConfig.pUserData = pImpl.get();

        if (ma_device_init(NULL, &pImpl->deviceConfig, &pImpl->device) != MA_SUCCESS)
        {
            std::cerr << "Failed to initialize capture device." << std::endl;
            return false;
        }

        if (ma_device_start(&pImpl->device) != MA_SUCCESS)
        {
            std::cerr << "Failed to start capture device." << std::endl;
            ma_device_uninit(&pImpl->device);
            return false;
        }

        pImpl->recording = true;
        std::cout << "Recording started at " << config.sampleRate << " Hz" << std::endl;
        return true;
    }

    void AudioRecorder::stopRecording()
    {
        if (!pImpl->recording)
            return;

        pImpl->recording = false;
        ma_device_uninit(&pImpl->device);
        std::cout << "Recording stopped. Captured " << getDuration() << " seconds." << std::endl;
    }

    bool AudioRecorder::isRecording() const
    {
        return pImpl->recording;
    }

    std::vector<float> AudioRecorder::getRecordedData() const
    {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        return pImpl->recordedData;
    }
    bool AudioRecorder::saveToWavTrimmed(const std::string &filename) const
    {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);

        if (pImpl->recordedData.empty())
        {
            std::cerr << "No audio data to save!" << std::endl;
            return false;
        }

        // Voice Activity Detection parameters
        const float silenceThreshold = pImpl->trimConfig.silenceThreshold;
        const float energyThreshold = pImpl->trimConfig.energyThreshold;
        const int windowSize = static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.windowDurationMs / 1000.0f);

        // Find audio boundaries
        size_t audioStart = 0;
        size_t audioEnd = pImpl->recordedData.size();

        // Find start
        int consecutiveSoundFrames = 0;
        int requiredFrames = static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.requiredSoundDurationMs / 1000.0f);

        for (size_t i = 0; i < pImpl->recordedData.size(); i += windowSize / 2)
        {
            float energy = 0.0f;
            float peak = 0.0f;
            int count = 0;

            for (size_t j = i; j < std::min(i + static_cast<size_t>(windowSize), pImpl->recordedData.size()); ++j)
            {
                energy += pImpl->recordedData[j] * pImpl->recordedData[j];
                peak = std::max(peak, std::abs(pImpl->recordedData[j]));
                count++;
            }

            if (count > 0)
            {
                energy /= count;
            }

            if (energy > energyThreshold || peak > silenceThreshold)
            {
                consecutiveSoundFrames += windowSize / 2;
                if (consecutiveSoundFrames >= requiredFrames)
                {
                    audioStart = std::max(0, static_cast<int>(i) - windowSize);
                    break;
                }
            }
            else
            {
                consecutiveSoundFrames = 0;
            }
        }

        // Find end (search backwards)
        int hangoverSamples = static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.hangoverDurationMs / 1000.0f);

        for (int i = static_cast<int>(pImpl->recordedData.size()) - windowSize; i >= 0; i -= windowSize / 2)
        {
            float energy = 0.0f;
            float peak = 0.0f;
            int count = 0;

            for (int j = i; j < std::min(i + windowSize, static_cast<int>(pImpl->recordedData.size())); ++j)
            {
                energy += pImpl->recordedData[j] * pImpl->recordedData[j];
                peak = std::max(peak, std::abs(pImpl->recordedData[j]));
                count++;
            }

            if (count > 0)
            {
                energy /= count;
            }

            if (energy > energyThreshold || peak > silenceThreshold)
            {
                audioEnd = std::min(pImpl->recordedData.size(),
                                    static_cast<size_t>(i + windowSize + hangoverSamples));
                break;
            }
            else
            {
                // If we encounter silence, reset consecutive sound frames for the end detection
                // This ensures we find the *last* significant audio segment
                // (This part of the logic was missing in the original backward loop)
                // For end detection, we don't need consecutive frames, just the first significant one from the end
            }
        }

        // Ensure valid range
        if (audioStart >= audioEnd)
        {
            std::cerr << "Warning: No significant audio detected, saving full recording" << std::endl;
            audioStart = 0;
            audioEnd = pImpl->recordedData.size();
        }

        // Create trimmed data
        std::vector<float> trimmedData(pImpl->recordedData.begin() + audioStart,
                                       pImpl->recordedData.begin() + audioEnd);

        // Apply fade in/out to avoid clicks
        int fadeLength = static_cast<int>(pImpl->config.sampleRate * pImpl->trimConfig.fadeDurationMs / 1000.0f);

        // Ensure fadeLength does not exceed half of trimmedData.size() to prevent overlap
        fadeLength = std::min(fadeLength, static_cast<int>(trimmedData.size() / 2));

        for (int i = 0; i < fadeLength; ++i)
        {
            float factor = static_cast<float>(i) / fadeLength;
            trimmedData[i] *= factor;
        }
        for (int i = 0; i < fadeLength; ++i)
        {
            float factor = static_cast<float>(i) / fadeLength;
            trimmedData[trimmedData.size() - 1 - i] *= (1.0f - factor); // Use (1.0f - factor) for fade out
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
        struct DrWavGuard
        {
            drwav *wav;
            DrWavGuard(drwav *w) : wav(w) {}
            ~DrWavGuard()
            {
                if (wav)
                {
                    drwav_uninit(wav);
                }
            }
            DrWavGuard(const DrWavGuard &) = delete;
            DrWavGuard &operator=(const DrWavGuard &) = delete;
        };

        if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL))
        {
            std::cerr << "Failed to create WAV file: " << filename << std::endl;
            return false;
        }
        DrWavGuard wavGuard(&wav);

        drwav_uint64 expectedFrames = trimmedData.size() / pImpl->config.channels;
        drwav_uint64 samplesWritten = drwav_write_pcm_frames(&wav,
                                                             expectedFrames,
                                                             trimmedData.data());

        float originalDuration = static_cast<float>(pImpl->recordedData.size()) /
                                 (pImpl->config.sampleRate * pImpl->config.channels);
        float trimmedDuration = static_cast<float>(trimmedData.size()) /
                                (pImpl->config.sampleRate * pImpl->config.channels);

        std::cout << "Saved trimmed audio to " << filename << std::endl;
        std::cout << "  Original: " << std::fixed << std::setprecision(2) << originalDuration << "s" << std::endl;
        std::cout << "  Trimmed:  " << std::fixed << std::setprecision(2) << trimmedDuration << "s" << std::endl;
        std::cout << "  Removed:  " << std::fixed << std::setprecision(2)
                  << (originalDuration - trimmedDuration) << "s of silence" << std::endl;

        return samplesWritten == expectedFrames;
    }
    bool AudioRecorder::saveToWav(const std::string &filename) const
    {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);

        drwav wav;
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        format.channels = pImpl->config.channels;
        format.sampleRate = pImpl->config.sampleRate;
        format.bitsPerSample = 32;

        // RAII for drwav to ensure uninit is called
        struct DrWavGuard
        {
            drwav *wav;
            DrWavGuard(drwav *w) : wav(w) {}
            ~DrWavGuard()
            {
                if (wav)
                {
                    drwav_uninit(wav);
                }
            }
            DrWavGuard(const DrWavGuard &) = delete;
            DrWavGuard &operator=(const DrWavGuard &) = delete;
        };

        if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL))
        {
            std::cerr << "Failed to create WAV file: " << filename << std::endl;
            return false;
        }
        DrWavGuard wavGuard(&wav);

        drwav_uint64 expectedFrames = pImpl->recordedData.size() / pImpl->config.channels;
        drwav_uint64 samplesWritten = drwav_write_pcm_frames(&wav, expectedFrames, pImpl->recordedData.data());

        std::cout << "Saved " << samplesWritten << " frames to " << filename << std::endl;
        return samplesWritten == expectedFrames;
    }

    float AudioRecorder::getCurrentLevel() const
    {
        return pImpl->currentLevel;
    }

    double AudioRecorder::getDuration() const
    {
        std::lock_guard<std::mutex> lock(pImpl->dataMutex);
        if (pImpl->config.sampleRate == 0 || pImpl->config.channels == 0)
            return 0.0;
        return static_cast<double>(pImpl->recordedData.size()) /
               (pImpl->config.sampleRate * pImpl->config.channels);
    }

} // namespace huntmaster
