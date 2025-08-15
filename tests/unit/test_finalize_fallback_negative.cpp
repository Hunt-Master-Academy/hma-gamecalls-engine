// Negative finalize fallback test: ensure finalizeFallbackUsed stays false when finalize can't
// cross high threshold.
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

static std::vector<std::vector<float>> synthMasterNeg(size_t frames = 40, size_t dim = 13) {
    std::vector<std::vector<float>> feats(frames, std::vector<float>(dim));
    for (size_t f = 0; f < frames; ++f) {
        for (size_t d = 0; d < dim; ++d) {
            feats[f][d] = 0.20f + 0.03f * std::sin(0.055 * static_cast<double>(f))
                          + 0.002f * static_cast<float>(d);
        }
    }
    return feats;
}

static std::vector<float> makeMismatch(float sr, float seconds) {
    size_t n = static_cast<size_t>(sr * seconds);
    std::vector<float> buf(n);
    uint32_t seed = 0xC0FFEEu;
    for (size_t i = 0; i < n; ++i) {
        seed = seed * 1664525u + 1013904223u;
        // Uniform noise in [-0.3,0.3]
        float val = ((seed >> 8) & 0xFFFF) / 65535.0f;  // [0,1]
        val = (val - 0.5f) * 0.6f;
        buf[i] = val;
    }
    return buf;
}

TEST(FinalizeFallbackNegativeTest, FallbackFlagFalseWhenNoThresholdCross) {
    bool hooksEnabled = false;
#ifdef HUNTMASTER_TEST_HOOKS
    hooksEnabled = true;
#endif
    auto engRes = UnifiedAudioEngine::create();
    ASSERT_TRUE(engRes.isOk());
    auto engine = std::move(engRes.value);
    auto sRes = engine->createSession(44100.0f);
    ASSERT_TRUE(sRes.isOk());
    SessionId sid = sRes.value;
    auto load = engine->loadMasterCall(sid, "test_sine_440");
    if (load != UnifiedAudioEngine::Status::OK) {
        ASSERT_EQ(engine->testInjectMasterCallFeatures(sid, synthMasterNeg()),
                  UnifiedAudioEngine::Status::OK);
    }
    (void)engine->testSetFinalizeFallbackThreshold(
        sid, 0.99f);  // very high threshold; finalize shouldn't cross
    (void)engine->testSetFinalizeFallbackThreshold(
        sid, 0.50f);  // threshold lower than pre-finalize similarity
    auto noise = makeMismatch(44100.0f, 0.8f);
    const size_t chunk = 1024;
    for (size_t i = 0; i < noise.size(); i += chunk) {
        const size_t len = std::min(chunk, noise.size() - i);
        std::span<const float> spanChunk(&noise[i], len);
        UnifiedAudioEngine::Status st = engine->processAudioChunk(sid, spanChunk);
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    // Set a high realtime similarity so preFinalizeSimilarity >= threshold ensuring no fallback
    // flag
    ASSERT_EQ(engine->testOverrideLastSimilarity(sid, 0.80f), UnifiedAudioEngine::Status::OK);
    UnifiedAudioEngine::Status fin = engine->finalizeSessionAnalysis(sid);
    if (fin == UnifiedAudioEngine::Status::INSUFFICIENT_DATA) {
        // Feed additional synthetic noise deterministically until readiness frames >= min
        for (int extra = 0; extra < 32 && fin == UnifiedAudioEngine::Status::INSUFFICIENT_DATA;
             ++extra) {
            auto add = makeMismatch(44100.0f, 0.1f);
            size_t off = 0;
            const size_t chunk2 = 1024;
            while (off < add.size()) {
                size_t len = std::min(chunk2, add.size() - off);
                ASSERT_EQ(engine->processAudioChunk(sid, {add.data() + off, len}),
                          UnifiedAudioEngine::Status::OK);
                off += len;
                (void)engine->getSimilarityScore(sid);
            }
            auto rs = engine->getRealtimeSimilarityState(sid);
            ASSERT_TRUE(rs.isOk());
            if (rs.value.framesObserved >= rs.value.minFramesRequired) {
                fin = engine->finalizeSessionAnalysis(sid);
            }
        }
    }
    ASSERT_TRUE(fin == UnifiedAudioEngine::Status::OK
                || fin == UnifiedAudioEngine::Status::ALREADY_FINALIZED);
    auto usedRes = engine->getFinalizeFallbackUsed(sid);
    ASSERT_TRUE(usedRes.isOk());
    ASSERT_FALSE(usedRes.value) << "Fallback flag should remain false when pre>=threshold (pre set "
                                   "to 0.80, threshold 0.50)";
    (void)engine->destroySession(sid);
}
