/**
 * @file input-validator.h
 * @brief Input Validation Security Header - Phase 3.3 Security Framework
 *
 * This header defines the InputValidator class and related structures
 * for comprehensive input validation and sanitization.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 25, 2025
 */

#ifndef HUNTMASTER_SECURITY_INPUT_VALIDATOR_H
#define HUNTMASTER_SECURITY_INPUT_VALIDATOR_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Validation Configuration
 */
struct ValidationConfig {
    bool enableStringValidation = true;
    bool enableNumericValidation = true;
    bool enableBufferValidation = true;
    bool enablePatternDetection = true;
    size_t maxStringLength = 65536;
    size_t maxBufferSize = 1048576;
    bool strictMode = false;
};

/**
 * Validation Types
 */
enum class StringType { Generic, FilePath, URL, AudioMetadata, Configuration };

enum class NumericType { Integer, Float, SampleRate, BufferSize, Timestamp };

enum class AccessType { Read, Write, Execute };

enum class ValidationSeverity { Info, Warning, Error, Critical };

/**
 * Data Structures
 */
struct AudioFormatParams {
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitDepth;
    std::string encoding;
};

struct AudioConfig {
    size_t bufferSize;
    uint32_t sampleRate;
    uint16_t channels;
    std::vector<std::string> callbacks;
};

struct WasmValue {
    enum Type { Int32, Int64, Float32, Float64, Pointer } type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        void* ptr;
    } value;
};

struct HttpRequest {
    std::string method;
    std::string url;
    std::vector<std::pair<std::string, std::string>> headers;
    std::vector<uint8_t> body;
};

struct ConfigurationData {
    std::vector<std::pair<std::string, std::string>> settings;
    std::string signature;
    uint64_t timestamp;
};

struct ValidationReport {
    uint64_t totalValidations;
    uint64_t errorCount;
    uint64_t warningCount;
    std::vector<std::string> criticalErrors;
    double validationSuccessRate;
};

struct SecurityRuleSet {
    std::vector<std::string> patterns;
    std::vector<std::string> whitelist;
    std::vector<std::string> blacklist;
    uint64_t version;
};

/**
 * InputValidator Class
 * Provides comprehensive input validation and sanitization
 */
class InputValidator {
  public:
    explicit InputValidator(const ValidationConfig& config = ValidationConfig{});
    ~InputValidator();

    // Disable copy construction and assignment
    InputValidator(const InputValidator&) = delete;
    InputValidator& operator=(const InputValidator&) = delete;

    // Audio Data Validation
    bool validateAudioBuffer(const float* buffer, size_t length);
    bool validateAudioFormat(const AudioFormatParams& params);
    bool validateAudioConfiguration(const AudioConfig& config);

    // WASM Interface Validation
    bool validateWasmFunctionCall(const std::string& functionName,
                                  const std::vector<WasmValue>& params);
    bool validateWasmMemoryAccess(void* ptr, size_t size, AccessType type);

    // String and Data Validation
    bool validateString(const std::string& input, StringType type = StringType::Generic);
    bool validateFilePath(const std::string& path);
    bool validateNumericInput(double value, NumericType type);

    // Network and Communication Validation
    bool validateNetworkData(const uint8_t* data, size_t length);
    bool validateHttpRequest(const HttpRequest& request);

    // Configuration and Settings Validation
    bool validateConfiguration(const ConfigurationData& config);

    // Error Handling and Reporting
    void reportValidationError(const std::string& error, ValidationSeverity severity);
    ValidationReport generateValidationReport() const;

    // Pattern Detection and Analysis
    bool detectMaliciousPatterns(const std::string& input);
    void updateSecurityRules(const SecurityRuleSet& rules);

    // Configuration Management
    void updateConfig(const ValidationConfig& config) {
        config_ = config;
    }
    const ValidationConfig& getConfig() const {
        return config_;
    }

  private:
    ValidationConfig config_;
    uint64_t errorCount_;
    uint64_t warningCount_;
    std::vector<std::string> customMaliciousPatterns_;
    std::vector<std::string> criticalErrors_;

    // TODO: Add private validation helper methods
    // TODO: Add pattern matching engine
    // TODO: Add security rule storage
    // TODO: Add validation statistics tracking
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_INPUT_VALIDATOR_H
