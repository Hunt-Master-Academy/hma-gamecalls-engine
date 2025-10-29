// [20251028-BINDINGS-021] Audio processing wrapper for Node-API
// Handles realtime audio buffer processing

#ifndef GAMECALLS_AUDIO_PROCESSOR_H
#define GAMECALLS_AUDIO_PROCESSOR_H

#include <vector>

#include <napi.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-022] Realtime analysis results structure
struct AnalysisResults {
    float similarityScore;
    float confidence;
    std::string readiness;

    // Enhanced analyzer results
    struct {
        float pitch;
        float pitchConfidence;
    } pitchAnalysis;

    struct {
        float harmonicity;
        float spectralCentroid;
    } harmonicAnalysis;

    struct {
        float tempo;
        float rhythmStrength;
    } cadenceAnalysis;

    // Audio levels
    float rmsLevel;
    float peakLevel;

    // Timestamp
    double timestamp;
};

// [20251028-BINDINGS-023] Audio buffer processing
class AudioProcessor {
  public:
    // Process audio buffer for session
    static AnalysisResults ProcessBuffer(uint32_t sessionId, const Napi::Float32Array& audioBuffer);

  private:
    // Convert Float32Array to C++ vector
    static std::vector<float> ConvertAudioBuffer(const Napi::Float32Array& buffer);
};

}  // namespace gamecalls_bindings

#endif  // GAMECALLS_AUDIO_PROCESSOR_H
