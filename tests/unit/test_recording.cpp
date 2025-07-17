#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "huntmaster/core/HuntmasterAudioEngine.h"

// Add this line to use HuntmasterAudioEngine without the namespace prefix
using huntmaster::HuntmasterAudioEngine;

void printLevel(float level) {
    int bars = static_cast<int>(level * 50);
    std::cout << "\rLevel: [";
    for (int j = 0; j < bars; ++j) std::cout << "=";
    for (int j = bars; j < 50; ++j) std::cout << " ";
    std::cout << "] " << std::fixed << std::setprecision(2) << level;
    std::cout.flush();
}

int main() {
    std::cout << "=== Huntmaster Recording Test ===" << std::endl;

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Test 1: Simple Recording
    std::cout << "\nTest 1: Recording for 3 seconds..." << std::endl;
    std::cout << "Speak into your microphone!" << std::endl;

    int recordingId = engine.startRecording(44100.0);
    if (recordingId < 0) {
        std::cerr << "Failed to start recording!" << std::endl;
        return 1;
    }

    // Monitor levels for 3 seconds
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        float level = engine.getRecordingLevel();
        printLevel(level);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;

    engine.stopRecording(recordingId);

    // Save recording
    std::string savedPath = engine.saveRecording(recordingId, "test_grunt").value;
    if (!savedPath.empty()) {
        std::cout << "Recording saved to: " << savedPath << std::endl;
    }

    // Test 2: Load and play master call
    std::cout << "\nTest 2: Loading and playing master call..." << std::endl;
    auto loadResult = engine.loadMasterCall("buck_grunt");
    auto playResult = engine.playMasterCall("buck_grunt");
    (void)loadResult;  // Suppress unused variable warning
    (void)playResult;  // Suppress unused variable warning

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Test 3: Record user attempt
    std::cout << "\nTest 3: Now try to imitate the buck grunt!" << std::endl;
    std::cout << "Recording in 3... ";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "2... ";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "1... ";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "GO!" << std::endl;

    recordingId = engine.startRecording(44100.0);

    // Record for 3 seconds with level monitoring
    start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        float level = engine.getRecordingLevel();
        printLevel(level);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;

    engine.stopRecording(recordingId);
    savedPath = engine.saveRecording(recordingId, "user_buck_grunt_attempt").value;
    std::cout << "Your attempt saved to: " << savedPath << std::endl;

    // Play back the user recording
    std::cout << "\nPlaying back your recording..." << std::endl;
    auto playRecordingResult = engine.playRecording("user_buck_grunt_attempt.wav");
    (void)playRecordingResult;  // Suppress unused variable warning

    std::this_thread::sleep_for(std::chrono::seconds(3));

    engine.shutdown();
    std::cout << "\nAll tests completed!" << std::endl;

    return 0;
}
