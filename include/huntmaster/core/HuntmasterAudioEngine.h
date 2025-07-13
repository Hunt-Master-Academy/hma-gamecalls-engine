// HuntmasterAudioEngine.h
#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace huntmaster {

class HuntmasterAudioEngine {
   public:
    enum class EngineStatus {
        OK = 0,
        INVALID_PARAMS = -1,
        INVALID_SESSION = -2,
        INVALID_RECORDING_ID = -3,
        FILE_NOT_FOUND = -4,
        FILE_WRITE_ERROR = -5,
        PROCESSING_ERROR = -11,
        BUFFER_OVERFLOW = -6,
        INSUFFICIENT_DATA = -7,
        RECORDER_INIT_FAILED = -8,
        PLAYER_NOT_INITIALIZED = -9,
        OUT_OF_MEMORY = -10
    };

    template <typename T>
    struct [[nodiscard]] Result {
        T value;
        EngineStatus status;

        bool isOk() const { return status == EngineStatus::OK; }
        operator bool() const { return isOk(); }
    };

    static HuntmasterAudioEngine &getInstance();

    void initialize();
    void shutdown();

    // Master call management
    [[nodiscard]] EngineStatus loadMasterCall(std::string_view masterCallId);

    // Real-time session management
    [[nodiscard]] Result<int> startRealtimeSession(float sampleRate, int bufferSize);
    [[nodiscard]] EngineStatus processAudioChunk(int sessionId, const float *audioBuffer,
                                                 int bufferSize);
    [[nodiscard]] Result<float> getSimilarityScore(int sessionId);
    void endRealtimeSession(int sessionId);
    [[nodiscard]] int getSessionFeatureCount(int sessionId) const;

    // Recording management
    [[nodiscard]] Result<int> startRecording(double sampleRate);
    void stopRecording(int recordingId);
    [[nodiscard]] Result<std::string> saveRecording(int recordingId, std::string_view filename);

    // Status queries
    [[nodiscard]] bool isRecording() const;                            // Added const
    [[nodiscard]] float getRecordingLevel() const;                     // Added const
    [[nodiscard]] double getRecordingDuration(int recordingId) const;  // Added const

    // Playback
    [[nodiscard]] EngineStatus playMasterCall(std::string_view callId);
    [[nodiscard]] EngineStatus playRecording(std::string_view filename);
    void stopPlayback();

   private:
    HuntmasterAudioEngine();
    ~HuntmasterAudioEngine();

    // Delete copy/move operations for singleton
    HuntmasterAudioEngine(const HuntmasterAudioEngine &) = delete;
    HuntmasterAudioEngine &operator=(const HuntmasterAudioEngine &) = delete;
    HuntmasterAudioEngine(HuntmasterAudioEngine &&) = delete;
    HuntmasterAudioEngine &operator=(HuntmasterAudioEngine &&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace huntmaster

// C API for WASM/FFI
extern "C" {
[[nodiscard]] int createEngine();
void destroyEngine(int engineId);
[[nodiscard]] int loadMasterCall(int engineId, const char *masterCallId);
[[nodiscard]] int startSession(int engineId);
[[nodiscard]] int processAudioChunk(int engineId, int sessionId, const float *audioBuffer,
                                    int bufferSize, float sampleRate);
[[nodiscard]] float getSimilarityScore(int engineId, int sessionId);
void endSession(int engineId, int sessionId);
[[nodiscard]] int getSessionFeatureCount(int engineId, int sessionId);
}