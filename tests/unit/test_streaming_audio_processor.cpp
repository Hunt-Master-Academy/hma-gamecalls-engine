/**
 * @file test_streaming_audio_processor.cpp
 * @brief Comprehensive test suite for StreamingAudioProcessor
 *
 * This test suite provides thorough testing of the StreamingAudioProcessor
 * including initialization, real-time processing, voice activity detection
 * integration, quality assessment, and performance monitoring.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "core/StreamingAudioProcessor.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace huntmaster::core;
using namespace huntmaster::test;
using namespace std::chrono_literals;

class StreamingAudioProcessorTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Initialize audio engine
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isSuccess());
        engine_ = std::move(engineResult.value);
        ASSERT_TRUE(engine_->initialize().isSuccess());

        // Create session
        auto sessionResult = engine_->createSession();
        ASSERT_TRUE(sessionResult.isSuccess());
        sessionId_ = sessionResult.getValue();

        // Initialize processor
        processor_ = std::make_unique<StreamingAudioProcessor>();

        // Configure test parameters
        config_.sampleRate = 44100;
        config_.channels = 1;
        config_.bufferSize = 1024;
        config_.processingSampleRate = 44100;

        processingResults_.clear();
        vadResults_.clear();
        qualityResults_.clear();
        errorMessages_.clear();
    }

    void TearDown() override {
        if (processor_) {
            processor_->stop();
            processor_.reset();
        }

        if (engine_ && sessionId_ != 0) {
            engine_->endSession(sessionId_);
        }

        engine_.reset();
        TestFixtureBase::TearDown();
    }

    // Generate test audio data
    std::vector<float> generateTestAudio(int durationMs, float frequency = 440.0f) {
        int sampleCount = (config_.sampleRate * durationMs) / 1000;
        std::vector<float> samples(sampleCount);

        for (int i = 0; i < sampleCount; ++i) {
            float t = static_cast<float>(i) / config_.sampleRate;
            samples[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
        }

        return samples;
    }

    // Generate silence
    std::vector<float> generateSilence(int durationMs) {
        int sampleCount = (config_.sampleRate * durationMs) / 1000;
        return std::vector<float>(sampleCount, 0.0f);
    }

    // Callback functions for testing
    void onProcessingResult(const ProcessingResult& result) {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        processingResults_.push_back(result);
    }

    void onVADResult(const VADResult& result) {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        vadResults_.push_back(result);
    }

    void onQualityResult(const QualityResult& result) {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        qualityResults_.push_back(result);
    }

    void onError(const std::string& error) {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        errorMessages_.push_back(error);
    }

    std::unique_ptr<UnifiedAudioEngine> engine_;
    std::unique_ptr<StreamingAudioProcessor> processor_;
    SessionId sessionId_;
    StreamingAudioProcessor::Config config_;

    // Test results storage
    std::mutex resultsMutex_;
    std::vector<ProcessingResult> processingResults_;
    std::vector<VADResult> vadResults_;
    std::vector<QualityResult> qualityResults_;
    std::vector<std::string> errorMessages_;
};

// Basic initialization and configuration tests
TEST_F(StreamingAudioProcessorTest, InitializationTest) {
    EXPECT_FALSE(processor_->isInitialized());
    EXPECT_FALSE(processor_->isStreaming());

    // Test successful initialization
    auto result = processor_->initialize(config_);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(processor_->isInitialized());
    EXPECT_FALSE(processor_->isStreaming());
}

TEST_F(StreamingAudioProcessorTest, ConfigurationValidationTest) {
    // Test invalid sample rate
    StreamingAudioProcessor::Config invalidConfig = config_;
    invalidConfig.sampleRate = 0;

    auto result = processor_->initialize(invalidConfig);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(processor_->isInitialized());

    // Test invalid buffer size
    invalidConfig = config_;
    invalidConfig.bufferSize = 0;

    result = processor_->initialize(invalidConfig);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(processor_->isInitialized());

    // Test invalid channels
    invalidConfig = config_;
    invalidConfig.channels = 0;

    result = processor_->initialize(invalidConfig);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(processor_->isInitialized());
}

TEST_F(StreamingAudioProcessorTest, CallbackRegistrationTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register callbacks
    processor_->setProcessingCallback(
        [this](const ProcessingResult& result) { onProcessingResult(result); });

    processor_->setVADCallback([this](const VADResult& result) { onVADResult(result); });

    processor_->setQualityCallback(
        [this](const QualityResult& result) { onQualityResult(result); });

    processor_->setErrorCallback([this](const std::string& error) { onError(error); });

    // Callbacks should be registered successfully
    EXPECT_TRUE(processor_->hasProcessingCallback());
    EXPECT_TRUE(processor_->hasVADCallback());
    EXPECT_TRUE(processor_->hasQualityCallback());
    EXPECT_TRUE(processor_->hasErrorCallback());
}

// Streaming lifecycle tests
TEST_F(StreamingAudioProcessorTest, StreamingLifecycleTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Start streaming
    auto result = processor_->start();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(processor_->isStreaming());

    // Wait a bit to ensure streaming thread is running
    std::this_thread::sleep_for(100ms);

    // Stop streaming
    result = processor_->stop();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(processor_->isStreaming());
}

TEST_F(StreamingAudioProcessorTest, MultipleStartStopTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Test multiple start/stop cycles
    for (int i = 0; i < 3; ++i) {
        auto startResult = processor_->start();
        EXPECT_TRUE(startResult.isSuccess());
        EXPECT_TRUE(processor_->isStreaming());

        std::this_thread::sleep_for(50ms);

        auto stopResult = processor_->stop();
        EXPECT_TRUE(stopResult.isSuccess());
        EXPECT_FALSE(processor_->isStreaming());

        std::this_thread::sleep_for(10ms);
    }
}

// Audio processing tests
TEST_F(StreamingAudioProcessorTest, AudioProcessingTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register processing callback
    processor_->setProcessingCallback(
        [this](const ProcessingResult& result) { onProcessingResult(result); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Generate test audio
    auto testAudio = generateTestAudio(1000, 440.0f);  // 1 second of 440 Hz tone

    // Process audio in chunks
    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, testAudio.size());
        std::vector<float> chunk(testAudio.begin() + i, testAudio.begin() + endIdx);

        auto result = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(result.isSuccess());
    }

    // Wait for processing to complete
    std::this_thread::sleep_for(500ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check that we received processing results
    std::lock_guard<std::mutex> lock(resultsMutex_);
    EXPECT_GT(processingResults_.size(), 0);
}

TEST_F(StreamingAudioProcessorTest, SilenceProcessingTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register VAD callback
    processor_->setVADCallback([this](const VADResult& result) { onVADResult(result); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process silence
    auto silence = generateSilence(1000);  // 1 second of silence

    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < silence.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, silence.size());
        std::vector<float> chunk(silence.begin() + i, silence.begin() + endIdx);

        auto result = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(result.isSuccess());
    }

    // Wait for processing
    std::this_thread::sleep_for(500ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check VAD results - should detect silence
    std::lock_guard<std::mutex> lock(resultsMutex_);
    if (!vadResults_.empty()) {
        bool foundSilence = false;
        for (const auto& result : vadResults_) {
            if (result.state == VADState::SILENCE) {
                foundSilence = true;
                break;
            }
        }
        EXPECT_TRUE(foundSilence);
    }
}

TEST_F(StreamingAudioProcessorTest, VoiceActivityDetectionTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register VAD callback
    processor_->setVADCallback([this](const VADResult& result) { onVADResult(result); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process audio with voice activity (loud tone)
    auto voiceAudio = generateTestAudio(1000, 440.0f);  // 1 second of tone

    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < voiceAudio.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, voiceAudio.size());
        std::vector<float> chunk(voiceAudio.begin() + i, voiceAudio.begin() + endIdx);

        auto result = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(result.isSuccess());
    }

    // Wait for processing
    std::this_thread::sleep_for(500ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check VAD results - should detect voice activity
    std::lock_guard<std::mutex> lock(resultsMutex_);
    if (!vadResults_.empty()) {
        bool foundActivity = false;
        for (const auto& result : vadResults_) {
            if (result.state == VADState::ACTIVE || result.state == VADState::CANDIDATE) {
                foundActivity = true;
                break;
            }
        }
        EXPECT_TRUE(foundActivity);
    }
}

// Quality assessment tests
TEST_F(StreamingAudioProcessorTest, QualityAssessmentTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register quality callback
    processor_->setQualityCallback(
        [this](const QualityResult& result) { onQualityResult(result); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process high-quality audio
    auto highQualityAudio = generateTestAudio(1000, 440.0f);

    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < highQualityAudio.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, highQualityAudio.size());
        std::vector<float> chunk(highQualityAudio.begin() + i, highQualityAudio.begin() + endIdx);

        auto result = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(result.isSuccess());
    }

    // Wait for processing
    std::this_thread::sleep_for(500ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check quality results
    std::lock_guard<std::mutex> lock(resultsMutex_);
    EXPECT_GT(qualityResults_.size(), 0);

    if (!qualityResults_.empty()) {
        // Quality scores should be reasonable (0.0 to 1.0)
        for (const auto& result : qualityResults_) {
            EXPECT_GE(result.overallScore, 0.0f);
            EXPECT_LE(result.overallScore, 1.0f);
        }
    }
}

// Performance and stress tests
TEST_F(StreamingAudioProcessorTest, PerformanceMetricsTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process audio for a reasonable duration
    auto testAudio = generateTestAudio(2000, 440.0f);  // 2 seconds

    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, testAudio.size());
        std::vector<float> chunk(testAudio.begin() + i, testAudio.begin() + endIdx);

        auto result = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(result.isSuccess());
    }

    // Wait for processing
    std::this_thread::sleep_for(1000ms);

    // Get performance metrics
    auto metrics = processor_->getPerformanceMetrics();

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Validate performance metrics
    EXPECT_GT(metrics.totalProcessingTime, 0.0);
    EXPECT_GT(metrics.totalSamplesProcessed, 0);
    EXPECT_GT(metrics.averageProcessingTime, 0.0);
    EXPECT_GE(metrics.cpuUsage, 0.0);
    EXPECT_LE(metrics.cpuUsage, 100.0);
}

TEST_F(StreamingAudioProcessorTest, StressTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register error callback
    processor_->setErrorCallback([this](const std::string& error) { onError(error); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process large amounts of audio rapidly
    for (int i = 0; i < 100; ++i) {
        auto testAudio = generateTestAudio(100, 440.0f + i * 10.0f);  // Varying frequency

        size_t chunkSize = config_.bufferSize;
        for (size_t j = 0; j < testAudio.size(); j += chunkSize) {
            size_t endIdx = std::min(j + chunkSize, testAudio.size());
            std::vector<float> chunk(testAudio.begin() + j, testAudio.begin() + endIdx);

            auto result = processor_->processAudioChunk(chunk);
            EXPECT_TRUE(result.isSuccess());
        }

        // Brief pause to avoid overwhelming the system
        if (i % 10 == 0) {
            std::this_thread::sleep_for(10ms);
        }
    }

    // Wait for processing to complete
    std::this_thread::sleep_for(2000ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check for errors
    std::lock_guard<std::mutex> lock(resultsMutex_);
    if (!errorMessages_.empty()) {
        std::cout << "Errors during stress test:" << std::endl;
        for (const auto& error : errorMessages_) {
            std::cout << "  " << error << std::endl;
        }
    }

    // Should handle stress without critical errors
    EXPECT_LT(errorMessages_.size(), 10);  // Allow some minor errors under stress
}

// Error handling tests
TEST_F(StreamingAudioProcessorTest, ErrorHandlingTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Register error callback
    processor_->setErrorCallback([this](const std::string& error) { onError(error); });

    // Test processing without starting
    auto testAudio = generateTestAudio(100, 440.0f);
    auto result = processor_->processAudioChunk(testAudio);
    EXPECT_FALSE(result.isSuccess());  // Should fail when not streaming

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Test processing empty audio
    std::vector<float> emptyAudio;
    result = processor_->processAudioChunk(emptyAudio);
    EXPECT_FALSE(result.isSuccess());  // Should handle empty input gracefully

    // Test processing audio with invalid samples (NaN, infinity)
    std::vector<float> invalidAudio = {1.0f, std::numeric_limits<float>::quiet_NaN(), 0.5f};
    result = processor_->processAudioChunk(invalidAudio);
    // Should either succeed by filtering invalid samples or fail gracefully

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());
}

// Master audio integration tests
TEST_F(StreamingAudioProcessorTest, MasterAudioIntegrationTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Set master audio for comparison
    auto masterAudio = generateTestAudio(2000, 440.0f);  // 2 seconds reference
    auto result = processor_->setMasterAudio(masterAudio);
    EXPECT_TRUE(result.isSuccess());

    // Register processing callback
    processor_->setProcessingCallback(
        [this](const ProcessingResult& result) { onProcessingResult(result); });

    // Start streaming
    ASSERT_TRUE(processor_->start().isSuccess());

    // Process similar audio
    auto testAudio = generateTestAudio(1000, 440.0f);  // Similar frequency

    size_t chunkSize = config_.bufferSize;
    for (size_t i = 0; i < testAudio.size(); i += chunkSize) {
        size_t endIdx = std::min(i + chunkSize, testAudio.size());
        std::vector<float> chunk(testAudio.begin() + i, testAudio.begin() + endIdx);

        auto processResult = processor_->processAudioChunk(chunk);
        EXPECT_TRUE(processResult.isSuccess());
    }

    // Wait for processing
    std::this_thread::sleep_for(1000ms);

    // Stop streaming
    ASSERT_TRUE(processor_->stop().isSuccess());

    // Check processing results for similarity scores
    std::lock_guard<std::mutex> lock(resultsMutex_);
    EXPECT_GT(processingResults_.size(), 0);

    if (!processingResults_.empty()) {
        bool foundSimilarityScore = false;
        for (const auto& result : processingResults_) {
            if (result.similarityScore >= 0.0f && result.similarityScore <= 1.0f) {
                foundSimilarityScore = true;
                break;
            }
        }
        EXPECT_TRUE(foundSimilarityScore);
    }
}

// Cleanup and resource management tests
TEST_F(StreamingAudioProcessorTest, ResourceCleanupTest) {
    ASSERT_TRUE(processor_->initialize(config_).isSuccess());

    // Start and stop multiple times
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(processor_->start().isSuccess());
        std::this_thread::sleep_for(100ms);
        ASSERT_TRUE(processor_->stop().isSuccess());
    }

    // Cleanup should be handled automatically
    auto cleanupResult = processor_->cleanup();
    EXPECT_TRUE(cleanupResult.isSuccess());

    // Should be able to re-initialize after cleanup
    auto reinitResult = processor_->initialize(config_);
    EXPECT_TRUE(reinitResult.isSuccess());
}
