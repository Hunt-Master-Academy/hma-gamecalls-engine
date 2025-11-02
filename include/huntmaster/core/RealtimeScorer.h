#pragma once

#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Expected.h"

namespace huntmaster {
// Forward declarations
class MFCCProcessor;
class DTWComparator;
class AudioLevelProcessor;

/**
 * @brief Real-time multi-dimensional similarity scorer with detailed feedback
 *
 * Provides comprehensive real-time similarity analysis combining MFCC pattern matching,
 * volume matching, pitch analysis, and timing accuracy. Designed for MVP integration
 * with progressive scoring and confidence intervals for enhanced user feedback.
 *
 * Key Features:
 * - Multi-dimensional scoring: MFCC + volume + timing + pitch
 * - Progressive confidence calculation with real-time updates
 * - Detailed score breakdown for user feedback
 * - JSON export for cross-platform compatibility
 * - Integration with existing engine components
 */
class RealtimeScorer {
  public:
    /// Configuration parameters for realtime scoring
    struct Config {
        float sampleRate = 44100.0f;       ///< Audio sample rate in Hz
        float updateRateMs = 100.0f;       ///< Score update rate in milliseconds
        float mfccWeight = 0.5f;           ///< Weight for MFCC similarity (0.0-1.0)
        float volumeWeight = 0.2f;         ///< Weight for volume matching (0.0-1.0)
        float timingWeight = 0.2f;         ///< Weight for timing accuracy (0.0-1.0)
        float pitchWeight = 0.1f;          ///< Weight for pitch similarity (0.0-1.0)
        float confidenceThreshold = 0.7f;  ///< Minimum confidence for reliable score
        float minScoreForMatch = 0.005f;   ///< Minimum similarity score for match
        bool enablePitchAnalysis = false;  ///< Enable pitch-based scoring (future feature)
        size_t scoringHistorySize = 50;    ///< Number of historical scores to retain
        // [20251101-FIX-034] Lower DTW scaling so identical audio maps to ~99% similarity
        // OLD: 100.0f gave ~87.5% for perfect matches (too conservative)
        // NEW: 10.0f gives ~98.6% for perfect matches (expected for self-similarity)
        float dtwDistanceScaling = 10.0f;        ///< Scaling factor for DTW distance to similarity
        size_t minSamplesForConfidence = 22050;  ///< Min samples for confident score (0.5s)

        /// Validate configuration parameters
        [[nodiscard]] bool isValid() const noexcept {
            const float totalWeight = mfccWeight + volumeWeight + timingWeight + pitchWeight;
            return sampleRate > 0.0f && updateRateMs > 0.0f && std::abs(totalWeight - 1.0f) < 0.01f
                   &&  // Weights should sum to 1.0 within a tolerance of 0.01
                   confidenceThreshold >= 0.0f && confidenceThreshold <= 1.0f
                   && minScoreForMatch >= 0.0f && scoringHistorySize > 0;
        }
    };

    /// Detailed similarity score breakdown
    struct SimilarityScore {
        float overall = 0.0f;        ///< Overall weighted similarity score
        float mfcc = 0.0f;           ///< MFCC pattern similarity
        float volume = 0.0f;         ///< Volume level matching
        float timing = 0.0f;         ///< Timing/rhythm accuracy
        float pitch = 0.0f;          ///< Pitch similarity (if enabled)
        float confidence = 0.0f;     ///< Confidence in the score (0.0-1.0)
        bool isReliable = false;     ///< Whether score meets confidence threshold
        bool isMatch = false;        ///< Whether score indicates a match
        size_t samplesAnalyzed = 0;  ///< Number of samples used for scoring
        std::chrono::steady_clock::time_point timestamp;  ///< Score timestamp

        SimilarityScore() : timestamp(std::chrono::steady_clock::now()) {}
    };

    /// Real-time feedback for user guidance
    struct RealtimeFeedback {
        SimilarityScore currentScore;   ///< Current similarity score
        SimilarityScore trendingScore;  ///< Trending average over recent history
        SimilarityScore peakScore;      ///< Best score achieved so far
        float progressRatio = 0.0f;     ///< Progress through master call (0.0-1.0)
        std::string qualityAssessment;  ///< Text description of match quality
        std::string recommendation;     ///< Suggestion for improvement
        bool isImproving = false;       ///< Whether score is trending upward

        /// Get quality assessment based on score
        [[nodiscard]] static std::string getQualityDescription(float score) noexcept {
            if (score >= 0.020f)
                return "Excellent match";
            if (score >= 0.010f)
                return "Very good match";
            if (score >= 0.005f)
                return "Good match";
            if (score >= 0.002f)
                return "Fair match";
            return "Needs improvement";
        }
    };

    /// Error types for RealtimeScorer
    enum class Error {
        INVALID_CONFIG,         ///< Invalid configuration parameters
        INVALID_AUDIO_DATA,     ///< Invalid audio data (null/empty)
        NO_MASTER_CALL,         ///< No master call loaded for comparison
        INSUFFICIENT_DATA,      ///< Not enough data for reliable scoring
        COMPONENT_ERROR,        ///< Error in underlying component (MFCC, DTW)
        INITIALIZATION_FAILED,  ///< Scorer initialization failed
        INTERNAL_ERROR          ///< Internal processing error
    };

    using Result = huntmaster::expected<SimilarityScore, Error>;
    using FeedbackResult = huntmaster::expected<RealtimeFeedback, Error>;

    /**
     * @brief Default constructor with default configuration
     */
    RealtimeScorer();

    /**
     * @brief Construct RealtimeScorer with configuration
     * @param config Scoring configuration parameters
     */
    explicit RealtimeScorer(const Config& config);

    /**
     * @brief Destructor
     */
    ~RealtimeScorer();

    // Non-copyable, movable
    RealtimeScorer(const RealtimeScorer&) = delete;
    RealtimeScorer& operator=(const RealtimeScorer&) = delete;
    RealtimeScorer(RealtimeScorer&&) noexcept;
    RealtimeScorer& operator=(RealtimeScorer&&) noexcept;

    /**
     * @brief Set master call for comparison
     *
     * Loads and prepares master call data for real-time comparison.
     *
     * @param masterCallPath Path to master call audio file or feature file
     * @return true if master call was loaded successfully
     */
    bool setMasterCall(const std::string& masterCallPath) noexcept;

    /**
     * @brief Process audio samples and calculate real-time similarity score
     *
     * Thread-safe method for processing incoming audio data and calculating
     * multi-dimensional similarity scores with detailed feedback.
     *
     * @param samples Audio samples to process (interleaved if multi-channel)
     * @param numChannels Number of audio channels (1=mono, 2=stereo)
     * @return Result containing similarity score breakdown or error
     */
    Result processAudio(std::span<const float> samples, int numChannels = 1) noexcept;

    /**
     * @brief Get current similarity score
     * @return Latest similarity score calculation (thread-safe)
     */
    [[nodiscard]] SimilarityScore getCurrentScore() const noexcept;

    /**
     * @brief Get comprehensive real-time feedback
     *
     * Provides detailed feedback including trending analysis, peak performance,
     * progress tracking, and improvement recommendations.
     *
     * @return Comprehensive real-time feedback or error
     */
    [[nodiscard]] FeedbackResult getRealtimeFeedback() const noexcept;

    /**
     * @brief Exports the current scoring history and feedback to a JSON string.
     * @return A string containing the JSON representation of the scores.
     */
    std::string exportScoresToJson() const;

    /**
     * @brief Retrieves the last N scores from the scoring history.
     * @param count The maximum number of scores to retrieve.
     * @return A vector of the most recent SimilarityScore objects.
     */
    std::vector<SimilarityScore> getScoringHistory(size_t count) const noexcept;

    /**
     * @brief Export current score as JSON string
     *
     * Provides standardized JSON format for cross-platform consumption:
     * {
     *   "overall": float,        // Overall weighted similarity score
     *   "mfcc": float,          // MFCC pattern similarity
     *   "volume": float,        // Volume level matching
     *   "timing": float,        // Timing/rhythm accuracy
     *   "pitch": float,         // Pitch similarity
     *   "confidence": float,    // Confidence in score (0.0-1.0)
     *   "isReliable": bool,     // Whether score meets confidence threshold
     *   "isMatch": bool,        // Whether score indicates a match
     *   "samplesAnalyzed": int, // Number of samples analyzed
     *   "timestamp": int64      // Unix timestamp in milliseconds
     * }
     *
     * @return JSON string representation of current similarity score
     */
    [[nodiscard]] std::string exportScoreToJson() const;

    /**
     * @brief Export real-time feedback as JSON string
     *
     * Provides comprehensive feedback in JSON format including trends,
     * recommendations, and progress information.
     *
     * @return JSON string representation of real-time feedback
     */
    [[nodiscard]] std::string exportFeedbackToJson() const;

    /**
     * @brief Export scoring history as JSON array
     * @param maxCount Maximum number of historical scores to export
     * @return JSON array string of similarity scores
     */
    [[nodiscard]] std::string exportHistoryToJson(size_t maxCount = 20) const;

    /**
     * @brief Reset scorer state
     *
     * Clears all scoring history and resets internal state while
     * preserving master call data. Thread-safe operation.
     */
    void reset() noexcept;

    /**
     * @brief Reset session completely
     *
     * Clears all data including master call and restarts scoring session.
     * Thread-safe operation.
     */
    void resetSession() noexcept;

    /**
     * @brief Update configuration parameters
     *
     * Updates scorer configuration and recalculates weighting factors.
     * Thread-safe operation.
     *
     * @param newConfig New configuration parameters
     * @return true if configuration was updated successfully
     */
    bool updateConfig(const Config& newConfig) noexcept;

    /**
     * @brief Get current configuration
     * @return Current scorer configuration
     */
    [[nodiscard]] Config getConfig() const noexcept;

    /**
     * @brief Check if scorer is properly initialized
     * @return true if scorer is ready for audio processing
     */
    [[nodiscard]] bool isInitialized() const noexcept;

    /**
     * @brief Check if master call is loaded and ready
     * @return true if master call is available for comparison
     */
    [[nodiscard]] bool hasMasterCall() const noexcept;

    /**
     * @brief Get progress through master call analysis
     *
     * Estimates how much of the master call has been analyzed based on
     * the current audio input duration.
     *
     * @return Progress ratio (0.0-1.0) or -1.0 if not applicable
     */
    [[nodiscard]] float getAnalysisProgress() const noexcept;

  private:
    /// Internal implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace huntmaster
