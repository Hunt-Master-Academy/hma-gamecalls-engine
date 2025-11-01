// [20251028-BINDINGS-002] Main Node-API addon entry point for GameCalls Engine
// Exposes UnifiedAudioEngine functionality to Node.js/JavaScript

#include <napi.h>

#include "audio_processor.h"
#include "session_wrapper.h"
#include "type_converters.h"

namespace gamecalls_bindings {

// [20251028-BINDINGS-003] Engine initialization and cleanup
Napi::Value InitializeEngine(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        // UnifiedAudioEngine is a singleton, initialize once
        // Actual initialization happens in SessionWrapper::CreateSession
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Engine initialization failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251028-BINDINGS-004] Create new audio analysis session
// Args: masterCallPath (string), options (object)
// Returns: sessionId (number)
Napi::Value CreateSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected masterCallPath as string").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string masterCallPath = info[0].As<Napi::String>().Utf8Value();

    // Parse optional options object
    float sampleRate = 44100.0f;
    bool enableEnhancedAnalysis = true;

    if (info.Length() >= 2 && info[1].IsObject()) {
        Napi::Object options = info[1].As<Napi::Object>();

        if (options.Has("sampleRate")) {
            sampleRate = options.Get("sampleRate").As<Napi::Number>().FloatValue();
        }
        if (options.Has("enableEnhancedAnalysis")) {
            enableEnhancedAnalysis =
                options.Get("enableEnhancedAnalysis").As<Napi::Boolean>().Value();
        }
    }

    try {
        uint32_t sessionId =
            SessionWrapper::CreateSession(masterCallPath, sampleRate, enableEnhancedAnalysis);
        return Napi::Number::New(env, sessionId);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to create session: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251028-BINDINGS-005] Process audio buffer for a session
// Args: sessionId (number), audioBuffer (Float32Array), sampleRate (number)
// Returns: analysis results (object)
Napi::Value ProcessAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsTypedArray()) {
        Napi::TypeError::New(env, "Expected sessionId (number) and audioBuffer (Float32Array)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    uint32_t sessionId = info[0].As<Napi::Number>().Uint32Value();
    Napi::Float32Array audioArray = info[1].As<Napi::Float32Array>();

    try {
        auto results = AudioProcessor::ProcessBuffer(sessionId, audioArray);
        return TypeConverters::AnalysisResultsToObject(env, results);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Audio processing failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251028-BINDINGS-006] Get current similarity score for session
// Args: sessionId (number)
// Returns: { score: number, confidence: number, readiness: string }
Napi::Value GetSimilarityScore(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected sessionId as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    uint32_t sessionId = info[0].As<Napi::Number>().Uint32Value();

    try {
        auto score = SessionWrapper::GetSimilarityScore(sessionId);
        return TypeConverters::SimilarityScoreToObject(env, score);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to get similarity score: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251029-BINDINGS-FIX-019] Finalize session analysis (segment selection, refined DTW) - FIXED
// call Args: sessionId (number) Returns: EnhancedAnalysisSummary object with comprehensive results
Napi::Value FinalizeSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected sessionId as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    uint32_t sessionId = info[0].As<Napi::Number>().Uint32Value();

    try {
        auto summary = SessionWrapper::FinalizeSession(sessionId);
        return TypeConverters::EnhancedAnalysisSummaryToObject(env, summary);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Session finalization failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251028-BINDINGS-008] Destroy session and free resources
// Args: sessionId (number)
// [20251102-FIX-006] Returns: { destroyed: boolean, wrapperId: number, activeSessions: number }
Napi::Value DestroySession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected sessionId as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    uint32_t sessionId = info[0].As<Napi::Number>().Uint32Value();

    try {
        auto result = SessionWrapper::DestroySession(sessionId);

        Napi::Object resultObj = Napi::Object::New(env);
        resultObj.Set("destroyed", Napi::Boolean::New(env, result.destroyed));
        resultObj.Set("wrapperId", Napi::Number::New(env, sessionId));
        resultObj.Set("cppSessionsDestroyed", Napi::Number::New(env, result.cppSessionsDestroyed));
        resultObj.Set("activeWrappers", Napi::Number::New(env, result.activeWrappers));

        return resultObj;
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to destroy session: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// [20251029-BINDINGS-FIX-020] Get engine version and build info - FIXED CMAKE_BUILD_TYPE
Napi::Value GetEngineInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Object infoObj = Napi::Object::New(env);
    infoObj.Set("version", "1.0.0");
// [20251029-BINDINGS-FIX-021] Use preprocessor check for DEBUG instead of CMAKE_BUILD_TYPE
// CMAKE variables don't exist in node-gyp build context
#ifdef DEBUG
    infoObj.Set("buildType", "Debug");
#else
    infoObj.Set("buildType", "Release");
#endif
    infoObj.Set("cppStandard", "C++20");
    infoObj.Set("apiVersion", NAPI_VERSION);

    return infoObj;
}

// [20251102-FIX-007] Get active sessions count for debugging
Napi::Value GetActiveSessionsInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto sessionInfo = SessionWrapper::GetActiveSessionsInfo();

    Napi::Object infoObj = Napi::Object::New(env);
    infoObj.Set("activeWrappers", Napi::Number::New(env, sessionInfo.activeWrappers));
    infoObj.Set("nextWrapperId", Napi::Number::New(env, sessionInfo.nextWrapperId));

    return infoObj;
}

// [20251028-BINDINGS-010] Module initialization - register all exports
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Core session management
    exports.Set("initializeEngine", Napi::Function::New(env, InitializeEngine));
    exports.Set("createSession", Napi::Function::New(env, CreateSession));
    exports.Set("destroySession", Napi::Function::New(env, DestroySession));

    // Audio processing
    exports.Set("processAudio", Napi::Function::New(env, ProcessAudio));
    exports.Set("getSimilarityScore", Napi::Function::New(env, GetSimilarityScore));
    exports.Set("finalizeSession", Napi::Function::New(env, FinalizeSession));

    // Utility
    exports.Set("getEngineInfo", Napi::Function::New(env, GetEngineInfo));
    exports.Set("getActiveSessionsInfo", Napi::Function::New(env, GetActiveSessionsInfo));

    return exports;
}

}  // namespace gamecalls_bindings

// [20251029-BINDINGS-FIX-022] Global scope wrapper for NODE_API_MODULE compatibility
// NODE_API_MODULE macro cannot handle namespace qualifiers directly
Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
    return gamecalls_bindings::Init(env, exports);
}

NODE_API_MODULE(gamecalls_engine, InitModule)
