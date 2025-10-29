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
    uint32_t sessionId;
    std::shared_ptr<huntmaster::UnifiedAudioEngine> engine;
    std::string masterCallPath;
    float sampleRate;
    bool enhancedAnalysisEnabled;
    std::chrono::steady_clock::time_point createdAt;
};

// [20251028-BINDINGS-013] Thread-safe session management
class SessionWrapper {
  public:
    // Create new audio analysis session
    static uint32_t
    CreateSession(const std::string& masterCallPath, float sampleRate, bool enableEnhancedAnalysis);

    // Get session engine instance
    static std::shared_ptr<huntmaster::UnifiedAudioEngine> GetEngine(uint32_t sessionId);

    // Get similarity score for session
    static huntmaster::SimilarityScore GetSimilarityScore(uint32_t sessionId);

    // Finalize session analysis
    static huntmaster::FinalAnalysis FinalizeSession(uint32_t sessionId);

    // Destroy session and cleanup resources
    static void DestroySession(uint32_t sessionId);

    // Check if session exists
    static bool SessionExists(uint32_t sessionId);

  private:
    static std::map<uint32_t, SessionState> sessions_;
    static std::mutex sessionsMutex_;
    static uint32_t nextSessionId_;
};

}  // namespace gamecalls_bindings

#endif  // GAMECALLS_SESSION_WRAPPER_H
