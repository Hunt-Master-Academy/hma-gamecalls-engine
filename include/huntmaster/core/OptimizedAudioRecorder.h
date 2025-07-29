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
// Forward declarations
class MasterIOOptimizer;
}

namespace huntmaster {

namespace io {
// Forward declarations
class MasterIOOptimizer;
}

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
    explicit OptimizedAudioRecorder(const Config& config = Config{});
    ~OptimizedAudioRecorder();

    /**
     * @brief Start recording with optimized I/O
     */
    bool startRecording();

    /**
     * @brief Stop recording and finalize file
     */
    bool stopRecording();

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
    IOPerformanceMetrics getMetrics() const;

    /**
     * @brief Set real-time audio callback
     */
    using AudioCallback =
        std::function<void(const float* data, size_t frameCount, double timestamp)>;
    void setAudioCallback(AudioCallback callback);

    /**
     * @brief Enable/disable monitoring during recording
     */
    void setMonitoringEnabled(bool enabled);

    /**
     * @brief Get buffer health information
     */
    StreamingAudioBuffer::BufferHealth getBufferHealth() const;

    /**
     * @brief Configure real-time processing
     */
    struct ProcessingConfig {
        bool enableEcho = false;
        bool enableReverb = false;
        float echoDelay = 0.3f;
        float echoFeedback = 0.4f;
        float reverbRoomSize = 0.5f;
    };

    void setProcessingConfig(const ProcessingConfig& config);
};

/**
 * @brief High-performance audio player with optimized I/O
 */
class OptimizedAudioPlayer {
  public:
    struct Config {
        // Playback settings
        bool enableGaplessPlayback = true;
        bool enableCrossfade = false;
        float crossfadeDuration = 0.1f;

        // Buffer settings
        size_t playbackBufferFrames = 8192;
        size_t prebufferFrames = 4096;

        // I/O optimization
        bool enableMemoryMapping = true;
        bool enablePrefetch = true;
        size_t prefetchSize = 1024 * 1024;  // 1MB

        // Quality settings
        bool enableResampling = true;
        bool enableVolumeControl = true;
        float defaultVolume = 1.0f;
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit OptimizedAudioPlayer(const Config& config = Config{});
    ~OptimizedAudioPlayer();

    /**
     * @brief Load audio file for playback
     */
    bool loadFile(const std::string& filename);

    /**
     * @brief Start playback
     */
    bool play();

    /**
     * @brief Pause playback
     */
    bool pause();

    /**
     * @brief Stop playback
     */
    bool stop();

    /**
     * @brief Seek to position (in seconds)
     */
    bool seek(double position);

    /**
     * @brief Check if currently playing
     */
    bool isPlaying() const;

    /**
     * @brief Get current playback position (in seconds)
     */
    double getPosition() const;

    /**
     * @brief Get total duration (in seconds)
     */
    double getDuration() const;

    /**
     * @brief Set playback volume (0.0 to 1.0)
     */
    void setVolume(float volume);

    /**
     * @brief Get current volume
     */
    float getVolume() const;

    /**
     * @brief Get performance metrics
     */
    IOPerformanceMetrics getMetrics() const;

    /**
     * @brief Set playback callback for real-time processing
     */
    using PlaybackCallback = std::function<void(float* data, size_t frameCount, double timestamp)>;
    void setPlaybackCallback(PlaybackCallback callback);

    /**
     * @brief Queue next file for gapless playback
     */
    bool queueFile(const std::string& filename);

    /**
     * @brief Clear playback queue
     */
    void clearQueue();

    /**
     * @brief Get queue size
     */
    size_t getQueueSize() const;
};

/**
 * @brief Batch audio processor for high-throughput operations
 */
class BatchAudioProcessor {
  public:
    struct Config {
        size_t maxParallelFiles = 4;
        size_t chunkSizeFrames = 16384;
        bool enableProgressReporting = true;
        bool enableErrorRecovery = true;
        std::string tempDirectory = "/tmp";
    };

    struct ProcessingJob {
        std::string inputFile;
        std::string outputFile;
        std::function<bool(const float*, size_t, float*)> processor;
        std::function<void(double)> progressCallback;
    };

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

  public:
    explicit BatchAudioProcessor(const Config& config = Config{});
    ~BatchAudioProcessor();

    /**
     * @brief Add processing job to queue
     */
    bool addJob(const ProcessingJob& job);

    /**
     * @brief Start batch processing
     */
    bool startProcessing();

    /**
     * @brief Stop batch processing
     */
    void stopProcessing();

    /**
     * @brief Check if processing is active
     */
    bool isProcessing() const;

    /**
     * @brief Get overall progress (0.0 to 1.0)
     */
    double getProgress() const;

    /**
     * @brief Get performance metrics
     */
    IOPerformanceMetrics getMetrics() const;

    /**
     * @brief Clear job queue
     */
    void clearJobs();

    /**
     * @brief Get queue size
     */
    size_t getQueueSize() const;
};

}  // namespace huntmaster
