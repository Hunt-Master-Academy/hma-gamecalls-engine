#include <gtest/gtest.h>
#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/RealtimeScorer.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace huntmaster {

class RealtimeScorerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        scorer_ = std::make_unique<RealtimeScorer>();
        testMasterCallPath_ = "../data/master_calls/buck_grunt.wav";  // Path to a test master call
    }

    void TearDown() override { scorer_.reset(); }

    std::unique_ptr<RealtimeScorer> scorer_;
    std::string testMasterCallPath_;
};

TEST_F(RealtimeScorerTest, ResetFunctionalityTest) {
    std::cout << "TEST: Starting ResetFunctionalityTest" << std::endl;

    std::cout << "TEST: Setting master call..." << std::endl;
    bool result = scorer_->setMasterCall(testMasterCallPath_);
    std::cout << "TEST: setMasterCall result: " << (result ? "SUCCESS" : "FAILED") << std::endl;
    ASSERT_TRUE(result);

    std::cout << "TEST: Processing audio chunks..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << "TEST: Processing chunk " << i << std::endl;
        std::vector<float> audio(1024, 0.5f);
        bool audioSuccess = scorer_->processAudio(audio, 1).has_value();
        auto audioResult = scorer_->processAudio(audio, 1);
        std::cout << "TEST: processAudio result: "
                  << (audioResult.has_value() ? "SUCCESS" : "FAILED") << std::endl;
        ASSERT_TRUE(audioResult.has_value());
    }

    std::cout << "TEST: Calling reset..." << std::endl;
    scorer_->reset();
    std::cout << "TEST: Reset completed successfully" << std::endl;

    std::cout << "TEST: Test completed" << std::endl;
}

}  // namespace huntmaster
