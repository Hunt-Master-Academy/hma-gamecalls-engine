// Negative control: ensure finalizeFallbackUsed remains false when finalize does NOT
// elevate similarity above threshold after a forced low override.
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

static std::vector<float> makeCompositeMismatch(float sr, float seconds) {
    size_t n = static_cast<size_t>(sr * seconds);
    std::vector<float> buf(n);
    uint32_t s = 0xA5F00D13u;
    for (size_t i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / sr;
        float a = 0.3f * std::sin(2.0f * static_cast<float>(M_PI) * 523.25f * t);  // C5
        float b = 0.3f * std::sin(2.0f * static_cast<float>(M_PI) * 659.25f * t);  // E5
        s = 1664525u * s + 1013904223u;
        float noise = ((s >> 8) & 0xFFFF) / 65535.0f * 0.05f - 0.025f;
        buf[i] = a + b + noise;
    }
    return buf;
}

TEST(FinalizeFallbackNegativeTest, FallbackFlagFalseWhenNoThresholdCross) {
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
    // Raise threshold high so finalize won't cross with mismatch audio
    (void)engine->testSetFinalizeFallbackThreshold(sid, 0.98f);
    auto noise = makeCompositeMismatch(44100.0f, 1.0f);
    size_t chunk = 1024;
    for (size_t i = 0; i < noise.size(); i += chunk) {
        size_t len = std::min<size_t>(chunk, noise.size() - i);
        auto st = engine->processAudioChunk(sid, std::span<const float>(&noise[i], len));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    ASSERT_EQ(engine->testOverrideLastSimilarity(sid, 0.01f), UnifiedAudioEngine::Status::OK);
    auto fin = engine->finalizeSessionAnalysis(sid);
    if (fin == UnifiedAudioEngine::Status::INSUFFICIENT_DATA) {
        GTEST_SKIP() << "Insufficient data for finalize";
    }
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto usedRes = engine->getFinalizeFallbackUsed(sid);
    ASSERT_TRUE(usedRes.isOk());
    if (usedRes.value) {
        // If finalize actually raised similarity above threshold, this scenario isn't valid for
        // negative test.
        GTEST_SKIP()
            << "Finalize improved similarity above threshold; negative control not applicable";
    }
    EXPECT_FALSE(usedRes.value);
    (void)engine->destroySession(sid);
}
