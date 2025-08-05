#pragma once

#include <memory>
#include <span>
#include <vector>

#include "huntmaster/core/Expected.h"

namespace huntmaster {

/**
 * @brief Harmonic and tonal quality analysis for wildlife call assessment
 *
 * Advanced spectral analyzer for extracting tonal characteristics and
 * harmonic content from wildlife calls. Provides detailed analysis of
 * spectral properties, formant detection, and tonal quality assessment.
 *
 * Features:
 * - Spectral centroid and spread calculation
 * - Harmonic ratio analysis with fundamental frequency detection
 * - Formant frequency extraction and tracking
 * - Tonal quality assessment (rasp, whine, resonance)
 * - Spectral rolloff and flatness measures
 * - JSON export for cross-platform compatibility
 * - Integration with existing audio pipeline
 */
class HarmonicAnalyzer {
  public:
    /**
     * @brief Convenience alias for expected type
     */
    template <typename T, typename E>
    using Result = expected<T, E>;

    /**
     * @brief Harmonic profile with detailed spectral analysis
     */
    struct HarmonicProfile {
        float spectralCentroid = 0.0f;  ///< Spectral centroid in Hz
        float spectralSpread = 0.0f;    ///< Spectral spread in Hz
        float spectralRolloff = 0.0f;   ///< 85% spectral rolloff frequency
        float spectralFlatness = 0.0f;  ///< Spectral flatness (0-1)

        std::vector<float> harmonicRatios;  ///< Harmonic-to-fundamental ratios
        std::vector<float> harmonicFreqs;   ///< Harmonic frequencies in Hz
        std::vector<float> harmonicAmps;    ///< Harmonic amplitudes

        struct TonalQualities {
            float rasp = 0.0f;        ///< Rasp quality (0-1 scale)
            float whine = 0.0f;       ///< Whine quality (0-1 scale)
            float resonance = 0.0f;   ///< Resonance quality (0-1 scale)
            float brightness = 0.0f;  ///< Brightness (0-1 scale)
            float roughness = 0.0f;   ///< Roughness (0-1 scale)
        } qualities;

        std::vector<float> formants;           ///< Formant frequencies in Hz
        std::vector<float> formantBandwidths;  ///< Formant bandwidths in Hz

        float fundamentalFreq = 0.0f;       ///< Fundamental frequency in Hz
        float harmonicToNoiseRatio = 0.0f;  ///< HNR in dB
        float inharmonicity = 0.0f;         ///< Inharmonicity measure

        bool isHarmonic = false;  ///< Whether signal is harmonic
        float confidence = 0.0f;  ///< Analysis confidence (0-1)
        float timestamp = 0.0f;   ///< Timestamp of analysis
    };

    /**
     * @brief Configuration for harmonic analysis
     */
    struct Config {
        float sampleRate = 44100.0f;        ///< Audio sample rate in Hz
        size_t fftSize = 4096;              ///< FFT size for analysis
        size_t hopSize = 1024;              ///< Hop size between windows
        float minFrequency = 80.0f;         ///< Minimum analysis frequency
        float maxFrequency = 8000.0f;       ///< Maximum analysis frequency
        size_t maxHarmonics = 10;           ///< Maximum harmonics to analyze
        float harmonicTolerance = 0.1f;     ///< Harmonic detection tolerance
        size_t numFormants = 4;             ///< Number of formants to extract
        bool enableFormantTracking = true;  ///< Enable formant analysis
        bool enableTonalAnalysis = true;    ///< Enable tonal quality analysis
        float noiseFloorDb = -60.0f;        ///< Noise floor threshold in dB
    };

    /**
     * @brief Error codes for harmonic analysis operations
     */
    enum class Error {
        OK = 0,
        INVALID_AUDIO_DATA,
        INSUFFICIENT_DATA,
        INVALID_SAMPLE_RATE,
        INVALID_FFT_SIZE,
        INITIALIZATION_FAILED,
        PROCESSING_ERROR,
        FFT_ERROR
    };

    /**
     * @brief Factory method for creating HarmonicAnalyzer instances
     * @param config Configuration parameters
     * @return Result containing HarmonicAnalyzer instance or error
     */
    static Result<std::unique_ptr<HarmonicAnalyzer>, Error> create(const Config& config);

    /**
     * @brief Virtual destructor
     */
    virtual ~HarmonicAnalyzer() = default;

    /**
     * @brief Analyze harmonic content in audio buffer
     * @param audio Audio samples to analyze
     * @return Result containing harmonic analysis or error
     */
    virtual Result<HarmonicProfile, Error> analyzeHarmonics(std::span<const float> audio) = 0;

    /**
     * @brief Process audio chunk for continuous harmonic tracking
     * @param audio Audio samples to process
     * @return Processing status
     */
    virtual Result<void, Error> processAudioChunk(std::span<const float> audio) = 0;

    /**
     * @brief Get current harmonic analysis
     * @return Most recent harmonic profile
     */
    virtual Result<HarmonicProfile, Error> getCurrentAnalysis() = 0;

    /**
     * @brief Analyze spectral characteristics without full harmonic analysis
     * @param audio Audio samples to analyze
     * @return Basic spectral features
     */
    virtual Result<std::pair<float, float>, Error>
    getSpectralFeatures(std::span<const float> audio) = 0;

    /**
     * @brief Extract formant frequencies from audio
     * @param audio Audio samples to analyze
     * @return Vector of formant frequencies in Hz
     */
    virtual Result<std::vector<float>, Error> extractFormants(std::span<const float> audio) = 0;

    /**
     * @brief Calculate tonal quality metrics
     * @param audio Audio samples to analyze
     * @return Tonal quality assessment
     */
    virtual Result<HarmonicProfile::TonalQualities, Error>
    assessTonalQualities(std::span<const float> audio) = 0;

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
     * @brief Export harmonic analysis as JSON string
     * @param profile Harmonic profile to export
     * @return JSON string representation
     */
    static std::string exportToJson(const HarmonicProfile& profile);

    /**
     * @brief Check if harmonic analysis is currently active
     * @return True if actively analyzing harmonics
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get processing statistics
     * @return Statistics about recent processing performance
     */
    virtual std::string getProcessingStats() const = 0;

    /**
     * @brief Get frequency bins for visualization
     * @return Vector of frequency bin centers in Hz
     */
    virtual std::vector<float> getFrequencyBins() const = 0;

    /**
     * @brief Get current spectrum magnitude
     * @return Current magnitude spectrum
     */
    virtual Result<std::vector<float>, Error> getCurrentSpectrum() = 0;

  protected:
    /**
     * @brief Protected constructor for factory pattern
     */
    HarmonicAnalyzer() = default;

    /**
     * @brief Copy constructor deleted
     */
    HarmonicAnalyzer(const HarmonicAnalyzer&) = delete;

    /**
     * @brief Assignment operator deleted
     */
    HarmonicAnalyzer& operator=(const HarmonicAnalyzer&) = delete;
};

}  // namespace huntmaster
