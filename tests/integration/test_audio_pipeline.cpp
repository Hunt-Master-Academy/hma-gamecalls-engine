#include <cmath>
#include <iostream>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace huntmaster;

class AudioPipelineTest : public ::testing::Test {
  protected:
    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId;

    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk()) << "Failed to create UnifiedAudioEngine";
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(16000.0f);
        ASSERT_TRUE(sessionResult.isOk()) << "Failed to create session";
        sessionId = *sessionResult;

        // Configure VAD for this test
        VADConfig vadConfig;
        vadConfig.enabled = true;
        vadConfig.energy_threshold = 0.01f;
        vadConfig.window_duration = 0.030f;     // 30ms in seconds
        vadConfig.min_sound_duration = 0.100f;  // 100ms in seconds
        vadConfig.pre_buffer = 0.050f;          // 50ms in seconds
        vadConfig.post_buffer = 0.050f;         // 50ms in seconds
        auto configResult = engine->configureVAD(sessionId, vadConfig);
        ASSERT_EQ(configResult, UnifiedAudioEngine::Status::OK);
    }

    void TearDown() override {
        if (engine) {
            engine->destroySession(sessionId);
        }
    }

    // Helper to generate sine wave for voice
    std::vector<float> generate_voice(int duration_ms, float freq = 440.0f) {
        int num_samples = 16000 * duration_ms / 1000;
        std::vector<float> voice(num_samples);
        for (int i = 0; i < num_samples; ++i) {
            voice[i] = 0.5f * std::sin(2.0f * M_PI * freq * i / 16000.0f);
        }
        return voice;
    }

    // Helper to generate silence
    std::vector<float> generate_silence(int duration_ms) {
        int num_samples = 16000 * duration_ms / 1000;
        return std::vector<float>(num_samples, 0.0f);
    }
};

TEST_F(AudioPipelineTest, FullPipelineStreamProcessing) {
    std::cout << "\n=== Full Audio Pipeline Integration Test ===" << std::endl;

    // Create a stream of audio: 200ms silence, 300ms voice, 200ms silence, 300ms voice, 200ms
    // silence
    std::vector<float> audio_stream;
    auto silence1 = generate_silence(200);
    auto voice1 = generate_voice(300);
    auto silence2 = generate_silence(200);
    auto voice2 = generate_voice(300, 660.0f);  // Different frequency
    auto silence3 = generate_silence(200);

    audio_stream.insert(audio_stream.end(), silence1.begin(), silence1.end());
    audio_stream.insert(audio_stream.end(), voice1.begin(), voice1.end());
    audio_stream.insert(audio_stream.end(), silence2.begin(), silence2.end());
    audio_stream.insert(audio_stream.end(), voice2.begin(), voice2.end());
    audio_stream.insert(audio_stream.end(), silence3.begin(), silence3.end());

    // Process in small chunks to simulate streaming
    size_t chunk_size = 160 * 10;  // 100ms chunks
    size_t pos = 0;
    int chunk_num = 0;

    engine->resetSession(sessionId);

    while (pos < audio_stream.size()) {
        size_t current_chunk_size = std::min(chunk_size, audio_stream.size() - pos);
        std::span<const float> audio_chunk(audio_stream.data() + pos, current_chunk_size);

        auto processResult = engine->processAudioChunk(sessionId, audio_chunk);
        EXPECT_EQ(processResult, UnifiedAudioEngine::Status::OK);

        pos += current_chunk_size;
        chunk_num++;
    }

    // After processing the whole stream, get feature count
    auto featureCountResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountResult.isOk());
    int feature_count = *featureCountResult;

    std::cout << "✓ Total features extracted from stream: " << feature_count << std::endl;

    // We expect features only from the voice parts.
    // Total voice duration is 600ms. Current system extracts ~39 features with VAD.
    // Let's check for a reasonable range.
    EXPECT_GE(feature_count, 35);
    EXPECT_LE(feature_count, 50);

    // Now, process the same audio with VAD disabled and compare
    engine->resetSession(sessionId);
    engine->disableVAD(sessionId);

    pos = 0;
    while (pos < audio_stream.size()) {
        size_t current_chunk_size = std::min(chunk_size, audio_stream.size() - pos);
        std::span<const float> audio_chunk(audio_stream.data() + pos, current_chunk_size);
        engine->processAudioChunk(sessionId, audio_chunk);
        pos += current_chunk_size;
    }

    auto featureCountNoVADResult = engine->getFeatureCount(sessionId);
    ASSERT_TRUE(featureCountNoVADResult.isOk());
    int feature_count_no_vad = *featureCountNoVADResult;

    std::cout << "✓ Total features extracted without VAD: " << feature_count_no_vad << std::endl;

    // Total duration is 1200ms. Current system extracts ~82 features without VAD.
    EXPECT_GE(feature_count_no_vad, 75);
    EXPECT_LE(feature_count_no_vad, 95);

    EXPECT_GT(feature_count_no_vad, feature_count);

    std::cout << "✓ Pipeline test complete. VAD correctly filtered silent sections." << std::endl;
}
