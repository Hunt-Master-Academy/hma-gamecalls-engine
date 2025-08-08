#include "huntmaster/core/PitchTracker.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <numeric>

#include <fftw3.h>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/security/memory-guard.h"

namespace huntmaster {

/**
 * @brief Internal implementation of PitchTracker using YIN algorithm
 */
class PitchTrackerImpl : public PitchTracker {
  private:
    Config config_;

    // YIN algorithm buffers
    std::vector<float> yinBuffer_;
    std::vector<float> audioBuffer_;
    std::vector<float> pitchHistory_;
    std::vector<float> confidenceHistory_;

    // Analysis state
    float currentPitch_ = 0.0f;
    float currentConfidence_ = 0.0f;
    bool isInitialized_ = false;
    size_t processedSamples_ = 0;

    // Performance monitoring
    mutable security::MemoryGuard memoryGuard_;

    // Smoothing state
    float smoothedPitch_ = 0.0f;
    bool hasValidPitch_ = false;

  public:
    explicit PitchTrackerImpl(const Config& config)
        : config_(config), memoryGuard_(security::GuardConfig{}) {
        initialize();
    }

    ~PitchTrackerImpl() override = default;

    Result<PitchResult, Error> detectPitch(std::span<const float> audio) override {
        if (audio.empty()) {
            return Result<PitchResult, Error>(unexpected<Error>(Error::INVALID_AUDIO_DATA));
        }

        if (audio.size() < config_.windowSize) {
            return Result<PitchResult, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

        try {
            PitchResult result;

            // Apply YIN algorithm
            auto yinResult = performYinAnalysis(audio);
            if (!yinResult.has_value()) {
                return Result<PitchResult, Error>(unexpected<Error>(yinResult.error()));
            }

            auto pitchConfidence = yinResult.value();
            float pitch = pitchConfidence.first;
            float confidence = pitchConfidence.second;

            result.frequency = pitch;
            result.confidence = confidence;
            result.isVoiced = confidence > config_.threshold;
            result.timestamp = static_cast<float>(processedSamples_) / config_.sampleRate;

            // Apply smoothing if enabled
            if (config_.enableSmoothing && hasValidPitch_) {
                result.frequency = applySmoothingFilter(pitch);
            }

            // Analyze vibrato if enabled
            if (config_.enableVibratoDetection) {
                result.vibrato = analyzeVibrato();
            }

            // Calculate pitch statistics
            result.statistics = calculatePitchStatistics();

            // Update history
            updatePitchHistory(result.frequency, result.confidence);

            // Generate pitch contour
            result.contour = generatePitchContour();

            currentPitch_ = result.frequency;
            currentConfidence_ = result.confidence;
            hasValidPitch_ = result.isVoiced;

            return Result<PitchResult, Error>(std::move(result));

        } catch (const std::exception& e) {
            DebugLogger::getInstance().log(Component::GENERAL,
                                           DebugLevel::ERROR,
                                           "PitchTracker::detectPitch failed: "
                                               + std::string(e.what()));
            return Result<PitchResult, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    Result<float, Error> getRealtimePitch() override {
        if (!isInitialized_) {
            return Result<float, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        return Result<float, Error>(currentPitch_);
    }

    Result<void, Error> processAudioChunk(std::span<const float> audio) override {
        if (audio.empty()) {
            return Result<void, Error>(unexpected<Error>(Error::INVALID_AUDIO_DATA));
        }

        try {
            // Append to audio buffer
            audioBuffer_.insert(audioBuffer_.end(), audio.begin(), audio.end());
            processedSamples_ += audio.size();

            // Process if we have enough samples
            if (audioBuffer_.size() >= config_.windowSize) {
                // Process overlapping windows
                size_t processed = 0;
                while (audioBuffer_.size() - processed >= config_.windowSize) {
                    std::span<const float> window(audioBuffer_.data() + processed,
                                                  config_.windowSize);

                    auto pitchResult = performYinAnalysis(window);
                    if (pitchResult.has_value()) {
                        auto pitchConfidence = pitchResult.value();
                        float pitch = pitchConfidence.first;
                        float confidence = pitchConfidence.second;

                        if (config_.enableSmoothing && hasValidPitch_) {
                            pitch = applySmoothingFilter(pitch);
                        }

                        updatePitchHistory(pitch, confidence);
                        currentPitch_ = pitch;
                        currentConfidence_ = confidence;
                        hasValidPitch_ = confidence > config_.threshold;
                    }

                    processed += config_.hopSize;
                }

                // Keep remaining samples for next chunk
                if (processed > 0) {
                    audioBuffer_.erase(audioBuffer_.begin(), audioBuffer_.begin() + processed);
                }
            }

            return Result<void, Error>();

        } catch (const std::exception& e) {
            DebugLogger::getInstance().log(Component::GENERAL,
                                           DebugLevel::ERROR,
                                           "PitchTracker::processAudioChunk failed: "
                                               + std::string(e.what()));
            return Result<void, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    Result<std::vector<float>, Error> getPitchContour(float durationMs) override {
        if (!isInitialized_) {
            return Result<std::vector<float>, Error>(
                unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        try {
            size_t numSamples = static_cast<size_t>((durationMs / 1000.0f)
                                                    * (config_.sampleRate / config_.hopSize));
            numSamples = std::min(numSamples, pitchHistory_.size());

            std::vector<float> contour;
            if (numSamples > 0) {
                contour.assign(pitchHistory_.end() - numSamples, pitchHistory_.end());
            }

            return Result<std::vector<float>, Error>(std::move(contour));

        } catch (const std::exception& e) {
            DebugLogger::getInstance().log(Component::GENERAL,
                                           DebugLevel::ERROR,
                                           "PitchTracker::getPitchContour failed: "
                                               + std::string(e.what()));
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    void reset() override {
        audioBuffer_.clear();
        pitchHistory_.clear();
        confidenceHistory_.clear();
        currentPitch_ = 0.0f;
        currentConfidence_ = 0.0f;
        smoothedPitch_ = 0.0f;
        hasValidPitch_ = false;
        processedSamples_ = 0;
    }

    Result<void, Error> updateConfig(const Config& config) override {
        if (config.sampleRate <= 0 || config.windowSize == 0) {
            return Result<void, Error>(unexpected<Error>(Error::INVALID_SAMPLE_RATE));
        }

        config_ = config;
        return initialize() ? Result<void, Error>()
                            : Result<void, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
    }

    const Config& getConfig() const override {
        return config_;
    }

    bool isActive() const override {
        return isInitialized_ && hasValidPitch_;
    }

    std::string getProcessingStats() const override {
        return "PitchTracker: Performance profiling disabled in enhanced analyzer development";
    }

  private:
    bool initialize() {
        try {
            // Validate configuration
            if (config_.sampleRate <= 0 || config_.windowSize == 0) {
                return false;
            }

            // Initialize buffers
            yinBuffer_.resize(config_.windowSize / 2);
            audioBuffer_.reserve(config_.windowSize * 2);

            // Reserve history buffers (keep last 10 seconds of data)
            size_t historySize = static_cast<size_t>(10.0f * config_.sampleRate / config_.hopSize);
            pitchHistory_.reserve(historySize);
            confidenceHistory_.reserve(historySize);

            isInitialized_ = true;
            return true;

        } catch (const std::exception& e) {
            DebugLogger::getInstance().log(Component::GENERAL,
                                           DebugLevel::ERROR,
                                           "PitchTracker initialization failed: "
                                               + std::string(e.what()));
            return false;
        }
    }

    Result<std::pair<float, float>, Error> performYinAnalysis(std::span<const float> audio) {
        if (audio.size() < config_.windowSize) {
            return Result<std::pair<float, float>, Error>(
                unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

        try {
            // Step 1: Calculate difference function
            calculateDifferenceFunction(audio);

            // Step 2: Cumulative mean normalized difference function
            calculateCumulativeMeanNormalizedDifference();

            // Step 3: Absolute threshold
            int tau = getAbsoluteThreshold();

            if (tau == -1) {
                // No pitch found
                return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
            }

            // Step 4: Parabolic interpolation
            float betterTau = parabolicInterpolation(tau);

            // Step 5: Convert to frequency
            float frequency = config_.sampleRate / betterTau;

            // Validate frequency range
            if (frequency < config_.minFrequency || frequency > config_.maxFrequency) {
                return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
            }

            // Calculate confidence (inverse of minimum YIN value)
            float confidence = 1.0f - yinBuffer_[tau];
            confidence = std::clamp(confidence, 0.0f, 1.0f);

            return Result<std::pair<float, float>, Error>(std::make_pair(frequency, confidence));

        } catch (const std::exception& e) {
            return Result<std::pair<float, float>, Error>(
                unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    void calculateDifferenceFunction(std::span<const float> audio) {
        size_t W = yinBuffer_.size();

        for (size_t tau = 0; tau < W; ++tau) {
            float sum = 0.0f;
            for (size_t i = 0; i < W; ++i) {
                float delta = audio[i] - audio[i + tau];
                sum += delta * delta;
            }
            yinBuffer_[tau] = sum;
        }
    }

    void calculateCumulativeMeanNormalizedDifference() {
        yinBuffer_[0] = 1.0f;

        float runningSum = 0.0f;
        for (size_t tau = 1; tau < yinBuffer_.size(); ++tau) {
            runningSum += yinBuffer_[tau];
            yinBuffer_[tau] = yinBuffer_[tau] * tau / runningSum;
        }
    }

    int getAbsoluteThreshold() {
        size_t tau_min = static_cast<size_t>(config_.sampleRate / config_.maxFrequency);
        size_t tau_max = static_cast<size_t>(config_.sampleRate / config_.minFrequency);
        tau_max = std::min(tau_max, yinBuffer_.size() - 1);

        for (size_t tau = tau_min; tau < tau_max; ++tau) {
            if (yinBuffer_[tau] < config_.threshold) {
                // Find local minimum starting from this point
                while (tau + 1 < tau_max && yinBuffer_[tau + 1] < yinBuffer_[tau]) {
                    tau++;
                }
                return static_cast<int>(tau);
            }
        }

        return -1;  // No period found
    }

    float parabolicInterpolation(int tau) {
        if (tau == 0 || tau >= static_cast<int>(yinBuffer_.size()) - 1) {
            return static_cast<float>(tau);
        }

        float s0 = yinBuffer_[tau - 1];
        float s1 = yinBuffer_[tau];
        float s2 = yinBuffer_[tau + 1];

        // Parabolic interpolation
        float a = (s0 - 2 * s1 + s2) / 2.0f;
        float b = (s2 - s0) / 2.0f;

        if (std::abs(a) < 1e-10f) {
            return static_cast<float>(tau);
        }

        float x0 = -b / (2 * a);
        return tau + x0;
    }

    float applySmoothingFilter(float newPitch) {
        if (!hasValidPitch_) {
            smoothedPitch_ = newPitch;
            return newPitch;
        }

        // Simple exponential smoothing
        smoothedPitch_ =
            (1.0f - config_.smoothingFactor) * smoothedPitch_ + config_.smoothingFactor * newPitch;
        return smoothedPitch_;
    }

    PitchResult::Vibrato analyzeVibrato() {
        PitchResult::Vibrato vibrato;

        if (pitchHistory_.size() < 20) {
            return vibrato;  // Not enough data
        }

        // Simple vibrato detection based on pitch variance and periodicity
        // Take last 2 seconds of pitch data
        size_t analysisWindow = std::min(
            static_cast<size_t>(2.0f * config_.sampleRate / config_.hopSize), pitchHistory_.size());

        auto start = pitchHistory_.end() - analysisWindow;
        auto end = pitchHistory_.end();

        // Calculate mean and variance
        float mean = std::accumulate(start, end, 0.0f) / analysisWindow;
        float variance = 0.0f;

        for (auto it = start; it != end; ++it) {
            float diff = *it - mean;
            variance += diff * diff;
        }
        variance /= analysisWindow;

        // Simple vibrato detection heuristics
        if (variance > 100.0f) {                            // Sufficient pitch variation
            vibrato.rate = 4.5f;                            // Typical vibrato rate
            vibrato.extent = std::sqrt(variance) / 100.0f;  // Convert to semitones approximation
            vibrato.regularity =
                std::min(1.0f, 1.0f / (variance / 10000.0f));  // Regularity heuristic
        }

        return vibrato;
    }

    PitchResult::PitchStatistics calculatePitchStatistics() {
        PitchResult::PitchStatistics stats;

        if (pitchHistory_.empty()) {
            return stats;
        }

        // Calculate statistics from recent pitch history (last 1 second)
        size_t analysisWindow = std::min(
            static_cast<size_t>(1.0f * config_.sampleRate / config_.hopSize), pitchHistory_.size());

        auto start = pitchHistory_.end() - analysisWindow;
        auto end = pitchHistory_.end();

        // Calculate mean
        stats.mean = std::accumulate(start, end, 0.0f) / analysisWindow;

        // Calculate standard deviation
        float variance = 0.0f;
        for (auto it = start; it != end; ++it) {
            float diff = *it - stats.mean;
            variance += diff * diff;
        }
        stats.standardDeviation = std::sqrt(variance / analysisWindow);

        // Calculate range
        auto minMax = std::minmax_element(start, end);
        stats.range = *minMax.second - *minMax.first;

        // Calculate stability (inverse of coefficient of variation)
        if (stats.mean > 0) {
            float cv = stats.standardDeviation / stats.mean;
            stats.stability = 1.0f / (1.0f + cv);
        }

        return stats;
    }

    void updatePitchHistory(float pitch, float confidence) {
        pitchHistory_.push_back(pitch);
        confidenceHistory_.push_back(confidence);

        // Keep only recent history (10 seconds)
        size_t maxHistory = static_cast<size_t>(10.0f * config_.sampleRate / config_.hopSize);
        if (pitchHistory_.size() > maxHistory) {
            pitchHistory_.erase(pitchHistory_.begin());
            confidenceHistory_.erase(confidenceHistory_.begin());
        }
    }

    std::vector<float> generatePitchContour() {
        // Return recent pitch contour (last 1 second)
        size_t contourLength = std::min(
            static_cast<size_t>(1.0f * config_.sampleRate / config_.hopSize), pitchHistory_.size());

        if (contourLength == 0) {
            return {};
        }

        std::vector<float> contour;
        contour.assign(pitchHistory_.end() - contourLength, pitchHistory_.end());
        return contour;
    }
};

// Factory method implementation
PitchTracker::Result<std::unique_ptr<PitchTracker>, PitchTracker::Error>
PitchTracker::create(const Config& config) {
    try {
        auto tracker = std::unique_ptr<PitchTracker>(new PitchTrackerImpl(config));
        return Result<std::unique_ptr<PitchTracker>, Error>(std::move(tracker));
    } catch (const std::exception& e) {
        DebugLogger::getInstance().log(Component::GENERAL,
                                       DebugLevel::ERROR,
                                       "PitchTracker::create failed: " + std::string(e.what()));
        return Result<std::unique_ptr<PitchTracker>, Error>(
            unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

// JSON export implementation
std::string PitchTracker::exportToJson(const PitchResult& result) {
    std::ostringstream json;
    json << "{";
    json << "\"frequency\":" << result.frequency << ",";
    json << "\"confidence\":" << result.confidence << ",";
    json << "\"isVoiced\":" << (result.isVoiced ? "true" : "false") << ",";
    json << "\"timestamp\":" << result.timestamp << ",";

    // Vibrato information
    json << "\"vibrato\":{";
    json << "\"rate\":" << result.vibrato.rate << ",";
    json << "\"extent\":" << result.vibrato.extent << ",";
    json << "\"regularity\":" << result.vibrato.regularity;
    json << "},";

    // Statistics
    json << "\"statistics\":{";
    json << "\"mean\":" << result.statistics.mean << ",";
    json << "\"standardDeviation\":" << result.statistics.standardDeviation << ",";
    json << "\"range\":" << result.statistics.range << ",";
    json << "\"stability\":" << result.statistics.stability;
    json << "},";

    // Pitch contour
    json << "\"contour\":[";
    for (size_t i = 0; i < result.contour.size(); ++i) {
        if (i > 0)
            json << ",";
        json << result.contour[i];
    }
    json << "]";

    json << "}";
    return json.str();
}

}  // namespace huntmaster
