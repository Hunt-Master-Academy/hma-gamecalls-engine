#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

// TODO: Add LoudnessNormalizationAccuracy test (MVP TODO Item: PENDING)
// TODO: Test RMS alignment accuracy with <2% error target
// TODO: Test normalization scalar calculation against analytic baseline
// TODO: Test clipping prevention through pre-scan headroom check

static std::vector<float> constantBuffer(float v, size_t n) {
    return std::vector<float>(n, v);
}

class LoudnessMetricsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto eng = UnifiedAudioEngine::create();
        ASSERT_TRUE(eng.isOk());
        engine = std::move(eng.value);
        auto s = engine->createSession(44100.0f);
        ASSERT_TRUE(s.isOk());
        sid = s.value;
        // Inject minimal master features to satisfy pipelines and set master RMS explicitly
#ifdef HUNTMASTER_TEST_HOOKS
        std::vector<std::vector<float>> feats(8, std::vector<float>(13, 0.0f));
        for (size_t i = 0; i < feats.size(); ++i)
            feats[i][0] = static_cast<float>(i + 1);
        ASSERT_EQ(engine->testInjectMasterCallFeatures(sid, feats), UnifiedAudioEngine::Status::OK);
#else
        // In non-test builds, attempt to load a default master call
        auto st = engine->loadMasterCall(sid, "test_sine_440");
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK)
            << "test_sine_440 master call should be available after path fix";
#endif
    }
    void TearDown() override {
        if (engine && sid != INVALID_SESSION_ID)
            (void)engine->destroySession(sid);
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sid{INVALID_SESSION_ID};
};

TEST_F(LoudnessMetricsTest, ZeroInput_UserRmsZero_NormalizationUnityAndDeviationMinusOne) {
#ifdef HUNTMASTER_TEST_HOOKS
    // Set master RMS to a known non-zero value
    ASSERT_EQ(engine->testSetMasterCallRms(sid, 0.5f), UnifiedAudioEngine::Status::OK);
#endif
    // Feed silent audio to generate features but keep userRms == 0
    std::vector<float> zeros(44100, 0.0f);
    for (size_t i = 0; i < zeros.size(); i += 2048) {
        size_t n = std::min<size_t>(2048, zeros.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&zeros[i], n)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    auto st = engine->finalizeSessionAnalysis(sid);
    ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    auto sumRes = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(sumRes.isOk());
    auto s = sumRes.value;
    // With userRms == 0, normalizationScalar remains default 1.0
    EXPECT_NEAR(s.normalizationScalar, 1.0f, 1e-6f);
    // loudnessDeviation when masterRms>0 and user=0 is (0-master)/master = -1
    EXPECT_LE(s.loudnessDeviation, 0.0f);
    EXPECT_NEAR(s.loudnessDeviation, -1.0f, 1e-5f);
}

TEST_F(LoudnessMetricsTest,
       ClippingInput_UserMuchLouder_NormalizationClampedLow_PositiveDeviation) {
#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(engine->testSetMasterCallRms(sid, 0.2f), UnifiedAudioEngine::Status::OK);
#endif
    // Feed a very loud clipped buffer
    auto buf = constantBuffer(1.0f, 44100);  // 1s at full scale
    for (size_t i = 0; i < buf.size(); i += 2048) {
        size_t n = std::min<size_t>(2048, buf.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&buf[i], n)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    auto st = engine->finalizeSessionAnalysis(sid);
    ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    auto sumRes = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(sumRes.isOk());
    auto s = sumRes.value;
    // userRms >> masterRms => normScalar < 1, clamped to >= 0.25
    EXPECT_GE(s.normalizationScalar, 0.25f);
    EXPECT_LE(s.normalizationScalar, 1.0f);
    // Deviation positive (user louder)
    EXPECT_GT(s.loudnessDeviation, 0.0f);
}

TEST_F(LoudnessMetricsTest, VeryQuietInput_UserMuchQuieter_NormalizationHigh_DeviationNegative) {
#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(engine->testSetMasterCallRms(sid, 0.6f), UnifiedAudioEngine::Status::OK);
#endif
    // Feed a very quiet buffer
    auto buf = constantBuffer(1e-3f, 44100);  // tiny amplitude
    for (size_t i = 0; i < buf.size(); i += 2048) {
        size_t n = std::min<size_t>(2048, buf.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&buf[i], n)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    auto st = engine->finalizeSessionAnalysis(sid);
    ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    auto sumRes = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(sumRes.isOk());
    auto s = sumRes.value;
    // userRms << masterRms => normScalar > 1, clamped <= 4
    EXPECT_GE(s.normalizationScalar, 1.0f);
    EXPECT_LE(s.normalizationScalar, 4.0f);
    // Deviation negative (user quieter)
    EXPECT_LT(s.loudnessDeviation, 0.0f);
}

// TODO: MVP TODO Item - LoudnessNormalizationAccuracy (PENDING)
// Test RMS alignment accuracy ≤2% as specified in MVP TODO acceptance criteria
TEST_F(LoudnessMetricsTest, LoudnessNormalizationAccuracy_WithinTwoPercentTarget) {
#ifdef HUNTMASTER_TEST_HOOKS
    // Set a known master RMS level
    float masterRMS = 0.4f;
    ASSERT_EQ(engine->testSetMasterCallRms(sid, masterRMS), UnifiedAudioEngine::Status::OK);

    // Generate user audio with different RMS level
    float userRMS = 0.2f;  // 50% of master level
    auto buf = constantBuffer(userRMS, 44100);

    // Process the audio
    for (size_t i = 0; i < buf.size(); i += 2048) {
        size_t remain = std::min(size_t(2048), buf.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&buf[i], remain)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }

    // Finalize to trigger normalization
    auto st = engine->finalizeSessionAnalysis(sid);
    ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);

    auto sumRes = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(sumRes.isOk());
    auto s = sumRes.value;

    // Calculate expected normalization scalar: masterRMS / userRMS = 0.4 / 0.2 = 2.0
    float expectedScalar = masterRMS / userRMS;

    // Verify normalization scalar accuracy within 2% tolerance (MVP TODO requirement)
    float scalarError = std::abs(s.normalizationScalar - expectedScalar) / expectedScalar;
    EXPECT_LE(scalarError, 0.02f)
        << "Normalization scalar error should be ≤2% (MVP TODO requirement)";

    // Verify loudness deviation accuracy within 2% tolerance
    float expectedDeviation = (userRMS - masterRMS) / masterRMS;  // Should be -50%
    float deviationError = std::abs(s.loudnessDeviation - expectedDeviation);
    EXPECT_LE(deviationError, 0.02f)
        << "Loudness deviation error should be ≤2% (MVP TODO requirement)";
#else
    GTEST_SKIP() << "Test hooks required for master RMS injection";
#endif
}
