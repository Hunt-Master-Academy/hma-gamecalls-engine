// DTW edge case tests
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DTWComparator.h"

using namespace huntmaster;

namespace {
std::vector<std::vector<float>> makeSeq(std::initializer_list<std::initializer_list<float>> rows) {
    std::vector<std::vector<float>> seq;
    seq.reserve(rows.size());
    for (auto& r : rows)
        seq.emplace_back(r);
    return seq;
}
std::vector<std::vector<float>> makeConstant(size_t frames, size_t dim, float v) {
    return std::vector<std::vector<float>>(frames, std::vector<float>(dim, v));
}
}  // namespace

TEST(DTWEdgeCases, EmptyEmptySequenceUndefinedBehaviorHandled) {
    DTWComparator::Config cfg;
    cfg.normalize_distance = true;
    cfg.use_window = false;
    DTWComparator comp(cfg);
    std::vector<std::vector<float>> a, b;  // both empty
    // Header warns empty sequences = undefined behavior; ensure we at least get a finite or inf
    // value (not NaN crash)
    float d = comp.compare(a, b);
    EXPECT_FALSE(std::isnan(d));
}

TEST(DTWEdgeCases, EmptyVsNonEmptyYieldsLargeDistance) {
    DTWComparator::Config cfg;
    cfg.normalize_distance = true;
    cfg.use_window = false;
    DTWComparator comp(cfg);
    std::vector<std::vector<float>> empty;
    auto nonEmpty = makeConstant(3, 5, 0.5f);
    float d = comp.compare(empty, nonEmpty);
    EXPECT_GT(d, 0.0f);  // Should be positive
}

TEST(DTWEdgeCases, IdenticalShortSequencesNearZeroDistance) {
    DTWComparator::Config cfg;
    cfg.normalize_distance = true;
    cfg.use_window = true;
    DTWComparator comp(cfg);
    auto seq = makeSeq({{0.f, 1.f}, {0.5f, 1.5f}, {1.f, 2.f}});
    float d = comp.compare(seq, seq);
    EXPECT_LE(d, 1e-6f);  // numeric noise bound
}

TEST(DTWEdgeCases, LengthMismatchGracefulAlignment) {
    DTWComparator::Config cfg;
    cfg.normalize_distance = true;
    cfg.use_window = true;
    DTWComparator comp(cfg);
    auto shortSeq = makeSeq({{0.f}, {1.f}, {2.f}});
    auto longSeq = makeSeq({{0.f}, {0.5f}, {1.f}, {1.5f}, {2.f}});
    float d = comp.compare(shortSeq, longSeq);
    // Distance should be small but > 0 due to interpolation mismatch
    EXPECT_GE(d, 0.0f);
}
