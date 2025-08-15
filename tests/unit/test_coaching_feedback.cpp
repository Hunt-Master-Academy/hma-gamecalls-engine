#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class CoachingFeedbackTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto er = UnifiedAudioEngine::create();
        ASSERT_TRUE(er.isOk());
        engine = std::move(er.value);
        auto sr = engine->createSession(44100.0f);
        ASSERT_TRUE(sr.isOk());
        sid = sr.value;
#ifdef HUNTMASTER_TEST_HOOKS
        // Provide minimal master features to satisfy pipelines
        std::vector<std::vector<float>> feats(8, std::vector<float>(13, 0.0f));
        for (size_t i = 0; i < feats.size(); ++i)
            feats[i][0] = static_cast<float>(i + 1);
        ASSERT_EQ(engine->testInjectMasterCallFeatures(sid, feats), UnifiedAudioEngine::Status::OK);
#endif
    }
    void TearDown() override {
        if (engine && sid != INVALID_SESSION_ID)
            (void)engine->destroySession(sid);
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sid{INVALID_SESSION_ID};
};

TEST_F(CoachingFeedbackTest, PositiveNudgeWhenAllGood) {
#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(engine->testSetEnhancedSummaryConfidences(sid, 0.95f, 0.92f, 0.91f),
              UnifiedAudioEngine::Status::OK);
#endif
    auto r = engine->getCoachingFeedback(sid);
    ASSERT_TRUE(r.isOk());
    bool found = false;
    for (const auto& s : r.value.suggestions) {
        if (s.find("Solid match") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CoachingFeedbackTest, SuggestionsForBadGradesAndQuiet) {
#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(engine->testSetEnhancedSummaryConfidences(sid, 0.2f, 0.3f, 0.35f),
              UnifiedAudioEngine::Status::OK);
    ASSERT_EQ(engine->testSetMasterCallRms(sid, 0.5f), UnifiedAudioEngine::Status::OK);
#endif
    // Feed silence and finalize to get negative loudnessDeviation
    std::vector<float> zeros(44100, 0.0f);
    for (size_t i = 0; i < zeros.size(); i += 2048) {
        size_t n = std::min<size_t>(2048, zeros.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&zeros[i], n)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    ASSERT_EQ(engine->finalizeSessionAnalysis(sid), UnifiedAudioEngine::Status::OK);
    auto r = engine->getCoachingFeedback(sid);
    ASSERT_TRUE(r.isOk());
    // Expect loudness increase suggestion and grade-related suggestions
    bool hasLoud = false, hasPitch = false, hasHarm = false, hasCad = false;
    for (const auto& s : r.value.suggestions) {
        hasLoud |= s.find("Increase volume") != std::string::npos;
        hasPitch |= s.find("pitch") != std::string::npos;
        hasHarm |= s.find("tone") != std::string::npos || s.find("harmonics") != std::string::npos;
        hasCad |= s.find("timing") != std::string::npos || s.find("rhythm") != std::string::npos;
    }
    EXPECT_TRUE(hasLoud);
    EXPECT_TRUE(hasPitch);
    EXPECT_TRUE(hasHarm);
    EXPECT_TRUE(hasCad);
}

TEST_F(CoachingFeedbackTest, SuggestionForTooLoud) {
#ifdef HUNTMASTER_TEST_HOOKS
    ASSERT_EQ(engine->testSetEnhancedSummaryConfidences(sid, 0.9f, 0.9f, 0.9f),
              UnifiedAudioEngine::Status::OK);
    ASSERT_EQ(engine->testSetMasterCallRms(sid, 0.2f), UnifiedAudioEngine::Status::OK);
#endif
    std::vector<float> loud(44100, 1.0f);
    for (size_t i = 0; i < loud.size(); i += 2048) {
        size_t n = std::min<size_t>(2048, loud.size() - i);
        ASSERT_EQ(engine->processAudioChunk(sid, std::span<const float>(&loud[i], n)),
                  UnifiedAudioEngine::Status::OK);
        (void)engine->getSimilarityScore(sid);
    }
    ASSERT_EQ(engine->finalizeSessionAnalysis(sid), UnifiedAudioEngine::Status::OK);
    auto r = engine->getCoachingFeedback(sid);
    ASSERT_TRUE(r.isOk());
    bool found = false;
    for (const auto& s : r.value.suggestions) {
        if (s.find("Reduce volume") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    // Also verify JSON export contains the reduce volume suggestion
    auto jr = engine->exportCoachingFeedbackToJson(sid);
    ASSERT_TRUE(jr.isOk());
    EXPECT_NE(jr.value.find("Reduce volume"), std::string::npos);
}
