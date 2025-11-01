// [20251028-BINDINGS-032] Type converter implementations
// C++ struct/class to JavaScript object conversions

#include "type_converters.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-033] Convert realtime analysis results to JS object
Napi::Object TypeConverters::AnalysisResultsToObject(Napi::Env env,
                                                     const AnalysisResults& results) {
    Napi::Object obj = Napi::Object::New(env);

    // Similarity metrics
    obj.Set("similarityScore", Napi::Number::New(env, results.similarityScore));
    obj.Set("confidence", Napi::Number::New(env, results.confidence));
    obj.Set("readiness", Napi::String::New(env, results.readiness));

    // Pitch analysis
    Napi::Object pitch = Napi::Object::New(env);
    pitch.Set("pitch", Napi::Number::New(env, results.pitchAnalysis.pitch));
    pitch.Set("confidence", Napi::Number::New(env, results.pitchAnalysis.pitchConfidence));
    obj.Set("pitch", pitch);

    // Harmonic analysis
    Napi::Object harmonic = Napi::Object::New(env);
    harmonic.Set("harmonicity", Napi::Number::New(env, results.harmonicAnalysis.harmonicity));
    harmonic.Set("spectralCentroid",
                 Napi::Number::New(env, results.harmonicAnalysis.spectralCentroid));
    obj.Set("harmonic", harmonic);

    // Cadence analysis
    Napi::Object cadence = Napi::Object::New(env);
    cadence.Set("tempo", Napi::Number::New(env, results.cadenceAnalysis.tempo));
    cadence.Set("rhythmStrength", Napi::Number::New(env, results.cadenceAnalysis.rhythmStrength));
    obj.Set("cadence", cadence);

    // Audio levels
    Napi::Object levels = Napi::Object::New(env);
    levels.Set("rms", Napi::Number::New(env, results.rmsLevel));
    levels.Set("peak", Napi::Number::New(env, results.peakLevel));
    obj.Set("levels", levels);

    // Timestamp
    obj.Set("timestamp", Napi::Number::New(env, results.timestamp));

    return obj;
}

// [20251029-BINDINGS-FIX-010] Convert RealtimeScoringResult to JS object - FIXED implementation
// Maps all fields from the actual C++ struct to JavaScript
Napi::Object
TypeConverters::SimilarityScoreToObject(Napi::Env env,
                                        const huntmaster::RealtimeScoringResult& score) {
    Napi::Object obj = Napi::Object::New(env);

    // [20251029-BINDINGS-FIX-011] Multi-dimensional similarity scores
    obj.Set("overall", Napi::Number::New(env, score.overall));
    obj.Set("mfcc", Napi::Number::New(env, score.mfcc));
    obj.Set("volume", Napi::Number::New(env, score.volume));
    obj.Set("timing", Napi::Number::New(env, score.timing));
    obj.Set("pitch", Napi::Number::New(env, score.pitch));
    obj.Set("confidence", Napi::Number::New(env, score.confidence));

    // [20251029-BINDINGS-FIX-012] Boolean flags and metadata
    obj.Set("isReliable", Napi::Boolean::New(env, score.isReliable));
    obj.Set("isMatch", Napi::Boolean::New(env, score.isMatch));
    obj.Set("samplesAnalyzed", Napi::Number::New(env, static_cast<double>(score.samplesAnalyzed)));

    return obj;
}

// [20251029-BINDINGS-FIX-013] Convert EnhancedAnalysisSummary to JS object - FIXED implementation
// Maps comprehensive analysis results including pitch, harmonic, cadence, and finalize metrics
Napi::Object TypeConverters::EnhancedAnalysisSummaryToObject(
    Napi::Env env, const huntmaster::UnifiedAudioEngine::EnhancedAnalysisSummary& summary) {
    Napi::Object obj = Napi::Object::New(env);

    // [20251029-BINDINGS-FIX-014] Pitch analysis results
    Napi::Object pitch = Napi::Object::New(env);
    pitch.Set("pitchHz", Napi::Number::New(env, summary.pitchHz));
    pitch.Set("confidence", Napi::Number::New(env, summary.pitchConfidence));
    pitch.Set("grade", Napi::String::New(env, std::string(1, summary.pitchGrade)));
    obj.Set("pitch", pitch);

    // [20251029-BINDINGS-FIX-015] Harmonic analysis results
    Napi::Object harmonic = Napi::Object::New(env);
    harmonic.Set("fundamental", Napi::Number::New(env, summary.harmonicFundamental));
    harmonic.Set("confidence", Napi::Number::New(env, summary.harmonicConfidence));
    harmonic.Set("grade", Napi::String::New(env, std::string(1, summary.harmonicGrade)));
    obj.Set("harmonic", harmonic);

    // [20251029-BINDINGS-FIX-016] Cadence/tempo analysis results
    Napi::Object cadence = Napi::Object::New(env);
    cadence.Set("tempoBPM", Napi::Number::New(env, summary.tempoBPM));
    cadence.Set("confidence", Napi::Number::New(env, summary.tempoConfidence));
    cadence.Set("grade", Napi::String::New(env, std::string(1, summary.cadenceGrade)));
    obj.Set("cadence", cadence);

    // [20251029-BINDINGS-FIX-017] Finalize-stage metrics (populated after finalizeSessionAnalysis)
    Napi::Object finalize = Napi::Object::New(env);
    finalize.Set("similarityAtFinalize", Napi::Number::New(env, summary.similarityAtFinalize));
    finalize.Set("normalizationScalar", Napi::Number::New(env, summary.normalizationScalar));
    finalize.Set("loudnessDeviation", Napi::Number::New(env, summary.loudnessDeviation));
    finalize.Set("segmentStartMs",
                 Napi::Number::New(env, static_cast<double>(summary.segmentStartMs)));
    finalize.Set("segmentDurationMs",
                 Napi::Number::New(env, static_cast<double>(summary.segmentDurationMs)));
    obj.Set("finalize", finalize);

    // [20251029-BINDINGS-FIX-018] Metadata flags
    obj.Set("valid", Napi::Boolean::New(env, summary.valid));
    obj.Set("finalized", Napi::Boolean::New(env, summary.finalized));

    return obj;
}

}  // namespace gamecalls_bindings
