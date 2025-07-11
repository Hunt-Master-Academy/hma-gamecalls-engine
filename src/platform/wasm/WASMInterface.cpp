// File: WASMInterface.cpp
#ifdef __EMSCRIPTEN__

#include "huntmaster/platform/wasm/WASMInterface.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/HuntmasterEngine.h"
#include "huntmaster/core/AudioBufferPool.h"
#include "huntmaster/core/VoiceActivityDetector.h"
#include "huntmaster/core/RealtimeAudioProcessor.h"
#include <emscripten.h>
#include <emscripten/threading.h>
#include <atomic>
#include <chrono>
#include <unordered_set>

namespace huntmaster
{

    namespace
    {
        // Helper to convert JS typed arrays to C++ spans
        template <typename T>
        std::span<const T> typedArrayToSpan(emscripten::val array)
        {
            const size_t length = array["length"].as<size_t>();
            const T *ptr = reinterpret_cast<const T *>(
                array["buffer"].as<uintptr_t>() +
                array["byteOffset"].as<size_t>());
            return std::span<const T>(ptr, length);
        }
    }

    class WASMInterface::Impl
    {
    public:
        std::unique_ptr<HuntmasterEngine> engine_;
        std::unique_ptr<RealtimeAudioProcessor> processor_;
        std::atomic<bool> streaming_enabled_{false};
        std::atomic<size_t> memory_usage_{0};
        std::chrono::steady_clock::time_point start_time_;

        // Session tracking
        std::atomic<int> next_session_id_{1};
        std::unordered_set<int> active_sessions_;
        std::mutex sessions_mutex_;

        Impl() : start_time_(std::chrono::steady_clock::now()) {}

        bool initializeEngine(int sampleRate, int frameSize, int mfccCoeffs)
        {
            try
            {
                PlatformEngineConfig config{
                    .sample_rate = static_cast<size_t>(sampleRate),
                    .frame_size = static_cast<size_t>(frameSize),
                    .mfcc_coefficients = static_cast<size_t>(mfccCoeffs),
                    .max_concurrent_sessions = 5, // Limited for WASM
                    .buffer_pool_size = 16        // Smaller for WASM
                };

                engine_ = std::make_unique<HuntmasterEngine>(config);

                RealtimeAudioProcessor::Config proc_config{
                    .ring_buffer_size = 256, // Smaller for WASM
                    .chunk_size = static_cast<size_t>(frameSize),
                    .enable_backpressure = false, // No blocking in WASM
                    .enable_metrics = true};

                processor_ = std::make_unique<RealtimeAudioProcessor>(proc_config);

                updateMemoryUsage();
                return engine_->isInitialized();
            }
            catch (const std::exception &e)
            {
                EM_ASM_({ console.error('Engine init failed:', UTF8ToString($0)); }, e.what());
                return false;
            }
        }

        void updateMemoryUsage()
        {
            // Estimate memory usage
            size_t usage = sizeof(*this);
            if (engine_)
                usage += sizeof(HuntmasterEngine) + 1024 * 1024; // 1MB estimate
            if (processor_)
                usage += sizeof(RealtimeAudioProcessor) + 256 * 2048 * 4;
            memory_usage_.store(usage, std::memory_order_relaxed);
        }
    };

    WASMInterface::WASMInterface() : pimpl_(std::make_unique<Impl>()) {}

    WASMInterface::~WASMInterface() = default;

    bool WASMInterface::initialize(int sampleRate, int frameSize, int mfccCoeffs)
    {
        return pimpl_->initializeEngine(sampleRate, frameSize, mfccCoeffs);
    }

    void WASMInterface::shutdown()
    {
        pimpl_->engine_.reset();
        pimpl_->processor_.reset();
        pimpl_->memory_usage_.store(0, std::memory_order_relaxed);
    }

    bool WASMInterface::isInitialized() const
    {
        return pimpl_->engine_ && pimpl_->engine_->isInitialized();
    }

    bool WASMInterface::loadMasterCall(const std::string &callName,
                                       emscripten::val audioData)
    {
        if (!pimpl_->engine_)
            return false;

        try
        {
            // Convert JS Float32Array to C++ vector
            auto span = typedArrayToSpan<float>(audioData);

            // TODO: Process audio data to extract MFCC features
            // For now, just load the call name
            auto result = pimpl_->engine_->loadMasterCall(callName);
            return result.has_value();
        }
        catch (const std::exception &e)
        {
            EM_ASM_({ console.error('Load master call failed:', UTF8ToString($0)); }, e.what());
            return false;
        }
    }

    float WASMInterface::processAudioChunk(uintptr_t audioPtr, size_t numSamples)
    {
        if (!pimpl_->engine_)
            return 0.0f;

        // Direct memory access for performance
        std::span<const float> audioData(
            reinterpret_cast<const float *>(audioPtr),
            numSamples);

        auto result = pimpl_->engine_->processChunk(audioData);
        if (result)
        {
            return result->similarity_score;
        }

        return 0.0f;
    }

    emscripten::val WASMInterface::processAudioArray(emscripten::val audioArray)
    {
        using namespace emscripten;

        if (!pimpl_->engine_)
        {
            return val::object();
        }

        try
        {
            auto audioSpan = typedArrayToSpan<float>(audioArray);

            auto startTime = std::chrono::high_resolution_clock::now();
            auto result = pimpl_->engine_->processChunk(audioSpan);
            auto endTime = std::chrono::high_resolution_clock::now();

            val response = val::object();

            if (result)
            {
                response.set("success", true);
                response.set("score", result->similarity_score);
                response.set("framesProcessed", static_cast<int>(result->frames_processed));
                response.set("processingTimeMs",
                             std::chrono::duration<float, std::milli>(endTime - startTime).count());
            }
            else
            {
                response.set("success", false);
                response.set("error", "Processing failed");
            }

            return response;
        }
        catch (const std::exception &e)
        {
            val response = val::object();
            response.set("success", false);
            response.set("error", std::string(e.what()));
            return response;
        }
    }

    int WASMInterface::startSession()
    {
        if (!pimpl_->engine_)
            return -1;

        int sessionId = pimpl_->next_session_id_.fetch_add(1, std::memory_order_relaxed);

        auto result = pimpl_->engine_->startSession(sessionId);
        if (result)
        {
            std::lock_guard lock(pimpl_->sessions_mutex_);
            pimpl_->active_sessions_.insert(sessionId);
            return sessionId;
        }

        return -1;
    }

    bool WASMInterface::endSession(int sessionId)
    {
        if (!pimpl_->engine_)
            return false;

        auto result = pimpl_->engine_->endSession(sessionId);
        if (result)
        {
            std::lock_guard lock(pimpl_->sessions_mutex_);
            pimpl_->active_sessions_.erase(sessionId);
            return true;
        }

        return false;
    }

    int WASMInterface::getActiveSessionCount() const
    {
        if (!pimpl_->engine_)
            return 0;
        return static_cast<int>(pimpl_->engine_->getActiveSessionCount());
    }

    bool WASMInterface::enableStreaming(bool enable)
    {
        pimpl_->streaming_enabled_.store(enable, std::memory_order_relaxed);
        return true;
    }

    bool WASMInterface::enqueueAudioBuffer(emscripten::val buffer)
    {
        if (!pimpl_->processor_ || !pimpl_->streaming_enabled_.load())
        {
            return false;
        }

        try
        {
            auto audioSpan = typedArrayToSpan<float>(buffer);
            return pimpl_->processor_->tryEnqueueAudio(audioSpan);
        }
        catch (...)
        {
            return false;
        }
    }

    emscripten::val WASMInterface::dequeueResults()
    {
        using namespace emscripten;

        val results = val::array();

        if (!pimpl_->processor_ || !pimpl_->streaming_enabled_.load())
        {
            return results;
        }

        // Dequeue up to 10 chunks at once
        auto chunks = pimpl_->processor_->dequeueBatch(10);

        for (const auto &chunk : chunks)
        {
            val chunkObj = val::object();
            chunkObj.set("frameIndex", static_cast<int>(chunk.frame_index));
            chunkObj.set("energyLevel", chunk.energy_level);
            chunkObj.set("containsVoice", chunk.contains_voice);
            chunkObj.set("samples", static_cast<int>(chunk.valid_samples));

            results.call<void>("push", chunkObj);
        }

        return results;
    }

    emscripten::val WASMInterface::getPerformanceStats() const
    {
        using namespace emscripten;

        val stats = val::object();

        if (pimpl_->processor_)
        {
            auto procStats = pimpl_->processor_->getStats();

            val processorStats = val::object();
            processorStats.set("chunksProcessed", static_cast<int>(procStats.total_chunks_processed));
            processorStats.set("chunksDropped", static_cast<int>(procStats.chunks_dropped));
            processorStats.set("bufferOverruns", static_cast<int>(procStats.buffer_overruns));
            processorStats.set("bufferUnderruns", static_cast<int>(procStats.buffer_underruns));
            processorStats.set("avgLatencyMs", procStats.average_latency_ms);
            processorStats.set("currentBufferUsage", static_cast<int>(procStats.current_buffer_usage));

            stats.set("processor", processorStats);
        }

        stats.set("memoryUsageMB", pimpl_->memory_usage_.load() / (1024.0 * 1024.0));
        stats.set("activeSessionCount", getActiveSessionCount());

        auto uptime = std::chrono::steady_clock::now() - pimpl_->start_time_;
        stats.set("uptimeSeconds",
                  std::chrono::duration<float>(uptime).count());

        return stats;
    }

    void WASMInterface::resetStats()
    {
        if (pimpl_->processor_)
        {
            pimpl_->processor_->resetStats();
        }
    }

    void WASMInterface::onMemoryPressure()
    {
        // Clear caches and reduce memory usage
        EM_ASM({ console.warn('Memory pressure detected, clearing caches'); });

        // TODO: Clear MFCC cache, reduce buffer sizes, etc.
    }

    size_t WASMInterface::getMemoryUsage() const
    {
        return pimpl_->memory_usage_.load(std::memory_order_relaxed);
    }

    // SharedArrayBuffer support for Web Workers
    class WASMAudioWorker::Impl
    {
        std::atomic<float> *shared_buffer_{nullptr};
        size_t buffer_size_{0};
        std::atomic<bool> processing_{false};

    public:
        bool initialize(uintptr_t ptr, size_t size)
        {
            if (size % sizeof(float) != 0)
                return false;

            shared_buffer_ = reinterpret_cast<std::atomic<float> *>(ptr);
            buffer_size_ = size / sizeof(float);

            return true;
        }

        void process()
        {
            if (!shared_buffer_ || processing_.exchange(true))
                return;

            // Process audio in shared buffer
            for (size_t i = 0; i < buffer_size_; ++i)
            {
                float sample = shared_buffer_[i].load(std::memory_order_relaxed);
                // Apply simple gain as example
                sample *= 0.9f;
                shared_buffer_[i].store(sample, std::memory_order_relaxed);
            }

            processing_.store(false, std::memory_order_release);
        }

        emscripten::val getStatus() const
        {
            emscripten::val status = emscripten::val::object();
            status.set("initialized", shared_buffer_ != nullptr);
            status.set("processing", processing_.load(std::memory_order_relaxed));
            status.set("bufferSize", static_cast<int>(buffer_size_));
            return status;
        }
    };

    WASMAudioWorker::WASMAudioWorker() : pimpl_(std::make_unique<Impl>()) {}

    bool WASMAudioWorker::initialize(uintptr_t sharedBufferPtr, size_t bufferSize)
    {
        return pimpl_->initialize(sharedBufferPtr, bufferSize);
    }

    void WASMAudioWorker::processSharedBuffer()
    {
        pimpl_->process();
    }

    emscripten::val WASMAudioWorker::getStatus() const
    {
        return pimpl_->getStatus();
    }

    // Emscripten bindings
    EMSCRIPTEN_BINDINGS(huntmaster_audio_engine)
    {
        using namespace emscripten;

        class_<WASMInterface>("HuntmasterEngine")
            .constructor<>()
            .function("initialize", &WASMInterface::initialize)
            .function("shutdown", &WASMInterface::shutdown)
            .function("isInitialized", &WASMInterface::isInitialized)
            .function("loadMasterCall", &WASMInterface::loadMasterCall)
            .function("processAudioChunk", &WASMInterface::processAudioChunk)
            .function("processAudioArray", &WASMInterface::processAudioArray)
            .function("startSession", &WASMInterface::startSession)
            .function("endSession", &WASMInterface::endSession)
            .function("getActiveSessionCount", &WASMInterface::getActiveSessionCount)
            .function("enableStreaming", &WASMInterface::enableStreaming)
            .function("enqueueAudioBuffer", &WASMInterface::enqueueAudioBuffer)
            .function("dequeueResults", &WASMInterface::dequeueResults)
            .function("getPerformanceStats", &WASMInterface::getPerformanceStats)
            .function("resetStats", &WASMInterface::resetStats)
            .function("onMemoryPressure", &WASMInterface::onMemoryPressure)
            .function("getMemoryUsage", &WASMInterface::getMemoryUsage);

        class_<WASMAudioWorker>("HuntmasterAudioWorker")
            .constructor<>()
            .function("initialize", &WASMAudioWorker::initialize)
            .function("processSharedBuffer", &WASMAudioWorker::processSharedBuffer)
            .function("getStatus", &WASMAudioWorker::getStatus);
    }

} // namespace huntmaster

#endif // __EMSCRIPTEN__