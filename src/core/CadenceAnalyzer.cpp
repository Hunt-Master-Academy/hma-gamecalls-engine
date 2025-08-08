#include "huntmaster/core/CadenceAnalyzer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <numeric>
#include <set>
#include <sstream>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/security/memory-guard.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster {

/**
 * @brief Internal implementation of CadenceAnalyzer
 */
class CadenceAnalyzerImpl : public CadenceAnalyzer {
  private:
    Config config_;
    std::vector<float> buffer_;
    std::vector<float> energyHistory_;
    std::vector<float> onsetDetectionFunction_;
    std::vector<float> beatTrackingState_;

    CadenceProfile currentProfile_;
    bool isInitialized_ = false;
    bool isActive_ = false;

    // Analysis state
    size_t frameSize_;
    size_t hopSize_;
    size_t processedFrames_ = 0;
    double totalProcessingTime_ = 0.0;
    double maxProcessingTime_ = 0.0;

    // Onset detection state
    std::vector<float> prevSpectrum_;
    std::vector<float> spectralFlux_;
    float adaptiveThreshold_ = 0.0f;

  public:
    CadenceAnalyzerImpl(const Config& config) : config_(config) {
        initializeParameters();
        initializeBuffers();
        isInitialized_ = true;

        // DEBUG_LOG removed for compilation
        // "Initialized with frame size: " + std::to_string(frameSize_)
        //     + ", hop size: " + std::to_string(hopSize_)
        //     + ", sample rate: " + std::to_string(config_.sampleRate));
    }

    ~CadenceAnalyzerImpl() = default;

    Result<CadenceProfile, Error> analyzeCadence(std::span<const float> audio) override {
        security::MemoryGuard guard(security::GuardConfig{});

        if (!isInitialized_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        if (audio.size() < frameSize_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            CadenceProfile profile;
            profile.timestamp =
                static_cast<float>(processedFrames_ * hopSize_) / config_.sampleRate;

            // Detect onsets
            auto onsetsResult = detectOnsetsInternal(audio);
            if (!onsetsResult.has_value()) {
                return unexpected(onsetsResult.error());
            }

            std::vector<float> onsets = onsetsResult.value();

            // Analyze call sequence
            analyzeCallSequence(profile, onsets);

            // Estimate tempo if beat tracking enabled
            if (config_.enableBeatTracking) {
                auto tempoResult = estimateTempoInternal(audio, onsets);
                if (tempoResult.has_value()) {
                    auto tempoConf = tempoResult.value();
                    float tempo = tempoConf.first;
                    float confidence = tempoConf.second;
                    profile.estimatedTempo = tempo;
                    profile.tempoConfidence = confidence;

                    // Extract beat times
                    extractBeats(profile, onsets);
                }
            }

            // Analyze periodicity
            analyzePeriodicityInternal(profile, audio);

            // Extract rhythmic features
            if (!onsets.empty()) {
                auto rhythmResult = extractRhythmicFeaturesInternal(onsets);
                if (rhythmResult.has_value()) {
                    profile.rhythm = rhythmResult.value();
                }
            }

            // Syllable analysis if enabled
            if (config_.enableSyllableAnalysis) {
                analyzeSyllables(profile, audio, onsets);
            }

            // Calculate overall rhythm score
            profile.overallRhythmScore = calculateOverallRhythmScore(profile);
            profile.confidence = calculateConfidence(profile);
            profile.hasStrongRhythm = profile.overallRhythmScore > 0.6f;

            currentProfile_ = profile;
            isActive_ = true;

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            updatePerformanceStats(duration);

            // DEBUG_LOG removed for compilation
            // "Analysis complete - Tempo: " + std::to_string(profile.estimatedTempo)
            //     + "BPM, Rhythm Score: " + std::to_string(profile.overallRhythmScore));

            return Result<CadenceProfile, Error>(std::move(profile));

        } catch (const std::exception& e) {
            // DEBUG_LOG removed for compilation
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    Result<void, Error> processAudioChunk(std::span<const float> audio) override {
        if (!isInitialized_) {
            return unexpected(Error::INITIALIZATION_FAILED);
        }

        // Add to buffer for continuous processing
        buffer_.insert(buffer_.end(), audio.begin(), audio.end());

        // Process if we have enough data
        while (buffer_.size() >= frameSize_) {
            std::span<const float> chunk(buffer_.data(), frameSize_);
            auto result = analyzeCadence(chunk);

            if (!result.has_value()) {
                return unexpected(result.error());
            }

            // Advance by hop size
            buffer_.erase(buffer_.begin(), buffer_.begin() + hopSize_);
            processedFrames_++;
        }

        return Result<void, Error>();
    }

    Result<CadenceProfile, Error> getCurrentAnalysis() override {
        if (!isActive_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<CadenceProfile, Error>(currentProfile_);
    }

    Result<std::vector<float>, Error> detectOnsets(std::span<const float> audio) override {
        return detectOnsetsInternal(audio);
    }

    Result<std::pair<float, float>, Error> estimateTempo(std::span<const float> audio) override {
        auto onsetsResult = detectOnsetsInternal(audio);
        if (!onsetsResult.has_value()) {
            return Result<std::pair<float, float>, Error>(unexpected<Error>(onsetsResult.error()));
        }

        return estimateTempoInternal(audio, onsetsResult.value());
    }

    Result<CadenceProfile::PeriodicityMeasures, Error>
    analyzePerodicity(std::span<const float> audio) override {
        CadenceProfile::PeriodicityMeasures measures;
        analyzePeriodicityInternal(measures, audio);
        return Result<CadenceProfile::PeriodicityMeasures, Error>(measures);
    }

    Result<CadenceProfile::RhythmicFeatures, Error>
    extractRhythmicFeatures(const std::vector<float>& onsets) override {
        return extractRhythmicFeaturesInternal(onsets);
    }

    void reset() override {
        buffer_.clear();
        energyHistory_.clear();
        onsetDetectionFunction_.clear();
        beatTrackingState_.clear();
        prevSpectrum_.clear();
        spectralFlux_.clear();

        currentProfile_ = CadenceProfile{};
        isActive_ = false;
        processedFrames_ = 0;
        totalProcessingTime_ = 0.0;
        maxProcessingTime_ = 0.0;
        adaptiveThreshold_ = 0.0f;

        // DEBUG_LOG removed for compilation
    }

    Result<void, Error> updateConfig(const Config& config) override {
        if (config.sampleRate <= 0 || config.frameSize <= 0 || config.hopSize <= 0) {
            return unexpected(Error::INVALID_SAMPLE_RATE);
        }

        config_ = config;
        initializeParameters();
        initializeBuffers();

        // DEBUG_LOG removed for compilation
        return Result<void, Error>();
    }

    const Config& getConfig() const override {
        return config_;
    }

    bool isActive() const override {
        return isActive_;
    }

    std::string getProcessingStats() const override {
        std::ostringstream oss;
        oss << "CadenceAnalyzer Stats:\n";
        oss << "  Processed frames: " << processedFrames_ << "\n";
        oss << "  Total processing time: " << totalProcessingTime_ << "ms\n";
        oss << "  Max processing time: " << maxProcessingTime_ << "ms\n";
        if (processedFrames_ > 0) {
            oss << "  Average processing time: " << (totalProcessingTime_ / processedFrames_)
                << "ms\n";
        }
        oss << "  Frame size: " << frameSize_ << " samples\n";
        oss << "  Hop size: " << hopSize_ << " samples\n";
        oss << "  Sample rate: " << config_.sampleRate << "Hz";
        return oss.str();
    }

    Result<std::vector<float>, Error> getOnsetDetectionFunction() override {
        if (onsetDetectionFunction_.empty()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<std::vector<float>, Error>(onsetDetectionFunction_);
    }

    Result<std::vector<float>, Error> getBeatTrackingState() override {
        if (beatTrackingState_.empty()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<std::vector<float>, Error>(beatTrackingState_);
    }

  private:
    void initializeParameters() {
        frameSize_ = static_cast<size_t>(config_.frameSize * config_.sampleRate);
        hopSize_ = static_cast<size_t>(config_.hopSize * config_.sampleRate);

        // Ensure frame size is reasonable
        frameSize_ = std::max(frameSize_, static_cast<size_t>(512));
        hopSize_ = std::max(hopSize_, static_cast<size_t>(256));

        // Ensure hop size doesn't exceed frame size
        hopSize_ = std::min(hopSize_, frameSize_ / 2);
    }

    void initializeBuffers() {
        buffer_.clear();
        buffer_.reserve(frameSize_ * 2);

        energyHistory_.clear();
        energyHistory_.reserve(1000);  // Reserve space for history

        onsetDetectionFunction_.clear();
        beatTrackingState_.clear();

        prevSpectrum_.resize(frameSize_ / 2 + 1, 0.0f);
        spectralFlux_.clear();
    }

    Result<std::vector<float>, Error> detectOnsetsInternal(std::span<const float> audio) {
        if (!config_.enableOnsetDetection) {
            return Result<std::vector<float>, Error>(std::vector<float>{});
        }

        std::vector<float> onsets;

        try {
            // Compute spectral flux for onset detection
            computeSpectralFlux(audio);

            // Peak picking on onset detection function
            peakPickOnsets(onsets);

            // Update onset detection function for visualization
            onsetDetectionFunction_ = spectralFlux_;

            return Result<std::vector<float>, Error>(std::move(onsets));

        } catch (const std::exception& e) {
            // DEBUG_LOG removed for compilation
            return Result<std::vector<float>, Error>(
                unexpected<Error>(Error::ONSET_DETECTION_ERROR));
        }
    }

    void computeSpectralFlux(std::span<const float> audio) {
        // Simple spectral flux implementation
        size_t numFrames = (audio.size() - frameSize_) / hopSize_ + 1;
        spectralFlux_.resize(numFrames);

        for (size_t frame = 0; frame < numFrames; ++frame) {
            size_t startIdx = frame * hopSize_;

            // Compute magnitude spectrum for current frame
            std::vector<float> currentSpectrum(frameSize_ / 2 + 1);
            computeMagnitudeSpectrum(audio.subspan(startIdx, frameSize_), currentSpectrum);

            // Compute spectral flux (positive differences)
            float flux = 0.0f;
            for (size_t bin = 0; bin < currentSpectrum.size(); ++bin) {
                float diff = currentSpectrum[bin] - prevSpectrum_[bin];
                if (diff > 0.0f) {
                    flux += diff;
                }
            }

            spectralFlux_[frame] = flux;
            prevSpectrum_ = std::move(currentSpectrum);
        }

        // Apply smoothing
        applySmoothingToFlux();
    }

    void computeMagnitudeSpectrum(std::span<const float> frame, std::vector<float>& spectrum) {
        // Simple DFT implementation for magnitude spectrum
        for (size_t k = 0; k < spectrum.size(); ++k) {
            float real = 0.0f, imag = 0.0f;

            for (size_t n = 0; n < frame.size(); ++n) {
                float angle = -2.0f * M_PI * k * n / frame.size();
                real += frame[n] * std::cos(angle);
                imag += frame[n] * std::sin(angle);
            }

            spectrum[k] = std::sqrt(real * real + imag * imag);
        }
    }

    void applySmoothingToFlux() {
        // Simple moving average smoothing
        const size_t windowSize = 3;
        std::vector<float> smoothed(spectralFlux_.size());

        for (size_t i = 0; i < spectralFlux_.size(); ++i) {
            float sum = 0.0f;
            size_t count = 0;

            size_t start = (i >= windowSize / 2) ? i - windowSize / 2 : 0;
            size_t end = std::min(i + windowSize / 2 + 1, spectralFlux_.size());

            for (size_t j = start; j < end; ++j) {
                sum += spectralFlux_[j];
                count++;
            }

            smoothed[i] = count > 0 ? sum / count : 0.0f;
        }

        spectralFlux_ = std::move(smoothed);
    }

    void peakPickOnsets(std::vector<float>& onsets) {
        if (spectralFlux_.size() < 3)
            return;

        // Adaptive thresholding
        updateAdaptiveThreshold();

        // Find peaks above threshold
        for (size_t i = 1; i < spectralFlux_.size() - 1; ++i) {
            if (spectralFlux_[i] > spectralFlux_[i - 1] && spectralFlux_[i] > spectralFlux_[i + 1]
                && spectralFlux_[i] > config_.onsetThreshold + adaptiveThreshold_) {
                float onsetTime = static_cast<float>(i * hopSize_) / config_.sampleRate;
                onsets.push_back(onsetTime);
            }
        }
    }

    void updateAdaptiveThreshold() {
        if (spectralFlux_.empty())
            return;

        // Calculate median of recent flux values for adaptive threshold
        const size_t historySize = std::min(spectralFlux_.size(), static_cast<size_t>(100));
        std::vector<float> recentFlux(spectralFlux_.end() - historySize, spectralFlux_.end());

        std::sort(recentFlux.begin(), recentFlux.end());
        float median = recentFlux[recentFlux.size() / 2];

        adaptiveThreshold_ = median * config_.adaptiveThreshold;
    }

    Result<std::pair<float, float>, Error> estimateTempoInternal(std::span<const float> audio,
                                                                 const std::vector<float>& onsets) {
        if (onsets.size() < 3) {
            return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
        }

        // Calculate inter-onset intervals
        std::vector<float> intervals;
        for (size_t i = 1; i < onsets.size(); ++i) {
            float interval = onsets[i] - onsets[i - 1];
            if (interval > 0.0f) {
                intervals.push_back(interval);
            }
        }

        if (intervals.empty()) {
            return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
        }

        // Find most common interval (simple mode estimation)
        std::sort(intervals.begin(), intervals.end());

        float bestInterval = 0.0f;
        float bestConfidence = 0.0f;

        // Create histogram of intervals
        const float binSize = 0.05f;  // 50ms bins
        std::map<int, int> histogram;

        for (float interval : intervals) {
            int bin = static_cast<int>(interval / binSize);
            histogram[bin]++;
        }

        // Find peak bin
        int maxCount = 0;
        int bestBin = 0;
        for (const auto& [bin, count] : histogram) {
            if (count > maxCount) {
                maxCount = count;
                bestBin = bin;
            }
        }

        if (maxCount > 0) {
            bestInterval = bestBin * binSize;
            bestConfidence = static_cast<float>(maxCount) / intervals.size();
        }

        // Convert to BPM
        float tempo = 0.0f;
        if (bestInterval > 0.0f) {
            tempo = 60.0f / bestInterval;

            // Clamp to valid range
            tempo = std::clamp(tempo, config_.minTempo, config_.maxTempo);
        }

        return Result<std::pair<float, float>, Error>(std::make_pair(tempo, bestConfidence));
    }

    void analyzeCallSequence(CadenceProfile& profile, const std::vector<float>& onsets) {
        auto& sequence = profile.sequence;

        sequence.callOnsets = onsets;
        sequence.numCalls = onsets.size();

        if (onsets.empty())
            return;

        sequence.sequenceDuration = onsets.back() - onsets.front();

        if (sequence.sequenceDuration > 0.0f) {
            sequence.callRate = static_cast<float>(sequence.numCalls) / sequence.sequenceDuration;
        }

        // Estimate call durations (simplified)
        sequence.callDurations.resize(onsets.size());
        sequence.interCallIntervals.resize(onsets.size() - 1);

        for (size_t i = 0; i < onsets.size(); ++i) {
            // Rough estimate: calls last until next onset or fixed duration
            if (i < onsets.size() - 1) {
                float duration = (onsets[i + 1] - onsets[i]) * 0.7f;   // 70% of interval
                sequence.callDurations[i] = std::min(duration, 2.0f);  // Max 2 seconds
                sequence.interCallIntervals[i] =
                    onsets[i + 1] - onsets[i] - sequence.callDurations[i];
            } else {
                sequence.callDurations[i] = 0.5f;  // Default duration
            }
        }
    }

    void extractBeats(CadenceProfile& profile, const std::vector<float>& onsets) {
        // Simplified beat tracking - use onsets as beat candidates
        profile.beatTimes = onsets;
        profile.beatStrengths.resize(onsets.size(), 1.0f);  // Uniform strength

        // Calculate inter-beat intervals
        for (size_t i = 1; i < onsets.size(); ++i) {
            float interval = onsets[i] - onsets[i - 1];
            profile.interBeatIntervals.push_back(interval);
        }

        // Update beat tracking state for visualization
        beatTrackingState_ = profile.beatStrengths;
    }

    void analyzePeriodicityInternal(CadenceProfile& profile, std::span<const float> audio) {
        analyzePeriodicityInternal(profile.periodicity, audio);
    }

    void analyzePeriodicityInternal(CadenceProfile::PeriodicityMeasures& measures,
                                    std::span<const float> audio) {
        // Compute autocorrelation for periodicity analysis
        std::vector<float> autocorr = computeAutocorrelation(audio);

        if (autocorr.empty())
            return;

        // Find peaks in autocorrelation
        std::vector<std::pair<size_t, float>> peaks;
        findAutocorrelationPeaks(autocorr, peaks);

        if (!peaks.empty()) {
            // Best peak (excluding lag 0)
            auto bestPeak =
                std::max_element(peaks.begin(), peaks.end(), [](const auto& a, const auto& b) {
                    return a.second < b.second;
                });

            measures.autocorrelationPeak = bestPeak->second;
            measures.dominantPeriod = static_cast<float>(bestPeak->first) / config_.sampleRate;
            measures.periodicityStrength = bestPeak->second;

            // Extract multiple periodicities
            for (const auto& [lag, strength] : peaks) {
                float period = static_cast<float>(lag) / config_.sampleRate;
                if (period >= config_.minPeriod && period <= config_.maxPeriod) {
                    measures.periodicities.push_back(period);
                    measures.periodicityStrengths.push_back(strength);
                }
            }
        }
    }

    std::vector<float> computeAutocorrelation(std::span<const float> audio) {
        size_t maxLag = std::min(config_.autocorrelationLags, audio.size() / 2);
        std::vector<float> autocorr(maxLag);

        for (size_t lag = 1; lag < maxLag; ++lag) {
            float sum = 0.0f;
            size_t count = 0;

            for (size_t i = 0; i < audio.size() - lag; ++i) {
                sum += audio[i] * audio[i + lag];
                count++;
            }

            autocorr[lag] = count > 0 ? sum / count : 0.0f;
        }

        // Normalize
        float maxVal = *std::max_element(autocorr.begin(), autocorr.end());
        if (maxVal > 0.0f) {
            for (float& val : autocorr) {
                val /= maxVal;
            }
        }

        return autocorr;
    }

    void findAutocorrelationPeaks(const std::vector<float>& autocorr,
                                  std::vector<std::pair<size_t, float>>& peaks) {
        // Find local maxima
        for (size_t i = 1; i < autocorr.size() - 1; ++i) {
            if (autocorr[i] > autocorr[i - 1] && autocorr[i] > autocorr[i + 1]
                && autocorr[i] > 0.1f) {
                peaks.push_back({i, autocorr[i]});
            }
        }

        // Sort by strength
        std::sort(peaks.begin(), peaks.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        // Keep only top peaks
        if (peaks.size() > 10) {
            peaks.resize(10);
        }
    }

    Result<CadenceProfile::RhythmicFeatures, Error>
    extractRhythmicFeaturesInternal(const std::vector<float>& onsets) {
        CadenceProfile::RhythmicFeatures features;

        if (onsets.size() < 3) {
            return Result<CadenceProfile::RhythmicFeatures, Error>(features);
        }

        // Calculate intervals
        std::vector<float> intervals;
        for (size_t i = 1; i < onsets.size(); ++i) {
            intervals.push_back(onsets[i] - onsets[i - 1]);
        }

        // Rhythm regularity - based on interval variance
        float meanInterval =
            std::accumulate(intervals.begin(), intervals.end(), 0.0f) / intervals.size();
        float variance = 0.0f;
        for (float interval : intervals) {
            float diff = interval - meanInterval;
            variance += diff * diff;
        }
        variance /= intervals.size();

        features.rhythmRegularity = 1.0f / (1.0f + variance);  // Higher regularity = lower variance

        // Rhythm complexity - based on number of unique intervals
        std::set<int> uniqueIntervals;
        for (float interval : intervals) {
            uniqueIntervals.insert(static_cast<int>(interval * 100));  // 10ms precision
        }
        features.rhythmComplexity = static_cast<float>(uniqueIntervals.size()) / intervals.size();

        // Simplified syncopation measure
        features.syncopation = 1.0f - features.rhythmRegularity;

        // Polyrhythm detection (simplified)
        features.polyrhythm = features.rhythmComplexity > 0.5f ? features.rhythmComplexity : 0.0f;

        // Groove factor (balance between regularity and complexity)
        features.groove = features.rhythmRegularity * features.rhythmComplexity;

        return Result<CadenceProfile::RhythmicFeatures, Error>(features);
    }

    void analyzeSyllables(CadenceProfile& profile,
                          std::span<const float> audio,
                          const std::vector<float>& onsets) {
        auto& syllables = profile.syllables;

        // Use onsets as syllable boundaries (simplified)
        syllables.syllableOnsets = onsets;

        if (onsets.size() < 2)
            return;

        // Estimate syllable durations
        syllables.syllableDurations.resize(onsets.size());
        float totalDuration = 0.0f;

        for (size_t i = 0; i < onsets.size(); ++i) {
            if (i < onsets.size() - 1) {
                syllables.syllableDurations[i] =
                    (onsets[i + 1] - onsets[i]) * 0.8f;  // 80% of interval
            } else {
                syllables.syllableDurations[i] = 0.3f;  // Default duration
            }
            totalDuration += syllables.syllableDurations[i];
        }

        syllables.avgSyllableDuration = totalDuration / onsets.size();

        float sequenceDuration = onsets.back() - onsets.front();
        if (sequenceDuration > 0.0f) {
            syllables.syllableRate = static_cast<float>(onsets.size()) / sequenceDuration;
        }

        // Speech rhythm score based on syllable regularity
        float avgDuration = syllables.avgSyllableDuration;
        float variance = 0.0f;
        for (float duration : syllables.syllableDurations) {
            float diff = duration - avgDuration;
            variance += diff * diff;
        }
        variance /= syllables.syllableDurations.size();

        syllables.speechRhythm =
            1.0f / (1.0f + variance * 10.0f);  // Higher score = more speech-like
    }

    float calculateOverallRhythmScore(const CadenceProfile& profile) {
        float score = 0.0f;

        // Weight different components
        score += profile.rhythm.rhythmRegularity * 0.3f;
        score += profile.rhythm.groove * 0.2f;
        score += profile.periodicity.periodicityStrength * 0.3f;
        score += (profile.tempoConfidence > 0.5f ? 0.2f : 0.0f);

        return std::min(1.0f, score);
    }

    float calculateConfidence(const CadenceProfile& profile) {
        float confidence = 0.0f;

        // Confidence based on various factors
        confidence += profile.tempoConfidence * 0.3f;
        confidence += profile.periodicity.periodicityStrength * 0.3f;
        confidence += profile.rhythm.rhythmRegularity * 0.2f;
        confidence += (profile.sequence.numCalls > 2 ? 0.2f : 0.0f);

        return std::min(1.0f, confidence);
    }

    void updatePerformanceStats(double processingTime) {
        totalProcessingTime_ += processingTime;
        maxProcessingTime_ = std::max(maxProcessingTime_, processingTime);
    }
};

// Factory method implementation
CadenceAnalyzer::Result<std::unique_ptr<CadenceAnalyzer>, CadenceAnalyzer::Error>
CadenceAnalyzer::create(const Config& config) {
    try {
        if (config.sampleRate <= 0) {
            return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_SAMPLE_RATE));
        }

        if (config.frameSize <= 0 || config.hopSize <= 0) {
            return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_FRAME_SIZE));
        }

        auto analyzer = std::make_unique<CadenceAnalyzerImpl>(config);
        return Result<std::unique_ptr<CadenceAnalyzer>, Error>(std::move(analyzer));

    } catch (const std::exception& e) {
        // DEBUG_LOG removed for compilation
        return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
            unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

// JSON export implementation
std::string CadenceAnalyzer::exportToJson(const CadenceProfile& profile) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"estimatedTempo\": " << profile.estimatedTempo << ",\n";
    json << "  \"tempoConfidence\": " << profile.tempoConfidence << ",\n";
    json << "  \"overallRhythmScore\": " << profile.overallRhythmScore << ",\n";
    json << "  \"confidence\": " << profile.confidence << ",\n";
    json << "  \"hasStrongRhythm\": " << (profile.hasStrongRhythm ? "true" : "false") << ",\n";
    json << "  \"timestamp\": " << profile.timestamp << ",\n";

    json << "  \"beatTimes\": [";
    for (size_t i = 0; i < profile.beatTimes.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << profile.beatTimes[i];
    }
    json << "],\n";

    json << "  \"sequence\": {\n";
    json << "    \"numCalls\": " << profile.sequence.numCalls << ",\n";
    json << "    \"callRate\": " << profile.sequence.callRate << ",\n";
    json << "    \"sequenceDuration\": " << profile.sequence.sequenceDuration << "\n";
    json << "  },\n";

    json << "  \"periodicity\": {\n";
    json << "    \"dominantPeriod\": " << profile.periodicity.dominantPeriod << ",\n";
    json << "    \"periodicityStrength\": " << profile.periodicity.periodicityStrength << ",\n";
    json << "    \"autocorrelationPeak\": " << profile.periodicity.autocorrelationPeak << "\n";
    json << "  },\n";

    json << "  \"rhythm\": {\n";
    json << "    \"rhythmComplexity\": " << profile.rhythm.rhythmComplexity << ",\n";
    json << "    \"rhythmRegularity\": " << profile.rhythm.rhythmRegularity << ",\n";
    json << "    \"syncopation\": " << profile.rhythm.syncopation << ",\n";
    json << "    \"groove\": " << profile.rhythm.groove << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

}  // namespace huntmaster
