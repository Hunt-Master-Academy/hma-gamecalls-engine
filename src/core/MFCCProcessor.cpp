// File: MFCCProcessor.cpp
#include "huntmaster/core/MFCCProcessor.h"

#include "huntmaster/core/ComponentErrorHandler.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/ErrorLogger.h"

#ifdef HAVE_KISSFFT
#include "kiss_fftr.h"
#endif

#include <algorithm>
#include <cmath>
#include <numeric>

// Define M_PI if not defined (common issue with some compilers)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Using centralized DebugLogger for consistent error reporting

namespace huntmaster {

// Private implementation class
class MFCCProcessor::Impl {
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

    Impl(const Config& cfg) : config(cfg) {
        // Validate configuration parameters
        if (config.sample_rate <= 0) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration(
                "sample_rate", std::to_string(config.sample_rate));
            throw std::invalid_argument("Invalid sample rate");
        }

        if (config.frame_size <= 0 || (config.frame_size & (config.frame_size - 1)) != 0) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration(
                "frame_size", std::to_string(config.frame_size));
            throw std::invalid_argument("Frame size must be a positive power of 2");
        }

        if (config.num_filters <= 0 || config.num_coefficients <= 0) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration(
                "filter_coefficients",
                "filters=" + std::to_string(config.num_filters)
                    + ", coeffs=" + std::to_string(config.num_coefficients));
            throw std::invalid_argument("Invalid filter or coefficient count");
        }

        if (config.high_freq == 0.0f) {
            config.high_freq = config.sample_rate / 2.0f;
        }

        if (config.high_freq > config.sample_rate / 2.0f) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration(
                "high_freq", std::to_string(config.high_freq));
            config.high_freq = config.sample_rate / 2.0f;
            LOG_WARN(Component::MFCC_PROCESSOR,
                     "High frequency clamped to Nyquist: " + std::to_string(config.high_freq));
        }

#ifdef HAVE_KISSFFT
        try {
            fftConfig = kiss_fftr_alloc(config.frame_size, 0, nullptr, nullptr);
            if (!fftConfig) {
                ComponentErrorHandler::MFCCProcessorErrors::logFFTInitializationFailure(
                    "kiss_fftr_alloc returned null");
                throw std::runtime_error("FFT initialization failed");
            }
            fftOutput.resize(config.frame_size / 2 + 1);
        } catch (const std::exception& e) {
            ComponentErrorHandler::MFCCProcessorErrors::logFFTInitializationFailure(e.what());
            throw;
        }
#else
        ComponentErrorHandler::MFCCProcessorErrors::logFFTInitializationFailure(
            "KissFFT not available - HAVE_KISSFFT not defined");
        throw std::runtime_error("FFT support not available");
#endif

        try {
            window.resize(config.frame_size);
            for (size_t i = 0; i < config.frame_size; ++i) {
                window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (config.frame_size - 1));
            }

            initializeMelFilterBank();
            initializeDCTMatrix();

            powerSpectrum.resize(config.frame_size / 2 + 1);
            melEnergies.resize(config.num_filters);

            LOG_INFO(Component::MFCC_PROCESSOR,
                     "MFCC processor initialized successfully - "
                         + std::to_string(config.num_filters) + " filters, "
                         + std::to_string(config.num_coefficients) + " coefficients");
        } catch (const std::bad_alloc& e) {
            ComponentErrorHandler::MFCCProcessorErrors::logMemoryExhaustion(0, 0);
            throw;
        } catch (const std::exception& e) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration("initialization",
                                                                                e.what());
            throw;
        }
    }

    ~Impl() {
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
            if (left_slope_denom > 1e-6f) {  // Use epsilon for float comparison
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

    huntmaster::expected<FeatureVector, MFCCError>
    extractFeatures(std::span<const float> audio_frame) {
        if (audio_frame.size() != config.frame_size) {
            ComponentErrorHandler::MFCCProcessorErrors::logInvalidInputSize(audio_frame.size(),
                                                                            config.frame_size);
            return huntmaster::unexpected(MFCCError::INVALID_INPUT);
        }

        // Validate input audio data
        bool hasValidData = false;
        float maxValue = 0.0f;
        for (const float& sample : audio_frame) {
            if (!std::isfinite(sample)) {
                ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionFailure(
                    config.frame_size, "Non-finite values in audio frame");
                return huntmaster::unexpected(MFCCError::INVALID_INPUT);
            }
            float absValue = std::abs(sample);
            if (absValue > maxValue) {
                maxValue = absValue;
            }
            if (absValue > 1e-8f) {
                hasValidData = true;
            }
        }

        if (!hasValidData) {
            LOG_DEBUG(Component::MFCC_PROCESSOR,
                      "Input frame contains only silence (max value: " + std::to_string(maxValue)
                          + ")");
        }

#ifdef HAVE_KISSFFT
        try {
            std::vector<float> windowedFrame(config.frame_size);
            for (size_t i = 0; i < config.frame_size; ++i) {
                windowedFrame[i] = audio_frame[i] * window[i];
            }

            kiss_fftr(fftConfig, windowedFrame.data(), fftOutput.data());

            for (size_t i = 0; i < powerSpectrum.size(); ++i) {
                powerSpectrum[i] =
                    fftOutput[i].r * fftOutput[i].r + fftOutput[i].i * fftOutput[i].i;

                // Check for numerical issues
                if (!std::isfinite(powerSpectrum[i])) {
                    ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionFailure(
                        config.frame_size, "Non-finite values in power spectrum");
                    return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
                }
            }

            // Apply mel filter bank
            for (size_t i = 0; i < config.num_filters; ++i) {
                try {
                    auto filter_row_start = melFilterBank.begin() + i * powerSpectrum.size();
                    melEnergies[i] =
                        logf(std::inner_product(filter_row_start,
                                                filter_row_start + powerSpectrum.size(),
                                                powerSpectrum.begin(),
                                                0.0f)
                             + 1e-10f);

                    if (!std::isfinite(melEnergies[i])) {
                        ComponentErrorHandler::MFCCProcessorErrors::logFilterBankError(
                            "Non-finite mel energy at filter " + std::to_string(i));
                        return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
                    }
                } catch (const std::exception& e) {
                    ComponentErrorHandler::MFCCProcessorErrors::logFilterBankError(
                        "Filter " + std::to_string(i) + " processing error: " + e.what());
                    return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
                }
            }

            // Apply DCT
            FeatureVector coefficients(config.num_coefficients, 0.0f);
            for (size_t i = 0; i < config.num_coefficients; ++i) {
                try {
                    for (size_t j = 0; j < config.num_filters; ++j) {
                        coefficients[i] += dctMatrix[i * config.num_filters + j] * melEnergies[j];
                    }

                    if (!std::isfinite(coefficients[i])) {
                        ComponentErrorHandler::MFCCProcessorErrors::logDCTError(
                            "Non-finite coefficient at index " + std::to_string(i));
                        return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
                    }
                } catch (const std::exception& e) {
                    ComponentErrorHandler::MFCCProcessorErrors::logDCTError(
                        "DCT coefficient " + std::to_string(i) + " error: " + e.what());
                    return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
                }
            }

            return coefficients;
        } catch (const std::exception& e) {
            ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionFailure(
                config.frame_size,
                "Unexpected error during MFCC extraction: " + std::string(e.what()));
            return huntmaster::unexpected(MFCCError::PROCESSING_FAILED);
        }
#else
        ComponentErrorHandler::MFCCProcessorErrors::logFFTInitializationFailure(
            "FFT not available - HAVE_KISSFFT not defined");
        return huntmaster::unexpected(MFCCError::FFT_FAILED);
#endif
    }
};

// Public Interface Implementation
MFCCProcessor::MFCCProcessor(const Config& config) : pimpl_(std::make_unique<Impl>(config)) {}
MFCCProcessor::~MFCCProcessor() = default;
MFCCProcessor::MFCCProcessor(MFCCProcessor&&) noexcept = default;
MFCCProcessor& MFCCProcessor::operator=(MFCCProcessor&&) noexcept = default;

huntmaster::expected<MFCCProcessor::FeatureVector, MFCCError>
MFCCProcessor::extractFeatures(std::span<const float> audio_frame) {
    LOG_DEBUG(Component::MFCC_PROCESSOR,
              "extractFeatures called with frame size: " + std::to_string(audio_frame.size()));
    auto result = pimpl_->extractFeatures(audio_frame);
    if (result.has_value()) {
        LOG_DEBUG(Component::MFCC_PROCESSOR,
                  "extractFeatures successful, feature vector size: "
                      + std::to_string(result->size()));
    } else {
        LOG_ERROR(Component::MFCC_PROCESSOR,
                  "extractFeatures failed - invalid input or processing error");
    }
    return result;
}

huntmaster::expected<MFCCProcessor::FeatureMatrix, MFCCError>
MFCCProcessor::extractFeaturesFromBuffer(std::span<const float> audio_buffer, size_t hop_size) {
    LOG_DEBUG(Component::MFCC_PROCESSOR,
              "extractFeaturesFromBuffer called with buffer size: "
                  + std::to_string(audio_buffer.size())
                  + ", hop_size: " + std::to_string(hop_size));
    if (audio_buffer.empty()) {
        LOG_ERROR(Component::MFCC_PROCESSOR, "extractFeaturesFromBuffer: empty buffer provided");
        return huntmaster::unexpected(MFCCError::INVALID_INPUT);
    }

    FeatureMatrix all_features;
    const size_t frame_size = pimpl_->config.frame_size;
    all_features.reserve(audio_buffer.size() / hop_size);  // Reserve an approximate size

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
size_t MFCCProcessor::getCacheSize() const noexcept {
    return 0;
}

}  // namespace huntmaster
