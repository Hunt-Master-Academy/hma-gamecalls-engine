#include "huntmaster/core/UnifiedAudioEngine.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <unordered_map>

#include "huntmaster/core/DebugLogger.h"

// Enable debug output for UnifiedAudioEngine
#define DEBUG_UNIFIED_AUDIO_ENGINE 0

// Include existing components
#include "dr_wav.h"
#include "huntmaster/core/AudioLevelProcessor.h"
#include "huntmaster/core/AudioPlayer.h"
#include "huntmaster/core/AudioRecorder.h"
#include "huntmaster/core/DTWProcessor.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/VoiceActivityDetector.h"

namespace huntmaster {

// RAII wrapper for dr_wav memory
class DrWavRAII {
   public:
    explicit DrWavRAII(float* data) : data_(data) {}
    ~DrWavRAII() {
        if (data_) {
            drwav_free(data_, nullptr);
        }
    }
    DrWavRAII(const DrWavRAII&) = delete;
    DrWavRAII& operator=(const DrWavRAII&) = delete;
    DrWavRAII(DrWavRAII&& other) noexcept : data_(other.data_) { other.data_ = nullptr; }
    DrWavRAII& operator=(DrWavRAII&& other) noexcept {
        if (this != &other) {
            if (data_) drwav_free(data_, nullptr);
            data_ = other.data_;
            other.data_ = nullptr;
        }
        return *this;
    }
    float* get() const { return data_; }

   private:
    float* data_;
};

class UnifiedAudioEngine::Impl {
   public:
    Impl() = default;
    ~Impl() = default;

    // Core functionality
    Result<SessionId> createSession(float sampleRate);
    Status destroySession(SessionId sessionId);
    std::vector<SessionId> getActiveSessions() const;

    // Master call management - PER SESSION
    Status loadMasterCall(SessionId sessionId, std::string_view masterCallId);
    Status unloadMasterCall(SessionId sessionId);
    Result<std::string> getCurrentMasterCall(SessionId sessionId) const;

    // Audio processing
    Status processAudioChunk(SessionId sessionId, std::span<const float> audioBuffer);
    Result<float> getSimilarityScore(SessionId sessionId);
    Result<int> getFeatureCount(SessionId sessionId) const;

    // Session state
    bool isSessionActive(SessionId sessionId) const;
    Result<float> getSessionDuration(SessionId sessionId) const;
    Status resetSession(SessionId sessionId);

    // Recording
    Status startRecording(SessionId sessionId);
    Status stopRecording(SessionId sessionId);
    Result<std::string> saveRecording(SessionId sessionId, std::string_view filename);
    bool isRecording(SessionId sessionId) const;
    Result<float> getRecordingLevel(SessionId sessionId) const;
    Result<double> getRecordingDuration(SessionId sessionId) const;

    // Audio Playback
    Status playMasterCall(SessionId sessionId, std::string_view masterCallId);
    Status playRecording(SessionId sessionId, std::string_view filename);
    Status stopPlayback(SessionId sessionId);
    bool isPlaying(SessionId sessionId) const;
    Result<double> getPlaybackPosition(SessionId sessionId) const;
    Status setPlaybackVolume(SessionId sessionId, float volume);

    // Real-time Session Management
    Result<SessionId> startRealtimeSession(float sampleRate, int bufferSize);
    Status endRealtimeSession(SessionId sessionId);
    bool isRealtimeSession(SessionId sessionId) const;

   private:
    // Session state structure - each session is completely isolated
    struct SessionState {
        SessionId id;
        float sampleRate;
        std::chrono::steady_clock::time_point startTime;

        // Per-session master call (KEY IMPROVEMENT)
        std::vector<std::vector<float>> masterCallFeatures;
        std::string masterCallId;

        // Audio processing state
        std::vector<float> currentSegmentBuffer;
        std::vector<std::vector<float>> sessionFeatures;

        // Processing components (per-session for true isolation)
        std::unique_ptr<MFCCProcessor> mfccProcessor;
        std::unique_ptr<VoiceActivityDetector> vad;
        std::unique_ptr<AudioPlayer> audioPlayer;
        std::unique_ptr<AudioRecorder> audioRecorder;
        std::unique_ptr<AudioLevelProcessor> levelProcessor;

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

        SessionState(SessionId id, float sampleRate)
            : id(id), sampleRate(sampleRate), startTime(std::chrono::steady_clock::now()) {
            // Initialize MFCC processor with standard configuration
            MFCCProcessor::Config mfccConfig;
            mfccConfig.sample_rate = sampleRate;
            mfccConfig.frame_size = 512;
            mfccConfig.num_coefficients = 13;
            mfccConfig.num_filters = 26;
            mfccProcessor = std::make_unique<MFCCProcessor>(mfccConfig);

            // Initialize VAD with default configuration
            VoiceActivityDetector::Config vadConfig;
            vad = std::make_unique<VoiceActivityDetector>(vadConfig);

            // Initialize audio components
            audioPlayer = std::make_unique<AudioPlayer>();
            audioRecorder = std::make_unique<AudioRecorder>();

            // Initialize level processor
            AudioLevelProcessor::Config levelConfig;
            levelConfig.sampleRate = sampleRate;
            levelProcessor = std::make_unique<AudioLevelProcessor>(levelConfig);
        }
    };

    // Thread-safe session management
    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<SessionId, std::unique_ptr<SessionState>> sessions_;
    std::atomic<SessionId> nextSessionId_{1};

    // Configuration paths
    std::string masterCallsPath_{"../data/master_calls/"};
    std::string featuresPath_{"../data/features/"};
    std::string recordingsPath_{"../data/recordings/"};

    // Helper methods
    SessionState* getSession(SessionId sessionId);
    const SessionState* getSession(SessionId sessionId) const;
    Status loadFeaturesFromFile(SessionState& session, const std::string& masterCallId);
    void saveFeaturesToFile(const SessionState& session, const std::string& masterCallId);
    void extractMFCCFeatures(SessionState& session);
};

// === Implementation ===

UnifiedAudioEngine::Result<std::unique_ptr<UnifiedAudioEngine>> UnifiedAudioEngine::create() {
    try {
        auto engine = std::unique_ptr<UnifiedAudioEngine>(new UnifiedAudioEngine());
        return {std::move(engine), Status::OK};
    } catch (const std::exception&) {
        return {nullptr, Status::INIT_FAILED};
    }
}

UnifiedAudioEngine::UnifiedAudioEngine() : pimpl(std::make_unique<Impl>()) {}
UnifiedAudioEngine::~UnifiedAudioEngine() = default;

// Session management
UnifiedAudioEngine::Result<SessionId> UnifiedAudioEngine::createSession(float sampleRate) {
    return pimpl->createSession(sampleRate);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::destroySession(SessionId sessionId) {
    return pimpl->destroySession(sessionId);
}

std::vector<SessionId> UnifiedAudioEngine::getActiveSessions() const {
    return pimpl->getActiveSessions();
}

// Master call management
UnifiedAudioEngine::Status UnifiedAudioEngine::loadMasterCall(SessionId sessionId,
                                                              std::string_view masterCallId) {
    return pimpl->loadMasterCall(sessionId, masterCallId);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::unloadMasterCall(SessionId sessionId) {
    return pimpl->unloadMasterCall(sessionId);
}

UnifiedAudioEngine::Result<std::string> UnifiedAudioEngine::getCurrentMasterCall(
    SessionId sessionId) const {
    return pimpl->getCurrentMasterCall(sessionId);
}

// Audio processing
UnifiedAudioEngine::Status UnifiedAudioEngine::processAudioChunk(
    SessionId sessionId, std::span<const float> audioBuffer) {
    return pimpl->processAudioChunk(sessionId, audioBuffer);
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::getSimilarityScore(SessionId sessionId) {
    return pimpl->getSimilarityScore(sessionId);
}

UnifiedAudioEngine::Result<int> UnifiedAudioEngine::getFeatureCount(SessionId sessionId) const {
    return pimpl->getFeatureCount(sessionId);
}

// Session state
bool UnifiedAudioEngine::isSessionActive(SessionId sessionId) const {
    return pimpl->isSessionActive(sessionId);
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::getSessionDuration(
    SessionId sessionId) const {
    return pimpl->getSessionDuration(sessionId);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::resetSession(SessionId sessionId) {
    return pimpl->resetSession(sessionId);
}

// Recording
UnifiedAudioEngine::Status UnifiedAudioEngine::startRecording(SessionId sessionId) {
    return pimpl->startRecording(sessionId);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::stopRecording(SessionId sessionId) {
    return pimpl->stopRecording(sessionId);
}

UnifiedAudioEngine::Result<std::string> UnifiedAudioEngine::saveRecording(
    SessionId sessionId, std::string_view filename) {
    return pimpl->saveRecording(sessionId, filename);
}

bool UnifiedAudioEngine::isRecording(SessionId sessionId) const {
    return pimpl->isRecording(sessionId);
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::getRecordingLevel(SessionId sessionId) const {
    return pimpl->getRecordingLevel(sessionId);
}

UnifiedAudioEngine::Result<double> UnifiedAudioEngine::getRecordingDuration(
    SessionId sessionId) const {
    return pimpl->getRecordingDuration(sessionId);
}

// Audio Playback
UnifiedAudioEngine::Status UnifiedAudioEngine::playMasterCall(SessionId sessionId,
                                                              std::string_view masterCallId) {
    return pimpl->playMasterCall(sessionId, masterCallId);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::playRecording(SessionId sessionId,
                                                             std::string_view filename) {
    return pimpl->playRecording(sessionId, filename);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::stopPlayback(SessionId sessionId) {
    return pimpl->stopPlayback(sessionId);
}

bool UnifiedAudioEngine::isPlaying(SessionId sessionId) const {
    return pimpl->isPlaying(sessionId);
}

UnifiedAudioEngine::Result<double> UnifiedAudioEngine::getPlaybackPosition(
    SessionId sessionId) const {
    return pimpl->getPlaybackPosition(sessionId);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::setPlaybackVolume(SessionId sessionId,
                                                                 float volume) {
    return pimpl->setPlaybackVolume(sessionId, volume);
}

// Real-time Session Management
UnifiedAudioEngine::Result<SessionId> UnifiedAudioEngine::startRealtimeSession(float sampleRate,
                                                                               int bufferSize) {
    return pimpl->startRealtimeSession(sampleRate, bufferSize);
}

UnifiedAudioEngine::Status UnifiedAudioEngine::endRealtimeSession(SessionId sessionId) {
    return pimpl->endRealtimeSession(sessionId);
}

bool UnifiedAudioEngine::isRealtimeSession(SessionId sessionId) const {
    return pimpl->isRealtimeSession(sessionId);
}

// === Implementation Details ===

UnifiedAudioEngine::Result<SessionId> UnifiedAudioEngine::Impl::createSession(float sampleRate) {
    if (sampleRate <= 0) {
        return {INVALID_SESSION_ID, Status::INVALID_PARAMS};
    }

    std::unique_lock lock(sessionsMutex_);
    const SessionId sessionId = nextSessionId_++;

    try {
        sessions_[sessionId] = std::make_unique<SessionState>(sessionId, sampleRate);
        return {sessionId, Status::OK};
    } catch (const std::exception&) {
        return {INVALID_SESSION_ID, Status::OUT_OF_MEMORY};
    }
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::destroySession(SessionId sessionId) {
    std::unique_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return Status::SESSION_NOT_FOUND;
    }
    sessions_.erase(it);
    return Status::OK;
}

std::vector<SessionId> UnifiedAudioEngine::Impl::getActiveSessions() const {
    std::shared_lock lock(sessionsMutex_);
    std::vector<SessionId> result;
    result.reserve(sessions_.size());
    for (const auto& [id, session] : sessions_) {
        result.push_back(id);
    }
    return result;
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::loadMasterCall(SessionId sessionId,
                                                                    std::string_view masterCallId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    const std::string masterCallIdStr(masterCallId);

    // Try to load cached features first
    if (loadFeaturesFromFile(*session, masterCallIdStr) == Status::OK) {
        session->masterCallId = masterCallIdStr;
        return Status::OK;
    }

    // Load and process audio file
    const std::string audioFilePath = masterCallsPath_ + masterCallIdStr + ".wav";
    unsigned int channels, sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float* rawData = drwav_open_file_and_read_pcm_frames_f32(
        audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (!rawData) {
        return Status::FILE_NOT_FOUND;
    }
    DrWavRAII audioData(rawData);

    // Convert to mono if necessary
    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float sampleSum = 0;
            for (unsigned int j = 0; j < channels; ++j) {
                sampleSum += rawData[i * channels + j];
            }
            monoSamples[i] = sampleSum / static_cast<float>(channels);
        }
    } else {
        std::copy(rawData, rawData + totalPCMFrameCount, monoSamples.begin());
    }

    // Extract MFCC features
    auto featuresResult = session->mfccProcessor->extractFeaturesFromBuffer(monoSamples, 256);
    if (!featuresResult) {
        return Status::PROCESSING_ERROR;
    }

    session->masterCallFeatures = std::move(*featuresResult);
    session->masterCallId = masterCallIdStr;
    saveFeaturesToFile(*session, masterCallIdStr);

    return Status::OK;
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::processAudioChunk(
    SessionId sessionId, std::span<const float> audioBuffer) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    // Temporarily disable VAD filtering to debug similarity scoring
    // TODO: Re-enable with proper VAD configuration once scoring works
    session->currentSegmentBuffer.insert(session->currentSegmentBuffer.end(), audioBuffer.begin(),
                                         audioBuffer.end());
    extractMFCCFeatures(*session);

    return Status::OK;
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::Impl::getSimilarityScore(
    SessionId sessionId) {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0.0f, Status::SESSION_NOT_FOUND};

    if (session->masterCallFeatures.empty() || session->sessionFeatures.empty()) {
        return {0.0f, Status::INSUFFICIENT_DATA};
    }

    const float distance =
        DTWProcessor::calculateDistance(session->masterCallFeatures, session->sessionFeatures);
    const float score = 1.0f / (1.0f + distance);
    return {score, Status::OK};
}

void UnifiedAudioEngine::Impl::extractMFCCFeatures(SessionState& session) {
    if (!session.mfccProcessor || session.currentSegmentBuffer.empty()) {
        return;
    }

    const size_t frameSize = 512;
    const size_t hopSize = frameSize / 2;

    auto featuresResult =
        session.mfccProcessor->extractFeaturesFromBuffer(session.currentSegmentBuffer, hopSize);
    if (featuresResult) {
        session.sessionFeatures.insert(session.sessionFeatures.end(), featuresResult->begin(),
                                       featuresResult->end());
    }

    // Keep only overlap for continuity
    if (session.currentSegmentBuffer.size() > frameSize) {
        std::vector<float> overlap(session.currentSegmentBuffer.end() - frameSize,
                                   session.currentSegmentBuffer.end());
        session.currentSegmentBuffer = std::move(overlap);
    }
}

UnifiedAudioEngine::Impl::SessionState* UnifiedAudioEngine::Impl::getSession(SessionId sessionId) {
    std::shared_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    return (it != sessions_.end()) ? it->second.get() : nullptr;
}

const UnifiedAudioEngine::Impl::SessionState* UnifiedAudioEngine::Impl::getSession(
    SessionId sessionId) const {
    std::shared_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    return (it != sessions_.end()) ? it->second.get() : nullptr;
}

// Additional implementations for remaining methods...
UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::unloadMasterCall(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    session->masterCallFeatures.clear();
    session->masterCallId.clear();
    return Status::OK;
}

UnifiedAudioEngine::Result<std::string> UnifiedAudioEngine::Impl::getCurrentMasterCall(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {"", Status::SESSION_NOT_FOUND};
    return {session->masterCallId, Status::OK};
}

UnifiedAudioEngine::Result<int> UnifiedAudioEngine::Impl::getFeatureCount(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0, Status::SESSION_NOT_FOUND};
    return {static_cast<int>(session->sessionFeatures.size()), Status::OK};
}

bool UnifiedAudioEngine::Impl::isSessionActive(SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    return session != nullptr;
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::Impl::getSessionDuration(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0.0f, Status::SESSION_NOT_FOUND};

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - session->startTime);
    return {duration.count() / 1000.0f, Status::OK};
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::resetSession(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    session->currentSegmentBuffer.clear();
    session->sessionFeatures.clear();
    session->recordingBuffer.clear();
    session->isRecording = false;
    session->startTime = std::chrono::steady_clock::now();

    return Status::OK;
}

// Recording implementations
UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::startRecording(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioRecorder) return Status::INIT_FAILED;

    AudioRecorder::Config config;
    config.sampleRate = static_cast<int>(session->sampleRate);
    config.channels = 1;  // Mono for voice analysis
    config.bufferSize = session->isRealtimeSession ? session->realtimeBufferSize : 512;

    if (!session->audioRecorder->startRecording(config)) {
        return Status::PROCESSING_ERROR;
    }

    session->isRecording = true;
    session->recordingBuffer.clear();
    return Status::OK;
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::stopRecording(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioRecorder) return Status::INIT_FAILED;

    session->audioRecorder->stopRecording();
    session->isRecording = false;

    // Copy recorded data to session buffer
    session->recordingBuffer = session->audioRecorder->getRecordedData();

    return Status::OK;
}

UnifiedAudioEngine::Result<std::string> UnifiedAudioEngine::Impl::saveRecording(
    SessionId sessionId, std::string_view filename) {
    SessionState* session = getSession(sessionId);
    if (!session) return {"", Status::SESSION_NOT_FOUND};

    if (!session->audioRecorder) return {"", Status::INIT_FAILED};

    const std::string fullPath = recordingsPath_ + std::string(filename);

    // Use the AudioRecorder's save functionality
    if (!session->audioRecorder->saveToWav(fullPath)) {
        return {"", Status::PROCESSING_ERROR};
    }

    return {fullPath, Status::OK};
}

bool UnifiedAudioEngine::Impl::isRecording(SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return false;
    return session->isRecording && session->audioRecorder && session->audioRecorder->isRecording();
}

UnifiedAudioEngine::Result<float> UnifiedAudioEngine::Impl::getRecordingLevel(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0.0f, Status::SESSION_NOT_FOUND};

    if (!session->audioRecorder) return {0.0f, Status::INIT_FAILED};

    return {session->audioRecorder->getCurrentLevel(), Status::OK};
}

UnifiedAudioEngine::Result<double> UnifiedAudioEngine::Impl::getRecordingDuration(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0.0, Status::SESSION_NOT_FOUND};

    if (!session->audioRecorder) return {0.0, Status::INIT_FAILED};

    return {session->audioRecorder->getDuration(), Status::OK};
}

// Audio Playback implementations
UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::playMasterCall(SessionId sessionId,
                                                                    std::string_view masterCallId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioPlayer) return Status::INIT_FAILED;

    const std::string audioFilePath = masterCallsPath_ + std::string(masterCallId) + ".wav";

    if (!session->audioPlayer->loadFile(audioFilePath)) {
        return Status::FILE_NOT_FOUND;
    }

    if (!session->audioPlayer->play()) {
        return Status::PROCESSING_ERROR;
    }

    session->isPlaying = true;
    session->currentPlaybackFile = audioFilePath;
    return Status::OK;
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::playRecording(SessionId sessionId,
                                                                   std::string_view filename) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioPlayer) return Status::INIT_FAILED;

    const std::string fullPath = recordingsPath_ + std::string(filename);

    if (!session->audioPlayer->loadFile(fullPath)) {
        return Status::FILE_NOT_FOUND;
    }

    if (!session->audioPlayer->play()) {
        return Status::PROCESSING_ERROR;
    }

    session->isPlaying = true;
    session->currentPlaybackFile = fullPath;
    return Status::OK;
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::stopPlayback(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioPlayer) return Status::INIT_FAILED;

    session->audioPlayer->stop();
    session->isPlaying = false;
    session->currentPlaybackFile.clear();
    return Status::OK;
}

bool UnifiedAudioEngine::Impl::isPlaying(SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return false;
    return session->isPlaying && session->audioPlayer && session->audioPlayer->isPlaying();
}

UnifiedAudioEngine::Result<double> UnifiedAudioEngine::Impl::getPlaybackPosition(
    SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return {0.0, Status::SESSION_NOT_FOUND};

    if (!session->audioPlayer) return {0.0, Status::INIT_FAILED};

    return {session->audioPlayer->getCurrentPosition(), Status::OK};
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::setPlaybackVolume(SessionId sessionId,
                                                                       float volume) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->audioPlayer) return Status::INIT_FAILED;

    if (volume < 0.0f || volume > 1.0f) return Status::INVALID_PARAMS;

    session->audioPlayer->setVolume(volume);
    session->playbackVolume = volume;
    return Status::OK;
}

// Real-time Session Management implementations
UnifiedAudioEngine::Result<SessionId> UnifiedAudioEngine::Impl::startRealtimeSession(
    float sampleRate, int bufferSize) {
    if (sampleRate <= 0 || bufferSize <= 0) {
        return {INVALID_SESSION_ID, Status::INVALID_PARAMS};
    }

    std::unique_lock lock(sessionsMutex_);
    const SessionId sessionId = nextSessionId_++;

    try {
        auto session = std::make_unique<SessionState>(sessionId, sampleRate);
        session->isRealtimeSession = true;
        session->realtimeBufferSize = bufferSize;

        sessions_[sessionId] = std::move(session);
        return {sessionId, Status::OK};
    } catch (const std::exception&) {
        return {INVALID_SESSION_ID, Status::OUT_OF_MEMORY};
    }
}

UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::endRealtimeSession(SessionId sessionId) {
    SessionState* session = getSession(sessionId);
    if (!session) return Status::SESSION_NOT_FOUND;

    if (!session->isRealtimeSession) return Status::INVALID_PARAMS;

    // Stop any ongoing recording or playback
    if (session->isRecording) {
        stopRecording(sessionId);
    }
    if (session->isPlaying) {
        stopPlayback(sessionId);
    }

    // Destroy the session
    return destroySession(sessionId);
}

bool UnifiedAudioEngine::Impl::isRealtimeSession(SessionId sessionId) const {
    const SessionState* session = getSession(sessionId);
    if (!session) return false;
    return session->isRealtimeSession;
}

// Feature file I/O
UnifiedAudioEngine::Status UnifiedAudioEngine::Impl::loadFeaturesFromFile(
    SessionState& session, const std::string& masterCallId) {
    const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);
    if (!inFile) return Status::FILE_NOT_FOUND;

    uint32_t numFrames = 0, numCoeffs = 0;
    inFile.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
    inFile.read(reinterpret_cast<char*>(&numCoeffs), sizeof(numCoeffs));
    if (!inFile || numFrames == 0 || numCoeffs == 0) return Status::PROCESSING_ERROR;

    session.masterCallFeatures.assign(numFrames, std::vector<float>(numCoeffs));
    for (uint32_t i = 0; i < numFrames; ++i) {
        inFile.read(reinterpret_cast<char*>(session.masterCallFeatures[i].data()),
                    numCoeffs * sizeof(float));
    }
    return inFile.good() ? Status::OK : Status::PROCESSING_ERROR;
}

void UnifiedAudioEngine::Impl::saveFeaturesToFile(const SessionState& session,
                                                  const std::string& masterCallId) {
    const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ofstream outFile(featureFilePath, std::ios::binary);
    if (!outFile || session.masterCallFeatures.empty()) return;

    uint32_t numFrames = static_cast<uint32_t>(session.masterCallFeatures.size());
    uint32_t numCoeffs = static_cast<uint32_t>(session.masterCallFeatures[0].size());
    outFile.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
    outFile.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(numCoeffs));

    for (const auto& frame : session.masterCallFeatures) {
        outFile.write(reinterpret_cast<const char*>(frame.data()), frame.size() * sizeof(float));
    }
}

}  // namespace huntmaster
