// [20251028-BINDINGS-014] Session wrapper implementation
// Thread-safe session lifecycle management

#include "session_wrapper.h"

#include <fstream>
#include <iostream>
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

    // [20251229-BINDINGS-FIX-008] Pass master call ID directly to engine
    // Engine will construct full path: masterCallsPath_ + masterCallId + ".wav"
    // No need to verify file exists here - engine handles that internally

    // Create UnifiedAudioEngine instance
    auto engineResult = huntmaster::UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        throw std::runtime_error("Failed to create UnifiedAudioEngine");
    }

    auto engine = std::move(engineResult.value);

    // Create session in engine
    auto sessionResult = engine->createSession(sampleRate);
    if (sessionResult.status != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Failed to create engine session");
    }

    uint32_t cppSessionId = sessionResult.value;
    std::cerr << "🎯 Created C++ session ID: " << cppSessionId << " in engine instance"
              << std::endl;

    // Load master call
    auto loadStatus = engine->loadMasterCall(cppSessionId, masterCallPath);
    if (loadStatus != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Failed to load master call: " + masterCallPath);
    }

    // Enable enhanced analysis if requested
    if (enableEnhancedAnalysis) {
        engine->setEnhancedAnalyzersEnabled(cppSessionId, true);
    }

    // Store session state
    uint32_t sessionId = nextSessionId_++;
    SessionState state;
    state.sessionId = sessionId;
    state.cppSessionId = cppSessionId;  // [20251102-FIX-010] Store actual C++ engine session ID
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

// [20251102-FIX-012] Get C++ session ID from wrapper session ID
uint32_t SessionWrapper::GetCppSessionId(uint32_t wrapperSessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    auto it = sessions_.find(wrapperSessionId);
    if (it == sessions_.end()) {
        throw std::runtime_error("Wrapper session not found: " + std::to_string(wrapperSessionId));
    }

    return it->second.cppSessionId;
}

// [20251029-BINDINGS-FIX-005] Get current similarity score - FIXED implementation
// Uses getRealtimeFeedback to retrieve current RealtimeScoringResult
huntmaster::RealtimeScoringResult SessionWrapper::GetSimilarityScore(uint32_t sessionId) {
    auto engine = GetEngine(sessionId);
    uint32_t cppSessionId = GetCppSessionId(sessionId);  // [20251102-FIX-015] Use C++ session ID

    // [20251029-BINDINGS-FIX-006] Get real-time feedback from engine (contains currentScore)
    auto feedbackResult = engine->getRealtimeFeedback(cppSessionId);

    if (!feedbackResult.isOk()) {
        throw std::runtime_error("Failed to get real-time feedback");
    }

    // Return the current score from feedback (RealtimeScoringResult type)
    return feedbackResult.value.currentScore;
}

// [20251029-BINDINGS-FIX-007] Finalize session analysis - FIXED implementation
// Triggers finalizeSessionAnalysis and returns EnhancedAnalysisSummary
huntmaster::UnifiedAudioEngine::EnhancedAnalysisSummary
SessionWrapper::FinalizeSession(uint32_t sessionId) {
    auto engine = GetEngine(sessionId);
    uint32_t cppSessionId = GetCppSessionId(sessionId);  // [20251102-FIX-016] Use C++ session ID

    // [20251029-BINDINGS-FIX-008] Trigger finalization (idempotent - OK if already finalized)
    auto status = engine->finalizeSessionAnalysis(cppSessionId);

    if (status != huntmaster::UnifiedAudioEngine::Status::OK
        && status != huntmaster::UnifiedAudioEngine::Status::ALREADY_FINALIZED) {
        throw std::runtime_error("Session finalization failed");
    }

    // [20251029-BINDINGS-FIX-009] Retrieve comprehensive analysis results
    auto summaryResult = engine->getEnhancedAnalysisSummary(cppSessionId);

    if (!summaryResult.isOk()) {
        throw std::runtime_error("Failed to get enhanced analysis summary");
    }

    return summaryResult.value;
}

// [20251028-BINDINGS-019] Destroy session and cleanup
// [20251102-FIX-009] Destroy session with detailed return for JS observability
DestroyResult SessionWrapper::DestroySession(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        throw std::runtime_error("Session not found: " + std::to_string(sessionId));
    }

    // [20251102-FIX-005] CRITICAL: Destroy C++ engine session before removing wrapper
    // Each SessionWrapper owns a UnifiedAudioEngine instance, which contains internal sessions.
    // We need to explicitly destroy the internal C++ session to trigger cleanup:
    // - ErrorLogger.clearRecentErrors()
    // - SessionState resource release
    // - Remove from engine's sessions_ map
    int cppSessionsDestroyed = 0;
    std::cerr << "🗑️  DestroySession wrapper=" << sessionId << std::endl;
    try {
        // Get all active sessions in this engine instance (should be just 1)
        auto activeSessions = it->second.engine->getActiveSessions();
        std::cerr << "   Active C++ sessions in this engine: " << activeSessions.size()
                  << std::endl;
        for (const auto& engineSessionId : activeSessions) {
            std::cerr << "   Destroying C++ session: " << engineSessionId << std::endl;
            auto status = it->second.engine->destroySession(engineSessionId);
            if (status != huntmaster::UnifiedAudioEngine::Status::OK) {
                std::cerr << "   ⚠️  Warning: Failed to destroy engine session " << engineSessionId
                          << std::endl;
            } else {
                std::cerr << "   ✅ Destroyed C++ session: " << engineSessionId << std::endl;
                cppSessionsDestroyed++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "   ⚠️  Warning: Exception during engine session cleanup: " << e.what()
                  << std::endl;
    }

    // Now remove wrapper session (shared_ptr destructor will cleanup engine instance)
    sessions_.erase(it);

    DestroyResult result;
    result.destroyed = true;
    result.cppSessionsDestroyed = cppSessionsDestroyed;
    result.activeWrappers = sessions_.size();

    return result;
}

// [20251102-FIX-010] Get active sessions info
SessionsInfo SessionWrapper::GetActiveSessionsInfo() {
    std::lock_guard<std::mutex> lock(sessionsMutex_);

    SessionsInfo info;
    info.activeWrappers = sessions_.size();
    info.nextWrapperId = nextSessionId_;

    return info;
}

// [20251028-BINDINGS-020] Check if session exists
bool SessionWrapper::SessionExists(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    return sessions_.find(sessionId) != sessions_.end();
}

}  // namespace gamecalls_bindings
