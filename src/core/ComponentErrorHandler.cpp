#include <chrono>
#include <sstream>
#include <thread>

#include <huntmaster/core/ComponentErrorHandler.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace huntmaster {

// AudioEngineErrors Implementation
void ComponentErrorHandler::AudioEngineErrors::logInitializationFailure(const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::INITIALIZATION,
                           "AE_INIT_001",
                           "Audio engine initialization failed",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logDeviceInitFailure(const std::string& deviceName,
                                                                    const std::string& error) {
    auto context = createSystemContext();
    context["device_name"] = deviceName;
    context["device_error"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::INITIALIZATION,
                           "AE_DEV_001",
                           "Audio device initialization failed",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logConfigurationError(const std::string& parameter,
                                                                     const std::string& value) {
    auto context = createSystemContext();
    context["parameter"] = parameter;
    context["invalid_value"] = value;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::CONFIGURATION,
                           "AE_CFG_001",
                           "Invalid audio engine configuration",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logRecordingStartFailure(const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "AE_REC_001",
                           "Failed to start audio recording",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logRecordingStopFailure(const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::PROCESSING,
                           "AE_REC_002",
                           "Failed to stop audio recording",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logBufferOverflow(size_t bufferSize,
                                                                 size_t attemptedWrite) {
    auto context = createSystemContext();
    context["buffer_size"] = std::to_string(bufferSize);
    context["attempted_write"] = std::to_string(attemptedWrite);
    context["overflow_amount"] = std::to_string(attemptedWrite - bufferSize);

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::MEMORY,
                           "AE_BUF_001",
                           "Audio buffer overflow detected",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logMemoryAllocationFailure(size_t requestedSize) {
    auto context = createSystemContext();
    context["requested_size"] = std::to_string(requestedSize);

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "AE_MEM_001",
                           "Memory allocation failure in audio engine",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logPlaybackFailure(const std::string& filename,
                                                                  const std::string& error) {
    auto context = createSystemContext();
    context["filename"] = filename;
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::IO,
                           "AE_PLAY_001",
                           "Audio playback failure",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logAudioFormatError(const std::string& format,
                                                                   const std::string& expected) {
    auto context = createSystemContext();
    context["actual_format"] = format;
    context["expected_format"] = expected;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "AE_FMT_001",
                           "Unsupported audio format",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logProcessingError(const std::string& operation,
                                                                  const std::string& error) {
    auto context = createSystemContext();
    context["operation"] = operation;
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "AE_PROC_001",
                           "Audio processing error",
                           context);
}

void ComponentErrorHandler::AudioEngineErrors::logInvalidAudioData(size_t dataSize,
                                                                   const std::string& reason) {
    auto context = createSystemContext();
    context["data_size"] = std::to_string(dataSize);
    context["validation_failure"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::AUDIO_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "AE_DATA_001",
                           "Invalid audio data detected",
                           context);
}

// MFCCProcessorErrors Implementation
void ComponentErrorHandler::MFCCProcessorErrors::logInvalidConfiguration(
    const std::string& parameter, const std::string& value) {
    auto context = createSystemContext();
    context["parameter"] = parameter;
    context["invalid_value"] = value;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::HIGH,
                           ErrorCategory::CONFIGURATION,
                           "MFCC_CFG_001",
                           "Invalid MFCC processor configuration",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logFFTInitializationFailure(
    const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::INITIALIZATION,
                           "MFCC_FFT_001",
                           "FFT initialization failure",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionFailure(
    size_t frameSize, const std::string& reason) {
    auto context = createSystemContext();
    context["frame_size"] = std::to_string(frameSize);
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "MFCC_FEAT_001",
                           "MFCC feature extraction failed",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logInvalidInputSize(size_t actualSize,
                                                                     size_t expectedSize) {
    auto context = createSystemContext();
    context["actual_size"] = std::to_string(actualSize);
    context["expected_size"] = std::to_string(expectedSize);

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "MFCC_SIZE_001",
                           "Invalid input size for MFCC processing",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logFilterBankError(const std::string& error) {
    auto context = createSystemContext();
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "MFCC_FILT_001",
                           "Mel filter bank processing error",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logDCTError(const std::string& error) {
    auto context = createSystemContext();
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "MFCC_DCT_001",
                           "DCT computation error",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logPerformanceDegradation(double processingTime,
                                                                           double threshold) {
    auto context = createSystemContext();
    context["processing_time"] = std::to_string(processingTime);
    context["threshold"] = std::to_string(threshold);
    context["degradation_factor"] = std::to_string(processingTime / threshold);

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::PROCESSING,
                           "MFCC_PERF_001",
                           "MFCC processing performance degradation",
                           context);
}

void ComponentErrorHandler::MFCCProcessorErrors::logMemoryExhaustion(size_t availableMemory,
                                                                     size_t requiredMemory) {
    auto context = createSystemContext();
    context["available_memory"] = std::to_string(availableMemory);
    context["required_memory"] = std::to_string(requiredMemory);
    context["shortage"] = std::to_string(requiredMemory - availableMemory);

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "MFCC_MEM_001",
                           "Insufficient memory for MFCC processing",
                           context);
}

// UnifiedEngineErrors Implementation
void ComponentErrorHandler::UnifiedEngineErrors::logSessionCreationFailure(
    const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::INITIALIZATION,
                           "UE_SESS_001",
                           "Session creation failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logSessionNotFound(uint32_t sessionId) {
    auto context = createSystemContext();
    context["session_id"] = std::to_string(sessionId);

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "UE_SESS_002",
                           "Session not found",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logSessionStateError(
    uint32_t sessionId, const std::string& currentState) {
    auto context = createSessionContext(sessionId, currentState);

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "UE_SESS_003",
                           "Invalid session state",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logMasterCallLoadFailure(
    const std::string& callId, const std::string& reason) {
    auto context = createSystemContext();
    context["call_id"] = callId;
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::IO,
                           "UE_CALL_001",
                           "Master call loading failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logMasterCallCorrupted(
    const std::string& callId, const std::string& details) {
    auto context = createSystemContext();
    context["call_id"] = callId;
    context["corruption_details"] = details;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::VALIDATION,
                           "UE_CALL_002",
                           "Master call data corrupted",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logProcessingChainFailure(
    const std::string& stage, const std::string& error) {
    auto context = createSystemContext();
    context["processing_stage"] = stage;
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "UE_PROC_001",
                           "Processing chain failure",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logInsufficientAudioData(size_t dataSize,
                                                                          size_t requiredSize) {
    auto context = createSystemContext();
    context["data_size"] = std::to_string(dataSize);
    context["required_size"] = std::to_string(requiredSize);

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "UE_DATA_001",
                           "Insufficient audio data for processing",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logScoringFailure(const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "UE_SCORE_001",
                           "Similarity scoring failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logSimilarityAnalysisError(
    const std::string& error) {
    auto context = createSystemContext();
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "UE_SIM_001",
                           "Similarity analysis error",
                           context);
}

// MemoryErrors Implementation
void ComponentErrorHandler::MemoryErrors::logAllocationFailure(const std::string& component,
                                                               size_t size) {
    auto context = createSystemContext();
    context["component"] = component;
    context["allocation_size"] = std::to_string(size);

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "MEM_ALLOC_001",
                           "Memory allocation failure",
                           context);
}

void ComponentErrorHandler::MemoryErrors::logDeallocationError(const std::string& component,
                                                               void* pointer) {
    auto context = createSystemContext();
    context["component"] = component;

    std::ostringstream ptrStr;
    ptrStr << pointer;
    context["pointer"] = ptrStr.str();

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::HIGH,
                           ErrorCategory::MEMORY,
                           "MEM_DEALLOC_001",
                           "Memory deallocation error",
                           context);
}

void ComponentErrorHandler::MemoryErrors::logMemoryLeak(const std::string& component,
                                                        size_t leakedBytes) {
    auto context = createSystemContext();
    context["component"] = component;
    context["leaked_bytes"] = std::to_string(leakedBytes);

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::MEMORY,
                           "MEM_LEAK_001",
                           "Memory leak detected",
                           context);
}

void ComponentErrorHandler::MemoryErrors::logBufferOverrun(const std::string& component,
                                                           size_t bufferSize,
                                                           size_t accessSize) {
    auto context = createSystemContext();
    context["component"] = component;
    context["buffer_size"] = std::to_string(bufferSize);
    context["access_size"] = std::to_string(accessSize);
    context["overrun_amount"] = std::to_string(accessSize - bufferSize);

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "MEM_OVERRUN_001",
                           "Buffer overrun detected",
                           context);
}

void ComponentErrorHandler::MemoryErrors::logOutOfMemory(const std::string& component,
                                                         size_t requiredMemory,
                                                         size_t availableMemory) {
    auto context = createSystemContext();
    context["component"] = component;
    context["required_memory"] = std::to_string(requiredMemory);
    context["available_memory"] = std::to_string(availableMemory);

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "MEM_OOM_001",
                           "Out of memory condition",
                           context);
}

// Utility functions
std::unordered_map<std::string, std::string> ComponentErrorHandler::createSystemContext() {
    std::unordered_map<std::string, std::string> context;

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    context["timestamp"] = oss.str();

    // Thread information
    std::ostringstream threadId;
    threadId << std::this_thread::get_id();
    context["thread_id"] = threadId.str();

    // System information
#ifdef _WIN32
    context["platform"] = "Windows";
#elif defined(__linux__)
    context["platform"] = "Linux";
#elif defined(__APPLE__)
    context["platform"] = "macOS";
#else
    context["platform"] = "Unknown";
#endif

    return context;
}

std::unordered_map<std::string, std::string> ComponentErrorHandler::createAudioContext(
    uint32_t sampleRate, uint16_t channels, size_t bufferSize) {
    auto context = createSystemContext();
    context["sample_rate"] = std::to_string(sampleRate);
    context["channels"] = std::to_string(channels);
    context["buffer_size"] = std::to_string(bufferSize);
    return context;
}

std::unordered_map<std::string, std::string>
ComponentErrorHandler::createSessionContext(uint32_t sessionId, const std::string& state) {
    auto context = createSystemContext();
    context["session_id"] = std::to_string(sessionId);
    context["session_state"] = state;
    return context;
}

std::string ComponentErrorHandler::statusToErrorCode(UnifiedAudioEngine::Status status) {
    switch (status) {
        case UnifiedAudioEngine::Status::OK:
            return "UE_OK";
        case UnifiedAudioEngine::Status::INVALID_PARAMS:
            return "UE_INVALID_PARAMS";
        case UnifiedAudioEngine::Status::SESSION_NOT_FOUND:
            return "UE_SESSION_NOT_FOUND";
        case UnifiedAudioEngine::Status::FILE_NOT_FOUND:
            return "UE_FILE_NOT_FOUND";
        case UnifiedAudioEngine::Status::PROCESSING_ERROR:
            return "UE_PROCESSING_ERROR";
        case UnifiedAudioEngine::Status::INSUFFICIENT_DATA:
            return "UE_INSUFFICIENT_DATA";
        case UnifiedAudioEngine::Status::OUT_OF_MEMORY:
            return "UE_OUT_OF_MEMORY";
        case UnifiedAudioEngine::Status::INIT_FAILED:
            return "UE_INIT_FAILED";
        default:
            return "UE_UNKNOWN";
    }
}

std::string ComponentErrorHandler::mfccErrorToCode(MFCCError error) {
    switch (error) {
        case MFCCError::INVALID_INPUT:
            return "MFCC_INVALID_INPUT";
        case MFCCError::FFT_FAILED:
            return "MFCC_FFT_FAILED";
        case MFCCError::INVALID_CONFIG:
            return "MFCC_INVALID_CONFIG";
        case MFCCError::PROCESSING_FAILED:
            return "MFCC_PROCESSING_FAILED";
        default:
            return "MFCC_UNKNOWN";
    }
}

// UnifiedEngineErrors Implementation
void ComponentErrorHandler::UnifiedEngineErrors::logInitializationError(const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::INITIALIZATION,
                           "UE_INIT_001",
                           "Unified engine initialization failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logParameterValidationError(
    const std::string& parameter, const std::string& value) {
    auto context = createSystemContext();
    context["parameter"] = parameter;
    context["invalid_value"] = value;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::VALIDATION,
                           "UE_PARAM_001",
                           "Parameter validation failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logResourceLimitError(const std::string& resource,
                                                                       const std::string& limit) {
    auto context = createSystemContext();
    context["resource"] = resource;
    context["limit_exceeded"] = limit;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::RESOURCE,
                           "UE_RES_001",
                           "Resource limit exceeded",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logSessionError(const std::string& sessionId,
                                                                 const std::string& error) {
    auto context = createSystemContext();
    context["session_id"] = sessionId;
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::RESOURCE,
                           "UE_SESS_001",
                           "Session management error",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logProcessingError(const std::string& operation,
                                                                    const std::string& error) {
    auto context = createSystemContext();
    context["operation"] = operation;
    context["error_details"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "UE_PROC_001",
                           "Processing operation failed",
                           context);
}

void ComponentErrorHandler::UnifiedEngineErrors::logFeatureExtractionError(
    const std::string& reason) {
    auto context = createSystemContext();
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::UNIFIED_ENGINE,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "UE_FEAT_001",
                           "Feature extraction failed",
                           context);
}

// MemoryErrors Implementation
void ComponentErrorHandler::MemoryErrors::logMemoryAllocationError(const std::string& component,
                                                                   size_t size) {
    auto context = createSystemContext();
    context["component"] = component;
    context["requested_size"] = std::to_string(size);

    LOG_ERROR_WITH_CONTEXT(Component::MEMORY_MANAGER,
                           ErrorSeverity::CRITICAL,
                           ErrorCategory::MEMORY,
                           "MEM_ALLOC_001",
                           "Memory allocation failed",
                           context);
}

// MFCCProcessorErrors additional methods
void ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionError(
    size_t frameSize, const std::string& reason) {
    auto context = createSystemContext();
    context["frame_size"] = std::to_string(frameSize);
    context["failure_reason"] = reason;

    LOG_ERROR_WITH_CONTEXT(Component::MFCC_PROCESSOR,
                           ErrorSeverity::HIGH,
                           ErrorCategory::PROCESSING,
                           "MFCC_FEAT_001",
                           "MFCC feature extraction failed",
                           context);
}

// Additional error handler implementations for other categories would go here...
// (ConcurrencyErrors, IOErrors, ValidationErrors, PerformanceErrors)

// IOErrors Implementation
void ComponentErrorHandler::IOErrors::logFileOpenError(const std::string& filename,
                                                       const std::string& mode,
                                                       const std::string& error) {
    auto context = createSystemContext();
    context["filename"] = filename;
    context["mode"] = mode;
    context["error"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::HIGH,
                           ErrorCategory::IO,
                           "IO_FILE_001",
                           "Failed to open file",
                           context);
}

void ComponentErrorHandler::IOErrors::logFileReadError(const std::string& filename,
                                                       size_t bytesRequested,
                                                       const std::string& error) {
    auto context = createSystemContext();
    context["filename"] = filename;
    context["bytes_requested"] = std::to_string(bytesRequested);
    context["error"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::IO,
                           "IO_FILE_002",
                           "Failed to read from file",
                           context);
}

void ComponentErrorHandler::IOErrors::logFileWriteError(const std::string& filename,
                                                        size_t bytesAttempted,
                                                        const std::string& error) {
    auto context = createSystemContext();
    context["filename"] = filename;
    context["bytes_attempted"] = std::to_string(bytesAttempted);
    context["error"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::HIGH,
                           ErrorCategory::IO,
                           "IO_FILE_003",
                           "Failed to write to file",
                           context);
}

void ComponentErrorHandler::IOErrors::logDirectoryError(const std::string& path,
                                                        const std::string& operation,
                                                        const std::string& error) {
    auto context = createSystemContext();
    context["path"] = path;
    context["operation"] = operation;
    context["error"] = error;

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::MEDIUM,
                           ErrorCategory::IO,
                           "IO_DIR_001",
                           "Directory operation failed",
                           context);
}

void ComponentErrorHandler::IOErrors::logPermissionError(const std::string& resource,
                                                         const std::string& operation) {
    auto context = createSystemContext();
    context["resource"] = resource;
    context["operation"] = operation;

    LOG_ERROR_WITH_CONTEXT(Component::GENERAL,
                           ErrorSeverity::HIGH,
                           ErrorCategory::VALIDATION,
                           "IO_PERM_001",
                           "Permission denied for operation",
                           context);
}

}  // namespace huntmaster
