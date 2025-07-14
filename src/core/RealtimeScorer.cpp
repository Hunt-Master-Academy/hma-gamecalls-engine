#include "huntmaster/core/RealtimeScorer.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "huntmaster/core/AudioLevelProcessor.h"
#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/MFCCProcessor.h"

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

    explicit Impl(const Config& config) : config_(config) {
        if (config_.isValid()) {
            initializeComponents();
            sessionStartTime_ = std::chrono::steady_clock::now();
            lastUpdateTime_ = sessionStartTime_;
            initialized_.store(true);
        }
    }

    void initializeComponents() {
        // Initialize MFCC processor with appropriate settings
        MFCCProcessor::Config mfccConfig;
        mfccConfig.sample_rate = config_.sampleRate;
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

    float calculateWeightedScore(float mfcc, float volume, float timing, float pitch) const {
        return config_.mfccWeight * mfcc + config_.volumeWeight * volume +
               config_.timingWeight * timing + config_.pitchWeight * pitch;
    }

    float calculateProgressRatio() const {
        if (!hasMasterCall_ || masterCallDuration_ <= 0.0f) {
            return 0.0f;
        }

        return std::min(1.0f, liveAudioDuration_ / masterCallDuration_);
    }

    std::string generateRecommendation(const SimilarityScore& score) const {
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

    bool isScoreTrendingUp() const {
        if (scoringHistory_.size() < 3) return false;

        // Compare recent scores with older ones
        const size_t recentCount = std::min(size_t(3), scoringHistory_.size());
        const size_t olderCount = std::min(size_t(3), scoringHistory_.size() - recentCount);

        if (olderCount == 0) return false;

        float recentAvg = 0.0f;
        float olderAvg = 0.0f;

        for (size_t i = 0; i < recentCount; ++i) {
            recentAvg += scoringHistory_[i].overall;
        }
        recentAvg /= recentCount;

        for (size_t i = recentCount; i < recentCount + olderCount; ++i) {
            olderAvg += scoringHistory_[i].overall;
        }
        olderAvg /= olderCount;

        return recentAvg > olderAvg * 1.1f;  // 10% improvement threshold
    }
};

RealtimeScorer::RealtimeScorer(const Config& config) : impl_(std::make_unique<Impl>(config)) {}

bool RealtimeScorer::setMasterCall(const std::string& masterCallPath) noexcept {
    try {
        std::lock_guard<std::mutex> lock(impl_->mutex_);

        // Try to load as feature file first (.mfc)
        if (masterCallPath.ends_with(".mfc")) {
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

            // Calculate approximate RMS from MFCC energy (first coefficient)
            float energySum = 0.0f;
            for (const auto& frame : impl_->masterMfccFeatures_) {
                if (!frame.empty()) {
                    energySum += frame[0];  // First MFCC coefficient relates to energy
                }
            }
            impl_->masterCallRms_ = energySum / impl_->masterMfccFeatures_.size();

        } else {
            // Try to load as audio file and extract features
            // For now, return false as we focus on .mfc files
            // TODO: Implement audio file loading with dr_wav
            return false;
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

        for (size_t frame = 0; frame < frameCount; ++frame) {
            float frameSum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                frameSum += samples[frame * numChannels + ch];
            }
            monoSamples.push_back(frameSum / numChannels);
        }

        // Accumulate live audio for analysis
        impl_->liveAudioBuffer_.insert(impl_->liveAudioBuffer_.end(), monoSamples.begin(),
                                       monoSamples.end());

        // Update duration
        impl_->liveAudioDuration_ += static_cast<float>(frameCount) / impl_->config_.sampleRate;

        // Process audio through level processor for volume analysis
        auto levelResult = impl_->levelProcessor_->processAudio(monoSamples, 1);
        if (!levelResult.has_value()) {
            return huntmaster::unexpected(Error::COMPONENT_ERROR);
        }

        auto levelMeasurement = *levelResult;

        // Extract MFCC features if we have enough audio
        if (impl_->liveAudioBuffer_.size() >= 1024) {  // Minimum frame size
            auto mfccResult = impl_->mfccProcessor_->extractFeatures(impl_->liveAudioBuffer_);
            if (mfccResult.has_value()) {
                auto features = *mfccResult;
                if (!features.empty()) {
                    impl_->liveMfccFeatures_.push_back(std::move(features));
                }
            }
        }

        // Calculate similarity scores
        SimilarityScore score;
        score.timestamp = std::chrono::steady_clock::now();
        score.samplesAnalyzed = impl_->liveAudioBuffer_.size();

        // 1. MFCC Similarity (using DTW)
        if (!impl_->liveMfccFeatures_.empty() && !impl_->masterMfccFeatures_.empty()) {
            float dtwDistance = impl_->dtwComparator_->compare(impl_->liveMfccFeatures_,
                                                               impl_->masterMfccFeatures_);
            // Convert DTW distance to similarity (lower distance = higher similarity)
            score.mfcc = std::max(0.0f, 1.0f / (1.0f + dtwDistance * 100.0f));
        }

        // 2. Volume Similarity
        if (impl_->masterCallRms_ > 0.0f) {
            score.volume =
                calculateVolumeSimilarity(levelMeasurement.rmsLinear, impl_->masterCallRms_, 0.3f);
        }

        // 3. Timing Accuracy (based on feature alignment quality)
        if (!impl_->liveMfccFeatures_.empty() && !impl_->masterMfccFeatures_.empty()) {
            score.timing = calculateTimingAccuracy(std::span<const float>{},
                                                   std::span<const float>{});  // Simplified for now
        }

        // 4. Pitch Similarity (placeholder - not implemented yet)
        score.pitch = 0.5f;  // Neutral score until pitch analysis is implemented

        // Calculate overall weighted score
        score.overall =
            impl_->calculateWeightedScore(score.mfcc, score.volume, score.timing, score.pitch);

        // Calculate confidence based on data quantity and quality
        const float signalQuality = std::min(1.0f, levelMeasurement.rmsLinear * 10.0f);
        score.confidence = calculateConfidence(score.samplesAnalyzed, signalQuality);

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

            feedback.trendingScore = impl_->currentScore_;
            feedback.trendingScore.overall = trendSum / trendCount;
        }

        // Generate quality assessment and recommendations
        if (feedback.currentScore.overall >= impl_->config_.minScoreForMatch) {
            feedback.qualityAssessment = "Good match - keep practicing!";
        } else {
            feedback.qualityAssessment = "Keep working on your technique";
        }
        feedback.recommendation = impl_->generateRecommendation(feedback.currentScore);
        feedback.isImproving = impl_->isScoreTrendingUp();

        return feedback;

    } catch (...) {
        return huntmaster::unexpected(Error::INTERNAL_ERROR);
    }
}

std::vector<RealtimeScorer::SimilarityScore> RealtimeScorer::getScoringHistory(
    size_t maxCount) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    const size_t count = (maxCount > 0) ? std::min(maxCount, impl_->scoringHistory_.size())
                                        : impl_->scoringHistory_.size();

    std::vector<SimilarityScore> result;
    result.reserve(count);

    auto it = impl_->scoringHistory_.begin();
    for (size_t i = 0; i < count && it != impl_->scoringHistory_.end(); ++i, ++it) {
        result.push_back(*it);
    }

    return result;
}

std::string RealtimeScorer::exportScoreToJson() const {
    const auto score = getCurrentScore();

    const auto epoch = score.timestamp.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{"
        << "\"overall\":" << score.overall << ","
        << "\"mfcc\":" << score.mfcc << ","
        << "\"volume\":" << score.volume << ","
        << "\"timing\":" << score.timing << ","
        << "\"pitch\":" << score.pitch << ","
        << "\"confidence\":" << score.confidence << ","
        << "\"isReliable\":" << (score.isReliable ? "true" : "false") << ","
        << "\"isMatch\":" << (score.isMatch ? "true" : "false") << ","
        << "\"samplesAnalyzed\":" << score.samplesAnalyzed << ","
        << "\"timestamp\":" << millis << "}";

    return oss.str();
}

std::string RealtimeScorer::exportFeedbackToJson() const {
    auto feedbackResult = getRealtimeFeedback();
    if (!feedbackResult.has_value()) {
        return "{\"error\":\"No feedback available\"}";
    }

    const auto feedback = *feedbackResult;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{"
        << "\"currentScore\":" << exportScoreToJson() << ","
        << "\"trendingScore\":" << feedback.trendingScore.overall << ","
        << "\"peakScore\":" << feedback.peakScore.overall << ","
        << "\"progressRatio\":" << feedback.progressRatio << ","
        << "\"qualityAssessment\":\"" << feedback.qualityAssessment << "\","
        << "\"recommendation\":\"" << feedback.recommendation << "\","
        << "\"isImproving\":" << (feedback.isImproving ? "true" : "false") << "}";

    return oss.str();
}

std::string RealtimeScorer::exportHistoryToJson(size_t maxCount) const {
    const auto history = getScoringHistory(maxCount);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "[";

    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) oss << ",";

        const auto& score = history[i];
        const auto epoch = score.timestamp.time_since_epoch();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

        oss << "{"
            << "\"overall\":" << score.overall << ","
            << "\"mfcc\":" << score.mfcc << ","
            << "\"volume\":" << score.volume << ","
            << "\"timing\":" << score.timing << ","
            << "\"pitch\":" << score.pitch << ","
            << "\"confidence\":" << score.confidence << ","
            << "\"timestamp\":" << millis << "}";
    }

    oss << "]";
    return oss.str();
}

void RealtimeScorer::reset() noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->liveAudioBuffer_.clear();
    impl_->liveMfccFeatures_.clear();
    impl_->scoringHistory_.clear();
    impl_->liveAudioDuration_ = 0.0f;

    impl_->currentScore_ = SimilarityScore{};
    impl_->peakScore_ = SimilarityScore{};

    impl_->totalSamplesProcessed_.store(0);
    impl_->averageSignalLevel_.store(0.0f);

    if (impl_->levelProcessor_) {
        impl_->levelProcessor_->reset();
    }

    impl_->sessionStartTime_ = std::chrono::steady_clock::now();
    impl_->lastUpdateTime_ = impl_->sessionStartTime_;
}

void RealtimeScorer::resetSession() noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    reset();
    impl_->masterMfccFeatures_.clear();
    impl_->masterCallRms_ = 0.0f;
    impl_->masterCallDuration_ = 0.0f;
    impl_->hasMasterCall_ = false;
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

bool RealtimeScorer::isInitialized() const noexcept { return impl_->initialized_.load(); }

bool RealtimeScorer::hasMasterCall() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->hasMasterCall_;
}

float RealtimeScorer::getAnalysisProgress() const noexcept {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->calculateProgressRatio();
}

// Utility functions
float calculateConfidence(size_t samplesAnalyzed, float signalQuality, size_t minSamples) noexcept {
    if (samplesAnalyzed == 0) return 0.0f;

    // Data quantity factor (0.0-1.0)
    const float quantityFactor = std::min(1.0f, static_cast<float>(samplesAnalyzed) / minSamples);

    // Signal quality factor (0.0-1.0)
    const float qualityFactor = std::clamp(signalQuality, 0.0f, 1.0f);

    // Combined confidence with exponential growth for better user experience
    const float baseConfidence = quantityFactor * qualityFactor;
    return std::sqrt(baseConfidence);  // Square root for smoother progression
}

float calculateVolumeSimilarity(float liveRms, float masterRms, float tolerance) noexcept {
    if (masterRms <= 0.0f) return 0.0f;

    const float ratio = liveRms / masterRms;
    const float difference = std::abs(1.0f - ratio);

    if (difference <= tolerance) {
        return 1.0f - (difference / tolerance);
    } else {
        // Exponential decay for large differences
        return std::exp(-(difference - tolerance) * 2.0f);
    }
}

float calculateTimingAccuracy(std::span<const float> liveFeatures,
                              std::span<const float> masterFeatures) noexcept {
    // Simplified timing analysis - can be enhanced with more sophisticated algorithms
    // For now, return a placeholder that considers feature vector lengths

    if (liveFeatures.empty() || masterFeatures.empty()) {
        return 0.5f;  // Neutral score
    }

    const float lengthRatio = static_cast<float>(liveFeatures.size()) / masterFeatures.size();
    const float lengthSimilarity = 1.0f - std::abs(1.0f - lengthRatio);

    return std::clamp(lengthSimilarity, 0.0f, 1.0f);
}

}  // namespace huntmaster
