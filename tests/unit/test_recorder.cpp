#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "huntmaster/core/AudioRecorder.h"

using namespace huntmaster;

// Audio Recording tests converted to Google Test format
TEST(AudioRecorderTest, BasicRecording) {
    std::cout << "=== Audio Recording Test ===" << std::endl;

    AudioRecorder recorder;
    AudioRecorder::Config config;
    config.sampleRate = 44100;
    config.channels = 1;

    std::cout << "Starting recording for 3 seconds..." << std::endl;
    if (!recorder.startRecording(config)) {
        std::cerr << "Failed to start recording!" << std::endl;
        FAIL() << "Failed to start recording!";
    }

    // Record for 3 seconds, showing levels
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        float level = recorder.getCurrentLevel();
        int bars = static_cast<int>(level * 50);
        std::cout << "\rLevel: [";
        for (int j = 0; j < bars; ++j) std::cout << "=";
        for (int j = bars; j < 50; ++j) std::cout << " ";
        std::cout << "] " << level;
        std::cout.flush();
    }
    std::cout << std::endl;

    recorder.stopRecording();

    std::cout << "Saving to test_recording.wav..." << std::endl;
    if (recorder.saveToWav("test_recording.wav")) {
        std::cout << "Recording saved successfully!" << std::endl;
        std::cout << "Duration: " << recorder.getDuration() << " seconds" << std::endl;
        EXPECT_TRUE(true);  // Recording successful
    } else {
        std::cerr << "Failed to save recording!" << std::endl;
        EXPECT_TRUE(false);  // Recording failed
    }
}
