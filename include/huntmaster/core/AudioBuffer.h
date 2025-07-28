/**
 * @file AudioBuffer.h
 * @brief Audio Buffer Interface
 */

#pragma once

#include <cstddef>

namespace huntmaster {
namespace core {

/**
 * @brief Simple audio buffer interface for waveform analysis
 */
class AudioBuffer {
  public:
    AudioBuffer() = default;
    virtual ~AudioBuffer() = default;

    // Basic interface methods
    virtual bool isEmpty() const = 0;
    virtual size_t getFrameCount() const = 0;
    virtual size_t getChannelCount() const = 0;
    virtual float getSample(size_t channel, size_t frame) const = 0;
};

}  // namespace core
}  // namespace huntmaster
