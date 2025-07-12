#include "MFCCProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <execution>
#include <immintrin.h>
#include <arm_neon.h>

namespace huntmaster
{

    namespace
    {

        constexpr float MEL_SCALE = 2595.0f;

        [[nodiscard]] inline float hzToMel(float hz)
        {
            return MEL_SCALE * std::log10(1.0f + hz / 700.0f);
        }

        [[nodiscard]] inline float melToHz(float mel)
        {
            return 700.0f * (std::pow(10.0f, mel / MEL_SCALE) - 1.0f);
        }

        struct MelFilterBank
        {
            std::vector<std::vector<std::pair<size_t, float>>> filters;
            size_t num_bins;

            MelFilterBank(size_t num_filters, size_t fft_size, float sample_rate,
                          float low_freq, float high_freq)
            {
                num_bins = fft_size / 2 + 1;
                filters.resize(num_filters);

                float low_mel = hzToMel(low_freq);
                float high_mel = hzToMel(high_freq > 0 ? high_freq : sample_rate / 2.0f);

                std::vector<float> mel_points(num_filters + 2);
                float mel_step = (high_mel - low_mel) / (num_filters + 1);

                std::generate(mel_points.begin(), mel_points.end(),
                              [low_mel, mel_step, n = 0]() mutable
                              {
                                  return low_mel + (n++) * mel_step;
                              });

                std::vector<size_t> bin_points(mel_points.size());
                std::transform(mel_points.begin(), mel_points.end(), bin_points.begin(),
                               [fft_size, sample_rate](float mel)
                               {
                                   float hz = melToHz(mel);
                                   return static_cast<size_t>(
                                       (fft_size + 1) * hz / sample_rate);
                               });

                for (size_t i = 0; i < num_filters; ++i)
                {
                    size_t start = bin_points[i];
                    size_t center = bin_points[i + 1];
                    size_t end = bin_points[i + 2];

                    for (size_t j = start; j < center; ++j)
                    {
                        float weight = static_cast<float>(j - start) / (center - start);
                        filters[i].emplace_back(j, weight);
                    }

                    for (size_t j = center; j < end; ++j)
                    {
                        float weight = static_cast<float>(end - j) / (end - center);
                        filters[i].emplace_back(j, weight);
                    }
                }
            }

            [[nodiscard]] std::vector<float> apply(std::span<const float> power_spectrum) const
            {
                std::vector<float> output(filters.size());

                std::transform(std::execution::unseq,
                               filters.begin(), filters.end(), output.begin(),
                               [&power_spectrum](const auto &filter)
                               {
                                   return std::transform_reduce(
                                       filter.begin(), filter.end(), 0.0f, std::plus{},
                                       [&power_spectrum](const auto &bin_weight)
                                       {
                                           auto [bin, weight] = bin_weight;
                                           return power_spectrum[bin] * weight;
                                       });
                               });

                return output;
            }
        };

        class DCTMatrix
        {
            std::vector<std::vector<float>> matrix_;
            size_t num_coeffs_;
            size_t num_filters_;

        public:
            DCTMatrix(size_t num_coeffs, size_t num_filters)
                : num_coeffs_(num_coeffs), num_filters_(num_filters)
            {
                matrix_.resize(num_coeffs);

                const float norm_factor = std::sqrt(2.0f / num_filters);

                for (size_t i = 0; i < num_coeffs; ++i)
                {
                    matrix_[i].resize(num_filters);
                    for (size_t j = 0; j < num_filters; ++j)
                    {
                        matrix_[i][j] = norm_factor * std::cos(
                                                          M_PI * i * (j + 0.5f) / num_filters);
                    }
                }
            }

            [[nodiscard]] std::vector<float> apply(std::span<const float> input) const
            {
                std::vector<float> output(num_coeffs_);

#ifdef __AVX2__
                if (num_filters_ >= 8)
                {
                    applyAVX2(input, output);
                }
                else
#elif defined(__ARM_NEON)
                if (num_filters_ >= 4)
                {
                    applyNEON(input, output);
                }
                else
#endif
                {
                    applyScalar(input, output);
                }

                return output;
            }

        private:
            void applyScalar(std::span<const float> input,
                             std::span<float> output) const
            {
                for (size_t i = 0; i < num_coeffs_; ++i)
                {
                    output[i] = std::inner_product(
                        matrix_[i].begin(), matrix_[i].end(),
                        input.begin(), 0.0f);
                }
            }

#ifdef __AVX2__
            [[gnu::target("avx2")]]
            void applyAVX2(std::span<const float> input,
                           std::span<float> output) const
            {
                for (size_t i = 0; i < num_coeffs_; ++i)
                {
                    __m256 sum = _mm256_setzero_ps();
                    size_t j = 0;

                    for (; j + 8 <= num_filters_; j += 8)
                    {
                        __m256 a = _mm256_loadu_ps(&matrix_[i][j]);
                        __m256 b = _mm256_loadu_ps(&input[j]);
                        sum = _mm256_fmadd_ps(a, b, sum);
                    }

                    float result[8];
                    _mm256_storeu_ps(result, sum);
                    float total = std::accumulate(result, result + 8, 0.0f);

                    for (; j < num_filters_; ++j)
                    {
                        total += matrix_[i][j] * input[j];
                    }

                    output[i] = total;
                }
            }
#endif

#ifdef __ARM_NEON
            void applyNEON(std::span<const float> input,
                           std::span<float> output) const
            {
                for (size_t i = 0; i < num_coeffs_; ++i)
                {
                    float32x4_t sum = vdupq_n_f32(0.0f);
                    size_t j = 0;

                    for (; j + 4 <= num_filters_; j += 4)
                    {
                        float32x4_t a = vld1q_f32(&matrix_[i][j]);
                        float32x4_t b = vld1q_f32(&input[j]);
                        sum = vmlaq_f32(sum, a, b);
                    }

                    float total = vaddvq_f32(sum);

                    for (; j < num_filters_; ++j)
                    {
                        total += matrix_[i][j] * input[j];
                    }

                    output[i] = total;
                }
            }
#endif
        };

    } // anonymous namespace

    class MFCCProcessor::Impl
    {
    public:
        Config config_;
        std::unique_ptr<MelFilterBank> mel_filters_;
        std::unique_ptr<DCTMatrix> dct_matrix_;
        std::vector<float> lifter_coeffs_;

        // FFT workspace
        std::vector<float> fft_buffer_;
        std::vector<std::complex<float>> fft_output_;

        // Feature cache
        mutable std::unordered_map<size_t, FeatureVector> cache_;
        mutable std::mutex cache_mutex_;

        // Pre-emphasis filter state
        float pre_emphasis_prev_{0.0f};
        static constexpr float PRE_EMPHASIS_COEFF = 0.97f;

        explicit Impl(const Config &config) : config_(config)
        {
            float high_freq = config.high_freq > 0 ? config.high_freq : config.sample_rate / 2.0f;

            mel_filters_ = std::make_unique<MelFilterBank>(
                config.num_filters, config.frame_size,
                static_cast<float>(config.sample_rate),
                config.low_freq, high_freq);

            dct_matrix_ = std::make_unique<DCTMatrix>(
                config.num_coefficients, config.num_filters);

            if (config.apply_lifter)
            {
                lifter_coeffs_.resize(config.num_coefficients);
                for (size_t i = 0; i < config.num_coefficients; ++i)
                {
                    lifter_coeffs_[i] = 1.0f + (config.lifter_coeff / 2.0f) *
                                                   std::sin(M_PI * i / config.lifter_coeff);
                }
            }

            fft_buffer_.resize(config.frame_size);
            fft_output_.resize(config.frame_size / 2 + 1);
        }

        [[nodiscard]] std::optional<FeatureVector> checkCache(size_t hash) const
        {
            if (!config_.enable_caching)
                return std::nullopt;

            std::lock_guard lock(cache_mutex_);
            auto it = cache_.find(hash);
            if (it != cache_.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        void updateCache(size_t hash, const FeatureVector &features)
        {
            if (!config_.enable_caching)
                return;

            std::lock_guard lock(cache_mutex_);
            cache_[hash] = features;

            // Simple cache eviction
            if (cache_.size() > 1000)
            {
                cache_.clear();
            }
        }

        [[nodiscard]] huntmaster::expected<FeatureVector, MFCCError>
        processFrame(std::span<const float> frame)
        {
            if (frame.size() != config_.frame_size)
            {
                return huntmaster::unexpected(MFCCError::INVALID_INPUT);
            }

            // Check cache
            size_t hash = std::hash<std::string_view>{}(
                std::string_view(reinterpret_cast<const char *>(frame.data()),
                                 frame.size() * sizeof(float)));

            if (auto cached = checkCache(hash))
            {
                return *cached;
            }

            // Pre-emphasis
            std::transform(frame.begin(), frame.end(), fft_buffer_.begin(),
                           [this, prev = pre_emphasis_prev_](float sample) mutable
                           {
                               float emphasized = sample - PRE_EMPHASIS_COEFF * prev;
                               prev = sample;
                               return emphasized;
                           });
            pre_emphasis_prev_ = frame.back();

            // Hamming window
            applyHammingWindow(fft_buffer_);

            // FFT (simplified - would use KissFFT in real implementation)
            computeFFT(fft_buffer_, fft_output_);

            // Power spectrum
            std::vector<float> power_spectrum(fft_output_.size());
            std::transform(fft_output_.begin(), fft_output_.end(),
                           power_spectrum.begin(),
                           [](const auto &c)
                           { return std::norm(c); });

            // Mel filter bank
            auto mel_energies = mel_filters_->apply(power_spectrum);

            // Log transform
            std::transform(mel_energies.begin(), mel_energies.end(),
                           mel_energies.begin(),
                           [](float x)
                           { return std::log(x + 1e-10f); });

            // DCT
            auto mfcc = dct_matrix_->apply(mel_energies);

            // Liftering
            if (config_.apply_lifter)
            {
                std::transform(mfcc.begin(), mfcc.end(), lifter_coeffs_.begin(),
                               mfcc.begin(), std::multiplies{});
            }

            // Update cache
            updateCache(hash, mfcc);

            return mfcc;
        }

    private:
        void applyHammingWindow(std::span<float> frame)
        {
            const size_t N = frame.size();
            const float a0 = 0.54f;
            const float a1 = 0.46f;

            for (size_t i = 0; i < N; ++i)
            {
                float window = a0 - a1 * std::cos(2.0f * M_PI * i / (N - 1));
                frame[i] *= window;
            }
        }

        void computeFFT(std::span<const float> input,
                        std::span<std::complex<float>> output)
        {
            // Placeholder - would use KissFFT
            for (size_t i = 0; i < output.size(); ++i)
            {
                output[i] = std::complex<float>(input[i % input.size()], 0.0f);
            }
        }
    };

    MFCCProcessor::MFCCProcessor(const Config &config)
        : pimpl_(std::make_unique<Impl>(config))
    {
    }

    MFCCProcessor::~MFCCProcessor() = default;

    MFCCProcessor::MFCCProcessor(MFCCProcessor &&) noexcept = default;

    MFCCProcessor &MFCCProcessor::operator=(MFCCProcessor &&) noexcept = default;

    huntmaster::expected<MFCCProcessor::FeatureVector, MFCCError>
    MFCCProcessor::extractFeatures(std::span<const float> audio_frame)
    {
        return pimpl_->processFrame(audio_frame);
    }

    huntmaster::expected<MFCCProcessor::FeatureMatrix, MFCCError>
    MFCCProcessor::extractFeaturesFromBuffer(std::span<const float> audio_buffer,
                                             size_t hop_size)
    {
        if (audio_buffer.size() < pimpl_->config_.frame_size)
        {
            return huntmaster::unexpected(MFCCError::INVALID_INPUT);
        }

        FeatureMatrix features;
        size_t num_frames = (audio_buffer.size() - pimpl_->config_.frame_size) / hop_size + 1;
        features.reserve(num_frames);

        for (size_t i = 0; i < num_frames; ++i)
        {
            size_t start = i * hop_size;
            auto frame = audio_buffer.subspan(start, pimpl_->config_.frame_size);

            auto result = extractFeatures(frame);
            if (!result)
            {
                return huntmaster::unexpected(result.error());
            }

            features.push_back(std::move(result.value()));
        }

        return features;
    }

    void MFCCProcessor::clearCache()
    {
        std::lock_guard lock(pimpl_->cache_mutex_);
        pimpl_->cache_.clear();
    }

    size_t MFCCProcessor::getCacheSize() const noexcept
    {
        std::lock_guard lock(pimpl_->cache_mutex_);
        return pimpl_->cache_.size();
    }

} // namespace huntmaster