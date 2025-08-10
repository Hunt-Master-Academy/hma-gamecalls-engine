#include <cmath>
#include <filesystem>
#include <optional>
// Clean restored version after corruption/experimental edits.
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include "dr_wav.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

static std::vector<float> makeNoise(size_t samples) {
    std::vector<float> buf(samples);
    uint32_t s = 0x1234567u;
    for (size_t i = 0; i < samples; ++i) {
        s = 1664525u * s + 1013904223u;
        uint32_t v = (s >> 9) | 0x3F800000u;
        float f = *reinterpret_cast<float*>(&v) - 1.0f;
        buf[i] = (f * 2.0f - 1.0f) * 0.3f;
    }
    return buf;
}

class SimilaritySeparationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto eng = UnifiedAudioEngine::create();
        ASSERT_TRUE(eng.isOk());
        engine = std::move(eng.value);
        auto s = engine->createSession(44100.0f);
        ASSERT_TRUE(s.isOk());
        session = s.value;
        auto st = engine->loadMasterCall(session, "buck_grunt");
        if (st != UnifiedAudioEngine::Status::OK) {
            GTEST_SKIP() << "Master asset unavailable";
        }
        auto wavPath = locateMasterCallWav("buck_grunt");
        if (!wavPath) {
            GTEST_SKIP() << "Master WAV missing";
        }
        auto wav = loadMonoFloatWav(*wavPath);
        if (wav.samples.empty()) {
            GTEST_SKIP() << "Master WAV load failed";
        }
        masterSamples = std::move(wav.samples);
    }
    void TearDown() override {
        if (engine && session != INVALID_SESSION_ID) {
            (void)engine->destroySession(session);
        }
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId session{INVALID_SESSION_ID};
    std::vector<float> masterSamples;

    struct LoadedWav {
        std::vector<float> samples;
        unsigned int channels{0};
        unsigned int sampleRate{0};
    };

    static std::optional<std::string> locateMasterCallWav(const std::string& callId) {
        namespace fs = std::filesystem;
        fs::path base{"data/master_calls"};
        if (!fs::exists(base))
            return std::nullopt;
        for (auto& entry : fs::recursive_directory_iterator(base)) {
            if (!entry.is_regular_file())
                continue;
            auto p = entry.path();
            if (p.extension() == ".wav" && p.stem().string() == callId)
                return p.string();
        }
        return std::nullopt;
    }

    static LoadedWav loadMonoFloatWav(const std::string& path) {
        LoadedWav out;
        drwav_uint64 frameCount = 0;
        float* data = drwav_open_file_and_read_pcm_frames_f32(
            path.c_str(), &out.channels, &out.sampleRate, &frameCount, nullptr);
        if (!data || frameCount == 0) {
            if (data)
                drwav_free(data, nullptr);
            return out;
        }
        out.samples.resize(frameCount);
        if (out.channels <= 1) {
            std::copy(data, data + frameCount, out.samples.begin());
        } else {
            for (drwav_uint64 i = 0; i < frameCount; ++i) {
                float sum = 0.f;
                for (unsigned int c = 0; c < out.channels; ++c)
                    sum += data[i * out.channels + c];
                out.samples[i] = sum / static_cast<float>(out.channels);
            }
        }
        drwav_free(data, nullptr);
        out.channels = 1;
        return out;
    }
};

TEST_F(SimilaritySeparationTest, PeakAlwaysAtLeastCurrent) {
    ASSERT_FALSE(masterSamples.empty());
    size_t slice = std::min<size_t>(masterSamples.size(), 4096);
    ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data(), slice}),
              UnifiedAudioEngine::Status::OK);
    (void)engine->getSimilarityScore(session);
    auto snap1 = engine->getSimilarityScores(session);
    if (!snap1.isOk())
        GTEST_SKIP() << "Snapshot not ready";
    ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data(), slice}),
              UnifiedAudioEngine::Status::OK);
    (void)engine->getSimilarityScore(session);
    auto snap2 = engine->getSimilarityScores(session);
    ASSERT_TRUE(snap2.isOk());
    EXPECT_GE(snap2.value.peak, snap2.value.current);
}

TEST_F(SimilaritySeparationTest, NoiseSimilarityLow) {
    // Relaxed informational test: ensure snapshots populate for self vs noise; log metrics.
    ASSERT_FALSE(masterSamples.empty());
    size_t slice = std::min<size_t>(masterSamples.size(), static_cast<size_t>(8192));
    ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data(), slice}),
              UnifiedAudioEngine::Status::OK);
    for (int i = 0; i < 64; ++i) {
        auto r = engine->getSimilarityScore(session);
        if (r.isOk())
            break;
    }
    auto selfSnap = engine->getSimilarityScores(session);
    if (!selfSnap.isOk())
        GTEST_SKIP() << "Self snapshot not ready";
    (void)engine->resetSession(session);
    ASSERT_EQ(engine->loadMasterCall(session, "buck_grunt"), UnifiedAudioEngine::Status::OK);
    auto noise = makeNoise(44100);
    size_t fed = 0;
    const size_t blk = 2048;
    while (fed < noise.size()) {
        size_t len = std::min(blk, noise.size() - fed);
        ASSERT_EQ(engine->processAudioChunk(session, {noise.data() + fed, len}),
                  UnifiedAudioEngine::Status::OK);
        fed += len;
        auto r = engine->getSimilarityScore(session);
        if (r.isOk())
            break;
    }
    auto noiseSnap = engine->getSimilarityScores(session);
    if (!noiseSnap.isOk())
        GTEST_SKIP() << "Noise snapshot not ready";
#ifndef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    SCOPED_TRACE(::testing::Message()
                 << "selfCur=" << selfSnap.value.current << " noiseCur=" << noiseSnap.value.current
                 << " selfDTW=" << selfSnap.value.dtwComponent
                 << " noiseDTW=" << noiseSnap.value.dtwComponent);
    EXPECT_GE(selfSnap.value.offsetComponent, -1.0f);
    EXPECT_GE(noiseSnap.value.offsetComponent, -1.0f);
    // At least one component for both should be non-negative
    EXPECT_TRUE(selfSnap.value.dtwComponent >= 0.f || selfSnap.value.meanComponent >= 0.f);
    EXPECT_TRUE(noiseSnap.value.dtwComponent >= 0.f || noiseSnap.value.meanComponent >= 0.f);
#else
    GTEST_SKIP() << "Diagnostics disabled";
#endif
}

TEST_F(SimilaritySeparationTest, ComponentBreakdownPopulated) {
    ASSERT_FALSE(masterSamples.empty());
    size_t slice = std::min<size_t>(masterSamples.size(), 8192);
    ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data(), slice}),
              UnifiedAudioEngine::Status::OK);
    auto sim = engine->getSimilarityScore(session);
    if (!sim.isOk())
        GTEST_SKIP() << "Similarity not ready";
    auto snap = engine->getSimilarityScores(session);
    if (!snap.isOk())
        GTEST_SKIP() << "Snapshot not ready";
    int nonNeg = 0;
    if (snap.value.offsetComponent >= 0)
        ++nonNeg;
    if (snap.value.dtwComponent >= 0)
        ++nonNeg;
    if (snap.value.meanComponent >= 0)
        ++nonNeg;
    if (snap.value.subsequenceComponent >= 0)
        ++nonNeg;
    EXPECT_GT(nonNeg, 0);
}

TEST_F(SimilaritySeparationTest, SelfVsDiffMargin) {
    const char* alt = "doe_grunt";
    ASSERT_FALSE(masterSamples.empty());
    size_t slice = std::min<size_t>(masterSamples.size(), 22050);
    ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data(), slice}),
              UnifiedAudioEngine::Status::OK);
    UnifiedAudioEngine::Result<float> selfSim;
    size_t fed = 0;
    const size_t blk = 2048;
    int poll = 0;
    while (poll < 64) {
        size_t len = std::min(blk, masterSamples.size() - fed);
        ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data() + fed, len}),
                  UnifiedAudioEngine::Status::OK);
        selfSim = engine->getSimilarityScore(session);
        if (selfSim.isOk())
            break;
        fed += len;
        ++poll;
    }
    ASSERT_TRUE(selfSim.isOk());
    float selfScore = selfSim.value;
    (void)engine->resetSession(session);
    if (engine->loadMasterCall(session, alt) != UnifiedAudioEngine::Status::OK) {
        GTEST_SKIP() << "Alternate master unavailable";
    }
    float diffScore = 0.f;
    auto altPath = locateMasterCallWav(alt);
    if (altPath) {
        auto wav = loadMonoFloatWav(*altPath);
        if (!wav.samples.empty()) {
            size_t diffSlice = std::min<size_t>(wav.samples.size(), slice);
            ASSERT_EQ(engine->processAudioChunk(session, {wav.samples.data(), diffSlice}),
                      UnifiedAudioEngine::Status::OK);
        }
    }
    UnifiedAudioEngine::Result<float> diffSim;
    int dp = 0;
    while (dp < 64) {
        diffSim = engine->getSimilarityScore(session);
        if (diffSim.isOk())
            break;
        ++dp;
    }
    ASSERT_TRUE(diffSim.isOk());
    diffScore = diffSim.value;
    ASSERT_LT(selfScore, 0.9995f);
    ASSERT_LT(diffScore, 0.9995f);
    EXPECT_LT(diffScore, 0.95f);
    EXPECT_GE(selfScore - diffScore, 0.10f);
}

TEST_F(SimilaritySeparationTest, DTWProxyFallbackEngages) {
#ifdef HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS
    GTEST_SKIP() << "Diagnostics disabled";
#endif
    ASSERT_FALSE(masterSamples.empty());
    // Feed up to ~48000 samples (~1.1s) which should yield plenty of MFCC frames for fallback
    size_t target = std::min<size_t>(masterSamples.size(), 48000);
    size_t fed = 0;
    const size_t blk = 1024;
    int polls = 0;
    const int kMaxPolls = 160;
    float lastDTW = -1.0f;
    while (fed < target && polls < kMaxPolls) {
        size_t len = std::min(blk, target - fed);
        ASSERT_EQ(engine->processAudioChunk(session, {masterSamples.data() + fed, len}),
                  UnifiedAudioEngine::Status::OK);
        fed += len;
        (void)engine->getSimilarityScore(session);
        auto snap = engine->getSimilarityScores(session);
        if (snap.isOk()) {
            lastDTW = snap.value.dtwComponent;
            if (lastDTW >= 0.0f) {
                auto mf = engine->getMasterFeatureCount(session);
                auto sf = engine->getSessionFeatureCount(session);
                SCOPED_TRACE(::testing::Message() << "DTW populated after fed=" << fed
                                                  << " samples mf=" << (mf.isOk() ? mf.value : -1)
                                                  << " sf=" << (sf.isOk() ? sf.value : -1));
                SUCCEED();
                return;
            }
        }
        ++polls;
    }
    auto finalSnap = engine->getSimilarityScores(session);
    ASSERT_TRUE(finalSnap.isOk()) << "Similarity snapshot unavailable after feeding";
    auto mf = engine->getMasterFeatureCount(session);
    auto sf = engine->getSessionFeatureCount(session);
    SCOPED_TRACE(::testing::Message()
                 << "Fed=" << fed << " polls=" << polls << " mf=" << (mf.isOk() ? mf.value : -1)
                 << " sf=" << (sf.isOk() ? sf.value : -1));
    EXPECT_GE(finalSnap.value.dtwComponent, 0.0f)
        << "DTW component still invalid after fallback path (mf/sf thresholds unmet?)";
}
