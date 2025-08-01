/**
 * @file TestAudioBuffer.h
 * @brief Concrete AudioBuffer implementation for testing
 */

#pragma once

#include <cstddef>
#include <vector>

// Include the AudioBuffer interface
#include "huntmaster/core/AudioBuffer.h"

namespace huntmaster {
namespace test {

/**
 * @brief Simple concrete AudioBuffer implementation for testing WaveformAnalyzer
 */
class TestAudioBuffer : public huntmaster::core::AudioBuffer {
  public:
    TestAudioBuffer(size_t channels, size_t frame_count, float sample_rate)
        : channels_(channels), frame_count_(frame_count), sample_rate_(sample_rate) {
        data_.resize(channels * frame_count, 0.0f);
    }

    // AudioBuffer interface methods
    bool isEmpty() const override {
        return frame_count_ == 0;
    }

    size_t getFrameCount() const override {
        return frame_count_;
    }

    size_t getChannelCount() const override {
        return channels_;
    }

    float getSample(size_t channel, size_t frame) const override {
        if (channel >= channels_ || frame >= frame_count_) {
            return 0.0f;
        }
        return data_[frame * channels_ + channel];
    }

    // Test helper methods
    void setSample(size_t channel, size_t frame, float value) {
        if (channel < channels_ && frame < frame_count_) {
            data_[frame * channels_ + channel] = value;
        }
    }

    float getSampleRate() const {
        return sample_rate_;
    }

  private:
    size_t channels_;
    size_t frame_count_;
    float sample_rate_;
    std::vector<float> data_;
};

}  // namespace test
}  // namespace huntmaster
