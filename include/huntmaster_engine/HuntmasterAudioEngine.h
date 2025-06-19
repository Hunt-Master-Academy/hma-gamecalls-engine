#ifndef HUNTMASTER_AUDIO_ENGINE_H
#define HUNTMASTER_AUDIO_ENGINE_H

#include <string>
#include <vector>

class HuntmasterAudioEngine {
public:
    // Returns the singleton instance of the engine.
    static HuntmasterAudioEngine& getInstance();

    // Initializes the engine and its dependencies.
    void initialize();

    // Loads a master call's pre-computed MFCC features into memory.
    void loadMasterCall(const std::string& masterCallId);

    // Starts a new analysis session for the currently loaded master call.
    int startRealtimeSession(float sampleRate, int bufferSize);

    // Processes a chunk of live audio from the microphone.
    void processAudioChunk(int sessionId, const float* audioBuffer, int bufferSize);

    // Retrieves the current similarity score for the session.
    float getSimilarityScore(int sessionId);

    // Ends the specified practice session and clears its state.
    void endRealtimeSession(int sessionId);

    // Releases all resources used by the engine.
    void shutdown();

private:
    // Private constructor and destructor for singleton pattern.
    HuntmasterAudioEngine();
    ~HuntmasterAudioEngine();

    // Deleted copy constructor and assignment operator to prevent copying.
    HuntmasterAudioEngine(const HuntmasterAudioEngine&) = delete;
    void operator=(const HuntmasterAudioEngine&) = delete;
};

#endif // HUNTMASTER_AUDIO_ENGINE_H