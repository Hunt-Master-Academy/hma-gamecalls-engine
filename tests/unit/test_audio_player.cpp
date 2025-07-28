/**
 * @file test_audio_player.cpp
 * @brief Comprehensive test suite for AudioPlayer
 *
 * This test suite provides thorough testing of the AudioPlayer class
 * including file loading, playback control, volume management, and
 * position tracking for wildlife call reproduction.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/AudioPlayer.h"

using namespace huntmaster;
using namespace huntmaster::test;
using namespace std::chrono_literals;

class AudioPlayerTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        player_ = std::make_unique<AudioPlayer>();

        // Create test audio files
        createTestWavFile("test_audio_short.wav", 1.0f, 440.0f);  // 1 second, 440 Hz
        createTestWavFile("test_audio_long.wav", 3.0f, 880.0f);   // 3 seconds, 880 Hz
        createEmptyWavFile("test_audio_empty.wav");
        createInvalidWavFile("test_audio_invalid.wav");
    }

    void TearDown() override {
        if (player_) {
            player_->stop();
            player_.reset();
        }

        // Clean up test files
        removeTestFile("test_audio_short.wav");
        removeTestFile("test_audio_long.wav");
        removeTestFile("test_audio_empty.wav");
        removeTestFile("test_audio_invalid.wav");
        removeTestFile("nonexistent.wav");

        TestFixtureBase::TearDown();
    }

    // Helper function to create a test WAV file
    void createTestWavFile(const std::string& filename, float duration, float frequency) {
        const int sampleRate = 44100;
        const int channels = 1;
        const int bitsPerSample = 16;
        const int numSamples = static_cast<int>(duration * sampleRate);

        std::vector<int16_t> samples(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float value = 0.5f * std::sin(2.0f * M_PI * frequency * t);
            samples[i] = static_cast<int16_t>(value * 32767.0f);
        }

        // Create a basic WAV file
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            FAIL() << "Could not create test file: " << filename;
            return;
        }

        // WAV header
        uint32_t fileSize = 36 + numSamples * channels * (bitsPerSample / 8);
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1;  // PCM
        uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
        uint16_t blockAlign = channels * (bitsPerSample / 8);
        uint32_t dataSize = numSamples * channels * (bitsPerSample / 8);

        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        file.write("fmt ", 4);
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        file.write(reinterpret_cast<const char*>(&channels), 2);
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&dataSize), 4);

        // Audio data
        file.write(reinterpret_cast<const char*>(samples.data()), dataSize);
        file.close();
    }

    // Helper function to create an empty WAV file
    void createEmptyWavFile(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            FAIL() << "Could not create empty test file: " << filename;
            return;
        }

        // Minimal invalid WAV header
        uint32_t fileSize = 36;
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1;
        uint16_t channels = 1;
        uint32_t sampleRate = 44100;
        uint32_t byteRate = 44100 * 1 * 2;
        uint16_t blockAlign = 2;
        uint16_t bitsPerSample = 16;
        uint32_t dataSize = 0;

        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        file.write("fmt ", 4);
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        file.write(reinterpret_cast<const char*>(&channels), 2);
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&dataSize), 4);

        file.close();
    }

    // Helper function to create an invalid WAV file
    void createInvalidWavFile(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            FAIL() << "Could not create invalid test file: " << filename;
            return;
        }

        // Write invalid data
        file.write("INVALID", 7);
        file.close();
    }

    // Helper function to remove test files
    void removeTestFile(const std::string& filename) {
        std::remove(filename.c_str());
    }

    std::unique_ptr<AudioPlayer> player_;
};

// Basic functionality tests
TEST_F(AudioPlayerTest, ConstructorDestructorTest) {
    // Constructor should create a valid player
    EXPECT_NE(player_, nullptr);

    // Should not be playing initially
    EXPECT_FALSE(player_->isPlaying());

    // Volume should be at default (1.0)
    EXPECT_FLOAT_EQ(player_->getVolume(), 1.0f);

    // Position should be 0
    EXPECT_DOUBLE_EQ(player_->getCurrentPosition(), 0.0);

    // Duration should be 0 (no file loaded)
    EXPECT_DOUBLE_EQ(player_->getDuration(), 0.0);
}

// File loading tests
TEST_F(AudioPlayerTest, LoadValidFileTest) {
    // Load a valid WAV file
    bool result = player_->loadFile("test_audio_short.wav");
    EXPECT_TRUE(result);

    // Duration should be approximately 1 second
    double duration = player_->getDuration();
    EXPECT_GT(duration, 0.9);
    EXPECT_LT(duration, 1.1);

    // Should not be playing after loading
    EXPECT_FALSE(player_->isPlaying());
}

TEST_F(AudioPlayerTest, LoadLongFileTest) {
    // Load a longer WAV file
    bool result = player_->loadFile("test_audio_long.wav");
    EXPECT_TRUE(result);

    // Duration should be approximately 3 seconds
    double duration = player_->getDuration();
    EXPECT_GT(duration, 2.9);
    EXPECT_LT(duration, 3.1);
}

TEST_F(AudioPlayerTest, LoadNonexistentFileTest) {
    // Try to load a file that doesn't exist
    bool result = player_->loadFile("nonexistent.wav");
    EXPECT_FALSE(result);

    // Duration should remain 0
    EXPECT_DOUBLE_EQ(player_->getDuration(), 0.0);
}

TEST_F(AudioPlayerTest, LoadInvalidFileTest) {
    // Try to load an invalid audio file
    bool result = player_->loadFile("test_audio_invalid.wav");
    EXPECT_FALSE(result);

    // Duration should remain 0
    EXPECT_DOUBLE_EQ(player_->getDuration(), 0.0);
}

TEST_F(AudioPlayerTest, LoadEmptyFileTest) {
    // Try to load an empty audio file
    bool result = player_->loadFile("test_audio_empty.wav");
    // This might succeed or fail depending on implementation
    // If it succeeds, duration should be near 0
    if (result) {
        double duration = player_->getDuration();
        EXPECT_LT(duration, 0.1);
    }
}

TEST_F(AudioPlayerTest, LoadMultipleFilesTest) {
    // Load first file
    EXPECT_TRUE(player_->loadFile("test_audio_short.wav"));
    double firstDuration = player_->getDuration();
    EXPECT_GT(firstDuration, 0.9);
    EXPECT_LT(firstDuration, 1.1);

    // Load second file (should replace first)
    EXPECT_TRUE(player_->loadFile("test_audio_long.wav"));
    double secondDuration = player_->getDuration();
    EXPECT_GT(secondDuration, 2.9);
    EXPECT_LT(secondDuration, 3.1);

    // Duration should have changed
    EXPECT_NE(firstDuration, secondDuration);
}

// Playback control tests
TEST_F(AudioPlayerTest, BasicPlaybackTest) {
    // Load file and play
    ASSERT_TRUE(player_->loadFile("test_audio_short.wav"));

    bool playResult = player_->play();
    EXPECT_TRUE(playResult);
    EXPECT_TRUE(player_->isPlaying());

    // Let it play for a short time
    std::this_thread::sleep_for(100ms);

    // Position should have advanced
    double position = player_->getCurrentPosition();
    EXPECT_GT(position, 0.0);

    // Stop playback
    player_->stop();
    EXPECT_FALSE(player_->isPlaying());
}

TEST_F(AudioPlayerTest, PlayWithoutLoadTest) {
    // Try to play without loading a file
    bool playResult = player_->play();
    EXPECT_FALSE(playResult);
    EXPECT_FALSE(player_->isPlaying());
}

TEST_F(AudioPlayerTest, MultiplePlayCallsTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_short.wav"));

    // First play call
    EXPECT_TRUE(player_->play());
    EXPECT_TRUE(player_->isPlaying());

    // Second play call (should succeed without issues)
    EXPECT_TRUE(player_->play());
    EXPECT_TRUE(player_->isPlaying());

    player_->stop();
}

TEST_F(AudioPlayerTest, StopWhenNotPlayingTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_short.wav"));

    // Stop without playing (should not cause issues)
    player_->stop();
    EXPECT_FALSE(player_->isPlaying());

    // Multiple stops should be safe
    player_->stop();
    player_->stop();
    EXPECT_FALSE(player_->isPlaying());
}

TEST_F(AudioPlayerTest, PlaybackCompletionTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_short.wav"));

    EXPECT_TRUE(player_->play());
    EXPECT_TRUE(player_->isPlaying());

    // Wait for playback to complete (with some margin)
    std::this_thread::sleep_for(1500ms);

    // Should have stopped automatically
    EXPECT_FALSE(player_->isPlaying());

    // Position should be near the end
    double position = player_->getCurrentPosition();
    double duration = player_->getDuration();
    EXPECT_GT(position, duration * 0.8);  // At least 80% through
}

// Volume control tests
TEST_F(AudioPlayerTest, VolumeControlTest) {
    // Test volume range
    EXPECT_FLOAT_EQ(player_->getVolume(), 1.0f);

    // Set various volume levels
    player_->setVolume(0.5f);
    EXPECT_FLOAT_EQ(player_->getVolume(), 0.5f);

    player_->setVolume(0.0f);
    EXPECT_FLOAT_EQ(player_->getVolume(), 0.0f);

    player_->setVolume(1.0f);
    EXPECT_FLOAT_EQ(player_->getVolume(), 1.0f);
}

TEST_F(AudioPlayerTest, VolumeClampingTest) {
    // Test volume clamping
    player_->setVolume(-0.5f);
    EXPECT_GE(player_->getVolume(), 0.0f);  // Should be clamped to 0 or higher

    player_->setVolume(2.0f);
    EXPECT_LE(player_->getVolume(), 1.0f);  // Should be clampled to 1 or handle gracefully
}

TEST_F(AudioPlayerTest, VolumeChangeDuringPlaybackTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_long.wav"));

    EXPECT_TRUE(player_->play());
    EXPECT_TRUE(player_->isPlaying());

    // Change volume during playback
    player_->setVolume(0.3f);
    EXPECT_FLOAT_EQ(player_->getVolume(), 0.3f);

    std::this_thread::sleep_for(200ms);

    player_->setVolume(0.8f);
    EXPECT_FLOAT_EQ(player_->getVolume(), 0.8f);

    player_->stop();
}

// Position tracking tests
TEST_F(AudioPlayerTest, PositionTrackingTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_long.wav"));

    // Initial position should be 0
    EXPECT_DOUBLE_EQ(player_->getCurrentPosition(), 0.0);

    EXPECT_TRUE(player_->play());

    // Track position over time
    std::this_thread::sleep_for(500ms);
    double pos1 = player_->getCurrentPosition();
    EXPECT_GT(pos1, 0.4);
    EXPECT_LT(pos1, 0.6);

    std::this_thread::sleep_for(500ms);
    double pos2 = player_->getCurrentPosition();
    EXPECT_GT(pos2, pos1);
    EXPECT_LT(pos2, 1.1);

    player_->stop();
}

TEST_F(AudioPlayerTest, SeekTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_long.wav"));

    double duration = player_->getDuration();
    ASSERT_GT(duration, 2.0);

    // Seek to middle of file
    double seekPosition = duration * 0.5;
    bool seekResult = player_->seek(seekPosition);

    if (seekResult) {  // Only test if seeking is supported
        double currentPos = player_->getCurrentPosition();
        EXPECT_NEAR(currentPos, seekPosition, 0.1);

        // Test seeking during playback
        EXPECT_TRUE(player_->play());
        std::this_thread::sleep_for(100ms);

        seekPosition = duration * 0.8;
        seekResult = player_->seek(seekPosition);
        if (seekResult) {
            currentPos = player_->getCurrentPosition();
            EXPECT_NEAR(currentPos, seekPosition, 0.1);
        }

        player_->stop();
    }
}

TEST_F(AudioPlayerTest, SeekBoundsTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_short.wav"));

    double duration = player_->getDuration();

    // Seek before beginning
    bool result = player_->seek(-1.0);
    if (result) {
        EXPECT_GE(player_->getCurrentPosition(), 0.0);
    }

    // Seek past end
    result = player_->seek(duration + 1.0);
    if (result) {
        EXPECT_LE(player_->getCurrentPosition(), duration);
    }
}

// Error handling and edge cases
TEST_F(AudioPlayerTest, LoadWhilePlayingTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_long.wav"));

    EXPECT_TRUE(player_->play());
    EXPECT_TRUE(player_->isPlaying());

    // Load a different file while playing
    bool loadResult = player_->loadFile("test_audio_short.wav");

    if (loadResult) {
        // Should have stopped previous playback
        // Note: Implementation may vary - might stop automatically or continue
        double newDuration = player_->getDuration();
        EXPECT_GT(newDuration, 0.9);
        EXPECT_LT(newDuration, 1.1);
    }
}

TEST_F(AudioPlayerTest, ThreadSafetyTest) {
    ASSERT_TRUE(player_->loadFile("test_audio_long.wav"));

    std::atomic<bool> keepRunning{true};
    std::atomic<int> errorCount{0};

    // Thread 1: Control playback
    std::thread controlThread([&]() {
        for (int i = 0; i < 10 && keepRunning; ++i) {
            if (!player_->play()) {
                errorCount++;
            }
            std::this_thread::sleep_for(50ms);
            player_->stop();
            std::this_thread::sleep_for(10ms);
        }
    });

    // Thread 2: Change volume
    std::thread volumeThread([&]() {
        for (int i = 0; i < 20 && keepRunning; ++i) {
            float volume = static_cast<float>(i % 10) / 10.0f;
            player_->setVolume(volume);
            std::this_thread::sleep_for(25ms);
        }
    });

    // Thread 3: Query state
    std::thread queryThread([&]() {
        for (int i = 0; i < 50 && keepRunning; ++i) {
            volatile bool playing = player_->isPlaying();
            volatile double position = player_->getCurrentPosition();
            volatile double duration = player_->getDuration();
            volatile float volume = player_->getVolume();
            (void)playing;
            (void)position;
            (void)duration;
            (void)volume;  // Suppress warnings
            std::this_thread::sleep_for(10ms);
        }
    });

    // Let threads run
    std::this_thread::sleep_for(2000ms);
    keepRunning = false;

    // Wait for threads to complete
    controlThread.join();
    volumeThread.join();
    queryThread.join();

    // Should not have many errors (some are acceptable under stress)
    EXPECT_LT(errorCount.load(), 5);

    // Clean up
    player_->stop();
}

// Performance tests
TEST_F(AudioPlayerTest, MultipleInstancesTest) {
    // Test creating multiple player instances
    std::vector<std::unique_ptr<AudioPlayer>> players;

    for (int i = 0; i < 5; ++i) {
        auto player = std::make_unique<AudioPlayer>();
        EXPECT_NE(player, nullptr);

        bool loadResult =
            player->loadFile(i % 2 == 0 ? "test_audio_short.wav" : "test_audio_long.wav");
        EXPECT_TRUE(loadResult);

        players.push_back(std::move(player));
    }

    // All players should be independent
    for (size_t i = 0; i < players.size(); ++i) {
        EXPECT_FALSE(players[i]->isPlaying());
        EXPECT_GT(players[i]->getDuration(), 0.0);
    }

    // Clean up (destructors should handle this)
    players.clear();
}

TEST_F(AudioPlayerTest, RapidLoadUnloadTest) {
    // Test rapid loading and unloading
    for (int i = 0; i < 20; ++i) {
        bool result =
            player_->loadFile(i % 2 == 0 ? "test_audio_short.wav" : "test_audio_long.wav");
        EXPECT_TRUE(result);

        double duration = player_->getDuration();
        EXPECT_GT(duration, 0.0);
    }

    // Final state should be valid
    EXPECT_FALSE(player_->isPlaying());
    EXPECT_GT(player_->getDuration(), 0.0);
}

}  // namespace huntmaster
