#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "huntmaster/core/UnifiedAudioEngine.h"

// Add this line to use HuntmasterAudioEngine without the namespace prefix
using huntmaster::UnifiedAudioEngine;

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

    // Create engine instance
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine instance!" << std::endl;
        return 1;
    }
    auto engine = std::move(engineResult.value);

    // Test 1: Simple Recording
    std::cout << "\nTest 1: Recording for 3 seconds..." << std::endl;
    std::cout << "Speak into your microphone!" << std::endl;

    // Create session for recording
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session for recording!" << std::endl;
        return 1;
    }
    int sessionId = sessionResult.value;

    auto recStatus = engine->startRecording(sessionId);
    if (recStatus != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to start recording!" << std::endl;
        engine->destroySession(sessionId);
        return 1;
    }

    // Monitor levels for 3 seconds
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        // UnifiedAudioEngine does not expose getRecordingLevel directly; skip or implement if
        // available
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;

    engine->stopRecording(sessionId);

    // Save recording
    auto saveResult = engine->saveRecording(sessionId, "test_grunt");
    std::string savedPath = saveResult.isOk() ? saveResult.value : "";
    if (!savedPath.empty()) {
        std::cout << "Recording saved to: " << savedPath << std::endl;
    }

    // Test 2: Load and play master call
    std::cout << "\nTest 2: Loading and playing master call..." << std::endl;

    auto masterStatus = engine->loadMasterCall(sessionId, "buck_grunt");
    (void)masterStatus;
    // UnifiedAudioEngine does not expose playMasterCall; skip or implement if available

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

    auto recStatus2 = engine->startRecording(sessionId);
    if (recStatus2 != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Failed to start recording!" << std::endl;
        engine->destroySession(sessionId);
        return 1;
    }

    // Record for 3 seconds
    start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;

    engine->stopRecording(sessionId);
    auto saveResult2 = engine->saveRecording(sessionId, "user_buck_grunt_attempt");
    savedPath = saveResult2.isOk() ? saveResult2.value : "";
    std::cout << "Your attempt saved to: " << savedPath << std::endl;

    // UnifiedAudioEngine does not expose playRecording; skip or implement if available

    std::this_thread::sleep_for(std::chrono::seconds(3));

    engine->destroySession(sessionId);
    std::cout << "\nAll tests completed!" << std::endl;

    return 0;
}
