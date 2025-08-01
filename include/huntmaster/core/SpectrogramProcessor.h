/**
 * @file SpectrogramProcessor.h
 * @brief STFT-based spectrogram generation for frequency visualization
 *
 * This file contains the SpectrogramProcessor class, which generates spectrograms
 * for real-time frequency analysis and visualization. It leverages the existing
 * KissFFT infrastructure from MFCCProcessor to compute Short-Time Fourier Transforms
 * and converts magnitude data to decibel representation for visualization platforms.
 *
 * @author Huntmaster Development Team
 * @version 1.0
 * @date 2025
 * @copyright All Rights Reserved - 3D Tech Solutions
 */

#pragma once

#include <array>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @enum SpectrogramError
 * @brief Error codes for spectrogram processing operations
 */
enum class SpectrogramError {
    INVALID_INPUT,     ///< Input audio data is invalid
    FFT_FAILED,        ///< FFT computation failed
    INVALID_CONFIG,    ///< Configuration parameters are invalid
    PROCESSING_FAILED  ///< General processing error
};

/**
 * @struct SpectrogramData
 * @brief Contains spectrogram analysis results for visualization
 */
struct SpectrogramData {
    std::vector<std::vector<float>> magnitude_db;  ///< 2D magnitude data in dB [time][frequency]
    std::vector<float> time_axis;                  ///< Time axis labels (seconds)
    std::vector<float> frequency_axis;             ///< Frequency axis labels (Hz)
    float min_db{-80.0f};                          ///< Minimum dB value for color mapping
    float max_db{0.0f};                            ///< Maximum dB value for color mapping
    size_t time_bins{0};                           ///< Number of time bins
    size_t frequency_bins{0};                      ///< Number of frequency bins
    float sample_rate{44100.0f};                   ///< Original sample rate
    float hop_size_seconds{0.0f};                  ///< Time step between frames
};

/**
 * @class SpectrogramProcessor
 * @brief High-performance STFT-based spectrogram generator
 *
 * The SpectrogramProcessor computes Short-Time Fourier Transforms on audio
 * signals and generates magnitude spectrograms suitable for visualization.
 * It reuses the FFT infrastructure from MFCCProcessor for efficiency.
 *
 * Key Features:
 * - Configurable window size and hop size
 * - Magnitude to decibel conversion
 * - JSON export for cross-platform visualization
 * - Real-time processing support
 * - Frequency range limiting for wildlife calls
 *
 * @example Basic Usage:
 * @code
 * SpectrogramProcessor::Config config;
 * config.window_size = 2048;
 * config.hop_size = 512;
 * config.sample_rate = 44100;
 *
 * auto processor = SpectrogramProcessor::create(config);
 * if (processor.isOk()) {
 *     std::vector<float> audio_data = loadAudioFile("call.wav");
 *     auto result = processor.value->computeSpectrogram(audio_data);
 *     if (result.isOk()) {
 *         std::string json = processor.value->exportForVisualization(result.value);
 *     }
 * }
 * @endcode
 */
class SpectrogramProcessor {
  public:
    /**
     * @struct Config
     * @brief Configuration parameters for spectrogram generation
     */
    struct Config {
        size_t window_size{2048};      ///< FFT window size (should be power of 2)
        size_t hop_size{512};          ///< Hop size between frames (typically window_size/4)
        float sample_rate{44100.0f};   ///< Audio sample rate in Hz
        float min_frequency{0.0f};     ///< Minimum frequency for display (Hz, 0 = auto)
        float max_frequency{8000.0f};  ///< Maximum frequency for display (Hz, 0 = nyquist)
        bool apply_window{true};       ///< Apply Hanning window to frames
        float db_floor{-80.0f};        ///< Minimum dB value (for visualization)
        float db_ceiling{0.0f};        ///< Maximum dB value (for normalization)

        /**
         * @brief Validate configuration parameters
         * @return true if config is valid, false otherwise
         */
        bool isValid() const noexcept;
    };

    /**
     * @brief Create a SpectrogramProcessor instance
     * @param config Configuration parameters
     * @return Result containing the processor or error
     */
    static huntmaster::expected<std::unique_ptr<SpectrogramProcessor>, SpectrogramError>
    create(const Config& config) noexcept;

    /**
     * @brief Destructor - cleans up FFT resources
     */
    ~SpectrogramProcessor();

    // Disable copy operations
    SpectrogramProcessor(const SpectrogramProcessor&) = delete;
    SpectrogramProcessor& operator=(const SpectrogramProcessor&) = delete;

    // Enable move operations
    SpectrogramProcessor(SpectrogramProcessor&&) noexcept;
    SpectrogramProcessor& operator=(SpectrogramProcessor&&) noexcept;

    /**
     * @brief Compute spectrogram from audio data
     * @param audio_data Input audio samples
     * @return Result containing spectrogram data or error
     */
    huntmaster::expected<SpectrogramData, SpectrogramError>
    computeSpectrogram(std::span<const float> audio_data) noexcept;

    /**
     * @brief Process a single audio frame for real-time spectrogram
     * @param audio_frame Single frame of audio data
     * @return Result containing magnitude spectrum in dB or error
     */
    huntmaster::expected<std::vector<float>, SpectrogramError>
    processFrame(std::span<const float> audio_frame) noexcept;

    /**
     * @brief Convert magnitude spectrum to decibel representation
     * @param magnitude_spectrum Linear magnitude values
     * @return Magnitude spectrum in decibels
     */
    static std::vector<float> magnitudeToDecibels(std::span<const float> magnitude_spectrum,
                                                  float floor_db = -80.0f) noexcept;

    /**
     * @brief Generate color mapping data for visualization
     * @param spectrogram_data Spectrogram magnitude data
     * @return Color values normalized to [0,1] range
     */
    static std::vector<std::vector<float>>
    generateColorMap(const SpectrogramData& spectrogram_data) noexcept;

    /**
     * @brief Export spectrogram data as JSON for visualization platforms
     * @param spectrogram_data Computed spectrogram data
     * @param max_time_bins Maximum time bins for display optimization (0 = no limit)
     * @param max_freq_bins Maximum frequency bins for display (0 = no limit)
     * @return JSON string containing visualization-ready data
     */
    std::string exportForVisualization(const SpectrogramData& spectrogram_data,
                                       size_t max_time_bins = 1000,
                                       size_t max_freq_bins = 512) const noexcept;

    /**
     * @brief Get current configuration
     * @return Current processor configuration
     */
    const Config& getConfig() const noexcept {
        return config_;
    }

    /**
     * @brief Reset processor state (clears any internal buffers)
     */
    void reset() noexcept;

  private:
    /**
     * @brief Private constructor - use create() instead
     * @param config Configuration parameters
     */
    explicit SpectrogramProcessor(const Config& config);

    /**
     * @brief Initialize FFT resources
     * @return true if successful, false otherwise
     */
    bool initializeFFT() noexcept;

    /**
     * @brief Apply windowing function to audio frame
     * @param frame Audio frame to window
     * @param windowed_frame Output windowed frame
     */
    void applyWindow(std::span<const float> frame, std::span<float> windowed_frame) const noexcept;

    /**
     * @brief Compute FFT magnitude spectrum
     * @param windowed_frame Windowed audio frame
     * @param magnitude_spectrum Output magnitude spectrum
     * @return true if successful, false on error
     */
    bool computeMagnitudeSpectrum(std::span<const float> windowed_frame,
                                  std::span<float> magnitude_spectrum) noexcept;

    // Forward declaration for PIMPL
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Config config_;
};

}  // namespace huntmaster
