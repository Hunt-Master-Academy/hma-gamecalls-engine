// Enhanced Analysis Processor Implementation
// Multi-modal audio analysis combining PitchTracker, HarmonicAnalyzer, and CadenceAnalyzer

#include "huntmaster/enhanced/EnhancedAnalysisProcessor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <sstream>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/security/memory-guard.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster {

/**
 * @brief Internal implementation of EnhancedAnalysisProcessor
 */
class EnhancedAnalysisProcessorImpl : public EnhancedAnalysisProcessor {
  private:
    Config config_;
    std::unique_ptr<PitchTracker> pitchTracker_;
    std::unique_ptr<HarmonicAnalyzer> harmonicAnalyzer_;
    std::unique_ptr<CadenceAnalyzer> cadenceAnalyzer_;

    EnhancedAnalysisProfile currentProfile_;
    bool isInitialized_ = false;
    bool isActive_ = false;

    // Performance tracking
    size_t processedFrames_ = 0;
    double totalProcessingTime_ = 0.0;
    double maxProcessingTime_ = 0.0;
    std::chrono::steady_clock::time_point lastProcessTime_;

  public:
    EnhancedAnalysisProcessorImpl(const Config& config) : config_(config) {
        try {
            // Create individual analyzers based on configuration
            if (config_.enablePitchTracking) {
                auto pitchResult = PitchTracker::create(config_.pitchConfig);
                if (!pitchResult.has_value()) {
                    throw std::runtime_error("Failed to create PitchTracker");
                }
                pitchTracker_ = std::move(pitchResult.value());
            }

            if (config_.enableHarmonicAnalysis) {
                auto harmonicResult = HarmonicAnalyzer::create(config_.harmonicConfig);
                if (!harmonicResult.has_value()) {
                    throw std::runtime_error("Failed to create HarmonicAnalyzer");
                }
                harmonicAnalyzer_ = std::move(harmonicResult.value());
            }

            if (config_.enableCadenceAnalysis) {
                auto cadenceResult = CadenceAnalyzer::create(config_.cadenceConfig);
                if (!cadenceResult.has_value()) {
                    throw std::runtime_error("Failed to create CadenceAnalyzer");
                }
                cadenceAnalyzer_ = std::move(cadenceResult.value());
            }

            isInitialized_ = true;
            lastProcessTime_ = std::chrono::steady_clock::now();

        } catch (const std::exception& e) {
            isInitialized_ = false;
            // Note: In production, use proper logging
        }
    }

    Result<EnhancedAnalysisProfile, Error> analyze(std::span<const float> audio) override {
        security::MemoryGuard guard(security::GuardConfig{});

        if (!isInitialized_) {
            return Result<EnhancedAnalysisProfile, Error>(
                huntmaster::unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        if (audio.empty()) {
            return Result<EnhancedAnalysisProfile, Error>(
                huntmaster::unexpected<Error>(Error::INVALID_AUDIO_DATA));
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            EnhancedAnalysisProfile profile;
            profile.timestamp = static_cast<float>(processedFrames_) * config_.pitchConfig.hopSize
                                / config_.sampleRate;
            profile.duration = static_cast<float>(audio.size()) / config_.sampleRate;

            // Individual analyzer processing
            bool anySuccess = false;

            // Pitch analysis
            if (pitchTracker_) {
                auto pitchResult = pitchTracker_->detectPitch(audio);
                if (pitchResult.has_value()) {
                    profile.pitchResult = pitchResult.value();
                    anySuccess = true;
                }
            }

            // Harmonic analysis
            if (harmonicAnalyzer_) {
                auto harmonicResult = harmonicAnalyzer_->analyzeHarmonics(audio);
                if (harmonicResult.has_value()) {
                    profile.harmonicProfile = harmonicResult.value();
                    anySuccess = true;
                }
            }

            // Cadence analysis
            if (cadenceAnalyzer_) {
                auto cadenceResult = cadenceAnalyzer_->analyzeCadence(audio);
                if (cadenceResult.has_value()) {
                    profile.cadenceProfile = cadenceResult.value();
                    anySuccess = true;
                }
            }

            if (!anySuccess) {
                return Result<EnhancedAnalysisProfile, Error>(
                    huntmaster::unexpected<Error>(Error::PROCESSING_ERROR));
            }

            // Combine features
            combineFeaturesInternal(profile);

            // Generate visualization data if enabled
            if (config_.enableVisualizationData) {
                generateVisualizationDataInternal(profile);
            }

            // Calculate overall confidence
            calculateOverallConfidence(profile);

            profile.isValid = true;
            currentProfile_ = profile;
            isActive_ = true;

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            updatePerformanceStats(duration);

            return Result<EnhancedAnalysisProfile, Error>(std::move(profile));

        } catch (const std::exception& e) {
            return Result<EnhancedAnalysisProfile, Error>(
                huntmaster::unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    EnhancedAnalysisProcessor::Result<void, Error>
    processChunk(std::span<const float> audio) override {
        auto result = analyze(audio);
        if (result.has_value()) {
            return EnhancedAnalysisProcessor::Result<void, Error>();
        } else {
            return EnhancedAnalysisProcessor::Result<void, Error>(
                huntmaster::unexpected<Error>(result.error()));
        }
    }

    EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile, Error>
    getCurrentAnalysis() override {
        if (!isActive_) {
            return EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile, Error>(
                huntmaster::unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile, Error>(currentProfile_);
    }

    EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile::CombinedFeatures, Error>
    extractMLFeatures(std::span<const float> audio) override {
        auto analysisResult = analyze(audio);
        if (!analysisResult.has_value()) {
            return EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile::CombinedFeatures,
                                                     Error>(
                huntmaster::unexpected<Error>(analysisResult.error()));
        }

        return EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile::CombinedFeatures, Error>(
            analysisResult.value().combinedFeatures);
    }

    EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile::VisualizationData, Error>
    generateVisualizationData(const EnhancedAnalysisProfile& profile) override {
        EnhancedAnalysisProfile::VisualizationData vizData;

        // Extract pitch track
        if (profile.pitchResult.has_value()) {
            vizData.pitchTrack = {profile.pitchResult->frequency};
        }

        // Extract harmonic spectrum
        if (profile.harmonicProfile.has_value()) {
            vizData.harmonicSpectrum = profile.harmonicProfile->harmonicFreqs;
        }

        // Extract onset function and beat tracking
        if (profile.cadenceProfile.has_value()) {
            // Use beat times as onset function
            vizData.onsetFunction = profile.cadenceProfile->beatTimes;
            vizData.beatTrackingState = profile.cadenceProfile->beatStrengths;
        }

        return EnhancedAnalysisProcessor::Result<EnhancedAnalysisProfile::VisualizationData, Error>(
            vizData);
    }

    void adaptToAudioContent(const EnhancedAnalysisProfile& profile) override {
        auto characteristics = AdaptiveConfigManager::detectCharacteristics(profile);
        auto adaptedConfig =
            AdaptiveConfigManager::adaptConfiguration(characteristics, config_.realTimeMode);

        // Update configurations (simplified - in practice would recreate analyzers)
        config_.pitchConfig = adaptedConfig.pitchConfig;
        config_.harmonicConfig = adaptedConfig.harmonicConfig;
        config_.cadenceConfig = adaptedConfig.cadenceConfig;
    }

    std::string getPerformanceStats() const override {
        std::ostringstream stats;
        stats << "Enhanced Analysis Performance Stats:\n";
        stats << "  Processed Frames: " << processedFrames_ << "\n";
        stats << "  Total Processing Time: " << totalProcessingTime_ << "ms\n";
        stats << "  Average Processing Time: "
              << (processedFrames_ > 0 ? totalProcessingTime_ / processedFrames_ : 0.0) << "ms\n";
        stats << "  Max Processing Time: " << maxProcessingTime_ << "ms\n";
        stats << "  Real-time Factor: " << calculateRealtimeFactor() << "x\n";
        return stats.str();
    }

    void reset() override {
        if (pitchTracker_)
            pitchTracker_->reset();
        if (harmonicAnalyzer_)
            harmonicAnalyzer_->reset();
        if (cadenceAnalyzer_)
            cadenceAnalyzer_->reset();

        currentProfile_ = EnhancedAnalysisProfile{};
        isActive_ = false;
        processedFrames_ = 0;
        totalProcessingTime_ = 0.0;
        maxProcessingTime_ = 0.0;
    }

  private:
    void combineFeaturesInternal(EnhancedAnalysisProfile& profile) {
        auto& features = profile.combinedFeatures;

        // Pitch features
        if (profile.pitchResult.has_value()) {
            features.fundamentalFrequency = profile.pitchResult->frequency;
            features.pitchStability = profile.pitchResult->confidence;
            features.pitchContour = {profile.pitchResult->frequency};  // Single point for now
        }

        // Harmonic features
        if (profile.harmonicProfile.has_value()) {
            features.spectralCentroid = profile.harmonicProfile->spectralCentroid;
            features.harmonicToNoiseRatio = profile.harmonicProfile->harmonicToNoiseRatio;

            // Calculate harmonic ratios
            if (!profile.harmonicProfile->harmonicFreqs.empty()
                && profile.harmonicProfile->fundamentalFreq > 0) {
                features.harmonicRatios.clear();
                for (float harmonic : profile.harmonicProfile->harmonicFreqs) {
                    features.harmonicRatios.push_back(harmonic
                                                      / profile.harmonicProfile->fundamentalFreq);
                }
            }

            // Tonal qualities
            features.brightness = profile.harmonicProfile->qualities.brightness;
            features.roughness = profile.harmonicProfile->qualities.roughness;
            features.resonance = profile.harmonicProfile->qualities.resonance;
        }

        // Rhythmic features
        if (profile.cadenceProfile.has_value()) {
            features.estimatedTempo = profile.cadenceProfile->estimatedTempo;
            features.rhythmComplexity = profile.cadenceProfile->rhythm.rhythmComplexity;
            features.onsetTimes = profile.cadenceProfile->beatTimes;
        }
    }

    void generateVisualizationDataInternal(EnhancedAnalysisProfile& profile) {
        auto& vizData = profile.visualizationData;

        // Generate pitch track
        if (profile.pitchResult.has_value()) {
            vizData.pitchTrack = {profile.pitchResult->frequency};
        }

        // Generate harmonic spectrum
        if (profile.harmonicProfile.has_value()) {
            vizData.harmonicSpectrum = profile.harmonicProfile->harmonicFreqs;
        }

        // Generate onset function and beat tracking state
        if (profile.cadenceProfile.has_value()) {
            // Use simplified onset function
            vizData.onsetFunction.clear();
            for (size_t i = 0; i < profile.cadenceProfile->beatTimes.size(); ++i) {
                vizData.onsetFunction.push_back(1.0f);  // Simplified
            }

            vizData.beatTrackingState = profile.cadenceProfile->beatStrengths;
        }
    }

    void calculateOverallConfidence(EnhancedAnalysisProfile& profile) {
        std::vector<float> confidences;

        if (profile.pitchResult.has_value()) {
            confidences.push_back(profile.pitchResult->confidence);
        }

        if (profile.harmonicProfile.has_value()) {
            confidences.push_back(profile.harmonicProfile->confidence);
        }

        if (profile.cadenceProfile.has_value()) {
            confidences.push_back(profile.cadenceProfile->confidence);
        }

        if (!confidences.empty()) {
            profile.overallConfidence =
                std::accumulate(confidences.begin(), confidences.end(), 0.0f) / confidences.size();
        } else {
            profile.overallConfidence = 0.0f;
        }
    }

    void updatePerformanceStats(double processingTime) {
        processedFrames_++;
        totalProcessingTime_ += processingTime;
        maxProcessingTime_ = std::max(maxProcessingTime_, processingTime);
    }

    double calculateRealtimeFactor() const {
        if (processedFrames_ == 0)
            return 0.0;

        auto now = std::chrono::steady_clock::now();
        [[maybe_unused]] auto elapsed = std::chrono::duration<double, std::milli>(now - lastProcessTime_).count();

        double audioTime = static_cast<double>(processedFrames_) * config_.pitchConfig.hopSize
                           * 1000.0 / config_.sampleRate;

        return totalProcessingTime_ / audioTime;
    }
};

// Factory method implementation
EnhancedAnalysisProcessor::Result<std::unique_ptr<EnhancedAnalysisProcessor>,
                                  EnhancedAnalysisProcessor::Error>
EnhancedAnalysisProcessor::create(const Config& config) {
    try {
        auto processor = std::make_unique<EnhancedAnalysisProcessorImpl>(config);
        return Result<std::unique_ptr<EnhancedAnalysisProcessor>, Error>(std::move(processor));
    } catch (const std::exception& e) {
        return Result<std::unique_ptr<EnhancedAnalysisProcessor>, Error>(
            huntmaster::unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

// JSON export implementation
std::string EnhancedAnalysisProcessor::exportToJson(const EnhancedAnalysisProfile& profile) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"timestamp\": " << profile.timestamp << ",\n";
    json << "  \"duration\": " << profile.duration << ",\n";
    json << "  \"isValid\": " << (profile.isValid ? "true" : "false") << ",\n";
    json << "  \"overallConfidence\": " << profile.overallConfidence << ",\n";

    // Pitch data
    if (profile.pitchResult.has_value()) {
        json << "  \"pitch\": {\n";
        json << "    \"frequency\": " << profile.pitchResult->frequency << ",\n";
        json << "    \"confidence\": " << profile.pitchResult->confidence << "\n";
        json << "  },\n";
    }

    // Harmonic data
    if (profile.harmonicProfile.has_value()) {
        json << "  \"harmonic\": {\n";
        json << "    \"fundamentalFreq\": " << profile.harmonicProfile->fundamentalFreq << ",\n";
        json << "    \"spectralCentroid\": " << profile.harmonicProfile->spectralCentroid << ",\n";
        json << "    \"confidence\": " << profile.harmonicProfile->confidence << "\n";
        json << "  },\n";
    }

    // Cadence data
    if (profile.cadenceProfile.has_value()) {
        json << "  \"cadence\": {\n";
        json << "    \"estimatedTempo\": " << profile.cadenceProfile->estimatedTempo << ",\n";
        json << "    \"confidence\": " << profile.cadenceProfile->confidence << "\n";
        json << "  },\n";
    }

    // Combined features
    json << "  \"combinedFeatures\": {\n";
    json << "    \"fundamentalFrequency\": " << profile.combinedFeatures.fundamentalFrequency
         << ",\n";
    json << "    \"spectralCentroid\": " << profile.combinedFeatures.spectralCentroid << ",\n";
    json << "    \"estimatedTempo\": " << profile.combinedFeatures.estimatedTempo << ",\n";
    json << "    \"brightness\": " << profile.combinedFeatures.brightness << "\n";
    json << "  }\n";

    json << "}";
    return json.str();
}

// Adaptive Configuration Manager implementation
AdaptiveConfigManager::AudioCharacteristics AdaptiveConfigManager::detectCharacteristics(
    const EnhancedAnalysisProcessor::EnhancedAnalysisProfile& profile) {
    AudioCharacteristics characteristics;

    // Detect vocal content
    if (profile.pitchResult.has_value() && profile.harmonicProfile.has_value()) {
        float freq = profile.pitchResult->frequency;
        float confidence = profile.pitchResult->confidence;

        // Human vocal range approximately 80-1000 Hz
        characteristics.isVocal = (freq >= 80.0f && freq <= 1000.0f && confidence > 0.7f);
        characteristics.dominantFrequency = freq;
    }

    // Detect rhythmic content
    if (profile.cadenceProfile.has_value()) {
        float tempo = profile.cadenceProfile->estimatedTempo;
        characteristics.isRhythmic = (tempo > 60.0f && tempo < 200.0f);
    }

    // Detect tonal content
    if (profile.harmonicProfile.has_value()) {
        float hnr = profile.harmonicProfile->harmonicToNoiseRatio;
        characteristics.isTonal = (hnr > 10.0f);    // >10dB HNR indicates tonal content
        characteristics.harmonicity = hnr / 30.0f;  // Normalize to 0-1
    }

    return characteristics;
}

EnhancedAnalysisProcessor::Config
AdaptiveConfigManager::adaptConfiguration(const AudioCharacteristics& characteristics,
                                          bool realTimeMode) {
    EnhancedAnalysisProcessor::Config config;
    config.realTimeMode = realTimeMode;

    if (realTimeMode) {
        // Optimize for speed
        config.pitchConfig.windowSize = 1024;
        config.harmonicConfig.fftSize = 1024;
        config.cadenceConfig.frameSize = 0.05f;
    } else {
        // Optimize for quality
        config.pitchConfig.windowSize = 2048;
        config.harmonicConfig.fftSize = 4096;
        config.cadenceConfig.frameSize = 0.025f;
    }

    // Adapt based on content
    if (characteristics.isVocal) {
        config.harmonicConfig.enableFormantTracking = true;
        config.cadenceConfig.enableSyllableAnalysis = true;
    }

    if (characteristics.isRhythmic) {
        config.cadenceConfig.enableBeatTracking = true;
        config.cadenceConfig.enableOnsetDetection = true;
    }

    return config;
}

EnhancedAnalysisProcessor::Config AdaptiveConfigManager::getRealTimeConfig(float sampleRate) {
    EnhancedAnalysisProcessor::Config config;
    config.sampleRate = sampleRate;
    config.realTimeMode = true;

    // Optimized for <10ms processing
    config.pitchConfig.sampleRate = sampleRate;
    config.pitchConfig.windowSize = 512;
    config.pitchConfig.hopSize = 128;
    config.pitchConfig.enableVibratoDetection = false;

    config.harmonicConfig.sampleRate = sampleRate;
    config.harmonicConfig.fftSize = 512;
    config.harmonicConfig.hopSize = 128;
    config.harmonicConfig.enableFormantTracking = false;
    config.harmonicConfig.enableTonalAnalysis = false;

    config.cadenceConfig.sampleRate = sampleRate;
    config.cadenceConfig.frameSize = 0.1f;
    config.cadenceConfig.hopSize = 0.05f;
    config.cadenceConfig.enableSyllableAnalysis = false;

    return config;
}

EnhancedAnalysisProcessor::Config AdaptiveConfigManager::getHighQualityConfig(float sampleRate) {
    EnhancedAnalysisProcessor::Config config;
    config.sampleRate = sampleRate;
    config.highQualityMode = true;

    // Optimized for accuracy
    config.pitchConfig.sampleRate = sampleRate;
    config.pitchConfig.windowSize = 4096;
    config.pitchConfig.hopSize = 1024;
    config.pitchConfig.enableVibratoDetection = true;

    config.harmonicConfig.sampleRate = sampleRate;
    config.harmonicConfig.fftSize = 8192;
    config.harmonicConfig.hopSize = 1024;
    config.harmonicConfig.enableFormantTracking = true;
    config.harmonicConfig.enableTonalAnalysis = true;

    config.cadenceConfig.sampleRate = sampleRate;
    config.cadenceConfig.frameSize = 0.025f;
    config.cadenceConfig.hopSize = 0.010f;
    config.cadenceConfig.enableSyllableAnalysis = true;

    return config;
}

}  // namespace huntmaster
