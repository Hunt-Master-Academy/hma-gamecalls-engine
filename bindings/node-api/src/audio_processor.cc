// [20251028-BINDINGS-024] Audio processor implementation
// Realtime audio buffer processing and analysis

#include "audio_processor.h"

#include <iostream>  // [20251101-V1.0-FIX] For std::cerr diagnostics
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

    // [20251101-V1.0-SAFETY] Add diagnostic info for chunk size validation
    const size_t chunkSize = audioData.size();
    constexpr size_t WARN_THRESHOLD = 5'000'000;  // Warn on >5M samples (~2 min @ 44.1kHz)

    if (chunkSize > WARN_THRESHOLD) {
        std::cerr << "[WARN] Large audio chunk: " << chunkSize << " samples, session=" << sessionId
                  << std::endl;
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
        // [20251101-V1.0-SAFETY] Enhanced error diagnostics
        std::string errorMsg = "Audio processing failed: " + statusStr
                               + " (session=" + std::to_string(sessionId)
                               + ", cppSessionId=" + std::to_string(cppSessionId)
                               + ", samples=" + std::to_string(audioData.size()) + ")";
        std::cerr << "[ERROR] " << errorMsg << std::endl;
        throw std::runtime_error(errorMsg);
    }

    // Gather analysis results
    AnalysisResults results{};  // [20251101-V1.0-FIX] Zero-initialize all fields

    // [20251029-BINDINGS-FIX-024] Get similarity score via RealtimeFeedback - FIXED: use
    // getRealtimeFeedback
    // [20251101-V1.0-FIX] Use cppSessionId instead of wrapper sessionId
    auto feedbackResult = engine->getRealtimeFeedback(cppSessionId);
    if (feedbackResult.isOk()) {
        results.similarityScore = feedbackResult.value.currentScore.overall;
        results.confidence = feedbackResult.value.currentScore.confidence;
        results.readiness = feedbackResult.value.currentScore.isReliable ? "ready" : "not_ready";
    } else {
        // [20251101-V1.0-DEBUG] Log why getRealtimeFeedback failed
        std::cerr << "[ERROR] getRealtimeFeedback failed for session " << sessionId
                  << " (cppSessionId=" << cppSessionId << ")" << std::endl;
        results.similarityScore = 0.0f;
        results.confidence = 0.0f;
        results.readiness = "error";
    }

    // [20251029-BINDINGS-FIX-025] Get enhanced analyzer results - FIXED: access Result.value fields
    // directly
    if (SessionWrapper::SessionExists(sessionId)) {
        // [20251101-V1.0-FIX] Use cppSessionId for engine calls
        auto summaryResult = engine->getEnhancedAnalysisSummary(cppSessionId);
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
