#ifndef HUNTMASTER_AUDIO_ENGINE_H
#define HUNTMASTER_AUDIO_ENGINE_H

#include <string>
#include <vector>
#include <memory> // Required for std::unique_ptr

// Forward declare MFCCProcessor to avoid including the full header here.
// This is good practice for keeping header dependencies clean.
namespace huntmaster
{
    class MFCCProcessor;
}

class HuntmasterAudioEngine
{
public:
    // Returns the singleton instance of the engine.
    static HuntmasterAudioEngine &getInstance();

    // Initializes the engine and its dependencies.
    void initialize();

    // Loads a master call's MFCC features.
    // It will first attempt to load pre-computed features from a .mfc file.
    // If not found, it will load the source audio, process it, and save the
    // features to a new .mfc file.
    void loadMasterCall(const std::string &masterCallId);

    // Starts a new analysis session for the currently loaded master call.
    int startRealtimeSession(float sampleRate, int bufferSize);

    // Processes a chunk of live audio from the microphone.
    void processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize);

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
    HuntmasterAudioEngine(const HuntmasterAudioEngine &) = delete;
    void operator=(const HuntmasterAudioEngine &) = delete;

    // --- Private Helper Methods ---

    // Saves the computed MFCC features to a binary file for faster loading next time.
    void saveFeaturesToFile(const std::string &masterCallId);

    // Attempts to load pre-computed MFCC features from a binary file.
    // Returns true on success, false on failure (e.g., file not found).
    bool loadFeaturesFromFile(const std::string &masterCallId);

    // --- Private Member Variables ---

    // A unique pointer to our MFCC processor instance.
    // Using a unique_ptr manages the memory for us automatically.
    std::unique_ptr<huntmaster::MFCCProcessor> mfccProcessor;

    // A vector to store the computed MFCC frames of the master call.
    // This will hold the sequence of feature vectors that represent the master audio.
    std::vector<std::vector<float>> masterCallFeatures;
};

#endif // HUNTMASTER_AUDIO_ENGINE_H
