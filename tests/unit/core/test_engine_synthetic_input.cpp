#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

namespace {
std::vector<float> makeSine(float freq, float sr, size_t n) {
    std::vector<float> v(n);
    const float pi = 3.14159265358979323846f;
    const float w = 2.0f * pi * freq / sr;
    for (size_t i = 0; i < n; ++i)
        v[i] = std::sin(w * static_cast<float>(i)) * 0.5f;
    return v;
}
}  // namespace

TEST(EngineSyntheticInput, OverlayFromSegmentBufferWithoutRecording) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sidR = eng->createSession(16000.0f);
    ASSERT_TRUE(sidR.isOk());
    SessionId sid = sidR.value;

    // Inject a tiny synthetic master via test hook so overlay has master source
    std::vector<std::vector<float>> masterFrames(32, std::vector<float>(13, 0.0f));
    for (size_t i = 0; i < masterFrames.size(); ++i)
        masterFrames[i][0] = static_cast<float>(i) / 31.0f;
    ASSERT_EQ(eng->testInjectMasterCallFeatures(sid, masterFrames), UnifiedAudioEngine::Status::OK);

    // Feed synthetic audio through processAudioChunk (no device I/O)
    auto chunk = makeSine(440.0f, 16000.0f, 4096);
    ASSERT_EQ(eng->processAudioChunk(sid, std::span<const float>(chunk.data(), chunk.size())),
              UnifiedAudioEngine::Status::OK);

    UnifiedAudioEngine::WaveformOverlayConfig cfg;
    cfg.maxPoints = 128;  // small for test
    auto ov = eng->getWaveformOverlayData(sid, cfg);
    ASSERT_TRUE(ov.isOk());
    EXPECT_TRUE(ov.value.valid);
    EXPECT_GT(ov.value.userPeaks.size(), 0u);
    EXPECT_GT(ov.value.masterPeaks.size(), 0u);
    EXPECT_EQ(ov.value.userPeaks.size(), ov.value.masterPeaks.size());

    // Sanity: decimation > 0 and peaks <= 1
    EXPECT_GT(ov.value.decimation, 0u);
    auto inRange = [](float p) { return p >= 0.0f && p <= 1.0f; };
    EXPECT_TRUE(std::all_of(ov.value.userPeaks.begin(), ov.value.userPeaks.end(), inRange));
    EXPECT_TRUE(std::all_of(ov.value.masterPeaks.begin(), ov.value.masterPeaks.end(), inRange));
}

TEST(EngineSyntheticInput, MemoryBufferInfoWhenIdle) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);
    auto sidR = eng->createSession(44100.0f);
    ASSERT_TRUE(sidR.isOk());

    // Query buffer info without starting device recording; should return either INIT_FAILED or OK
    auto info = eng->getMemoryBufferInfo(sidR.value);
    // Some implementations may require recorder to be initialized; accept either but don't crash
    if (!info.isOk()) {
        EXPECT_EQ(info.error(), UnifiedAudioEngine::Status::INIT_FAILED);
    } else {
        EXPECT_GE(info.value.totalCapacityFrames, 0u);
    }
}
