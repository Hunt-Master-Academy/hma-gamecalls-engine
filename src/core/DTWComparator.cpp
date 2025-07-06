// File: DTWComparator.cpp
#include "huntmaster/core/DTWComparator.h"
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>
#include <execution>
#include <immintrin.h>

namespace huntmaster
{

    class DTWComparator::Impl
    {
    public:
        Config config_;

        // Reusable buffers to avoid allocations
        std::vector<std::vector<float>> cost_matrix_;
        std::vector<std::vector<size_t>> path_matrix_;

        explicit Impl(const Config &config) : config_(config) {}

        [[nodiscard]] float euclideanDistance(
            std::span<const float> vec1,
            std::span<const float> vec2)
        {

#ifdef __AVX2__
            if (config_.enable_simd && vec1.size() >= 8)
            {
                return euclideanDistanceAVX2(vec1, vec2);
            }
#endif

            return euclideanDistanceScalar(vec1, vec2);
        }

        [[nodiscard]] float euclideanDistanceScalar(
            std::span<const float> vec1,
            std::span<const float> vec2)
        {

            return std::sqrt(
                std::transform_reduce(
                    vec1.begin(), vec1.end(), vec2.begin(), 0.0f,
                    std::plus{},
                    [](float a, float b)
                    {
                        float diff = a - b;
                        return diff * diff;
                    }));
        }

#ifdef __AVX2__
        [[gnu::target("avx2")]] [[nodiscard]] float euclideanDistanceAVX2(
            std::span<const float> vec1,
            std::span<const float> vec2)
        {

            __m256 sum = _mm256_setzero_ps();
            size_t i = 0;

            for (; i + 8 <= vec1.size(); i += 8)
            {
                __m256 a = _mm256_loadu_ps(&vec1[i]);
                __m256 b = _mm256_loadu_ps(&vec2[i]);
                __m256 diff = _mm256_sub_ps(a, b);
                sum = _mm256_fmadd_ps(diff, diff, sum);
            }

            alignas(32) float result[8];
            _mm256_store_ps(result, sum);
            float total = std::accumulate(result, result + 8, 0.0f);

            for (; i < vec1.size(); ++i)
            {
                float diff = vec1[i] - vec2[i];
                total += diff * diff;
            }

            return std::sqrt(total);
        }
#endif

        [[nodiscard]] float computeDTW(
            const std::vector<std::vector<float>> &seq1,
            const std::vector<std::vector<float>> &seq2,
            std::vector<std::pair<size_t, size_t>>* path_out = nullptr)
        {

            const size_t len1 = seq1.size();
            const size_t len2 = seq2.size();

            if (len1 == 0 || len2 == 0)
            {
                return std::numeric_limits<float>::infinity();
            }

            // Resize matrices
            cost_matrix_.resize(len1 + 1);
            for (auto &row : cost_matrix_)
            {
                row.resize(len2 + 1, std::numeric_limits<float>::infinity());
            }

            if (path_out)
            {
                path_matrix_.resize(len1 + 1);
                for (auto &row : path_matrix_)
                {
                    row.resize(len2 + 1);
                }
            }

            // Initialize
            cost_matrix_[0][0] = 0.0f;

            // Compute window bounds
            int window_size = config_.use_window ? static_cast<int>(std::max(len1, len2) * config_.window_ratio) : std::numeric_limits<int>::max();

            // Fill cost matrix
            for (size_t i = 1; i <= len1; ++i)
            {
                int j_start = config_.use_window ? std::max(1, static_cast<int>(i) - window_size) : 1;
                int j_end = config_.use_window ? std::min(static_cast<int>(len2), static_cast<int>(i) + window_size) : len2;

                for (int j = j_start; j <= j_end; ++j)
                {
                    float cost = euclideanDistance(seq1[i - 1], seq2[j - 1]) *
                                 config_.distance_weight;

                    float insertion = cost_matrix_[i - 1][j];
                    float deletion = cost_matrix_[i][j - 1];
                    float match = cost_matrix_[i - 1][j - 1];

                    float min_cost = std::min({insertion, deletion, match});
                    cost_matrix_[i][j] = cost + min_cost;

                    if (path_out)
                    {
                        if (min_cost == match)
                        {
                            path_matrix_[i][j] = 0; // diagonal
                        }
                        else if (min_cost == insertion)
                        {
                            path_matrix_[i][j] = 1; // up
                        }
                        else
                        {
                            path_matrix_[i][j] = 2; // left
                        }
                    }
                }
            }

            float distance = cost_matrix_[len1][len2];

            // Normalize by path length if requested
            if (config_.normalize_distance)
            {
                distance /= (len1 + len2);
            }

            // Reconstruct path if requested
            if (path_out)
            {
                auto &path = *path_out;
                path.clear();

                size_t i = len1, j = len2;
                while (i > 0 && j > 0)
                {
                    path.emplace_back(i - 1, j - 1);

                    switch (path_matrix_[i][j])
                    {
                    case 0: // diagonal
                        i--;
                        j--;
                        break;
                    case 1: // up
                        i--;
                        break;
                    case 2: // left
                        j--;
                        break;
                    }
                }

                std::reverse(path.begin(), path.end());
            }

            return distance;
        }
    };

    DTWComparator::DTWComparator(const Config &config)
        : pimpl_(std::make_unique<Impl>(config))
    {
    }

    DTWComparator::~DTWComparator() = default;

    DTWComparator::DTWComparator(DTWComparator &&) noexcept = default;

    DTWComparator &DTWComparator::operator=(DTWComparator &&) noexcept = default;

    float DTWComparator::compare(
        const std::vector<std::vector<float>> &sequence1,
        const std::vector<std::vector<float>> &sequence2)
    {
        return pimpl_->computeDTW(sequence1, sequence2);
    }

    float DTWComparator::compareWithPath(
        const std::vector<std::vector<float>> &sequence1,
        const std::vector<std::vector<float>> &sequence2,
        std::vector<std::pair<size_t, size_t>> &alignment_path)
    {
        return pimpl_->computeDTW(sequence1, sequence2, &alignment_path);
    }

    void DTWComparator::setWindowRatio(float ratio)
    {
        pimpl_->config_.window_ratio = std::clamp(ratio, 0.0f, 1.0f);
    }

} // namespace huntmaster