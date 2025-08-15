// This test demonstrates multi-frame MFCC feature generation by checking the feature count
// after feeding enough audio for multiple overlapping analysis windows.

#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class MFCCMultiFrameTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto engResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engResult.isOk());
        engine = std::move(engResult.value);
        auto sessResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessResult.isOk());
        session = sessResult.value;
    }
    void TearDown() override {
        if (engine && session != -1)
            engine->destroySession(session);
    }
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId session{-1};
};

TEST_F(MFCCMultiFrameTest, GeneratesMultipleMfccFrames) {
    // Assume MFCC frame size 512, hop 256 (as referenced in realtime readiness heuristic)
    // We want at least 8 frames: N = frame + (frames-1)*hop = 512 + 7*256 = 2304 samples
    const size_t samplesNeeded = 512 + 7 * 256;
    std::vector<float> audio(samplesNeeded);
    double freq = 350.0;
    double sr = 44100.0;
    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] =
            static_cast<float>(0.3 * std::sin(2.0 * M_PI * freq * (static_cast<double>(i) / sr)));
    }
    auto status =
        engine->processAudioChunk(session, std::span<const float>(audio.data(), audio.size()));
    ASSERT_EQ(status, UnifiedAudioEngine::Status::OK);

    auto featureCount = engine->getFeatureCount(session);
    ASSERT_TRUE(featureCount.isOk());

    // Expected MFCC frames formula: floor((N - frameSize)/hop) + 1
    size_t expectedFrames = 0;
    const size_t frameSize = 512;
    const size_t hop = 256;
    if (samplesNeeded >= frameSize) {
        expectedFrames = ((samplesNeeded - frameSize) / hop) + 1;
    }

#ifdef HUNTMASTER_TEST_HOOKS
    auto realtimeFrames = engine->testGetRealtimeFrameCount(session);
    if (realtimeFrames.isOk()) {
        std::cout << "[Diag] Expected MFCC frames: " << expectedFrames
                  << ", getFeatureCount(): " << featureCount.value
                  << ", realtime framesObserved: " << realtimeFrames.value << std::endl;
    }
#else
    std::cout << "[Diag] Expected MFCC frames: " << expectedFrames
              << ", getFeatureCount(): " << featureCount.value << std::endl;
#endif

    EXPECT_GE(featureCount.value, 8) << "Expected at least 8 MFCC frames from input audio";
    EXPECT_EQ(featureCount.value, static_cast<int>(expectedFrames))
        << "Feature count should match expected frame calculation";
}
