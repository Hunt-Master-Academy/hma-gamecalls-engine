/**
 * @file OptimizedAudioRecorder.h
 * @brief Optimized audio recorder with high-performance I/O and memory-based recording
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "huntmaster/core/OptimizedAudioIO.h"
#include "miniaudio.h"

namespace huntmaster {

namespace io {
// Forward declaration for optional IO optimizer integration
class MasterIOOptimizer;
}  // namespace io

/**
 * @brief High-performance audio recorder with optimized I/O and multiple recording modes
 */
class OptimizedAudioRecorder {
  public:
    enum class RecordingMode {
        FILE_BASED,    ///< Record directly to file with async I/O
        MEMORY_BASED,  ///< Record to memory buffer for later processing
        HYBRID         ///< Record to both memory and file simultaneously
    };

    struct Config {
        uint32_t sampleRate = 44100;
        uint16_t channels = 1;
        uint16_t bitsPerSample = 32;

        // Recording mode
        RecordingMode recordingMode = RecordingMode::FILE_BASED;
        std::string outputFile;

        // Buffer settings
        size_t recordingBufferFrames = 16384;
        size_t streamingBufferFrames = 8192;
        size_t bufferDurationMs = 100;  // Buffer duration in milliseconds
        size_t chunkSize = 4096;        // Chunk size for processing

        // Memory-based recording settings
        size_t memoryBufferMaxFrames = 48000 * 60 * 5;  // 5 minutes at 48kHz
        bool enableMemoryGrowth = true;
        size_t memoryGrowthIncrement = 48000 * 10;  // 10 seconds at 48kHz
        bool enableMemoryCompression = false;

        // I/O optimization
        bool enableAsyncWrite = true;
        bool enableBufferedStreaming = true;
        size_t asyncWriteBufferSize = 2 * 1024 * 1024;  // 2MB

        // Processing settings
        bool enableChunkProcessing = false;
        bool enableRealTimeProcessing = false;
        bool enableLevelMonitoring = true;
        bool enableVAD = false;

        // Quality settings
        bool enableAutoGain = false;
        bool enableNoiseReduction = false;
        float targetLevel = 0.7f;
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit OptimizedAudioRecorder(const Config& config);
    ~OptimizedAudioRecorder();

    /**
     * @brief Start recording with optimized I/O
     */
    // Low-level controls implemented in the .cpp
    bool start();
    void stop();

    // Backwards-compatible wrappers for higher-level API usage
    bool startRecording() {
        return start();
    }
    bool stopRecording() {
        stop();
        return true;
    }

    /**
     * @brief Stop recording and finalize file
     */
    /**
     * @brief Check if currently recording
     */
    bool isRecording() const;

    /**
     * @brief Set output filename for recording
     */
    void setOutputFile(const std::string& filename);

    /**
     * @brief Set recording mode
     */
    void setRecordingMode(RecordingMode mode);

    /**
     * @brief Get current recording mode
     */
    RecordingMode getRecordingMode() const;

    /**
     * @brief Get recorded audio data from memory buffer
     * @return Vector containing recorded samples (interleaved if multi-channel)
     */
    std::vector<float> getRecordedData() const;

    /**
     * @brief Get recorded audio data as raw pointer with size
     * @param[out] size Number of samples available
     * @return Pointer to recorded audio data (const, do not modify)
     */
    const float* getRecordedDataPtr(size_t& size) const;

    /**
     * @brief Copy recorded data to external buffer
     * @param buffer Destination buffer (must be large enough)
     * @param maxSamples Maximum samples to copy
     * @return Number of samples actually copied
     */
    size_t copyRecordedData(float* buffer, size_t maxSamples) const;

    /**
     * @brief Save memory-recorded data to file
     * @param filename Output file path
     * @param format Audio format (default: WAV)
     * @return True if save was successful
     */
    bool saveMemoryToFile(const std::string& filename, const std::string& format = "wav") const;

    /**
     * @brief Clear memory buffer (for memory-based recording)
     */
    void clearMemoryBuffer();

    /**
     * @brief Get memory buffer usage information
     */
    struct MemoryBufferInfo {
        size_t totalCapacityFrames;
        size_t usedFrames;
        size_t freeFrames;
        double usagePercentage;
        size_t memorySizeBytes;
        bool isGrowthEnabled;
        bool hasOverflowed;
    };
    MemoryBufferInfo getMemoryBufferInfo() const;

    /**
     * @brief Get current recording level (0.0 to 1.0)
     */
    float getCurrentLevel() const;

    /**
     * @brief Get recording duration in seconds
     */
    double getDuration() const;

    /**
     * @brief Get recorded sample count
     */
    size_t getSampleCount() const;

    /**
     * @brief Get performance metrics
     */
    // Metrics API not yet implemented for optimized recorder path
    // IOPerformanceMetrics getMetrics() const;

    /**
     * @brief Set real-time audio callback
     */
    // Realtime callbacks/monitoring/config are not exposed yet

#ifdef HUNTMASTER_TEST_HOOKS
    // Test-only helpers to inject samples without opening audio devices.
    // Only effective in MEMORY_BASED or HYBRID modes; no-ops otherwise.
    void testFeedMemorySamples(const float* samples, size_t sampleCount);
    // Feed frames through the file-based pipeline (uses internal streaming buffer). No device
    // required. frames is in units of frames (per channel groups), not samples.
    void testFeedFileSamples(const float* interleaved, size_t frames);
    // Force flush any buffered frames from the streaming buffer into recordedData (file/hybrid
    // modes).
    void testForceFlushFileBuffer();
    // Get number of samples captured on the file path (valid in FILE_BASED/HYBRID).
    size_t testGetFileRecordedSamples() const;
#endif
};

// OptimizedAudioPlayer and BatchAudioProcessor are internal-only for now and
// intentionally not exposed in the public header until stabilized.

}  // namespace huntmaster
