// Runtime assertions under diagnostics-disabled build
#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"
using namespace huntmaster;

TEST(NoDiagnosticsBuildTest, SimilaritySnapshotInsufficient) {
    auto eng = UnifiedAudioEngine::create();
    ASSERT_TRUE(eng.isOk());
    auto engine = std::move(eng.value);
    auto s = engine->createSession(44100.0f);
    ASSERT_TRUE(s.isOk());
    auto snap = engine->getSimilarityScores(s.value);
    EXPECT_FALSE(snap.isOk());  // insufficient data expected when no frames yet
    (void)engine->destroySession(s.value);
}
