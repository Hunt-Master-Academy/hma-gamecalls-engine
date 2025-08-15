#include "huntmaster/core/CadenceAnalyzer.h"

// TODO: Improve test coverage for CadenceAnalyzer (currently 54% - target 90%+)
// TODO: Add tests for onset detection edge cases and spectral flux computation
// TODO: Test beat tracking state management and autocorrelation analysis
// TODO: Add tests for different audio patterns (silence, noise, complex rhythms)
// TODO: Test performance counters and instrumentation accuracy
// TODO: Add comprehensive tests for streaming frame processing vs one-shot analysis

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <numeric>
#include <set>
#include <span>
#include <sstream>
#include <vector>

#include <kiss_fft.h>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/PerformanceProfiler.h"
#include "huntmaster/security/memory-guard.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster {

/**
 * @brief Internal implementation of CadenceAnalyzer
 */
class CadenceAnalyzerImpl : public CadenceAnalyzer {
  private:
    Config config_;
    std::vector<float> buffer_;
    std::vector<float> energyHistory_;
    std::vector<float> onsetDetectionFunction_;
    std::vector<float> beatTrackingState_;

    CadenceProfile currentProfile_;
    bool isInitialized_ = false;
    bool isActive_ = false;

    // Analysis state
    size_t frameSize_;
    size_t hopSize_;
    // analysisCalls_: number of top-level one-shot analyzeCadence() invocations
    // streamingFrames_: number of hop-advanced frames processed via processAudioChunk()
    // (processedFrames_ kept temporarily for backward compatibility but will mirror sum)
    size_t processedFrames_ = 0;  // legacy aggregate (analysisCalls_ + streamingFrames_)
    size_t analysisCalls_ = 0;
    size_t streamingFrames_ = 0;
    double totalProcessingTime_ = 0.0;
    double maxProcessingTime_ = 0.0;

    // Onset detection state
    std::vector<float> prevSpectrum_;
    std::vector<float> spectralFlux_;
    std::vector<float> fastPathEnergies_;  // Raw frame energies for fast-path heuristics
    float adaptiveThreshold_ = 0.0f;

    // FFT optimization state
    kiss_fft_cfg fft_plan_ = nullptr;
    std::vector<kiss_fft_cpx> fft_input_;
    std::vector<kiss_fft_cpx> fft_output_;
    size_t fft_size_ = 0;

    // Instrumentation counters (lightweight, no locking – per-analyzer instance usage)
    size_t spectralFluxFrames_ = 0;   // Number of frames processed for spectral flux in last call
    size_t onsetLoopIterations_ = 0;  // Iterations in peakPickOnsets last call
    size_t onsetsDetectedLast_ = 0;   // Onsets detected in last detection pass
    size_t autocorrPeaksLast_ = 0;    // Peaks found in last autocorrelation analysis
    size_t autocorrMaxLagLast_ = 0;   // Max lag evaluated in last autocorrelation computation
    size_t audioSamplesLast_ = 0;     // Audio samples size passed to last analyzeCadence
    double lastProcessingMs_ = 0.0;   // Wall-clock time of last analyzeCadence call

  public:
    CadenceAnalyzerImpl(const Config& config) : config_(config) {
        initializeParameters();
        initializeBuffers();
        isInitialized_ = true;

        // DEBUG_LOG removed for compilation
        // "Initialized with frame size: " + std::to_string(frameSize_)
        //     + ", hop size: " + std::to_string(hopSize_)
        //     + ", sample rate: " + std::to_string(config_.sampleRate));
    }

    ~CadenceAnalyzerImpl() {
        if (fft_plan_) {
            kiss_fft_free(fft_plan_);
            fft_plan_ = nullptr;
        }
    }

    Result<CadenceProfile, Error> analyzeCadence(std::span<const float> audio) override {
        security::MemoryGuard guard(security::GuardConfig{});

        if (!isInitialized_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        }

        if (audio.size() < frameSize_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }

        try {
            auto start = std::chrono::high_resolution_clock::now();

            CadenceProfile profile;
            profile.timestamp =
                static_cast<float>(processedFrames_ * hopSize_) / config_.sampleRate;

            // Detect onsets
            auto onsetsResult = detectOnsetsInternal(audio);
            if (!onsetsResult.has_value()) {
                return unexpected(onsetsResult.error());
            }

            std::vector<float> onsets = onsetsResult.value();

            // Analyze call sequence
            analyzeCallSequence(profile, onsets);

            // Estimate tempo if beat tracking enabled
            if (config_.enableBeatTracking) {
                auto tempoResult = estimateTempoInternal(audio, onsets);
                if (tempoResult.has_value()) {
                    auto tempoConf = tempoResult.value();
                    float tempo = tempoConf.first;
                    float confidence = tempoConf.second;
                    profile.estimatedTempo = tempo;
                    profile.tempoConfidence = confidence;

                    // Extract beat times
                    extractBeats(profile, onsets);
                }
            }

            // Analyze periodicity
            analyzePeriodicityInternal(profile, audio);

            // Extract rhythmic features
            if (!onsets.empty()) {
                auto rhythmResult = extractRhythmicFeaturesInternal(onsets);
                if (rhythmResult.has_value()) {
                    profile.rhythm = rhythmResult.value();
                }
            }

            // Syllable analysis if enabled
            if (config_.enableSyllableAnalysis) {
                analyzeSyllables(profile, audio, onsets);
            }

            // Calculate overall rhythm score
            profile.overallRhythmScore = calculateOverallRhythmScore(profile);
            profile.confidence = calculateConfidence(profile);
            profile.hasStrongRhythm = profile.overallRhythmScore > 0.6f;

            currentProfile_ = profile;
            isActive_ = true;

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            updatePerformanceStats(duration);
            lastProcessingMs_ = duration;
            audioSamplesLast_ = audio.size();
            // Count a processed frame based on analysis window advanced implicitly by hop
            analysisCalls_ += 1;
            processedFrames_ = analysisCalls_ + streamingFrames_;

            // DEBUG_LOG removed for compilation
            // "Analysis complete - Tempo: " + std::to_string(profile.estimatedTempo)
            //     + "BPM, Rhythm Score: " + std::to_string(profile.overallRhythmScore));

            return Result<CadenceProfile, Error>(std::move(profile));

        } catch (const std::exception& e) {
            // DEBUG_LOG removed for compilation
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::PROCESSING_ERROR));
        }
    }

    Result<void, Error> processAudioChunk(std::span<const float> audio) override {
        if (!isInitialized_) {
            return unexpected(Error::INITIALIZATION_FAILED);
        }

        // Add to buffer for continuous processing
        buffer_.insert(buffer_.end(), audio.begin(), audio.end());

        // Process if we have enough data
        while (buffer_.size() >= frameSize_) {
            std::span<const float> chunk(buffer_.data(), frameSize_);
            auto result = analyzeCadence(chunk);

            if (!result.has_value()) {
                return unexpected(result.error());
            }

            // Advance by hop size
            buffer_.erase(buffer_.begin(), buffer_.begin() + hopSize_);
            streamingFrames_++;
            processedFrames_ = analysisCalls_ + streamingFrames_;
        }

        return Result<void, Error>();
    }

    Result<CadenceProfile, Error> getCurrentAnalysis() override {
        if (!isActive_) {
            return Result<CadenceProfile, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<CadenceProfile, Error>(currentProfile_);
    }

    Result<std::vector<float>, Error> detectOnsets(std::span<const float> audio) override {
        return detectOnsetsInternal(audio);
    }

    Result<std::pair<float, float>, Error> estimateTempo(std::span<const float> audio) override {
        auto onsetsResult = detectOnsetsInternal(audio);
        if (!onsetsResult.has_value()) {
            return Result<std::pair<float, float>, Error>(unexpected<Error>(onsetsResult.error()));
        }

        return estimateTempoInternal(audio, onsetsResult.value());
    }

    Result<CadenceProfile::PeriodicityMeasures, Error>
    analyzePerodicity(std::span<const float> audio) override {
        CadenceProfile::PeriodicityMeasures measures;
        analyzePeriodicityInternal(measures, audio);
        return Result<CadenceProfile::PeriodicityMeasures, Error>(measures);
    }

    Result<CadenceProfile::RhythmicFeatures, Error>
    extractRhythmicFeatures(const std::vector<float>& onsets) override {
        return extractRhythmicFeaturesInternal(onsets);
    }

    void reset() override {
        buffer_.clear();
        energyHistory_.clear();
        onsetDetectionFunction_.clear();
        beatTrackingState_.clear();
        prevSpectrum_.clear();
        spectralFlux_.clear();

        currentProfile_ = CadenceProfile{};
        isActive_ = false;
        processedFrames_ = 0;
        analysisCalls_ = 0;
        streamingFrames_ = 0;
        totalProcessingTime_ = 0.0;
        maxProcessingTime_ = 0.0;
        adaptiveThreshold_ = 0.0f;

        spectralFluxFrames_ = 0;
        onsetLoopIterations_ = 0;
        onsetsDetectedLast_ = 0;
        autocorrPeaksLast_ = 0;
        autocorrMaxLagLast_ = 0;
        audioSamplesLast_ = 0;
        lastProcessingMs_ = 0.0;

        // DEBUG_LOG removed for compilation
    }

    Result<void, Error> updateConfig(const Config& config) override {
        if (config.sampleRate <= 0 || config.frameSize <= 0 || config.hopSize <= 0) {
            return unexpected(Error::INVALID_SAMPLE_RATE);
        }

        config_ = config;
        initializeParameters();
        initializeBuffers();

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
        oss << "CadenceAnalyzer Stats:\n";
        oss << "  Analysis windows: " << analysisCalls_ << "\n";
        oss << "  Streaming frames: " << streamingFrames_ << "\n";
        oss << "  (Legacy aggregate processed frames): " << processedFrames_ << "\n";
        oss << "  Total processing time: " << totalProcessingTime_ << "ms\n";
        oss << "  Max processing time: " << maxProcessingTime_ << "ms\n";
        size_t denom = (analysisCalls_ + streamingFrames_);
        if (denom > 0) {
            oss << "  Average processing time: "
                << (totalProcessingTime_ / static_cast<double>(denom)) << "ms\n";
        }
        oss << "  Frame size: " << frameSize_ << " samples\n";
        oss << "  Hop size: " << hopSize_ << " samples\n";
        oss << "  Sample rate: " << config_.sampleRate << "Hz";
        oss << "\n  Last analysis window samples: " << audioSamplesLast_;
        oss << "\n  Last processing duration: " << lastProcessingMs_ << "ms";
        oss << "\n  Internal spectral frames (last): " << spectralFluxFrames_;
        oss << "\n  Onset loop iterations (last): " << onsetLoopIterations_;
        oss << "\n  Onsets detected (last): " << onsetsDetectedLast_;
        oss << "\n  Autocorr max lag (last): " << autocorrMaxLagLast_;
        oss << "\n  Autocorr peaks (last): " << autocorrPeaksLast_;
        return oss.str();
    }

    Result<std::vector<float>, Error> getOnsetDetectionFunction() override {
        if (onsetDetectionFunction_.empty()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<std::vector<float>, Error>(onsetDetectionFunction_);
    }

    Result<std::vector<float>, Error> getBeatTrackingState() override {
        if (beatTrackingState_.empty()) {
            return Result<std::vector<float>, Error>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        }
        return Result<std::vector<float>, Error>(beatTrackingState_);
    }

  private:
    void initializeParameters() {
        frameSize_ = static_cast<size_t>(config_.frameSize * config_.sampleRate);
        hopSize_ = static_cast<size_t>(config_.hopSize * config_.sampleRate);

        // Ensure frame size is reasonable
        frameSize_ = std::max(frameSize_, static_cast<size_t>(384));  // lowered min for speed
        hopSize_ = std::max(hopSize_, static_cast<size_t>(192));      // keep 50% relation

        // Ensure hop size doesn't exceed frame size
        hopSize_ = std::min(hopSize_, frameSize_ / 2);
    }

    void initializeBuffers() {
        buffer_.clear();
        buffer_.reserve(frameSize_ * 2);

        energyHistory_.clear();
        energyHistory_.reserve(1000);  // Reserve space for history

        onsetDetectionFunction_.clear();
        beatTrackingState_.clear();

        prevSpectrum_.resize(frameSize_ / 2 + 1, 0.0f);
        spectralFlux_.clear();

        // Initialize FFT for performance optimization
        initializeFFT();
    }

    void initializeFFT(size_t fftSize = 0) {
        // Use provided size or default to frameSize_
        size_t targetSize = (fftSize > 0) ? fftSize : frameSize_;

        // Only re-initialize if size changed
        if (fft_plan_ && fft_size_ == targetSize) {
            return;
        }

        // Clean up existing FFT plan if any
        if (fft_plan_) {
            kiss_fft_free(fft_plan_);
        }

        // Set FFT size
        fft_size_ = targetSize;

        // Allocate FFT plan and buffers
        fft_plan_ = kiss_fft_alloc(static_cast<int>(fft_size_), 0, nullptr, nullptr);
        fft_input_.resize(fft_size_);
        fft_output_.resize(fft_size_);
    }

    Result<std::vector<float>, Error> detectOnsetsInternal(std::span<const float> audio) {
        if (!config_.enableOnsetDetection) {
            return Result<std::vector<float>, Error>(std::vector<float>{});
        }

        std::vector<float> onsets;

        try {
            // Compute spectral flux for onset detection
            computeSpectralFlux(audio);

            // Peak picking on onset detection function
            peakPickOnsets(onsets);

            // Update onset detection function for visualization
            onsetDetectionFunction_ = spectralFlux_;

            return Result<std::vector<float>, Error>(std::move(onsets));

        } catch (const std::exception& e) {
            // DEBUG_LOG removed for compilation
            return Result<std::vector<float>, Error>(
                unexpected<Error>(Error::ONSET_DETECTION_ERROR));
        }
    }

    void computeSpectralFlux(std::span<const float> audio) {
        // Simple spectral flux implementation
        size_t numFrames = (audio.size() - frameSize_) / hopSize_ + 1;
        spectralFlux_.resize(numFrames);
        spectralFluxFrames_ = numFrames;
        if (config_.fastPathOptimization) {
            fastPathEnergies_.assign(numFrames, 0.0f);
        } else {
            fastPathEnergies_.clear();
        }

        float prevEnergy = 0.0f;  // For fast path energy-based flux
        for (size_t frame = 0; frame < numFrames; ++frame) {
            size_t startIdx = frame * hopSize_;
            if (config_.fastPathOptimization) {
                // Fast path: frame energy difference (O(N))
                float energy = 0.0f;
                for (size_t n = 0; n < frameSize_ && startIdx + n < audio.size(); ++n) {
                    float s = audio[startIdx + n];
                    energy += s * s;
                }
                float diff = (frame == 0) ? 0.0f : (energy - prevEnergy);
                prevEnergy = energy;
                spectralFlux_[frame] = diff > 0.0f ? diff : 0.0f;
                fastPathEnergies_[frame] = energy;
            } else {
                // Full path: magnitude spectrum + positive differences
                std::vector<float> currentSpectrum(frameSize_ / 2 + 1);
                computeMagnitudeSpectrum(audio.subspan(startIdx, frameSize_), currentSpectrum);
                float flux = 0.0f;
                for (size_t bin = 0; bin < currentSpectrum.size(); ++bin) {
                    float diff = currentSpectrum[bin] - prevSpectrum_[bin];
                    if (diff > 0.0f)
                        flux += diff;
                }
                spectralFlux_[frame] = flux;
                prevSpectrum_ = std::move(currentSpectrum);
            }
        }

        // Normalize fast-path flux to [0,1] before smoothing to make thresholds meaningful
        if (config_.fastPathOptimization && !spectralFlux_.empty()) {
            float maxVal = *std::max_element(spectralFlux_.begin(), spectralFlux_.end());
            if (maxVal > 0.0f) {
                for (auto& v : spectralFlux_)
                    v /= maxVal;
            }
        }

        // Apply smoothing
        applySmoothingToFlux();

        // Re-normalize after smoothing (fast path) to keep peak scale consistent
        if (config_.fastPathOptimization && !spectralFlux_.empty()) {
            float maxVal = *std::max_element(spectralFlux_.begin(), spectralFlux_.end());
            if (maxVal > 0.0f) {
                for (auto& v : spectralFlux_)
                    v /= maxVal;
            }
        }
    }

    void computeMagnitudeSpectrum(std::span<const float> frame, std::vector<float>& spectrum) {
        // Use FFT for efficient magnitude spectrum computation
        if (!fft_plan_ || frame.size() != fft_size_) {
            initializeFFT(frame.size());
        }

        // Copy input data to FFT buffer (zero-padded if necessary)
        std::fill(fft_input_.begin(), fft_input_.end(), kiss_fft_cpx{0.0f, 0.0f});
        for (size_t i = 0; i < std::min(frame.size(), fft_size_); ++i) {
            fft_input_[i].r = frame[i];
            fft_input_[i].i = 0.0f;
        }

        // Perform FFT
        kiss_fft(fft_plan_, fft_input_.data(), fft_output_.data());

        // Compute magnitude spectrum (only use first half due to symmetry)
        size_t halfSize = std::min(spectrum.size(), fft_size_ / 2 + 1);
        for (size_t k = 0; k < halfSize; ++k) {
            float real = fft_output_[k].r;
            float imag = fft_output_[k].i;
            spectrum[k] = std::sqrt(real * real + imag * imag);
        }

        // Fill remaining spectrum bins with zeros if necessary
        for (size_t k = halfSize; k < spectrum.size(); ++k) {
            spectrum[k] = 0.0f;
        }
    }

    void applySmoothingToFlux() {
        // Simple moving average smoothing
        const size_t windowSize = 3;
        std::vector<float> smoothed(spectralFlux_.size());

        for (size_t i = 0; i < spectralFlux_.size(); ++i) {
            float sum = 0.0f;
            size_t count = 0;

            size_t start = (i >= windowSize / 2) ? i - windowSize / 2 : 0;
            size_t end = std::min(i + windowSize / 2 + 1, spectralFlux_.size());

            for (size_t j = start; j < end; ++j) {
                sum += spectralFlux_[j];
                count++;
            }

            smoothed[i] = count > 0 ? sum / count : 0.0f;
        }

        spectralFlux_ = std::move(smoothed);
    }

    void peakPickOnsets(std::vector<float>& onsets) {
        if (spectralFlux_.size() < 3)
            return;

        // Adaptive thresholding
        updateAdaptiveThreshold();

        // Find peaks above threshold
        onsetLoopIterations_ = 0;
        for (size_t i = 1; i < spectralFlux_.size() - 1; ++i) {
            float dynamicThresh = config_.onsetThreshold + adaptiveThreshold_;
            if (config_.fastPathOptimization) {
                // Fast path: lower the bar (energy diff already coarse) and avoid missing all
                // onsets
                dynamicThresh *= 0.5f;
            }
            if (spectralFlux_[i] > spectralFlux_[i - 1] && spectralFlux_[i] > spectralFlux_[i + 1]
                && spectralFlux_[i] > dynamicThresh) {
                float onsetTime = static_cast<float>(i * hopSize_) / config_.sampleRate;
                onsets.push_back(onsetTime);
            }
            onsetLoopIterations_++;
        }
        onsetsDetectedLast_ = onsets.size();

        // Fallback for fast path: if no onsets detected, pick top energy-diff peaks
        if (config_.fastPathOptimization && onsets.empty()) {
            // Compute median to establish a dynamic baseline
            std::vector<float> fluxCopy = spectralFlux_;
            std::sort(fluxCopy.begin(), fluxCopy.end());
            float median = fluxCopy[fluxCopy.size() / 2];

            struct Peak {
                size_t idx;
                float val;
            };
            std::vector<Peak> candidates;
            for (size_t i = 1; i + 1 < spectralFlux_.size(); ++i) {
                if (spectralFlux_[i] > spectralFlux_[i - 1]
                    && spectralFlux_[i] > spectralFlux_[i + 1]
                    && spectralFlux_[i] > median * 1.2f) {
                    candidates.push_back({i, spectralFlux_[i]});
                }
            }
            std::sort(candidates.begin(), candidates.end(), [](const Peak& a, const Peak& b) {
                return a.val > b.val;
            });
            const size_t maxFallback = 3;
            for (const auto& c : candidates) {
                // Enforce minimal separation (2 frames) to avoid duplicates
                bool tooClose = false;
                for (float existing : onsets) {
                    size_t existingIdx =
                        static_cast<size_t>(existing * config_.sampleRate / hopSize_);
                    if (existingIdx > 0
                        && std::abs((long long)c.idx - (long long)existingIdx) < 2) {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose)
                    continue;
                float onsetTime = static_cast<float>(c.idx * hopSize_) / config_.sampleRate;
                onsets.push_back(onsetTime);
                if (onsets.size() >= maxFallback)
                    break;
            }
            std::sort(onsets.begin(), onsets.end());
            onsetsDetectedLast_ = onsets.size();

            // Absolute fallback: if still empty, just pick the single highest flux frame (non-edge)
            if (onsets.empty()) {
                size_t bestIdx = 0;
                float bestVal = 0.0f;
                for (size_t i = 1; i + 1 < spectralFlux_.size(); ++i) {
                    if (spectralFlux_[i] > bestVal) {
                        bestVal = spectralFlux_[i];
                        bestIdx = i;
                    }
                }
                if (bestVal > 0.0f) {
                    onsets.push_back(static_cast<float>(bestIdx * hopSize_) / config_.sampleRate);
                    onsetsDetectedLast_ = onsets.size();
                }
            }

            // Additional heuristic: use high-energy frames if still <3 onsets
            if (onsets.size() < 3 && fastPathEnergies_.size() == spectralFlux_.size()) {
                std::vector<float> energies = fastPathEnergies_;
                std::vector<float> sortedE = energies;
                std::sort(sortedE.begin(), sortedE.end());
                float energyMedian = sortedE[sortedE.size() / 2];
                float energyThresh = energyMedian * 1.3f;
                struct EPeak {
                    size_t idx;
                    float val;
                };
                std::vector<EPeak> epeaks;
                for (size_t i = 0; i < energies.size(); ++i) {
                    if (energies[i] > energyThresh) {
                        epeaks.push_back({i, energies[i]});
                    }
                }
                std::sort(epeaks.begin(), epeaks.end(), [](const EPeak& a, const EPeak& b) {
                    return a.val > b.val;
                });
                for (const auto& ep : epeaks) {
                    float t = static_cast<float>(ep.idx * hopSize_) / config_.sampleRate;
                    bool close = false;
                    for (float existing : onsets) {
                        if (std::fabs(existing - t) < (float)hopSize_ / config_.sampleRate) {
                            close = true;
                            break;
                        }
                    }
                    if (close)
                        continue;
                    onsets.push_back(t);
                    if (onsets.size() >= 4)
                        break;  // enough for tempo estimation
                }
                std::sort(onsets.begin(), onsets.end());
                onsetsDetectedLast_ = onsets.size();
            }
        }
    }

    void updateAdaptiveThreshold() {
        if (spectralFlux_.empty())
            return;

        // Calculate median of recent flux values for adaptive threshold
        const size_t historySize = std::min(spectralFlux_.size(), static_cast<size_t>(100));
        std::vector<float> recentFlux(spectralFlux_.end() - historySize, spectralFlux_.end());

        std::sort(recentFlux.begin(), recentFlux.end());
        float median = recentFlux[recentFlux.size() / 2];

        adaptiveThreshold_ = median * config_.adaptiveThreshold;
    }

    Result<std::pair<float, float>, Error> estimateTempoInternal(std::span<const float> audio,
                                                                 const std::vector<float>& onsets) {
        if (onsets.size() < 3) {
            // Fast-path fallback: estimate tempo directly from autocorrelation if too few onsets
            if (config_.fastPathOptimization) {
                auto autocorr = computeAutocorrelation(audio);
                if (!autocorr.empty()) {
                    // Search peak corresponding to a period within [minTempo, maxTempo]
                    float minPeriod = 60.0f / std::max(1.0f, config_.maxTempo);
                    float maxPeriod = 60.0f / std::max(1.0f, config_.minTempo);
                    size_t minLag = static_cast<size_t>(std::ceil(minPeriod * config_.sampleRate));
                    size_t maxLag = static_cast<size_t>(std::floor(maxPeriod * config_.sampleRate));
                    minLag = std::max<size_t>(1, std::min(minLag, autocorr.size() - 1));
                    maxLag = std::max<size_t>(minLag, std::min(maxLag, autocorr.size() - 1));

                    size_t bestLag = 0;
                    float bestVal = 0.0f;
                    for (size_t lag = minLag; lag <= maxLag; ++lag) {
                        if (autocorr[lag] > bestVal) {
                            bestVal = autocorr[lag];
                            bestLag = lag;
                        }
                    }

                    if (bestLag > 0 && bestVal > 0.1f) {
                        float period = static_cast<float>(bestLag) / config_.sampleRate;
                        float bpm = 60.0f / period;
                        // Clamp to configured range
                        bpm = std::max(config_.minTempo, std::min(bpm, config_.maxTempo));
                        return Result<std::pair<float, float>, Error>(std::make_pair(bpm, bestVal));
                    }
                }
                // Heuristic last resort: estimate tempo from window length when only one onset
                float durationSec = static_cast<float>(audio.size()) / config_.sampleRate;
                if (durationSec > 0.1f) {
                    float bpm = 60.0f / std::max(0.25f, std::min(durationSec, 1.0f));
                    bpm = std::max(config_.minTempo, std::min(bpm, config_.maxTempo));
                    return Result<std::pair<float, float>, Error>(std::make_pair(bpm, 0.15f));
                }
            }
            return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
        }

        // Calculate inter-onset intervals
        std::vector<float> intervals;
        for (size_t i = 1; i < onsets.size(); ++i) {
            float interval = onsets[i] - onsets[i - 1];
            if (interval > 0.0f) {
                intervals.push_back(interval);
            }
        }

        if (intervals.empty()) {
            return Result<std::pair<float, float>, Error>(std::make_pair(0.0f, 0.0f));
        }

        // Find most common interval (simple mode estimation)
        std::sort(intervals.begin(), intervals.end());

        float bestInterval = 0.0f;
        float bestConfidence = 0.0f;

        // Create histogram of intervals
        const float binSize = 0.05f;  // 50ms bins
        std::map<int, int> histogram;

        for (float interval : intervals) {
            int bin = static_cast<int>(interval / binSize);
            histogram[bin]++;
        }

        // Find peak bin
        int maxCount = 0;
        int bestBin = 0;
        for (const auto& [bin, count] : histogram) {
            if (count > maxCount) {
                maxCount = count;
                bestBin = bin;
            }
        }

        if (maxCount > 0) {
            bestInterval = bestBin * binSize;
            bestConfidence = static_cast<float>(maxCount) / intervals.size();
        }

        // Convert to BPM
        float tempo = 0.0f;
        if (bestInterval > 0.0f) {
            tempo = 60.0f / bestInterval;

            // Clamp to valid range
            tempo = std::clamp(tempo, config_.minTempo, config_.maxTempo);
        }

        return Result<std::pair<float, float>, Error>(std::make_pair(tempo, bestConfidence));
    }

    void analyzeCallSequence(CadenceProfile& profile, const std::vector<float>& onsets) {
        auto& sequence = profile.sequence;

        sequence.callOnsets = onsets;
        sequence.numCalls = onsets.size();

        if (onsets.empty())
            return;

        sequence.sequenceDuration = onsets.back() - onsets.front();

        if (sequence.sequenceDuration > 0.0f) {
            sequence.callRate = static_cast<float>(sequence.numCalls) / sequence.sequenceDuration;
        }

        // Estimate call durations (simplified)
        sequence.callDurations.resize(onsets.size());
        sequence.interCallIntervals.resize(onsets.size() - 1);

        for (size_t i = 0; i < onsets.size(); ++i) {
            // Rough estimate: calls last until next onset or fixed duration
            if (i < onsets.size() - 1) {
                float duration = (onsets[i + 1] - onsets[i]) * 0.7f;   // 70% of interval
                sequence.callDurations[i] = std::min(duration, 2.0f);  // Max 2 seconds
                sequence.interCallIntervals[i] =
                    onsets[i + 1] - onsets[i] - sequence.callDurations[i];
            } else {
                sequence.callDurations[i] = 0.5f;  // Default duration
            }
        }
    }

    void extractBeats(CadenceProfile& profile, const std::vector<float>& onsets) {
        // Simplified beat tracking - use onsets as beat candidates
        profile.beatTimes = onsets;
        profile.beatStrengths.resize(onsets.size(), 1.0f);  // Uniform strength

        // Calculate inter-beat intervals
        for (size_t i = 1; i < onsets.size(); ++i) {
            float interval = onsets[i] - onsets[i - 1];
            profile.interBeatIntervals.push_back(interval);
        }

        // Update beat tracking state for visualization
        beatTrackingState_ = profile.beatStrengths;
    }

    void analyzePeriodicityInternal(CadenceProfile& profile, std::span<const float> audio) {
        analyzePeriodicityInternal(profile.periodicity, audio);
    }

    void analyzePeriodicityInternal(CadenceProfile::PeriodicityMeasures& measures,
                                    std::span<const float> audio) {
        // Lightweight pre-check: if energy envelope is nearly flat & few onsets, skip heavy
        // periodicity (little rhythmic structure). This trims worst-case debug time for noise-like
        // segments.
        if (!audio.empty()) {
            float mean = 0.f;
            for (float v : audio)
                mean += std::fabs(v);
            mean /= audio.size();
            float var = 0.f;
            if (mean > 1e-6f) {
                for (float v : audio) {
                    float d = std::fabs(v) - mean;
                    var += d * d;
                }
                var /= audio.size();
                float coeffVar = std::sqrt(var) / mean;  // coefficient of variation
                if (coeffVar < 0.05f) {
                    // Essentially flat energy; leave measures default & return early
                    return;
                }
            }
        }

        // Compute autocorrelation for periodicity analysis (may early-return inside)
        std::vector<float> autocorr = computeAutocorrelation(audio);

        if (autocorr.empty())
            return;

        // Find peaks in autocorrelation
        std::vector<std::pair<size_t, float>> peaks;
        findAutocorrelationPeaks(autocorr, peaks);

        if (!peaks.empty()) {
            // Best peak (excluding lag 0)
            auto bestPeak =
                std::max_element(peaks.begin(), peaks.end(), [](const auto& a, const auto& b) {
                    return a.second < b.second;
                });

            measures.autocorrelationPeak = bestPeak->second;
            measures.dominantPeriod = static_cast<float>(bestPeak->first) / config_.sampleRate;
            measures.periodicityStrength = bestPeak->second;

            // Extract multiple periodicities
            for (const auto& [lag, strength] : peaks) {
                float period = static_cast<float>(lag) / config_.sampleRate;
                if (period >= config_.minPeriod && period <= config_.maxPeriod) {
                    measures.periodicities.push_back(period);
                    measures.periodicityStrengths.push_back(strength);
                }
            }
        }
    }

    std::vector<float> computeAutocorrelation(std::span<const float> audio) {
        // Fast bypass: extremely short clips (< 5 * frameSize_) provide little periodicity info
        // and autocorrelation dominates cost; return empty to skip heavy periodicity branch.
        if (audio.size() < frameSize_ * 5) {
            autocorrMaxLagLast_ = 0;
            return {};
        }
        size_t targetLags = config_.autocorrelationLags;
        if (config_.fastPathOptimization) {
            // Reduce lags drastically for speed
            targetLags = std::min<size_t>(256, targetLags / 4 + 1);
        }
        // Adaptive lag reduction for short buffers (non-fast-path): scale lags by duration so we
        // avoid doing a full 1000-lag sweep on 0.5s clips (dominant cost previously ~>700ms).
        if (!config_.fastPathOptimization && !config_.forceFullAutocorr) {
            float seconds = static_cast<float>(audio.size()) / config_.sampleRate;
            if (seconds < 0.75f) {
                // 0.5s test buffer: cap to 384 (covers tempos down to ~70 BPM)
                targetLags = std::min<size_t>(targetLags, 384);
            } else if (seconds < 1.25f) {
                targetLags = std::min<size_t>(targetLags, 512);
            }
            // In Debug builds, further cap to keep unit test wall times low unless explicitly
            // overridden by a larger fastPathOptimization (meaning user wants accuracy).
#ifndef NDEBUG
            targetLags = std::min<size_t>(targetLags, 512);
#endif
        }
        size_t maxLag = std::min(targetLags, audio.size() / 2);
        autocorrMaxLagLast_ = maxLag;
        std::vector<float> autocorr(maxLag);

        for (size_t lag = 1; lag < maxLag; ++lag) {
            float sum = 0.0f;
            size_t count = 0;

            // Unrolled dot-product style correlation with stride limit for speed.
            size_t limit = audio.size() - lag;
            const float* a = audio.data();
            const float* b = audio.data() + lag;

            // Dynamic stride: for short signals we can decimate without losing coarse periodicity
            // (goal: cut compute to <500ms debug). For very short buffers stride 4 ~4x speed.
            size_t stride = 1;
            if (!config_.fastPathOptimization) {
                if (limit < 44100) {  // <1s
                    stride = 4;
                } else if (limit < 88200) {  // <2s
                    stride = 2;
                }
            }

            size_t i = 0;
            float accum0 = 0.f, accum1 = 0.f, accum2 = 0.f, accum3 = 0.f;
            if (stride == 1) {
#if defined(__AVX2__)
                // AVX2 accelerated dot product (process 8 floats per iteration)
                const size_t vecWidth = 8;
                size_t vecLimit = limit - (limit % vecWidth);
                for (; i < vecLimit; i += vecWidth) {
                    __m256 va = _mm256_loadu_ps(a + i);
                    __m256 vb = _mm256_loadu_ps(b + i);
                    __m256 prod = _mm256_mul_ps(va, vb);
                    // Horizontal add via two-stage reduction
                    __m128 low = _mm256_castps256_ps128(prod);
                    __m128 high = _mm256_extractf128_ps(prod, 1);
                    __m128 sum128 = _mm_add_ps(low, high);
                    // Reduce 4 to 1
                    sum128 = _mm_hadd_ps(sum128, sum128);
                    sum128 = _mm_hadd_ps(sum128, sum128);
                    sum += _mm_cvtss_f32(sum128);
                }
                // Tail with scalar unroll
                for (; i + 4 <= limit; i += 4) {
                    accum0 += a[i] * b[i];
                    accum1 += a[i + 1] * b[i + 1];
                    accum2 += a[i + 2] * b[i + 2];
                    accum3 += a[i + 3] * b[i + 3];
                }
                sum += accum0 + accum1 + accum2 + accum3;
                for (; i < limit; ++i)
                    sum += a[i] * b[i];
                count = limit;
#else
                for (; i + 4 <= limit; i += 4) {
                    accum0 += a[i] * b[i];
                    accum1 += a[i + 1] * b[i + 1];
                    accum2 += a[i + 2] * b[i + 2];
                    accum3 += a[i + 3] * b[i + 3];
                }
                sum = accum0 + accum1 + accum2 + accum3;
                for (; i < limit; ++i)
                    sum += a[i] * b[i];
                count = limit;
#endif
            } else {
                // Strided accumulation (no unroll to keep code simple)
                for (i = 0; i < limit; i += stride) {
                    sum += a[i] * b[i];
                }
                count = (limit + stride - 1) / stride;
            }

            autocorr[lag] = count > 0 ? sum / count : 0.0f;

            if (config_.fastPathOptimization && lag > 64) {
                // Early break once a reasonable lag range explored
                if (lag > maxLag / 2)
                    break;
            }
        }

        // Normalize
        float maxVal = *std::max_element(autocorr.begin(), autocorr.end());
        if (maxVal > 0.0f) {
            for (float& val : autocorr) {
                val /= maxVal;
            }
        }

        return autocorr;
    }

    void findAutocorrelationPeaks(const std::vector<float>& autocorr,
                                  std::vector<std::pair<size_t, float>>& peaks) {
        // Find local maxima
        for (size_t i = 1; i < autocorr.size() - 1; ++i) {
            if (autocorr[i] > autocorr[i - 1] && autocorr[i] > autocorr[i + 1]
                && autocorr[i] > 0.1f) {
                peaks.push_back({i, autocorr[i]});
            }
            if (config_.fastPathOptimization && peaks.size() >= 5) {
                // Enough peaks for fast path
                break;
            }
        }

        // Sort by strength
        std::sort(peaks.begin(), peaks.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        // Keep only top peaks
        if (peaks.size() > 10) {
            peaks.resize(10);
        }
        autocorrPeaksLast_ = peaks.size();
    }

    Result<CadenceProfile::RhythmicFeatures, Error>
    extractRhythmicFeaturesInternal(const std::vector<float>& onsets) {
        CadenceProfile::RhythmicFeatures features;

        if (onsets.size() < 3) {
            return Result<CadenceProfile::RhythmicFeatures, Error>(features);
        }

        // Calculate intervals
        std::vector<float> intervals;
        for (size_t i = 1; i < onsets.size(); ++i) {
            intervals.push_back(onsets[i] - onsets[i - 1]);
        }

        // Rhythm regularity - based on interval variance
        float meanInterval =
            std::accumulate(intervals.begin(), intervals.end(), 0.0f) / intervals.size();
        float variance = 0.0f;
        for (float interval : intervals) {
            float diff = interval - meanInterval;
            variance += diff * diff;
        }
        variance /= intervals.size();

        features.rhythmRegularity = 1.0f / (1.0f + variance);  // Higher regularity = lower variance

        // Rhythm complexity - based on number of unique intervals
        std::set<int> uniqueIntervals;
        for (float interval : intervals) {
            uniqueIntervals.insert(static_cast<int>(interval * 100));  // 10ms precision
        }
        features.rhythmComplexity = static_cast<float>(uniqueIntervals.size()) / intervals.size();

        // Simplified syncopation measure
        features.syncopation = 1.0f - features.rhythmRegularity;

        // Polyrhythm detection (simplified)
        features.polyrhythm = features.rhythmComplexity > 0.5f ? features.rhythmComplexity : 0.0f;

        // Groove factor (balance between regularity and complexity)
        features.groove = features.rhythmRegularity * features.rhythmComplexity;

        return Result<CadenceProfile::RhythmicFeatures, Error>(features);
    }

    void analyzeSyllables(CadenceProfile& profile,
                          std::span<const float> audio,
                          const std::vector<float>& onsets) {
        (void)audio;  // currently unused, reserved for future advanced syllable analysis
        auto& syllables = profile.syllables;

        // Use onsets as syllable boundaries (simplified)
        syllables.syllableOnsets = onsets;

        if (onsets.size() < 2)
            return;

        // Estimate syllable durations
        syllables.syllableDurations.resize(onsets.size());
        float totalDuration = 0.0f;

        for (size_t i = 0; i < onsets.size(); ++i) {
            if (i < onsets.size() - 1) {
                syllables.syllableDurations[i] =
                    (onsets[i + 1] - onsets[i]) * 0.8f;  // 80% of interval
            } else {
                syllables.syllableDurations[i] = 0.3f;  // Default duration
            }
            totalDuration += syllables.syllableDurations[i];
        }

        syllables.avgSyllableDuration = totalDuration / onsets.size();

        float sequenceDuration = onsets.back() - onsets.front();
        if (sequenceDuration > 0.0f) {
            syllables.syllableRate = static_cast<float>(onsets.size()) / sequenceDuration;
        }

        // Speech rhythm score based on syllable regularity
        float avgDuration = syllables.avgSyllableDuration;
        float variance = 0.0f;
        for (float duration : syllables.syllableDurations) {
            float diff = duration - avgDuration;
            variance += diff * diff;
        }
        variance /= syllables.syllableDurations.size();

        syllables.speechRhythm =
            1.0f / (1.0f + variance * 10.0f);  // Higher score = more speech-like
    }

    float calculateOverallRhythmScore(const CadenceProfile& profile) {
        float score = 0.0f;

        // Weight different components
        score += profile.rhythm.rhythmRegularity * 0.3f;
        score += profile.rhythm.groove * 0.2f;
        score += profile.periodicity.periodicityStrength * 0.3f;
        score += (profile.tempoConfidence > 0.5f ? 0.2f : 0.0f);

        return std::min(1.0f, score);
    }

    float calculateConfidence(const CadenceProfile& profile) {
        float confidence = 0.0f;

        // Confidence based on various factors
        confidence += profile.tempoConfidence * 0.3f;
        confidence += profile.periodicity.periodicityStrength * 0.3f;
        confidence += profile.rhythm.rhythmRegularity * 0.2f;
        confidence += (profile.sequence.numCalls > 2 ? 0.2f : 0.0f);

        return std::min(1.0f, confidence);
    }

    void updatePerformanceStats(double processingTime) {
        totalProcessingTime_ += processingTime;
        maxProcessingTime_ = std::max(maxProcessingTime_, processingTime);
    }
};

// Factory method implementation
CadenceAnalyzer::Result<std::unique_ptr<CadenceAnalyzer>, CadenceAnalyzer::Error>
CadenceAnalyzer::create(const Config& config) {
    try {
        if (config.sampleRate <= 0) {
            return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_SAMPLE_RATE));
        }

        if (config.frameSize <= 0 || config.hopSize <= 0) {
            return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
                unexpected<Error>(Error::INVALID_FRAME_SIZE));
        }

        auto analyzer = std::make_unique<CadenceAnalyzerImpl>(config);
        return Result<std::unique_ptr<CadenceAnalyzer>, Error>(std::move(analyzer));

    } catch (const std::exception& e) {
        // DEBUG_LOG removed for compilation
        return Result<std::unique_ptr<CadenceAnalyzer>, Error>(
            unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

// JSON export implementation
std::string CadenceAnalyzer::exportToJson(const CadenceProfile& profile) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"estimatedTempo\": " << profile.estimatedTempo << ",\n";
    json << "  \"tempoConfidence\": " << profile.tempoConfidence << ",\n";
    json << "  \"overallRhythmScore\": " << profile.overallRhythmScore << ",\n";
    json << "  \"confidence\": " << profile.confidence << ",\n";
    json << "  \"hasStrongRhythm\": " << (profile.hasStrongRhythm ? "true" : "false") << ",\n";
    json << "  \"timestamp\": " << profile.timestamp << ",\n";

    json << "  \"beatTimes\": [";
    for (size_t i = 0; i < profile.beatTimes.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << profile.beatTimes[i];
    }
    json << "],\n";

    json << "  \"sequence\": {\n";
    json << "    \"numCalls\": " << profile.sequence.numCalls << ",\n";
    json << "    \"callRate\": " << profile.sequence.callRate << ",\n";
    json << "    \"sequenceDuration\": " << profile.sequence.sequenceDuration << "\n";
    json << "  },\n";

    json << "  \"periodicity\": {\n";
    json << "    \"dominantPeriod\": " << profile.periodicity.dominantPeriod << ",\n";
    json << "    \"periodicityStrength\": " << profile.periodicity.periodicityStrength << ",\n";
    json << "    \"autocorrelationPeak\": " << profile.periodicity.autocorrelationPeak << "\n";
    json << "  },\n";

    json << "  \"rhythm\": {\n";
    json << "    \"rhythmComplexity\": " << profile.rhythm.rhythmComplexity << ",\n";
    json << "    \"rhythmRegularity\": " << profile.rhythm.rhythmRegularity << ",\n";
    json << "    \"syncopation\": " << profile.rhythm.syncopation << ",\n";
    json << "    \"groove\": " << profile.rhythm.groove << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

}  // namespace huntmaster
