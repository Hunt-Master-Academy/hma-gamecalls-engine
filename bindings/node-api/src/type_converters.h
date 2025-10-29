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

    // Convert SimilarityScore to JavaScript object
    static Napi::Object SimilarityScoreToObject(Napi::Env env,
                                                const huntmaster::SimilarityScore& score);

    // Convert FinalAnalysis to JavaScript object
    static Napi::Object FinalAnalysisToObject(Napi::Env env,
                                              const huntmaster::FinalAnalysis& analysis);
};

}  // namespace gamecalls_bindings

#endif  // GAMECALLS_TYPE_CONVERTERS_H
