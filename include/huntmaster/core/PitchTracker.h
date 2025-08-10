#pragma once

#include <memory>
#include <span>
#include <vector>

#include "huntmaster/core/Expected.h"

namespace huntmaster {

/**
 * @brief YIN algorithm-based pitch detection and contour tracking
 *
 * Advanced pitch tracker implementing the YIN algorithm for fundamental
 * frequency detection with confidence scoring and pitch smoothing.
 * Designed for real-time wildlife call analysis.
 *
 * Features:
 * - YIN algorithm implementation for robust pitch detection
 * - Real-time pitch estimation with confidence scoring
 * - Pitch contour smoothing and octave correction
 * - Vibrato and pitch modulation analysis
 * - JSON export for cross-platform compatibility
 * - Integration with existing audio pipeline
 */
class PitchTracker {
  public:
    /**
     * @brief Convenience alias for expected type
     */
    template <typename T, typename E>
    using Result = expected<T, E>;

    /**
     * @brief Pitch detection result with detailed analysis
     */
    struct PitchResult {
        float frequency = 0.0f;      ///< Fundamental frequency in Hz
        float confidence = 0.0f;     ///< 0-1 confidence score
        std::vector<float> contour;  ///< Pitch over time

        struct Vibrato {
            float rate = 0.0f;        ///< Vibrato rate in Hz
            float extent = 0.0f;      ///< Vibrato extent in semitones
            float regularity = 0.0f;  ///< Vibrato regularity (0-1)
        } vibrato;

        struct PitchStatistics {
            float mean = 0.0f;               ///< Mean pitch in Hz
            float standardDeviation = 0.0f;  ///< Pitch variance
            float range = 0.0f;              ///< Pitch range (max - min)
            float stability = 0.0f;          ///< Pitch stability (0-1)
        } statistics;

        bool isVoiced = false;   ///< Whether the signal is voiced
        float timestamp = 0.0f;  ///< Timestamp of analysis
    };

    /**
     * @brief Configuration for pitch tracking
     */
    struct Config {
        float sampleRate = 44100.0f;         ///< Audio sample rate in Hz
        float minFrequency = 80.0f;          ///< Minimum detectable frequency
        float maxFrequency = 8000.0f;        ///< Maximum detectable frequency
        float threshold = 0.2f;              ///< YIN threshold (0.1-0.5)
        size_t windowSize = 2048;            ///< Analysis window size
        size_t hopSize = 512;                ///< Hop size between windows
        bool enableSmoothing = true;         ///< Enable pitch contour smoothing
        bool enableVibratoDetection = true;  ///< Enable vibrato analysis
        float smoothingFactor = 0.1f;        ///< Smoothing factor (0-1)
    };

    /**
     * @brief Error codes for pitch tracking operations
     */
    enum class Error {
        OK = 0,
        INVALID_AUDIO_DATA,
        INSUFFICIENT_DATA,
        INVALID_SAMPLE_RATE,
        INVALID_WINDOW_SIZE,
        INITIALIZATION_FAILED,
        PROCESSING_ERROR
    };

    /**
     * @brief Factory method for creating PitchTracker instances
     * @param config Configuration parameters
     * @return Result containing PitchTracker instance or error
     */
    static Result<std::unique_ptr<PitchTracker>, Error> create(const Config& config);

    /**
     * @brief Virtual destructor
     */
    virtual ~PitchTracker() = default;

    /**
     * @brief Detect pitch in audio buffer using YIN algorithm
     * @param audio Audio samples to analyze
     * @return Result containing pitch analysis or error
     */
    virtual Result<PitchResult, Error> detectPitch(std::span<const float> audio) = 0;

    /**
     * @brief Get real-time pitch estimate from ongoing analysis
     * @return Current pitch estimate in Hz (0 if no pitch detected)
     */
    virtual Result<float, Error> getRealtimePitch() = 0;

    /**
     * @brief Get real-time pitch confidence (0-1)
     * @return Current confidence score for last detected pitch
     */
    virtual Result<float, Error> getRealtimeConfidence() = 0;

    /**
     * @brief Process audio chunk for continuous pitch tracking
     * @param audio Audio samples to process
     * @return Processing status
     */
    virtual Result<void, Error> processAudioChunk(std::span<const float> audio) = 0;

    /**
     * @brief Get pitch contour from recent analysis
     * @param durationMs Duration of contour to retrieve in milliseconds
     * @return Vector of pitch values over time
     */
    virtual Result<std::vector<float>, Error> getPitchContour(float durationMs = 1000.0f) = 0;

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
     * @brief Export pitch analysis as JSON string
     * @param result Pitch result to export
     * @return JSON string representation
     */
    static std::string exportToJson(const PitchResult& result);

    /**
     * @brief Check if pitch tracking is currently active
     * @return True if actively tracking pitch
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get processing statistics
     * @return Statistics about recent processing performance
     */
    virtual std::string getProcessingStats() const = 0;

  protected:
    /**
     * @brief Protected constructor for factory pattern
     */
    PitchTracker() = default;

    /**
     * @brief Copy constructor deleted
     */
    PitchTracker(const PitchTracker&) = delete;

    /**
     * @brief Assignment operator deleted
     */
    PitchTracker& operator=(const PitchTracker&) = delete;
};

}  // namespace huntmaster
