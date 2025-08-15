// Clean finalize fallback flag test with synthetic master injection fallback
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

static std::vector<std::vector<float>> synthMasterFlag(size_t frames = 40, size_t dim = 13) {
    std::vector<std::vector<float>> v(frames, std::vector<float>(dim));
    for (size_t f = 0; f < frames; ++f) {
        for (size_t d = 0; d < dim; ++d) {
            v[f][d] = 0.18f + 0.04f * std::sin(0.045 * static_cast<double>(f))
                      + 0.0025f * static_cast<float>(d);
        }
    }
    return v;
}

static std::vector<float> makeSine(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> out(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = std::sin(2.0 * M_PI * freq * static_cast<double>(i) / sr) * 0.22f;
    }
    return out;
}

TEST(FinalizeFallbackFlagTest, FlagRemainsFalseWhenRealtimeAlreadyHigh) {
#ifndef HUNTMASTER_TEST_HOOKS
    GTEST_SKIP() << "Test hooks disabled";
#endif
    auto engRes = UnifiedAudioEngine::create();
    ASSERT_TRUE(engRes.isOk());
    auto engine = std::move(engRes.value);
    auto sRes = engine->createSession(44100.0f);
    ASSERT_TRUE(sRes.isOk());
    SessionId sid = sRes.value;
    auto load = engine->loadMasterCall(sid, "buck_grunt");
    if (load != UnifiedAudioEngine::Status::OK) {
        ASSERT_EQ(engine->testInjectMasterCallFeatures(sid, synthMasterFlag()),
                  UnifiedAudioEngine::Status::OK);
    }
    auto audio = makeSine(440.0f, 1.0f, 44100.0f);
    const size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        const size_t remain = std::min(chunk, audio.size() - i);
        std::span<const float> spanChunk(&audio[i], remain);
        UnifiedAudioEngine::Status st = engine->processAudioChunk(sid, spanChunk);
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    // Ensure reliability before finalize; top up with small slices if required.
    size_t guard = 0;
    while (guard < 60) {
        auto stateR = engine->getRealtimeSimilarityState(sid);
        if (stateR.isOk() && stateR.value.reliable)
            break;
        auto extra = makeSine(440.0f, 0.04f, 44100.0f);
        auto st =
            engine->processAudioChunk(sid, std::span<const float>(extra.data(), extra.size()));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
        ++guard;
    }
    auto ready = engine->getRealtimeSimilarityState(sid);
    ASSERT_TRUE(ready.isOk());
    ASSERT_TRUE(ready.value.reliable)
        << "Similarity state never reliable for finalize fallback flag test";
    UnifiedAudioEngine::Status fin = engine->finalizeSessionAnalysis(sid);
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto snap = engine->getSimilarityScores(sid);
    ASSERT_TRUE(snap.isOk());
#ifndef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    EXPECT_FALSE(snap.value.finalizeFallbackUsed);
#endif
    (void)engine->destroySession(sid);
}
