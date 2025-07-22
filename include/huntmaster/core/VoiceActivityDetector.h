/**
 * @file VoiceActivityDetector.h
 * @brief Voice Activity Detection for wildlife call analysis
 * 
 * This file provides the VoiceActivityDetector class which implements robust
 * voice activity detection specifically tuned for wildlife calls. The detector
 * identifies audio segments containing meaningful acoustic activity and filters
 * out background noise and silence periods.
 * 
 * @author Huntmaster Development Team
 * @date 2024
 * @copyright All Rights Reserved
 * @version 4.1
 */

#pragma once

#include <chrono>
#include <expected>
#include <memory>
#include <span>
#include <variant>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @enum VADError
 * @brief Error codes for Voice Activity Detection operations
 * 
 * Defines possible error states that can occur during VAD processing,
 * enabling proper error handling and debugging in client applications.
 */
enum class VADError { 
    INVALID_INPUT,      ///< Input audio data is invalid or corrupted
    PROCESSING_FAILED,  ///< Internal processing error occurred
    NOT_INITIALIZED     ///< VAD instance is not properly initialized
};

/**
 * @struct VADResult
 * @brief Result structure for voice activity detection analysis
 * 
 * Contains the results of voice activity detection processing, including
 * activity status, energy measurements, and timing information for the
 * analyzed audio window.
 */
struct VADResult {
    /** @brief True if voice activity was detected in the audio window */
    bool is_active{false};
    
    /** @brief Normalized energy level of the audio window [0.0, 1.0] */
    float energy_level{0.0f};
    
    /** @brief Duration of continuous activity if detected */
    std::chrono::milliseconds duration{0};
};

/**
 * @class VoiceActivityDetector
 * @brief Voice Activity Detection optimized for wildlife call analysis
 * 
 * The VoiceActivityDetector implements energy-based voice activity detection
 * with adaptive thresholding and temporal filtering. It's specifically tuned
 * for wildlife calls, which often have different characteristics than human
 * speech in terms of frequency content and temporal patterns.
 * 
 * Key features:
 * - Energy-based detection with configurable thresholds
 * - Temporal filtering to reduce false triggers
 * - Pre and post buffering for complete call capture
 * - Real-time processing with minimal latency
 * - Robust handling of background noise
 * 
 * @example
 * @code
 * // Configure VAD for wildlife calls
 * huntmaster::VoiceActivityDetector::Config config;
 * config.energy_threshold = 0.02f;        // Adjust for ambient noise
 * config.min_sound_duration = std::chrono::milliseconds(150);
 * config.sample_rate = 44100;
 * 
 * huntmaster::VoiceActivityDetector vad(config);
 * 
 * // Process audio windows
 * std::vector<float> audioWindow(512);
 * // ... fill audioWindow with audio data ...
 * 
 * auto result = vad.processWindow(audioWindow);
 * if (result) {
 *     if (result->is_active) {
 *         std::cout << "Activity detected! Energy: " << result->energy_level
 *                   << ", Duration: " << result->duration.count() << "ms" << std::endl;
 *     }
 * } else {
 *     std::cerr << "VAD processing error" << std::endl;
 * }
 * @endcode
 */
class VoiceActivityDetector {
  public:
    /**
     * @struct Config
     * @brief Configuration parameters for voice activity detection
     * 
     * This structure contains all parameters needed to configure the VAD
     * algorithm for optimal performance in different acoustic environments
     * and for various types of wildlife calls.
     */
    struct Config {
        /** @brief Energy threshold for voice activity detection [0.0, 1.0] */
        float energy_threshold{0.01f};
        
        /** @brief Duration of each analysis window */
        std::chrono::milliseconds window_duration{20};
        
        /** @brief Minimum duration required to consider sound as valid activity */
        std::chrono::milliseconds min_sound_duration{100};
        
        /** @brief Pre-buffer time to capture call onset */
        std::chrono::milliseconds pre_buffer{50};
        
        /** @brief Post-buffer time to capture call completion */
        std::chrono::milliseconds post_buffer{100};
        
        /** @brief Audio sample rate in Hz */
        size_t sample_rate{44100};
    };

    /**
     * @brief Construct a new Voice Activity Detector object
     * 
     * Creates a VAD instance with the specified configuration parameters.
     * The detector is immediately ready for processing audio windows.
     * 
     * @param config Configuration parameters for the VAD algorithm
     * 
     * @note The configuration cannot be changed after construction
     * @note All timing parameters are converted to sample counts based on sample_rate
     */
    explicit VoiceActivityDetector(const Config& config);
    
    /**
     * @brief Destroy the Voice Activity Detector object
     * 
     * Cleans up all internal resources and buffers used by the VAD algorithm.
     */
    ~VoiceActivityDetector();

    /**
     * @brief Move constructor
     * 
     * @param other VoiceActivityDetector to move from
     */
    VoiceActivityDetector(VoiceActivityDetector&&) noexcept;
    
    /**
     * @brief Move assignment operator
     * 
     * @param other VoiceActivityDetector to move from
     * @return Reference to this object
     */
    VoiceActivityDetector& operator=(VoiceActivityDetector&&) noexcept;

    /**
     * @brief Process an audio window for voice activity detection
     * 
     * Analyzes the provided audio window to determine if voice activity
     * is present. The algorithm considers energy levels, temporal patterns,
     * and configured thresholds to make the determination.
     * 
     * @param audio Audio samples to analyze (normalized float values)
     * @return VADResult containing detection results, or VADError on failure
     * 
     * @note Audio samples should be normalized to the range [-1.0, 1.0]
     * @note The window size should match the configured window_duration
     * @note Processing is stateful - previous windows affect current decisions
     * 
     * @example
     * @code
     * std::vector<float> audioWindow(512);
     * // ... fill with audio data ...
     * 
     * auto result = vad.processWindow(audioWindow);
     * if (result.has_value()) {
     *     std::cout << "Activity: " << (result->is_active ? "YES" : "NO") << std::endl;
     * }
     * @endcode
     */
    [[nodiscard]] huntmaster::expected<VADResult, VADError>
    processWindow(std::span<const float> audio);

    /**
     * @brief Reset the VAD state to initial conditions
     * 
     * Clears all internal state and buffers, returning the detector to
     * its initial state as if just constructed. This is useful when
     * switching to a new audio stream or after processing errors.
     * 
     * @note All accumulated timing and energy history is lost
     * @note Configuration parameters remain unchanged
     */
    void reset();

    /**
     * @brief Check if voice activity is currently detected
     * 
     * Returns the current voice activity status based on the most recent
     * audio window processing and temporal filtering.
     * 
     * @return true if voice activity is currently active, false otherwise
     * 
     * @note This reflects the filtered decision, not just the raw energy
     * @note Status may persist briefly after activity stops due to post-buffering
     */
    [[nodiscard]] bool isVoiceActive() const noexcept;
    
    /**
     * @brief Get the duration of current continuous voice activity
     * 
     * Returns the duration of the current continuous voice activity session.
     * This value is reset when activity stops and starts again.
     * 
     * @return Duration of current voice activity session
     * 
     * @note Returns 0ms when no activity is currently detected
     * @note Duration includes pre-buffer time when activity first starts
     */
    [[nodiscard]] std::chrono::milliseconds getActiveDuration() const noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster