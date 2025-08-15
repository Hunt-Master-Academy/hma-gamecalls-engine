#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

#ifdef HUNTMASTER_TEST_HOOKS
TEST(EnhancedSummaryStalenessTest, InvalidatesAfterVirtualTimeAdvance) {
    auto r = UnifiedAudioEngine::create();
    ASSERT_TRUE(r.isOk());
    auto eng = std::move(r.value);
    auto s = eng->createSession(44100.0f);
    ASSERT_TRUE(s.isOk());
    auto sid = s.value;
    ASSERT_EQ(eng->setEnhancedAnalyzersEnabled(sid, true), UnifiedAudioEngine::Status::OK);
    ASSERT_EQ(eng->testSetEnhancedSummaryConfidences(sid, 0.9f, 0.9f, 0.9f),
              UnifiedAudioEngine::Status::OK);
    auto summary1 = eng->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(summary1.isOk());
    ASSERT_TRUE(summary1.value.valid);
    // Advance virtual clock by > 2000 ms (staleness threshold)
    ASSERT_EQ(eng->testAdvanceVirtualClock(2500), UnifiedAudioEngine::Status::OK);
    auto summary2 = eng->getEnhancedAnalysisSummary(sid);
    ASSERT_TRUE(summary2.isOk());
    EXPECT_FALSE(summary2.value.valid)
        << "Summary should be invalidated after virtual time advance";
}
#else
TEST(EnhancedSummaryStalenessTest, HooksDisabled) {
    GTEST_SKIP() << "Hooks disabled";
}
#endif
