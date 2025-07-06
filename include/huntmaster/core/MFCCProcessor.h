// File: MFCCProcessor.h
#pragma once

#include <span>
#include <vector>
#include <array>
#include <expected>
#include <memory>
#include <unordered_map>
#include <optional>

namespace huntmaster
{

    enum class MFCCError
    {
        INVALID_INPUT,
        FFT_FAILED,
        INVALID_CONFIG,
        PROCESSING_FAILED
    };

    class MFCCProcessor
    {
    public:
        struct Config
        {
            size_t sample_rate{44100};
            size_t frame_size{512};
            size_t num_coefficients{13};
            size_t num_filters{26};
            float low_freq{0.0f};
            float high_freq{0.0f}; // 0 = sample_rate/2
            bool use_energy{true};
            bool apply_lifter{true};
            size_t lifter_coeff{22};
            bool enable_simd{true};
            bool enable_caching{true};
        };

        using FeatureVector = std::vector<float>;
        using FeatureMatrix = std::vector<FeatureVector>;

        explicit MFCCProcessor(const Config &config);
        ~MFCCProcessor();

        MFCCProcessor(MFCCProcessor &&) noexcept;
        MFCCProcessor &operator=(MFCCProcessor &&) noexcept;

        [[nodiscard]] std::expected<FeatureVector, MFCCError>
        extractFeatures(std::span<const float> audio_frame);

        [[nodiscard]] std::expected<FeatureMatrix, MFCCError>
        extractFeaturesFromBuffer(std::span<const float> audio_buffer,
                                  size_t hop_size);

        void clearCache();
        [[nodiscard]] size_t getCacheSize() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;
    };

} // namespace huntmaster