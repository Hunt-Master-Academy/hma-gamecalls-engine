#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "huntmaster/core/DTWComparator.h"

namespace {

// Returns a default configuration for DTWComparator with all options set to their baseline values.
huntmaster::DTWComparator::Config DefaultConfig() {
    huntmaster::DTWComparator::Config cfg;
    cfg.enable_simd = false;
    cfg.use_window = false;
    cfg.window_ratio = 1.0f;
    cfg.distance_weight = 1.0f;
    cfg.normalize_distance = false;
    return cfg;
}
}

TEST(DTWComparatorTest, IdenticalSequencesShouldHaveZeroDistance) {
    huntmaster::DTWComparator dtw(DefaultConfig());
    std::vector<std::vector<float>> seq = {
        {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
    float dist = dtw.compare(seq, seq);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
}

TEST(DTWComparatorTest, DifferentSequencesShouldHaveNonZeroDistance) {
    huntmaster::DTWComparator dtw(DefaultConfig());
    std::vector<std::vector<float>> seq1 = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f, 2.0f, 3.0f}, {7.0f, 8.0f, 9.0f}};
    float dist = dtw.compare(seq1, seq2);
    EXPECT_GT(dist, 0.0f);
}

TEST(DTWComparatorTest, EmptySequenceReturnsInfinity) {
    huntmaster::DTWComparator dtw(DefaultConfig());
    std::vector<std::vector<float>> seq1 = {};
    std::vector<std::vector<float>> seq2 = {{1.0f, 2.0f, 3.0f}};
    float dist = dtw.compare(seq1, seq2);
    EXPECT_TRUE(std::isinf(dist));
}

TEST(DTWComparatorTest, WindowConstraintWorks) {
    auto cfg = DefaultConfig();
    cfg.use_window = true;
    cfg.window_ratio = 0.1f;
    std::vector<std::vector<float>> seq1;
    std::vector<std::vector<float>> seq2;
    for (int i = 0; i < 10; ++i) {
        seq1.push_back(std::vector<float>{1.0f, 2.0f});
        seq2.push_back(std::vector<float>{1.0f, 2.0f});
    }
    std::vector<std::vector<float>> seq2(10, std::vector<float>{1.0f, 2.0f});
    float dist = dtw.compare(seq1, seq2);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
}

TEST(DTWComparatorTest, CompareWithPathReturnsAlignment) {
    huntmaster::DTWComparator dtw(DefaultConfig());
    std::vector<std::vector<float>> seq1 = {{1.0f}, {2.0f}, {3.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f}, {2.0f}, {3.0f}};
    std::vector<std::pair<size_t, size_t>> path;
    float dist = dtw.compareWithPath(seq1, seq2, path);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
    ASSERT_EQ(path.size(), 3);
    for (size_t i = 0; i < path.size(); ++i) {
        EXPECT_EQ(path[i].first, i);
        EXPECT_EQ(path[i].second, i);
    }
}

TEST(DTWComparatorTest, NormalizedDistance) {
    auto cfg = DefaultConfig();
    cfg.normalize_distance = true;
    huntmaster::DTWComparator dtw(cfg);
    std::vector<std::vector<float>> seq1 = {{1.0f}, {2.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f}, {2.0f}};
}  // namespace <anonymous>= dtw.compare(seq1, seq2);
EXPECT_NEAR(dist, 0.0f, 1e-5f);
}

}  // namespace