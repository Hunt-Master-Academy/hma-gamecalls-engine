#pragma once

#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/Expected.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

namespace huntmaster {

/**
 * @class EnhancedAnalysisProcessor
 * @brief Advanced multi-modal audio analysis combining pitch, harmonic, and cadence analysis
 *
 * This processor integrates the Enhanced Analyzers (PitchTracker, HarmonicAnalyzer,
 * CadenceAnalyzer) into a unified interface for comprehensive audio analysis.
 */
class EnhancedAnalysisProcessor {
  public:
    /**
     * @brief Combined analysis profile containing results from all analyzers
     */
    struct EnhancedAnalysisProfile {
        float timestamp = 0.0f;          ///< Analysis timestamp in seconds
        float duration = 0.0f;           ///< Audio duration analyzed
        bool isValid = false;            ///< Validity flag
        float overallConfidence = 0.0f;  ///< Overall confidence (0-1)

        // Individual analyzer results
        std::optional<PitchTracker::PitchResult> pitchResult;
        std::optional<HarmonicAnalyzer::HarmonicProfile> harmonicProfile;
        std::optional<CadenceAnalyzer::CadenceProfile> cadenceProfile;

        /**
         * @brief Combined features for ML analysis
         */
        struct CombinedFeatures {
            // Pitch features
            float fundamentalFrequency = 0.0f;
            float pitchStability = 0.0f;
            std::vector<float> pitchContour;

            // Harmonic features
            float spectralCentroid = 0.0f;
            float harmonicToNoiseRatio = 0.0f;
            std::vector<float> harmonicRatios;

            // Tonal quality features
            float brightness = 0.0f;
            float roughness = 0.0f;
            float resonance = 0.0f;

            // Rhythmic features
            float estimatedTempo = 0.0f;
            float rhythmComplexity = 0.0f;
            std::vector<float> onsetTimes;
        } combinedFeatures;

        /**
         * @brief Visualization data for real-time display
         */
        struct VisualizationData {
            std::vector<float> pitchTrack;         ///< Pitch over time
            std::vector<float> harmonicSpectrum;   ///< Harmonic frequencies
            std::vector<float> onsetFunction;      ///< Onset detection function
            std::vector<float> beatTrackingState;  ///< Beat tracking state
        } visualizationData;
    };

    /**
     * @brief Configuration for enhanced analysis processor
     */
    struct Config {
        float sampleRate = 44100.0f;
        bool enablePitchTracking = true;
        bool enableHarmonicAnalysis = true;
        bool enableCadenceAnalysis = true;
        bool enableVisualizationData = false;
        bool realTimeMode = false;
        bool highQualityMode = false;

        // Individual analyzer configurations
        PitchTracker::Config pitchConfig;
        HarmonicAnalyzer::Config harmonicConfig;
        CadenceAnalyzer::Config cadenceConfig;
    };

    /**
     * @brief Error codes for enhanced analysis operations
     */
    enum class Error {
        OK = 0,
        INITIALIZATION_FAILED,
        INVALID_AUDIO_DATA,
        INSUFFICIENT_DATA,
        PITCH_ANALYSIS_FAILED,
        HARMONIC_ANALYSIS_FAILED,
        CADENCE_ANALYSIS_FAILED,
        PROCESSING_ERROR
    };

    /**
     * @brief Convenience alias for expected type
     */
    template <typename T, typename E = Error>
    using Result = huntmaster::expected<T, E>;

    /**
     * @brief Factory method for creating enhanced analysis processor
     */
    static Result<std::unique_ptr<EnhancedAnalysisProcessor>, Error> create(const Config& config);

    /**
     * @brief Analyze audio chunk with all enabled analyzers
     */
    virtual Result<EnhancedAnalysisProfile, Error> analyze(std::span<const float> audio) = 0;

    /**
     * @brief Process audio chunk for real-time analysis
     */
    virtual Result<void, Error> processChunk(std::span<const float> audio) = 0;

    /**
     * @brief Get current analysis result
     */
    virtual Result<EnhancedAnalysisProfile, Error> getCurrentAnalysis() = 0;

    /**
     * @brief Extract ML features from audio
     */
    virtual Result<EnhancedAnalysisProfile::CombinedFeatures, Error>
    extractMLFeatures(std::span<const float> audio) = 0;

    /**
     * @brief Generate visualization data from analysis profile
     */
    virtual Result<EnhancedAnalysisProfile::VisualizationData, Error>
    generateVisualizationData(const EnhancedAnalysisProfile& profile) = 0;

    /**
     * @brief Adapt configuration based on audio content characteristics
     */
    virtual void adaptToAudioContent(const EnhancedAnalysisProfile& profile) = 0;

    /**
     * @brief Get performance statistics
     */
    virtual std::string getPerformanceStats() const = 0;

    /**
     * @brief Reset internal state
     */
    virtual void reset() = 0;

    /**
     * @brief Export analysis profile to JSON
     */
    static std::string exportToJson(const EnhancedAnalysisProfile& profile);

  protected:
    EnhancedAnalysisProcessor() = default;

  public:
    virtual ~EnhancedAnalysisProcessor() = default;
};

/**
 * @class AdaptiveConfigManager
 * @brief Manages adaptive configuration based on audio content characteristics
 */
class AdaptiveConfigManager {
  public:
    struct AudioCharacteristics {
        bool isVocal = false;
        bool isRhythmic = false;
        bool isTonal = false;
        float dominantFrequency = 0.0f;
        float harmonicity = 0.0f;
    };

    static AudioCharacteristics
    detectCharacteristics(const EnhancedAnalysisProcessor::EnhancedAnalysisProfile& profile);
    static EnhancedAnalysisProcessor::Config
    adaptConfiguration(const AudioCharacteristics& characteristics, bool realTimeMode);
    static EnhancedAnalysisProcessor::Config getRealTimeConfig(float sampleRate);
    static EnhancedAnalysisProcessor::Config getHighQualityConfig(float sampleRate);
};

}  // namespace huntmaster
