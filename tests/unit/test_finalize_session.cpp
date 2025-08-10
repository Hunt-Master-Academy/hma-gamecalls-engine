#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class FinalizeSessionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engResult.isOk());
        engine = std::move(engResult.value);
        auto s = engine->createSession(44100.0f);
        ASSERT_TRUE(s.isOk());
        session = s.value;
        // Load a simple master call (use existing test asset sine 440) if exists
        auto status = engine->loadMasterCall(session, "test_sine_440");
        // Some environments may not have asset; allow SESSION_NOT_FOUND pattern fallback
        if (status != UnifiedAudioEngine::Status::OK) {
            GTEST_SKIP() << "Master call asset unavailable in test environment";
        }
    }
    void TearDown() override {
        if (engine && session != INVALID_SESSION_ID) {
            auto ds = engine->destroySession(session);
            (void)ds;  // Explicitly ignore; session cleanup best-effort
        }
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId session{INVALID_SESSION_ID};
};

// Helper to generate a short sine wave buffer
static std::vector<float> makeSine(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> out(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = std::sin(2.0 * M_PI * freq * static_cast<double>(i) / sr);
    }
    return out;
}

TEST_F(FinalizeSessionTest, FinalizeIdempotentAndPopulatesSummaryBasic) {
    // Feed some audio to accumulate features
    auto audio = makeSine(440.0f, 0.8f, 44100.0f);  // ~0.8s should yield multiple frames
    // Chunk feed
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t remain = std::min(chunk, audio.size() - i);
        UnifiedAudioEngine::Status st =
            engine->processAudioChunk(session, std::span<const float>(&audio[i], remain));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    }
    // First finalize
    auto st1 = engine->finalizeSessionAnalysis(session);
    ASSERT_EQ(st1, UnifiedAudioEngine::Status::OK);
    // Second finalize should be ALREADY_FINALIZED
    auto st2 = engine->finalizeSessionAnalysis(session);
    ASSERT_EQ(st2, UnifiedAudioEngine::Status::ALREADY_FINALIZED);

    auto summaryRes = engine->getEnhancedAnalysisSummary(session);
    ASSERT_TRUE(summaryRes.isOk());
    auto summary = summaryRes.value;
    EXPECT_TRUE(summary.finalized);
    EXPECT_GE(summary.segmentDurationMs, 10u);  // should have non-zero duration
    EXPECT_GE(summary.similarityAtFinalize, 0.0f);
    EXPECT_GE(summary.normalizationScalar, 0.0f);
    EXPECT_LT(summary.normalizationScalar, 5.0f);
}

TEST_F(FinalizeSessionTest, FinalizeInsufficientData) {
    auto s2 = engine->createSession(44100.0f);
    ASSERT_TRUE(s2.isOk());
    SessionId sid2 = s2.value;
    auto status = engine->loadMasterCall(sid2, "test_sine_440");
    if (status != UnifiedAudioEngine::Status::OK) {
        auto ds = engine->destroySession(sid2);
        (void)ds;  // ignore result in skip path
        GTEST_SKIP() << "Master call unavailable";
    }
    // Do NOT feed enough audio (none) -> expect INSUFFICIENT_DATA
    auto st = engine->finalizeSessionAnalysis(sid2);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::INSUFFICIENT_DATA);
    auto ds2 = engine->destroySession(sid2);
    EXPECT_TRUE(ds2 == UnifiedAudioEngine::Status::OK
                || ds2 == UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}
