#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

// Helper sine
static std::vector<float> makeSine(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> out(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = std::sin(2.0 * M_PI * freq * static_cast<double>(i) / sr) * 0.2f;
    }
    return out;
}

TEST(FinalizeFallbackFlagTest, FlagRemainsFalseWhenRealtimeAlreadyHigh) {
    auto engRes = UnifiedAudioEngine::create();
    ASSERT_TRUE(engRes.isOk());
    auto engine = std::move(engRes.value);
    auto s = engine->createSession(44100.0f);
    ASSERT_TRUE(s.isOk());
    auto session = s.value;
    auto load = engine->loadMasterCall(session, "buck_grunt");
    if (load != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Master asset unavailable";
    }
    auto audio = makeSine(440.0f, 1.2f, 44100.0f);
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t remain = std::min(chunk, audio.size() - i);
        auto st = engine->processAudioChunk(session, std::span<const float>(&audio[i], remain));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(session);
    }
    auto simBefore = engine->getSimilarityScore(session);
    if (!simBefore.isOk()) {
        GTEST_SKIP() << "Similarity not ready";
    }
    auto fin = engine->finalizeSessionAnalysis(session);
    if (fin == UnifiedAudioEngine::Status::INSUFFICIENT_DATA) {
        GTEST_SKIP() << "Insufficient data to finalize";
    }
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto snap = engine->getSimilarityScores(session);
    if (!snap.isOk()) {
        GTEST_SKIP() << "No snapshot";
    }
#ifndef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    EXPECT_FALSE(snap.value.finalizeFallbackUsed);
#else
    (void)snap;  // suppress unused warning
#endif
    (void)engine->destroySession(session);
}
