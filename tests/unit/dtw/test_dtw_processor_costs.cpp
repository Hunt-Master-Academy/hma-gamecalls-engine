// DTWProcessor direct distance tests
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DTWProcessor.h"

using namespace huntmaster;

static std::vector<std::vector<float>>
makeSeq(size_t frames, size_t dims, float base = 0.0f, float step = 1.0f) {
    std::vector<std::vector<float>> s(frames, std::vector<float>(dims, 0.0f));
    for (size_t i = 0; i < frames; ++i) {
        for (size_t d = 0; d < dims; ++d)
            s[i][d] = base + static_cast<float>(i) * step + static_cast<float>(d);
    }
    return s;
}

TEST(DTWProcessorCosts, IdenticalSequencesHaveNearZeroDistance) {
    auto a = makeSeq(8, 3, 0.0f, 0.5f);
    auto b = a;
    float dist = DTWProcessor::calculateDistance(a, b);
    EXPECT_GE(dist, 0.0f);
    EXPECT_NEAR(dist, 0.0f, 1e-6f);
}

TEST(DTWProcessorCosts, ShiftedSequencesIncreaseDistance) {
    auto a = makeSeq(10, 2, 0.0f, 1.0f);
    auto b = makeSeq(10, 2, 0.5f, 1.0f);  // shift all values by +0.5
    float d1 = DTWProcessor::calculateDistance(a, b);
    float d0 = DTWProcessor::calculateDistance(a, a);
    EXPECT_GT(d1, d0);
}

TEST(DTWProcessorCosts, EmptySequenceReturnsInfinity) {
    std::vector<std::vector<float>> empty;
    auto a = makeSeq(5, 3);
    float d1 = DTWProcessor::calculateDistance(empty, a);
    float d2 = DTWProcessor::calculateDistance(a, empty);
    EXPECT_TRUE(std::isinf(d1));
    EXPECT_TRUE(std::isinf(d2));
}
