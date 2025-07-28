/**
 * @file test_input_validator.cpp
 * @brief Comprehensive test suite for InputValidator security component
 *
 * This test suite provides thorough testing of the InputValidator class
 * including audio data validation, WASM interface validation, string
 * sanitization, network data validation, and malicious pattern detection.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/security/input-validator.h"

using namespace huntmaster;
using namespace huntmaster::security;
using namespace huntmaster::test;

class InputValidatorTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Configure validator for comprehensive testing
        config_.enableStringValidation = true;
        config_.enableNumericValidation = true;
        config_.enableBufferValidation = true;
        config_.enablePatternDetection = true;
        config_.maxStringLength = 1024;
        config_.maxBufferSize = 1048576;
        config_.strictMode = true;

        validator_ = std::make_unique<InputValidator>(config_);
    }

    void TearDown() override {
        validator_.reset();
        TestFixtureBase::TearDown();
    }

    // Helper function to generate test audio data
    std::vector<float> generateValidAudioBuffer(size_t samples, float amplitude = 0.5f) {
        std::vector<float> buffer(samples);
        for (size_t i = 0; i < samples; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            buffer[i] = amplitude * std::sin(2.0f * M_PI * 440.0f * t);
        }
        return buffer;
    }

    // Helper function to generate invalid audio data
    std::vector<float> generateInvalidAudioBuffer(size_t samples) {
        std::vector<float> buffer(samples);
        for (size_t i = 0; i < samples; ++i) {
            buffer[i] = (i % 2 == 0) ? std::numeric_limits<float>::infinity()
                                     : std::numeric_limits<float>::quiet_NaN();
        }
        return buffer;
    }

    // Helper function to create valid audio format parameters
    AudioFormatParams createValidAudioFormat() {
        AudioFormatParams params;
        params.sampleRate = 44100;
        params.channels = 2;
        params.bitDepth = 16;
        params.encoding = "PCM";
        return params;
    }

    // Helper function to create valid audio configuration
    AudioConfig createValidAudioConfig() {
        AudioConfig config;
        config.bufferSize = 1024;
        config.sampleRate = 44100;
        config.channels = 2;
        config.callbacks = {"onAudioReady", "onBufferFull"};
        return config;
    }

    // Helper function to create valid WASM values
    std::vector<WasmValue> createValidWasmParams() {
        std::vector<WasmValue> params(3);

        params[0].type = WasmValue::Int32;
        params[0].value.i32 = 1024;

        params[1].type = WasmValue::Float32;
        params[1].value.f32 = 44100.0f;

        params[2].type = WasmValue::Pointer;
        params[2].value.ptr = reinterpret_cast<void*>(0x1000);

        return params;
    }

    // Helper function to create HTTP request
    HttpRequest createValidHttpRequest() {
        HttpRequest request;
        request.method = "POST";
        request.url = "https://api.huntmaster.com/audio/analyze";
        request.headers.push_back({"Content-Type", "application/json"});
        request.headers.push_back({"Authorization", "Bearer token123"});
        request.body = {
            '{', ' ', '"', 't', 'e', 's', 't', '"', ':', ' ', '"', 'd', 'a', 't', 'a', '"', '}'};
        return request;
    }

    ValidationConfig config_;
    std::unique_ptr<InputValidator> validator_;
};

// Constructor and basic functionality tests
TEST_F(InputValidatorTest, ConstructorDestructorTest) {
    EXPECT_NE(validator_, nullptr);

    // Test configuration was set correctly
    auto currentConfig = validator_->getConfig();
    EXPECT_EQ(currentConfig.enableStringValidation, config_.enableStringValidation);
    EXPECT_EQ(currentConfig.enableNumericValidation, config_.enableNumericValidation);
    EXPECT_EQ(currentConfig.enableBufferValidation, config_.enableBufferValidation);
    EXPECT_EQ(currentConfig.enablePatternDetection, config_.enablePatternDetection);
    EXPECT_EQ(currentConfig.maxStringLength, config_.maxStringLength);
    EXPECT_EQ(currentConfig.maxBufferSize, config_.maxBufferSize);
    EXPECT_EQ(currentConfig.strictMode, config_.strictMode);
}

TEST_F(InputValidatorTest, ConfigurationUpdateTest) {
    // Create new configuration
    ValidationConfig newConfig;
    newConfig.enableStringValidation = false;
    newConfig.maxStringLength = 512;
    newConfig.strictMode = false;

    // Update configuration
    validator_->updateConfig(newConfig);

    // Verify configuration was updated
    auto currentConfig = validator_->getConfig();
    EXPECT_EQ(currentConfig.enableStringValidation, false);
    EXPECT_EQ(currentConfig.maxStringLength, 512);
    EXPECT_EQ(currentConfig.strictMode, false);
}

// Audio data validation tests
TEST_F(InputValidatorTest, ValidAudioBufferTest) {
    auto validBuffer = generateValidAudioBuffer(1024);

    EXPECT_TRUE(validator_->validateAudioBuffer(validBuffer.data(), validBuffer.size()));
}

TEST_F(InputValidatorTest, InvalidAudioBufferTest) {
    auto invalidBuffer = generateInvalidAudioBuffer(1024);

    EXPECT_FALSE(validator_->validateAudioBuffer(invalidBuffer.data(), invalidBuffer.size()));
}

TEST_F(InputValidatorTest, NullAudioBufferTest) {
    EXPECT_FALSE(validator_->validateAudioBuffer(nullptr, 1024));
}

TEST_F(InputValidatorTest, ZeroLengthAudioBufferTest) {
    auto validBuffer = generateValidAudioBuffer(1024);

    EXPECT_FALSE(validator_->validateAudioBuffer(validBuffer.data(), 0));
}

TEST_F(InputValidatorTest, OversizedAudioBufferTest) {
    size_t oversizedLength = config_.maxBufferSize + 1;
    auto validBuffer = generateValidAudioBuffer(100);  // Generate smaller buffer for efficiency

    EXPECT_FALSE(validator_->validateAudioBuffer(validBuffer.data(), oversizedLength));
}

TEST_F(InputValidatorTest, ValidAudioFormatTest) {
    auto validFormat = createValidAudioFormat();

    EXPECT_TRUE(validator_->validateAudioFormat(validFormat));
}

TEST_F(InputValidatorTest, InvalidAudioFormatTest) {
    auto invalidFormat = createValidAudioFormat();

    // Test invalid sample rates
    invalidFormat.sampleRate = 0;
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));

    invalidFormat.sampleRate = 200000;  // Too high
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));

    // Test invalid channels
    invalidFormat = createValidAudioFormat();
    invalidFormat.channels = 0;
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));

    invalidFormat.channels = 1000;  // Too many
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));

    // Test invalid bit depth
    invalidFormat = createValidAudioFormat();
    invalidFormat.bitDepth = 0;
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));

    // Test invalid encoding
    invalidFormat = createValidAudioFormat();
    invalidFormat.encoding = "";
    EXPECT_FALSE(validator_->validateAudioFormat(invalidFormat));
}

TEST_F(InputValidatorTest, ValidAudioConfigurationTest) {
    auto validConfig = createValidAudioConfig();

    EXPECT_TRUE(validator_->validateAudioConfiguration(validConfig));
}

TEST_F(InputValidatorTest, InvalidAudioConfigurationTest) {
    auto invalidConfig = createValidAudioConfig();

    // Test invalid buffer size
    invalidConfig.bufferSize = 0;
    EXPECT_FALSE(validator_->validateAudioConfiguration(invalidConfig));

    // Test invalid sample rate
    invalidConfig = createValidAudioConfig();
    invalidConfig.sampleRate = 0;
    EXPECT_FALSE(validator_->validateAudioConfiguration(invalidConfig));

    // Test invalid channels
    invalidConfig = createValidAudioConfig();
    invalidConfig.channels = 0;
    EXPECT_FALSE(validator_->validateAudioConfiguration(invalidConfig));
}

// WASM interface validation tests
TEST_F(InputValidatorTest, ValidWasmFunctionCallTest) {
    auto validParams = createValidWasmParams();

    EXPECT_TRUE(validator_->validateWasmFunctionCall("processAudio", validParams));
    EXPECT_TRUE(validator_->validateWasmFunctionCall("initializeEngine", validParams));
}

TEST_F(InputValidatorTest, InvalidWasmFunctionCallTest) {
    auto validParams = createValidWasmParams();

    // Test invalid function names
    EXPECT_FALSE(validator_->validateWasmFunctionCall("", validParams));
    EXPECT_FALSE(validator_->validateWasmFunctionCall("malicious_function", validParams));
    EXPECT_FALSE(validator_->validateWasmFunctionCall("__internal_func", validParams));

    // Test with too many parameters
    std::vector<WasmValue> tooManyParams(100);
    for (auto& param : tooManyParams) {
        param.type = WasmValue::Int32;
        param.value.i32 = 0;
    }
    EXPECT_FALSE(validator_->validateWasmFunctionCall("processAudio", tooManyParams));
}

TEST_F(InputValidatorTest, ValidWasmMemoryAccessTest) {
    void* validPtr = reinterpret_cast<void*>(0x1000);
    size_t validSize = 1024;

    EXPECT_TRUE(validator_->validateWasmMemoryAccess(validPtr, validSize, AccessType::Read));
    EXPECT_TRUE(validator_->validateWasmMemoryAccess(validPtr, validSize, AccessType::Write));
}

TEST_F(InputValidatorTest, InvalidWasmMemoryAccessTest) {
    // Test null pointer
    EXPECT_FALSE(validator_->validateWasmMemoryAccess(nullptr, 1024, AccessType::Read));

    // Test zero size
    void* validPtr = reinterpret_cast<void*>(0x1000);
    EXPECT_FALSE(validator_->validateWasmMemoryAccess(validPtr, 0, AccessType::Read));

    // Test oversized access
    EXPECT_FALSE(validator_->validateWasmMemoryAccess(
        validPtr, config_.maxBufferSize + 1, AccessType::Read));

    // Test invalid pointer ranges
    void* invalidPtr = reinterpret_cast<void*>(0x1);  // Too low
    EXPECT_FALSE(validator_->validateWasmMemoryAccess(invalidPtr, 1024, AccessType::Write));
}

// String validation tests
TEST_F(InputValidatorTest, ValidStringTest) {
    EXPECT_TRUE(validator_->validateString("Valid audio file name.wav", StringType::FilePath));
    EXPECT_TRUE(validator_->validateString("https://example.com/api", StringType::URL));
    EXPECT_TRUE(validator_->validateString("Sample Rate: 44100 Hz", StringType::AudioMetadata));
    EXPECT_TRUE(validator_->validateString("bufferSize=1024", StringType::Configuration));
    EXPECT_TRUE(validator_->validateString("Normal text input", StringType::Generic));
}

TEST_F(InputValidatorTest, InvalidStringTest) {
    // Test empty strings
    EXPECT_FALSE(validator_->validateString("", StringType::Generic));

    // Test oversized strings
    std::string oversizedString(config_.maxStringLength + 1, 'A');
    EXPECT_FALSE(validator_->validateString(oversizedString, StringType::Generic));

    // Test strings with null bytes
    std::string nullByteString = "valid\0malicious";
    EXPECT_FALSE(validator_->validateString(nullByteString, StringType::Generic));

    // Test strings with control characters
    std::string controlString = "text\x01\x02\x03";
    EXPECT_FALSE(validator_->validateString(controlString, StringType::Generic));
}

TEST_F(InputValidatorTest, FilePathValidationTest) {
    // Valid file paths
    EXPECT_TRUE(validator_->validateFilePath("/home/user/audio.wav"));
    EXPECT_TRUE(validator_->validateFilePath("C:\\Users\\Audio\\recording.mp3"));
    EXPECT_TRUE(validator_->validateFilePath("./relative/path/file.ogg"));
    EXPECT_TRUE(validator_->validateFilePath("../parent/file.flac"));

    // Invalid file paths
    EXPECT_FALSE(validator_->validateFilePath(""));
    EXPECT_FALSE(validator_->validateFilePath("/etc/passwd"));           // System file
    EXPECT_FALSE(validator_->validateFilePath("../../../etc/shadow"));   // Directory traversal
    EXPECT_FALSE(validator_->validateFilePath("file|with|pipes"));       // Shell metacharacters
    EXPECT_FALSE(validator_->validateFilePath("file;with;semicolons"));  // Command injection
}

// Numeric validation tests
TEST_F(InputValidatorTest, ValidNumericInputTest) {
    EXPECT_TRUE(validator_->validateNumericInput(44100.0, NumericType::SampleRate));
    EXPECT_TRUE(validator_->validateNumericInput(1024.0, NumericType::BufferSize));
    EXPECT_TRUE(validator_->validateNumericInput(123456789.0, NumericType::Timestamp));
    EXPECT_TRUE(validator_->validateNumericInput(42.0, NumericType::Integer));
    EXPECT_TRUE(validator_->validateNumericInput(3.14159, NumericType::Float));
}

TEST_F(InputValidatorTest, InvalidNumericInputTest) {
    // Test negative values where not allowed
    EXPECT_FALSE(validator_->validateNumericInput(-44100.0, NumericType::SampleRate));
    EXPECT_FALSE(validator_->validateNumericInput(-1024.0, NumericType::BufferSize));

    // Test extreme values
    EXPECT_FALSE(validator_->validateNumericInput(std::numeric_limits<double>::infinity(),
                                                  NumericType::Float));
    EXPECT_FALSE(validator_->validateNumericInput(std::numeric_limits<double>::quiet_NaN(),
                                                  NumericType::Float));

    // Test out of range values
    EXPECT_FALSE(validator_->validateNumericInput(1000000.0, NumericType::SampleRate));  // Too high
    EXPECT_FALSE(validator_->validateNumericInput(0.0, NumericType::SampleRate));        // Too low
}

// Network data validation tests
TEST_F(InputValidatorTest, ValidNetworkDataTest) {
    std::vector<uint8_t> validData = {0x01, 0x02, 0x03, 0x04, 0x05};

    EXPECT_TRUE(validator_->validateNetworkData(validData.data(), validData.size()));
}

TEST_F(InputValidatorTest, InvalidNetworkDataTest) {
    // Test null data
    EXPECT_FALSE(validator_->validateNetworkData(nullptr, 100));

    // Test zero length
    std::vector<uint8_t> validData = {0x01, 0x02, 0x03};
    EXPECT_FALSE(validator_->validateNetworkData(validData.data(), 0));

    // Test oversized data
    std::vector<uint8_t> data(100);
    EXPECT_FALSE(validator_->validateNetworkData(data.data(), config_.maxBufferSize + 1));
}

TEST_F(InputValidatorTest, ValidHttpRequestTest) {
    auto validRequest = createValidHttpRequest();

    EXPECT_TRUE(validator_->validateHttpRequest(validRequest));
}

TEST_F(InputValidatorTest, InvalidHttpRequestTest) {
    auto invalidRequest = createValidHttpRequest();

    // Test invalid methods
    invalidRequest.method = "";
    EXPECT_FALSE(validator_->validateHttpRequest(invalidRequest));

    invalidRequest.method = "INVALID_METHOD";
    EXPECT_FALSE(validator_->validateHttpRequest(invalidRequest));

    // Test invalid URLs
    invalidRequest = createValidHttpRequest();
    invalidRequest.url = "";
    EXPECT_FALSE(validator_->validateHttpRequest(invalidRequest));

    invalidRequest.url = "javascript:alert('xss')";
    EXPECT_FALSE(validator_->validateHttpRequest(invalidRequest));

    // Test oversized body
    invalidRequest = createValidHttpRequest();
    invalidRequest.body = std::vector<uint8_t>(config_.maxBufferSize + 1, 0);
    EXPECT_FALSE(validator_->validateHttpRequest(invalidRequest));
}

// Configuration validation tests
TEST_F(InputValidatorTest, ValidConfigurationTest) {
    ConfigurationData validConfig;
    validConfig.settings.push_back({"sampleRate", "44100"});
    validConfig.settings.push_back({"bufferSize", "1024"});
    validConfig.signature = "valid_signature_hash";
    validConfig.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

    EXPECT_TRUE(validator_->validateConfiguration(validConfig));
}

TEST_F(InputValidatorTest, InvalidConfigurationTest) {
    ConfigurationData invalidConfig;

    // Test empty configuration
    EXPECT_FALSE(validator_->validateConfiguration(invalidConfig));

    // Test invalid signature
    invalidConfig.settings.push_back({"sampleRate", "44100"});
    invalidConfig.signature = "";  // Empty signature
    invalidConfig.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
    EXPECT_FALSE(validator_->validateConfiguration(invalidConfig));

    // Test invalid timestamp
    invalidConfig.signature = "valid_signature";
    invalidConfig.timestamp = 0;  // Invalid timestamp
    EXPECT_FALSE(validator_->validateConfiguration(invalidConfig));
}

// Malicious pattern detection tests
TEST_F(InputValidatorTest, MaliciousPatternDetectionTest) {
    // Test SQL injection patterns
    EXPECT_TRUE(validator_->detectMaliciousPatterns("'; DROP TABLE users; --"));
    EXPECT_TRUE(validator_->detectMaliciousPatterns("UNION SELECT * FROM passwords"));

    // Test XSS patterns
    EXPECT_TRUE(validator_->detectMaliciousPatterns("<script>alert('xss')</script>"));
    EXPECT_TRUE(validator_->detectMaliciousPatterns("javascript:void(0)"));

    // Test command injection patterns
    EXPECT_TRUE(validator_->detectMaliciousPatterns("$(rm -rf /)"));
    EXPECT_TRUE(validator_->detectMaliciousPatterns("`cat /etc/passwd`"));

    // Test path traversal patterns
    EXPECT_TRUE(validator_->detectMaliciousPatterns("../../../etc/passwd"));
    EXPECT_TRUE(validator_->detectMaliciousPatterns("..\\..\\..\\windows\\system32"));

    // Test legitimate patterns should not be flagged
    EXPECT_FALSE(validator_->detectMaliciousPatterns("Normal audio processing text"));
    EXPECT_FALSE(validator_->detectMaliciousPatterns("File: recording_2025_01_27.wav"));
}

// Security rules update test
TEST_F(InputValidatorTest, SecurityRulesUpdateTest) {
    SecurityRuleSet rules;
    rules.patterns = {"malicious_pattern", "another_bad_pattern"};
    rules.whitelist = {"safe_function", "allowed_operation"};
    rules.blacklist = {"forbidden_function", "blocked_operation"};
    rules.version = 1;

    // Update should not throw
    EXPECT_NO_THROW(validator_->updateSecurityRules(rules));

    // Test that patterns are now detected
    EXPECT_TRUE(validator_->detectMaliciousPatterns("malicious_pattern"));
    EXPECT_TRUE(validator_->detectMaliciousPatterns("another_bad_pattern"));
}

// Error reporting and validation report tests
TEST_F(InputValidatorTest, ErrorReportingTest) {
    // Report some validation errors
    validator_->reportValidationError("Test error 1", ValidationSeverity::Error);
    validator_->reportValidationError("Test warning", ValidationSeverity::Warning);
    validator_->reportValidationError("Critical issue", ValidationSeverity::Critical);

    // Generate validation report
    auto report = validator_->generateValidationReport();

    EXPECT_GT(report.totalValidations, 0);
    EXPECT_GT(report.errorCount, 0);
    EXPECT_GT(report.warningCount, 0);
    EXPECT_FALSE(report.criticalErrors.empty());
    EXPECT_LE(report.validationSuccessRate, 1.0);
    EXPECT_GE(report.validationSuccessRate, 0.0);
}

// Performance and stress tests
TEST_F(InputValidatorTest, PerformanceTest) {
    const int numValidations = 1000;
    auto validBuffer = generateValidAudioBuffer(1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numValidations; ++i) {
        validator_->validateAudioBuffer(validBuffer.data(), validBuffer.size());
        validator_->validateString("test_string_" + std::to_string(i), StringType::Generic);
        validator_->validateNumericInput(i * 44.1, NumericType::Float);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    double avgTimePerValidation = static_cast<double>(duration.count()) / (numValidations * 3);

    std::cout << "Average validation time: " << avgTimePerValidation << " μs" << std::endl;

    // Performance should be reasonable
    EXPECT_LT(avgTimePerValidation, 1000.0);  // Less than 1ms per validation
}

TEST_F(InputValidatorTest, ConcurrentValidationTest) {
    const int numThreads = 4;
    const int validationsPerThread = 250;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};

    auto validBuffer = generateValidAudioBuffer(512);

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < validationsPerThread; ++i) {
                bool result =
                    validator_->validateAudioBuffer(validBuffer.data(), validBuffer.size());
                if (result) {
                    successCount++;
                } else {
                    failCount++;
                }

                // Mix in string validations
                std::string testStr =
                    "thread_" + std::to_string(t) + "_iteration_" + std::to_string(i);
                validator_->validateString(testStr, StringType::Generic);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All validations should succeed with valid data
    EXPECT_EQ(successCount.load(), numThreads * validationsPerThread);
    EXPECT_EQ(failCount.load(), 0);
}

// Edge cases and boundary tests
TEST_F(InputValidatorTest, BoundaryConditionsTest) {
    // Test maximum allowed string length
    std::string maxLengthString(config_.maxStringLength, 'A');
    EXPECT_TRUE(validator_->validateString(maxLengthString, StringType::Generic));

    // Test maximum allowed buffer size
    std::vector<float> maxBuffer(config_.maxBufferSize / sizeof(float));
    for (auto& sample : maxBuffer) {
        sample = 0.1f;  // Valid audio sample
    }
    EXPECT_TRUE(validator_->validateAudioBuffer(maxBuffer.data(), maxBuffer.size()));

    // Test minimum valid values
    EXPECT_TRUE(validator_->validateString("A", StringType::Generic));
    EXPECT_TRUE(validator_->validateNumericInput(1.0, NumericType::Integer));
}

}  // namespace huntmaster
