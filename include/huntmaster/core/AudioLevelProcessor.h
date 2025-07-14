#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @brief Real-time audio level processor for RMS, peak, and dB calculations
 *
 * Provides thread-safe, real-time audio level monitoring with configurable
 * smoothing, attack/release parameters, and circular buffer for level history.
 * Designed for MVP integration with platform-specific UI components.
 *
 * Key Features:
 * - Lock-free atomic operations for real-time safety
 * - Configurable smoothing and time constants
 * - dB conversion with proper headroom handling
 * - JSON export for cross-platform compatibility
 * - Efficient circular buffer for level history
 */
class AudioLevelProcessor {
   public:
    /// Configuration parameters for level processing
    struct Config {
        float sampleRate = 44100.0f;       ///< Audio sample rate in Hz
        float updateRateMs = 50.0f;        ///< Update rate in milliseconds (20 updates/sec)
        float rmsAttackTimeMs = 10.0f;     ///< RMS attack time constant in ms
        float rmsReleaseTimeMs = 100.0f;   ///< RMS release time constant in ms
        float peakAttackTimeMs = 1.0f;     ///< Peak attack time constant in ms
        float peakReleaseTimeMs = 300.0f;  ///< Peak release time constant in ms
        float dbFloor = -60.0f;            ///< Minimum dB level (silence floor)
        float dbCeiling = 6.0f;            ///< Maximum dB level (clipping threshold)
        size_t historySize = 100;          ///< Number of level measurements to retain

        /// Validate configuration parameters
        [[nodiscard]] bool isValid() const noexcept {
            return sampleRate > 0.0f && updateRateMs > 0.0f && rmsAttackTimeMs > 0.0f &&
                   rmsReleaseTimeMs > 0.0f && peakAttackTimeMs > 0.0f && peakReleaseTimeMs > 0.0f &&
                   dbFloor < dbCeiling && historySize > 0;
        }
    };

    /// Real-time audio level measurements
    struct LevelMeasurement {
        float rmsLinear = 0.0f;                           ///< RMS level (linear, 0.0-1.0)
        float rmsDb = -60.0f;                             ///< RMS level in dB
        float peakLinear = 0.0f;                          ///< Peak level (linear, 0.0-1.0)
        float peakDb = -60.0f;                            ///< Peak level in dB
        std::chrono::steady_clock::time_point timestamp;  ///< Measurement timestamp

        LevelMeasurement() : timestamp(std::chrono::steady_clock::now()) {}
    };

    /// Error types for AudioLevelProcessor
    enum class Error {
        INVALID_CONFIG,         ///< Invalid configuration parameters
        INVALID_AUDIO_DATA,     ///< Invalid audio data (null/empty)
        INSUFFICIENT_DATA,      ///< Not enough data for level calculation
        INITIALIZATION_FAILED,  ///< Processor initialization failed
        INTERNAL_ERROR          ///< Internal processing error
    };

    using Result = Expected<LevelMeasurement, Error>;

    /**
     * @brief Construct AudioLevelProcessor with configuration
     * @param config Processing configuration parameters
     */
    explicit AudioLevelProcessor(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~AudioLevelProcessor() = default;

    // Non-copyable, movable
    AudioLevelProcessor(const AudioLevelProcessor&) = delete;
    AudioLevelProcessor& operator=(const AudioLevelProcessor&) = delete;
    AudioLevelProcessor(AudioLevelProcessor&&) = default;
    AudioLevelProcessor& operator=(AudioLevelProcessor&&) = default;

    /**
     * @brief Process audio samples and update level measurements
     *
     * Thread-safe method for processing incoming audio data.
     * Updates internal level calculations using attack/release smoothing.
     *
     * @param samples Audio samples to process (interleaved if multi-channel)
     * @param numChannels Number of audio channels (1=mono, 2=stereo)
     * @return Result containing latest level measurement or error
     */
    Result processAudio(std::span<const float> samples, int numChannels = 1) noexcept;

    /**
     * @brief Get current level measurement
     * @return Latest level measurement (thread-safe)
     */
    [[nodiscard]] LevelMeasurement getCurrentLevel() const noexcept;

    /**
     * @brief Get level measurement history
     * @param maxCount Maximum number of historical measurements to return
     * @return Vector of historical level measurements (newest first)
     */
    [[nodiscard]] std::vector<LevelMeasurement> getLevelHistory(size_t maxCount = 0) const;

    /**
     * @brief Export current level data as JSON string
     *
     * Provides standardized JSON format for cross-platform consumption:
     * {
     *   "rms": float,      // RMS level in dB
     *   "peak": float,     // Peak level in dB
     *   "rmsLinear": float, // RMS level (linear 0.0-1.0)
     *   "peakLinear": float, // Peak level (linear 0.0-1.0)
     *   "timestamp": int64   // Unix timestamp in milliseconds
     * }
     *
     * @return JSON string representation of current levels
     */
    [[nodiscard]] std::string exportToJson() const;

    /**
     * @brief Export level history as JSON array
     * @param maxCount Maximum number of historical measurements to export
     * @return JSON array string of level measurements
     */
    [[nodiscard]] std::string exportHistoryToJson(size_t maxCount = 50) const;

    /**
     * @brief Reset processor state
     *
     * Clears all level history and resets smoothing filters.
     * Thread-safe operation.
     */
    void reset() noexcept;

    /**
     * @brief Update configuration parameters
     *
     * Updates processor configuration and recalculates smoothing coefficients.
     * Thread-safe operation.
     *
     * @param newConfig New configuration parameters
     * @return true if configuration was updated successfully
     */
    bool updateConfig(const Config& newConfig) noexcept;

    /**
     * @brief Get current configuration
     * @return Current processor configuration
     */
    [[nodiscard]] Config getConfig() const noexcept;

    /**
     * @brief Check if processor is properly initialized
     * @return true if processor is ready for audio processing
     */
    [[nodiscard]] bool isInitialized() const noexcept;

   private:
    /// Internal implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convert linear amplitude to dB with proper floor/ceiling handling
 * @param linear Linear amplitude (0.0-1.0+)
 * @param floor Minimum dB value (silence floor)
 * @param ceiling Maximum dB value (clipping threshold)
 * @return dB value clamped to [floor, ceiling] range
 */
[[nodiscard]] float linearToDb(float linear, float floor = -60.0f, float ceiling = 6.0f) noexcept;

/**
 * @brief Convert dB to linear amplitude
 * @param db dB value
 * @return Linear amplitude
 */
[[nodiscard]] float dbToLinear(float db) noexcept;

}  // namespace huntmaster
