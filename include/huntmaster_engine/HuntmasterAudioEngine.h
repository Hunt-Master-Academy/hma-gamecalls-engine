#ifndef HUNTMASTER_AUDIO_ENGINE_H
#define HUNTMASTER_AUDIO_ENGINE_H

#include <string>
#include <vector>
#include <memory>

// Forward declare MFCCProcessor and DTWProcessor to keep header clean.
namespace huntmaster
{
    class MFCCProcessor;
    class DTWProcessor;
}

class HuntmasterAudioEngine
{
public:
    static HuntmasterAudioEngine &getInstance();

    void initialize();
    void loadMasterCall(const std::string &masterCallId);
    int startRealtimeSession(float sampleRate, int bufferSize);
    void processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize);
    float getSimilarityScore(int sessionId);
    void endRealtimeSession(int sessionId);
    void shutdown();

private:
    HuntmasterAudioEngine();
    ~HuntmasterAudioEngine();
    HuntmasterAudioEngine(const HuntmasterAudioEngine &) = delete;
    void operator=(const HuntmasterAudioEngine &) = delete;

    // --- Private Helper Methods ---
    void saveFeaturesToFile(const std::string &masterCallId);
    bool loadFeaturesFromFile(const std::string &masterCallId);

    // --- Private Member Variables ---
    std::unique_ptr<huntmaster::MFCCProcessor> mfccProcessor;

    // --- Master Call Data ---
    std::vector<std::vector<float>> masterCallFeatures;

    // --- Real-time Session Data ---
    std::vector<float> sessionAudioBuffer;
    std::vector<std::vector<float>> sessionFeatures;
};

#endif // HUNTMASTER_AUDIO_ENGINE_H