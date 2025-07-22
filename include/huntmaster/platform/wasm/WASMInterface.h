#pragma once

#ifdef __EMSCRIPTEN__

#include <memory>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

namespace wasm {
class WASMEngine {
  private:
    std::unique_ptr<UnifiedAudioEngine> engine;
    std::string sessionId;

  public:
    WASMEngine();
    ~WASMEngine() = default;

    // Engine lifecycle
    bool initialize(int sampleRate = 44100, int bufferSize = 1024);
    bool loadMasterCall(const std::string& audioPath);
    bool startSession(const std::string& newSessionId);
    bool endSession();

    // Real-time processing
    bool processAudio(const emscripten::val& audioData);
    emscripten::val getWaveformData(int startTime, int duration);
    emscripten::val exportWaveformToJSON();

    // Analysis
    emscripten::val getCurrentSimilarity();
    emscripten::val getRecentFeatures();

    // Utilities
    std::string getSessionId() const;
    emscripten::val getEngineStatus();
};

}  // namespace wasm
WASMAudioWorker();

}  // namespace huntmaster

// Emscripten bindings
EMSCRIPTEN_BINDINGS(huntmaster_wasm) {
    emscripten::class_<huntmaster::wasm::WASMEngine>("HuntmasterEngine")
        .constructor<>()
        .function("initialize", &huntmaster::wasm::WASMEngine::initialize)
        .function("loadMasterCall", &huntmaster::wasm::WASMEngine::loadMasterCall)
        .function("startSession", &huntmaster::wasm::WASMEngine::startSession)
        .function("endSession", &huntmaster::wasm::WASMEngine::endSession)
        .function("processAudio", &huntmaster::wasm::WASMEngine::processAudio)
        .function("getWaveformData", &huntmaster::wasm::WASMEngine::getWaveformData)
        .function("exportWaveformToJSON", &huntmaster::wasm::WASMEngine::exportWaveformToJSON)
        .function("getCurrentSimilarity", &huntmaster::wasm::WASMEngine::getCurrentSimilarity)
        .function("getRecentFeatures", &huntmaster::wasm::WASMEngine::getRecentFeatures)
        .function("getSessionId", &huntmaster::wasm::WASMEngine::getSessionId)
        .function("getEngineStatus", &huntmaster::wasm::WASMEngine::getEngineStatus);
}

#endif  // __EMSCRIPTEN__