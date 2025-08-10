#include "huntmaster/core/HarmonicAnalyzer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <numeric>
#include <sstream>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/security/memory-guard.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef USE_FFTW3
#include <fftw3.h>
#else
#include "kiss_fftr.h"
#endif

namespace huntmaster {

/**
 * @brief Internal implementation of HarmonicAnalyzer
 */
class HarmonicAnalyzerImpl : public HarmonicAnalyzer {
  private:
    Config config_;
    std::vector<float> buffer_;
    std::vector<float> window_;
    std::vector<float> spectrum_;
    std::vector<float> frequencyBins_;

    HarmonicProfile currentProfile_;
    bool isInitialized_ = false;
    bool isActive_ = false;

    // Performance tracking
    size_t processedFrames_ = 0;
    double totalProcessingTime_ = 0.0;
    double maxProcessingTime_ = 0.0;

#ifdef USE_FFTW3
    fftwf_plan fftPlan_ = nullptr;
    fftwf_complex* fftInput_ = nullptr;
    fftwf_complex* fftOutput_ = nullptr;
#endif
#ifndef USE_FFTW3
    kiss_fftr_cfg kissCfg_ = nullptr;
    std::vector<kiss_fft_cpx> kissOut_;
#endif

  public:
    HarmonicAnalyzerImpl(const Config& config) : config_(config) {
        initializeBuffers();
        createFFTPlan();
        generateWindow();
        generateFrequencyBins();
        isInitialized_ = true;

        // DEBUG_LOG removed for compilation
        // "Initialized with FFT size: " + std::to_string(config_.fftSize)
        //      + ", sample rate: " + std::to_string(config_.sampleRate));
    }

    ~HarmonicAnalyzerImpl() {
        cleanup();
    }

    Result<HarmonicProfile, Error> analyzeHarmonics(std::span<const float> audio) override {
        security::MemoryGuard guard(security::GuardConfig{});

        if (!isInitialized_) {
            return Result<HarmonicProfile, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        if (audio.size() < config_.fftSize) {
            return Result<HarmonicProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            // Compute spectrum
            auto spectrumResult = computeSpectrum(audio);
            if (!spectrumResult.has_value()) {
                return Result<HarmonicProfile, Error>(unexpected<Error>(spectrumResult.error()));
            }

            HarmonicProfile profile;
            profile.timestamp =
                static_cast<float>(processedFrames_ * config_.hopSize) / config_.sampleRate;

            // Basic spectral features
            computeSpectralFeatures(profile);

            // Find fundamental frequency
            profile.fundamentalFreq = findFundamentalFrequency();

            if (profile.fundamentalFreq > 0.0f) {
                // Analyze harmonics
                analyzeHarmonicStructure(profile);

                // Extract formants if enabled
                if (config_.enableFormantTracking) {
                    extractFormantsInternal(profile);
                }

                // Assess tonal qualities if enabled
                if (config_.enableTonalAnalysis) {
                    assessTonalQualitiesInternal(profile);
                }

                profile.isHarmonic = true;
                profile.confidence = calculateConfidence(profile);
            } else {
                profile.isHarmonic = false;
                profile.confidence = 0.0f;
            }

            currentProfile_ = profile;
            isActive_ = true;

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            updatePerformanceStats(duration);

            // DEBUG_LOG removed for compilation
            // "Analysis complete - Fundamental: " + std::to_string(profile.fundamentalFreq)
            //     + "Hz, Confidence: " + std::to_string(profile.confidence));

            return Result<HarmonicProfile, Error>(std::move(profile));
        } catch (const std::exception& e) {
            // DEBUG_LOG removed for compilation
            // "Exception in analyzeHarmonics: " + std::string(e.what()));
            return Result<HarmonicProfile, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    Result<void, Error> processAudioChunk(std::span<const float> audio) override {
        if (!isInitialized_) {
            return Result<void, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        // Add to buffer for continuous processing
        buffer_.insert(buffer_.end(), audio.begin(), audio.end());

        // Process if we have enough data
        while (buffer_.size() >= config_.fftSize) {
            std::span<const float> chunk(buffer_.data(), config_.fftSize);
            auto result = analyzeHarmonics(chunk);

            if (!result.has_value()) {
                return Result<void, Error>(unexpected<Error>(result.error()));
            }

            // Advance by hop size
            buffer_.erase(buffer_.begin(), buffer_.begin() + config_.hopSize);
            processedFrames_++;
        }

        return Result<void, Error>();
    }

    Result<HarmonicProfile, Error> getCurrentAnalysis() override {
        if (!isActive_) {
            return Result<HarmonicProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<HarmonicProfile, Error>(currentProfile_);
    }

    Result<std::pair<float, float>, Error>
    getSpectralFeatures(std::span<const float> audio) override {
        auto spectrumResult = computeSpectrum(audio);
        if (!spectrumResult.has_value()) {
            return Result<std::pair<float, float>, Error>(
                unexpected<Error>(spectrumResult.error()));
        }

        float centroid = computeSpectralCentroid();
        float spread = computeSpectralSpread(centroid);

        return Result<std::pair<float, float>, Error>(std::make_pair(centroid, spread));
    }

    Result<std::vector<float>, Error> extractFormants(std::span<const float> audio) override {
        auto spectrumResult = computeSpectrum(audio);
        if (!spectrumResult.has_value()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(spectrumResult.error()));
        }

        std::vector<float> formants;
        extractFormantsFromSpectrum(formants);

        return Result<std::vector<float>, Error>(std::move(formants));
    }

    Result<HarmonicProfile::TonalQualities, Error>
    assessTonalQualities(std::span<const float> audio) override {
        auto spectrumResult = computeSpectrum(audio);
        if (!spectrumResult.has_value()) {
            return Result<HarmonicProfile::TonalQualities, Error>(
                unexpected<Error>(spectrumResult.error()));
        }

        HarmonicProfile::TonalQualities qualities;
        assessTonalQualitiesFromSpectrum(qualities);

        return Result<HarmonicProfile::TonalQualities, Error>(qualities);
    }

    void reset() override {
        buffer_.clear();
        spectrum_.clear();
        currentProfile_ = HarmonicProfile{};
        isActive_ = false;
        processedFrames_ = 0;
        totalProcessingTime_ = 0.0;
        maxProcessingTime_ = 0.0;

        // DEBUG_LOG removed for compilation
    }

    Result<void, Error> updateConfig(const Config& config) override {
        if (config.sampleRate <= 0 || config.fftSize == 0) {
            return Result<void, Error>(unexpected<Error>(Error::INVALID_SAMPLE_RATE));
        }

        config_ = config;
        cleanup();
        initializeBuffers();
        createFFTPlan();
        generateWindow();
        generateFrequencyBins();

        // DEBUG_LOG removed for compilation
        return Result<void, Error>();
    }

    const Config& getConfig() const override {
        return config_;
    }

    bool isActive() const override {
        return isActive_;
    }

    std::string getProcessingStats() const override {
        std::ostringstream oss;
        oss << "HarmonicAnalyzer Stats:\n";
        oss << "  Processed frames: " << processedFrames_ << "\n";
        oss << "  Total processing time: " << totalProcessingTime_ << "ms\n";
        oss << "  Max processing time: " << maxProcessingTime_ << "ms\n";
        if (processedFrames_ > 0) {
            oss << "  Average processing time: " << (totalProcessingTime_ / processedFrames_)
                << "ms\n";
        }
        oss << "  FFT size: " << config_.fftSize << "\n";
        oss << "  Sample rate: " << config_.sampleRate << "Hz";
        return oss.str();
    }

    std::vector<float> getFrequencyBins() const override {
        return frequencyBins_;
    }

    Result<std::vector<float>, Error> getCurrentSpectrum() override {
        if (spectrum_.empty()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<std::vector<float>, Error>(spectrum_);
    }

  private:
    void initializeBuffers() {
        buffer_.clear();
        buffer_.reserve(config_.fftSize * 2);
        spectrum_.resize(config_.fftSize / 2 + 1);
        window_.resize(config_.fftSize);
    }

    void createFFTPlan() {
#ifdef USE_FFTW3
        if (fftInput_)
            fftwf_free(fftInput_);
        if (fftOutput_)
            fftwf_free(fftOutput_);
        if (fftPlan_)
            fftwf_destroy_plan(fftPlan_);

        fftInput_ = fftwf_alloc_complex(config_.fftSize);
        fftOutput_ = fftwf_alloc_complex(config_.fftSize);
        fftPlan_ =
            fftwf_plan_dft_1d(config_.fftSize, fftInput_, fftOutput_, FFTW_FORWARD, FFTW_ESTIMATE);
#else
        if (kissCfg_) {
            kiss_fftr_free(kissCfg_);
            kissCfg_ = nullptr;
        }
        kissCfg_ = kiss_fftr_alloc(static_cast<int>(config_.fftSize), 0, nullptr, nullptr);
        kissOut_.resize(config_.fftSize / 2 + 1);
#endif
    }

    void generateWindow() {
        // Hann window
        for (size_t i = 0; i < config_.fftSize; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (config_.fftSize - 1)));
        }
    }

    void generateFrequencyBins() {
        frequencyBins_.resize(config_.fftSize / 2 + 1);
        for (size_t i = 0; i < frequencyBins_.size(); ++i) {
            frequencyBins_[i] = static_cast<float>(i) * config_.sampleRate / config_.fftSize;
        }
    }

    Result<void, Error> computeSpectrum(std::span<const float> audio) {
        if (audio.size() < config_.fftSize) {
            return Result<void, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

#ifdef USE_FFTW3
        // Apply window and copy to FFT input
        for (size_t i = 0; i < config_.fftSize; ++i) {
            fftInput_[i][0] = audio[i] * window_[i];
            fftInput_[i][1] = 0.0f;
        }

        // Execute FFT
        fftwf_execute(fftPlan_);

        // Compute magnitude spectrum
        for (size_t i = 0; i < spectrum_.size(); ++i) {
            float real = fftOutput_[i][0];
            float imag = fftOutput_[i][1];
            spectrum_[i] = std::sqrt(real * real + imag * imag);
        }
#else
        // Kiss FFT real-to-complex path
        std::vector<float> tmp(config_.fftSize);
        for (size_t i = 0; i < config_.fftSize; ++i)
            tmp[i] = audio[i] * window_[i];
        kiss_fftr(kissCfg_, tmp.data(), kissOut_.data());
        for (size_t i = 0; i < spectrum_.size(); ++i) {
            float real = kissOut_[i].r;
            float imag = kissOut_[i].i;
            spectrum_[i] = std::sqrt(real * real + imag * imag);
        }
#endif

        return Result<void, Error>();
    }

    void computeSpectralFeatures(HarmonicProfile& profile) {
        profile.spectralCentroid = computeSpectralCentroid();
        profile.spectralSpread = computeSpectralSpread(profile.spectralCentroid);
        profile.spectralRolloff = computeSpectralRolloff();
        profile.spectralFlatness = computeSpectralFlatness();
    }

    float computeSpectralCentroid() {
        float weightedSum = 0.0f;
        float magnitudeSum = 0.0f;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            float magnitude = spectrum_[i];
            float frequency = frequencyBins_[i];

            if (frequency >= config_.minFrequency && frequency <= config_.maxFrequency) {
                weightedSum += magnitude * frequency;
                magnitudeSum += magnitude;
            }
        }

        return magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
    }

    float computeSpectralSpread(float centroid) {
        float weightedSum = 0.0f;
        float magnitudeSum = 0.0f;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            float magnitude = spectrum_[i];
            float frequency = frequencyBins_[i];

            if (frequency >= config_.minFrequency && frequency <= config_.maxFrequency) {
                float diff = frequency - centroid;
                weightedSum += magnitude * diff * diff;
                magnitudeSum += magnitude;
            }
        }

        return magnitudeSum > 0.0f ? std::sqrt(weightedSum / magnitudeSum) : 0.0f;
    }

    float computeSpectralRolloff() {
        float totalMagnitude = 0.0f;
        for (size_t i = 1; i < spectrum_.size(); ++i) {
            if (frequencyBins_[i] >= config_.minFrequency
                && frequencyBins_[i] <= config_.maxFrequency) {
                totalMagnitude += spectrum_[i];
            }
        }

        float threshold = 0.85f * totalMagnitude;
        float cumulativeMagnitude = 0.0f;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            if (frequencyBins_[i] >= config_.minFrequency
                && frequencyBins_[i] <= config_.maxFrequency) {
                cumulativeMagnitude += spectrum_[i];
                if (cumulativeMagnitude >= threshold) {
                    return frequencyBins_[i];
                }
            }
        }

        return config_.maxFrequency;
    }

    float computeSpectralFlatness() {
        float geometricMean = 1.0f;
        float arithmeticMean = 0.0f;
        size_t count = 0;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            if (frequencyBins_[i] >= config_.minFrequency
                && frequencyBins_[i] <= config_.maxFrequency) {
                float magnitude = std::max(spectrum_[i], 1e-10f);  // Avoid log(0)
                geometricMean *= std::pow(magnitude, 1.0f / (spectrum_.size() - 1));
                arithmeticMean += magnitude;
                count++;
            }
        }

        arithmeticMean /= count;
        return count > 0 ? geometricMean / arithmeticMean : 0.0f;
    }

    float findFundamentalFrequency() {
        // Find peak in spectrum within fundamental frequency range
        size_t peakIndex = 0;
        float maxMagnitude = 0.0f;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            float frequency = frequencyBins_[i];
            if (frequency >= config_.minFrequency && frequency <= config_.maxFrequency) {
                if (spectrum_[i] > maxMagnitude) {
                    maxMagnitude = spectrum_[i];
                    peakIndex = i;
                }
            }
        }

        return peakIndex > 0 ? frequencyBins_[peakIndex] : 0.0f;
    }

    void analyzeHarmonicStructure(HarmonicProfile& profile) {
        if (profile.fundamentalFreq <= 0.0f)
            return;

        profile.harmonicRatios.clear();
        profile.harmonicFreqs.clear();
        profile.harmonicAmps.clear();

        float fundamental = profile.fundamentalFreq;
        float totalHarmonicEnergy = 0.0f;
        float totalEnergy = 0.0f;

        // Analyze harmonics
        for (size_t h = 1; h <= config_.maxHarmonics; ++h) {
            float expectedFreq = fundamental * h;
            if (expectedFreq > config_.maxFrequency)
                break;

            // Find closest bin
            size_t binIndex =
                static_cast<size_t>(expectedFreq * config_.fftSize / config_.sampleRate);
            if (binIndex >= spectrum_.size())
                break;

            // Look for peak within tolerance
            float tolerance = fundamental * config_.harmonicTolerance;
            size_t startBin = std::max(1UL,
                                       static_cast<size_t>((expectedFreq - tolerance)
                                                           * config_.fftSize / config_.sampleRate));
            size_t endBin = std::min(spectrum_.size() - 1,
                                     static_cast<size_t>((expectedFreq + tolerance)
                                                         * config_.fftSize / config_.sampleRate));

            float maxAmp = 0.0f;
            size_t maxBin = binIndex;

            for (size_t i = startBin; i <= endBin; ++i) {
                if (spectrum_[i] > maxAmp) {
                    maxAmp = spectrum_[i];
                    maxBin = i;
                }
            }

            if (maxAmp > 0.0f) {
                profile.harmonicFreqs.push_back(frequencyBins_[maxBin]);
                profile.harmonicAmps.push_back(maxAmp);
                profile.harmonicRatios.push_back(maxAmp / spectrum_[1]);  // Ratio to DC component
                totalHarmonicEnergy += maxAmp * maxAmp;
            }
        }

        // Calculate total energy for HNR
        for (size_t i = 1; i < spectrum_.size(); ++i) {
            totalEnergy += spectrum_[i] * spectrum_[i];
        }

        // Calculate harmonic-to-noise ratio
        if (totalEnergy > 0.0f && totalHarmonicEnergy > 0.0f) {
            float noiseEnergy = totalEnergy - totalHarmonicEnergy;
            profile.harmonicToNoiseRatio =
                10.0f * std::log10(totalHarmonicEnergy / std::max(noiseEnergy, 1e-10f));
        }

        // Calculate inharmonicity
        profile.inharmonicity = calculateInharmonicity(profile);
    }

    float calculateInharmonicity(const HarmonicProfile& profile) {
        if (profile.harmonicFreqs.size() < 2)
            return 0.0f;

        float totalDeviation = 0.0f;
        size_t count = 0;

        for (size_t i = 0; i < profile.harmonicFreqs.size(); ++i) {
            float expectedFreq = profile.fundamentalFreq * (i + 1);
            float actualFreq = profile.harmonicFreqs[i];
            float deviation = std::abs(actualFreq - expectedFreq) / expectedFreq;
            totalDeviation += deviation;
            count++;
        }

        return count > 0 ? totalDeviation / count : 0.0f;
    }

    void extractFormantsInternal(HarmonicProfile& profile) {
        extractFormantsFromSpectrum(profile.formants);

        // Estimate bandwidths (simplified)
        profile.formantBandwidths.resize(profile.formants.size());
        for (size_t i = 0; i < profile.formants.size(); ++i) {
            profile.formantBandwidths[i] = profile.formants[i] * 0.1f;  // Rough estimate
        }
    }

    void extractFormantsFromSpectrum(std::vector<float>& formants) {
        formants.clear();

        // Simple peak picking for formant detection
        std::vector<size_t> peaks;

        // Find local maxima
        for (size_t i = 2; i < spectrum_.size() - 2; ++i) {
            if (spectrum_[i] > spectrum_[i - 1] && spectrum_[i] > spectrum_[i + 1]
                && spectrum_[i] > spectrum_[i - 2] && spectrum_[i] > spectrum_[i + 2]) {
                float frequency = frequencyBins_[i];
                if (frequency >= 200.0f && frequency <= 4000.0f) {  // Typical formant range
                    peaks.push_back(i);
                }
            }
        }

        // Sort peaks by magnitude and take the strongest ones
        std::sort(peaks.begin(), peaks.end(), [this](size_t a, size_t b) {
            return spectrum_[a] > spectrum_[b];
        });

        size_t numFormants = std::min(peaks.size(), config_.numFormants);
        for (size_t i = 0; i < numFormants; ++i) {
            formants.push_back(frequencyBins_[peaks[i]]);
        }

        // Sort formants by frequency
        std::sort(formants.begin(), formants.end());
    }

    void assessTonalQualitiesInternal(HarmonicProfile& profile) {
        assessTonalQualitiesFromSpectrum(profile.qualities);
    }

    void assessTonalQualitiesFromSpectrum(HarmonicProfile::TonalQualities& qualities) {
        // Rasp: High frequency energy and irregularity
        float highFreqEnergy = 0.0f;
        float totalEnergy = 0.0f;

        for (size_t i = 1; i < spectrum_.size(); ++i) {
            float freq = frequencyBins_[i];
            float energy = spectrum_[i] * spectrum_[i];
            totalEnergy += energy;

            if (freq > 2000.0f) {
                highFreqEnergy += energy;
            }
        }

        qualities.rasp =
            totalEnergy > 0.0f ? std::min(1.0f, highFreqEnergy / totalEnergy * 3.0f) : 0.0f;

        // Whine: Narrow spectral content, high spectral centroid
        float spectralCentroid = computeSpectralCentroid();
        qualities.whine = std::min(1.0f, spectralCentroid / 3000.0f);

        // Resonance: Strong harmonics, low inharmonicity
        qualities.resonance = 1.0f - std::min(1.0f, currentProfile_.inharmonicity * 10.0f);

        // Brightness: Spectral centroid relative to fundamental
        if (currentProfile_.fundamentalFreq > 0.0f) {
            qualities.brightness =
                std::min(1.0f, spectralCentroid / (currentProfile_.fundamentalFreq * 3.0f));
        }

        // Roughness: Spectral irregularity
        float spectralFlatness = computeSpectralFlatness();
        qualities.roughness = 1.0f - spectralFlatness;
    }

    float calculateConfidence(const HarmonicProfile& profile) {
        float confidence = 0.0f;

        // Confidence based on harmonic strength
        if (!profile.harmonicRatios.empty()) {
            float avgRatio =
                std::accumulate(profile.harmonicRatios.begin(), profile.harmonicRatios.end(), 0.0f)
                / profile.harmonicRatios.size();
            confidence += std::min(1.0f, avgRatio) * 0.4f;
        }

        // Confidence based on HNR
        if (profile.harmonicToNoiseRatio > 0.0f) {
            confidence += std::min(1.0f, profile.harmonicToNoiseRatio / 20.0f) * 0.3f;
        }

        // Confidence based on low inharmonicity
        confidence += (1.0f - std::min(1.0f, profile.inharmonicity * 5.0f)) * 0.3f;

        return std::min(1.0f, confidence);
    }

    void updatePerformanceStats(double processingTime) {
        totalProcessingTime_ += processingTime;
        maxProcessingTime_ = std::max(maxProcessingTime_, processingTime);
    }

    void cleanup() {
#ifdef USE_FFTW3
        if (fftPlan_) {
            fftwf_destroy_plan(fftPlan_);
            fftPlan_ = nullptr;
        }
        if (fftInput_) {
            fftwf_free(fftInput_);
            fftInput_ = nullptr;
        }
        if (fftOutput_) {
            fftwf_free(fftOutput_);
            fftOutput_ = nullptr;
        }
#endif
#ifndef USE_FFTW3
        if (kissCfg_) {
            kiss_fftr_free(kissCfg_);
            kissCfg_ = nullptr;
        }
        kissOut_.clear();
#endif
    }
};

// Factory method implementation
HarmonicAnalyzer::Result<std::unique_ptr<HarmonicAnalyzer>, HarmonicAnalyzer::Error>
HarmonicAnalyzer::create(const Config& config) {
    try {
        if (config.sampleRate <= 0) {
            return Result<std::unique_ptr<HarmonicAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_SAMPLE_RATE));
        }

        if (config.fftSize == 0 || (config.fftSize & (config.fftSize - 1)) != 0) {
            return Result<std::unique_ptr<HarmonicAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_FFT_SIZE));
        }

        auto analyzer = std::make_unique<HarmonicAnalyzerImpl>(config);
        return Result<std::unique_ptr<HarmonicAnalyzer>, Error>(std::move(analyzer));

    } catch (const std::exception& e) {
        // DEBUG_LOG removed for compilation
        return Result<std::unique_ptr<HarmonicAnalyzer>, Error>(
            unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

// JSON export implementation
std::string HarmonicAnalyzer::exportToJson(const HarmonicProfile& profile) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"spectralCentroid\": " << profile.spectralCentroid << ",\n";
    json << "  \"spectralSpread\": " << profile.spectralSpread << ",\n";
    json << "  \"spectralRolloff\": " << profile.spectralRolloff << ",\n";
    json << "  \"spectralFlatness\": " << profile.spectralFlatness << ",\n";
    json << "  \"fundamentalFreq\": " << profile.fundamentalFreq << ",\n";
    json << "  \"harmonicToNoiseRatio\": " << profile.harmonicToNoiseRatio << ",\n";
    json << "  \"inharmonicity\": " << profile.inharmonicity << ",\n";
    json << "  \"isHarmonic\": " << (profile.isHarmonic ? "true" : "false") << ",\n";
    json << "  \"confidence\": " << profile.confidence << ",\n";
    json << "  \"timestamp\": " << profile.timestamp << ",\n";

    json << "  \"harmonicFreqs\": [";
    for (size_t i = 0; i < profile.harmonicFreqs.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << profile.harmonicFreqs[i];
    }
    json << "],\n";

    json << "  \"harmonicAmps\": [";
    for (size_t i = 0; i < profile.harmonicAmps.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << profile.harmonicAmps[i];
    }
    json << "],\n";

    json << "  \"formants\": [";
    for (size_t i = 0; i < profile.formants.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << profile.formants[i];
    }
    json << "],\n";

    json << "  \"tonalQualities\": {\n";
    json << "    \"rasp\": " << profile.qualities.rasp << ",\n";
    json << "    \"whine\": " << profile.qualities.whine << ",\n";
    json << "    \"resonance\": " << profile.qualities.resonance << ",\n";
    json << "    \"brightness\": " << profile.qualities.brightness << ",\n";
    json << "    \"roughness\": " << profile.qualities.roughness << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

}  // namespace huntmaster
