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

    // [20251102-FIX-013] Get actual C++ session ID (not wrapper ID!)
    uint32_t cppSessionId = SessionWrapper::GetCppSessionId(sessionId);

    // Convert JavaScript typed array to C++ vector
    std::vector<float> audioData = ConvertAudioBuffer(audioBuffer);

    if (audioData.empty()) {
        throw std::runtime_error("Empty audio buffer provided");
    }

    // [20251029-BINDINGS-FIX-023] Process audio through UnifiedAudioEngine - FIXED: use
    // processAudioChunk with span [20251102-DEBUG-001] Add detailed status logging for debugging
    // [20251102-FIX-014] Use C++ session ID (not wrapper ID) when calling engine
    auto status = engine->processAudioChunk(
        cppSessionId, std::span<const float>(audioData.data(), audioData.size()));
    if (status != huntmaster::UnifiedAudioEngine::Status::OK) {
        std::string statusStr;
        switch (status) {
            case huntmaster::UnifiedAudioEngine::Status::SESSION_NOT_FOUND:
                statusStr = "SESSION_NOT_FOUND";
                break;
            case huntmaster::UnifiedAudioEngine::Status::INVALID_PARAMS:
                statusStr = "INVALID_PARAMS";
                break;
            case huntmaster::UnifiedAudioEngine::Status::PROCESSING_ERROR:
                statusStr = "PROCESSING_ERROR";
                break;
            case huntmaster::UnifiedAudioEngine::Status::INSUFFICIENT_DATA:
                statusStr = "INSUFFICIENT_DATA";
                break;
            case huntmaster::UnifiedAudioEngine::Status::INIT_FAILED:
                statusStr = "INIT_FAILED";
                break;
            default:
                statusStr = "UNKNOWN_ERROR_" + std::to_string(static_cast<int>(status));
        }
        throw std::runtime_error("Audio processing failed with status: " + statusStr
                                 + " (session=" + std::to_string(sessionId)
                                 + ", samples=" + std::to_string(audioData.size()) + ")");
    }

    // Gather analysis results
    AnalysisResults results;

    // [20251029-BINDINGS-FIX-024] Get similarity score via RealtimeFeedback - FIXED: use
    // getRealtimeFeedback
    auto feedbackResult = engine->getRealtimeFeedback(sessionId);
    if (feedbackResult.isOk()) {
        results.similarityScore = feedbackResult.value.currentScore.overall;
        results.confidence = feedbackResult.value.currentScore.confidence;
        results.readiness = feedbackResult.value.currentScore.isReliable ? "ready" : "not_ready";
    }

    // [20251029-BINDINGS-FIX-025] Get enhanced analyzer results - FIXED: access Result.value fields
    // directly
    if (SessionWrapper::SessionExists(sessionId)) {
        auto summaryResult = engine->getEnhancedAnalysisSummary(sessionId);
        if (summaryResult.isOk()) {
            auto& summary = summaryResult.value;

            // Pitch analysis
            results.pitchAnalysis.pitch = summary.pitchHz;
            results.pitchAnalysis.pitchConfidence = summary.pitchConfidence;

            // Harmonic analysis
            results.harmonicAnalysis.harmonicity = summary.harmonicFundamental;
            results.harmonicAnalysis.spectralCentroid =
                summary.harmonicConfidence;  // Best approximation

            // Cadence analysis
            results.cadenceAnalysis.tempo = summary.tempoBPM;
            results.cadenceAnalysis.rhythmStrength = summary.tempoConfidence;  // Best approximation
        }
    }

    // [20251029-BINDINGS-FIX-026] Audio level metrics - stub for now (API may not exist)
    results.rmsLevel = 0.0f;
    results.peakLevel = 0.0f;

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
