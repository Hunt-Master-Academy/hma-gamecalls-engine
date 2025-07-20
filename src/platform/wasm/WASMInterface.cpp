#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "huntmaster/factories/UnifiedAudioEngineFactory.h"
#include "huntmaster/platform/wasm/WASMInterface.h"

using namespace huntmaster;

namespace huntmaster {
namespace wasm {

WASMEngine::WASMEngine() : engine(nullptr) {
    // Initialize as needed
}

bool WASMEngine::initialize(int sampleRate, int bufferSize) {
    try {
        // Create engine using factory
        auto result = UnifiedAudioEngineFactory::createEngine();
        if (result.status != Status::SUCCESS) {
            return false;
        }

        engine = std::move(result.engine);

        // Configure engine
        AudioConfig config;
        config.sampleRate = sampleRate;
        config.bufferSize = bufferSize;
        config.channels = 1;  // Mono for now

        auto configStatus = engine->configureAudio(config);
        return configStatus == Status::SUCCESS;

    } catch (const std::exception&) {
        return false;
    }
}

bool WASMEngine::loadMasterCall(const std::string& audioPath) {
    if (!engine) return false;

    try {
        auto status = engine->loadMasterCall(audioPath);
        return status == Status::SUCCESS;
    } catch (const std::exception&) {
        return false;
    }
}

bool WASMEngine::startSession(const std::string& newSessionId) {
    if (!engine) return false;

    try {
        auto status = engine->startRealtimeSession(newSessionId);
        if (status == Status::SUCCESS) {
            sessionId = newSessionId;
            return true;
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool WASMEngine::endSession() {
    if (!engine || sessionId.empty()) return false;

    try {
        auto status = engine->endRealtimeSession(sessionId);
        if (status == Status::SUCCESS) {
            sessionId.clear();
            return true;
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool WASMEngine::processAudio(const emscripten::val& audioData) {
    if (!engine || sessionId.empty()) return false;

    try {
        // Convert JavaScript array to vector
        std::vector<float> audioVector;
        const int length = audioData["length"].as<int>();
        audioVector.reserve(length);

        for (int i = 0; i < length; ++i) {
            audioVector.push_back(audioData[i].as<float>());
        }

        auto status =
            engine->processRealtimeAudio(sessionId, audioVector.data(), audioVector.size());
        return status == Status::SUCCESS;

    } catch (const std::exception&) {
        return false;
    }
}

emscripten::val WASMEngine::getWaveformData(int startTime, int duration) {
    if (!engine || sessionId.empty()) {
        return emscripten::val::array();
    }

    try {
        auto result = engine->getWaveformRange(sessionId, startTime, duration);
        if (result.status != Status::SUCCESS) {
            return emscripten::val::array();
        }

        // Convert vector to JavaScript array
        emscripten::val jsArray = emscripten::val::array();
        for (size_t i = 0; i < result.waveform.size(); ++i) {
            jsArray.call<void>("push", result.waveform[i]);
        }

        return jsArray;
    } catch (const std::exception&) {
        return emscripten::val::array();
    }
}

emscripten::val WASMEngine::exportWaveformToJSON() {
    if (!engine || sessionId.empty()) {
        return emscripten::val::null();
    }

    try {
        auto result = engine->exportWaveformToJson(sessionId);
        if (result.status != Status::SUCCESS) {
            return emscripten::val::null();
        }

        return emscripten::val(result.jsonData);
    } catch (const std::exception&) {
        return emscripten::val::null();
    }
}

emscripten::val WASMEngine::getCurrentSimilarity() {
    if (!engine || sessionId.empty()) {
        return emscripten::val::object();
    }

    try {
        auto result = engine->getCurrentSimilarity(sessionId);
        if (result.status != Status::SUCCESS) {
            return emscripten::val::object();
        }

        emscripten::val obj = emscripten::val::object();
        obj.set("similarity", result.similarity);
        obj.set("confidence", result.confidence);
        return obj;

    } catch (const std::exception&) {
        return emscripten::val::object();
    }
}

emscripten::val WASMEngine::getRecentFeatures() {
    if (!engine || sessionId.empty()) {
        return emscripten::val::array();
    }

    try {
        auto result = engine->getRecentFeatures(sessionId, 10);  // Last 10 features
        if (result.status != Status::SUCCESS) {
            return emscripten::val::array();
        }

        emscripten::val jsArray = emscripten::val::array();
        for (const auto& feature : result.features) {
            emscripten::val featureArray = emscripten::val::array();
            for (float coeff : feature) {
                featureArray.call<void>("push", coeff);
            }
            jsArray.call<void>("push", featureArray);
        }

        return jsArray;
    } catch (const std::exception&) {
        return emscripten::val::array();
    }
}

std::string WASMEngine::getSessionId() const { return sessionId; }

emscripten::val WASMEngine::getEngineStatus() {
    emscripten::val status = emscripten::val::object();

    status.set("initialized", engine != nullptr);
    status.set("hasSession", !sessionId.empty());
    status.set("sessionId", sessionId);

    if (engine) {
        try {
            auto engineStatus = engine->getEngineStatus();
            status.set("hasMasterCall", engineStatus.hasMasterCall);
            status.set("isProcessing", engineStatus.isProcessing);
            status.set("activeSessionCount", engineStatus.activeSessionCount);
        } catch (const std::exception&) {
            status.set("error", "Failed to get engine status");
        }
    }

    return status;
}

}  // namespace wasm
}  // namespace huntmaster

#endif  // __EMSCRIPTEN__
