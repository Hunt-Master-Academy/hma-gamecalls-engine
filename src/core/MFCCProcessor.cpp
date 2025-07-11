// File: MFCCProcessor.cpp
#include "huntmaster/core/MFCCProcessor.h"

#ifdef HAVE_KISSFFT
#include "kiss_fftr.h"
#endif

#include <cmath>
#include <algorithm>
#include <numeric>

// Define M_PI if not defined (common issue with some compilers)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster
{

// Private implementation class
class MFCCProcessor::Impl
{
public:
    Config config;

#ifdef HAVE_KISSFFT
    kiss_fftr_cfg fftConfig = nullptr;
    std::vector<kiss_fft_cpx> fftOutput;
#endif

    std::vector<float> window;
    std::vector<float> melFilterBank;
    std::vector<int> filterBankIndices;
    std::vector<float> dctMatrix;
    std::vector<float> powerSpectrum;
    std::vector<float> melEnergies;

    Impl(const Config& cfg) : config(cfg)
    {
        if (config.high_freq == 0.0f) {
            config.high_freq = config.sample_rate / 2.0f;
        }

#ifdef HAVE_KISSFFT
        fftConfig = kiss_fftr_alloc(config.frame_size, 0, nullptr, nullptr);
        fftOutput.resize(config.frame_size / 2 + 1);
#endif

        window.resize(config.frame_size);
        for (size_t i = 0; i < config.frame_size; ++i) {
            window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (config.frame_size - 1));
        }

        initializeMelFilterBank();
        initializeDCTMatrix();

        powerSpectrum.resize(config.frame_size / 2 + 1);
        melEnergies.resize(config.num_filters);
    }

    ~Impl()
    {
#ifdef HAVE_KISSFFT
        if (fftConfig) {
            kiss_fftr_free(fftConfig);
        }
#endif
    }

    void initializeMelFilterBank() {
        auto freqToMel = [](float freq) { return 2595.0f * log10f(1.0f + freq / 700.0f); };
        auto melToFreq = [](float mel) { return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f); };

        float melLow = freqToMel(config.low_freq);
        float melHigh = freqToMel(config.high_freq);
        float melStep = (melHigh - melLow) / (config.num_filters + 1);

        std::vector<float> melPoints;
        for (size_t i = 0; i < config.num_filters + 2; ++i) {
            melPoints.push_back(melLow + i * melStep);
        }

        std::vector<float> freqPoints;
        for (float mel : melPoints) {
            freqPoints.push_back(melToFreq(mel));
        }

        filterBankIndices.clear();
        for (float freq : freqPoints) {
            int bin = static_cast<int>(freq * config.frame_size / config.sample_rate);
            filterBankIndices.push_back(bin);
        }

        size_t numBins = config.frame_size / 2 + 1;
        melFilterBank.assign(config.num_filters * numBins, 0.0f);

        for (size_t i = 0; i < config.num_filters; ++i) {
            int startBin = filterBankIndices[i];
            int centerBin = filterBankIndices[i + 1];
            int endBin = filterBankIndices[i + 2];

            // Add check to prevent division by zero
            float left_slope_denom = static_cast<float>(centerBin - startBin);
            if (left_slope_denom > 1e-6f) { // Use epsilon for float comparison
                for (int bin = startBin; bin < centerBin; ++bin) {
                    if (bin >= 0 && (size_t)bin < numBins) {
                        melFilterBank[i * numBins + bin] = (bin - startBin) / left_slope_denom;
                    }
                }
            }

            float right_slope_denom = static_cast<float>(endBin - centerBin);
            if (right_slope_denom > 1e-6f) {
                for (int bin = centerBin; bin < endBin; ++bin) {
                    if (bin >= 0 && (size_t)bin < numBins) {
                        melFilterBank[i * numBins + bin] = (endBin - bin) / right_slope_denom;
                    }
                }
            }
        }
    }

    void initializeDCTMatrix() {
        dctMatrix.resize(config.num_coefficients * config.num_filters);
        float scale1 = sqrtf(1.0f / config.num_filters);
        float scale2 = sqrtf(2.0f / config.num_filters);

        for (size_t i = 0; i < config.num_coefficients; ++i) {
            for (size_t j = 0; j < config.num_filters; ++j) {
                float val = cosf(M_PI * i * (j + 0.5f) / config.num_filters);
                dctMatrix[i * config.num_filters + j] = val * (i == 0 ? scale1 : scale2);
            }
        }
    }

    huntmaster::expected<FeatureVector, MFCCError> extractFeatures(std::span<const float> audio_frame) {
        if (audio_frame.size() != config.frame_size) {
            return huntmaster::unexpected(MFCCError::INVALID_INPUT);
        }

#ifdef HAVE_KISSFFT
        std::vector<float> windowedFrame(config.frame_size);
        for (size_t i = 0; i < config.frame_size; ++i) {
            windowedFrame[i] = audio_frame[i] * window[i];
        }

        kiss_fftr(fftConfig, windowedFrame.data(), fftOutput.data());

        for (size_t i = 0; i < powerSpectrum.size(); ++i) {
            powerSpectrum[i] = fftOutput[i].r * fftOutput[i].r + fftOutput[i].i * fftOutput[i].i;
        }

        for (size_t i = 0; i < config.num_filters; ++i) {
            // Use std::inner_product for a clearer and more robust dot product calculation.
            auto filter_row_start = melFilterBank.begin() + i * powerSpectrum.size();
            melEnergies[i] = logf(std::inner_product(
                filter_row_start, 
                filter_row_start + powerSpectrum.size(),
                powerSpectrum.begin(), 
                0.0f) + 1e-10f);
        }

        FeatureVector coefficients(config.num_coefficients, 0.0f);
        for (size_t i = 0; i < config.num_coefficients; ++i) {
            for (size_t j = 0; j < config.num_filters; ++j) {
                coefficients[i] += dctMatrix[i * config.num_filters + j] * melEnergies[j];
            }
        }
        return coefficients;
#else
        return huntmaster::unexpected(MFCCError::FFT_FAILED);
#endif
    }
};

// Public Interface Implementation
MFCCProcessor::MFCCProcessor(const Config& config) : pimpl_(std::make_unique<Impl>(config)) {}
MFCCProcessor::~MFCCProcessor() = default;
MFCCProcessor::MFCCProcessor(MFCCProcessor&&) noexcept = default;
MFCCProcessor& MFCCProcessor::operator=(MFCCProcessor&&) noexcept = default;

huntmaster::expected<MFCCProcessor::FeatureVector, MFCCError> MFCCProcessor::extractFeatures(std::span<const float> audio_frame) {
    return pimpl_->extractFeatures(audio_frame);
}

huntmaster::expected<MFCCProcessor::FeatureMatrix, MFCCError> MFCCProcessor::extractFeaturesFromBuffer(std::span<const float> audio_buffer, size_t hop_size) {
    if (audio_buffer.empty()) return huntmaster::unexpected(MFCCError::INVALID_INPUT);

    FeatureMatrix all_features;
    const size_t frame_size = pimpl_->config.frame_size;
    all_features.reserve(audio_buffer.size() / hop_size); // Reserve an approximate size

    for (size_t offset = 0; offset + frame_size <= audio_buffer.size(); offset += hop_size) {
        auto frame_span = audio_buffer.subspan(offset, frame_size);
        auto features_result = extractFeatures(frame_span);
        if (features_result) {
            all_features.push_back(std::move(*features_result));
        } else {
            // Propagate the first error encountered
            return huntmaster::unexpected(features_result.error());
        }
    }
    return all_features;
}

void MFCCProcessor::clearCache() { /* Caching logic to be implemented */ }
size_t MFCCProcessor::getCacheSize() const noexcept { return 0; }

} // namespace huntmaster
