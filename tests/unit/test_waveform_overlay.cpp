// Unit test for waveform overlay data export
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

namespace {
std::vector<float> genSine(float sr, float freq, float dur) {
    size_t n = static_cast<size_t>(dur * sr);
    std::vector<float> v(n);
    for (size_t i = 0; i < n; ++i)
        v[i] = 0.5f * std::sin(2.0 * M_PI * freq * (double)i / sr);
    return v;
}

std::vector<std::vector<float>> genSyntheticMaster(size_t frames, size_t coeffs) {
    std::vector<std::vector<float>> f(frames, std::vector<float>(coeffs, 0.0f));
    for (size_t i = 0; i < frames; ++i) {
        f[i][0] = static_cast<float>(i) / static_cast<float>(std::max<size_t>(1, frames - 1));
    }
    return f;
}
}  // namespace

TEST(WaveformOverlayTest, ReturnsInsufficientWithoutData) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sR = eng->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

    auto ovR = eng->getWaveformOverlayData(sid, 128);
    ASSERT_FALSE(ovR.isOk());
    EXPECT_EQ(ovR.error(), UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    auto d1 = eng->destroySession(sid);
    EXPECT_EQ(d1, UnifiedAudioEngine::Status::OK);
}

TEST(WaveformOverlayTest, ProvidesOverlayAfterData) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sR = eng->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

#ifdef HUNTMASTER_TEST_HOOKS
    auto masterFrames = genSyntheticMaster(64, 13);
    ASSERT_EQ(eng->testInjectMasterCallFeatures(sid, masterFrames), UnifiedAudioEngine::Status::OK);
#else
    GTEST_SKIP() << "Overlay test requires test hooks to inject master features";
#endif
    auto audio = genSine(44100.0f, 440.0f, 0.25f);
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t end = std::min(audio.size(), i + chunk);
        std::span<const float> span(&audio[i], end - i);
        auto st = eng->processAudioChunk(sid, span);
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    }

    auto ovR = eng->getWaveformOverlayData(sid, 128);
    ASSERT_TRUE(ovR.isOk()) << "status=" << (int)ovR.error();
    auto data = ovR.value;
    EXPECT_TRUE(data.valid);
    EXPECT_GT(data.userPeaks.size(), 0u);
    EXPECT_EQ(data.userPeaks.size(), data.masterPeaks.size());
    EXPECT_LE(data.userPeaks.size(), 128u);
    EXPECT_GT(data.decimation, 0u);

    auto d2 = eng->destroySession(sid);
    EXPECT_EQ(d2, UnifiedAudioEngine::Status::OK);
}

TEST(WaveformOverlayTest, DecimationOverrideControlsPeakCount) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sR = eng->createSession(48000.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

#ifdef HUNTMASTER_TEST_HOOKS
    // Inject synthetic master features (no raw master), so energy approximation path is used
    auto masterFrames = genSyntheticMaster(100, 13);
    ASSERT_EQ(eng->testInjectMasterCallFeatures(sid, masterFrames), UnifiedAudioEngine::Status::OK);
#else
    GTEST_SKIP() << "Requires test hooks to inject master features";
#endif

    // Feed some user audio so userPeaks are generated
    auto audio = genSine(48000.0f, 220.0f, 0.5f);  // 24k samples
    // Enable in-memory recording so the full buffer is retained for overlay export
    ASSERT_EQ(eng->startMemoryRecording(sid, 1.0), UnifiedAudioEngine::Status::OK);
    size_t chunk = 1024;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t end = std::min(audio.size(), i + chunk);
        std::span<const float> span(&audio[i], end - i);
        ASSERT_EQ(eng->processAudioChunk(sid, span), UnifiedAudioEngine::Status::OK);
    }

    UnifiedAudioEngine::WaveformOverlayConfig cfg100;
    cfg100.maxPoints = 4096;              // large to avoid limiting
    cfg100.userDecimationOverride = 100;  // force decimation

    UnifiedAudioEngine::WaveformOverlayConfig cfg200 = cfg100;
    cfg200.userDecimationOverride = 200;

    auto ov100 = eng->getWaveformOverlayData(sid, cfg100);
    ASSERT_TRUE(ov100.isOk()) << (int)ov100.error();
    auto ov200 = eng->getWaveformOverlayData(sid, cfg200);
    ASSERT_TRUE(ov200.isOk()) << (int)ov200.error();

    auto d100 = ov100.value;
    auto d200 = ov200.value;
    EXPECT_TRUE(d100.valid && d200.valid);
    EXPECT_EQ(d100.decimation, 100u);
    EXPECT_EQ(d200.decimation, 200u);
    // Smaller decimation -> more buckets
    EXPECT_GE(d100.userPeaks.size(), d200.userPeaks.size());
    EXPECT_EQ(d100.userPeaks.size(), d100.masterPeaks.size());
    EXPECT_EQ(d200.userPeaks.size(), d200.masterPeaks.size());

    EXPECT_EQ(eng->destroySession(sid), UnifiedAudioEngine::Status::OK);
}

TEST(WaveformOverlayTest, EnergyMappingAffectsMasterPeaks) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto eng = std::move(engR.value);

    auto sR = eng->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

#ifdef HUNTMASTER_TEST_HOOKS
    // Linear ramp energy across frames
    auto masterFrames = genSyntheticMaster(120, 13);
    ASSERT_EQ(eng->testInjectMasterCallFeatures(sid, masterFrames), UnifiedAudioEngine::Status::OK);
#else
    GTEST_SKIP() << "Requires test hooks to inject master features";
#endif

    // Provide user audio to define userPeaks length deterministically
    auto audio = genSine(44100.0f, 440.0f, 0.3f);  // ~13230 samples
    ASSERT_EQ(eng->startMemoryRecording(sid, 1.0), UnifiedAudioEngine::Status::OK);
    size_t chunk = 2048;
    for (size_t i = 0; i < audio.size(); i += chunk) {
        size_t end = std::min(audio.size(), i + chunk);
        std::span<const float> span(&audio[i], end - i);
        ASSERT_EQ(eng->processAudioChunk(sid, span), UnifiedAudioEngine::Status::OK);
    }

    UnifiedAudioEngine::WaveformOverlayConfig cfgBase;
    cfgBase.maxPoints = 4096;
    cfgBase.userDecimationOverride = 110;  // ensures around 120 buckets

    auto cfgLin = cfgBase;
    cfgLin.energyMap = UnifiedAudioEngine::WaveformOverlayConfig::EnergyMap::Linear;
    cfgLin.preferEnergyApprox = true;
    auto cfgSqrt = cfgBase;
    cfgSqrt.energyMap = UnifiedAudioEngine::WaveformOverlayConfig::EnergyMap::Sqrt;
    cfgSqrt.preferEnergyApprox = true;

    auto ovLinR = eng->getWaveformOverlayData(sid, cfgLin);
    ASSERT_TRUE(ovLinR.isOk()) << (int)ovLinR.error();
    auto ovSqrtR = eng->getWaveformOverlayData(sid, cfgSqrt);
    ASSERT_TRUE(ovSqrtR.isOk()) << (int)ovSqrtR.error();

    auto lin = ovLinR.value;
    auto sq = ovSqrtR.value;
    ASSERT_TRUE(lin.valid && sq.valid);
    ASSERT_EQ(lin.masterPeaks.size(), sq.masterPeaks.size());
    ASSERT_GE(lin.masterPeaks.size(), 2u);
    double l1 = 0.0;
    for (size_t i = 0; i < lin.masterPeaks.size(); ++i) {
        l1 += std::fabs(static_cast<double>(sq.masterPeaks[i])
                        - static_cast<double>(lin.masterPeaks[i]));
    }
    EXPECT_GT(l1, 1e-4) << "Energy mapping should alter the master peak envelope";

    EXPECT_EQ(eng->destroySession(sid), UnifiedAudioEngine::Status::OK);
}
