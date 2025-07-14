#pragma once

#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @brief Real-time waveform generator for visualization data
 *
 * Provides decimated time-domain visualization data with configurable resolution,
 * peak-hold and RMS overlays for platform consumption. Designed for MVP integration
 * with efficient downsampling and real-time processing capabilities.
 *
 * Key Features:
 * - Configurable downsampling ratios for different zoom levels
 * - Peak-hold and RMS envelope generation
 * - Circular buffer for continuous waveform display
 * - JSON export for cross-platform compatibility
 * - Memory-efficient processing for real-time performance
 */
class WaveformGenerator {
   public:
    /// Configuration parameters for waveform generation
    struct Config {
        float sampleRate = 44100.0f;   ///< Audio sample rate in Hz
        size_t maxSamples = 8192;      ///< Maximum samples to store (buffer size)
        size_t downsampleRatio = 32;   ///< Downsampling ratio (samples per pixel)
        float updateRateMs = 50.0f;    ///< Update rate in milliseconds
        bool enablePeakHold = true;    ///< Enable peak-hold envelope generation
        bool enableRmsOverlay = true;  ///< Enable RMS envelope generation
        float rmsWindowMs = 10.0f;     ///< RMS calculation window in milliseconds
        bool normalizeOutput = true;   ///< Normalize output to [-1.0, 1.0] range

        /// Validate configuration parameters
        [[nodiscard]] bool isValid() const noexcept {
            return sampleRate > 0.0f && maxSamples > 0 && downsampleRatio > 0 &&
                   updateRateMs > 0.0f && rmsWindowMs > 0.0f;
        }
    };

    /// Waveform visualization data for a time segment
    struct WaveformData {
        std::vector<float> samples;      ///< Decimated audio samples
        std::vector<float> peaks;        ///< Peak envelope (if enabled)
        std::vector<float> rmsEnvelope;  ///< RMS envelope (if enabled)
        float maxAmplitude = 0.0f;       ///< Maximum amplitude in this segment
        float rmsAmplitude = 0.0f;       ///< RMS amplitude in this segment
        size_t originalSampleCount = 0;  ///< Number of original samples represented
        std::chrono::steady_clock::time_point timestamp;  ///< Data timestamp

        WaveformData() : timestamp(std::chrono::steady_clock::now()) {}
    };

    /// Error types for WaveformGenerator
    enum class Error {
        INVALID_CONFIG,         ///< Invalid configuration parameters
        INVALID_AUDIO_DATA,     ///< Invalid audio data (null/empty)
        BUFFER_OVERFLOW,        ///< Buffer capacity exceeded
        INSUFFICIENT_DATA,      ///< Not enough data for processing
        INITIALIZATION_FAILED,  ///< Generator initialization failed
        INTERNAL_ERROR          ///< Internal processing error
    };

    using Result = Expected<WaveformData, Error>;

    /**
     * @brief Construct WaveformGenerator with configuration
     * @param config Waveform generation configuration parameters
     */
    explicit WaveformGenerator(const Config& config = Config{});

    /**
     * @brief Destructor
     */
    ~WaveformGenerator() = default;

    // Non-copyable, movable
    WaveformGenerator(const WaveformGenerator&) = delete;
    WaveformGenerator& operator=(const WaveformGenerator&) = delete;
    WaveformGenerator(WaveformGenerator&&) = default;
    WaveformGenerator& operator=(WaveformGenerator&&) = default;

    /**
     * @brief Process audio samples and generate waveform visualization data
     *
     * Thread-safe method for processing incoming audio data and generating
     * downsampled visualization data with optional peak-hold and RMS overlays.
     *
     * @param samples Audio samples to process (interleaved if multi-channel)
     * @param numChannels Number of audio channels (1=mono, 2=stereo)
     * @return Result containing waveform visualization data or error
     */
    Result processAudio(std::span<const float> samples, int numChannels = 1) noexcept;

    /**
     * @brief Get the current complete waveform data buffer
     *
     * Returns the entire accumulated waveform data buffer for full display.
     * This is useful for drawing the complete visible waveform.
     *
     * @return Complete waveform data buffer (thread-safe)
     */
    [[nodiscard]] WaveformData getCompleteWaveform() const;

    /**
     * @brief Get waveform data for a specific time range
     *
     * @param startTimeMs Start time in milliseconds relative to buffer start
     * @param durationMs Duration in milliseconds
     * @return Waveform data for the specified time range
     */
    [[nodiscard]] WaveformData getWaveformRange(float startTimeMs, float durationMs) const;

    /**
     * @brief Export current waveform data as JSON string
     *
     * Provides standardized JSON format for cross-platform consumption:
     * {
     *   "samples": [float],        // Decimated audio samples
     *   "peaks": [float],          // Peak envelope (if enabled)
     *   "rms": [float],           // RMS envelope (if enabled)
     *   "maxAmplitude": float,     // Maximum amplitude
     *   "rmsAmplitude": float,     // RMS amplitude
     *   "sampleCount": int,        // Number of original samples
     *   "sampleRate": float,       // Sample rate
     *   "downsampleRatio": int,    // Downsampling ratio
     *   "timestamp": int64         // Unix timestamp in milliseconds
     * }
     *
     * @param includeRawSamples Whether to include raw sample data (can be large)
     * @return JSON string representation of waveform data
     */
    [[nodiscard]] std::string exportToJson(bool includeRawSamples = true) const;

    /**
     * @brief Export waveform data optimized for specific display resolution
     *
     * Generates waveform data optimized for a specific pixel width, ensuring
     * efficient data transfer and rendering performance.
     *
     * @param displayWidthPixels Target display width in pixels
     * @param includeEnvelopes Whether to include peak/RMS envelopes
     * @return JSON string optimized for the specified display resolution
     */
    [[nodiscard]] std::string exportForDisplay(size_t displayWidthPixels,
                                               bool includeEnvelopes = true) const;

    /**
     * @brief Clear all accumulated waveform data
     *
     * Resets the internal buffer and starts fresh data accumulation.
     * Thread-safe operation.
     */
    void reset() noexcept;

    /**
     * @brief Update configuration parameters
     *
     * Updates generator configuration and adjusts internal buffers.
     * Thread-safe operation.
     *
     * @param newConfig New configuration parameters
     * @return true if configuration was updated successfully
     */
    bool updateConfig(const Config& newConfig) noexcept;

    /**
     * @brief Get current configuration
     * @return Current generator configuration
     */
    [[nodiscard]] Config getConfig() const noexcept;

    /**
     * @brief Check if generator is properly initialized
     * @return true if generator is ready for audio processing
     */
    [[nodiscard]] bool isInitialized() const noexcept;

    /**
     * @brief Get buffer utilization statistics
     *
     * @return Pair of (used samples, total capacity)
     */
    [[nodiscard]] std::pair<size_t, size_t> getBufferStats() const noexcept;

    /**
     * @brief Set zoom level for dynamic resolution adjustment
     *
     * Adjusts the downsampling ratio dynamically for different zoom levels.
     * Higher zoom = more detail = lower downsampling ratio.
     *
     * @param zoomLevel Zoom level (1.0 = normal, 2.0 = 2x zoom, 0.5 = 2x zoom out)
     */
    void setZoomLevel(float zoomLevel) noexcept;

   private:
    /// Internal implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Calculate optimal downsampling ratio for display resolution
 *
 * Helper function to calculate the best downsampling ratio based on
 * available data and desired display resolution.
 *
 * @param totalSamples Total number of audio samples available
 * @param displayWidthPixels Target display width in pixels
 * @param sampleRate Audio sample rate
 * @return Optimal downsampling ratio
 */
[[nodiscard]] size_t calculateOptimalDownsampleRatio(size_t totalSamples, size_t displayWidthPixels,
                                                     float sampleRate) noexcept;

/**
 * @brief Apply peak-hold algorithm to sample data
 *
 * Generates peak envelope data using peak-hold algorithm with configurable
 * attack and decay characteristics.
 *
 * @param samples Input audio samples
 * @param windowSize Peak detection window size
 * @return Peak envelope data
 */
[[nodiscard]] std::vector<float> generatePeakEnvelope(std::span<const float> samples,
                                                      size_t windowSize) noexcept;

/**
 * @brief Calculate RMS envelope for sample data
 *
 * Generates RMS envelope data using sliding window RMS calculation.
 *
 * @param samples Input audio samples
 * @param windowSize RMS calculation window size
 * @return RMS envelope data
 */
[[nodiscard]] std::vector<float> generateRmsEnvelope(std::span<const float> samples,
                                                     size_t windowSize) noexcept;

}  // namespace huntmaster
