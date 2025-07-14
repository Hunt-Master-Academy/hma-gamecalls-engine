#include "huntmaster/core/WaveformGenerator.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace huntmaster {

struct WaveformGenerator::Impl {
    Config config_;
    std::mutex mutex_;

    // Buffers for downsampled data
    std::deque<float> sampleBuffer_;
    std::deque<float> peakBuffer_;
    std::deque<float> rmsBuffer_;

    // State for downsampling
    std::vector<float> downsampleAccumulator_;
    size_t downsampleCount_{0};
    size_t currentDownsampleRatio_;

    // State for RMS calculation
    std::vector<float> rmsWindow_;
    float rmsSum_{0.0f};
    size_t rmsIndex_{0};
    size_t rmsWindowSamples_;

    // Processing state
    WaveformData waveform_;
    std::atomic<bool> initialized_{false};
    std::atomic<float> currentMaxAmplitude_{0.0f};
    std::atomic<float> currentRmsAmplitude_{0.0f};
    std::atomic<size_t> totalSamplesProcessed_{0};

    explicit Impl(const Config& config) : config_(config) {
        initializeBuffers();
        initialized_.store(true);
    }

    void initializeBuffers() {
        currentDownsampleRatio_ = config_.downsampleRatio;
        sampleBuffer_.clear();
        peakBuffer_.clear();
        rmsBuffer_.clear();
        downsampleAccumulator_.clear();
        downsampleCount_ = 0;

        if (config_.enableRmsOverlay) {
            rmsWindowSamples_ =
                static_cast<size_t>(config_.sampleRate * config_.rmsWindowMs / 1000.0f);
            if (rmsWindowSamples_ == 0) rmsWindowSamples_ = 1;
            rmsWindow_.assign(rmsWindowSamples_, 0.0f);
            rmsSum_ = 0.0f;
            rmsIndex_ = 0;
        }
    }

    void processSample(float sample) {
        float absSample = std::abs(sample);

        if (config_.enableRmsOverlay) {
            const float oldSample = rmsWindow_[rmsIndex_];
            rmsWindow_[rmsIndex_] = absSample;
            rmsSum_ = rmsSum_ - oldSample * oldSample + absSample * absSample;
            rmsIndex_ = (rmsIndex_ + 1) % rmsWindowSamples_;
        }

        downsampleAccumulator_.push_back(absSample);
        downsampleCount_++;

        if (downsampleCount_ >= currentDownsampleRatio_) {
            float downsampledSample = 0.0f;
            float downsampledPeak = 0.0f;

            for (float s : downsampleAccumulator_) {
                downsampledSample += s;
                if (s > downsampledPeak) {
                    downsampledPeak = s;
                }
            }
            downsampledSample /= downsampleAccumulator_.size();

            sampleBuffer_.push_back(downsampledSample);
            if (config_.enablePeakHold) {
                peakBuffer_.push_back(downsampledPeak);
            }
            if (config_.enableRmsOverlay) {
                float currentRms = std::sqrt(std::max(0.0f, rmsSum_ / rmsWindowSamples_));
                rmsBuffer_.push_back(currentRms);
            }

            const size_t maxBufferSize = config_.maxSamples / currentDownsampleRatio_;
            while (sampleBuffer_.size() > maxBufferSize) {
                sampleBuffer_.pop_front();
            }
            if (config_.enablePeakHold) {
                while (peakBuffer_.size() > maxBufferSize) {
                    peakBuffer_.pop_front();
                }
            }
            if (config_.enableRmsOverlay) {
                while (rmsBuffer_.size() > maxBufferSize) {
                    rmsBuffer_.pop_front();
                }
            }

            downsampleAccumulator_.clear();
            downsampleCount_ = 0;
        }
    }
};

WaveformGenerator::WaveformGenerator() : impl_(std::make_unique<Impl>(Config{})) {}

WaveformGenerator::WaveformGenerator(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

WaveformGenerator::~WaveformGenerator() = default;

WaveformGenerator::Result WaveformGenerator::processAudio(std::span<const float> samples,
                                                          int numChannels) noexcept {
    if (!impl_->initialized_.load()) {
        return huntmaster::unexpected(Error::INITIALIZATION_FAILED);
    }

    if (samples.empty() || numChannels <= 0) {
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        const size_t frameCount = samples.size() / numChannels;

        for (size_t i = 0; i < frameCount; ++i) {
            float monoSample = 0.0f;
            for (int j = 0; j < numChannels; ++j) {
                monoSample += samples[i * numChannels + j];
            }
            impl_->processSample(monoSample / numChannels);
        }

        impl_->totalSamplesProcessed_.fetch_add(frameCount);

        WaveformData result;
        result.samples.assign(impl_->sampleBuffer_.begin(), impl_->sampleBuffer_.end());
        if (impl_->config_.enablePeakHold && !impl_->peakBuffer_.empty()) {
            result.peaks.assign(impl_->peakBuffer_.begin(), impl_->peakBuffer_.end());
        }
        if (impl_->config_.enableRmsOverlay && !impl_->rmsBuffer_.empty()) {
            result.rmsEnvelope.assign(impl_->rmsBuffer_.begin(), impl_->rmsBuffer_.end());
        }
        return result;

    } catch (...) {
        return huntmaster::unexpected(Error::INTERNAL_ERROR);
    }
}

WaveformGenerator::WaveformData WaveformGenerator::getCompleteWaveform() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    WaveformData result;
    result.samples.assign(impl_->sampleBuffer_.begin(), impl_->sampleBuffer_.end());
    if (impl_->config_.enablePeakHold) {
        result.peaks.assign(impl_->peakBuffer_.begin(), impl_->peakBuffer_.end());
    }
    if (impl_->config_.enableRmsOverlay) {
        result.rmsEnvelope.assign(impl_->rmsBuffer_.begin(), impl_->rmsBuffer_.end());
    }
    return result;
}

WaveformGenerator::WaveformData WaveformGenerator::getWaveformRange(float startTimeMs,
                                                                    float endTimeMs) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    WaveformData result;
    const float msPerSample = 1000.0f * impl_->currentDownsampleRatio_ / impl_->config_.sampleRate;

    size_t startIndex = (msPerSample > 0) ? static_cast<size_t>(startTimeMs / msPerSample) : 0;
    size_t endIndex = (msPerSample > 0) ? static_cast<size_t>(endTimeMs / msPerSample) : 0;

    if (startIndex < impl_->sampleBuffer_.size()) {
        endIndex = std::min(endIndex, impl_->sampleBuffer_.size());
        auto begin = impl_->sampleBuffer_.begin() + startIndex;
        auto end = impl_->sampleBuffer_.begin() + endIndex;
        result.samples.assign(begin, end);

        if (impl_->config_.enablePeakHold && startIndex < impl_->peakBuffer_.size()) {
            auto peakBegin = impl_->peakBuffer_.begin() + startIndex;
            auto peakEnd =
                impl_->peakBuffer_.begin() + std::min(endIndex, impl_->peakBuffer_.size());
            result.peaks.assign(peakBegin, peakEnd);
        }
        if (impl_->config_.enableRmsOverlay && startIndex < impl_->rmsBuffer_.size()) {
            auto rmsBegin = impl_->rmsBuffer_.begin() + startIndex;
            auto rmsEnd = impl_->rmsBuffer_.begin() + std::min(endIndex, impl_->rmsBuffer_.size());
            result.rmsEnvelope.assign(rmsBegin, rmsEnd);
        }
    }
    return result;
}

std::string WaveformGenerator::exportToJson(bool prettyPrint) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    std::stringstream ss;
    auto waveform = getCompleteWaveform();

    auto vectorToJson = [&](const std::vector<float>& vec, const std::string& name) {
        ss << "\"" << name << "\":[";
        for (size_t i = 0; i < vec.size(); ++i) {
            ss << vec[i] << (i == vec.size() - 1 ? "" : ",");
        }
        ss << "]";
    };

    ss << "{";
    vectorToJson(waveform.samples, "samples");
    if (!waveform.peaks.empty()) {
        ss << ",";
        vectorToJson(waveform.peaks, "peaks");
    }
    if (!waveform.rmsEnvelope.empty()) {
        ss << ",";
        vectorToJson(waveform.rmsEnvelope, "rmsEnvelope");
    }
    ss << "}";

    return ss.str();
}

void WaveformGenerator::reset() noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->initializeBuffers();
    impl_->totalSamplesProcessed_.store(0);
    impl_->currentMaxAmplitude_.store(0.0f);
    impl_->currentRmsAmplitude_.store(0.0f);
}

bool WaveformGenerator::updateConfig(const Config& newConfig) noexcept {
    if (!newConfig.isValid()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->config_ = newConfig;
    impl_->initializeBuffers();
    return true;
}

std::pair<size_t, size_t> WaveformGenerator::getBufferStats() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    const size_t used = impl_->sampleBuffer_.size();
    const size_t capacity = impl_->config_.maxSamples / impl_->currentDownsampleRatio_;
    return {used, capacity};
}

void WaveformGenerator::setZoomLevel(float zoomFactor) noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    size_t newRatio =
        static_cast<size_t>(std::max(1.0f, impl_->config_.downsampleRatio / zoomFactor));
    if (newRatio != impl_->currentDownsampleRatio_) {
        impl_->currentDownsampleRatio_ = newRatio;
        impl_->sampleBuffer_.clear();
        impl_->peakBuffer_.clear();
        impl_->rmsBuffer_.clear();
        impl_->downsampleAccumulator_.clear();
        impl_->downsampleCount_ = 0;
    }
}

}  // namespace huntmaster
