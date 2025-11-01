// [20251028-BINDINGS-030] Type converters for C++ to JavaScript objects
// Converts engine results to Node.js/JavaScript representations

#ifndef GAMECALLS_TYPE_CONVERTERS_H
#define GAMECALLS_TYPE_CONVERTERS_H

#include <napi.h>

#include "audio_processor.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-031] Type conversion utilities
class TypeConverters {
  public:
    // Convert AnalysisResults to JavaScript object
    static Napi::Object AnalysisResultsToObject(Napi::Env env, const AnalysisResults& results);

    // [20251029-BINDINGS-FIX-003] Convert RealtimeScoringResult to JavaScript object - FIXED: Use
    // type from UnifiedAudioEngine
    static Napi::Object SimilarityScoreToObject(Napi::Env env,
                                                const huntmaster::RealtimeScoringResult& score);

    // [20251029-BINDINGS-FIX-004] Convert EnhancedAnalysisSummary to JavaScript object - FIXED: Use
    // actual type from engine
    static Napi::Object EnhancedAnalysisSummaryToObject(
        Napi::Env env, const huntmaster::UnifiedAudioEngine::EnhancedAnalysisSummary& summary);
};

}  // namespace gamecalls_bindings

#endif  // GAMECALLS_TYPE_CONVERTERS_H
