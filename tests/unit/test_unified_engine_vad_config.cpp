/**
 * @file test_unified_engine_vaTEST_F(UnifiedEngineVADConfigTes    auto configResult =
engine->getVADConfig(sessionId); ASSERT_TRUE(configResult.isOk()); const auto& config =
configResult.value;

    EXPECT_EQ(config.energy_threshold, 0.05f);
    EXPECT_EQ(config.window_duration, 0.03f);  // 30ms = 0.03 seconds
    EXPECT_EQ(config.min_sound_duration, 0.2f);  // 200ms = 0.2 seconds
    EXPECT_EQ(config.pre_buffer, 0.1f);  // 100ms = 0.1 seconds
    EXPECT_EQ(config.post_buffer, 0.15f);  // 150ms = 0.15 seconds
    EXPECT_TRUE(config.enabled);ultVADConfig) {
    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    const auto& config = configResult.value;

    EXPECT_EQ(config.energy_threshold, 0.01f);
    EXPECT_EQ(config.window_duration, 0.02f);  // 20ms = 0.02 seconds
    EXPECT_EQ(config.min_sound_duration, 0.1f);  // 100ms = 0.1 seconds
    EXPECT_TRUE(config.enabled);
}pp
 * @brief Unit tests for VAD configuration in UnifiedAudioEngine
 */

#include <chrono>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class UnifiedEngineVADConfigTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = *sessionResult;
    }

    void TearDown() override {
        if (engine && sessionId != 0) {
            engine->destroySession(sessionId);
        }
    }
};

TEST_F(UnifiedEngineVADConfigTest, GetDefaultVADConfig) {
    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    const auto& config = configResult.value;

    EXPECT_EQ(config.energy_threshold, 0.01f);
    EXPECT_EQ(config.window_duration, 0.02f);    // 20ms = 0.02 seconds
    EXPECT_EQ(config.min_sound_duration, 0.1f);  // 100ms = 0.1 seconds
    EXPECT_TRUE(config.enabled);
}

TEST_F(UnifiedEngineVADConfigTest, ConfigureVAD) {
    VADConfig customConfig;
    customConfig.energy_threshold = 0.05f;
    customConfig.window_duration = 0.03f;    // 30ms = 0.03 seconds
    customConfig.min_sound_duration = 0.2f;  // 200ms = 0.2 seconds
    customConfig.pre_buffer = 0.1f;          // 100ms = 0.1 seconds
    customConfig.post_buffer = 0.15f;        // 150ms = 0.15 seconds
    customConfig.enabled = true;

    auto setResult = engine->configureVAD(sessionId, customConfig);
    ASSERT_EQ(setResult, UnifiedAudioEngine::Status::OK);

    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    const auto& config = configResult.value;

    EXPECT_EQ(config.energy_threshold, 0.05f);
    EXPECT_EQ(config.window_duration, 0.03f);    // 30ms = 0.03 seconds
    EXPECT_EQ(config.min_sound_duration, 0.2f);  // 200ms = 0.2 seconds
    EXPECT_EQ(config.pre_buffer, 0.1f);          // 100ms = 0.1 seconds
    EXPECT_EQ(config.post_buffer, 0.15f);        // 150ms = 0.15 seconds
    EXPECT_TRUE(config.enabled);
}

TEST_F(UnifiedEngineVADConfigTest, EnableDisableVAD) {
    // Disable VAD
    auto disableResult = engine->disableVAD(sessionId);
    ASSERT_EQ(disableResult, UnifiedAudioEngine::Status::OK);

    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    EXPECT_FALSE(configResult.value.enabled);

    // Enable VAD
    auto enableResult = engine->enableVAD(sessionId, true);
    ASSERT_EQ(enableResult, UnifiedAudioEngine::Status::OK);

    configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    EXPECT_TRUE(configResult.value.enabled);
}
