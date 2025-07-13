#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <string>
#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

void printLevel(float level)
{
    int bars = static_cast<int>(level * 50);
    std::cout << "\rLevel: [";
    for (int j = 0; j < bars; ++j)
        std::cout << "=";
    for (int j = bars; j < 50; ++j)
        std::cout << " ";
    std::cout << "] " << std::fixed << std::setprecision(2) << level;
    std::cout.flush();
}

void showMenu()
{
    std::cout << "\n=== Huntmaster Interactive Recorder ===" << std::endl;
    std::cout << "1. Record audio (with live monitoring)" << std::endl;
    std::cout << "2. Play last recording" << std::endl;
    std::cout << "3. Load and play master call" << std::endl;
    std::cout << "4. Record and compare to master" << std::endl;
    std::cout << "5. Exit" << std::endl;
    std::cout << "Choice: ";
}

int main()
{
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    int lastRecordingId = -1;
    std::string lastRecordingFile;
    bool running = true;

    while (running)
    {
        showMenu();

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline

        switch (choice)
        {
        case 1:
        {
            std::cout << "\nHow many seconds to record? ";
            int seconds;
            std::cin >> seconds;
            std::cin.ignore();

            std::cout << "Starting recording in 3... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "2... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "1... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "GO!" << std::endl;

            lastRecordingId = engine.startRecording(44100.0);

            // Monitor levels
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(seconds))
            {
                float level = engine.getRecordingLevel();
                printLevel(level);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << std::endl;

            engine.stopRecording(lastRecordingId);

            std::cout << "Enter filename (without .wav): ";
            std::string filename;
            std::getline(std::cin, filename);

            lastRecordingFile = engine.saveRecording(lastRecordingId, filename);
            std::cout << "Saved to: " << lastRecordingFile << std::endl;
            break;
        }

        case 2:
        {
            if (lastRecordingFile.empty())
            {
                std::cout << "No recording available!" << std::endl;
            }
            else
            {
                std::cout << "Playing: " << lastRecordingFile << std::endl;
                engine.playRecording(lastRecordingFile);

                // Wait for playback to finish (simple approach)
                std::this_thread::sleep_for(std::chrono::seconds(5));
                engine.stopPlayback();
            }
            break;
        }

        case 3:
        {
            std::cout << "Enter master call ID (e.g., 'buck_grunt'): ";
            std::string callId;
            std::getline(std::cin, callId);

            engine.loadMasterCall(callId);
            engine.playMasterCall(callId);

            std::cout << "Playing master call..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            break;
        }

        case 4:
        {
            std::cout << "Enter master call ID to compare against: ";
            std::string callId;
            std::getline(std::cin, callId);

            engine.loadMasterCall(callId);

            // Play master first
            std::cout << "\nPlaying master call..." << std::endl;
            engine.playMasterCall(callId);
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Record user attempt
            std::cout << "\nNow imitate the call! Recording in 3... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "2... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "1... ";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "GO!" << std::endl;

            int sessionId = engine.startRealtimeSession(44100.0, 1024);
            lastRecordingId = engine.startRecording(44100.0);

            // Record for 3 seconds with real-time analysis
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3))
            {
                float level = engine.getRecordingLevel();
                printLevel(level);

                // In a real implementation, you'd feed audio chunks here
                // float score = engine.getSimilarityScore(sessionId);

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << std::endl;

            engine.stopRecording(lastRecordingId);
            engine.endRealtimeSession(sessionId);

            lastRecordingFile = engine.saveRecording(lastRecordingId, "comparison_attempt");
            std::cout << "Recording saved. Analysis complete!" << std::endl;

            // TODO: Display similarity score when real-time analysis is fully implemented
            break;
        }

        case 5:
            running = false;
            break;

        default:
            std::cout << "Invalid choice!" << std::endl;
        }
    }

    engine.shutdown();
    return 0;
}