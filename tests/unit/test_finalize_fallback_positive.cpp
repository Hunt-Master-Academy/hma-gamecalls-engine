#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

static std::vector<float> makeSine(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> v(n);
    for (size_t i = 0; i < n; ++i)
        v[i] = 0.2f * std::sin(2.0 * M_PI * freq * (double)i / sr);
    return v;
}

TEST(FinalizeFallbackPositiveTest, FallbackFlagTrueWhenFinalizeImproves) {
#ifndef HUNTMASTER_TEST_HOOKS
    GTEST_SKIP() << "Test hooks disabled";
#endif
    auto engRes = UnifiedAudioEngine::create();
    ASSERT_TRUE(engRes.isOk());
    auto engine = std::move(engRes.value);
    auto sRes = engine->createSession(44100.0f);
    ASSERT_TRUE(sRes.isOk());
    auto sid = sRes.value;
    auto load = engine->loadMasterCall(sid, "test_sine_440");
    if (load != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Master asset unavailable";
    }
    // Feed enough audio for features
    auto audio = makeSine(440.0f, 1.2f, 44100.0f);
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t len = std::min(chunk, audio.size() - i);
        auto st = engine->processAudioChunk(sid, std::span<const float>(&audio[i], len));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    // Lower threshold so typical finalize improvement crosses it
    (void)engine->testSetFinalizeFallbackThreshold(sid, 0.40f);
    // Force artificially low last similarity before finalize
    auto hookStatus = engine->testOverrideLastSimilarity(sid, 0.10f);
    ASSERT_EQ(hookStatus, UnifiedAudioEngine::Status::OK);
    auto fin = engine->finalizeSessionAnalysis(sid);
    if (fin == UnifiedAudioEngine::Status::INSUFFICIENT_DATA) {
        GTEST_SKIP() << "Insufficient data for finalize";
    }
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto usedRes = engine->getFinalizeFallbackUsed(sid);
    ASSERT_TRUE(usedRes.isOk());
#ifndef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    EXPECT_TRUE(usedRes.value) << "Finalize fallback flag not set despite improvement";
#endif
    (void)engine->destroySession(sid);
}
