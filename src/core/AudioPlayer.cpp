#include "huntmaster/core/AudioPlayer.h"

#include <atomic>
#include <iostream>
#include <vector>

#include "../libs/dr_wav.h"
#include "../libs/miniaudio.h"

namespace huntmaster {

// Using the Pimpl idiom to hide miniaudio implementation details.
class AudioPlayer::Impl {
  public:
    ma_device device;
    ma_decoder decoder;
    bool isDeviceInitialized = false;
    bool isDecoderInitialized = false;  // Track decoder state
    std::atomic<bool> playing{false};

    // This callback is called by miniaudio when it needs more audio data to play.
    static void
    dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        (void)pInput;  // Unused for playback.

        auto* pImpl = static_cast<Impl*>(pDevice->pUserData);
        if (pImpl == nullptr)
            return;

        // Read decoded frames from our decoder into the output buffer.
        ma_uint64 framesRead = 0;
        ma_result result =
            ma_decoder_read_pcm_frames(&pImpl->decoder, pOutput, frameCount, &framesRead);

        // Check if we've successfully read frames
        if (result != MA_SUCCESS || framesRead < frameCount) {
            pImpl->playing = false;  // Signal that playback is finished.
        }
    }

    // Destructor ensures the device and decoder are uninitialized.
    ~Impl() {
        if (isDeviceInitialized) {
            ma_device_uninit(&device);
            isDeviceInitialized = false;
        }
        if (isDecoderInitialized) {
            ma_decoder_uninit(&decoder);
            isDecoderInitialized = false;
        }
    }
};

// --- Public Interface Implementation ---

AudioPlayer::AudioPlayer() : pImpl(std::make_unique<Impl>()) {}
AudioPlayer::~AudioPlayer() = default;

bool AudioPlayer::loadFile(const std::string& filename) {
    if (pImpl->playing.load()) {
        stop();
    }

    // Uninitialize previous decoder if it exists.
    if (pImpl->isDecoderInitialized) {
        ma_decoder_uninit(&pImpl->decoder);
        pImpl->isDecoderInitialized = false;
    }

    if (ma_decoder_init_file(filename.c_str(), NULL, &pImpl->decoder) != MA_SUCCESS) {
        std::cerr << "AudioPlayer Error: Failed to load file " << filename << std::endl;
        return false;
    }

    pImpl->isDecoderInitialized = true;
    return true;
}

bool AudioPlayer::play() {
    if (pImpl->playing.load())
        return true;  // Already playing

    if (!pImpl->isDecoderInitialized) {
        std::cerr << "AudioPlayer Error: No file loaded." << std::endl;
        return false;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = pImpl->decoder.outputFormat;
    deviceConfig.playback.channels = pImpl->decoder.outputChannels;
    deviceConfig.sampleRate = pImpl->decoder.outputSampleRate;
    deviceConfig.dataCallback = Impl::dataCallback;
    deviceConfig.pUserData = pImpl.get();

    if (ma_device_init(NULL, &deviceConfig, &pImpl->device) != MA_SUCCESS) {
        std::cerr << "AudioPlayer Error: Failed to initialize playback device." << std::endl;
        return false;
    }

    if (ma_device_start(&pImpl->device) != MA_SUCCESS) {
        std::cerr << "AudioPlayer Error: Failed to start playback device." << std::endl;
        ma_device_uninit(&pImpl->device);
        return false;
    }

    pImpl->isDeviceInitialized = true;
    pImpl->playing = true;
    return true;
}

void AudioPlayer::stop() {
    if (pImpl->isDeviceInitialized) {
        ma_device_uninit(&pImpl->device);
        pImpl->isDeviceInitialized = false;
    }
    pImpl->playing = false;
}

bool AudioPlayer::isPlaying() const {
    return pImpl->playing.load();
}

double AudioPlayer::getDuration() const {
    ma_uint64 lengthInPCMFrames;
    if (ma_decoder_get_length_in_pcm_frames(&pImpl->decoder, &lengthInPCMFrames) != MA_SUCCESS) {
        return 0.0;
    }
    return static_cast<double>(lengthInPCMFrames) / pImpl->decoder.outputSampleRate;
}

double AudioPlayer::getCurrentPosition() const {
    if (!pImpl->isDecoderInitialized) {
        return 0.0;
    }

    ma_uint64 cursorInPCMFrames;
    if (ma_decoder_get_cursor_in_pcm_frames(&pImpl->decoder, &cursorInPCMFrames) != MA_SUCCESS) {
        return 0.0;
    }
    return static_cast<double>(cursorInPCMFrames) / pImpl->decoder.outputSampleRate;
}

void AudioPlayer::setVolume(float volume) {
    if (pImpl->isDeviceInitialized) {
        ma_device_set_master_volume(&pImpl->device, volume);
    }
}

}  // namespace huntmaster
