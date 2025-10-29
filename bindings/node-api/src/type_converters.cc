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

// [20251028-BINDINGS-034] Convert similarity score to JS object
Napi::Object TypeConverters::SimilarityScoreToObject(Napi::Env env,
                                                     const huntmaster::SimilarityScore& score) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set("score", Napi::Number::New(env, score.score));
    obj.Set("confidence", Napi::Number::New(env, score.confidence));
    obj.Set("readiness", Napi::String::New(env, score.readiness));

    return obj;
}

// [20251028-BINDINGS-035] Convert final analysis to JS object
Napi::Object TypeConverters::FinalAnalysisToObject(Napi::Env env,
                                                   const huntmaster::FinalAnalysis& analysis) {
    Napi::Object obj = Napi::Object::New(env);

    // Overall scores
    obj.Set("overallScore", Napi::Number::New(env, analysis.overallScore));
    obj.Set("similarityScore", Napi::Number::New(env, analysis.similarityScore));
    obj.Set("confidence", Napi::Number::New(env, analysis.confidence));

    // Segment information
    Napi::Object segment = Napi::Object::New(env);
    segment.Set("startMs", Napi::Number::New(env, analysis.segment.startMs));
    segment.Set("endMs", Napi::Number::New(env, analysis.segment.endMs));
    segment.Set("durationMs", Napi::Number::New(env, analysis.segment.durationMs));
    obj.Set("segment", segment);

    // Enhanced analysis summary
    Napi::Object enhanced = Napi::Object::New(env);

    // Pitch
    Napi::Object pitch = Napi::Object::New(env);
    pitch.Set("mean", Napi::Number::New(env, analysis.enhanced.pitch.mean));
    pitch.Set("std", Napi::Number::New(env, analysis.enhanced.pitch.std));
    pitch.Set("confidence", Napi::Number::New(env, analysis.enhanced.pitch.confidence));
    enhanced.Set("pitch", pitch);

    // Harmonic
    Napi::Object harmonic = Napi::Object::New(env);
    harmonic.Set("harmonicity", Napi::Number::New(env, analysis.enhanced.harmonic.harmonicity));
    harmonic.Set("spectralCentroid",
                 Napi::Number::New(env, analysis.enhanced.harmonic.spectralCentroid));
    enhanced.Set("harmonic", harmonic);

    // Cadence
    Napi::Object cadence = Napi::Object::New(env);
    cadence.Set("tempo", Napi::Number::New(env, analysis.enhanced.cadence.tempo));
    cadence.Set("rhythmStrength", Napi::Number::New(env, analysis.enhanced.cadence.rhythmStrength));
    enhanced.Set("cadence", cadence);

    // Loudness
    Napi::Object loudness = Napi::Object::New(env);
    loudness.Set("normalizationScalar",
                 Napi::Number::New(env, analysis.enhanced.loudness.normalizationScalar));
    loudness.Set("loudnessDeviation",
                 Napi::Number::New(env, analysis.enhanced.loudness.loudnessDeviation));
    enhanced.Set("loudness", loudness);

    obj.Set("enhanced", enhanced);

    // Timing
    obj.Set("processingTimeMs", Napi::Number::New(env, analysis.processingTimeMs));
    obj.Set("timestamp", Napi::Number::New(env, analysis.timestamp));

    return obj;
}

}  // namespace gamecalls_bindings
