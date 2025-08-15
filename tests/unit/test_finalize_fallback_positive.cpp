// Positive finalize fallback test: ensure finalizeFallbackUsed becomes true when finalize improves
// similarity beyond threshold.
#include <algorithm>
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

static std::vector<std::vector<float>> synthMasterPos(size_t frames = 40, size_t dim = 13) {
    std::vector<std::vector<float>> feats(frames, std::vector<float>(dim));
    for (size_t f = 0; f < frames; ++f) {
        for (size_t d = 0; d < dim; ++d) {
            feats[f][d] = 0.25f + 0.06f * std::sin(0.06 * static_cast<double>(f))
                          + 0.004f * static_cast<float>(d);
        }
    }
    return feats;
}

static std::vector<float> makeSine(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> buf(n);
    for (size_t i = 0; i < n; ++i) {
        buf[i] = 0.2f * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / sr);
    }
    return buf;
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
    SessionId sid = sRes.value;
    auto load = engine->loadMasterCall(sid, "test_sine_440");
    if (load != UnifiedAudioEngine::Status::OK) {
        ASSERT_EQ(engine->testInjectMasterCallFeatures(sid, synthMasterPos()),
                  UnifiedAudioEngine::Status::OK);
    }
    auto audio = makeSine(440.0f, 1.2f, 44100.0f);
    const size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        const size_t len = std::min(chunk, audio.size() - i);
        std::span<const float> spanChunk(&audio[i], len);
        UnifiedAudioEngine::Status st = engine->processAudioChunk(sid, spanChunk);
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    // Assert readiness (feed incremental slices if needed to avoid finalize insufficient data
    // cases).
    size_t guard = 0;
    while (guard < 60) {
        auto stateR = engine->getRealtimeSimilarityState(sid);
        if (stateR.isOk() && stateR.value.reliable)
            break;
        auto topUp = makeSine(440.0f, 0.04f, 44100.0f);
        UnifiedAudioEngine::Status st =
            engine->processAudioChunk(sid, std::span<const float>(topUp.data(), topUp.size()));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
        ++guard;
    }
    auto ready = engine->getRealtimeSimilarityState(sid);
    ASSERT_TRUE(ready.isOk());
    ASSERT_TRUE(ready.value.reliable)
        << "Similarity state never reliable for finalize fallback positive test";
    (void)engine->testSetFinalizeFallbackThreshold(sid, 0.40f);
    ASSERT_EQ(engine->testOverrideLastSimilarity(sid, 0.10f), UnifiedAudioEngine::Status::OK);
    UnifiedAudioEngine::Status fin = engine->finalizeSessionAnalysis(sid);
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto usedRes = engine->getFinalizeFallbackUsed(sid);
    ASSERT_TRUE(usedRes.isOk());
#ifndef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    EXPECT_TRUE(usedRes.value)
        << "Expected finalizeFallbackUsed to be true when finalize improves similarity";
#endif
    (void)engine->destroySession(sid);
}
