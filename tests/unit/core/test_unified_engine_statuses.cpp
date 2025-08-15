#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class UnifiedEngineStatusTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;

    void SetUp() override {
        auto eR = UnifiedAudioEngine::create();
        ASSERT_TRUE(eR.isOk());
        engine = std::move(eR.value);
    }
};

TEST_F(UnifiedEngineStatusTest, InvalidSessionReturnsSessionNotFound) {
    SessionId bad = 999999;

    auto st1 = engine->destroySession(bad);
    EXPECT_EQ(st1, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto r1 = engine->getSimilarityScore(bad);
    EXPECT_FALSE(r1.isOk());
    EXPECT_EQ(r1.status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto st2 = engine->unloadMasterCall(bad);
    EXPECT_EQ(st2, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto r2 = engine->getEnhancedAnalysisSummary(bad);
    EXPECT_FALSE(r2.isOk());
    EXPECT_EQ(r2.status, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);

    auto st3 = engine->configureDTW(bad, 0.2f);
    EXPECT_EQ(st3, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
}

TEST_F(UnifiedEngineStatusTest, ConfigureDTWRejectsOutOfRangeValues) {
    auto sR = engine->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

    // Out of range ratios return INVALID_PARAMS
    EXPECT_EQ(engine->configureDTW(sid, -0.1f), UnifiedAudioEngine::Status::INVALID_PARAMS);
    EXPECT_EQ(engine->configureDTW(sid, 1.1f), UnifiedAudioEngine::Status::INVALID_PARAMS);

    // Valid range accepted
    EXPECT_EQ(engine->configureDTW(sid, 0.0f), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->configureDTW(sid, 0.5f), UnifiedAudioEngine::Status::OK);
    EXPECT_EQ(engine->configureDTW(sid, 1.0f), UnifiedAudioEngine::Status::OK);

    EXPECT_EQ(engine->destroySession(sid), UnifiedAudioEngine::Status::OK);
}

TEST_F(UnifiedEngineStatusTest, FinalizeInsufficientData) {
    auto sR = engine->createSession(44100.0f);
    ASSERT_TRUE(sR.isOk());
    SessionId sid = sR.value;

    // Without features, finalize should report INSUFFICIENT_DATA
    auto st = engine->finalizeSessionAnalysis(sid);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::INSUFFICIENT_DATA);

    EXPECT_EQ(engine->destroySession(sid), UnifiedAudioEngine::Status::OK);
}
