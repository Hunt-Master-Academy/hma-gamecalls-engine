#include <gtest/gtest.h>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

class CoreValidationTest : public ::testing::Test
{
protected:
    HuntmasterAudioEngine *engine;

    void SetUp() override
    {
        engine = &HuntmasterAudioEngine::getInstance();
        engine->initialize();
    }
};

TEST_F(CoreValidationTest, MFCCDeterministic)
{
    // Load same file multiple times
    std::vector<std::vector<float>> results;

    for (int i = 0; i < 10; i++)
    {
        engine->loadMasterCall("test_tone");
        // Get MFCC features (need to expose this)
        auto features = engine->getMasterFeatures();
        results.push_back(features[0]); // First frame
    }

    // Verify all identical
    for (int i = 1; i < 10; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            EXPECT_FLOAT_EQ(results[0][j], results[i][j]);
        }
    }
}

TEST_F(CoreValidationTest, DTWSelfSimilarity)
{
    engine->loadMasterCall("buck_grunt");

    // Process same file as "user attempt"
    auto score = engine->analyzeRecording("buck_grunt.wav");

    // Should be nearly perfect match
    EXPECT_GT(score, 0.99f); // Assuming normalized 0-1
}