// DTW window ratio clamping and path reconstruction tests
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DTWComparator.h"

using namespace huntmaster;

static std::vector<std::vector<float>> seqIncreasing(size_t frames, size_t dims) {
    std::vector<std::vector<float>> s(frames, std::vector<float>(dims, 0.0f));
    for (size_t i = 0; i < frames; ++i) {
        for (size_t d = 0; d < dims; ++d)
            s[i][d] = static_cast<float>(i + d);
    }
    return s;
}

TEST(DTWWindowAndPath, SetWindowRatioClampsToValidRange) {
    DTWComparator::Config cfg;
    cfg.use_window = true;
    cfg.window_ratio = 0.2f;
    cfg.normalize_distance = true;
    DTWComparator comp(cfg);
    comp.setWindowRatio(-1.0f);  // should clamp to 0
    auto a = seqIncreasing(8, 3);
    auto b = seqIncreasing(9, 3);
    float d0 = comp.compare(a, b);
    comp.setWindowRatio(2.0f);  // should clamp to 1
    float d1 = comp.compare(a, b);
    // Distances should be finite and non-negative
    EXPECT_GE(d0, 0.0f);
    EXPECT_GE(d1, 0.0f);
}

TEST(DTWWindowAndPath, CompareWithPathProducesEndpoints) {
    DTWComparator::Config cfg;
    cfg.use_window = true;
    cfg.window_ratio = 0.5f;
    cfg.normalize_distance = true;
    DTWComparator comp(cfg);
    auto a = seqIncreasing(6, 2);
    auto b = seqIncreasing(9, 2);
    std::vector<std::pair<size_t, size_t>> path;
    (void)comp.compareWithPath(a, b, path);
    ASSERT_FALSE(path.empty());
    // Path should start near (0,0) and end near (len(a)-1, len(b)-1)
    EXPECT_EQ(path.front().first, 0u);
    EXPECT_EQ(path.front().second, 0u);
    EXPECT_EQ(path.back().first, a.size() - 1);
    EXPECT_EQ(path.back().second, b.size() - 1);
}
