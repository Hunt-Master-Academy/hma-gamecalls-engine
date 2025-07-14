#include "huntmaster/core/DTWProcessor.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace huntmaster {

float DTWProcessor::euclideanDistanceSquared(const std::vector<float> &v1,
                                             const std::vector<float> &v2) {
    float distance = 0.0f;
    // Note: For simplicity and performance, we assume vectors are the same size.
    // A robust implementation might add a size check.
    for (size_t i = 0; i < v1.size(); ++i) {
        float diff = v1[i] - v2[i];
        distance += diff * diff;
    }
    return distance;
}

float DTWProcessor::calculateDistance(const std::vector<std::vector<float>> &seq1,
                                      const std::vector<std::vector<float>> &seq2) {
    if (seq1.empty() || seq2.empty()) {
        return std::numeric_limits<float>::infinity();  // Cannot compare empty sequences.
    }

    const size_t n = seq1.size();
    const size_t m = seq2.size();

    // Create a cost matrix. We only need to store two rows at a time to save space.
    // This is a common optimization for DTW.
    std::vector<float> prevRow(m + 1, std::numeric_limits<float>::infinity());
    std::vector<float> currentRow(m + 1, std::numeric_limits<float>::infinity());

    prevRow[0] = 0;  // The starting point of the path has zero cost.

    for (size_t i = 1; i <= n; ++i) {
        // Reset the current row for the new iteration.
        currentRow[0] = std::numeric_limits<float>::infinity();
        for (size_t j = 1; j <= m; ++j) {
            // Calculate the cost of the current pair of points.
            float cost = euclideanDistanceSquared(seq1[i - 1], seq2[j - 1]);

            // The value of the current cell is the cost of the current match
            // plus the minimum cost of the three possible previous cells.
            float minPrevCost = std::min({prevRow[j],           // Insertion
                                          prevRow[j - 1],       // Match
                                          currentRow[j - 1]});  // Deletion

            currentRow[j] = cost + minPrevCost;
        }
        // The current row becomes the previous row for the next iteration.
        prevRow = currentRow;
    }

    // The final value at the end of the matrix is the total cost of the optimal path.
    // For meaningful similarity comparison, normalize by the sequence length
    float totalDistance = std::sqrt(currentRow[m]);
    float normalizedDistance = totalDistance / std::sqrt(static_cast<float>(n * m));

    return normalizedDistance;
}

}  // namespace huntmaster
