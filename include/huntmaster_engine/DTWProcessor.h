#ifndef HUNTMASTER_DTW_PROCESSOR_H
#define HUNTMASTER_DTW_PROCESSOR_H

#include <vector>

namespace huntmaster
{

    /**
     * @class DTWProcessor
     * @brief Computes the similarity between two sequences using Dynamic Time Warping.
     *
     * This class provides a static method to calculate the DTW distance between
     * two time series, such as sequences of MFCC vectors. DTW finds the optimal
     * alignment between two sequences, making it robust to slight variations in
     * timing and speed, which is common in audio signals like animal calls.
     */
    class DTWProcessor
    {
    public:
        /**
         * @brief Calculates the DTW distance between two sequences of feature vectors.
         * @param seq1 The first sequence (e.g., master call features).
         * @param seq2 The second sequence (e.g., user's attempt features).
         * @return A non-negative float representing the accumulated cost of the optimal
         * alignment path. A lower value indicates higher similarity.
         */
        static float calculateDistance(const std::vector<std::vector<float>> &seq1, const std::vector<std::vector<float>> &seq2);

    private:
        /**
         * @brief Calculates the squared Euclidean distance between two vectors.
         * We use squared distance to avoid the computationally expensive sqrt operation
         * inside the main DTW loop. Since we only care about the relative costs,
         * this optimization is valid and common.
         * @param v1 The first vector.
         * @param v2 The second vector.
         * @return The squared Euclidean distance between the two vectors.
         */
        static float euclideanDistanceSquared(const std::vector<float> &v1, const std::vector<float> &v2);
    };

} // namespace huntmaster

#endif // HUNTMASTER_DTW_PROCESSOR_H
