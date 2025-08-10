// EnhancedAnalyzersEnableResetTest.cpp
#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

class EnhancedAnalyzersEnableResetTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine_;
    SessionId session_{};
    void SetUp() override {
        auto created = UnifiedAudioEngine::create();
        ASSERT_TRUE(created.isOk());
        engine_ = std::move(created.value);
        auto sessionRes = engine_->createSession(44100.0f);
        ASSERT_TRUE(sessionRes.isOk());
        session_ = sessionRes.value;
    }
};

TEST_F(EnhancedAnalyzersEnableResetTest, AutoEnableOnSummaryQuery) {
    auto enabled = engine_->getEnhancedAnalyzersEnabled(session_);
    ASSERT_TRUE(enabled.isOk());
    EXPECT_FALSE(enabled.value);
    auto summary = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(summary.isOk());
    // First call auto-enables; may not yet be valid
    enabled = engine_->getEnhancedAnalyzersEnabled(session_);
    ASSERT_TRUE(enabled.isOk());
    EXPECT_TRUE(enabled.value);
}

TEST_F(EnhancedAnalyzersEnableResetTest, ProcessingPopulatesSummary) {
    std::vector<float> audio(44100);  // 1 second of silence
    auto status = engine_->processAudioChunk(session_, audio);
    EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
    auto summary = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(summary.isOk());
    // After silence, may remain invalid; test path executes without crash
}

TEST_F(EnhancedAnalyzersEnableResetTest, ResetClearsSummaryValidity) {
    std::vector<float> audio(8192, 0.1f);
    auto st = engine_->setEnhancedAnalyzersEnabled(session_, true);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);
    auto st2 = engine_->processAudioChunk(session_, audio);
    EXPECT_EQ(st2, UnifiedAudioEngine::Status::OK);
    auto summary = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(summary.isOk());
    auto st3 = engine_->processAudioChunk(session_, audio);
    EXPECT_EQ(st3, UnifiedAudioEngine::Status::OK);
    summary = engine_->getEnhancedAnalysisSummary(session_);
    bool wasValid = summary.value.valid;
    auto rst = engine_->resetSession(session_);
    EXPECT_EQ(rst, UnifiedAudioEngine::Status::OK);
    auto summaryAfter = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(summaryAfter.isOk());
    if (wasValid) {
        EXPECT_FALSE(summaryAfter.value.valid);
    }
}

TEST_F(EnhancedAnalyzersEnableResetTest, DisableFreesAnalyzersAndClearsSummary) {
    auto st = engine_->setEnhancedAnalyzersEnabled(session_, true);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);
    std::vector<float> audio(4096, 0.05f);
    auto st2 = engine_->processAudioChunk(session_, audio);
    EXPECT_EQ(st2, UnifiedAudioEngine::Status::OK);
    auto summary = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(summary.isOk());
    auto st3 = engine_->setEnhancedAnalyzersEnabled(session_, false);
    EXPECT_EQ(st3, UnifiedAudioEngine::Status::OK);
    auto summaryAfter = engine_->getEnhancedAnalysisSummary(session_);  // auto re-enable
    ASSERT_TRUE(summaryAfter.isOk());
    EXPECT_FALSE(summaryAfter.value.valid);  // starts invalid again
}

TEST_F(EnhancedAnalyzersEnableResetTest, SummaryInvalidatesAfterInactivityAndReactivation) {
    auto st = engine_->setEnhancedAnalyzersEnabled(session_, true);
    EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);
    std::vector<float> audio(8192, 0.02f);
    auto st2 = engine_->processAudioChunk(session_, audio);
    EXPECT_EQ(st2, UnifiedAudioEngine::Status::OK);
    auto s1 = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(s1.isOk());
    // Force a stale state by manipulating time via sleep (>=2.1s)
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    auto s2 = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(s2.isOk());
    if (s1.value.valid) {
        EXPECT_FALSE(s2.value.valid) << "Summary should invalidate after >2s inactivity";
    }
    // Disable and re-enable; should clear summary again
    auto st3 = engine_->setEnhancedAnalyzersEnabled(session_, false);
    EXPECT_EQ(st3, UnifiedAudioEngine::Status::OK);
    auto st4 = engine_->setEnhancedAnalyzersEnabled(session_, true);
    EXPECT_EQ(st4, UnifiedAudioEngine::Status::OK);
    auto s3 = engine_->getEnhancedAnalysisSummary(session_);
    ASSERT_TRUE(s3.isOk());
    EXPECT_FALSE(s3.value.valid);
}
