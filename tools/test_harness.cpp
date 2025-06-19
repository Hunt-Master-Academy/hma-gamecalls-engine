#include <iostream>
#include "huntmaster_engine/HuntmasterAudioEngine.h"

// The main entry point for the TestHarness executable.
int main() {
    std::cout << "--- Huntmaster Engine Test Harness ---" << std::endl;

    // Get the engine instance and initialize it.
    HuntmasterAudioEngine& engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Simulate a practice session.
    engine.loadMasterCall("deer_grunt_01");
    int sessionId = engine.startRealtimeSession(44100.0f, 1024);
    
    // In a real scenario, we would feed audio chunks here.
    // For now, we just get a dummy score.
    float score = engine.getSimilarityScore(sessionId);

    std::cout << "Final Score: " << score << std::endl;

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    std::cout << "--- Test Harness Finished ---" << std::endl;

    return 0;
}
