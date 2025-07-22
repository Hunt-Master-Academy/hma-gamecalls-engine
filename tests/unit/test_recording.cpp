#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <span>
#include <thread>

#include "huntmaster/core/UnifiedAudioEngine.h"

// Use the huntmaster namespace
using namespace huntmaster;

void printLevel(float level) {
    int bars = static_cast<int>(level * 50);
    std::cout << "\rLevel: [";
    for (int j = 0; j < bars; ++j)
        std::cout << "=";
    for (int j = bars; j < 50; ++j)
        std::cout << " ";
    std::cout << "] " << std::fixed << std::setprecision(2) << level;
    std::cout.flush();
}

int main() {
    std::cout << "=== Huntmaster Recording Test ===" << std::endl;

    // Create UnifiedAudioEngine instance
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "Failed to create UnifiedAudioEngine!" << std::endl;
        return -1;
    }
    auto engine = std::move(*engineResult);

    // Create a session
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "Failed to create session!" << std::endl;
        return -1;
    }
    SessionId sessionId = *sessionResult;

    // Test 1: Recording Functionality Test
    std::cout << "\nTest 1: Testing recording capabilities..." << std::endl;

    // Load a master call for comparison
    auto loadResult = engine->loadMasterCall(sessionId, "buck_grunt");
    if (loadResult != UnifiedAudioEngine::Status::OK) {
        std::cout << "Warning: buck_grunt master call not available" << std::endl;
    } else {
        std::cout << "Successfully loaded buck_grunt master call" << std::endl;
    }

    // Test recording functionality
    std::cout << "Starting recording test..." << std::endl;
    auto recordResult = engine->startRecording(sessionId);
    if (recordResult == UnifiedAudioEngine::Status::OK) {
        std::cout << "✓ Recording started successfully" << std::endl;

        // Check if recording status is correct
        if (engine->isRecording(sessionId)) {
            std::cout << "✓ Recording status confirmed" << std::endl;
        } else {
            std::cout << "✗ Recording status check failed" << std::endl;
        }

        // Simulate some recording time with level monitoring
        std::cout << "Recording for 2 seconds with level monitoring..." << std::endl;
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Get current recording level
            auto levelResult = engine->getRecordingLevel(sessionId);
            if (levelResult.isOk()) {
                printLevel(*levelResult);
            }
        }
        std::cout << std::endl;

        // Stop recording
        auto stopResult = engine->stopRecording(sessionId);
        if (stopResult == UnifiedAudioEngine::Status::OK) {
            std::cout << "✓ Recording stopped successfully" << std::endl;

            // Check recording duration
            auto durationResult = engine->getRecordingDuration(sessionId);
            if (durationResult.isOk()) {
                std::cout << "✓ Recording duration: " << *durationResult << " seconds" << std::endl;
            }

            // Save the recording
            auto saveResult = engine->saveRecording(sessionId, "test_recording.wav");
            if (saveResult.isOk()) {
                std::cout << "✓ Recording saved to: " << *saveResult << std::endl;
            } else {
                std::cout << "✗ Failed to save recording" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to stop recording" << std::endl;
        }
    } else {
        std::cout << "✗ Failed to start recording" << std::endl;
    }

    // Test 2: Audio Processing and Similarity Scoring
    std::cout << "\nTest 2: Testing audio processing..." << std::endl;

    // Test processing some dummy audio data
    std::vector<float> testAudio(4410);  // 0.1 seconds of 44.1kHz audio
    for (size_t i = 0; i < testAudio.size(); ++i) {
        testAudio[i] = 0.5f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);  // 440Hz sine wave
    }

    std::span<const float> audioSpan(testAudio.data(), testAudio.size());
    auto processResult = engine->processAudioChunk(sessionId, audioSpan);
    if (processResult == UnifiedAudioEngine::Status::OK) {
        std::cout << "✓ Successfully processed test audio chunk" << std::endl;
    } else {
        std::cout << "✗ Failed to process test audio chunk" << std::endl;
    }

    // Try to get a similarity score
    auto scoreResult = engine->getSimilarityScore(sessionId);
    if (scoreResult.isOk()) {
        float score = *scoreResult;
        std::cout << "✓ Got similarity score: " << score << std::endl;
    } else {
        std::cout << "✗ Could not get similarity score" << std::endl;
    }

    // Test playback functionality if we have a saved recording
    std::cout << "\nTest 3: Testing playback capabilities..." << std::endl;

    // Try to play back the recording we just made
    auto playResult = engine->playRecording(sessionId, "test_recording.wav");
    if (playResult == UnifiedAudioEngine::Status::OK) {
        std::cout << "✓ Started playback of recorded audio" << std::endl;

        // Monitor playback for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (engine->isPlaying(sessionId)) {
            std::cout << "✓ Playback status confirmed" << std::endl;

            auto positionResult = engine->getPlaybackPosition(sessionId);
            if (positionResult.isOk()) {
                std::cout << "✓ Playback position: " << *positionResult << " seconds" << std::endl;
            }
        }

        // Stop playback
        auto stopPlayResult = engine->stopPlayback(sessionId);
        if (stopPlayResult == UnifiedAudioEngine::Status::OK) {
            std::cout << "✓ Playback stopped successfully" << std::endl;
        }
    } else {
        std::cout << "Note: Could not play back recording (file may not exist)" << std::endl;
    }

    // Clean up
    auto resetResult = engine->resetSession(sessionId);
    if (resetResult != UnifiedAudioEngine::Status::OK) {
        std::cerr << "Warning: Failed to reset session!" << std::endl;
    }

    std::cout << "\nRecording and playback tests completed!" << std::endl;

    return 0;
}
