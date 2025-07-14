#include "huntmaster/core/WaveformGenerator.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace huntmaster {

/// Implementation details for WaveformGenerator
class WaveformGenerator::Impl {
   public:
    Config config_;
    mutable std::mutex mutex_;

    // Circular buffer for continuous waveform data
    std::deque<float> sampleBuffer_;
    std::deque<float> peakBuffer_;
    std::deque<float> rmsBuffer_;

    // Processing state
    std::atomic<bool> initialized_{false};
    std::chrono::steady_clock::time_point lastUpdateTime_;

    // Statistics
    std::atomic<float> currentMaxAmplitude_{0.0f};
    std::atomic<float> currentRmsAmplitude_{0.0f};
    std::atomic<size_t> totalSamplesProcessed_{0};

    // Downsampling state
    std::vector<float> downsampleAccumulator_;
    size_t downsampleCount_ = 0;
    size_t currentDownsampleRatio_;

    // RMS calculation state
    std::vector<float> rmsWindow_;
    size_t rmsWindowSamples_;
    size_t rmsIndex_ = 0;
    float rmsSum_ = 0.0f;

    explicit Impl(const Config& config) : config_(config) {
        if (config_.isValid()) {
            initializeBuffers();
            lastUpdateTime_ = std::chrono::steady_clock::now();
            initialized_.store(true);
        }
    }

    void initializeBuffers() {
        currentDownsampleRatio_ = config_.downsampleRatio;

        // Calculate RMS window size in samples
        rmsWindowSamples_ = static_cast<size_t>(config_.rmsWindowMs * config_.sampleRate / 1000.0f);
        rmsWindowSamples_ = std::max(rmsWindowSamples_, size_t(1));

        // Initialize RMS sliding window
        rmsWindow_.resize(rmsWindowSamples_, 0.0f);
        rmsSum_ = 0.0f;
        rmsIndex_ = 0;

        // Reserve buffer capacity
        const size_t bufferCapacity = config_.maxSamples / currentDownsampleRatio_;
        sampleBuffer_.clear();
        if (config_.enablePeakHold) {
            peakBuffer_.clear();
        }
        if (config_.enableRmsOverlay) {
            rmsBuffer_.clear();
        }
    }
};

WaveformGenerator::WaveformGenerator(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

WaveformGenerator::WaveformGenerator() : impl_(std::make_unique<Impl>(Config{})) {}

WaveformGenerator::Result WaveformGenerator::processAudio(std::span<const float> samples,
                                                          int numChannels) noexcept {
    if (!impl_->initialized_.load()) {
        return huntmaster::unexpected(Error::INITIALIZATION_FAILED);
    }

    if (samples.empty()) {
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    if (numChannels <= 0 || numChannels > 8) {
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);

        // Process samples (average across channels if multi-channel)
        const size_t numSamples = samples.size();
        const size_t framesCount = numSamples / numChannels;

        float maxAmplitudeThisChunk = 0.0f;
        float rmsSum = 0.0f;

        for (size_t frame = 0; frame < framesCount; ++frame) {
            float frameSum = 0.0f;
            float frameMaxAbs = 0.0f;

            // Process all channels in this frame
            for (int ch = 0; ch < numChannels; ++ch) {
                const size_t index = frame * numChannels + ch;
                if (index < numSamples) {
                    const float sample = samples[index];
                    frameSum += sample;
                    frameMaxAbs = std::max(frameMaxAbs, std::abs(sample));
                }
            }

            // Average amplitude across channels for this frame
            const float avgSample = frameSum / numChannels;
            maxAmplitudeThisChunk = std::max(maxAmplitudeThisChunk, frameMaxAbs);

            // Update RMS sliding window
            const float oldSample = impl_->rmsWindow_[impl_->rmsIndex_];
            impl_->rmsWindow_[impl_->rmsIndex_] = avgSample;
            impl_->rmsSum_ = impl_->rmsSum_ - oldSample * oldSample + avgSample * avgSample;
            impl_->rmsIndex_ = (impl_->rmsIndex_ + 1) % impl_->rmsWindowSamples_;

            // Calculate current RMS
            const float currentRms =
                std::sqrt(std::max(0.0f, impl_->rmsSum_ / impl_->rmsWindowSamples_));

            // Downsample processing
            impl_->downsampleAccumulator_.push_back(avgSample);
            impl_->downsampleCount_++;

            if (impl_->downsampleCount_ >= impl_->currentDownsampleRatio_) {
                // Calculate downsampled values
                float downsampledSample = 0.0f;
                float downsampledPeak = 0.0f;

                for (float sample : impl_->downsampleAccumulator_) {
                    downsampledSample += sample;
                    downsampledPeak = std::max(downsampledPeak, std::abs(sample));
                }
                downsampledSample /= impl_->downsampleAccumulator_.size();

                // Add to circular buffers
                impl_->sampleBuffer_.push_back(downsampledSample);

                if (impl_->config_.enablePeakHold) {
                    impl_->peakBuffer_.push_back(downsampledPeak);
                }

                if (impl_->config_.enableRmsOverlay) {
                    impl_->rmsBuffer_.push_back(currentRms);
                }

                // Maintain buffer size limits
                const size_t maxBufferSize =
                    impl_->config_.maxSamples / impl_->currentDownsampleRatio_;

                while (impl_->sampleBuffer_.size() > maxBufferSize) {
                    impl_->sampleBuffer_.pop_front();
                }

                if (impl_->config_.enablePeakHold) {
                    while (impl_->peakBuffer_.size() > maxBufferSize) {
                        impl_->peakBuffer_.pop_front();
                    }
                }

                if (impl_->config_.enableRmsOverlay) {
                    while (impl_->rmsBuffer_.size() > maxBufferSize) {
                        impl_->rmsBuffer_.pop_front();
                    }
                }

                // Reset accumulator
                impl_->downsampleAccumulator_.clear();
                impl_->downsampleCount_ = 0;
            }

            rmsSum += avgSample * avgSample;
        }

        // Update statistics
        impl_->currentMaxAmplitude_.store(
            std::max(impl_->currentMaxAmplitude_.load(), maxAmplitudeThisChunk));

        const float chunkRms = (framesCount > 0) ? std::sqrt(rmsSum / framesCount) : 0.0f;
        impl_->currentRmsAmplitude_.store(chunkRms);

        impl_->totalSamplesProcessed_.fetch_add(framesCount);

        // Create result
        WaveformData result;
        result.samples.assign(impl_->sampleBuffer_.begin(), impl_->sampleBuffer_.end());

        if (impl_->config_.enablePeakHold && !impl_->peakBuffer_.empty()) {
            result.peaks.assign(impl_->peakBuffer_.begin(), impl_->peakBuffer_.end());
        }

        if (impl_->config_.enableRmsOverlay && !impl_->rmsBuffer_.empty()) {
            result.rmsEnvelope.assign(impl_->rmsBuffer_.begin(), impl_->rmsBuffer_.end());
        }

        result.maxAmplitude = maxAmplitudeThisChunk;
        result.rmsAmplitude = chunkRms;
        result.originalSampleCount = framesCount;
        result.timestamp = std::chrono::steady_clock::now();

        impl_->lastUpdateTime_ = result.timestamp;

        // Normalize output if requested
        if (impl_->config_.normalizeOutput && result.maxAmplitude > 0.0f) {
            const float normalizationFactor = 1.0f / result.maxAmplitude;

            for (float& sample : result.samples) {
                sample *= normalizationFactor;
            }

            for (float& peak : result.peaks) {
                peak *= normalizationFactor;
            }

            // RMS envelope doesn't need normalization as it's already relative
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

    result.maxAmplitude = impl_->currentMaxAmplitude_.load();
    result.rmsAmplitude = impl_->currentRmsAmplitude_.load();
    result.originalSampleCount = impl_->totalSamplesProcessed_.load();
    result.timestamp = impl_->lastUpdateTime_;

    return result;
}

WaveformGenerator::WaveformData WaveformGenerator::getWaveformRange(float startTimeMs,
                                                                    float durationMs) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    const float samplePeriodMs =
        1000.0f * impl_->currentDownsampleRatio_ / impl_->config_.sampleRate;
    const size_t startIndex = static_cast<size_t>(startTimeMs / samplePeriodMs);
    const size_t sampleCount = static_cast<size_t>(durationMs / samplePeriodMs);

    WaveformData result;

    // Extract range from buffers
    if (startIndex < impl_->sampleBuffer_.size()) {
        const size_t endIndex = std::min(startIndex + sampleCount, impl_->sampleBuffer_.size());

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

    result.maxAmplitude = impl_->currentMaxAmplitude_.load();
    result.rmsAmplitude = impl_->currentRmsAmplitude_.load();
    result.originalSampleCount = sampleCount * impl_->currentDownsampleRatio_;
    result.timestamp = impl_->lastUpdateTime_;

    return result;
}

std::string WaveformGenerator::exportToJson(bool includeRawSamples) const {
    const auto waveformData = getCompleteWaveform();

    const auto epoch = waveformData.timestamp.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{"
        << "\"maxAmplitude\":" << waveformData.maxAmplitude << ","
        << "\"rmsAmplitude\":" << waveformData.rmsAmplitude << ","
        << "\"sampleCount\":" << waveformData.originalSampleCount << ","
        << "\"sampleRate\":" << impl_->config_.sampleRate << ","
        << "\"downsampleRatio\":" << impl_->currentDownsampleRatio_ << ","
        << "\"timestamp\":" << millis;

    if (includeRawSamples && !waveformData.samples.empty()) {
        oss << ",\"samples\":[";
        for (size_t i = 0; i < waveformData.samples.size(); ++i) {
            if (i > 0) oss << ",";
            oss << waveformData.samples[i];
        }
        oss << "]";
    }

    if (!waveformData.peaks.empty()) {
        oss << ",\"peaks\":[";
        for (size_t i = 0; i < waveformData.peaks.size(); ++i) {
            if (i > 0) oss << ",";
            oss << waveformData.peaks[i];
        }
        oss << "]";
    }

    if (!waveformData.rmsEnvelope.empty()) {
        oss << ",\"rms\":[";
        for (size_t i = 0; i < waveformData.rmsEnvelope.size(); ++i) {
            if (i > 0) oss << ",";
            oss << waveformData.rmsEnvelope[i];
        }
        oss << "]";
    }

    oss << "}";
    return oss.str();
}

std::string WaveformGenerator::exportForDisplay(size_t displayWidthPixels,
                                                bool includeEnvelopes) const {
    // Calculate optimal downsampling for display
    const auto waveformData = getCompleteWaveform();

    if (waveformData.samples.empty()) {
        return "{\"samples\":[],\"displayWidth\":" + std::to_string(displayWidthPixels) + "}";
    }

    // Downsample for display if needed
    std::vector<float> displaySamples;
    std::vector<float> displayPeaks;
    std::vector<float> displayRms;

    const size_t samplesPerPixel =
        std::max(size_t(1), waveformData.samples.size() / displayWidthPixels);

    if (samplesPerPixel == 1) {
        // No additional downsampling needed
        displaySamples = waveformData.samples;
        if (includeEnvelopes) {
            displayPeaks = waveformData.peaks;
            displayRms = waveformData.rmsEnvelope;
        }
    } else {
        // Additional downsampling for display
        displaySamples.reserve(displayWidthPixels);
        if (includeEnvelopes) {
            displayPeaks.reserve(displayWidthPixels);
            displayRms.reserve(displayWidthPixels);
        }

        for (size_t pixel = 0; pixel < displayWidthPixels; ++pixel) {
            const size_t startIdx = pixel * samplesPerPixel;
            const size_t endIdx = std::min(startIdx + samplesPerPixel, waveformData.samples.size());

            if (startIdx < waveformData.samples.size()) {
                float sum = 0.0f;
                float peak = 0.0f;
                float rmsSum = 0.0f;
                size_t count = 0;

                for (size_t i = startIdx; i < endIdx; ++i) {
                    sum += waveformData.samples[i];
                    peak = std::max(peak, std::abs(waveformData.samples[i]));

                    if (includeEnvelopes && i < waveformData.rmsEnvelope.size()) {
                        rmsSum += waveformData.rmsEnvelope[i];
                    }
                    count++;
                }

                displaySamples.push_back(count > 0 ? sum / count : 0.0f);

                if (includeEnvelopes) {
                    displayPeaks.push_back(peak);
                    displayRms.push_back(count > 0 ? rmsSum / count : 0.0f);
                }
            }
        }
    }

    // Generate JSON
    const auto epoch = waveformData.timestamp.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{"
        << "\"displayWidth\":" << displayWidthPixels << ","
        << "\"samplesPerPixel\":" << samplesPerPixel << ","
        << "\"maxAmplitude\":" << waveformData.maxAmplitude << ","
        << "\"timestamp\":" << millis << ","
        << "\"samples\":[";

    for (size_t i = 0; i < displaySamples.size(); ++i) {
        if (i > 0) oss << ",";
        oss << displaySamples[i];
    }
    oss << "]";

    if (includeEnvelopes && !displayPeaks.empty()) {
        oss << ",\"peaks\":[";
        for (size_t i = 0; i < displayPeaks.size(); ++i) {
            if (i > 0) oss << ",";
            oss << displayPeaks[i];
        }
        oss << "]";
    }

    if (includeEnvelopes && !displayRms.empty()) {
        oss << ",\"rms\":[";
        for (size_t i = 0; i < displayRms.size(); ++i) {
            if (i > 0) oss << ",";
            oss << displayRms[i];
        }
        oss << "]";
    }

    oss << "}";
    return oss.str();
}

void WaveformGenerator::reset() noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->sampleBuffer_.clear();
    impl_->peakBuffer_.clear();
    impl_->rmsBuffer_.clear();
    impl_->downsampleAccumulator_.clear();
    impl_->downsampleCount_ = 0;

    impl_->currentMaxAmplitude_.store(0.0f);
    impl_->currentRmsAmplitude_.store(0.0f);
    impl_->totalSamplesProcessed_.store(0);

    // Reset RMS window
    std::fill(impl_->rmsWindow_.begin(), impl_->rmsWindow_.end(), 0.0f);
    impl_->rmsSum_ = 0.0f;
    impl_->rmsIndex_ = 0;

    impl_->lastUpdateTime_ = std::chrono::steady_clock::now();
}

bool WaveformGenerator::updateConfig(const Config& newConfig) noexcept {
    if (!newConfig.isValid()) {
        return false;
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        impl_->config_ = newConfig;
        impl_->initializeBuffers();
        return true;
    } catch (...) {
        return false;
    }
}

WaveformGenerator::Config WaveformGenerator::getConfig() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->config_;
}

bool WaveformGenerator::isInitialized() const noexcept { return impl_->initialized_.load(); }

std::pair<size_t, size_t> WaveformGenerator::getBufferStats() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    const size_t used = impl_->sampleBuffer_.size();
    const size_t capacity = impl_->config_.maxSamples / impl_->currentDownsampleRatio_;
    return {used, capacity};
}

void WaveformGenerator::setZoomLevel(float zoomLevel) noexcept {
    if (zoomLevel <= 0.0f) return;

    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Adjust downsampling ratio based on zoom level
    // Higher zoom = more detail = lower downsampling ratio
    const size_t baseRatio = impl_->config_.downsampleRatio;
    const size_t newRatio = std::max(size_t(1), static_cast<size_t>(baseRatio / zoomLevel));

    if (newRatio != impl_->currentDownsampleRatio_) {
        impl_->currentDownsampleRatio_ = newRatio;
        // Clear accumulator when changing ratio
        impl_->downsampleAccumulator_.clear();
        impl_->downsampleCount_ = 0;
    }
}

// Utility functions
size_t calculateOptimalDownsampleRatio(size_t totalSamples, size_t displayWidthPixels,
                                       float sampleRate) noexcept {
    if (displayWidthPixels == 0 || sampleRate <= 0.0f) {
        return 1;
    }

    // Calculate samples per pixel for optimal display
    const size_t samplesPerPixel = std::max(size_t(1), totalSamples / displayWidthPixels);

    // Ensure reasonable bounds (1 to 1024)
    return std::clamp(samplesPerPixel, size_t(1), size_t(1024));
}

std::vector<float> generatePeakEnvelope(std::span<const float> samples,
                                        size_t windowSize) noexcept {
    if (samples.empty() || windowSize == 0) {
        return {};
    }

    std::vector<float> envelope;
    envelope.reserve(samples.size());

    for (size_t i = 0; i < samples.size(); ++i) {
        float peak = 0.0f;

        const size_t startIdx = (i >= windowSize / 2) ? i - windowSize / 2 : 0;
        const size_t endIdx = std::min(i + windowSize / 2 + 1, samples.size());

        for (size_t j = startIdx; j < endIdx; ++j) {
            peak = std::max(peak, std::abs(samples[j]));
        }

        envelope.push_back(peak);
    }

    return envelope;
}

std::vector<float> generateRmsEnvelope(std::span<const float> samples, size_t windowSize) noexcept {
    if (samples.empty() || windowSize == 0) {
        return {};
    }

    std::vector<float> envelope;
    envelope.reserve(samples.size());

    for (size_t i = 0; i < samples.size(); ++i) {
        float sum = 0.0f;
        size_t count = 0;

        const size_t startIdx = (i >= windowSize / 2) ? i - windowSize / 2 : 0;
        const size_t endIdx = std::min(i + windowSize / 2 + 1, samples.size());

        for (size_t j = startIdx; j < endIdx; ++j) {
            const float sample = samples[j];
            sum += sample * sample;
            count++;
        }

        const float rms = (count > 0) ? std::sqrt(sum / count) : 0.0f;
        envelope.push_back(rms);
    }

    return envelope;
}

}  // namespace huntmaster
