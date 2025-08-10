#pragma once

#include <memory>
#include <span>
#include <vector>

#include "huntmaster/core/Expected.h"

namespace huntmaster {

/**
 * @brief Cadence and rhythm pattern analysis for wildlife call assessment
 *
 * Advanced temporal analyzer for extracting rhythm patterns, timing characteristics,
 * and cadence features from wildlife calls. Provides detailed analysis of call
 * sequences, inter-call intervals, and rhythmic structures.
 *
 * Features:
 * - Beat detection and tempo estimation
 * - Inter-call interval analysis
 * - Rhythmic pattern recognition
 * - Call sequence structure analysis
 * - Syllable segmentation and timing
 * - Periodicity detection and quantification
 * - JSON export for cross-platform compatibility
 * - Integration with existing audio pipeline
 */
class CadenceAnalyzer {
  public:
    /**
     * @brief Convenience alias for expected type
     */
    template <typename T, typename E>
    using Result = expected<T, E>;

    /**
     * @brief Cadence profile with detailed temporal analysis
     */
    struct CadenceProfile {
        float estimatedTempo = 0.0f;   ///< Estimated tempo in BPM
        float tempoConfidence = 0.0f;  ///< Tempo estimation confidence (0-1)

        std::vector<float> beatTimes;           ///< Detected beat timestamps
        std::vector<float> beatStrengths;       ///< Beat detection strengths
        std::vector<float> interBeatIntervals;  ///< Time between beats in seconds

        struct CallSequence {
            std::vector<float> callOnsets;          ///< Call onset times in seconds
            std::vector<float> callDurations;       ///< Call durations in seconds
            std::vector<float> interCallIntervals;  ///< Silence between calls
            float sequenceDuration = 0.0f;          ///< Total sequence duration
            size_t numCalls = 0;                    ///< Number of detected calls
            float callRate = 0.0f;                  ///< Calls per second
        } sequence;

        struct PeriodicityMeasures {
            float autocorrelationPeak = 0.0f;         ///< Peak autocorrelation value
            float periodicityStrength = 0.0f;         ///< Overall periodicity (0-1)
            float dominantPeriod = 0.0f;              ///< Dominant period in seconds
            std::vector<float> periodicities;         ///< Multiple period candidates
            std::vector<float> periodicityStrengths;  ///< Strengths of each period
        } periodicity;

        struct RhythmicFeatures {
            float rhythmComplexity = 0.0f;  ///< Complexity measure (0-1)
            float rhythmRegularity = 0.0f;  ///< Regularity measure (0-1)
            float syncopation = 0.0f;       ///< Syncopation index (0-1)
            float polyrhythm = 0.0f;        ///< Multi-rhythm detection (0-1)
            float groove = 0.0f;            ///< Groove/swing factor (0-1)
        } rhythm;

        struct SyllableAnalysis {
            std::vector<float> syllableOnsets;     ///< Syllable start times
            std::vector<float> syllableDurations;  ///< Syllable durations
            float avgSyllableDuration = 0.0f;      ///< Average syllable length
            float syllableRate = 0.0f;             ///< Syllables per second
            float speechRhythm = 0.0f;             ///< Speech-like rhythm score
        } syllables;

        float overallRhythmScore = 0.0f;  ///< Combined rhythm assessment
        float confidence = 0.0f;          ///< Analysis confidence (0-1)
        float timestamp = 0.0f;           ///< Analysis timestamp
        bool hasStrongRhythm = false;     ///< Whether strong rhythm detected
    };

    /**
     * @brief Configuration for cadence analysis
     */
    struct Config {
        float sampleRate = 44100.0f;         ///< Audio sample rate in Hz
        float frameSize = 0.025f;            ///< Analysis frame size in seconds
        float hopSize = 0.010f;              ///< Hop size in seconds
        float minTempo = 30.0f;              ///< Minimum tempo in BPM
        float maxTempo = 300.0f;             ///< Maximum tempo in BPM
        float minPeriod = 0.1f;              ///< Minimum period in seconds
        float maxPeriod = 5.0f;              ///< Maximum period in seconds
        float onsetThreshold = 0.3f;         ///< Onset detection threshold
        float silenceThreshold = -30.0f;     ///< Silence threshold in dB
        size_t autocorrelationLags = 1000;   ///< Number of autocorr lags
        bool enableBeatTracking = true;      ///< Enable beat detection
        bool enableOnsetDetection = true;    ///< Enable onset detection
        bool enableSyllableAnalysis = true;  ///< Enable syllable analysis
        float adaptiveThreshold = 0.1f;      ///< Adaptive threshold factor
        bool fastPathOptimization = false;   ///< Enable simplified fast-path (reduced feature cost)
        bool forceFullAutocorr = false;      ///< Force full autocorrelation (disable adaptive caps)
    };

    /**
     * @brief Error codes for cadence analysis operations
     */
    enum class Error {
        OK = 0,
        INVALID_AUDIO_DATA,
        INSUFFICIENT_DATA,
        INVALID_SAMPLE_RATE,
        INVALID_FRAME_SIZE,
        INITIALIZATION_FAILED,
        PROCESSING_ERROR,
        ONSET_DETECTION_ERROR
    };

    /**
     * @brief Factory method for creating CadenceAnalyzer instances
     * @param config Configuration parameters
     * @return Result containing CadenceAnalyzer instance or error
     */
    static Result<std::unique_ptr<CadenceAnalyzer>, Error> create(const Config& config);

    /**
     * @brief Virtual destructor
     */
    virtual ~CadenceAnalyzer() = default;

    /**
     * @brief Analyze cadence and rhythm in audio buffer
     * @param audio Audio samples to analyze
     * @return Result containing cadence analysis or error
     */
    virtual Result<CadenceProfile, Error> analyzeCadence(std::span<const float> audio) = 0;

    /**
     * @brief Process audio chunk for continuous cadence tracking
     * @param audio Audio samples to process
     * @return Processing status
     */
    virtual Result<void, Error> processAudioChunk(std::span<const float> audio) = 0;

    /**
     * @brief Get current cadence analysis
     * @return Most recent cadence profile
     */
    virtual Result<CadenceProfile, Error> getCurrentAnalysis() = 0;

    /**
     * @brief Detect onsets in audio buffer
     * @param audio Audio samples to analyze
     * @return Vector of onset times in seconds
     */
    virtual Result<std::vector<float>, Error> detectOnsets(std::span<const float> audio) = 0;

    /**
     * @brief Estimate tempo from audio
     * @param audio Audio samples to analyze
     * @return Tempo in BPM and confidence
     */
    virtual Result<std::pair<float, float>, Error> estimateTempo(std::span<const float> audio) = 0;

    /**
     * @brief Analyze periodicity in audio signal
     * @param audio Audio samples to analyze
     * @return Periodicity measures
     */
    virtual Result<CadenceProfile::PeriodicityMeasures, Error>
    analyzePerodicity(std::span<const float> audio) = 0;

    /**
     * @brief Extract rhythmic features
     * @param onsets Vector of onset times
     * @return Rhythmic feature analysis
     */
    virtual Result<CadenceProfile::RhythmicFeatures, Error>
    extractRhythmicFeatures(const std::vector<float>& onsets) = 0;

    /**
     * @brief Reset internal state for new analysis
     */
    virtual void reset() = 0;

    /**
     * @brief Update configuration parameters
     * @param config New configuration
     * @return Success status
     */
    virtual Result<void, Error> updateConfig(const Config& config) = 0;

    /**
     * @brief Get current configuration
     * @return Current configuration parameters
     */
    virtual const Config& getConfig() const = 0;

    /**
     * @brief Export cadence analysis as JSON string
     * @param profile Cadence profile to export
     * @return JSON string representation
     */
    static std::string exportToJson(const CadenceProfile& profile);

    /**
     * @brief Check if cadence analysis is currently active
     * @return True if actively analyzing cadence
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get processing statistics
     * @return Statistics about recent processing performance
     */
    virtual std::string getProcessingStats() const = 0;

    /**
     * @brief Get current onset detection function
     * @return Current onset strength values
     */
    virtual Result<std::vector<float>, Error> getOnsetDetectionFunction() = 0;

    /**
     * @brief Get current beat tracking state
     * @return Beat tracking information
     */
    virtual Result<std::vector<float>, Error> getBeatTrackingState() = 0;

  protected:
    /**
     * @brief Protected constructor for factory pattern
     */
    CadenceAnalyzer() = default;

    /**
     * @brief Copy constructor deleted
     */
    CadenceAnalyzer(const CadenceAnalyzer&) = delete;

    /**
     * @brief Assignment operator deleted
     */
    CadenceAnalyzer& operator=(const CadenceAnalyzer&) = delete;
};

}  // namespace huntmaster
