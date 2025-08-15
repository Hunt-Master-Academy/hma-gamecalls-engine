#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

namespace {
std::vector<float> genSine(float sr, float freq, float dur) {
    size_t n = static_cast<size_t>(dur * sr);
    std::vector<float> v(n);
    const double twoPiF = 2.0 * 3.14159265358979323846 * freq;
    for (size_t i = 0; i < n; ++i)
        v[i] = 0.4f * std::sin(twoPiF * (double)i / sr);
    return v;
}

std::vector<std::vector<float>> genMaster(size_t frames, size_t coeffs) {
    std::vector<std::vector<float>> f(frames, std::vector<float>(coeffs, 0.0f));
    for (size_t i = 0; i < frames; ++i)
        f[i][0] = static_cast<float>(i) / std::max<size_t>(1, frames - 1);
    return f;
}
}  // namespace

TEST(FinalizeSuccess, FinalizeReturnsOKAfterValidData) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sR = eng->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(eng->testInjectMasterCallFeatures(sid, genMaster(100, 13)),
              UnifiedAudioEngine::Status::OK);
#else
    GTEST_SKIP() << "Requires test hooks for master injection";
#endif

    // Enable in-memory recording and feed sufficient user audio
    ASSERT_EQ(eng->startMemoryRecording(sid, 1.0), UnifiedAudioEngine::Status::OK);
    auto audio = genSine(44100.0f, 440.0f, 0.5f);
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t end = std::min(audio.size(), i + chunk);
        std::span<const float> span(&audio[i], end - i);
        ASSERT_EQ(eng->processAudioChunk(sid, span), UnifiedAudioEngine::Status::OK);
    }

    // Finalize should succeed when sufficient data exists
    auto st = eng->finalizeSessionAnalysis(sid);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);

    // Cleanup
    EXPECT_EQ(eng->destroySession(sid), UnifiedAudioEngine::Status::OK);
}
