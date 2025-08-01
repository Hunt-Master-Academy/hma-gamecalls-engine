/**
 * @file MFCCProcessor.h
 * @brief Mel-Frequency Cepstral Coefficients (MFCC) feature extraction processor
 *
 * This file contains the MFCC processor class, which is responsible for extracting
 * MFCC features from audio signals. MFCC is a widely-used technique in audio
 * analysis and speech recognition that represents the short-term power spectrum
 * of audio signals in a compact and perceptually meaningful way.
 *
 * @author Huntmaster Development Team
 * @version 4.1
 * @date 2025
 * @copyright All Rights Reserved - 3D Tech Solutions
 */

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @enum MFCCError
 * @brief Error codes for MFCC processing operations
 *
 * Defines specific error conditions that can occur during MFCC feature
 * extraction, allowing for precise error handling and debugging.
 */
enum class MFCCError {
    INVALID_INPUT,     ///< Input audio data is invalid (null, empty, or malformed)
    FFT_FAILED,        ///< Fast Fourier Transform computation failed
    INVALID_CONFIG,    ///< Configuration parameters are invalid or incompatible
    PROCESSING_FAILED  ///< General processing error during feature extraction
};

/**
 * @class MFCCProcessor
 * @brief High-performance MFCC feature extractor for audio analysis
 *
 * The MFCCProcessor transforms raw audio signals into Mel-Frequency Cepstral
 * Coefficients, which are compact representations of the spectral characteristics
 * of audio signals. This class is optimized for real-time processing and supports
 * various configuration options for different use cases.
 *
 * Key Features:
 * - Configurable number of coefficients and mel filters
 * - SIMD optimizations for performance
 * - Built-in caching for repeated computations
 * - Support for various audio sample rates
 * - Energy and liftering options
 *
 * @note This processor is thread-safe for read operations but not for
 *       configuration changes or cache management.
 *
 * @example Basic Usage:
 * @code
 * MFCCProcessor::Config config;
 * config.sample_rate = 44100;
 * config.num_coefficients = 13;
 *
 * MFCCProcessor processor(config);
 *
 * std::vector<float> audio_frame(512);
 * // ... fill audio_frame with data ...
 *
 * auto result = processor.extractFeatures(audio_frame);
 * if (result.has_value()) {
 *     const auto& mfcc_features = result.value();
 *     // Use the extracted MFCC features...
 * }
 * @endcode
 */
class MFCCProcessor {
  public:
    /**
     * @struct Config
     * @brief Configuration parameters for MFCC feature extraction
     *
     * This structure contains all parameters needed to configure the MFCC
     * extraction process. Default values are suitable for general wildlife
     * call analysis, but can be adjusted for specific requirements.
     */
    struct Config {
        size_t sample_rate{44100};    ///< Audio sample rate in Hz
        size_t frame_size{512};       ///< Analysis frame size in samples (should be power of 2)
        size_t num_coefficients{13};  ///< Number of MFCC coefficients to extract (typically 12-13)
        size_t num_filters{26};       ///< Number of mel filter banks (typically 20-40)
        float low_freq{0.0f};         ///< Lowest frequency for mel filter bank (Hz, 0 = auto)
        float high_freq{0.0f};    ///< Highest frequency for mel filter bank (Hz, 0 = sample_rate/2)
        bool use_energy{true};    ///< Include energy as the 0th coefficient
        bool apply_lifter{true};  ///< Apply liftering to enhance higher coefficients
        size_t lifter_coeff{22};  ///< Liftering coefficient (higher = more emphasis on high coeffs)
        bool enable_simd{true};   ///< Enable SIMD optimizations for performance
        bool enable_caching{true};  ///< Enable caching of intermediate computations
    };

    /**
     * @typedef FeatureVector
     * @brief Single frame of MFCC features
     *
     * Represents the MFCC coefficients extracted from one audio frame.
     * Typically contains 12-13 coefficients plus optional energy.
     */
    using FeatureVector = std::vector<float>;

    /**
     * @typedef FeatureMatrix
     * @brief Multiple frames of MFCC features arranged as a matrix
     *
     * Each row represents one time frame, and each column represents
     * one MFCC coefficient across time.
     */
    using FeatureMatrix = std::vector<FeatureVector>;

    /**
     * @brief Construct MFCC processor with specified configuration
     *
     * Initializes the processor with the given configuration parameters.
     * This includes setting up mel filter banks, FFT plans, and other
     * internal structures required for feature extraction.
     *
     * @param config Configuration parameters for MFCC extraction
     *
     * @throws std::invalid_argument if configuration parameters are invalid
     * @note Construction may be expensive due to FFT plan creation and
     *       filter bank computation. Consider reusing processor instances.
     */
    explicit MFCCProcessor(const Config& config);

    /**
     * @brief Destructor - cleans up FFT resources and cached data
     */
    ~MFCCProcessor();

    /**
     * @brief Move constructor
     * @param other Source processor to move from
     */
    MFCCProcessor(MFCCProcessor&& other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Source processor to move from
     * @return Reference to this processor
     */
    MFCCProcessor& operator=(MFCCProcessor&& other) noexcept;

    /**
     * @brief Extract MFCC features from a single audio frame
     *
     * Processes one frame of audio data and extracts MFCC coefficients.
     * The frame size must match the configured frame_size parameter.
     *
     * @param audio_frame Audio samples for one frame (32-bit float, normalized)
     * @return Expected containing feature vector on success, or MFCCError on failure
     *
     * @note Input audio should be normalized to the range [-1.0, 1.0] for
     *       optimal results. Frame size must exactly match the configured size.
     */
    [[nodiscard]] huntmaster::expected<FeatureVector, MFCCError>
    extractFeatures(std::span<const float> audio_frame);

    /**
     * @brief Extract MFCC features from a longer audio buffer with overlapping frames
     *
     * Processes a buffer of audio data by extracting features from overlapping
     * frames. This is more efficient than calling extractFeatures() repeatedly
     * for consecutive frames.
     *
     * @param audio_buffer Complete audio buffer to process
     * @param hop_size Number of samples to advance between frames
     * @return Expected containing feature matrix on success, or MFCCError on failure
     *
     * @note The hop_size determines frame overlap. A hop_size equal to frame_size
     *       means no overlap, while smaller values create overlap between frames.
     */
    [[nodiscard]] huntmaster::expected<FeatureMatrix, MFCCError>
    extractFeaturesFromBuffer(std::span<const float> audio_buffer, size_t hop_size);

    /**
     * @brief Clear all cached intermediate computations
     *
     * Frees memory used by cached mel filter bank responses and other
     * intermediate data. This can help reduce memory usage but may
     * impact performance for subsequent extractions.
     */
    void clearCache();

    /**
     * @brief Get the current size of the internal cache
     *
     * @return Number of cached entries (useful for memory monitoring)
     */
    [[nodiscard]] size_t getCacheSize() const noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster
