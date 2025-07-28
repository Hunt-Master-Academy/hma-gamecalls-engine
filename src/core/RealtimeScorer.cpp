#include "huntmaster/core/RealtimeScorer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/AudioLevelProcessor.h"
#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/MFCCProcessor.h"

// Enable debug output for RealtimeScorer
#define DEBUG_REALTIME_SCORER 0

// Debug logging macros
#if DEBUG_REALTIME_SCORER
#define SCORER_LOG_DEBUG(msg) std::cout << "[SCORER DEBUG] " << msg << std::endl
#define SCORER_LOG_ERROR(msg) std::cerr << "[SCORER ERROR] " << msg << std::endl
#else
#define SCORER_LOG_DEBUG(msg) \
    do {                      \
    } while (0)
#define SCORER_LOG_ERROR(msg) \
    do {                      \
    } while (0)
#endif

namespace huntmaster {

/// Implementation details for RealtimeScorer
class RealtimeScorer::Impl {
  public:
    Config config_;
    mutable std::mutex mutex_;

    // Component processors
    std::unique_ptr<MFCCProcessor> mfccProcessor_;
    std::unique_ptr<DTWComparator> dtwComparator_;
    std::unique_ptr<AudioLevelProcessor> levelProcessor_;

    // Master call data
    std::vector<std::vector<float>> masterMfccFeatures_;
    float masterCallRms_ = 0.0f;
    float masterCallDuration_ = 0.0f;
    bool hasMasterCall_ = false;

    // Live audio accumulation
    std::vector<float> liveAudioBuffer_;
    std::vector<std::vector<float>> liveMfccFeatures_;
    float liveAudioDuration_ = 0.0f;

    // Scoring state
    std::deque<SimilarityScore> scoringHistory_;
    SimilarityScore currentScore_;
    SimilarityScore peakScore_;
    std::atomic<bool> initialized_{false};
    std::chrono::steady_clock::time_point lastUpdateTime_;
    std::chrono::steady_clock::time_point sessionStartTime_;

    // Processing statistics
    std::atomic<size_t> totalSamplesProcessed_{0};
    std::atomic<float> averageSignalLevel_{0.0f};

    explicit Impl(const Config& config);

    void initializeComponents();
    float calculateWeightedScore(float mfcc, float volume, float timing, float pitch) const;
    float calculatePitchEstimate(const std::vector<float>& audioBuffer) const;
    float calculateProgressRatio() const;
    std::string generateRecommendation(const SimilarityScore& score) const;
    bool isScoreTrendingUp() const;
};

// Helper functions for scoring logic, scoped to this file.
static float
calculateVolumeSimilarity(float liveRms, float masterRms, float tolerance = 0.3f) noexcept {
    if (masterRms < 1e-6f) {
        return (liveRms < 1e-6f) ? 1.0f : 0.0f;
    }
    const float ratio = liveRms / masterRms;
    const float error = std::abs(1.0f - ratio);
    float result = std::max(0.0f, 1.0f - (error / tolerance));

#if DEBUG_REALTIME_SCORER
    std::cout << "[DEBUG] calculateVolumeSimilarity: liveRms=" << liveRms
              << ", masterRms=" << masterRms << ", ratio=" << ratio << ", error=" << error
              << ", tolerance=" << tolerance << ", result=" << result << std::endl;
#endif

    return result;
}

static float calculateTimingAccuracy(float liveDuration, float masterDuration) noexcept {
    if (masterDuration <= 0.0f)
        return 0.5f;  // Neutral score if master duration is unknown
    const float ratio = liveDuration / masterDuration;
    // Penalize for being too short or too long
    if (ratio < 1.0f) {
        return ratio;  // Linearly increases as live duration approaches master duration
    }
    // Slower penalty for going over time
    return std::max(0.0f, 1.0f - (ratio - 1.0f) * 0.5f);
}

static float calculateConfidence(size_t samplesAnalyzed,
                                 float signalQuality,
                                 size_t minSamplesForConfidence) noexcept {
    if (samplesAnalyzed < minSamplesForConfidence) {
        return static_cast<float>(samplesAnalyzed) / static_cast<float>(minSamplesForConfidence)
               * signalQuality;
    }
    return signalQuality;
}

RealtimeScorer::RealtimeScorer() : impl_(std::make_unique<Impl>(Config{})) {}

RealtimeScorer::RealtimeScorer(const Config& config) : impl_(std::make_unique<Impl>(config)) {}

RealtimeScorer::~RealtimeScorer() = default;

RealtimeScorer::Impl::Impl(const Config& config) : config_(config) {
    if (config_.isValid()) {
        initializeComponents();
        sessionStartTime_ = std::chrono::steady_clock::now();
        lastUpdateTime_ = sessionStartTime_;
        initialized_.store(true);
    }
}

void RealtimeScorer::Impl::initializeComponents() {
    // Initialize MFCC processor with appropriate settings
    MFCCProcessor::Config mfccConfig;
    mfccConfig.sample_rate = static_cast<size_t>(config_.sampleRate);
    mfccConfig.frame_size = 1024;
    mfccConfig.num_coefficients = 13;
    mfccProcessor_ = std::make_unique<MFCCProcessor>(mfccConfig);

    // Initialize DTW comparator
    DTWComparator::Config dtwConfig;
    dtwComparator_ = std::make_unique<DTWComparator>(dtwConfig);

    // Initialize audio level processor for volume analysis
    AudioLevelProcessor::Config levelConfig;
    levelConfig.sampleRate = config_.sampleRate;
    levelConfig.updateRateMs = config_.updateRateMs;
    levelProcessor_ = std::make_unique<AudioLevelProcessor>(levelConfig);
}

float RealtimeScorer::Impl::calculateWeightedScore(float mfcc,
                                                   float volume,
                                                   float timing,
                                                   float pitch) const {
    return config_.mfccWeight * mfcc + config_.volumeWeight * volume + config_.timingWeight * timing
           + config_.pitchWeight * pitch;
}

float RealtimeScorer::Impl::calculatePitchEstimate(const std::vector<float>& audioBuffer) const {
    if (audioBuffer.empty() || audioBuffer.size() < 256) {
        return 0.0f;  // Not enough data for reliable pitch estimation
    }

    // Simple pitch estimation using autocorrelation-based fundamental frequency detection
    const size_t windowSize = std::min(static_cast<size_t>(1024), audioBuffer.size());
    const float sampleRate = config_.sampleRate;
    const size_t minPeriod = static_cast<size_t>(sampleRate / 8000.0f);  // 8kHz max frequency
    const size_t maxPeriod = static_cast<size_t>(sampleRate / 80.0f);    // 80Hz min frequency

    // Calculate autocorrelation to find periodic patterns
    float maxCorrelation = 0.0f;
    size_t bestPeriod = 0;

    for (size_t period = minPeriod; period < maxPeriod && period < windowSize / 2; ++period) {
        float correlation = 0.0f;
        float normalization = 0.0f;

        for (size_t i = 0; i < windowSize - period; ++i) {
            correlation += audioBuffer[i] * audioBuffer[i + period];
            normalization += audioBuffer[i] * audioBuffer[i];
        }

        if (normalization > 1e-10f) {
            correlation /= normalization;

            if (correlation > maxCorrelation) {
                maxCorrelation = correlation;
                bestPeriod = period;
            }
        }
    }

    // Convert period to frequency, with confidence threshold
    if (maxCorrelation > 0.3f && bestPeriod > 0) {
        float fundamentalFreq = sampleRate / static_cast<float>(bestPeriod);

        // Sanity check: typical wildlife call range (80Hz - 8kHz)
        if (fundamentalFreq >= 80.0f && fundamentalFreq <= 8000.0f) {
            return fundamentalFreq;
        }
    }

    // Fallback: estimate pitch using spectral centroid as frequency indicator
    float spectralCentroid = 0.0f;
    float magnitudeSum = 0.0f;

    for (size_t i = 0; i < std::min(windowSize, audioBuffer.size()); ++i) {
        float magnitude = std::abs(audioBuffer[i]);
        spectralCentroid += static_cast<float>(i) * magnitude;
        magnitudeSum += magnitude;
    }

    if (magnitudeSum > 1e-10f) {
        spectralCentroid /= magnitudeSum;
        // Convert bin index to approximate frequency
        float estimatedFreq = (spectralCentroid / windowSize) * (sampleRate / 2.0f);
        return std::clamp(estimatedFreq, 80.0f, 8000.0f);
    }

    return 1000.0f;  // Default fallback frequency for wildlife calls
}

float RealtimeScorer::Impl::calculateProgressRatio() const {
    if (!hasMasterCall_ || masterCallDuration_ <= 0.0f) {
        return 0.0f;
    }

    return std::min(1.0f, liveAudioDuration_ / masterCallDuration_);
}

std::string RealtimeScorer::Impl::generateRecommendation(const SimilarityScore& score) const {
    if (score.overall >= config_.minScoreForMatch) {
        if (score.mfcc < score.volume) {
            return "Good volume matching! Focus on call pattern and timing.";
        } else if (score.volume < score.mfcc) {
            return "Good call pattern! Adjust your volume level.";
        } else {
            return "Excellent technique! Keep it consistent.";
        }
    } else {
        if (score.mfcc < 0.002f) {
            return "Focus on matching the call pattern and pitch contour.";
        } else if (score.volume < 0.5f) {
            return "Adjust your volume to better match the master call.";
        } else {
            return "Work on timing and overall consistency.";
        }
    }
}

bool RealtimeScorer::Impl::isScoreTrendingUp() const {
    // Require at least 6 scores to compare 3 recent and 3 older
    if (scoringHistory_.size() < 6)
        return false;

    const size_t recentCount = 3;
    const size_t olderCount = 3;

    float recentAvg = 0.0f;
    float olderAvg = 0.0f;

    // Most recent scores: [0, 1, 2]
    for (size_t i = 0; i < recentCount; ++i) {
        recentAvg += scoringHistory_[i].overall;
    }
    recentAvg /= recentCount;

    // Older scores: [3, 4, 5]
    for (size_t i = recentCount; i < recentCount + olderCount; ++i) {
        olderAvg += scoringHistory_[i].overall;
    }
    olderAvg /= olderCount;

    return recentAvg > olderAvg * 1.1f;  // 10% improvement threshold
}

bool RealtimeScorer::setMasterCall(const std::string& masterCallPath) noexcept {
    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);

        // Try to load as feature file first (.mfc)
        if (masterCallPath.size() >= 4
            && masterCallPath.compare(masterCallPath.size() - 4, 4, ".mfc") == 0) {
            std::ifstream file(masterCallPath, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }

            // Read feature dimensions
            uint32_t numFrames, numCoeffs;
            file.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
            file.read(reinterpret_cast<char*>(&numCoeffs), sizeof(numCoeffs));

            if (!file || numFrames == 0 || numCoeffs == 0) {
                return false;
            }

            // Read feature data
            impl_->masterMfccFeatures_.clear();
            impl_->masterMfccFeatures_.reserve(numFrames);

            for (uint32_t frame = 0; frame < numFrames; ++frame) {
                std::vector<float> frameFeatures(numCoeffs);
                file.read(reinterpret_cast<char*>(frameFeatures.data()), numCoeffs * sizeof(float));

                if (!file) {
                    impl_->masterMfccFeatures_.clear();
                    return false;
                }

                impl_->masterMfccFeatures_.push_back(std::move(frameFeatures));
            }

            // Estimate master call duration (approximate)
            const float frameRateMs =
                512.0f / impl_->config_.sampleRate * 1000.0f;                // Hop size based
            impl_->masterCallDuration_ = numFrames * frameRateMs / 1000.0f;  // Convert to seconds

            // Calculate approximate RMS from MFCC energy (using first coefficient as proxy).
            // Note: The first MFCC coefficient may or may not represent true signal energy,
            // depending on the MFCC implementation. Adjust this if your MFCCs are computed
            // differently.
            float energySum = 0.0f;
            for (const auto& frame : impl_->masterMfccFeatures_) {
                if (!frame.empty()) {
                    energySum += frame[0];  // Using first MFCC coefficient as energy proxy
                }
            }
            impl_->masterCallRms_ = energySum / impl_->masterMfccFeatures_.size();

#if DEBUG_REALTIME_SCORER
            std::cout
                << "[DEBUG] RealtimeScorer setMasterCall: loaded from .mfc file, masterCallRms_="
                << impl_->masterCallRms_ << std::endl;
#endif

        } else {
            // Load from audio file
            unsigned int channels;
            unsigned int sampleRate;
            drwav_uint64 totalFrameCount;
            float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
                masterCallPath.c_str(), &channels, &sampleRate, &totalFrameCount, nullptr);

            if (pSampleData == nullptr) {
                return false;  // Failed to load WAV file
            }

            std::vector<float> audioData(pSampleData, pSampleData + totalFrameCount * channels);
            drwav_free(pSampleData, nullptr);

            // Convert to mono if necessary
            std::vector<float> monoData;
            if (channels > 1) {
                monoData.reserve(totalFrameCount);
                for (drwav_uint64 i = 0; i < totalFrameCount; ++i) {
                    float frameSum = 0.0f;
                    for (unsigned int c = 0; c < channels; ++c) {
                        frameSum += audioData[i * channels + c];
                    }
                    monoData.push_back(frameSum / channels);
                }
            } else {
                monoData = std::move(audioData);
            }

            // Extract features
            auto featuresResult = impl_->mfccProcessor_->extractFeaturesFromBuffer(monoData, 512);
            if (!featuresResult.has_value()) {
                return false;
            }
            impl_->masterMfccFeatures_ = std::move(*featuresResult);

            // Calculate RMS and duration
            float rms = 0.0f;
            for (const auto& sample : monoData) {
                rms += sample * sample;
            }
            impl_->masterCallRms_ = std::sqrt(rms / monoData.size());
            impl_->masterCallDuration_ = static_cast<float>(totalFrameCount) / sampleRate;

#if DEBUG_REALTIME_SCORER
            std::cout
                << "[DEBUG] RealtimeScorer setMasterCall: loaded from audio file, masterCallRms_="
                << impl_->masterCallRms_ << ", duration=" << impl_->masterCallDuration_
                << std::endl;
#endif
        }

        impl_->hasMasterCall_ = true;
        return true;

    } catch (...) {
        impl_->hasMasterCall_ = false;
        return false;
    }
}

RealtimeScorer::Result RealtimeScorer::processAudio(std::span<const float> samples,
                                                    int numChannels) noexcept {
    if (!impl_->initialized_.load()) {
        return huntmaster::unexpected(Error::INITIALIZATION_FAILED);
    }

    if (!impl_->hasMasterCall_) {
        return huntmaster::unexpected(Error::NO_MASTER_CALL);
    }

    if (samples.empty()) {
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    if (numChannels <= 0 || numChannels > 8) {
        return huntmaster::unexpected(Error::INVALID_AUDIO_DATA);
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);

        // Convert multi-channel to mono by averaging
        std::vector<float> monoSamples;
        const size_t frameCount = samples.size() / numChannels;
        monoSamples.reserve(frameCount);

#if DEBUG_REALTIME_SCORER
        std::cout << "[DEBUG] RealtimeScorer processAudio: samples.size()=" << samples.size()
                  << ", numChannels=" << numChannels << ", frameCount=" << frameCount << std::endl;
#endif

        for (size_t frame = 0; frame < frameCount; ++frame) {
            float frameSum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                frameSum += samples[frame * numChannels + ch];
            }
            monoSamples.push_back(frameSum / numChannels);
        }

        // Accumulate live audio for analysis
        impl_->liveAudioBuffer_.insert(
            impl_->liveAudioBuffer_.end(), monoSamples.begin(), monoSamples.end());

        // Update duration
        impl_->liveAudioDuration_ += static_cast<float>(frameCount) / impl_->config_.sampleRate;

        // Process audio through level processor for volume analysis
        auto levelResult = impl_->levelProcessor_->processAudio(monoSamples, 1);
        if (!levelResult.has_value()) {
#if DEBUG_REALTIME_SCORER
            std::cout << "[DEBUG] RealtimeScorer processAudio: levelProcessor failed" << std::endl;
#endif
            return huntmaster::unexpected(Error::COMPONENT_ERROR);
        }

        auto levelMeasurement = *levelResult;

#if DEBUG_REALTIME_SCORER
        std::cout << "[DEBUG] RealtimeScorer processAudio: levelMeasurement.rmsLinear="
                  << levelMeasurement.rmsLinear << ", masterCallRms_=" << impl_->masterCallRms_
                  << std::endl;
#endif

        // Extract MFCC features if we have enough audio
        if (impl_->liveAudioBuffer_.size() >= 1024) {  // Minimum frame size
            auto mfccResult =
                impl_->mfccProcessor_->extractFeaturesFromBuffer(impl_->liveAudioBuffer_, 512);
            if (mfccResult.has_value()) {
                auto features = *mfccResult;
                if (!features.empty()) {
                    impl_->liveMfccFeatures_ = std::move(features);
                }
            }
        }

        // Calculate similarity scores
        SimilarityScore score;
        score.timestamp = std::chrono::steady_clock::now();
        score.samplesAnalyzed = samples.size();  // Use total input samples for tracking

#if DEBUG_REALTIME_SCORER
        std::cout << "[DEBUG] RealtimeScorer processAudio: samplesAnalyzed set to samples.size()="
                  << score.samplesAnalyzed << " (frameCount=" << frameCount << ")" << std::endl;
#endif

        // 1. MFCC Similarity (using DTW)
        if (!impl_->liveMfccFeatures_.empty() && !impl_->masterMfccFeatures_.empty()) {
            float dtwDistance = impl_->dtwComparator_->compare(impl_->liveMfccFeatures_,
                                                               impl_->masterMfccFeatures_);

            // Convert DTW distance to similarity (lower distance = higher similarity)
            float scaling = impl_->config_.dtwDistanceScaling;
            score.mfcc = std::max(0.0f, 1.0f / (1.0f + dtwDistance * scaling));
        }

        // 2. Volume Similarity
        if (impl_->masterCallRms_ > 0.0f) {
            score.volume =
                calculateVolumeSimilarity(levelMeasurement.rmsLinear, impl_->masterCallRms_, 2.0f);
#if DEBUG_REALTIME_SCORER
            std::cout << "[DEBUG] RealtimeScorer processAudio: volume similarity calculated="
                      << score.volume << std::endl;
#endif
        } else {
#if DEBUG_REALTIME_SCORER
            std::cout << "[DEBUG] RealtimeScorer processAudio: masterCallRms_ is 0, volume score "
                         "not calculated"
                      << std::endl;
#endif
        }

        // 3. Timing Accuracy
        score.timing =
            calculateTimingAccuracy(impl_->liveAudioDuration_, impl_->masterCallDuration_);

        // 4. Pitch Similarity - Analyze fundamental frequency patterns
        if (impl_->config_.enablePitchAnalysis && !impl_->liveAudioBuffer_.empty()
            && impl_->masterCallDuration_ > 0.0f) {
            // Simplified pitch similarity based on spectral centroid and RMS variation
            float livePitchEstimate = impl_->calculatePitchEstimate(impl_->liveAudioBuffer_);

            // For master call pitch, we'd ideally have pre-computed values
            // Here we estimate based on typical wildlife call characteristics
            float masterPitchEstimate = 2000.0f;  // Typical bird call frequency in Hz

            // Calculate similarity based on frequency ratio
            if (livePitchEstimate > 100.0f && masterPitchEstimate > 100.0f) {
                float freqRatio = std::min(livePitchEstimate, masterPitchEstimate)
                                  / std::max(livePitchEstimate, masterPitchEstimate);
                // Convert ratio to similarity score (closer to 1.0 = more similar)
                score.pitch = std::max(0.0f, std::min(1.0f, freqRatio * freqRatio));
            } else {
                score.pitch = 0.3f;  // Default for unclear pitch
            }
        } else {
            score.pitch = 0.5f;  // Neutral score when pitch analysis is disabled
        }

        // Calculate overall weighted score
        score.overall =
            impl_->calculateWeightedScore(score.mfcc, score.volume, score.timing, score.pitch);

        // Calculate confidence based on data quantity and quality
        const float signalQuality = std::min(1.0f, levelMeasurement.rmsLinear * 10.0f);
        score.confidence = calculateConfidence(impl_->totalSamplesProcessed_.load(),
                                               signalQuality,
                                               impl_->config_.minSamplesForConfidence);

        // Determine reliability and match status
        score.isReliable = score.confidence >= impl_->config_.confidenceThreshold;
        score.isMatch = score.overall >= impl_->config_.minScoreForMatch;

        // Update current score and history
        impl_->currentScore_ = score;

        // Update peak score
        if (score.overall > impl_->peakScore_.overall) {
            impl_->peakScore_ = score;
        }

        // Add to history
        impl_->scoringHistory_.push_front(score);
        while (impl_->scoringHistory_.size() > impl_->config_.scoringHistorySize) {
            impl_->scoringHistory_.pop_back();
        }

        // Update statistics
        impl_->totalSamplesProcessed_.fetch_add(frameCount);
        impl_->averageSignalLevel_.store(levelMeasurement.rmsLinear);
        impl_->lastUpdateTime_ = score.timestamp;

        return score;

    } catch (...) {
        return huntmaster::unexpected(Error::INTERNAL_ERROR);
    }
}

RealtimeScorer::SimilarityScore RealtimeScorer::getCurrentScore() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->currentScore_;
}

RealtimeScorer::FeedbackResult RealtimeScorer::getRealtimeFeedback() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);

        if (!impl_->hasMasterCall_) {
            return huntmaster::unexpected(Error::NO_MASTER_CALL);
        }

        RealtimeFeedback feedback;
        feedback.currentScore = impl_->currentScore_;
        feedback.peakScore = impl_->peakScore_;
        feedback.progressRatio = impl_->calculateProgressRatio();

        // Calculate trending score (average of recent scores)
        if (!impl_->scoringHistory_.empty()) {
            const size_t trendCount = std::min(size_t(5), impl_->scoringHistory_.size());
            float trendSum = 0.0f;

            for (size_t i = 0; i < trendCount; ++i) {
                trendSum += impl_->scoringHistory_[i].overall;
            }
            feedback.trendingScore.overall = trendSum / trendCount;
        }

        // Generate quality assessment and recommendations
        feedback.qualityAssessment = feedback.getQualityDescription(feedback.currentScore.overall);
        feedback.recommendation = impl_->generateRecommendation(feedback.currentScore);
        feedback.isImproving = impl_->isScoreTrendingUp();

        return feedback;

    } catch (...) {
        return huntmaster::unexpected(Error::INTERNAL_ERROR);
    }
}

std::vector<RealtimeScorer::SimilarityScore>
RealtimeScorer::getScoringHistory(size_t count) const noexcept {
    SCORER_LOG_DEBUG("getScoringHistory() called with count=" + std::to_string(count));
    try {
        SCORER_LOG_DEBUG("getScoringHistory() acquiring lock");
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        SCORER_LOG_DEBUG("getScoringHistory() lock acquired");

        std::vector<SimilarityScore> history;
        size_t numToCopy = std::min(count, impl_->scoringHistory_.size());
        SCORER_LOG_DEBUG("getScoringHistory() numToCopy=" + std::to_string(numToCopy)
                         + ", history size=" + std::to_string(impl_->scoringHistory_.size()));

        history.reserve(numToCopy);
        for (size_t i = 0; i < numToCopy; ++i) {
            history.push_back(impl_->scoringHistory_[i]);
        }

        SCORER_LOG_DEBUG("getScoringHistory() returning " + std::to_string(history.size())
                         + " items");
        return history;
    } catch (const std::exception& e) {
        SCORER_LOG_ERROR("getScoringHistory() exception: " + std::string(e.what()));
        return {};
    } catch (...) {
        SCORER_LOG_ERROR("getScoringHistory() unknown exception");
        return {};
    }
}

std::string RealtimeScorer::exportScoreToJson() const {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"overall\": " << impl_->currentScore_.overall << ",\n";
    ss << "  \"mfcc\": " << impl_->currentScore_.mfcc << ",\n";
    ss << "  \"volume\": " << impl_->currentScore_.volume << ",\n";
    ss << "  \"timing\": " << impl_->currentScore_.timing << ",\n";
    ss << "  \"pitch\": " << impl_->currentScore_.pitch << ",\n";
    ss << "  \"confidence\": " << impl_->currentScore_.confidence << ",\n";
    ss << "  \"isReliable\": " << (impl_->currentScore_.isReliable ? "true" : "false") << ",\n";
    ss << "  \"isMatch\": " << (impl_->currentScore_.isMatch ? "true" : "false") << ",\n";
    ss << "  \"samplesAnalyzed\": " << impl_->currentScore_.samplesAnalyzed << ",\n";
    ss << "  \"timestamp\": "
       << std::chrono::duration_cast<std::chrono::milliseconds>(
              impl_->currentScore_.timestamp.time_since_epoch())
              .count()
       << "\n";
    ss << "}";

    return ss.str();
}

std::string RealtimeScorer::exportFeedbackToJson() const {
    auto feedbackResult = getRealtimeFeedback();
    if (!feedbackResult.has_value()) {
        return "{\"error\": \"Failed to get feedback\"}";
    }
    auto& feedback = *feedbackResult;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{" << "\"currentScore\":" << exportScoreToJson() << ","
        << "\"trendingScore\":" << feedback.trendingScore.overall << ","
        << "\"peakScore\":" << feedback.peakScore.overall << ","
        << "\"progressRatio\":" << feedback.progressRatio << "," << "\"qualityAssessment\":\""
        << feedback.qualityAssessment << "\"," << "\"recommendation\":\"" << feedback.recommendation
        << "\"," << "\"isImproving\":" << (feedback.isImproving ? "true" : "false") << "}";

    return oss.str();
}

std::string RealtimeScorer::exportHistoryToJson(size_t maxCount) const {
    const auto history = getScoringHistory(maxCount);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[";

    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0)
            oss << ",";

        const auto& score = history[i];
        const auto epoch = score.timestamp.time_since_epoch();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

        oss << "{" << "\"overall\":" << score.overall << "," << "\"mfcc\":" << score.mfcc << ","
            << "\"volume\":" << score.volume << "," << "\"timing\":" << score.timing << ","
            << "\"pitch\":" << score.pitch << "," << "\"confidence\":" << score.confidence << ","
            << "\"timestamp\":" << millis << "}";
    }

    oss << "]";
    return oss.str();
}

void RealtimeScorer::reset() noexcept {
    SCORER_LOG_DEBUG("reset() called - acquiring lock");
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    SCORER_LOG_DEBUG("reset() lock acquired - clearing data structures");

    impl_->liveAudioBuffer_.clear();
    SCORER_LOG_DEBUG("reset() liveAudioBuffer_ cleared");

    impl_->liveMfccFeatures_.clear();
    SCORER_LOG_DEBUG("reset() liveMfccFeatures_ cleared");

    impl_->scoringHistory_.clear();
    SCORER_LOG_DEBUG("reset() scoringHistory_ cleared");

    impl_->liveAudioDuration_ = 0.0f;
    SCORER_LOG_DEBUG("reset() liveAudioDuration_ reset");

    impl_->currentScore_ = SimilarityScore{};
    SCORER_LOG_DEBUG("reset() currentScore_ reset");

    impl_->peakScore_ = SimilarityScore{};
    SCORER_LOG_DEBUG("reset() peakScore_ reset");

    impl_->totalSamplesProcessed_.store(0);
    SCORER_LOG_DEBUG("reset() totalSamplesProcessed_ reset");

    impl_->averageSignalLevel_.store(0.0f);
    SCORER_LOG_DEBUG("reset() averageSignalLevel_ reset");

    if (impl_->levelProcessor_) {
        SCORER_LOG_DEBUG("reset() calling levelProcessor_->reset()");
        impl_->levelProcessor_->reset();
        SCORER_LOG_DEBUG("reset() levelProcessor_->reset() completed");
    } else {
        SCORER_LOG_DEBUG("reset() levelProcessor_ is null, skipping");
    }

    impl_->sessionStartTime_ = std::chrono::steady_clock::now();
    impl_->lastUpdateTime_ = impl_->sessionStartTime_;
    SCORER_LOG_DEBUG("reset() timestamps updated - method completed");
}

void RealtimeScorer::resetSession() noexcept {
    SCORER_LOG_DEBUG("resetSession() called - acquiring lock");
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    SCORER_LOG_DEBUG("resetSession() lock acquired - performing reset operations");

    // Perform reset operations inline since we already hold the lock
    impl_->liveAudioBuffer_.clear();
    SCORER_LOG_DEBUG("resetSession() liveAudioBuffer_ cleared");

    impl_->liveMfccFeatures_.clear();
    SCORER_LOG_DEBUG("resetSession() liveMfccFeatures_ cleared");

    impl_->scoringHistory_.clear();
    SCORER_LOG_DEBUG("resetSession() scoringHistory_ cleared");

    impl_->liveAudioDuration_ = 0.0f;
    SCORER_LOG_DEBUG("resetSession() liveAudioDuration_ reset");

    impl_->currentScore_ = SimilarityScore{};
    SCORER_LOG_DEBUG("resetSession() currentScore_ reset");

    impl_->peakScore_ = SimilarityScore{};
    SCORER_LOG_DEBUG("resetSession() peakScore_ reset");

    impl_->totalSamplesProcessed_.store(0);
    SCORER_LOG_DEBUG("resetSession() totalSamplesProcessed_ reset");

    impl_->averageSignalLevel_.store(0.0f);
    SCORER_LOG_DEBUG("resetSession() averageSignalLevel_ reset");

    if (impl_->levelProcessor_) {
        SCORER_LOG_DEBUG("resetSession() calling levelProcessor_->reset()");
        impl_->levelProcessor_->reset();
        SCORER_LOG_DEBUG("resetSession() levelProcessor_->reset() completed");
    } else {
        SCORER_LOG_DEBUG("resetSession() levelProcessor_ is null, skipping");
    }

    impl_->sessionStartTime_ = std::chrono::steady_clock::now();
    impl_->lastUpdateTime_ = impl_->sessionStartTime_;
    SCORER_LOG_DEBUG("resetSession() timestamps updated");

    // Clear master call data
    impl_->masterMfccFeatures_.clear();
    SCORER_LOG_DEBUG("resetSession() masterMfccFeatures_ cleared");

    impl_->masterCallRms_ = 0.0f;
    SCORER_LOG_DEBUG("resetSession() masterCallRms_ reset");

    impl_->masterCallDuration_ = 0.0f;
    SCORER_LOG_DEBUG("resetSession() masterCallDuration_ reset");

    impl_->hasMasterCall_ = false;
    SCORER_LOG_DEBUG("resetSession() hasMasterCall_ set to false - method completed");
}

bool RealtimeScorer::updateConfig(const Config& newConfig) noexcept {
    if (!newConfig.isValid()) {
        return false;
    }

    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        impl_->config_ = newConfig;

        // Update component configurations if needed
        if (impl_->levelProcessor_) {
            AudioLevelProcessor::Config levelConfig;
            levelConfig.sampleRate = newConfig.sampleRate;
            levelConfig.updateRateMs = newConfig.updateRateMs;
            impl_->levelProcessor_->updateConfig(levelConfig);
        }

        return true;
    } catch (...) {
        return false;
    }
}

RealtimeScorer::Config RealtimeScorer::getConfig() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->config_;
}

bool RealtimeScorer::isInitialized() const noexcept {
    return impl_->initialized_.load();
}

bool RealtimeScorer::hasMasterCall() const noexcept {
    SCORER_LOG_DEBUG("hasMasterCall() called - acquiring lock");
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    SCORER_LOG_DEBUG("hasMasterCall() lock acquired - returning "
                     + std::to_string(impl_->hasMasterCall_));
    return impl_->hasMasterCall_;
}

float RealtimeScorer::getAnalysisProgress() const noexcept {
    SCORER_LOG_DEBUG("getAnalysisProgress() called - acquiring lock");
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    SCORER_LOG_DEBUG("getAnalysisProgress() lock acquired - calling calculateProgressRatio()");

    float progress = impl_->calculateProgressRatio();
    SCORER_LOG_DEBUG("getAnalysisProgress() progress=" + std::to_string(progress));

    return progress;
}

// Utility functions
}  // namespace huntmaster
