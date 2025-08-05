#pragma once

#include <memory>
#include <span>

#include "huntmaster/core/AudioBuffer.h"
#include "huntmaster/core/Expected.h"

namespace huntmaster {

/**
 * @brief Error types for audio processing components
 */
enum class AudioProcessorError {
    INITIALIZATION_FAILED,
    INVALID_AUDIO_DATA,
    PROCESSING_ERROR,
    INSUFFICIENT_DATA,
    CONFIGURATION_ERROR
};

/**
 * @brief Base class for audio processing components
 *
 * Provides a common interface for all audio processing components
 * in the Huntmaster Engine, supporting the enhanced platform roadmap
 * with advanced analyzers.
 */
class AudioProcessor {
  public:
    virtual ~AudioProcessor() = default;

    /**
     * @brief Initialize the audio processor
     * @param sampleRate Sample rate in Hz
     * @param channels Number of channels
     * @return Expected indicating success or failure
     */
    virtual expected<void, AudioProcessorError> initialize(int sampleRate, int channels) = 0;

    /**
     * @brief Process a chunk of audio data
     * @param audioData Input audio samples
     * @return Expected indicating success or failure
     */
    virtual expected<void, AudioProcessorError> processAudio(std::span<const float> audioData) = 0;

    /**
     * @brief Reset the processor state
     */
    virtual void reset() = 0;

    /**
     * @brief Check if the processor is initialized
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Get the current sample rate
     */
    virtual int getSampleRate() const = 0;

    /**
     * @brief Get the number of channels
     */
    virtual int getChannels() const = 0;

  protected:
    int sampleRate_ = 0;
    int channels_ = 0;
    bool initialized_ = false;
};

}  // namespace huntmaster
