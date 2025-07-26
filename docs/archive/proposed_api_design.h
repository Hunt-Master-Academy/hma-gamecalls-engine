// Proposed improved API design
#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace huntmaster {

using SessionId = uint32_t;
using EngineId = uint32_t;

class HuntmasterAudioEngine {
   public:
    enum class EngineStatus {
        OK = 0,
        INVALID_PARAMS = -1,
        INVALID_SESSION = -2,
        SESSION_NOT_FOUND = -3,
        FILE_NOT_FOUND = -4,
        PROCESSING_ERROR = -5,
        INSUFFICIENT_DATA = -6,
        OUT_OF_MEMORY = -7
    };

    template <typename T>
    struct [[nodiscard]] Result {
        T value;
        EngineStatus status;
        bool isOk() const { return status == EngineStatus::OK; }
        operator bool() const { return isOk(); }
        EngineStatus error() const { return status; }
        T operator*() const { return value; }
    };

    // Engine lifecycle - instance-based, not singleton
    static Result<std::unique_ptr<HuntmasterAudioEngine>> create();
    ~HuntmasterAudioEngine();

    // Session management - all operations require valid sessionId
    [[nodiscard]] Result<SessionId> createSession(float sampleRate = 44100.0f,
                                                  int bufferSize = 4096);
    [[nodiscard]] EngineStatus destroySession(SessionId sessionId);
    [[nodiscard]] std::vector<SessionId> getActiveSessions() const;

    // Master call management - per session
    [[nodiscard]] EngineStatus loadMasterCall(SessionId sessionId, std::string_view masterCallId);
    [[nodiscard]] EngineStatus unloadMasterCall(SessionId sessionId);
    [[nodiscard]] Result<std::string> getCurrentMasterCall(SessionId sessionId) const;

    // Audio processing - always requires sessionId
    [[nodiscard]] EngineStatus processAudioChunk(SessionId sessionId,
                                                 std::span<const float> audioBuffer);
    [[nodiscard]] Result<float> getSimilarityScore(SessionId sessionId);
    [[nodiscard]] Result<int> getFeatureCount(SessionId sessionId) const;

    // Session state queries
    [[nodiscard]] bool isSessionActive(SessionId sessionId) const;
    [[nodiscard]] Result<float> getSessionDuration(SessionId sessionId) const;
    [[nodiscard]] EngineStatus resetSession(SessionId sessionId);

    // Recording (per session)
    [[nodiscard]] EngineStatus startRecording(SessionId sessionId);
    [[nodiscard]] EngineStatus stopRecording(SessionId sessionId);
    [[nodiscard]] Result<std::string> saveRecording(SessionId sessionId, std::string_view filename);

   private:
    HuntmasterAudioEngine();

    // Non-copyable, moveable
    HuntmasterAudioEngine(const HuntmasterAudioEngine&) = delete;
    HuntmasterAudioEngine& operator=(const HuntmasterAudioEngine&) = delete;
    HuntmasterAudioEngine(HuntmasterAudioEngine&&) = default;
    HuntmasterAudioEngine& operator=(HuntmasterAudioEngine&&) = default;

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

// Engine manager for multiple engine instances (useful for complex apps)
class EngineManager {
   public:
    static EngineManager& getInstance();

    [[nodiscard]] Result<EngineId> createEngine();
    [[nodiscard]] EngineStatus destroyEngine(EngineId engineId);
    [[nodiscard]] HuntmasterAudioEngine* getEngine(EngineId engineId);

   private:
    EngineManager() = default;
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace huntmaster

// Clean C API that matches the C++ design
extern "C" {
// Engine management
[[nodiscard]] int hm_create_engine();
void hm_destroy_engine(int engineId);

// Session management
[[nodiscard]] int hm_create_session(int engineId, float sampleRate, int bufferSize);
[[nodiscard]] int hm_destroy_session(int engineId, int sessionId);

// Master calls
[[nodiscard]] int hm_load_master_call(int engineId, int sessionId, const char* masterCallId);

// Audio processing
[[nodiscard]] int hm_process_audio_chunk(int engineId, int sessionId, const float* audioBuffer,
                                         int bufferSize);
[[nodiscard]] float hm_get_similarity_score(int engineId, int sessionId);
[[nodiscard]] int hm_get_feature_count(int engineId, int sessionId);
}
