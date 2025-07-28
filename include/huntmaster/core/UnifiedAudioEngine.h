/**
 * @file UnifiedAudioEngine.h
 * @brief Unified Audio Engine API - Core interface for the Huntmaster audio analysis system
 *
 * This file contains the main API for the Huntmaster Audio Engine, providing a unified
 * interface for audio recording, playback, analysis, and real-time wildlife call comparison.
 * The engine is designed to be cross-platform and can be integrated into desktop, mobile,
 * and web applications.
 *
 * @author Huntmaster Development Team
 * @version 4.1
 * @date 2025
 * @copyright All Rights Reserved - 3D Tech Solutions
 */

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

/**
 * @typedef SessionId
 * @brief Unique identifier for audio processing sessions
 *
 * Used to track and manage individual recording or analysis sessions.
 * Each session maintains its own state and can be independently controlled.
 */
using SessionId = uint32_t;

/**
 * @brief Invalid session ID constant
 *
 * Used to indicate an invalid or uninitialized session.
 * Returned by functions when session creation fails.
 */
constexpr SessionId INVALID_SESSION_ID = 0;

/**
 * @struct RealtimeScorerConfig
 * @brief Configuration parameters for real-time wildlife call scoring
 *
 * This structure defines the weights and thresholds used by the real-time
 * scoring algorithm to compare incoming audio against master wildlife calls.
 * All weight values should sum to approximately 1.0 for optimal results.
 */
struct RealtimeScorerConfig {
    float mfccWeight = 0.5f;           ///< Weight for MFCC pattern similarity (0.0-1.0)
    float volumeWeight = 0.2f;         ///< Weight for volume level matching (0.0-1.0)
    float timingWeight = 0.2f;         ///< Weight for timing/rhythm accuracy (0.0-1.0)
    float pitchWeight = 0.1f;          ///< Weight for pitch similarity (0.0-1.0, experimental)
    float confidenceThreshold = 0.7f;  ///< Minimum confidence for reliable score (0.0-1.0)
    float minScoreForMatch = 0.005f;   ///< Minimum similarity score to indicate a match
    bool enablePitchAnalysis = false;  ///< Enable experimental pitch-based scoring
    size_t scoringHistorySize = 50;    ///< Number of historical scores to retain for smoothing
};

/**
 * @struct RealtimeScoringResult
 * @brief Detailed breakdown of real-time wildlife call similarity analysis
 *
 * Contains comprehensive scoring information from the real-time analysis,
 * including individual component scores, confidence metrics, and metadata.
 * This structure provides transparency into the scoring algorithm's decision-making.
 */
struct RealtimeScoringResult {
    float overall = 0.0f;        ///< Overall weighted similarity score (0.0-1.0)
    float mfcc = 0.0f;           ///< MFCC pattern similarity component (0.0-1.0)
    float volume = 0.0f;         ///< Volume level matching component (0.0-1.0)
    float timing = 0.0f;         ///< Timing/rhythm accuracy component (0.0-1.0)
    float pitch = 0.0f;          ///< Pitch similarity component (0.0-1.0)
    float confidence = 0.0f;     ///< Algorithm confidence in result (0.0-1.0)
    bool isReliable = false;     ///< Whether score meets confidence threshold
    bool isMatch = false;        ///< Whether score indicates a positive match
    size_t samplesAnalyzed = 0;  ///< Number of audio samples analyzed
    std::chrono::steady_clock::time_point timestamp;  ///< Timestamp when score was computed
};

/**
 * @struct RealtimeFeedback
 * @brief Real-time user feedback and guidance during wildlife call analysis
 *
 * Provides comprehensive feedback to help users improve their wildlife calling
 * technique in real-time. Includes current performance, trends, and actionable
 * recommendations for better matching against master calls.
 */
struct RealtimeFeedback {
    RealtimeScoringResult currentScore;   ///< Current instantaneous similarity score
    RealtimeScoringResult trendingScore;  ///< Trending average over recent analysis history
    RealtimeScoringResult peakScore;      ///< Best score achieved during the current session
    float progressRatio = 0.0f;           ///< Progress through master call playback (0.0-1.0)
    std::string qualityAssessment;        ///< Human-readable description of match quality
    std::string recommendation;           ///< Actionable suggestion for technique improvement
    bool isImproving = false;             ///< Whether the score trend is improving over time
};

/**
 * @struct VADConfig
 * @brief Voice Activity Detection configuration parameters
 *
 * Controls the behavior of the Voice Activity Detection (VAD) system, which
 * automatically identifies periods of actual wildlife calls versus silence
 * or background noise. Proper tuning improves analysis accuracy and reduces
 * false positives.
 */
struct VADConfig {
    float energy_threshold = 0.01f;   ///< Energy threshold for voice detection (0.0-1.0)
    float window_duration = 0.025f;   ///< Analysis window duration in seconds
    float min_sound_duration = 0.1f;  ///< Minimum duration for valid voice activity (seconds)
    float pre_buffer = 0.1f;          ///< Pre-buffer duration for voice start (seconds)
    float post_buffer = 0.2f;         ///< Post-buffer duration for voice end (seconds)
    bool enabled = true;              ///< Whether VAD processing is enabled
};

/**
 * @class UnifiedAudioEngine
 * @brief Main interface for the Huntmaster Audio Analysis Engine
 *
 * The UnifiedAudioEngine provides a comprehensive API for wildlife call analysis,
 * including audio recording, playback, real-time analysis, and comparison against
 * master calls. This is the primary entry point for all client applications.
 *
 * Key Features:
 * - Cross-platform audio recording and playback
 * - Real-time wildlife call analysis and scoring
 * - MFCC-based feature extraction and DTW comparison
 * - Voice Activity Detection (VAD)
 * - Session-based state management
 * - Thread-safe operation
 *
 * @note This class is designed to be instantiated once per application and
 *       supports multiple concurrent analysis sessions.
 *
 * @example Basic Usage:
 * @code
 * auto engineResult = UnifiedAudioEngine::create();
 * if (engineResult.isOk()) {
 *     auto engine = std::move(*engineResult);
 *
 *     // Load master call
 *     auto loadResult = engine->loadMasterCall("deer_grunt.wav");
 *
 *     // Start real-time analysis
 *     auto sessionResult = engine->startRealtimeAnalysis();
 *     if (sessionResult.isOk()) {
 *         SessionId session = *sessionResult;
 *         // Process audio and get feedback...
 *     }
 * }
 * @endcode
 */
class UnifiedAudioEngine {
  public:
    /**
     * @enum Status
     * @brief Error codes and status indicators for engine operations
     *
     * All engine methods return status codes to indicate success or specific
     * failure conditions. Use isOk() or boolean conversion to check for success.
     */
    enum class Status {
        OK = 0,                  ///< Operation completed successfully
        INVALID_PARAMS = -1,     ///< Invalid parameters provided to method
        SESSION_NOT_FOUND = -2,  ///< Specified session ID does not exist
        FILE_NOT_FOUND = -3,     ///< Requested audio file could not be found
        PROCESSING_ERROR = -4,   ///< Error occurred during audio processing
        INSUFFICIENT_DATA = -5,  ///< Not enough audio data for analysis
        OUT_OF_MEMORY = -6,      ///< Memory allocation failed
        INIT_FAILED = -7,        ///< Engine initialization failed
        INTERNAL_ERROR = -8      ///< Internal engine error
    };

    /**
     * @struct Result
     * @brief Template wrapper for operation results with error handling
     *
     * Provides a safe way to return values along with status information.
     * Follows modern C++ practices for error handling without exceptions.
     *
     * @tparam T Type of the value being returned
     */
    template <typename T>
    struct [[nodiscard]] Result {
        T value{};                   ///< The result value (valid only if status == OK)
        Status status = Status::OK;  ///< Status code indicating success or failure type

        /**
         * @brief Check if the operation was successful
         * @return true if status == Status::OK, false otherwise
         */
        bool isOk() const {
            return status == Status::OK;
        }

        /**
         * @brief Boolean conversion operator for convenient success checking
         * @return true if operation was successful
         */
        operator bool() const {
            return isOk();
        }

        /**
         * @brief Get the error status
         * @return The status code (meaningful only if isOk() returns false)
         */
        Status error() const {
            return status;
        }

        /**
         * @brief Dereference operator to access the result value
         * @return Const reference to the result value
         * @warning Only call this if isOk() returns true
         */
        const T& operator*() const {
            return value;
        }

        /**
         * @brief Dereference operator to access the result value
         * @return Reference to the result value
         * @warning Only call this if isOk() returns true
         */
        T& operator*() {
            return value;
        }
    };

    /**
     * @brief Factory method to create a new UnifiedAudioEngine instance
     *
     * Creates and initializes a new engine instance with default configuration.
     * This is the preferred way to create engine instances as it ensures
     * proper initialization and resource allocation.
     *
     * @return Result containing unique_ptr to engine instance, or error status
     *
     * @note The engine performs initialization of audio systems, which may
     *       fail on systems without audio hardware or proper drivers.
     */
    [[nodiscard]] static Result<std::unique_ptr<UnifiedAudioEngine>> create();

    /**
     * @brief Destructor - cleans up all resources and active sessions
     *
     * Automatically stops all recording, closes audio devices, and frees
     * all allocated resources. All active sessions are destroyed.
     */
    ~UnifiedAudioEngine();

    // === Session Management ===

    /**
     * @brief Create a new audio analysis session
     *
     * Creates a new session with the specified sample rate. Each session
     * maintains independent state and can have its own master call loaded.
     *
     * @param sampleRate Audio sample rate in Hz (default: 44100.0f)
     * @return Result containing new SessionId, or error status
     *
     * @note Higher sample rates provide better frequency resolution but
     *       require more computational resources.
     */
    [[nodiscard]] Result<SessionId> createSession(float sampleRate = 44100.0f);

    /**
     * @brief Destroy an existing session and free its resources
     *
     * Stops any active recording, clears session state, and frees all
     * associated memory. The SessionId becomes invalid after this call.
     *
     * @param sessionId The session to destroy
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status destroySession(SessionId sessionId);

    /**
     * @brief Get list of all currently active session IDs
     *
     * @return Vector containing all valid session IDs
     */
    [[nodiscard]] std::vector<SessionId> getActiveSessions() const;

    // === Master Call Management ===

    /**
     * @brief Load a master wildlife call for comparison
     *
     * Loads and preprocesses a master call audio file for use in similarity
     * analysis. The call is associated with the specific session.
     *
     * @param sessionId Target session for the master call
     * @param masterCallId Identifier for the master call (file path or ID)
     * @return Status::OK on success, error code on failure
     *
     * @note Master calls should be high-quality recordings with minimal
     *       background noise for optimal analysis results.
     */
    [[nodiscard]] Status loadMasterCall(SessionId sessionId, std::string_view masterCallId);

    /**
     * @brief Unload the current master call from a session
     *
     * Removes the loaded master call and frees associated resources.
     * The session remains active but cannot perform similarity analysis
     * until a new master call is loaded.
     *
     * @param sessionId Target session
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status unloadMasterCall(SessionId sessionId);

    /**
     * @brief Get the identifier of the currently loaded master call
     *
     * @param sessionId Target session
     * @return Result containing master call ID string, or error status
     */
    [[nodiscard]] Result<std::string> getCurrentMasterCall(SessionId sessionId) const;

    // === Audio Processing ===

    /**
     * @brief Process a chunk of audio data for analysis
     *
     * Analyzes the provided audio buffer against the loaded master call
     * and updates the session's similarity scores and analysis state.
     *
     * @param sessionId Target session
     * @param audioBuffer Audio samples (32-bit float, normalized -1.0 to 1.0)
     * @return Status::OK on success, error code on failure
     *
     * @note Audio chunks should be consistently sized for optimal performance.
     *       Recommended chunk size is 1024-4096 samples.
     */
    [[nodiscard]] Status processAudioChunk(SessionId sessionId, std::span<const float> audioBuffer);

    /**
     * @brief Get the current similarity score for a session
     *
     * Returns the most recent similarity score computed from audio analysis.
     * Score range is typically 0.0 (no similarity) to 1.0 (perfect match).
     *
     * @param sessionId Target session
     * @return Result containing similarity score, or error status
     */
    [[nodiscard]] Result<float> getSimilarityScore(SessionId sessionId);

    /**
     * @brief Get the number of feature vectors extracted so far
     *
     * @param sessionId Target session
     * @return Result containing feature count, or error status
     */
    [[nodiscard]] Result<int> getFeatureCount(SessionId sessionId) const;

    // === Real-time Scoring Features ===

    /**
     * @brief Configure the real-time scoring algorithm parameters
     *
     * Updates the weights and thresholds used by the real-time scoring
     * system. Changes take effect immediately for subsequent analysis.
     *
     * @param sessionId Target session
     * @param config Configuration parameters for scoring algorithm
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status setRealtimeScorerConfig(SessionId sessionId,
                                                 const struct RealtimeScorerConfig& config);

    /**
     * @brief Get detailed breakdown of similarity analysis
     *
     * Returns comprehensive scoring information including individual
     * component scores, confidence metrics, and analysis metadata.
     *
     * @param sessionId Target session
     * @return Result containing detailed scoring breakdown, or error status
     */
    [[nodiscard]] Result<struct RealtimeScoringResult> getDetailedScore(SessionId sessionId);

    /**
     * @brief Get real-time user feedback and guidance
     *
     * Provides actionable feedback to help users improve their calling
     * technique, including trend analysis and specific recommendations.
     *
     * @param sessionId Target session
     * @return Result containing feedback structure, or error status
     */
    [[nodiscard]] Result<struct RealtimeFeedback> getRealtimeFeedback(SessionId sessionId);

    /**
     * @brief Export current scoring data as JSON
     *
     * @param sessionId Target session
     * @return Result containing JSON string with score data, or error status
     */
    [[nodiscard]] Result<std::string> exportScoreToJson(SessionId sessionId);

    /**
     * @brief Export current feedback data as JSON
     *
     * @param sessionId Target session
     * @return Result containing JSON string with feedback data, or error status
     */
    [[nodiscard]] Result<std::string> exportFeedbackToJson(SessionId sessionId);

    /**
     * @brief Export historical scoring data as JSON
     *
     * @param sessionId Target session
     * @param maxCount Maximum number of historical scores to include (default: 20)
     * @return Result containing JSON string with historical data, or error status
     */
    [[nodiscard]] Result<std::string> exportScoringHistoryToJson(SessionId sessionId,
                                                                 size_t maxCount = 20);

    // === Session State Queries ===

    /**
     * @brief Check if a session ID is valid and active
     *
     * @param sessionId Session ID to check
     * @return true if session exists and is active, false otherwise
     */
    [[nodiscard]] bool isSessionActive(SessionId sessionId) const;

    /**
     * @brief Get the total duration of a session
     *
     * @param sessionId Target session
     * @return Result containing duration in seconds, or error status
     */
    [[nodiscard]] Result<float> getSessionDuration(SessionId sessionId) const;

    /**
     * @brief Reset a session's analysis state
     *
     * Clears all analysis history and resets scoring state while keeping
     * the loaded master call and session configuration intact.
     *
     * @param sessionId Target session
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status resetSession(SessionId sessionId);

    // === Recording Management ===

    /**
     * @brief Start audio recording for a session
     *
     * Begins capturing audio from the default input device. Recorded
     * audio is automatically processed for analysis if a master call
     * is loaded.
     *
     * @param sessionId Target session
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status startRecording(SessionId sessionId);

    /**
     * @brief Start memory-based audio recording for a session
     *
     * Begins audio capture directly to memory buffer for later processing.
     * This mode is optimal for scenarios requiring post-processing or
     * immediate access to recorded data.
     *
     * @param sessionId Target session
     * @param maxDurationSeconds Maximum recording duration (0 = no limit)
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status startMemoryRecording(SessionId sessionId, double maxDurationSeconds = 0.0);

    /**
     * @brief Stop audio recording for a session
     *
     * Ends audio capture. Any buffered audio is processed before stopping.
     *
     * @param sessionId Target session
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status stopRecording(SessionId sessionId);

    /**
     * @brief Save recorded audio to file
     *
     * Saves the recorded audio data to a file. Works for both file-based
     * and memory-based recordings.
     *
     * @param sessionId Target session
     * @param filename Output filename
     * @return Full path to saved file on success, error on failure
     */
    [[nodiscard]] Result<std::string> saveRecording(SessionId sessionId, std::string_view filename);

    /**
     * @brief Get recorded audio data from memory
     *
     * Returns recorded audio samples directly from memory buffer.
     * Only works for memory-based recordings.
     *
     * @param sessionId Target session
     * @return Vector of audio samples (interleaved if multi-channel)
     */
    [[nodiscard]] Result<std::vector<float>> getRecordedAudioData(SessionId sessionId) const;

    /**
     * @brief Copy recorded audio data to external buffer
     *
     * Efficiently copies recorded audio data to a user-provided buffer.
     * Only works for memory-based recordings.
     *
     * @param sessionId Target session
     * @param buffer Destination buffer
     * @param maxSamples Maximum samples to copy
     * @return Number of samples actually copied
     */
    [[nodiscard]] Result<size_t>
    copyRecordedAudioData(SessionId sessionId, float* buffer, size_t maxSamples) const;

    /**
     * @brief Clear memory recording buffer
     *
     * Clears the memory recording buffer, freeing up space for new recordings.
     * Only affects memory-based recordings.
     *
     * @param sessionId Target session
     * @return Status::OK on success, error code on failure
     */
    [[nodiscard]] Status clearRecordingBuffer(SessionId sessionId);

    /**
     * @brief Get recording mode information
     */
    enum class RecordingMode { FILE_BASED, MEMORY_BASED, HYBRID };

    [[nodiscard]] Result<RecordingMode> getRecordingMode(SessionId sessionId) const;
    [[nodiscard]] Status setRecordingMode(SessionId sessionId, RecordingMode mode);

    /**
     * @brief Get memory buffer usage information
     */
    struct MemoryBufferInfo {
        size_t totalCapacityFrames;
        size_t usedFrames;
        size_t freeFrames;
        double usagePercentage;
        size_t memorySizeBytes;
        bool isGrowthEnabled;
        bool hasOverflowed;
    };
    [[nodiscard]] Result<MemoryBufferInfo> getMemoryBufferInfo(SessionId sessionId) const;
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
    [[nodiscard]] Status
    configureDTW(SessionId sessionId, float windowRatio, bool enableSIMD = true);
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
[[nodiscard]] int
unified_process_audio_chunk(int engineId, int sessionId, const float* audioBuffer, int bufferSize);
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
