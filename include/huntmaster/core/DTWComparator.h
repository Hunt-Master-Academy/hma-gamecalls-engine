/**
 * @file DTWComparator.h
 * @brief Dynamic Time Warping (DTW) algorithm for sequence comparison
 *
 * This file contains the DTW comparator class, which implements the Dynamic Time
 * Warping algorithm for comparing sequences of different lengths. DTW is particularly
 * useful for comparing audio features where temporal alignment is important, such
 * as comparing wildlife calls that may vary in duration or speaking rate.
 *
 * @author Huntmaster Development Team
 * @version 4.1
 * @date 2025
 * @copyright All Rights Reserved - 3D Tech Solutions
 */

#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @class DTWComparator
 * @brief High-performance Dynamic Time Warping implementation for sequence comparison
 *
 * The DTWComparator implements the Dynamic Time Warping algorithm, which finds
 * the optimal alignment between two sequences by allowing for temporal distortions.
 * This is essential for comparing wildlife calls or other audio features where
 * the timing may vary but the overall pattern should match.
 *
 * Key Features:
 * - Sakoe-Chiba band constraint for computational efficiency
 * - SIMD optimizations for performance
 * - Optional path tracking for alignment visualization
 * - Configurable distance normalization
 * - Memory-efficient implementation
 *
 * Algorithm Details:
 * DTW finds the minimum-cost alignment between two sequences by computing
 * a cumulative distance matrix and finding the optimal warping path through it.
 * The algorithm handles sequences of different lengths and accounts for
 * temporal variations in the data.
 *
 * @note This implementation is optimized for real-time performance while
 *       maintaining numerical accuracy for wildlife call analysis.
 *
 * @example Basic Usage:
 * @code
 * DTWComparator::Config config;
 * config.window_ratio = 0.1f;  // 10% window constraint
 * config.normalize_distance = true;
 *
 * DTWComparator comparator(config);
 *
 * std::vector<std::vector<float>> call1 = // ... MFCC features from call 1
 * std::vector<std::vector<float>> call2 = // ... MFCC features from call 2
 *
 * float similarity = comparator.compare(call1, call2);
 * // Lower values indicate better similarity
 * @endcode
 */
class DTWComparator {
  public:
    /**
     * @struct Config
     * @brief Configuration parameters for DTW algorithm behavior
     *
     * These parameters control the DTW algorithm's performance and accuracy
     * characteristics. The default values are optimized for wildlife call
     * comparison but can be adjusted for specific requirements.
     */
    struct Config {
        float window_ratio{0.1f};  ///< Sakoe-Chiba band width as ratio of sequence length (0.0-1.0)
        bool use_window{true};     ///< Enable Sakoe-Chiba band constraint for efficiency
        float distance_weight{1.0f};    ///< Weight applied to distance calculations
        bool normalize_distance{true};  ///< Normalize final distance by path length
        bool enable_simd{true};         ///< Enable SIMD optimizations for performance
    };

    /**
     * @brief Construct DTW comparator with specified configuration
     *
     * Initializes the comparator with the given configuration parameters.
     * Memory allocation for the DTW matrix is deferred until the first
     * comparison operation to optimize for different sequence sizes.
     *
     * @param config Configuration parameters for DTW algorithm
     */
    explicit DTWComparator(const Config& config);

    /**
     * @brief Destructor - cleans up allocated DTW computation matrices
     */
    ~DTWComparator();

    /**
     * @brief Move constructor
     * @param other Source comparator to move from
     */
    DTWComparator(DTWComparator&& other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Source comparator to move from
     * @return Reference to this comparator
     */
    DTWComparator& operator=(DTWComparator&& other) noexcept;

    /**
     * @brief Compare two feature sequences using DTW algorithm
     *
     * Computes the DTW distance between two sequences of feature vectors.
     * Lower distances indicate higher similarity between the sequences.
     * The algorithm automatically handles sequences of different lengths.
     *
     * @param sequence1 First sequence of feature vectors (e.g., MFCC frames)
     * @param sequence2 Second sequence of feature vectors for comparison
     * @return DTW distance (lower values = higher similarity)
     *
     * @note Each inner vector represents one time frame of features.
     *       Both sequences should have the same feature dimensionality
     *       (same size for each inner vector).
     *
     * @warning Empty sequences or mismatched feature dimensions will
     *          result in undefined behavior. Validate inputs before calling.
     */
    [[nodiscard]] float compare(const std::vector<std::vector<float>>& sequence1,
                                const std::vector<std::vector<float>>& sequence2);

    /**
     * @brief Compare sequences and return optimal alignment path
     *
     * Performs DTW comparison and additionally computes the optimal warping
     * path that shows how frames in sequence1 align with frames in sequence2.
     * This is useful for visualization and detailed analysis of the alignment.
     *
     * @param sequence1 First sequence of feature vectors
     * @param sequence2 Second sequence of feature vectors
     * @param alignment_path Output vector containing alignment path as (i,j) pairs
     * @return DTW distance (lower values = higher similarity)
     *
     * @note The alignment_path contains pairs where first element is the
     *       frame index in sequence1 and second element is the frame index
     *       in sequence2. The path starts at (0,0) and ends at (len1-1, len2-1).
     *
     * @example Alignment Path Usage:
     * @code
     * std::vector<std::pair<size_t, size_t>> path;
     * float distance = comparator.compareWithPath(call1, call2, path);
     *
     * // Visualize alignment
     * for (const auto& [i, j] : path) {
     *     std::cout << "Frame " << i << " in call1 aligns with frame "
     *               << j << " in call2\n";
     * }
     * @endcode
     */
    [[nodiscard]] float compareWithPath(const std::vector<std::vector<float>>& sequence1,
                                        const std::vector<std::vector<float>>& sequence2,
                                        std::vector<std::pair<size_t, size_t>>& alignment_path);

    /**
     * @brief Update the Sakoe-Chiba band window ratio
     *
     * Changes the window constraint ratio, which affects the trade-off between
     * computational efficiency and alignment flexibility. Smaller ratios are
     * faster but more restrictive.
     *
     * @param ratio New window ratio (0.0 = no warping allowed, 1.0 = full warping)
     *
     * @note Changes take effect for subsequent comparison operations.
     *       Values outside [0.0, 1.0] are clamped to valid range.
     */
    void setWindowRatio(float ratio);

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster
