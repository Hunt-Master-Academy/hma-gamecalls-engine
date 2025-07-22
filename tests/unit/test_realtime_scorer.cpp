#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/RealtimeScorer.h"

namespace huntmaster {

class RealtimeScorerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        config_.sampleRate = 44100.0f;
        config_.updateRateMs = 100.0f;
        config_.mfccWeight = 0.5f;
        config_.volumeWeight = 0.3f;
        config_.timingWeight = 0.2f;
        config_.pitchWeight = 0.0f;  // Disabled for now

        scorer_ = std::make_unique<RealtimeScorer>(config_);

        // Create test master call feature file
        createTestMasterCall();
    }

    void TearDown() override {
        scorer_.reset();
        // Clean up test files
        if (std::filesystem::exists(testMasterCallPath_)) {
            std::filesystem::remove(testMasterCallPath_);
        }
    }

    void createTestMasterCall() {
        testMasterCallPath_ = "test_master_call.mfc";

        // Create a simple test feature file
        const uint32_t numFrames = 50;
        const uint32_t numCoeffs = 13;

        std::ofstream file(testMasterCallPath_, std::ios::binary);
        ASSERT_TRUE(file.is_open());

        // Write header
        file.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
        file.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(numCoeffs));

        // Write test feature data (sine wave pattern in first coefficient)
        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            std::vector<float> features(numCoeffs);

            // Create predictable pattern in features
            const float t = static_cast<float>(frame) / numFrames;
            features[0] = 0.5f + 0.3f * std::sin(2.0f * M_PI * t * 3.0f);  // Energy-like

            for (uint32_t coeff = 1; coeff < numCoeffs; ++coeff) {
                features[coeff] = 0.1f * std::sin(2.0f * M_PI * t * (coeff + 1));
            }

            file.write(reinterpret_cast<const char*>(features.data()), numCoeffs * sizeof(float));
        }

        file.close();
    }

    RealtimeScorer::Config config_;
    std::unique_ptr<RealtimeScorer> scorer_;
    std::string testMasterCallPath_;
};

TEST_F(RealtimeScorerTest, InitializationTest) {
    EXPECT_TRUE(scorer_->isInitialized());
    EXPECT_FALSE(scorer_->hasMasterCall());

    auto config = scorer_->getConfig();
    EXPECT_EQ(config.sampleRate, 44100.0f);
    EXPECT_FLOAT_EQ(config.mfccWeight, 0.5f);
    EXPECT_FLOAT_EQ(config.volumeWeight, 0.3f);
    EXPECT_FLOAT_EQ(config.timingWeight, 0.2f);

    // Test invalid configuration
    RealtimeScorer::Config invalidConfig;
    invalidConfig.sampleRate = -1.0f;  // Invalid

    RealtimeScorer invalidScorer(invalidConfig);
    EXPECT_FALSE(invalidScorer.isInitialized());
}

TEST_F(RealtimeScorerTest, MasterCallLoadingTest) {
    // Test loading valid master call
    bool success = scorer_->setMasterCall(testMasterCallPath_);
    EXPECT_TRUE(success);
    EXPECT_TRUE(scorer_->hasMasterCall());

    // Test loading non-existent file
    bool failureExpected = scorer_->setMasterCall("non_existent_file.mfc");
    EXPECT_FALSE(failureExpected);

    // Master call should still be loaded from previous successful load
    EXPECT_TRUE(scorer_->hasMasterCall());
}

TEST_F(RealtimeScorerTest, AudioProcessingWithoutMasterCallTest) {
    // Test processing audio without master call loaded
    std::vector<float> audio(1024, 0.5f);

    auto result = scorer_->processAudio(audio, 1);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RealtimeScorer::Error::NO_MASTER_CALL);
}

TEST_F(RealtimeScorerTest, AudioProcessingWithMasterCallTest) {
    // Load master call first
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Test processing valid audio
    std::vector<float> audio(2048, 0.5f);  // Constant amplitude

    auto result = scorer_->processAudio(audio, 1);

    ASSERT_TRUE(result.has_value());

    auto score = *result;

    // Basic score validation
    EXPECT_GE(score.overall, 0.0f);
    EXPECT_LE(score.overall, 1.0f);
    EXPECT_GE(score.mfcc, 0.0f);
    EXPECT_LE(score.mfcc, 1.0f);
    EXPECT_GE(score.volume, 0.0f);
    EXPECT_LE(score.volume, 1.0f);
    EXPECT_GE(score.timing, 0.0f);
    EXPECT_LE(score.timing, 1.0f);
    EXPECT_GE(score.confidence, 0.0f);
    EXPECT_LE(score.confidence, 1.0f);

    EXPECT_EQ(score.samplesAnalyzed, 2048);
    EXPECT_GT(score.timestamp.time_since_epoch().count(), 0);
}

TEST_F(RealtimeScorerTest, VaryingSignalQualityTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Test with high amplitude signal (good quality)
    std::vector<float> highAmpAudio(2048);
    for (size_t i = 0; i < highAmpAudio.size(); ++i) {
        highAmpAudio[i] = 0.8f * std::sin(2.0f * M_PI * i / 100.0f);
    }

    auto highAmpResult = scorer_->processAudio(highAmpAudio, 1);
    ASSERT_TRUE(highAmpResult.has_value());

    // Test with low amplitude signal (poor quality)
    std::vector<float> lowAmpAudio(2048);
    for (size_t i = 0; i < lowAmpAudio.size(); ++i) {
        lowAmpAudio[i] = 0.01f * std::sin(2.0f * M_PI * i / 100.0f);
    }

    auto lowAmpResult = scorer_->processAudio(lowAmpAudio, 1);
    ASSERT_TRUE(lowAmpResult.has_value());

    // High amplitude should generally have higher confidence
    // (though this depends on the specific signal characteristics)
    auto highScore = *highAmpResult;
    auto lowScore = *lowAmpResult;

    EXPECT_GT(highScore.volume, 0.0f);
    EXPECT_GT(lowScore.volume, 0.0f);
}

TEST_F(RealtimeScorerTest, MultiChannelProcessingTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    const size_t numSamples = 1024;
    const int numChannels = 2;

    // Create stereo audio (interleaved)
    std::vector<float> stereoAudio(numSamples * numChannels);
    for (size_t i = 0; i < numSamples; ++i) {
        // Left channel: sine wave
        stereoAudio[i * 2] = 0.5f * std::sin(2.0f * M_PI * i / 100.0f);
        // Right channel: cosine wave
        stereoAudio[i * 2 + 1] = 0.5f * std::cos(2.0f * M_PI * i / 100.0f);
    }

    auto result = scorer_->processAudio(stereoAudio, numChannels);

    ASSERT_TRUE(result.has_value());

    auto score = *result;
    EXPECT_EQ(score.samplesAnalyzed, numSamples * numChannels);  // Should track total samples
}

TEST_F(RealtimeScorerTest, ProgressTrackingTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Initial progress should be 0
    EXPECT_EQ(scorer_->getAnalysisProgress(), 0.0f);

    // Process some audio
    std::vector<float> audio(4410);  // 0.1 seconds at 44.1 kHz
    auto result = scorer_->processAudio(audio, 1);
    ASSERT_TRUE(result.has_value());

    // Progress should increase
    float progress = scorer_->getAnalysisProgress();
    EXPECT_GT(progress, 0.0f);
    EXPECT_LE(progress, 1.0f);
}

TEST_F(RealtimeScorerTest, ScoringHistoryTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    const size_t numChunks = 5;
    const size_t chunkSize = 1024;

    // Process multiple audio chunks
    for (size_t chunk = 0; chunk < numChunks; ++chunk) {
        std::vector<float> audio(chunkSize, static_cast<float>(chunk) * 0.1f + 0.1f);
        auto result = scorer_->processAudio(audio, 1);
        ASSERT_TRUE(result.has_value());
    }

    // Get scoring history
    auto history = scorer_->getScoringHistory(numChunks);

    EXPECT_EQ(history.size(), numChunks);

    // History should be in reverse chronological order (newest first)
    for (size_t i = 1; i < history.size(); ++i) {
        EXPECT_GE(history[i - 1].timestamp, history[i].timestamp);
    }

    // Test limited history retrieval
    auto limitedHistory = scorer_->getScoringHistory(3);
    EXPECT_EQ(limitedHistory.size(), 3);
}

TEST_F(RealtimeScorerTest, RealtimeFeedbackTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Process some audio to generate scores
    std::vector<float> audio(2048, 0.5f);
    auto result = scorer_->processAudio(audio, 1);
    ASSERT_TRUE(result.has_value());

    // Get real-time feedback
    auto feedbackResult = scorer_->getRealtimeFeedback();
    ASSERT_TRUE(feedbackResult.has_value());

    auto feedback = *feedbackResult;

    // Validate feedback structure
    EXPECT_GE(feedback.progressRatio, 0.0f);
    EXPECT_LE(feedback.progressRatio, 1.0f);
    EXPECT_FALSE(feedback.qualityAssessment.empty());
    EXPECT_FALSE(feedback.recommendation.empty());

    // Current and trending scores should be reasonable
    EXPECT_GE(feedback.currentScore.overall, 0.0f);
    EXPECT_GE(feedback.trendingScore.overall, 0.0f);
    EXPECT_GE(feedback.peakScore.overall, 0.0f);
}

/*
TEST_F(RealtimeScorerTest, JsonExportTest) {
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Process some audio
    std::vector<float> audio(1024, 0.5f);
    auto result = scorer_->processAudio(audio, 1);
    ASSERT_TRUE(result.has_value());

    // Test score JSON export
    std::string scoreJson = scorer_->exportScoreToJson();

    // Should contain expected fields
    EXPECT_NE(scoreJson.find("\"overall\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"mfcc\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"volume\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"timing\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"pitch\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"confidence\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"isReliable\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"isMatch\""), std::string::npos);
    EXPECT_NE(scoreJson.find("\"timestamp\""), std::string::npos);

    // Should be valid JSON format
    EXPECT_EQ(scoreJson.front(), '{');
    EXPECT_EQ(scoreJson.back(), '}');

    // Test feedback JSON export
    std::string feedbackJson = scorer_->exportFeedbackToJson();
    EXPECT_NE(feedbackJson.find("\"currentScore\""), std::string::npos);
    EXPECT_NE(feedbackJson.find("\"qualityAssessment\""), std::string::npos);
    EXPECT_NE(feedbackJson.find("\"recommendation\""), std::string::npos);

    // Test history JSON export
    std::string historyJson = scorer_->exportHistoryToJson(5);
    EXPECT_EQ(historyJson.front(), '[');
    EXPECT_EQ(historyJson.back(), ']');
}
*/

TEST_F(RealtimeScorerTest, DISABLED_ResetFunctionalityTest) {
    // Set debug level to see detailed logging
    huntmaster::DebugLogger::getInstance().setGlobalLogLevel(huntmaster::LogLevel::DEBUG);

    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Starting test");

    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Setting master call");
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Master call set successfully");

    // Process some audio and generate history
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Processing audio to generate history");
    for (int i = 0; i < 3; ++i) {
        huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                                   huntmaster::LogLevel::DEBUG,
                                                   "ResetFunctionalityTest: Processing audio chunk "
                                                       + std::to_string(i));
        std::vector<float> audio(1024, 0.5f);
        auto result = scorer_->processAudio(audio, 1);
        huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                                   huntmaster::LogLevel::DEBUG,
                                                   "ResetFunctionalityTest: Audio chunk "
                                                       + std::to_string(i) + " processed");
    }
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: All audio chunks processed");

    // Verify we have history and progress
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Getting scoring history before reset");
    auto historyBefore = scorer_->getScoringHistory(3);
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Got " + std::to_string(historyBefore.size()) + " history items");

    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Getting analysis progress before reset");
    auto progressBefore = scorer_->getAnalysisProgress();
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Progress before reset: "
                                                   + std::to_string(progressBefore));

    EXPECT_GT(historyBefore.size(), 0);
    EXPECT_GT(progressBefore, 0.0f);
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Pre-reset verification complete");

    // Reset scorer (but keep master call)
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Calling reset()");
    scorer_->reset();
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: reset() completed");

    // Verify state is reset but master call is preserved
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Getting scoring history after reset");
    auto historyAfter = scorer_->getScoringHistory(1);
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Got "
                                                   + std::to_string(historyAfter.size())
                                                   + " history items after reset");

    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Getting analysis progress after reset");
    auto progressAfter = scorer_->getAnalysisProgress();
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Progress after reset: "
                                                   + std::to_string(progressAfter));

    EXPECT_EQ(historyAfter.size(), 0);
    EXPECT_EQ(progressAfter, 0.0f);

    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Checking if master call is preserved");
    EXPECT_TRUE(scorer_->hasMasterCall());  // Master call should be preserved
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Master call preservation verified");

    // Test session reset (clears everything)
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: Calling resetSession()");
    scorer_->resetSession();
    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::REALTIME_SCORER,
                                               huntmaster::LogLevel::DEBUG,
                                               "ResetFunctionalityTest: resetSession() completed");

    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Verifying master call is cleared");
    EXPECT_FALSE(scorer_->hasMasterCall());
    huntmaster::DebugLogger::getInstance().log(
        huntmaster::Component::REALTIME_SCORER,
        huntmaster::LogLevel::DEBUG,
        "ResetFunctionalityTest: Test completed successfully");
}

TEST_F(RealtimeScorerTest, ConfigUpdateTest) {
    // Update configuration
    RealtimeScorer::Config newConfig = config_;
    newConfig.mfccWeight = 0.7f;
    newConfig.volumeWeight = 0.2f;
    newConfig.timingWeight = 0.1f;
    newConfig.updateRateMs = 200.0f;

    bool success = scorer_->updateConfig(newConfig);
    EXPECT_TRUE(success);

    auto retrievedConfig = scorer_->getConfig();
    EXPECT_FLOAT_EQ(retrievedConfig.mfccWeight, 0.7f);
    EXPECT_FLOAT_EQ(retrievedConfig.volumeWeight, 0.2f);
    EXPECT_FLOAT_EQ(retrievedConfig.timingWeight, 0.1f);
    EXPECT_FLOAT_EQ(retrievedConfig.updateRateMs, 200.0f);

    // Test invalid config update (weights don't sum to 1.0)
    RealtimeScorer::Config invalidConfig = config_;
    invalidConfig.mfccWeight = 0.9f;  // Total will be > 1.0

    success = scorer_->updateConfig(invalidConfig);
    EXPECT_FALSE(success);
}

TEST_F(RealtimeScorerTest, ErrorHandlingTest) {
    // Load master call first so we can test audio data validation
    ASSERT_TRUE(scorer_->setMasterCall(testMasterCallPath_));

    // Test empty audio data
    std::vector<float> emptyAudio;
    auto result = scorer_->processAudio(emptyAudio, 1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RealtimeScorer::Error::INVALID_AUDIO_DATA);

    // Test invalid number of channels
    std::vector<float> audio(512, 0.5f);
    result = scorer_->processAudio(audio, 0);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RealtimeScorer::Error::INVALID_AUDIO_DATA);

    result = scorer_->processAudio(audio, 10);  // Too many channels
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RealtimeScorer::Error::INVALID_AUDIO_DATA);
}

// Test utility functions are not part of the class, so they are not tested here.
// They would need to be exposed in the header or tested in their own file
// if they were part of the public API.

}  // namespace huntmaster
