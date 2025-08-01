#include "huntmaster/core/SpectrogramProcessor.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef HAVE_KISSFFT
#include "kiss_fftr.h"
#endif

#include "huntmaster/core/ComponentErrorHandler.h"
#include "huntmaster/core/DebugLogger.h"

namespace huntmaster {

// PIMPL implementation
struct SpectrogramProcessor::Impl {
#ifdef HAVE_KISSFFT
    kiss_fftr_cfg fft_config = nullptr;
    std::vector<kiss_fft_cpx> fft_output;
#endif
    std::vector<float> window_function;
    std::vector<float> windowed_frame;
    std::vector<float> magnitude_spectrum;
    bool initialized = false;

    explicit Impl(const Config& config) {
        try {
#ifdef HAVE_KISSFFT
            // Initialize KissFFT
            fft_config = kiss_fftr_alloc(config.window_size, 0, nullptr, nullptr);
            if (!fft_config) {
                LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                          "Failed to allocate KissFFT configuration");
                return;
            }

            // Allocate FFT output buffer (complex values)
            fft_output.resize(config.window_size / 2 + 1);
#else
            LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                      "KissFFT not available - HAVE_KISSFFT not defined");
            return;
#endif

            // Allocate working buffers
            windowed_frame.resize(config.window_size);
            magnitude_spectrum.resize(config.window_size / 2 + 1);

            // Generate Hanning window
            if (config.apply_window) {
                window_function.resize(config.window_size);
                for (size_t i = 0; i < config.window_size; ++i) {
                    window_function[i] =
                        0.5f * (1.0f - std::cos(2.0f * M_PI * i / (config.window_size - 1)));
                }
            }

            initialized = true;
            LOG_DEBUG(Component::SPECTROGRAM_PROCESSOR,
                      "SpectrogramProcessor initialized successfully");

        } catch (const std::exception& e) {
            LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                      "Failed to initialize SpectrogramProcessor: " + std::string(e.what()));
        }
    }

    ~Impl() {
#ifdef HAVE_KISSFFT
        if (fft_config) {
            kiss_fftr_free(fft_config);
            fft_config = nullptr;
        }
#endif
    }

    // Disable copy operations
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    // Enable move operations
    Impl(Impl&& other) noexcept
        : window_function(std::move(other.window_function)),
          windowed_frame(std::move(other.windowed_frame)),
          magnitude_spectrum(std::move(other.magnitude_spectrum)), initialized(other.initialized) {
#ifdef HAVE_KISSFFT
        fft_config = other.fft_config;
        fft_output = std::move(other.fft_output);
        other.fft_config = nullptr;
#endif
        other.initialized = false;
    }

    Impl& operator=(Impl&& other) noexcept {
        if (this != &other) {
#ifdef HAVE_KISSFFT
            if (fft_config) {
                kiss_fftr_free(fft_config);
            }
            fft_config = other.fft_config;
            fft_output = std::move(other.fft_output);
            other.fft_config = nullptr;
#endif
            window_function = std::move(other.window_function);
            windowed_frame = std::move(other.windowed_frame);
            magnitude_spectrum = std::move(other.magnitude_spectrum);
            initialized = other.initialized;
            other.initialized = false;
        }
        return *this;
    }
};

// Config validation
bool SpectrogramProcessor::Config::isValid() const noexcept {
    if (window_size == 0 || hop_size == 0 || sample_rate <= 0) {
        return false;
    }

    // Check if window_size is power of 2
    if ((window_size & (window_size - 1)) != 0) {
        return false;
    }

    // Hop size should be reasonable relative to window size
    if (hop_size > window_size) {
        return false;
    }

    // Frequency range validation
    float nyquist = sample_rate / 2.0f;
    if (max_frequency > nyquist || min_frequency < 0 || min_frequency >= max_frequency) {
        return false;
    }

    return true;
}

// Factory method
huntmaster::expected<std::unique_ptr<SpectrogramProcessor>, SpectrogramError>
SpectrogramProcessor::create(const Config& config) noexcept {
    try {
        if (!config.isValid()) {
            LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                      "Invalid SpectrogramProcessor configuration");
            return huntmaster::unexpected(SpectrogramError::INVALID_CONFIG);
        }

        auto processor = std::unique_ptr<SpectrogramProcessor>(new SpectrogramProcessor(config));

        if (!processor->impl_->initialized) {
            LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                      "Failed to initialize SpectrogramProcessor");
            return huntmaster::unexpected(SpectrogramError::FFT_FAILED);
        }

        return processor;
    } catch (const std::exception& e) {
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                  "Exception during SpectrogramProcessor creation: " + std::string(e.what()));
        return huntmaster::unexpected(SpectrogramError::PROCESSING_FAILED);
    }
}

// Constructor
SpectrogramProcessor::SpectrogramProcessor(const Config& config)
    : impl_(std::make_unique<Impl>(config)), config_(config) {}

// Destructor
SpectrogramProcessor::~SpectrogramProcessor() = default;

// Move constructor
SpectrogramProcessor::SpectrogramProcessor(SpectrogramProcessor&& other) noexcept
    : impl_(std::move(other.impl_)), config_(std::move(other.config_)) {}

// Move assignment
SpectrogramProcessor& SpectrogramProcessor::operator=(SpectrogramProcessor&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        config_ = std::move(other.config_);
    }
    return *this;
}

// Compute full spectrogram
huntmaster::expected<SpectrogramData, SpectrogramError>
SpectrogramProcessor::computeSpectrogram(std::span<const float> audio_data) noexcept {
    try {
        if (audio_data.empty() || !impl_->initialized) {
            return huntmaster::unexpected(SpectrogramError::INVALID_INPUT);
        }

        // Check if audio is too short for even one frame
        if (audio_data.size() < config_.window_size) {
            return huntmaster::unexpected(SpectrogramError::INVALID_INPUT);
        }

        SpectrogramData result;
        result.sample_rate = config_.sample_rate;
        result.hop_size_seconds = static_cast<float>(config_.hop_size) / config_.sample_rate;
        result.min_db = config_.db_floor;
        result.max_db = config_.db_ceiling;
        result.frequency_bins = config_.window_size / 2 + 1;

        // Calculate number of time frames
        const size_t total_samples = audio_data.size();
        const size_t num_frames = (total_samples > config_.window_size)
                                      ? (total_samples - config_.window_size) / config_.hop_size + 1
                                      : 1;

        result.time_bins = num_frames;
        result.magnitude_db.resize(num_frames);

        // Generate time axis
        result.time_axis.resize(num_frames);
        for (size_t i = 0; i < num_frames; ++i) {
            result.time_axis[i] = i * result.hop_size_seconds;
        }

        // Generate frequency axis
        result.frequency_axis.resize(result.frequency_bins);
        for (size_t i = 0; i < result.frequency_bins; ++i) {
            result.frequency_axis[i] = i * config_.sample_rate / (2.0f * result.frequency_bins);
        }

        // Process each frame
        for (size_t frame_idx = 0; frame_idx < num_frames; ++frame_idx) {
            size_t start_sample = frame_idx * config_.hop_size;
            size_t end_sample = std::min(start_sample + config_.window_size, total_samples);

            // Extract frame (zero-pad if needed)
            std::vector<float> frame(config_.window_size, 0.0f);
            std::copy(
                audio_data.begin() + start_sample, audio_data.begin() + end_sample, frame.begin());

            // Process frame
            auto frame_result = processFrame(frame);
            if (!frame_result.has_value()) {
                return huntmaster::unexpected(frame_result.error());
            }

            result.magnitude_db[frame_idx] = std::move(frame_result.value());
        }

        LOG_DEBUG(Component::SPECTROGRAM_PROCESSOR,
                  "Computed spectrogram: " + std::to_string(num_frames) + " frames, "
                      + std::to_string(result.frequency_bins) + " frequency bins");

        return result;

    } catch (const std::exception& e) {
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                  "Exception in computeSpectrogram: " + std::string(e.what()));
        return huntmaster::unexpected(SpectrogramError::PROCESSING_FAILED);
    }
}

// Process single frame
huntmaster::expected<std::vector<float>, SpectrogramError>
SpectrogramProcessor::processFrame(std::span<const float> audio_frame) noexcept {
    try {
        if (audio_frame.size() != config_.window_size || !impl_->initialized) {
            return huntmaster::unexpected(SpectrogramError::INVALID_INPUT);
        }

#ifdef HAVE_KISSFFT
        // Apply windowing if configured
        if (config_.apply_window && !impl_->window_function.empty()) {
            applyWindow(audio_frame, impl_->windowed_frame);
        } else {
            std::copy(audio_frame.begin(), audio_frame.end(), impl_->windowed_frame.begin());
        }

        // Compute magnitude spectrum
        if (!computeMagnitudeSpectrum(impl_->windowed_frame, impl_->magnitude_spectrum)) {
            return huntmaster::unexpected(SpectrogramError::FFT_FAILED);
        }

        // Convert to decibels
        auto result = magnitudeToDecibels(impl_->magnitude_spectrum, config_.db_floor);
        return result;

#else
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR, "KissFFT not available");
        return huntmaster::unexpected(SpectrogramError::FFT_FAILED);
#endif

    } catch (const std::exception& e) {
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                  "Exception in processFrame: " + std::string(e.what()));
        return huntmaster::unexpected(SpectrogramError::PROCESSING_FAILED);
    }
}

// Convert magnitude to decibels
std::vector<float>
SpectrogramProcessor::magnitudeToDecibels(std::span<const float> magnitude_spectrum,
                                          float floor_db) noexcept {
    std::vector<float> db_spectrum;
    db_spectrum.reserve(magnitude_spectrum.size());

    const float floor_linear = std::pow(10.0f, floor_db / 20.0f);

    for (float magnitude : magnitude_spectrum) {
        // Ensure minimum magnitude to avoid log(0)
        magnitude = std::max(magnitude, floor_linear);
        float db_value = 20.0f * std::log10(magnitude);
        db_spectrum.push_back(std::max(db_value, floor_db));
    }

    return db_spectrum;
}

// Generate color map
std::vector<std::vector<float>>
SpectrogramProcessor::generateColorMap(const SpectrogramData& spectrogram_data) noexcept {
    std::vector<std::vector<float>> color_map;

    // Validate input data
    if (spectrogram_data.time_bins == 0 || spectrogram_data.frequency_bins == 0
        || spectrogram_data.magnitude_db.empty()) {
        return color_map;
    }

    color_map.resize(spectrogram_data.time_bins);

    const float db_range = spectrogram_data.max_db - spectrogram_data.min_db;
    const float inv_range = (db_range > 0) ? 1.0f / db_range : 1.0f;

    for (size_t t = 0; t < spectrogram_data.time_bins; ++t) {
        color_map[t].resize(spectrogram_data.frequency_bins);

        // Ensure we have valid magnitude data for this time bin
        if (t < spectrogram_data.magnitude_db.size()
            && spectrogram_data.magnitude_db[t].size() >= spectrogram_data.frequency_bins) {
            for (size_t f = 0; f < spectrogram_data.frequency_bins; ++f) {
                // Normalize to [0,1] range
                float normalized =
                    (spectrogram_data.magnitude_db[t][f] - spectrogram_data.min_db) * inv_range;
                color_map[t][f] = std::clamp(normalized, 0.0f, 1.0f);
            }
        } else {
            // Fill with zeros if data is missing
            std::fill(color_map[t].begin(), color_map[t].end(), 0.0f);
        }
    }

    return color_map;
}

// Export for visualization
std::string SpectrogramProcessor::exportForVisualization(const SpectrogramData& spectrogram_data,
                                                         size_t max_time_bins,
                                                         size_t max_freq_bins) const noexcept {
    try {
        std::ostringstream json;
        json << std::fixed << std::setprecision(3);

        json << "{\n";
        json << "  \"type\": \"spectrogram\",\n";
        json << "  \"sampleRate\": " << spectrogram_data.sample_rate << ",\n";
        json << "  \"hopSizeSeconds\": " << spectrogram_data.hop_size_seconds << ",\n";
        json << "  \"timeBins\": " << spectrogram_data.time_bins << ",\n";
        json << "  \"frequencyBins\": " << spectrogram_data.frequency_bins << ",\n";
        json << "  \"minDb\": " << spectrogram_data.min_db << ",\n";
        json << "  \"maxDb\": " << spectrogram_data.max_db << ",\n";

        // Downsample if requested
        size_t time_step = (max_time_bins > 0 && spectrogram_data.time_bins > max_time_bins)
                               ? spectrogram_data.time_bins / max_time_bins
                               : 1;
        size_t freq_step = (max_freq_bins > 0 && spectrogram_data.frequency_bins > max_freq_bins)
                               ? spectrogram_data.frequency_bins / max_freq_bins
                               : 1;

        // Export magnitude data
        json << "  \"magnitude_db\": [\n";
        for (size_t t = 0; t < spectrogram_data.time_bins; t += time_step) {
            if (t > 0)
                json << ",\n";
            json << "    [";
            for (size_t f = 0; f < spectrogram_data.frequency_bins; f += freq_step) {
                if (f > 0)
                    json << ", ";
                json << spectrogram_data.magnitude_db[t][f];
            }
            json << "]";
        }
        json << "\n  ],\n";

        // Export time axis
        json << "  \"time_axis\": [";
        for (size_t t = 0; t < spectrogram_data.time_bins; t += time_step) {
            if (t > 0)
                json << ", ";
            json << spectrogram_data.time_axis[t];
        }
        json << "],\n";

        // Export frequency axis
        json << "  \"frequency_axis\": [";
        for (size_t f = 0; f < spectrogram_data.frequency_bins; f += freq_step) {
            if (f > 0)
                json << ", ";
            json << spectrogram_data.frequency_axis[f];
        }
        json << "],\n";

        // Export color map
        auto color_map = generateColorMap(spectrogram_data);
        json << "  \"color_map\": [\n";
        for (size_t t = 0; t < color_map.size(); t += time_step) {
            if (t > 0)
                json << ",\n";
            json << "    [";
            for (size_t f = 0; f < color_map[t].size(); f += freq_step) {
                if (f > 0)
                    json << ", ";
                json << color_map[t][f];
            }
            json << "]";
        }
        json << "\n  ],\n";

        json << "  \"min_db\": " << spectrogram_data.min_db << ",\n";
        json << "  \"max_db\": " << spectrogram_data.max_db << "\n";
        json << "}";

        return json.str();

    } catch (const std::exception& e) {
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                  "Exception in exportForVisualization: " + std::string(e.what()));
        return "{}";  // Return empty JSON on error
    }
}

// Reset processor state
void SpectrogramProcessor::reset() noexcept {
    if (impl_) {
        // Clear working buffers
        std::fill(impl_->windowed_frame.begin(), impl_->windowed_frame.end(), 0.0f);
        std::fill(impl_->magnitude_spectrum.begin(), impl_->magnitude_spectrum.end(), 0.0f);
#ifdef HAVE_KISSFFT
        std::fill(impl_->fft_output.begin(), impl_->fft_output.end(), kiss_fft_cpx{0.0f, 0.0f});
#endif
    }
}

// Private helper methods
void SpectrogramProcessor::applyWindow(std::span<const float> frame,
                                       std::span<float> windowed_frame) const noexcept {
    for (size_t i = 0; i < frame.size(); ++i) {
        windowed_frame[i] = frame[i] * impl_->window_function[i];
    }
}

bool SpectrogramProcessor::computeMagnitudeSpectrum(std::span<const float> windowed_frame,
                                                    std::span<float> magnitude_spectrum) noexcept {
    try {
#ifdef HAVE_KISSFFT
        // Perform FFT
        kiss_fftr(impl_->fft_config, windowed_frame.data(), impl_->fft_output.data());

        // Compute magnitude spectrum
        for (size_t i = 0; i < magnitude_spectrum.size(); ++i) {
            const auto& complex_val = impl_->fft_output[i];
            magnitude_spectrum[i] =
                std::sqrt(complex_val.r * complex_val.r + complex_val.i * complex_val.i);

            // Check for numerical issues
            if (!std::isfinite(magnitude_spectrum[i])) {
                LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                          "Non-finite value in magnitude spectrum at bin " + std::to_string(i));
                return false;
            }
        }

        return true;
#else
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR, "KissFFT not available");
        return false;
#endif
    } catch (const std::exception& e) {
        LOG_ERROR(Component::SPECTROGRAM_PROCESSOR,
                  "Exception in computeMagnitudeSpectrum: " + std::string(e.what()));
        return false;
    }
}

}  // namespace huntmaster
