#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

// Simple deterministic sine generator
static std::vector<float> sine(float freq, float sr, float sec) {
    size_t n = static_cast<size_t>(sec * sr);
    std::vector<float> out(n);
    double w = 2.0 * M_PI * freq / sr;
    for (size_t i = 0; i < n; ++i)
        out[i] = std::sin(w * i) * 0.4f;
    return out;
}

class FinalizeImprovementTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engRes = UnifiedAudioEngine::create();
        ASSERT_TRUE(engRes.isOk());
        engine = std::move(engRes.value);
        auto s = engine->createSession(44100.0f);
        ASSERT_TRUE(s.isOk());
        session = s.value;
        auto st = engine->loadMasterCall(session, "buck_grunt");
        if (st != UnifiedAudioEngine::Status::OK) {
            GTEST_SKIP() << "Required master call not available";
        }
    }
    void TearDown() override {
        if (engine && session != INVALID_SESSION_ID) {
            (void)engine->destroySession(session);
        }
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId session{INVALID_SESSION_ID};
};

TEST_F(FinalizeImprovementTest, FinalizeScoreNotLowerThanRealtime) {
    // Feed several chunks (~0.6s) using a low-amplitude sine to exercise feature extraction
    auto sineBuf = sine(440.0f, 44100.0f, 0.6f);  // ~0.6s
    size_t chunk = 2048;
    for (size_t i = 0; i < sineBuf.size(); i += chunk) {
        size_t sz = std::min(chunk, sineBuf.size() - i);
        auto st =
            engine->processAudioChunk(session, std::span<const float>(sineBuf.data() + i, sz));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    }
    float preFinalize = 0.0f;
    auto pre = engine->getSimilarityScore(session);
    if (pre.isOk())
        preFinalize = pre.value;

    auto fst = engine->finalizeSessionAnalysis(session);
    if (fst == UnifiedAudioEngine::Status::INSUFFICIENT_DATA) {
        GTEST_SKIP() << "Insufficient data to finalize";
    }
    ASSERT_TRUE(fst == UnifiedAudioEngine::Status::OK
                || fst == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto summaryRes = engine->getEnhancedAnalysisSummary(session);
    ASSERT_TRUE(summaryRes.isOk());
    if (pre.isOk()) {
        EXPECT_GE(summaryRes.value.similarityAtFinalize + 1e-4f, preFinalize - 1e-4f);
    }
}
