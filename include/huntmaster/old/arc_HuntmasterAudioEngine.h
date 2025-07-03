#ifndef HUNTMASTER_AUDIO_ENGINE_H
#define HUNTMASTER_AUDIO_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex> // Add this!

// Forward declare internal components to keep header clean.
namespace huntmaster
{
    class MFCCProcessor;
    class DTWProcessor;
    class AudioRecorder;
    class AudioPlayer;
}

// Internal struct to manage a recording session's state.
struct RecordingSession
{
    std::unique_ptr<huntmaster::AudioRecorder> recorder;
    int id;
    double sampleRate;
};

// Encapsulates all state for a single real-time analysis session.
struct RealtimeSessionState {
    std::deque<float> audioBuffer;
    std::deque<float> current_vad_segment_buffer;
    std::vector<std::vector<float>> features;
    bool is_in_sound_segment = false;
    int consecutive_sound_frames = 0;
    int consecutive_silence_frames = 0;
    int mfcc_processed_frames_in_current_vad_segment = 0;
};

class HuntmasterAudioEngine
{
public:
    // --- Singleton Access ---
    static HuntmasterAudioEngine &getInstance();

    // --- Core Lifecycle ---
    void initialize();
    void shutdown();

    // --- Analysis and Scoring ---
    void loadMasterCall(const std::string &masterCallId);
    int startRealtimeSession(float sampleRate, int bufferSize);
    void processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize);
    float getSimilarityScore(int sessionId);
    void endRealtimeSession(int sessionId);

    // --- Recording ---
    int startRecording(double sampleRate = 44100.0);
    void stopRecording(int recordingId);
    std::string saveRecording(int recordingId, const std::string &filename);
    bool isRecording() const;
    float getRecordingLevel() const; // 0.0 to 1.0
    double getRecordingDuration(int recordingId) const;

    // --- Playback ---
    void playMasterCall(const std::string &callId);
    void playRecording(const std::string &filename);
    void stopPlayback();

private:
    // --- Private Lifecycle & Helpers ---
    HuntmasterAudioEngine();
    ~HuntmasterAudioEngine();
    HuntmasterAudioEngine(const HuntmasterAudioEngine &) = delete;
    void operator=(const HuntmasterAudioEngine &) = delete;
    void saveFeaturesToFile(const std::string &masterCallId);
    bool loadFeaturesFromFile(const std::string &masterCallId);

    // --- Member Variables ---
    std::unique_ptr<huntmaster::MFCCProcessor> mfccProcessor;
    std::unique_ptr<huntmaster::AudioPlayer> audioPlayer;

    // Path configuration
    std::string masterCallsPath_;
    std::string featuresPath_;
    std::string recordingsPath_;

    // Master Call Data
    std::vector<std::vector<float>> masterCallFeatures;

    // --- Thread-Safe Session Management ---
    mutable std::mutex recordingsMutex_;
    std::unordered_map<int, RecordingSession> activeRecordings;
    std::atomic<int> nextRecordingId{1};

    mutable std::mutex realtimeSessionsMutex_;
    std::unordered_map<int, RealtimeSessionState> activeRealtimeSessions_;
    std::atomic<int> nextRealtimeSessionId_{1};

    // VAD Configuration
    struct RealtimeVADConfig
    {
        float silenceThreshold = 0.01f;
        float energyThreshold = 0.0001f;
        float windowDurationMs = 10.0f;
        float minSoundDurationMs = 100.0f;
        float minSilenceDurationMs = 50.0f;
        float hangoverDurationMs = 100.0f;
    } vad_config;

    // Helper method - changed to work with deque
    float calculateEnergy(const std::deque<float> &samples, size_t start, size_t windowSize);
};

#endif // HUNTMASTER_AUDIO_ENGINE_H