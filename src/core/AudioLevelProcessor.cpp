#include "huntmaster/core/AudioLevelProcessor.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "huntmaster/core/DebugLogger.h"

// Enable debug output for AudioLevelProcessor
#define DEBUG_AUDIO_LEVEL_PROCESSOR 0

namespace huntmaster {

/// Implementation details for AudioLevelProcessor
class AudioLevelProcessor::Impl {
   public:
    Config config_;
    mutable std::mutex mutex_;

    // Smoothing filter state (atomic for lock-free reads)
    std::atomic<float> currentRmsLinear_{0.0f};
    std::atomic<float> currentPeakLinear_{0.0f};
    std::atomic<float> currentRmsDb_{-60.0f};
    std::atomic<float> currentPeakDb_{-60.0f};

    // Smoothing coefficients (recalculated when config changes)
    float rmsAttackCoeff_ = 0.0f;
    float rmsReleaseCoeff_ = 0.0f;
    float peakAttackCoeff_ = 0.0f;
    float peakReleaseCoeff_ = 0.0f;

    // Level history (protected by mutex)
    std::deque<LevelMeasurement> levelHistory_;

    // Processing state
    std::atomic<bool> initialized_{false};
    std::chrono::steady_clock::time_point lastUpdateTime_;

    explicit Impl(const Config& config) : config_(config) {
        if (config_.isValid()) {
            calculateSmoothingCoefficients();
            lastUpdateTime_ = std::chrono::steady_clock::now();
            initialized_.store(true);
        }
    }

    void calculateSmoothingCoefficients() {
        // Calculate smoothing coefficients for exponential smoothing
        // coeff = 1 - exp(-1 / (timeConstant * sampleRate / 1000))

        const float sampleRateMs = config_.sampleRate / 1000.0f;

        rmsAttackCoeff_ = 1.0f - std::exp(-1.0f / (config_.rmsAttackTimeMs * sampleRateMs));
        rmsReleaseCoeff_ = 1.0f - std::exp(-1.0f / (config_.rmsReleaseTimeMs * sampleRateMs));
        peakAttackCoeff_ = 1.0f - std::exp(-1.0f / (config_.peakAttackTimeMs * sampleRateMs));
        peakReleaseCoeff_ = 1.0f - std::exp(-1.0f / (config_.peakReleaseTimeMs * sampleRateMs));

        // Clamp coefficients to valid range
        rmsAttackCoeff_ = std::clamp(rmsAttackCoeff_, 0.001f, 1.0f);
        rmsReleaseCoeff_ = std::clamp(rmsReleaseCoeff_, 0.001f, 1.0f);
        peakAttackCoeff_ = std::clamp(peakAttackCoeff_, 0.001f, 1.0f);
        peakReleaseCoeff_ = std::clamp(peakReleaseCoeff_, 0.001f, 1.0f);
    }
};

AudioLevelProcessor::AudioLevelProcessor() : AudioLevelProcessor(Config{}) {}

AudioLevelProcessor::AudioLevelProcessor(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

AudioLevelProcessor::~AudioLevelProcessor() = default;

AudioLevelProcessor::Result AudioLevelProcessor::processAudio(std::span<const float> samples,
                                                              int numChannels) noexcept {
    // AUDIO_LOG_DEBUG("processAudio called with " + std::to_string(samples.size()) + " samples, " +
    //                 std::to_string(numChannels) + " channels");

    if (!impl_->initialized_.load()) {
        // AUDIO_LOG_ERROR("processAudio: processor not initialized");
        return huntmaster::unexpected(Error::INITIALIZATION_FAILED);
    }

    if (samples.empty()) {
        // AUDIO_LOG_ERROR("processAudio: empty samples provided");
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    if (numChannels <= 0 || numChannels > 8) {
        // AUDIO_LOG_ERROR("processAudio: invalid channel count: " + std::to_string(numChannels));
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    try {
        // Calculate RMS and peak values for this audio chunk
        float sumSquares = 0.0f;
        float peakSample = 0.0f;

        // Process samples (average across channels if multi-channel)
        const size_t numSamples = samples.size();
        const size_t framesCount = numSamples / numChannels;

        for (size_t frame = 0; frame < framesCount; ++frame) {
            float framePeak = 0.0f;
            float frameSum = 0.0f;

            // Process all channels in this frame
            for (int ch = 0; ch < numChannels; ++ch) {
                const size_t index = frame * numChannels + ch;
                if (index < numSamples) {
                    const float sample = samples[index];
                    framePeak = std::max(framePeak, std::abs(sample));
                    frameSum += sample;
                }
            }

            // Average amplitude across channels for this frame
            const float avgAmplitude = frameSum / numChannels;
            sumSquares += avgAmplitude * avgAmplitude;
            peakSample = std::max(peakSample, framePeak);
        }

        // Calculate RMS from sum of squares
        const float rmsLinear = (framesCount > 0) ? std::sqrt(sumSquares / framesCount) : 0.0f;

        // Apply smoothing filters
        const float currentRms = impl_->currentRmsLinear_.load();
        const float currentPeak = impl_->currentPeakLinear_.load();

        // Choose attack or release coefficient based on signal direction
        const float rmsCoeff =
            (rmsLinear > currentRms) ? impl_->rmsAttackCoeff_ : impl_->rmsReleaseCoeff_;
        const float peakCoeff =
            (peakSample > currentPeak) ? impl_->peakAttackCoeff_ : impl_->peakReleaseCoeff_;

        // Apply exponential smoothing
        const float smoothedRms = currentRms + rmsCoeff * (rmsLinear - currentRms);
        const float smoothedPeak = currentPeak + peakCoeff * (peakSample - currentPeak);

        // Convert to dB
        const float rmsDb =
            linearToDb(smoothedRms, impl_->config_.dbFloor, impl_->config_.dbCeiling);
        const float peakDb =
            linearToDb(smoothedPeak, impl_->config_.dbFloor, impl_->config_.dbCeiling);

        // Update atomic values
        impl_->currentRmsLinear_.store(smoothedRms);
        impl_->currentPeakLinear_.store(smoothedPeak);
        impl_->currentRmsDb_.store(rmsDb);
        impl_->currentPeakDb_.store(peakDb);

        // Create measurement result
        LevelMeasurement measurement;
        measurement.rmsLinear = smoothedRms;
        measurement.rmsDb = rmsDb;
        measurement.peakLinear = smoothedPeak;
        measurement.peakDb = peakDb;
        measurement.timestamp = std::chrono::steady_clock::now();

        // Update history (thread-safe)
        {
            std::lock_guard<std::mutex> lock(impl_->mutex_);
            impl_->levelHistory_.push_front(measurement);

            // Trim history to configured size
            while (impl_->levelHistory_.size() > impl_->config_.historySize) {
                impl_->levelHistory_.pop_back();
            }
        }

        impl_->lastUpdateTime_ = measurement.timestamp;
        return measurement;

    } catch (...) {
        return huntmaster::unexpected(Error::INTERNAL_ERROR);
    }
}

AudioLevelProcessor::LevelMeasurement AudioLevelProcessor::getCurrentLevel() const noexcept {
    LevelMeasurement current;
    current.rmsLinear = impl_->currentRmsLinear_.load();
    current.peakLinear = impl_->currentPeakLinear_.load();
    current.rmsDb = impl_->currentRmsDb_.load();
    current.peakDb = impl_->currentPeakDb_.load();
    current.timestamp = impl_->lastUpdateTime_;
    return current;
}

std::vector<AudioLevelProcessor::LevelMeasurement> AudioLevelProcessor::getLevelHistory(
    size_t maxCount) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    const size_t count = (maxCount > 0) ? std::min(maxCount, impl_->levelHistory_.size())
                                        : impl_->levelHistory_.size();

    std::vector<LevelMeasurement> result;
    result.reserve(count);

    auto it = impl_->levelHistory_.begin();
    for (size_t i = 0; i < count && it != impl_->levelHistory_.end(); ++i, ++it) {
        result.push_back(*it);
    }

    return result;
}

std::string AudioLevelProcessor::exportToJson() const {
    const auto current = getCurrentLevel();

    // Convert timestamp to milliseconds since epoch
    const auto epoch = current.timestamp.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "{"
        << "\"rms\":" << current.rmsDb << ","
        << "\"peak\":" << current.peakDb << ","
        << "\"rmsLinear\":" << current.rmsLinear << ","
        << "\"peakLinear\":" << current.peakLinear << ","
        << "\"timestamp\":" << millis << "}";

    return oss.str();
}

std::string AudioLevelProcessor::exportHistoryToJson(size_t maxCount) const {
    const auto history = getLevelHistory(maxCount);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "[";

    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) oss << ",";

        const auto& measurement = history[i];
        const auto epoch = measurement.timestamp.time_since_epoch();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

        oss << "{"
            << "\"rms\":" << measurement.rmsDb << ","
            << "\"peak\":" << measurement.peakDb << ","
            << "\"rmsLinear\":" << measurement.rmsLinear << ","
            << "\"peakLinear\":" << measurement.peakLinear << ","
            << "\"timestamp\":" << millis << "}";
    }

    oss << "]";
    return oss.str();
}

void AudioLevelProcessor::reset() noexcept {
    // AUDIO_LOG_DEBUG("reset called - clearing all audio level data");
    impl_->currentRmsLinear_.store(0.0f);
    impl_->currentPeakLinear_.store(0.0f);
    impl_->currentRmsDb_.store(impl_->config_.dbFloor);
    impl_->currentPeakDb_.store(impl_->config_.dbFloor);
    // AUDIO_LOG_DEBUG("reset - atomic values reset");

    {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        impl_->levelHistory_.clear();
    }

    impl_->lastUpdateTime_ = std::chrono::steady_clock::now();
}

bool AudioLevelProcessor::updateConfig(const Config& newConfig) noexcept {
    if (!newConfig.isValid()) {
        return false;
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        impl_->config_ = newConfig;
        impl_->calculateSmoothingCoefficients();

        // Resize history if needed
        while (impl_->levelHistory_.size() > newConfig.historySize) {
            impl_->levelHistory_.pop_back();
        }

        return true;
    } catch (...) {
        return false;
    }
}

AudioLevelProcessor::Config AudioLevelProcessor::getConfig() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->config_;
}

bool AudioLevelProcessor::isInitialized() const noexcept { return impl_->initialized_.load(); }

// Utility functions
float linearToDb(float linear, float floor, float ceiling) noexcept {
    if (linear <= 0.0f) {
        return floor;
    }

    const float db = 20.0f * std::log10(linear);
    return std::clamp(db, floor, ceiling);
}

float dbToLinear(float db) noexcept { return std::pow(10.0f, db / 20.0f); }

}  // namespace huntmaster
