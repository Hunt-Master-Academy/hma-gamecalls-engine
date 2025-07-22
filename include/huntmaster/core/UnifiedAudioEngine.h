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
class RealtimeScorer;

using SessionId = uint32_t;
constexpr SessionId INVALID_SESSION_ID = 0;

// Configuration for RealtimeScorer integration
struct RealtimeScorerConfig {
    float mfccWeight = 0.5f;           ///< Weight for MFCC similarity (0.0-1.0)
    float volumeWeight = 0.2f;         ///< Weight for volume matching (0.0-1.0)
    float timingWeight = 0.2f;         ///< Weight for timing accuracy (0.0-1.0)
    float pitchWeight = 0.1f;          ///< Weight for pitch similarity (0.0-1.0)
    float confidenceThreshold = 0.7f;  ///< Minimum confidence for reliable score
    float minScoreForMatch = 0.005f;   ///< Minimum similarity score for match
    bool enablePitchAnalysis = false;  ///< Enable pitch-based scoring
    size_t scoringHistorySize = 50;    ///< Number of historical scores to retain
};

// Detailed similarity score breakdown from RealtimeScorer
struct RealtimeScoringResult {
    float overall = 0.0f;                             ///< Overall weighted similarity score
    float mfcc = 0.0f;                                ///< MFCC pattern similarity
    float volume = 0.0f;                              ///< Volume level matching
    float timing = 0.0f;                              ///< Timing/rhythm accuracy
    float pitch = 0.0f;                               ///< Pitch similarity
    float confidence = 0.0f;                          ///< Confidence in score (0.0-1.0)
    bool isReliable = false;                          ///< Whether score meets confidence threshold
    bool isMatch = false;                             ///< Whether score indicates a match
    size_t samplesAnalyzed = 0;                       ///< Number of samples analyzed
    std::chrono::steady_clock::time_point timestamp;  ///< Score timestamp
};

// Real-time feedback for user guidance
struct RealtimeFeedback {
    RealtimeScoringResult currentScore;   ///< Current similarity score
    RealtimeScoringResult trendingScore;  ///< Trending average over recent history
    RealtimeScoringResult peakScore;      ///< Best score achieved so far
    float progressRatio = 0.0f;           ///< Progress through master call (0.0-1.0)
    std::string qualityAssessment;        ///< Text description of match quality
    std::string recommendation;           ///< Suggestion for improvement
    bool isImproving = false;             ///< Whether score is trending upward
};

// Voice Activity Detection Configuration
struct VADConfig {
    float energy_threshold = 0.01f;   ///< Energy threshold for voice detection
    float window_duration = 0.025f;   ///< Analysis window duration in seconds
    float min_sound_duration = 0.1f;  ///< Minimum duration for valid voice activity
    float pre_buffer = 0.1f;          ///< Pre-buffer duration for voice start
    float post_buffer = 0.2f;         ///< Post-buffer duration for voice end
    bool enabled = true;              ///< Whether VAD is enabled
};

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

    // Real-time scoring features using RealtimeScorer toolset
    [[nodiscard]] Status setRealtimeScorerConfig(SessionId sessionId,
                                                 const struct RealtimeScorerConfig& config);
    [[nodiscard]] Result<struct RealtimeScoringResult> getDetailedScore(SessionId sessionId);
    [[nodiscard]] Result<struct RealtimeFeedback> getRealtimeFeedback(SessionId sessionId);
    [[nodiscard]] Result<std::string> exportScoreToJson(SessionId sessionId);
    [[nodiscard]] Result<std::string> exportFeedbackToJson(SessionId sessionId);
    [[nodiscard]] Result<std::string> exportScoringHistoryToJson(SessionId sessionId,
                                                                 size_t maxCount = 20);

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

    // Voice Activity Detection Configuration (per session)
    [[nodiscard]] Status configureVAD(SessionId sessionId, const VADConfig& config);
    [[nodiscard]] Result<VADConfig> getVADConfig(SessionId sessionId) const;
    [[nodiscard]] bool isVADActive(SessionId sessionId) const;
    [[nodiscard]] Status enableVAD(SessionId sessionId, bool enable);
    [[nodiscard]] Status disableVAD(SessionId sessionId);

    // DTW Configuration for advanced pattern matching tuning (per session)
    [[nodiscard]] Status configureDTW(SessionId sessionId, float windowRatio, bool enableSIMD = true);
    [[nodiscard]] Result<float> getDTWWindowRatio(SessionId sessionId) const;

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
        std::unique_ptr<class RealtimeScorer> realtimeScorer;
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
