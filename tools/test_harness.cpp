#include <iostream>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

// The main entry point for the TestHarness executable.
int main()
{
    std::cout << "--- Huntmaster Engine Test Harness ---" << std::endl;
    std::cout << "--- Testing Master Call Pipeline ---" << std::endl;

    // Get the engine instance and initialize it.
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // ----------------------------------------------------------------------
    // Test Run 1: Load the master call.
    // This should process the .mp3 file and save the features to a .mfc file.
    // ----------------------------------------------------------------------
    std::cout << "\n[RUN 1] Loading 'buck_grunt_master' for the first time..." << std::endl;
    engine.loadMasterCall("buck_grunt_master");

    // ----------------------------------------------------------------------
    // Test Run 2: Load the same master call again.
    // This time, it should load directly from the cached .mfc file.
    // ----------------------------------------------------------------------
    std::cout << "\n[RUN 2] Loading 'buck_grunt_master' for the second time..." << std::endl;
    engine.loadMasterCall("buck_grunt_master");

    // Simulate a practice session with the loaded call.
    std::cout << "\n--- Simulating a Practice Session ---" << std::endl;
    int sessionId = engine.startRealtimeSession(44100.0f, 1024);

    // In a real scenario, we would feed audio chunks here.
    // For now, we just get a dummy score.
    float score = engine.getSimilarityScore(sessionId);

    std::cout << "Final Score: " << score << std::endl;

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    std::cout << "\n--- Test Harness Finished ---" << std::endl;

    return 0;
}
