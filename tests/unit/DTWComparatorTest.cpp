#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "huntmaster/core/DTWComparator.h"

// Use a test fixture to reduce boilerplate and set up a common environment.
class DTWComparatorTest : public ::testing::Test {
protected:
    // This method is called before each test.
    void SetUp() override {
        // Initialize with a default, non-normalizing, non-windowed config.
        huntmaster::DTWComparator::Config cfg;
        cfg.enable_simd = false;
        cfg.use_window = false;
        cfg.normalize_distance = false;
        dtw_ = std::make_unique<huntmaster::DTWComparator>(cfg);
    }

    // Member variables accessible by all tests in this fixture.
    std::unique_ptr<huntmaster::DTWComparator> dtw_;
};

TEST_F(DTWComparatorTest, IdenticalSequencesShouldHaveZeroDistance) {
    std::vector<std::vector<float>> seq = {
        {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
    float dist = dtw_->compare(seq, seq);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
}

TEST_F(DTWComparatorTest, DifferentSequencesShouldHaveNonZeroDistance) {
    std::vector<std::vector<float>> seq1 = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f, 2.0f, 3.0f}, {7.0f, 8.0f, 9.0f}};
    float dist = dtw_->compare(seq1, seq2);
    EXPECT_GT(dist, 0.0f);
}

TEST_F(DTWComparatorTest, EmptySequenceReturnsInfinity) {
    std::vector<std::vector<float>> seq1 = {};
    std::vector<std::vector<float>> seq2 = {{1.0f, 2.0f, 3.0f}};
    float dist = dtw_->compare(seq1, seq2);
    EXPECT_TRUE(std::isinf(dist));
}

TEST_F(DTWComparatorTest, WindowConstraintWorks) {
    // Re-configure the comparator for this specific test.
    huntmaster::DTWComparator::Config cfg;
    cfg.use_window = true;
    cfg.window_ratio = 0.1f;
    huntmaster::DTWComparator dtw(cfg);

    std::vector<std::vector<float>> seq1;
    std::vector<std::vector<float>> seq2;
    for (int i = 0; i < 10; ++i) {
        seq1.push_back(std::vector<float>{1.0f, 2.0f});
        seq2.push_back(std::vector<float>{1.0f, 2.0f});
    }
    float dist = dtw.compare(seq1, seq2);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
}

TEST_F(DTWComparatorTest, CompareWithPathReturnsAlignment) {
    std::vector<std::vector<float>> seq1 = {{1.0f}, {2.0f}, {3.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f}, {2.0f}, {3.0f}};
    std::vector<std::pair<size_t, size_t>> path;
    float dist = dtw_->compareWithPath(seq1, seq2, path);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
    ASSERT_EQ(path.size(), 3);
    for (size_t i = 0; i < path.size(); ++i) {
        EXPECT_EQ(path[i].first, i);
        EXPECT_EQ(path[i].second, i);
    }
}

TEST_F(DTWComparatorTest, NormalizedDistance) {
    // Re-configure the comparator for this specific test.
    huntmaster::DTWComparator::Config cfg;
    cfg.normalize_distance = true;
    huntmaster::DTWComparator dtw(cfg);

    std::vector<std::vector<float>> seq1 = {{1.0f}, {2.0f}};
    std::vector<std::vector<float>> seq2 = {{1.0f}, {2.0f}};
    float dist = dtw.compare(seq1, seq2);
    EXPECT_NEAR(dist, 0.0f, 1e-5f);
}