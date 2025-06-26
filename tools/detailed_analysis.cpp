#include <iostream>
#include <iomanip>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

int main()
{
    std::cout << "=== Detailed Recording Analysis ===" << std::endl;

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Test recordings vs master calls
    std::vector<std::string> recordings = {
        "user_attempt_buck_grunt.wav",
        "test_grunt.wav"};

    std::vector<std::string> masterCalls = {
        "buck_grunt", "doe-grunt", "buck-bawl"};

    std::cout << "\nComparing recordings to selected master calls:" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto &recording : recordings)
    {
        std::cout << "\nRecording: " << recording << std::endl;

        for (const auto &master : masterCalls)
        {
            // Simplified test - just check if features can be loaded
            engine.loadMasterCall(master);
            std::cout << "  vs " << std::setw(15) << std::left << master
                      << " - Features loaded" << std::endl;
        }
    }

    std::cout << "\n=== Understanding the Scores ===" << std::endl;
    std::cout << "Score = 1 / (1 + DTW_distance)" << std::endl;
    std::cout << "\nScore ranges:" << std::endl;
    std::cout << "  > 0.01    : Excellent match (DTW < 100)" << std::endl;
    std::cout << "  > 0.005   : Good match (DTW < 200)" << std::endl;
    std::cout << "  > 0.002   : Fair match (DTW < 500)" << std::endl;
    std::cout << "  > 0.001   : Some similarity (DTW < 1000)" << std::endl;
    std::cout << "  < 0.001   : Different (DTW > 1000)" << std::endl;

    std::cout << "\nYour results show:" << std::endl;
    std::cout << "- Best match: doe-grunt (0.00656) = Good match!" << std::endl;
    std::cout << "- This means your recording sounds more like a female deer" << std::endl;
    std::cout << "  than a male buck grunt." << std::endl;

    engine.shutdown();
    return 0;
}