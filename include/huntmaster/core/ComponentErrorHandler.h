#pragma once

#include <string>
#include <unordered_map>

#include <huntmaster/core/ErrorLogger.h>
#include <huntmaster/core/MFCCProcessor.h>
#include <huntmaster/core/UnifiedAudioEngine.h>

namespace huntmaster {

/**
 * @brief Utility class for component-specific error handling and logging
 *
 * This class provides standardized error codes, messages, and context
 * collection for different components in the Huntmaster engine.
 */
class ComponentErrorHandler {
  public:
    /**
     * @brief Audio Engine specific error handling
     */
    struct AudioEngineErrors {
        // Initialization errors
        static void logInitializationFailure(const std::string& reason);
        static void logDeviceInitFailure(const std::string& deviceName, const std::string& error);
        static void logConfigurationError(const std::string& parameter, const std::string& value);

        // Recording errors
        static void logRecordingStartFailure(const std::string& reason);
        static void logRecordingStopFailure(const std::string& reason);
        static void logBufferOverflow(size_t bufferSize, size_t attemptedWrite);
        static void logMemoryAllocationFailure(size_t requestedSize);

        // Playback errors
        static void logPlaybackFailure(const std::string& filename, const std::string& error);
        static void logAudioFormatError(const std::string& format, const std::string& expected);

        // Processing errors
        static void logProcessingError(const std::string& operation, const std::string& error);
        static void logInvalidAudioData(size_t dataSize, const std::string& reason);
    };

    /**
     * @brief MFCC Processor specific error handling
     */
    struct MFCCProcessorErrors {
        // Configuration errors
        static void logInvalidConfiguration(const std::string& parameter, const std::string& value);
        static void logFFTInitializationFailure(const std::string& reason);

        // Processing errors
        static void logFeatureExtractionFailure(size_t frameSize, const std::string& reason);
        static void logInvalidInputSize(size_t actualSize, size_t expectedSize);
        static void logFilterBankError(const std::string& error);
        static void logDCTError(const std::string& error);

        // Performance issues
        static void logPerformanceDegradation(double processingTime, double threshold);
        static void logMemoryExhaustion(size_t availableMemory, size_t requiredMemory);

        // Additional required methods
        static void logFeatureExtractionError(size_t frameSize, const std::string& reason);
    };

    /**
     * @brief Unified Audio Engine specific error handling
     */
    struct UnifiedEngineErrors {
        // Session management errors
        static void logSessionCreationFailure(const std::string& reason);
        static void logSessionNotFound(uint32_t sessionId);
        static void logSessionStateError(uint32_t sessionId, const std::string& currentState);

        // Master call loading errors
        static void logMasterCallLoadFailure(const std::string& callId, const std::string& reason);
        static void logMasterCallCorrupted(const std::string& callId, const std::string& details);

        // Processing chain errors
        static void logProcessingChainFailure(const std::string& stage, const std::string& error);
        static void logInsufficientAudioData(size_t dataSize, size_t requiredSize);

        // Scoring errors
        static void logScoringFailure(const std::string& reason);
        static void logSimilarityAnalysisError(const std::string& error);

        // Additional required methods
        static void logInitializationError(const std::string& reason);
        static void logParameterValidationError(const std::string& parameter,
                                                const std::string& value);
        static void logResourceLimitError(const std::string& resource, const std::string& limit);
        static void logSessionError(const std::string& sessionId, const std::string& error);
        static void logProcessingError(const std::string& operation, const std::string& error);
        static void logFeatureExtractionError(const std::string& reason);
    };

    /**
     * @brief Memory Management error handling
     */
    struct MemoryErrors {
        static void logAllocationFailure(const std::string& component, size_t size);
        static void logDeallocationError(const std::string& component, void* pointer);
        static void logMemoryLeak(const std::string& component, size_t leakedBytes);
        static void
        logBufferOverrun(const std::string& component, size_t bufferSize, size_t accessSize);
        static void
        logOutOfMemory(const std::string& component, size_t requiredMemory, size_t availableMemory);
        static void logMemoryAllocationError(const std::string& component, size_t size);
    };

    /**
     * @brief Threading and Concurrency error handling
     */
    struct ConcurrencyErrors {
        static void logDeadlock(const std::string& component, const std::string& resources);
        static void logRaceCondition(const std::string& component, const std::string& operation);
        static void logMutexError(const std::string& component,
                                  const std::string& mutexName,
                                  const std::string& error);
        static void logThreadCreationFailure(const std::string& component,
                                             const std::string& reason);
        static void logThreadSynchronizationError(const std::string& component,
                                                  const std::string& details);
    };

    /**
     * @brief I/O Operation error handling
     */
    struct IOErrors {
        static void logFileOpenError(const std::string& filename,
                                     const std::string& mode,
                                     const std::string& error);
        static void logFileReadError(const std::string& filename,
                                     size_t bytesRequested,
                                     const std::string& error);
        static void logFileWriteError(const std::string& filename,
                                      size_t bytesAttempted,
                                      const std::string& error);
        static void logDirectoryError(const std::string& path,
                                      const std::string& operation,
                                      const std::string& error);
        static void logPermissionError(const std::string& resource, const std::string& operation);
    };

    /**
     * @brief Validation error handling
     */
    struct ValidationErrors {
        static void logParameterValidationError(const std::string& parameter,
                                                const std::string& value,
                                                const std::string& constraint);
        static void
        logRangeValidationError(const std::string& parameter, double value, double min, double max);
        static void logFormatValidationError(const std::string& input,
                                             const std::string& expectedFormat);
        static void logDependencyValidationError(const std::string& component,
                                                 const std::string& dependency);
    };

    /**
     * @brief Performance Monitoring error handling
     */
    struct PerformanceErrors {
        static void logPerformanceThresholdExceeded(const std::string& operation,
                                                    double actualTime,
                                                    double threshold);
        static void logResourceUtilizationHigh(const std::string& resource,
                                               double utilization,
                                               double threshold);
        static void logBottleneckDetected(const std::string& component,
                                          const std::string& operation);
        static void logLatencySpike(const std::string& operation, double latency, double baseline);
    };

    /**
     * @brief Create context map with common system information
     */
    static std::unordered_map<std::string, std::string> createSystemContext();

    /**
     * @brief Create context map with audio-specific information
     */
    static std::unordered_map<std::string, std::string>
    createAudioContext(uint32_t sampleRate, uint16_t channels, size_t bufferSize);

    /**
     * @brief Create context map with session information
     */
    static std::unordered_map<std::string, std::string>
    createSessionContext(uint32_t sessionId, const std::string& state);

    /**
     * @brief Convert UnifiedAudioEngine::Status to error context
     */
    static std::string statusToErrorCode(UnifiedAudioEngine::Status status);

    /**
     * @brief Convert MFCCError to error context
     */
    static std::string mfccErrorToCode(MFCCError error);

  private:
    ComponentErrorHandler() = delete;
};

}  // namespace huntmaster
