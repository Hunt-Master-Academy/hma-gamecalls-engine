// [20251028-BINDINGS-014] Session wrapper implementation
// Thread-safe session lifecycle management

#include "session_wrapper.h"

#include <fstream>
#include <stdexcept>

namespace gamecalls_bindings {

// Static member initialization
std::map<uint32_t, SessionState> SessionWrapper::sessions_;
std::mutex SessionWrapper::sessionsMutex_;
uint32_t SessionWrapper::nextSessionId_ = 1;

// [20251028-BINDINGS-015] Create new analysis session
uint32_t SessionWrapper::CreateSession(const std::string& masterCallPath,
                                       float sampleRate,
                                       bool enableEnhancedAnalysis) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    // Verify master call file exists
    std::ifstream masterFile(masterCallPath, std::ios::binary);
    if (!masterFile.good()) {
        throw std::runtime_error("Master call file not found: " + masterCallPath);
    }

    // Create UnifiedAudioEngine instance
    auto engineResult = huntmaster::UnifiedAudioEngine::create();
    if (!engineResult.ok) {
        throw std::runtime_error("Failed to create UnifiedAudioEngine: "
                                 + std::string(engineResult.error));
    }

    auto engine = std::move(engineResult.value);

    // Create session in engine
    auto sessionResult = engine->createSession(sampleRate);
    if (sessionResult.status != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Failed to create engine session");
    }

    // Load master call
    auto loadStatus = engine->loadMasterCall(sessionResult.value, masterCallPath);
    if (loadStatus != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Failed to load master call: " + masterCallPath);
    }

    // Enable enhanced analysis if requested
    if (enableEnhancedAnalysis) {
        engine->setEnhancedAnalysisEnabled(sessionResult.value, true);
    }

    // Store session state
    uint32_t sessionId = nextSessionId_++;
    SessionState state;
    state.sessionId = sessionId;
    state.engine = std::move(engine);
    state.masterCallPath = masterCallPath;
    state.sampleRate = sampleRate;
    state.enhancedAnalysisEnabled = enableEnhancedAnalysis;
    state.createdAt = std::chrono::steady_clock::now();

    sessions_[sessionId] = std::move(state);

    return sessionId;
}

// [20251028-BINDINGS-016] Get engine instance for session
std::shared_ptr<huntmaster::UnifiedAudioEngine> SessionWrapper::GetEngine(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        throw std::runtime_error("Session not found: " + std::to_string(sessionId));
    }

    return it->second.engine;
}

// [20251028-BINDINGS-017] Get current similarity score
huntmaster::SimilarityScore SessionWrapper::GetSimilarityScore(uint32_t sessionId) {
    auto engine = GetEngine(sessionId);

    // Get latest similarity metrics from engine
    auto state = engine->getSimilarityRealtimeState(sessionId);

    huntmaster::SimilarityScore score;
    score.score = state.currentScore;
    score.confidence = state.confidence;
    score.readiness = state.readiness;

    return score;
}

// [20251028-BINDINGS-018] Finalize session analysis
huntmaster::FinalAnalysis SessionWrapper::FinalizeSession(uint32_t sessionId) {
    auto engine = GetEngine(sessionId);

    // Run finalization process (segment selection, refined DTW)
    auto finalizeResult = engine->finalizeSessionAnalysis(sessionId);
    if (finalizeResult.status != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Session finalization failed");
    }

    return finalizeResult.analysis;
}

// [20251028-BINDINGS-019] Destroy session and cleanup
void SessionWrapper::DestroySession(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        throw std::runtime_error("Session not found: " + std::to_string(sessionId));
    }

    // Engine cleanup happens automatically via shared_ptr destruction
    sessions_.erase(it);
}

// [20251028-BINDINGS-020] Check if session exists
bool SessionWrapper::SessionExists(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    return sessions_.find(sessionId) != sessions_.end();
}

}  // namespace gamecalls_bindings
