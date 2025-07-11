#pragma once

#ifdef __EMSCRIPTEN__

#include "huntmaster/core/HuntmasterEngine.h"
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <vector>

namespace huntmaster
{

    class WASMInterface
    {
    public:
        WASMInterface();
        ~WASMInterface();

        // Engine lifecycle
        bool initialize(int sampleRate, int frameSize, int mfccCoeffs);
        void shutdown();
        bool isInitialized() const;

        // Master call management
        bool loadMasterCall(const std::string &callName,
                            emscripten::val audioData);
        std::vector<std::string> getAvailableCalls() const;

        // Audio processing
        float processAudioChunk(uintptr_t audioPtr, size_t numSamples);
        emscripten::val processAudioArray(emscripten::val audioArray);

        // Session management
        int startSession();
        bool endSession(int sessionId);
        int getActiveSessionCount() const;

        // Real-time streaming
        bool enableStreaming(bool enable);
        bool enqueueAudioBuffer(emscripten::val buffer);
        emscripten::val dequeueResults();

        // Performance metrics
        emscripten::val getPerformanceStats() const;
        void resetStats();

        // Memory pressure handling
        void onMemoryPressure();
        size_t getMemoryUsage() const;

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    // SharedArrayBuffer support for audio workers
    class WASMAudioWorker
    {
    public:
        WASMAudioWorker();

        bool initialize(uintptr_t sharedBufferPtr, size_t bufferSize);
        void processSharedBuffer();
        emscripten::val getStatus() const;

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;
    };

} // namespace huntmaster

#endif // __EMSCRIPTEN__