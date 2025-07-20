// New unified API design that fixes architectural inconsistencies
#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace huntmaster {

// Forward declarations for audio components
class AudioPlayer;
class AudioRecorder;
class AudioLevelProcessor;

using SessionId = uint32_t;
constexpr SessionId INVALID_SESSION_ID = 0;

class UnifiedAudioEngine {
   public:
    enum class Status {
        OK = 0,
        INVALID_PARAMS = -1,
        SESSION_NOT_FOUND = -2,
        FILE_NOT_FOUND = -3,
        PROCESSING_ERROR = -4,
        INSUFFICIENT_DATA = -5,
        OUT_OF_MEMORY = -6,
        INIT_FAILED = -7
    };

    template <typename T>
    struct [[nodiscard]] Result {
        T value{};
        Status status = Status::OK;

        bool isOk() const { return status == Status::OK; }
        operator bool() const { return isOk(); }
        Status error() const { return status; }
        const T& operator*() const { return value; }
        T& operator*() { return value; }
    };

    // Engine lifecycle - instance-based, not singleton
    [[nodiscard]] static Result<std::unique_ptr<UnifiedAudioEngine>> create();
    ~UnifiedAudioEngine();

    // Session management - all operations require valid sessionId
    [[nodiscard]] Result<SessionId> createSession(float sampleRate = 44100.0f);
    [[nodiscard]] Status destroySession(SessionId sessionId);
    [[nodiscard]] std::vector<SessionId> getActiveSessions() const;

    // Master call management - per session (this is the key improvement)
    [[nodiscard]] Status loadMasterCall(SessionId sessionId, std::string_view masterCallId);
    [[nodiscard]] Status unloadMasterCall(SessionId sessionId);
    [[nodiscard]] Result<std::string> getCurrentMasterCall(SessionId sessionId) const;

    // Audio processing - always requires sessionId
    [[nodiscard]] Status processAudioChunk(SessionId sessionId, std::span<const float> audioBuffer);
    [[nodiscard]] Result<float> getSimilarityScore(SessionId sessionId);
    [[nodiscard]] Result<int> getFeatureCount(SessionId sessionId) const;

    // Session state queries
    [[nodiscard]] bool isSessionActive(SessionId sessionId) const;
    [[nodiscard]] Result<float> getSessionDuration(SessionId sessionId) const;
    [[nodiscard]] Status resetSession(SessionId sessionId);

    // Recording (per session)
    [[nodiscard]] Status startRecording(SessionId sessionId);
    [[nodiscard]] Status stopRecording(SessionId sessionId);
    [[nodiscard]] Result<std::string> saveRecording(SessionId sessionId, std::string_view filename);
    [[nodiscard]] bool isRecording(SessionId sessionId) const;
    [[nodiscard]] Result<float> getRecordingLevel(SessionId sessionId) const;
    [[nodiscard]] Result<double> getRecordingDuration(SessionId sessionId) const;

    // Audio Playback (per session)
    [[nodiscard]] Status playMasterCall(SessionId sessionId, std::string_view masterCallId);
    [[nodiscard]] Status playRecording(SessionId sessionId, std::string_view filename);
    [[nodiscard]] Status stopPlayback(SessionId sessionId);
    [[nodiscard]] bool isPlaying(SessionId sessionId) const;
    [[nodiscard]] Result<double> getPlaybackPosition(SessionId sessionId) const;
    [[nodiscard]] Status setPlaybackVolume(SessionId sessionId, float volume);

    // Real-time Session Management (for interactive tools)
    [[nodiscard]] Result<SessionId> startRealtimeSession(float sampleRate = 44100.0f,
                                                         int bufferSize = 512);
    [[nodiscard]] Status endRealtimeSession(SessionId sessionId);
    [[nodiscard]] bool isRealtimeSession(SessionId sessionId) const;

   private:
    UnifiedAudioEngine();

    // Non-copyable, moveable
    UnifiedAudioEngine(const UnifiedAudioEngine&) = delete;
    UnifiedAudioEngine& operator=(const UnifiedAudioEngine&) = delete;
    UnifiedAudioEngine(UnifiedAudioEngine&&) = default;
    UnifiedAudioEngine& operator=(UnifiedAudioEngine&&) = default;

    struct SessionState {
        SessionId id;
        float sampleRate;
        std::chrono::steady_clock::time_point startTime;

        // Per-session master call
        std::vector<std::vector<float>> masterCallFeatures;
        std::string masterCallId;

        // Audio processing state
        std::vector<float> audioBuffer;
        std::vector<std::vector<float>> sessionFeatures;

        // Recording state
        bool isRecording = false;
        std::vector<float> recordingBuffer;

        // Playback state
        bool isPlaying = false;
        std::string currentPlaybackFile;
        float playbackVolume = 1.0f;

        // Real-time session properties
        bool isRealtimeSession = false;
        int realtimeBufferSize = 512;

        // Processing components (per-session for thread safety)
        std::unique_ptr<class MFCCProcessor> mfccProcessor;
        std::unique_ptr<class VoiceActivityDetector> vad;
        std::unique_ptr<class AudioPlayer> audioPlayer;
        std::unique_ptr<class AudioRecorder> audioRecorder;
        std::unique_ptr<class AudioLevelProcessor> levelProcessor;
    };

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

// Clean C API that matches the C++ design
extern "C" {
// Engine management
[[nodiscard]] int unified_create_engine();
void unified_destroy_engine(int engineId);

// Session management
[[nodiscard]] int unified_create_session(int engineId, float sampleRate);
[[nodiscard]] int unified_destroy_session(int engineId, int sessionId);

// Real-time session management
[[nodiscard]] int unified_start_realtime_session(int engineId, float sampleRate, int bufferSize);
[[nodiscard]] int unified_end_realtime_session(int engineId, int sessionId);

// Master calls - now per session
[[nodiscard]] int unified_load_master_call(int engineId, int sessionId, const char* masterCallId);

// Audio processing
[[nodiscard]] int unified_process_audio_chunk(int engineId, int sessionId, const float* audioBuffer,
                                              int bufferSize);
[[nodiscard]] float unified_get_similarity_score(int engineId, int sessionId);
[[nodiscard]] int unified_get_feature_count(int engineId, int sessionId);

// Recording
[[nodiscard]] int unified_start_recording(int engineId, int sessionId);
[[nodiscard]] int unified_stop_recording(int engineId, int sessionId);
[[nodiscard]] int unified_save_recording(int engineId, int sessionId, const char* filename);
[[nodiscard]] int unified_is_recording(int engineId, int sessionId);
[[nodiscard]] float unified_get_recording_level(int engineId, int sessionId);

// Playback
[[nodiscard]] int unified_play_master_call(int engineId, int sessionId, const char* masterCallId);
[[nodiscard]] int unified_play_recording(int engineId, int sessionId, const char* filename);
[[nodiscard]] int unified_stop_playback(int engineId, int sessionId);
[[nodiscard]] int unified_is_playing(int engineId, int sessionId);
}

}  // namespace huntmaster
