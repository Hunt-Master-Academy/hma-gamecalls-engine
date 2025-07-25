/**
 * @file EnhancedWASMInterface.h
 * @brief Enhanced WASM Interface for Huntmaster Audio Engine
 *
 * This header provides an advanced WebAssembly interface with comprehensive
 * session management, real-time processing capabilities, and performance
 * monitoring for the Huntmaster Audio Engine.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 *
 * Key Features:
 * - Advanced session management with multi-session support
 * - Real-time audio processing with streaming capabilities
 * - Comprehensive error handling and propagation
 * - Performance monitoring and memory management
 * - Voice Activity Detection integration
 * - Advanced audio analysis and comparison
 *
 * @note This interface requires Emscripten and modern WebAssembly features
 */

#pragma once

#ifdef __EMSCRIPTEN__

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>

#include "huntmaster/core/RealTimeAudioProcessor.h"
#include "huntmaster/core/SessionManager.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

namespace huntmaster {
namespace wasm {

// Advanced Error Code Definitions for Comprehensive Error Handling
enum class ErrorCode : int {
    SUCCESS = 0,

    // Initialization Errors (1-99)
    INITIALIZATION_FAILED = 1,
    ENGINE_ALREADY_INITIALIZED = 2,
    ENGINE_NOT_INITIALIZED = 3,
    INVALID_CONFIGURATION = 4,
    MEMORY_ALLOCATION_FAILED = 5,
    WASM_MODULE_LOAD_FAILED = 6,
    DEPENDENCIES_NOT_AVAILABLE = 7,

    // Session Management Errors (100-199)
    SESSION_CREATE_FAILED = 100,
    SESSION_NOT_FOUND = 101,
    SESSION_ALREADY_EXISTS = 102,
    SESSION_LIMIT_EXCEEDED = 103,
    SESSION_INVALID_STATE = 104,
    SESSION_DESTROYED = 105,
    SESSION_TIMEOUT = 106,
    SESSION_PERMISSIONS_DENIED = 107,

    // Audio Processing Errors (200-299)
    AUDIO_FORMAT_UNSUPPORTED = 200,
    AUDIO_SAMPLE_RATE_INVALID = 201,
    AUDIO_CHANNELS_INVALID = 202,
    AUDIO_BUFFER_OVERFLOW = 203,
    AUDIO_BUFFER_UNDERRUN = 204,
    AUDIO_PROCESSING_FAILED = 205,
    AUDIO_QUALITY_INSUFFICIENT = 206,
    AUDIO_DURATION_INVALID = 207,

    // Real-time Processing Errors (300-399)
    REALTIME_PROCESSING_FAILED = 300,
    REALTIME_LATENCY_EXCEEDED = 301,
    REALTIME_BUFFER_FULL = 302,
    REALTIME_SYNC_LOST = 303,
    REALTIME_THREAD_ERROR = 304,

    // Voice Activity Detection Errors (400-449)
    VAD_INITIALIZATION_FAILED = 400,
    VAD_CONFIGURATION_INVALID = 401,
    VAD_PROCESSING_ERROR = 402,

    // Memory Management Errors (500-599)
    MEMORY_LIMIT_EXCEEDED = 500,
    MEMORY_CORRUPTION_DETECTED = 501,
    MEMORY_LEAK_DETECTED = 502,
    MEMORY_FRAGMENTATION_HIGH = 503,

    // Performance Errors (600-699)
    PERFORMANCE_DEGRADED = 600,
    CPU_USAGE_HIGH = 601,
    PROCESSING_TIMEOUT = 602,
    RESOURCE_EXHAUSTION = 603,

    // Network/Streaming Errors (700-799)
    NETWORK_CONNECTION_FAILED = 700,
    STREAMING_INTERRUPTED = 701,
    BANDWIDTH_INSUFFICIENT = 702,

    // Configuration Errors (800-899)
    CONFIG_PARSE_ERROR = 800,
    CONFIG_VALIDATION_FAILED = 801,
    CONFIG_VALUE_OUT_OF_RANGE = 802,

    // Security Errors (900-999)
    SECURITY_VIOLATION = 900,
    AUTHENTICATION_FAILED = 901,
    AUTHORIZATION_DENIED = 902,

    // Unknown/Generic Errors (1000+)
    UNKNOWN_ERROR = 1000,
    INTERNAL_ERROR = 1001,
    NOT_IMPLEMENTED = 1002
};

/**
 * @brief Detailed error information structure
 */
struct ErrorInfo {
    ErrorCode code;                              ///< Standardized error code
    std::string message;                         ///< Human-readable error message
    std::string details;                         ///< Technical details and context
    std::string stackTrace;                      ///< Stack trace (if available)
    uint64_t timestamp;                          ///< Error occurrence timestamp
    std::string functionName;                    ///< Function where error occurred
    std::string fileName;                        ///< Source file name
    int lineNumber;                              ///< Source line number
    std::string sessionId;                       ///< Associated session ID (if applicable)
    std::vector<std::string> additionalContext;  ///< Additional context information

    ErrorInfo() : code(ErrorCode::SUCCESS), timestamp(0), lineNumber(0) {}
    ErrorInfo(ErrorCode c, const std::string& msg)
        : code(c), message(msg), timestamp(0), lineNumber(0) {}
};

/**
 * @brief Performance metrics structure for monitoring and optimization
 */
struct PerformanceMetrics {
    // CPU metrics
    float cpuUsagePercent;  ///< Current CPU usage (0.0-100.0)
    float averageCpuUsage;  ///< Average CPU usage over time
    float peakCpuUsage;     ///< Peak CPU usage recorded

    // Memory metrics
    size_t memoryUsedBytes;     ///< Current memory usage in bytes
    size_t peakMemoryUsed;      ///< Peak memory usage recorded
    size_t memoryAvailable;     ///< Available memory
    float memoryFragmentation;  ///< Memory fragmentation level (0.0-1.0)

    // Processing metrics
    double averageLatencyMs;       ///< Average processing latency
    double peakLatencyMs;          ///< Peak processing latency
    uint64_t samplesProcessed;     ///< Total samples processed
    uint64_t operationsPerSecond;  ///< Current processing rate

    // Quality metrics
    float processingQuality;   ///< Processing quality score (0.0-1.0)
    uint32_t droppedFrames;    ///< Number of dropped frames
    uint32_t bufferUnderruns;  ///< Buffer underrun count
    uint32_t bufferOverruns;   ///< Buffer overrun count

    // Threading metrics
    uint32_t activeThreads;     ///< Number of active threads
    float threadUtilization;    ///< Thread utilization (0.0-1.0)
    uint32_t threadContention;  ///< Thread contention events

    // Timing metrics
    uint64_t uptime;           ///< System uptime in microseconds
    uint64_t sessionDuration;  ///< Current session duration
    double frameRate;          ///< Current processing frame rate

    PerformanceMetrics()
        : cpuUsagePercent(0.0f), averageCpuUsage(0.0f), peakCpuUsage(0.0f), memoryUsedBytes(0),
          peakMemoryUsed(0), memoryAvailable(0), memoryFragmentation(0.0f), averageLatencyMs(0.0),
          peakLatencyMs(0.0), samplesProcessed(0), operationsPerSecond(0), processingQuality(1.0f),
          droppedFrames(0), bufferUnderruns(0), bufferOverruns(0), activeThreads(0),
          threadUtilization(0.0f), threadContention(0), uptime(0), sessionDuration(0),
          frameRate(0.0) {}
};

/**
 * @brief Comprehensive result structure for real-time scoring operations
 *
 * Provides complete scoring information with component breakdown,
 * performance metrics, quality assessment, and diagnostic data.
 */
struct RealtimeScoringResult {
    // Core scoring results
    float overallSimilarity;  ///< Overall similarity score (0.0-1.0)
    float confidence;         ///< Confidence in the scoring result (0.0-1.0)

    // Component scores - detailed breakdown of similarity analysis
    float mfccSimilarity;      ///< MFCC pattern matching score (0.0-1.0)
    float volumeSimilarity;    ///< Volume level matching score (0.0-1.0)
    float timingSimilarity;    ///< Timing/rhythm accuracy score (0.0-1.0)
    float pitchSimilarity;     ///< Pitch similarity score (0.0-1.0)
    float spectralSimilarity;  ///< Frequency spectrum similarity (0.0-1.0)
    float harmonicSimilarity;  ///< Harmonic content similarity (0.0-1.0)

    // Quality metrics - audio quality assessment
    float signalToNoiseRatio;    ///< Signal quality measurement (dB)
    float clippingLevel;         ///< Audio clipping detection level (0.0-1.0)
    bool voiceActivityDetected;  ///< Voice activity detection result
    float vadConfidence;         ///< VAD confidence level (0.0-1.0)
    float dynamicRange;          ///< Audio dynamic range (dB)
    float backgroundNoiseLevel;  ///< Background noise level (dB)

    // Frequency analysis results
    std::vector<float> frequencySpectrum;  ///< Current frequency spectrum
    std::vector<float> spectralCentroid;   ///< Spectral centroid over time
    std::vector<float> spectralRolloff;    ///< Spectral rolloff frequencies
    float fundamentalFrequency;            ///< Estimated fundamental frequency (Hz)

    // Temporal alignment data
    float timeOffset;             ///< Time offset for best alignment (seconds)
    float alignmentConfidence;    ///< Confidence in temporal alignment (0.0-1.0)
    std::vector<float> dtw_path;  ///< Dynamic Time Warping alignment path
    float rhythmSimilarity;       ///< Rhythm pattern similarity (0.0-1.0)

    // Performance metrics
    double processingLatencyMs;    ///< Processing time in milliseconds
    size_t memoryUsedBytes;        ///< Memory used for this operation
    uint64_t timestamp;            ///< High-resolution timestamp (microseconds)
    uint32_t sequenceNumber;       ///< Sequence number for result ordering
    float cpuUsagePercent;         ///< CPU usage during processing
    size_t audioSamplesProcessed;  ///< Number of audio samples processed

    // Error handling and diagnostics
    int errorCode;                      ///< Error code (0 = success, see ErrorCodes enum)
    std::string errorMessage;           ///< Human-readable error description
    std::string debugInfo;              ///< Additional debug information
    bool isReliable;                    ///< Whether the result is considered reliable
    std::vector<std::string> warnings;  ///< Non-fatal warnings during processing

    // Analysis metadata
    uint32_t audioSampleRate;     ///< Sample rate of processed audio
    uint32_t audioChannels;       ///< Number of audio channels
    float analysisWindowSeconds;  ///< Analysis window duration
    std::string analysisMethod;   ///< Analysis method identifier

    // Default constructor with safe initialization
    RealtimeScoringResult()
        : overallSimilarity(0.0f), confidence(0.0f), mfccSimilarity(0.0f), volumeSimilarity(0.0f),
          timingSimilarity(0.0f), pitchSimilarity(0.0f), spectralSimilarity(0.0f),
          harmonicSimilarity(0.0f), signalToNoiseRatio(-60.0f), clippingLevel(0.0f),
          voiceActivityDetected(false), vadConfidence(0.0f), dynamicRange(0.0f),
          backgroundNoiseLevel(-60.0f), fundamentalFrequency(0.0f), timeOffset(0.0f),
          alignmentConfidence(0.0f), rhythmSimilarity(0.0f), processingLatencyMs(0.0),
          memoryUsedBytes(0), timestamp(0), sequenceNumber(0), cpuUsagePercent(0.0f),
          audioSamplesProcessed(0), errorCode(0), isReliable(true), audioSampleRate(44100),
          audioChannels(1), analysisWindowSeconds(0.0f) {}
};

/**
 * @brief Real-time feedback structure for live audio processing
 *
 * Provides comprehensive feedback data for visualization, monitoring,
 * quality assessment, and user guidance during live audio processing.
 */
struct RealtimeFeedback {
    // Visualization data for waveform and spectrum displays
    std::vector<float> waveformData;      ///< Current waveform samples (normalized)
    std::vector<float> spectrumData;      ///< Frequency spectrum data (magnitude)
    std::vector<float> mfccFeatures;      ///< Current MFCC feature vector
    std::vector<float> spectrogramSlice;  ///< Current spectrogram time slice
    std::vector<float> pitchContour;      ///< Pitch contour over time (Hz)

    // Real-time audio monitoring
    float currentLevel;     ///< Current audio level (dB)
    float peakLevel;        ///< Peak level in current window (dB)
    float rmsLevel;         ///< RMS level in current window (dB)
    bool isClipping;        ///< Audio clipping detected
    bool isVoiceActive;     ///< Voice activity detection result
    float vadConfidence;    ///< VAD confidence level (0.0-1.0)
    float silenceDuration;  ///< Duration of current silence (seconds)
    float speechDuration;   ///< Duration of current speech (seconds)

    // Quality indicators and assessment
    float backgroundNoiseLevel;         ///< Background noise level (dB)
    float signalQuality;                ///< Overall signal quality score (0.0-1.0)
    float distortionLevel;              ///< Audio distortion level (0.0-1.0)
    float dynamicRange;                 ///< Current dynamic range (dB)
    std::vector<std::string> warnings;  ///< Real-time warnings and alerts
    std::vector<std::string> errors;    ///< Real-time error messages

    // Intelligent user guidance and coaching
    std::vector<std::string> suggestions;  ///< Real-time improvement suggestions
    float recordingProgress;               ///< Progress through optimal recording length (0.0-1.0)
    bool readyForAnalysis;                 ///< Whether enough data for analysis
    float optimalVolumeLevel;              ///< Recommended volume level (dB)
    float currentVolumeDeviation;          ///< Deviation from optimal volume
    std::string currentGuidance;           ///< Current user guidance message
    bool shouldStartRecording;             ///< Recommendation to start recording
    bool shouldStopRecording;              ///< Recommendation to stop recording

    // Advanced analysis insights
    float harmonicRichness;           ///< Harmonic content richness (0.0-1.0)
    float spectralCentroid;           ///< Current spectral centroid (Hz)
    float zeroCrossingRate;           ///< Zero crossing rate
    float spectralFlatness;           ///< Spectral flatness measure
    std::vector<float> formantFreqs;  ///< Estimated formant frequencies
    float fundamentalFreq;            ///< Estimated fundamental frequency (Hz)

    // Performance and system metrics
    float cpuUsage;            ///< Current CPU usage (0.0-1.0)
    float memoryUsage;         ///< Current memory usage (bytes)
    double processingLatency;  ///< Current processing latency (ms)
    uint32_t bufferUnderruns;  ///< Count of buffer underruns
    uint32_t bufferOverruns;   ///< Count of buffer overruns
    bool isRealtime;           ///< Whether processing is real-time

    // Machine learning insights and predictions
    float callQualityPrediction;          ///< Predicted call quality score (0.0-1.0)
    float improvementPotential;           ///< Potential for improvement (0.0-1.0)
    std::vector<std::string> mlInsights;  ///< ML-generated insights
    float learningProgress;               ///< User learning progress indicator (0.0-1.0)
    std::string skillLevel;               ///< Estimated user skill level

    // Comparative analysis with historical data
    float historicalComparison;       ///< Comparison with user's history (0.0-1.0)
    float improvementTrend;           ///< Improvement trend indicator (-1.0 to 1.0)
    std::vector<float> recentScores;  ///< Recent scoring history
    float personalBest;               ///< User's personal best score
    float sessionAverage;             ///< Average score for current session

    // Timestamp and metadata
    uint64_t timestamp;        ///< High-resolution timestamp (microseconds)
    uint32_t frameNumber;      ///< Frame sequence number
    float analysisWindowSize;  ///< Size of analysis window (seconds)
    uint32_t sampleRate;       ///< Audio sample rate (Hz)
    uint32_t channels;         ///< Number of audio channels

    // Default constructor with safe initialization
    RealtimeFeedback()
        : currentLevel(-60.0f), peakLevel(-60.0f), rmsLevel(-60.0f), isClipping(false),
          isVoiceActive(false), vadConfidence(0.0f), silenceDuration(0.0f), speechDuration(0.0f),
          backgroundNoiseLevel(-60.0f), signalQuality(0.0f), distortionLevel(0.0f),
          dynamicRange(0.0f), recordingProgress(0.0f), readyForAnalysis(false),
          optimalVolumeLevel(-20.0f), currentVolumeDeviation(0.0f), shouldStartRecording(false),
          shouldStopRecording(false), harmonicRichness(0.0f), spectralCentroid(0.0f),
          zeroCrossingRate(0.0f), spectralFlatness(0.0f), fundamentalFreq(0.0f), cpuUsage(0.0f),
          memoryUsage(0.0f), processingLatency(0.0), bufferUnderruns(0), bufferOverruns(0),
          isRealtime(false), callQualityPrediction(0.0f), improvementPotential(0.0f),
          learningProgress(0.0f), historicalComparison(0.0f), improvementTrend(0.0f),
          personalBest(0.0f), sessionAverage(0.0f), timestamp(0), frameNumber(0),
          analysisWindowSize(0.0f), sampleRate(44100), channels(1) {}
};

// Forward declarations for advanced session management
class AudioSession;
struct SessionConfiguration;

/**
 * @brief Session configuration structure for flexible session setup
 */
struct SessionConfiguration {
    // Audio configuration
    uint32_t sampleRate;      ///< Audio sample rate (Hz)
    uint32_t channels;        ///< Number of audio channels
    uint32_t bitDepth;        ///< Audio bit depth
    std::string audioFormat;  ///< Audio format identifier

    // Processing configuration
    bool enableRealTimeProcessing;  ///< Enable real-time processing
    bool enableVAD;                 ///< Enable Voice Activity Detection
    float vadSensitivity;           ///< VAD sensitivity (0.0-1.0)
    bool enableQualityAssessment;   ///< Enable quality assessment

    // Buffer configuration
    uint32_t bufferSizeMs;     ///< Buffer size in milliseconds
    uint32_t maxBufferCount;   ///< Maximum number of buffers
    bool enableStreamingMode;  ///< Enable streaming mode

    // Performance configuration
    uint32_t maxCpuUsage;   ///< Maximum CPU usage percentage
    size_t maxMemoryUsage;  ///< Maximum memory usage in bytes
    double maxLatencyMs;    ///< Maximum acceptable latency

    // Session metadata
    std::string name;         ///< Session name
    std::string description;  ///< Session description
    std::string userAgent;    ///< User agent information
    uint32_t timeoutSeconds;  ///< Session timeout in seconds

    // Security configuration
    bool enableSecureMode;    ///< Enable secure mode
    std::string accessToken;  ///< Access token for authentication

    SessionConfiguration()
        : sampleRate(44100), channels(1), bitDepth(16), audioFormat("pcm"),
          enableRealTimeProcessing(true), enableVAD(true), vadSensitivity(0.5f),
          enableQualityAssessment(true), bufferSizeMs(100), maxBufferCount(10),
          enableStreamingMode(false), maxCpuUsage(80), maxMemoryUsage(1024 * 1024 * 100),
          maxLatencyMs(50.0), timeoutSeconds(3600), enableSecureMode(false) {}
};

/**
 * @brief Advanced audio session with comprehensive state management
 */
class AudioSession {
  public:
    enum class State {
        CREATED,       ///< Session created but not initialized
        INITIALIZING,  ///< Session initialization in progress
        READY,         ///< Session ready for processing
        ACTIVE,        ///< Session actively processing audio
        SUSPENDED,     ///< Session suspended/paused
        ERROR,         ///< Session in error state
        DESTROYING,    ///< Session being destroyed
        DESTROYED      ///< Session destroyed
    };

    explicit AudioSession(const std::string& id, const SessionConfiguration& config);
    ~AudioSession();

    // Session lifecycle management
    bool initialize();
    bool start();
    bool suspend();
    bool resume();
    bool stop();
    bool destroy();

    // State management
    State getState() const {
        return state_;
    }
    bool isActive() const {
        return state_ == State::ACTIVE;
    }
    bool isReady() const {
        return state_ == State::READY || state_ == State::ACTIVE;
    }

    // Session information
    const std::string& getId() const {
        return id_;
    }
    const SessionConfiguration& getConfiguration() const {
        return config_;
    }
    uint64_t getCreationTime() const {
        return creationTime_;
    }
    uint64_t getLastActivityTime() const {
        return lastActivityTime_;
    }

    // Performance and monitoring
    PerformanceMetrics getPerformanceMetrics() const;
    void updateActivity();
    bool hasTimedOut() const;

    // Error handling
    ErrorInfo getLastError() const {
        return lastError_;
    }
    void setError(const ErrorInfo& error);

  private:
    std::string id_;
    SessionConfiguration config_;
    std::atomic<State> state_;
    uint64_t creationTime_;
    mutable std::atomic<uint64_t> lastActivityTime_;
    ErrorInfo lastError_;
    mutable std::mutex sessionMutex_;

    // Session-specific resources
    std::unique_ptr<RealTimeAudioProcessor> processor_;
    std::unique_ptr<CircularAudioBuffer> buffer_;
    PerformanceMetrics metrics_;
};

/**
 * @brief Advanced session manager for multi-session WASM operations
 *
 * Provides comprehensive session management with isolation, lifecycle management,
 * resource tracking, and performance monitoring.
 */
class EnhancedSessionManager {
  public:
    EnhancedSessionManager();
    ~EnhancedSessionManager();

    /**
     * @brief Create a new audio processing session with configuration validation
     * @param sessionConfig JSON configuration for the session
     * @return Session ID on success, empty string on failure
     */
    std::string createSession(const std::string& sessionConfig);

    /**
     * @brief Create a new session with structured configuration
     * @param config Session configuration structure
     * @return Session ID on success, empty string on failure
     */
    std::string createSession(const SessionConfiguration& config);

    /**
     * @brief Destroy an existing session and cleanup resources
     * @param sessionId ID of the session to destroy
     * @return true on successful destruction, false on failure
     */
    bool destroySession(const std::string& sessionId);

    /**
     * @brief Suspend a session (pause processing, retain state)
     * @param sessionId ID of the session to suspend
     * @return true on successful suspension, false on failure
     */
    bool suspendSession(const std::string& sessionId);

    /**
     * @brief Resume a suspended session
     * @param sessionId ID of the session to resume
     * @return true on successful resumption, false on failure
     */
    bool resumeSession(const std::string& sessionId);

    /**
     * @brief Get session reference for direct operations
     * @param sessionId ID of the session
     * @return Shared pointer to session, nullptr if not found
     */
    std::shared_ptr<AudioSession> getSession(const std::string& sessionId);

    /**
     * @brief Get current session statistics and performance metrics
     * @param sessionId ID of the session
     * @return JavaScript object with session statistics
     */
    emscripten::val getSessionStats(const std::string& sessionId);

    /**
     * @brief Get list of all active sessions with their states
     * @return Vector of session IDs
     */
    std::vector<std::string> getActiveSessions();

    /**
     * @brief Get comprehensive session information
     * @return JavaScript object with all session information
     */
    emscripten::val getAllSessionsInfo();

    /**
     * @brief Cleanup timed-out sessions
     * @return Number of sessions cleaned up
     */
    uint32_t cleanupTimedOutSessions();

    /**
     * @brief Get overall session manager statistics
     * @return JavaScript object with manager statistics
     */
    emscripten::val getManagerStats();

    /**
     * @brief Set global session limits and constraints
     * @param maxSessions Maximum number of concurrent sessions
     * @param maxMemoryPerSession Maximum memory per session (bytes)
     * @param defaultTimeoutSeconds Default session timeout
     */
    void setGlobalLimits(uint32_t maxSessions,
                         size_t maxMemoryPerSession,
                         uint32_t defaultTimeoutSeconds);

    /**
     * @brief Enable or disable session persistence
     * @param enabled Whether to enable session state persistence
     */
    void setSessionPersistence(bool enabled);

    /**
     * @brief Export session data for backup or analysis
     * @param sessionId ID of the session to export
     * @return JSON string with session data
     */
    std::string exportSessionData(const std::string& sessionId);

    /**
     * @brief Import session data to restore a session
     * @param sessionData JSON string with session data
     * @return Session ID on success, empty string on failure
     */
    std::string importSessionData(const std::string& sessionData);

  private:
    // Internal session management data structures
    std::unordered_map<std::string, std::shared_ptr<AudioSession>> sessions_;
    mutable std::shared_mutex sessionsMutex_;
    std::atomic<uint32_t> nextSessionId_{1};

    // Global configuration and limits
    uint32_t maxSessions_{10};
    size_t maxMemoryPerSession_{1024 * 1024 * 100};  // 100MB default
    uint32_t defaultTimeoutSeconds_{3600};           // 1 hour default
    bool persistenceEnabled_{false};

    // Performance monitoring
    mutable PerformanceMetrics managerMetrics_;
    std::atomic<uint64_t> totalSessionsCreated_{0};
    std::atomic<uint64_t> totalSessionsDestroyed_{0};

    // Internal helper methods
    std::string generateSessionId();
    bool validateSessionConfiguration(const SessionConfiguration& config);
    SessionConfiguration parseConfigurationString(const std::string& configStr);
    void updateManagerMetrics();
    void cleanupSessionResources(std::shared_ptr<AudioSession> session);

    // Monitoring and cleanup thread
    std::unique_ptr<std::thread> cleanupThread_;
    std::atomic<bool> shouldStopCleanup_{false};
    void cleanupThreadFunction();
};

/**
 * @brief Enhanced WASM interface with advanced features
 *
 * This class provides the main interface between JavaScript and the
 * Huntmaster Audio Engine WebAssembly module, with comprehensive
 * functionality for real-time audio processing and analysis.
 *
 * Features include:
 * - Advanced session management and multi-session support
 * - Real-time audio processing with streaming capabilities
 * - Voice Activity Detection integration and configuration
 * - Performance monitoring and memory management
 * - Advanced error handling and recovery mechanisms
 * - Comprehensive audio analysis and comparison features
 * - Configuration management and persistence
 * - Audio format detection and conversion support
 * - Advanced debugging and diagnostic capabilities
 * - Machine learning model integration and inference
 */
class EnhancedWASMInterface {
  public:
    EnhancedWASMInterface();
    ~EnhancedWASMInterface();

    // Core Engine Management (TODO 1.2.4)
    /**
     * @brief Initialize the WASM interface with advanced configuration
     * @param config JavaScript object with initialization parameters
     * @return true on successful initialization, false on failure
     */
    bool initialize(emscripten::val config);

    /**
     * @brief Shutdown the interface and cleanup all resources
     * Ensures safe cleanup of all sessions, threads, and allocated memory
     */
    void shutdown();

    /**
     * @brief Check if the interface is properly initialized
     * @return true if initialized and ready for use
     */
    bool isInitialized() const {
        return initialized_.load();
    }

    /**
     * @brief Get current engine status and health metrics
     * @return JavaScript object with detailed status information
     */
    emscripten::val getEngineStatus();

    /**
     * @brief Restart the engine with new configuration
     * @param config New configuration parameters
     * @return true on successful restart
     */
    bool restart(emscripten::val config);

    // Advanced Session Management (TODO 1.2.5)
    /**
     * @brief Create new session with specified configuration
     * @param sessionConfig JavaScript object with session parameters
     * @return Session ID string on success, empty string on failure
     */
    std::string createSession(emscripten::val sessionConfig);

    /**
     * @brief Create session with default configuration
     * @return Session ID string on success, empty string on failure
     */
    std::string createDefaultSession();

    /**
     * @brief Destroy session and cleanup resources
     * @param sessionId ID of the session to destroy
     * @return true on successful destruction
     */
    bool destroySession(const std::string& sessionId);

    /**
     * @brief Suspend session (pause processing, retain state)
     * @param sessionId ID of the session to suspend
     * @return true on successful suspension
     */
    bool suspendSession(const std::string& sessionId);

    /**
     * @brief Resume suspended session
     * @param sessionId ID of the session to resume
     * @return true on successful resumption
     */
    bool resumeSession(const std::string& sessionId);

    /**
     * @brief Get session statistics and performance data
     * @param sessionId ID of the session
     * @return JavaScript object with session statistics
     */
    emscripten::val getSessionStats(const std::string& sessionId);

    /**
     * @brief Get list of all active sessions
     * @return JavaScript array of session IDs
     */
    emscripten::val getActiveSessions();

    /**
     * @brief Configure session parameters during runtime
     * @param sessionId ID of the session to configure
     * @param config JavaScript object with new configuration
     * @return true on successful configuration
     */
    bool configureSession(const std::string& sessionId, emscripten::val config);

    // Real-time Audio Processing (TODO 1.2.6)
    /**
     * @brief Process audio chunk with real-time feedback
     * @param sessionId ID of the processing session
     * @param audioData JavaScript ArrayBuffer with audio data
     * @param enableRealtimeFeedback Whether to generate real-time feedback
     * @return JavaScript object with processing results and feedback
     */
    emscripten::val processAudioChunk(const std::string& sessionId,
                                      emscripten::val audioData,
                                      bool enableRealtimeFeedback = true);

    /**
     * @brief Process audio with reference comparison
     * @param sessionId ID of the processing session
     * @param audioData Audio data to process
     * @param referenceData Reference audio for comparison
     * @return JavaScript object with comparison results
     */
    emscripten::val processAudioWithReference(const std::string& sessionId,
                                              emscripten::val audioData,
                                              emscripten::val referenceData);

    /**
     * @brief Start streaming audio processing mode
     * @param sessionId ID of the processing session
     * @param streamConfig JavaScript object with streaming configuration
     * @return true on successful streaming start
     */
    bool startStreaming(const std::string& sessionId, emscripten::val streamConfig);

    /**
     * @brief Stop streaming mode and finalize results
     * @param sessionId ID of the processing session
     * @return JavaScript object with final streaming results
     */
    emscripten::val stopStreaming(const std::string& sessionId);

    /**
     * @brief Get current streaming status and progress
     * @param sessionId ID of the processing session
     * @return JavaScript object with streaming status
     */
    emscripten::val getStreamingStatus(const std::string& sessionId);

    /**
     * @brief Process audio file (non-real-time batch processing)
     * @param sessionId ID of the processing session
     * @param audioFileData Complete audio file data
     * @param analysisOptions Analysis options and parameters
     * @return JavaScript object with complete analysis results
     */
    emscripten::val processAudioFile(const std::string& sessionId,
                                     emscripten::val audioFileData,
                                     emscripten::val analysisOptions);

    // Voice Activity Detection (TODO 1.2.7)
    /**
     * @brief Configure Voice Activity Detection parameters
     * @param sessionId ID of the session
     * @param vadConfig JavaScript object with VAD configuration
     * @return true on successful configuration
     */
    bool configureVAD(const std::string& sessionId, emscripten::val vadConfig);

    /**
     * @brief Get current VAD state and confidence
     * @param sessionId ID of the session
     * @return JavaScript object with VAD status and metrics
     */
    emscripten::val getVADStatus(const std::string& sessionId);

    /**
     * @brief Enable or disable VAD for a session
     * @param sessionId ID of the session
     * @param enabled Whether to enable VAD
     * @return true on successful state change
     */
    bool setVADEnabled(const std::string& sessionId, bool enabled);

    /**
     * @brief Get VAD sensitivity threshold
     * @param sessionId ID of the session
     * @return Current VAD sensitivity (0.0-1.0)
     */
    float getVADSensitivity(const std::string& sessionId);

    /**
     * @brief Set VAD sensitivity threshold
     * @param sessionId ID of the session
     * @param sensitivity New sensitivity value (0.0-1.0)
     * @return true on successful setting
     */
    bool setVADSensitivity(const std::string& sessionId, float sensitivity);

    // Memory Management and Performance (TODO 1.2.8)
    /**
     * @brief Get current memory usage statistics
     * @return JavaScript object with detailed memory information
     */
    emscripten::val getMemoryStats();

    /**
     * @brief Force garbage collection and memory cleanup
     * Attempts to free unused memory and optimize memory layout
     */
    void forceGarbageCollection();

    /**
     * @brief Get performance metrics and profiling data
     * @return JavaScript object with comprehensive performance data
     */
    emscripten::val getPerformanceMetrics();

    /**
     * @brief Set memory usage limits
     * @param maxMemoryBytes Maximum memory usage in bytes
     * @param alertThresholdPercent Alert threshold as percentage (0-100)
     * @return true on successful limit setting
     */
    bool setMemoryLimits(size_t maxMemoryBytes, float alertThresholdPercent);

    /**
     * @brief Enable or disable performance monitoring
     * @param enabled Whether to enable performance monitoring
     * @param detailLevel Detail level (0=basic, 1=normal, 2=detailed)
     */
    void setPerformanceMonitoring(bool enabled, int detailLevel = 1);

    /**
     * @brief Get system resource usage
     * @return JavaScript object with system resource information
     */
    emscripten::val getSystemResourceUsage();

    // Advanced Error Handling (TODO 1.2.9)
    /**
     * @brief Get last error information with detailed diagnostics
     * @return JavaScript object with comprehensive error information
     */
    emscripten::val getLastError();

    /**
     * @brief Get all recent errors
     * @param maxErrors Maximum number of errors to return
     * @return JavaScript array of error objects
     */
    emscripten::val getRecentErrors(int maxErrors = 10);

    /**
     * @brief Clear error state and reset error tracking
     */
    void clearErrors();

    /**
     * @brief Enable or disable detailed error logging
     * @param level Logging level (0=none, 1=errors, 2=warnings, 3=info, 4=debug)
     */
    void setErrorLoggingLevel(int level);

    /**
     * @brief Register error callback for JavaScript notifications
     * @param callback JavaScript function to call on errors
     */
    void registerErrorCallback(emscripten::val callback);

    /**
     * @brief Get error statistics
     * @return JavaScript object with error statistics
     */
    emscripten::val getErrorStats();

    // Audio Format and Configuration
    /**
     * @brief Get supported audio formats
     * @return JavaScript array of supported format strings
     */
    emscripten::val getSupportedAudioFormats();

    /**
     * @brief Detect audio format from data
     * @param audioData Audio data to analyze
     * @return JavaScript object with format information
     */
    emscripten::val detectAudioFormat(emscripten::val audioData);

    /**
     * @brief Convert audio format
     * @param audioData Input audio data
     * @param targetFormat Target format specification
     * @return JavaScript object with converted audio data
     */
    emscripten::val convertAudioFormat(emscripten::val audioData, emscripten::val targetFormat);

    // Advanced Features
    /**
     * @brief Get engine capabilities and feature support
     * @return JavaScript object with capability information
     */
    emscripten::val getEngineCapabilities();

    /**
     * @brief Enable experimental features
     * @param featureName Name of the experimental feature
     * @param enabled Whether to enable the feature
     * @return true if feature was successfully enabled/disabled
     */
    bool setExperimentalFeature(const std::string& featureName, bool enabled);

    /**
     * @brief Get version information
     * @return JavaScript object with version details
     */
    emscripten::val getVersionInfo();

    /**
     * @brief Run diagnostic tests
     * @return JavaScript object with diagnostic results
     */
    emscripten::val runDiagnostics();

  private:
  private:
    // Core engine components
    std::unique_ptr<UnifiedAudioEngine> engine_;
    std::unique_ptr<EnhancedSessionManager> sessionManager_;
    std::unique_ptr<RealTimeAudioProcessor> realtimeProcessor_;

    // Threading and synchronization
    mutable std::shared_mutex interfaceMutex_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdownRequested_{false};

    // Configuration and state management
    struct InterfaceConfiguration {
        bool enableRealTimeProcessing;
        bool enablePerformanceMonitoring;
        bool enableAdvancedErrorHandling;
        int errorLoggingLevel;
        size_t maxMemoryUsage;
        uint32_t maxSessions;
        double maxProcessingLatencyMs;
        bool enableExperimentalFeatures;
        std::vector<std::string> enabledExperimentalFeatures;

        InterfaceConfiguration()
            : enableRealTimeProcessing(true), enablePerformanceMonitoring(true),
              enableAdvancedErrorHandling(true), errorLoggingLevel(2),
              maxMemoryUsage(1024 * 1024 * 500), maxSessions(10), maxProcessingLatencyMs(50.0),
              enableExperimentalFeatures(false) {}
    } configuration_;

    // Error tracking and management
    mutable std::mutex errorMutex_;
    std::queue<ErrorInfo> recentErrors_;
    std::atomic<uint32_t> totalErrorCount_{0};
    std::atomic<uint32_t> errorCountSinceLastClear_{0};
    emscripten::val errorCallback_;
    bool hasErrorCallback_{false};

    // Performance monitoring
    mutable std::mutex perfMutex_;
    PerformanceMetrics interfaceMetrics_;
    std::atomic<uint64_t> totalOperationsCount_{0};
    std::atomic<uint64_t> interfaceStartTime_{0};
    bool performanceMonitoringEnabled_{true};
    int performanceDetailLevel_{1};

    // Memory management
    struct MemoryLimits {
        size_t maxTotalMemory;
        size_t alertThreshold;
        bool enforceLimit;
        float alertThresholdPercent;

        MemoryLimits()
            : maxTotalMemory(1024 * 1024 * 1024),  // 1GB default
              alertThresholdPercent(80.0f), enforceLimit(false) {
            alertThreshold = static_cast<size_t>(maxTotalMemory * alertThresholdPercent / 100.0f);
        }
    } memoryLimits_;

    std::atomic<size_t> currentMemoryUsage_{0};
    mutable std::mutex memoryMutex_;

    // Audio format support tracking
    std::vector<std::string> supportedFormats_;
    std::unordered_map<std::string, bool> formatCapabilities_;

    // Experimental features registry
    std::unordered_map<std::string, bool> experimentalFeatures_;

    // Internal helper methods for initialization and configuration
    bool initializeEngine(const emscripten::val& config);
    bool initializeSessionManager();
    bool initializeRealtimeProcessor();
    bool loadConfiguration(const emscripten::val& config);
    bool validateConfiguration() const;
    void setupDefaultConfiguration();

    // Resource management helpers
    void cleanupResources();
    bool checkResourceLimits() const;
    void updateMemoryUsage();
    void optimizeMemoryLayout();

    // Error handling helpers
    void recordError(ErrorCode code,
                     const std::string& message,
                     const std::string& details = "",
                     const std::string& context = "");
    void notifyErrorCallback(const ErrorInfo& error);
    std::string formatErrorForLogging(const ErrorInfo& error) const;
    void maintainErrorHistory();

    // Performance monitoring helpers
    void updatePerformanceMetrics();
    void recordOperation(const std::string& operationType, double durationMs);
    emscripten::val getDetailedPerformanceData() const;
    void resetPerformanceCounters();

    // Session management helpers
    bool validateSessionId(const std::string& sessionId) const;
    SessionConfiguration parseSessionConfig(const emscripten::val& config) const;
    emscripten::val sessionStatsToJSObject(const PerformanceMetrics& metrics) const;

    // Audio processing helpers
    bool validateAudioData(const emscripten::val& audioData) const;
    std::vector<float> extractAudioSamples(const emscripten::val& audioData) const;
    emscripten::val createResultObject(const RealtimeScoringResult& result) const;
    emscripten::val createFeedbackObject(const RealtimeFeedback& feedback) const;

    // Format detection and conversion helpers
    std::string detectFormatFromData(const std::vector<float>& audioData) const;
    bool isFormatSupported(const std::string& format) const;
    std::vector<float> convertAudioData(const std::vector<float>& input,
                                        const std::string& sourceFormat,
                                        const std::string& targetFormat) const;

    // Threading and async operation management
    std::unique_ptr<std::thread> backgroundThread_;
    std::atomic<bool> backgroundThreadRunning_{false};
    void backgroundThreadFunction();
    void scheduleBackgroundTask(std::function<void()> task);
    std::queue<std::function<void()>> backgroundTasks_;
    std::mutex backgroundTasksMutex_;
    std::condition_variable backgroundTasksCV_;

    // Diagnostic and debugging support
    struct DiagnosticInfo {
        std::string component;
        std::string status;
        std::string details;
        uint64_t timestamp;

        DiagnosticInfo(const std::string& comp,
                       const std::string& stat,
                       const std::string& det = "")
            : component(comp), status(stat), details(det), timestamp(0) {}
    };

    std::vector<DiagnosticInfo> runInternalDiagnostics() const;
    bool checkEngineHealth() const;
    bool checkMemoryHealth() const;
    bool checkPerformanceHealth() const;

    // Utility methods
    uint64_t getCurrentTimestamp() const;
    std::string generateUniqueId() const;
    emscripten::val vectorToJSArray(const std::vector<float>& vec) const;
    std::vector<float> jsArrayToVector(const emscripten::val& arr) const;
    emscripten::val createVersionObject() const;

    // Constants and static configuration
    static constexpr uint32_t MAX_ERROR_HISTORY = 100;
    static constexpr uint32_t MAX_PERFORMANCE_SAMPLES = 1000;
    static constexpr double DEFAULT_PROCESSING_TIMEOUT_MS = 5000.0;
    static constexpr size_t MIN_AUDIO_SAMPLES = 1024;
    static constexpr size_t MAX_AUDIO_SAMPLES = 1024 * 1024 * 10;  // 10M samples max
};

// Emscripten Bindings Enhancement (TODO 1.2.11)
// ==============================================

/**
 * @brief Comprehensive Emscripten bindings for JavaScript integration
 *
 * This section provides complete bindings for all classes, structures,
 * and enums to enable seamless JavaScript integration with proper type
 * safety, error handling, and memory management.
 */

// Helper functions for JavaScript integration
namespace bindings_helpers {
/**
 * @brief Convert RealtimeScoringResult to JavaScript object
 */
emscripten::val scoringResultToJS(const RealtimeScoringResult& result);

/**
 * @brief Convert RealtimeFeedback to JavaScript object
 */
emscripten::val feedbackToJS(const RealtimeFeedback& feedback);

/**
 * @brief Convert ErrorInfo to JavaScript object
 */
emscripten::val errorInfoToJS(const ErrorInfo& error);

/**
 * @brief Convert PerformanceMetrics to JavaScript object
 */
emscripten::val performanceMetricsToJS(const PerformanceMetrics& metrics);

/**
 * @brief Convert SessionConfiguration from JavaScript object
 */
SessionConfiguration sessionConfigFromJS(const emscripten::val& jsConfig);

/**
 * @brief Validate JavaScript audio data
 */
bool validateJSAudioData(const emscripten::val& audioData);

/**
 * @brief Convert JavaScript ArrayBuffer to float vector
 */
std::vector<float> arrayBufferToFloatVector(const emscripten::val& buffer);

/**
 * @brief Convert float vector to JavaScript ArrayBuffer
 */
emscripten::val floatVectorToArrayBuffer(const std::vector<float>& data);

/**
 * @brief Handle JavaScript exceptions safely
 */
template <typename T>
emscripten::val safeCall(std::function<T()> func, const std::string& operation);
}  // namespace bindings_helpers

// Callback management for JavaScript integration
class JavaScriptCallbackManager {
  public:
    // Callback types
    using ErrorCallback = std::function<void(const ErrorInfo&)>;
    using ProgressCallback = std::function<void(float)>;
    using ResultCallback = std::function<void(const RealtimeScoringResult&)>;
    using FeedbackCallback = std::function<void(const RealtimeFeedback&)>;

    /**
     * @brief Register error callback
     */
    void registerErrorCallback(emscripten::val callback);

    /**
     * @brief Register progress callback
     */
    void registerProgressCallback(emscripten::val callback);

    /**
     * @brief Register result callback
     */
    void registerResultCallback(emscripten::val callback);

    /**
     * @brief Register feedback callback
     */
    void registerFeedbackCallback(emscripten::val callback);

    /**
     * @brief Notify all registered callbacks
     */
    void notifyError(const ErrorInfo& error);
    void notifyProgress(float progress);
    void notifyResult(const RealtimeScoringResult& result);
    void notifyFeedback(const RealtimeFeedback& feedback);

    /**
     * @brief Clear all callbacks
     */
    void clearCallbacks();

  private:
    emscripten::val errorCallback_;
    emscripten::val progressCallback_;
    emscripten::val resultCallback_;
    emscripten::val feedbackCallback_;

    bool hasErrorCallback_{false};
    bool hasProgressCallback_{false};
    bool hasResultCallback_{false};
    bool hasFeedbackCallback_{false};
};

// Promise-based async operations for JavaScript integration
class AsyncOperationManager {
  public:
    /**
     * @brief Process audio asynchronously with Promise return
     */
    emscripten::val processAudioAsync(const std::string& sessionId,
                                      emscripten::val audioData,
                                      emscripten::val options);

    /**
     * @brief Initialize engine asynchronously
     */
    emscripten::val initializeAsync(emscripten::val config);

    /**
     * @brief Load audio file asynchronously
     */
    emscripten::val loadAudioFileAsync(emscripten::val fileData);

    /**
     * @brief Export session data asynchronously
     */
    emscripten::val exportSessionAsync(const std::string& sessionId);

  private:
    // Promise resolver helper
    template <typename T>
    void resolvePromise(emscripten::val resolver, const T& result);

    template <typename E>
    void rejectPromise(emscripten::val resolver, const E& error);
};

// WebWorker integration support
class WebWorkerInterface {
  public:
    /**
     * @brief Setup message handling for WebWorker
     */
    void setupWorkerMessageHandling();

    /**
     * @brief Process message from main thread
     */
    void processWorkerMessage(emscripten::val message);

    /**
     * @brief Send result back to main thread
     */
    void sendWorkerResult(const std::string& messageId, emscripten::val result);

    /**
     * @brief Send error back to main thread
     */
    void sendWorkerError(const std::string& messageId, const ErrorInfo& error);

  private:
    std::unordered_map<std::string, std::string> pendingOperations_;
    std::mutex workerMutex_;
};

/**
 * @brief Main binding function to expose all functionality to JavaScript
 *
 * This function creates comprehensive Emscripten bindings for:
 * - All classes with complete method exposure
 * - All structures with field access and constructors
 * - All enums with proper value mapping
 * - Type conversions and validation
 * - Error handling and exception propagation
 * - Memory management for complex types
 * - Async operation support with promises
 * - Callback registration and management
 * - Event system for real-time notifications
 * - Configuration object handling
 * - Array and buffer management optimization
 */
void registerWASMBindings();

}  // namespace wasm
}  // namespace huntmaster

#endif  // __EMSCRIPTEN__
