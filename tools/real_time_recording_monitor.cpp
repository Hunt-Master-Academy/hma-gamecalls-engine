#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <cmath>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

using namespace huntmaster;

void showRecordingLevels(int durationSeconds = 10)
{
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    std::cout << "=== Real-Time Recording Monitor ===" << std::endl;
    std::cout << "Recording for " << durationSeconds << " seconds..." << std::endl;
    std::cout << "Starting in 3..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "2..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "1..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "RECORDING!" << std::endl
              << std::endl;

    int recordingId = engine.startRecording(44100.0);

    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(durationSeconds);

    float peakLevel = 0.0f;
    float avgLevel = 0.0f;
    int samples = 0;

    while (std::chrono::steady_clock::now() < endTime)
    {
        float level = engine.getRecordingLevel();
        peakLevel = std::max(peakLevel, level);
        avgLevel += level;
        samples++;

        // Show level bar
        int barLength = static_cast<int>(level * 50);
        std::cout << "\r[";

        for (int i = 0; i < 50; ++i)
        {
            if (i < barLength)
            {
                if (level > 0.9f)
                    std::cout << "!"; // Clipping warning
                else if (level > 0.7f)
                    std::cout << "=";
                else
                    std::cout << "-";
            }
            else
            {
                std::cout << " ";
            }
        }

        std::cout << "] " << std::fixed << std::setprecision(1)
                  << (level * 100) << "%";

        // Show status
        if (level < 0.05f)
        {
            std::cout << " [Too Quiet]    ";
        }
        else if (level > 0.9f)
        {
            std::cout << " [CLIPPING!]    ";
        }
        else if (level > 0.7f)
        {
            std::cout << " [Loud]         ";
        }
        else
        {
            std::cout << " [Good]         ";
        }

        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << std::endl
              << std::endl;
    engine.stopRecording(recordingId);

    // Show summary
    avgLevel /= samples;
    std::cout << "Recording Summary:" << std::endl;
    std::cout << "  Peak Level: " << (peakLevel * 100) << "%" << std::endl;
    std::cout << "  Average Level: " << (avgLevel * 100) << "%" << std::endl;

    if (avgLevel < 0.1f)
    {
        std::cout << "  Warning: Recording may be too quiet!" << std::endl;
    }
    if (peakLevel > 0.95f)
    {
        std::cout << "  Warning: Recording may have clipping!" << std::endl;
    }

    std::cout << "\nSave recording as (without .wav): ";
    std::string filename;
    std::getline(std::cin, filename);

    auto saveResult = engine.saveRecording(recordingId, filename);
    if (!saveResult.isOk())
    {
        std::cerr << "Failed to save recording!" << std::endl;
        // Handle error...
    }
    else
    {
        std::string savedPath = saveResult.value;
        std::cout << "Recording saved to: " << savedPath << std::endl;
    }
    std::cout << "Saved to: " << savedPath << std::endl;

    engine.shutdown();
}

int main(int argc, char *argv[])
{
    int duration = 10;
    if (argc > 1)
    {
        duration = std::atoi(argv[1]);
    }

    showRecordingLevels(duration);
    return 0;
}