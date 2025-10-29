// [20251028-BINDINGS-024] Audio processor implementation
// Realtime audio buffer processing and analysis

#include "audio_processor.h"

#include <stdexcept>

#include "session_wrapper.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-025] Process audio buffer through engine
AnalysisResults AudioProcessor::ProcessBuffer(uint32_t sessionId,
                                              const Napi::Float32Array& audioBuffer) {
    // Get engine instance for session
    auto engine = SessionWrapper::GetEngine(sessionId);
    if (!engine) {
        throw std::runtime_error("Invalid session ID: " + std::to_string(sessionId));
    }

    // Convert JavaScript typed array to C++ vector
    std::vector<float> audioData = ConvertAudioBuffer(audioBuffer);

    if (audioData.empty()) {
        throw std::runtime_error("Empty audio buffer provided");
    }

    // Process audio through UnifiedAudioEngine
    auto status = engine->processAudioBuffer(sessionId, audioData.data(), audioData.size());
    if (status != huntmaster::UnifiedAudioEngine::Status::OK) {
        throw std::runtime_error("Audio processing failed in engine");
    }

    // Gather analysis results
    AnalysisResults results;

    // [20251028-BINDINGS-026] Get similarity score and confidence
    auto similarityState = engine->getSimilarityRealtimeState(sessionId);
    results.similarityScore = similarityState.currentScore;
    results.confidence = similarityState.confidence;
    results.readiness =
        similarityState.readiness == huntmaster::ReadinessLevel::Ready ? "ready" : "not_ready";

    // [20251028-BINDINGS-027] Get enhanced analyzer results
    if (SessionWrapper::SessionExists(sessionId)) {
        auto enhancedSummary = engine->getEnhancedAnalysisSummary(sessionId);

        // Pitch analysis
        results.pitchAnalysis.pitch = enhancedSummary.pitch.mean;
        results.pitchAnalysis.pitchConfidence = enhancedSummary.pitch.confidence;

        // Harmonic analysis
        results.harmonicAnalysis.harmonicity = enhancedSummary.harmonic.harmonicity;
        results.harmonicAnalysis.spectralCentroid = enhancedSummary.harmonic.spectralCentroid;

        // Cadence analysis
        results.cadenceAnalysis.tempo = enhancedSummary.cadence.tempo;
        results.cadenceAnalysis.rhythmStrength = enhancedSummary.cadence.rhythmStrength;
    }

    // [20251028-BINDINGS-028] Get audio level metrics
    auto levels = engine->getAudioLevels(sessionId);
    results.rmsLevel = levels.rms;
    results.peakLevel = levels.peak;

    // Add timestamp
    results.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count()
                        / 1000.0;

    return results;
}

// [20251028-BINDINGS-029] Convert Float32Array to std::vector<float>
std::vector<float> AudioProcessor::ConvertAudioBuffer(const Napi::Float32Array& buffer) {
    size_t length = buffer.ElementLength();
    std::vector<float> result(length);

    for (size_t i = 0; i < length; ++i) {
        result[i] = buffer[i];
    }

    return result;
}

}  // namespace gamecalls_bindings
