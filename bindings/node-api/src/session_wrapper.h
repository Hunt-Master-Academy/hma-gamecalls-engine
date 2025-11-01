// [20251028-BINDINGS-011] Session management wrapper for Node-API
// Wraps UnifiedAudioEngine session lifecycle operations

#ifndef GAMECALLS_SESSION_WRAPPER_H
#define GAMECALLS_SESSION_WRAPPER_H

#include <map>
#include <memory>
#include <mutex>

#include <napi.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-012] Session state and metadata
struct SessionState {
    uint32_t sessionId;     // Wrapper-level session ID (increments globally)
    uint32_t cppSessionId;  // [20251102-FIX-009] C++ engine session ID (per-engine instance)
    std::shared_ptr<huntmaster::UnifiedAudioEngine> engine;
    std::string masterCallPath;
    float sampleRate;
    bool enhancedAnalysisEnabled;
    std::chrono::steady_clock::time_point createdAt;
};

// [20251102-FIX-008] Result structures for observable JS returns
struct DestroyResult {
    bool destroyed;
    int cppSessionsDestroyed;
    int activeWrappers;
};

struct SessionsInfo {
    int activeWrappers;
    uint32_t nextWrapperId;
};

// [20251028-BINDINGS-013] Thread-safe session management
class SessionWrapper {
  public:
    // Create new audio analysis session
    static uint32_t
    CreateSession(const std::string& masterCallPath, float sampleRate, bool enableEnhancedAnalysis);

    // Get session engine instance
    static std::shared_ptr<huntmaster::UnifiedAudioEngine> GetEngine(uint32_t sessionId);

    // [20251102-FIX-011] Get C++ session ID from wrapper session ID
    static uint32_t GetCppSessionId(uint32_t wrapperSessionId);

    // [20251029-BINDINGS-FIX-001] Get similarity score for session - FIXED: Use
    // RealtimeScoringResult from UnifiedAudioEngine
    static huntmaster::RealtimeScoringResult GetSimilarityScore(uint32_t sessionId);

    // [20251029-BINDINGS-FIX-002] Finalize session analysis - FIXED: Use EnhancedAnalysisSummary
    // instead of non-existent FinalAnalysis
    static huntmaster::UnifiedAudioEngine::EnhancedAnalysisSummary
    FinalizeSession(uint32_t sessionId);

    // [20251102-FIX-009] Destroy session and return detailed results for JS observability
    static DestroyResult DestroySession(uint32_t sessionId);

    // [20251102-FIX-010] Get active sessions info for debugging
    static SessionsInfo GetActiveSessionsInfo();

    // Check if session exists
    static bool SessionExists(uint32_t sessionId);

  private:
    static std::map<uint32_t, SessionState> sessions_;
    static std::mutex sessionsMutex_;
    static uint32_t nextSessionId_;
};

}  // namespace gamecalls_bindings

#endif  // GAMECALLS_SESSION_WRAPPER_H
