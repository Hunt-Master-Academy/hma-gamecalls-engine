/**
 * @file test_unified_engine_vad_config.cpp
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
    const auto& config = *configResult;

    EXPECT_EQ(config.energy_threshold, 0.01f);
    EXPECT_EQ(config.window_duration, 20ms);
    EXPECT_EQ(config.min_sound_duration, 100ms);
    EXPECT_TRUE(config.enabled);
}

TEST_F(UnifiedEngineVADConfigTest, ConfigureVAD) {
    UnifiedAudioEngine::VADConfig customConfig;
    customConfig.energy_threshold = 0.05f;
    customConfig.window_duration = 30ms;
    customConfig.min_sound_duration = 200ms;
    customConfig.pre_buffer = 100ms;
    customConfig.post_buffer = 150ms;
    customConfig.enabled = true;

    auto setResult = engine->configureVAD(sessionId, customConfig);
    ASSERT_EQ(setResult, UnifiedAudioEngine::Status::OK);

    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    const auto& config = *configResult;

    EXPECT_EQ(config.energy_threshold, 0.05f);
    EXPECT_EQ(config.window_duration, 30ms);
    EXPECT_EQ(config.min_sound_duration, 200ms);
    EXPECT_EQ(config.pre_buffer, 100ms);
    EXPECT_EQ(config.post_buffer, 150ms);
    EXPECT_TRUE(config.enabled);
}

TEST_F(UnifiedEngineVADConfigTest, EnableDisableVAD) {
    // Disable VAD
    auto disableResult = engine->disableVAD(sessionId);
    ASSERT_EQ(disableResult, UnifiedAudioEngine::Status::OK);

    auto configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    EXPECT_FALSE(configResult->enabled);

    // Enable VAD
    auto enableResult = engine->enableVAD(sessionId, true);
    ASSERT_EQ(enableResult, UnifiedAudioEngine::Status::OK);

    configResult = engine->getVADConfig(sessionId);
    ASSERT_TRUE(configResult.isOk());
    EXPECT_TRUE(configResult->enabled);
}
