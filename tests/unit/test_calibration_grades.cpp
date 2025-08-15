// Calibration grade mapping tests (minimal deterministic)
// TODO: Add MicCalibrationAdvisor_HeadroomBounds test (MVP TODO Item: PENDING)
// TODO: Test headroom and noise floor calculation on synthetic fixtures
// TODO: Test recommendation bands match documented thresholds
// TODO: Add LatencyDriftCalibrator_SyntheticOffsetAndDrift test (MVP TODO Item: PENDING)
// TODO: Test offset (ms) and drift (ppm) estimation vs synthetic ground truth
// TODO: Test calibration advisors return deterministic results on synthetic fixtures
#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class CalibrationGradeTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto r = UnifiedAudioEngine::create();
        ASSERT_TRUE(r.isOk());
        engine = std::move(r.value);
        auto s = engine->createSession(44100.0f);
        ASSERT_TRUE(s.isOk());
        sid = s.value;
        ASSERT_EQ(engine->setEnhancedAnalyzersEnabled(sid, true), UnifiedAudioEngine::Status::OK);
    }
    void TearDown() override {
        if (engine && sid != INVALID_SESSION_ID) {
            (void)engine->destroySession(sid);
        }
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sid{INVALID_SESSION_ID};
};

#ifdef HUNTMASTER_TEST_HOOKS
static UnifiedAudioEngine::EnhancedAnalysisSummary
setAndFetch(UnifiedAudioEngine& e, SessionId sid, float pc, float hc, float tc) {
    auto st = e.testSetEnhancedSummaryConfidences(sid, pc, hc, tc);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);
    auto r = e.getEnhancedAnalysisSummary(sid);
    EXPECT_TRUE(r.isOk());
    return r.isOk() ? r.value : UnifiedAudioEngine::EnhancedAnalysisSummary{};
}

TEST_F(CalibrationGradeTest, GradeThresholdBoundaries) {
    struct Case {
        float v;
        char g;
    } cases[] = {{0.90f, 'A'},
                 {0.85f, 'A'},
                 {0.84f, 'B'},
                 {0.70f, 'B'},
                 {0.69f, 'C'},
                 {0.55f, 'C'},
                 {0.54f, 'D'},
                 {0.40f, 'D'},
                 {0.39f, 'E'},
                 {0.25f, 'E'},
                 {0.24f, 'F'},
                 {0.0f, 'F'}};
    for (auto c : cases) {
        auto s = setAndFetch(*engine, sid, c.v, c.v, c.v);
        ASSERT_TRUE(s.valid) << c.v;
        EXPECT_EQ(s.pitchGrade, c.g) << c.v;
        EXPECT_EQ(s.harmonicGrade, c.g) << c.v;
        EXPECT_EQ(s.cadenceGrade, c.g) << c.v;
    }
}

TEST_F(CalibrationGradeTest, RegradeOnSecondInjection) {
    auto first = setAndFetch(*engine, sid, 0.90f, 0.90f, 0.90f);
    ASSERT_TRUE(first.valid);
    EXPECT_EQ(first.pitchGrade, 'A');
    auto second = setAndFetch(*engine, sid, 0.10f, 0.10f, 0.10f);
    ASSERT_TRUE(second.valid);
    EXPECT_EQ(second.pitchGrade, 'F');
    EXPECT_EQ(second.harmonicGrade, 'F');
    EXPECT_EQ(second.cadenceGrade, 'F');
}

// TODO: MVP TODO Item - MicCalibrationAdvisor_HeadroomBounds (PENDING)
// Test headroom and noise floor calculation on synthetic fixtures
TEST_F(CalibrationGradeTest, MicCalibrationAdvisor_HeadroomBounds_SyntheticFixtures) {
    // Test with synthetic audio at known amplitude levels
    std::vector<float> syntheticAudio;

    // Create audio with known peak level (0.8) and noise floor (-60dB ≈ 0.001)
    float peakLevel = 0.8f;
    float noiseFloor = 0.001f;

    // Add some noise floor
    for (int i = 0; i < 4410; ++i) {
        syntheticAudio.push_back(noiseFloor * (2.0f * (float)rand() / RAND_MAX - 1.0f));
    }

    // Add signal at known peak level
    for (int i = 0; i < 8820; ++i) {
        syntheticAudio.push_back(peakLevel * sin(2.0 * M_PI * 1000.0 * i / 44100.0));
    }

    // Process the synthetic audio
    for (size_t i = 0; i < syntheticAudio.size(); i += 1024) {
        size_t remain = std::min(size_t(1024), syntheticAudio.size() - i);
        auto st =
            engine->processAudioChunk(sid, std::span<const float>(&syntheticAudio[i], remain));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    }

    // TODO: When MicCalibrationAdvisor is implemented, verify:
    // 1. Headroom calculation: (1.0 - peakLevel) = 0.2 (±1dB accuracy per MVP)
    // 2. Noise floor detection: ~-60dB (±1dB accuracy per MVP)
    // 3. Recommendation bands match documented thresholds

    // Placeholder assertion until API is available
    auto summary = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(summary.isOk()) << "Should be able to get summary for calibration analysis";

    // Future: auto calibration = engine->getMicCalibration(sid);
    // Future: ASSERT_TRUE(calibration.isOk());
    // Future: EXPECT_NEAR(calibration.value.headroomDb, 1.94, 1.0); // 20*log10(1.0/0.8) ≈ 1.94dB
    // Future: EXPECT_NEAR(calibration.value.noiseFloorDb, -60.0, 1.0);
}

// TODO: MVP TODO Item - LatencyDriftCalibrator_SyntheticOffsetAndDrift (PENDING)
// Test offset (ms) and drift (ppm) estimation vs synthetic ground truth
TEST_F(CalibrationGradeTest, LatencyDriftCalibrator_SyntheticOffsetAndDrift_GroundTruth) {
    // Create synthetic master and user audio with known offset and drift
    float sampleRate = 44100.0f;
    float knownOffsetMs = 50.0f;   // 50ms offset
    float knownDriftPpm = 100.0f;  // 100 ppm drift (samples per million)

    // Generate reference tone
    std::vector<float> masterTone;
    for (int i = 0; i < 22050; ++i) {  // 0.5 seconds
        masterTone.push_back(0.5f * sin(2.0 * M_PI * 1000.0 * i / sampleRate));
    }

    // Generate user tone with offset and drift
    std::vector<float> userTone;
    int offsetSamples = (int)(knownOffsetMs * sampleRate / 1000.0f);  // 50ms = ~2205 samples

    // Add initial silence (offset)
    for (int i = 0; i < offsetSamples; ++i) {
        userTone.push_back(0.0f);
    }

    // Add tone with slight drift (sample rate error simulation)
    float driftedSampleRate = sampleRate * (1.0f + knownDriftPpm / 1000000.0f);
    for (int i = 0; i < 22050; ++i) {
        userTone.push_back(0.5f * sin(2.0 * M_PI * 1000.0 * i / driftedSampleRate));
    }

    // Process the user audio (master would be loaded separately)
    for (size_t i = 0; i < userTone.size(); i += 1024) {
        size_t remain = std::min(size_t(1024), userTone.size() - i);
        auto st = engine->processAudioChunk(sid, std::span<const float>(&userTone[i], remain));
        ASSERT_EQ(st, UnifiedAudioEngine::Status::OK);
    }

    // TODO: When LatencyDriftCalibrator is implemented, verify:
    // 1. Offset estimation within ±1ms (MVP requirement)
    // 2. Drift estimation within ±10ppm (MVP requirement)

    // Placeholder assertion until API is available
    auto summary = engine->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(summary.isOk()) << "Should be able to get summary for latency/drift analysis";

    // Future: auto latencyDrift = engine->getLatencyDrift(sid);
    // Future: ASSERT_TRUE(latencyDrift.isOk());
    // Future: EXPECT_NEAR(latencyDrift.value.offsetMs, knownOffsetMs, 1.0f); // ±1ms accuracy
    // Future: EXPECT_NEAR(latencyDrift.value.driftPpm, knownDriftPpm, 10.0f); // ±10ppm accuracy
}
#else
TEST_F(CalibrationGradeTest, HooksDisabled) {
    GTEST_SKIP() << "Hooks disabled";
}
#endif
