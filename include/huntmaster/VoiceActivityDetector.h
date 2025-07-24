/**
 * @file VoiceActivityDetector.h
 * @brief Voice Activity Detection System Header
 *
 * This header defines the Voice Activity Detection (VAD) system for real-time
 * audio processing with advanced algorithms and comprehensive monitoring.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "AudioBuffer.h"
#include "AudioConfig.h"

namespace huntmaster {
namespace core {

// TODO: Phase 2.4 - Advanced Audio Engine Components - COMPREHENSIVE HEADER TODO
// =================================================================================

// TODO 2.4.30: Voice Activity Detection System Header
// ---------------------------------------------------
/**
 * TODO: Define comprehensive Voice Activity Detection system with:
 * [ ] Multi-algorithm VAD with energy, spectral, and machine learning approaches
 * [ ] Real-time processing with low-latency detection and minimal CPU overhead
 * [ ] Adaptive thresholding with environmental noise compensation
 * [ ] Context-aware detection with speech pattern recognition
 * [ ] Performance monitoring with accuracy measurement and optimization
 * [ ] Configuration management with parameter validation and updates
 * [ ] Statistical analysis with detection history and trend analysis
 * [ ] Integration support with callback system and event notification
 * [ ] Error handling with comprehensive logging and recovery mechanisms
 * [ ] Platform optimization with SIMD instructions and multi-threading
 */

/**
 * @brief Voice Activity Detection Configuration
 *
 * Configuration structure for Voice Activity Detection with comprehensive
 * parameter control and optimization settings.
 */
struct VADConfig {
    // Basic Parameters
    float threshold;              ///< Voice detection threshold (0.0-1.0)
    uint32_t hangTime;            ///< Hang time in milliseconds
    uint32_t minSpeechDuration;   ///< Minimum speech duration in ms
    uint32_t minSilenceDuration;  ///< Minimum silence duration in ms

    // Algorithm Selection
    enum class Algorithm {
        ENERGY_BASED,        ///< Simple energy-based detection
        SPECTRAL_ENTROPY,    ///< Spectral entropy-based detection
        ZERO_CROSSING_RATE,  ///< Zero crossing rate analysis
        SPECTRAL_CENTROID,   ///< Spectral centroid analysis
        MACHINE_LEARNING,    ///< ML-based detection
        ENSEMBLE             ///< Combined multiple algorithms
    } algorithm = Algorithm::ENSEMBLE;

    // Advanced Parameters
    float energyFloor;            ///< Energy floor for normalization
    float spectralFloor;          ///< Spectral floor for analysis
    uint32_t analysisWindowSize;  ///< Analysis window size in samples
    uint32_t analysisHopSize;     ///< Analysis hop size in samples

    // Adaptive Control
    bool enableAdaptiveThreshold;  ///< Enable adaptive threshold adjustment
    float adaptationRate;          ///< Threshold adaptation rate (0.0-1.0)
    uint32_t adaptationInterval;   ///< Adaptation interval in ms

    // Noise Compensation
    bool enableNoiseCompensation;  ///< Enable noise level compensation
    float noiseFloor;              ///< Estimated noise floor level
    uint32_t noiseEstimationTime;  ///< Noise estimation time in ms

    // Performance Settings
    bool enableOptimizations;       ///< Enable performance optimizations
    uint32_t maxProcessingLatency;  ///< Maximum processing latency in ms
    bool enableParallelProcessing;  ///< Enable multi-threading

    // Quality Control
    float confidenceThreshold;       ///< Minimum confidence for detection
    bool enableQualityMetrics;       ///< Enable quality measurement
    uint32_t qualityUpdateInterval;  ///< Quality update interval in ms

    // TODO: Add more advanced parameters as needed
    bool enableContextAwareness;  ///< Enable context-aware processing
    float contextWeight;          ///< Weight for context information
    uint32_t contextWindowSize;   ///< Context analysis window size
};

/**
 * @brief Voice Activity Detection Results
 *
 * Comprehensive results structure containing detection status, confidence,
 * and detailed analysis information.
 */
struct VADResult {
    // Detection Results
    bool voiceDetected;  ///< Voice activity detected
    float probability;   ///< Voice probability (0.0-1.0)
    float confidence;    ///< Detection confidence (0.0-1.0)

    // Analysis Metrics
    float energyLevel;       ///< Current energy level
    float spectralEntropy;   ///< Spectral entropy value
    float zeroCrossingRate;  ///< Zero crossing rate
    float spectralCentroid;  ///< Spectral centroid frequency

    // Timing Information
    std::chrono::microseconds timestamp;  ///< Detection timestamp
    uint32_t sequenceNumber;              ///< Sequence number for ordering
    uint32_t speechDuration;              ///< Current speech segment duration (ms)
    uint32_t silenceDuration;             ///< Current silence segment duration (ms)

    // Quality Metrics
    float signalQuality;  ///< Signal quality assessment
    float noiseLevel;     ///< Estimated noise level
    bool isReliable;      ///< Result reliability flag

    // Algorithm-Specific Results
    struct AlgorithmResults {
        float energyResult;    ///< Energy-based result
        float spectralResult;  ///< Spectral-based result
        float zcrResult;       ///< ZCR-based result
        float centroidResult;  ///< Centroid-based result
        float mlResult;        ///< ML-based result
    } algorithmResults;

    // Error Information
    int errorCode;             ///< Error code (0 = success)
    std::string errorMessage;  ///< Error message if any
    std::string debugInfo;     ///< Debug information
};

/**
 * @brief Voice Activity Detection Statistics
 *
 * Statistical information about VAD performance and accuracy over time.
 */
struct VADStatistics {
    // Detection Statistics
    uint64_t totalDetections;    ///< Total number of detections
    uint64_t voiceDetections;    ///< Number of voice detections
    uint64_t silenceDetections;  ///< Number of silence detections
    float voiceRatio;            ///< Ratio of voice to total time

    // Accuracy Metrics
    float accuracy;   ///< Overall detection accuracy
    float precision;  ///< Detection precision
    float recall;     ///< Detection recall
    float f1Score;    ///< F1 score

    // Timing Statistics
    float averageProcessingTime;  ///< Average processing time (ms)
    float maxProcessingTime;      ///< Maximum processing time (ms)
    float minProcessingTime;      ///< Minimum processing time (ms)

    // Current State
    float currentProbability;  ///< Current voice probability
    float currentConfidence;   ///< Current confidence level
    bool currentState;         ///< Current detection state

    // Trend Analysis
    float trendDirection;   ///< Trend direction (-1.0 to 1.0)
    float trendStrength;    ///< Trend strength (0.0-1.0)
    uint32_t stateChanges;  ///< Number of state changes

    // Error Metrics
    uint32_t totalErrors;     ///< Total number of errors
    uint32_t falsePositives;  ///< False positive count
    uint32_t falseNegatives;  ///< False negative count
};

/**
 * @brief Voice Activity Detector Class
 *
 * Main Voice Activity Detection class with comprehensive algorithms,
 * real-time processing, and advanced monitoring capabilities.
 */
class VoiceActivityDetector {
  public:
    // TODO 2.4.31: Constructor and Destructor
    // ---------------------------------------
    /**
     * TODO: Implement comprehensive constructor and destructor with:
     * [ ] Default parameter initialization with intelligent defaults
     * [ ] Resource allocation with memory optimization
     * [ ] Algorithm initialization with performance optimization
     * [ ] Error handling with proper exception safety
     * [ ] Logging setup with structured information
     * [ ] Performance monitoring initialization
     * [ ] Thread-safety setup with mutex initialization
     * [ ] Platform-specific optimizations
     * [ ] Memory leak prevention with proper cleanup
     * [ ] Integration with external libraries if needed
     */
    VoiceActivityDetector();
    ~VoiceActivityDetector();

    // TODO 2.4.32: Initialization and Configuration
    // ---------------------------------------------
    /**
     * TODO: Implement comprehensive initialization and configuration with:
     * [ ] Configuration parameter validation with range checking
     * [ ] Algorithm selection and initialization with optimization
     * [ ] Memory allocation with leak detection and monitoring
     * [ ] Performance baseline establishment with calibration
     * [ ] Error handling setup with logging configuration
     * [ ] Thread preparation with priority and affinity settings
     * [ ] Platform-specific optimization detection and setup
     * [ ] Integration with audio processing pipeline
     * [ ] Statistical analysis initialization with baseline metrics
     * [ ] Quality assessment setup with metric configuration
     */
    bool initialize(const VADConfig& config);
    bool initialize(const StreamingConfig& streamingConfig);  // For compatibility
    bool updateConfiguration(const VADConfig& config);
    bool updateParameters(float threshold, uint32_t hangTime);
    bool isInitialized() const;
    VADConfig getConfiguration() const;

    // TODO 2.4.33: Real-time Detection Processing
    // -------------------------------------------
    /**
     * TODO: Implement comprehensive real-time detection with:
     * [ ] High-performance audio buffer processing with SIMD optimization
     * [ ] Multi-algorithm detection with ensemble decision making
     * [ ] Low-latency processing with minimal CPU overhead
     * [ ] Adaptive threshold adjustment with environmental compensation
     * [ ] Context-aware analysis with speech pattern recognition
     * [ ] Quality assessment integration with reliability scoring
     * [ ] Performance monitoring with real-time optimization
     * [ ] Error handling with graceful degradation and recovery
     * [ ] Memory optimization with efficient buffer management
     * [ ] Thread-safe processing with lock-free operations where possible
     */
    bool processBuffer(const AudioBuffer& buffer, float& probability, float& confidence);
    VADResult processBufferDetailed(const AudioBuffer& buffer);
    bool processRealtime(const float* audioData, size_t sampleCount, VADResult& result);

    // TODO 2.4.34: Algorithm-Specific Processing
    // ------------------------------------------
    /**
     * TODO: Implement algorithm-specific processing methods with:
     * [ ] Energy-based detection with adaptive normalization
     * [ ] Spectral entropy analysis with frequency domain processing
     * [ ] Zero crossing rate analysis with noise compensation
     * [ ] Spectral centroid analysis with harmonic consideration
     * [ ] Machine learning-based detection with trained models
     * [ ] Ensemble method with weighted algorithm combination
     * [ ] Performance optimization with algorithm selection
     * [ ] Error handling with algorithm fallback mechanisms
     * [ ] Parameter tuning with automatic optimization
     * [ ] Integration with external algorithm libraries
     */
    float processEnergyBased(const AudioBuffer& buffer);
    float processSpectralEntropy(const AudioBuffer& buffer);
    float processZeroCrossingRate(const AudioBuffer& buffer);
    float processSpectralCentroid(const AudioBuffer& buffer);
    float processMachineLearning(const AudioBuffer& buffer);
    float processEnsemble(const AudioBuffer& buffer);

    // TODO 2.4.35: Adaptive Processing and Optimization
    // -------------------------------------------------
    /**
     * TODO: Implement adaptive processing and optimization with:
     * [ ] Automatic threshold adaptation with environmental learning
     * [ ] Noise level estimation and compensation with real-time updates
     * [ ] Performance optimization with algorithm selection and tuning
     * [ ] Context awareness with speech pattern analysis
     * [ ] Quality-based parameter adjustment with feedback loops
     * [ ] Environmental adaptation with acoustic scene analysis
     * [ ] User-specific calibration with personalized model training
     * [ ] Long-term learning with statistical model updates
     * [ ] Resource optimization with CPU and memory management
     * [ ] Real-time adaptation with immediate parameter updates
     */
    bool enableAdaptiveMode(bool enabled);
    bool calibrateToEnvironment(uint32_t calibrationTimeMs);
    bool adaptToUser(const std::vector<VADResult>& trainingData);
    float estimateNoiseLevel(const AudioBuffer& buffer);
    bool optimizePerformance();

    // TODO 2.4.36: Statistics and Monitoring
    // --------------------------------------
    /**
     * TODO: Implement comprehensive statistics and monitoring with:
     * [ ] Real-time performance metrics with minimal overhead
     * [ ] Accuracy measurement with ground truth comparison
     * [ ] Trend analysis with pattern recognition
     * [ ] Quality assessment with reliability scoring
     * [ ] Error tracking with categorization and analysis
     * [ ] Resource usage monitoring with optimization recommendations
     * [ ] Historical data analysis with long-term trends
     * [ ] Performance regression detection with alerting
     * [ ] Comparative analysis with algorithm performance
     * [ ] Reporting with user-friendly visualizations
     */
    VADStatistics getStatistics() const;
    void resetStatistics();
    bool exportStatistics(const std::string& filename) const;
    float getAccuracy() const;
    float getCurrentConfidence() const;
    bool isPerformingWell() const;

    // TODO 2.4.37: Callback and Event System
    // --------------------------------------
    /**
     * TODO: Implement comprehensive callback and event system with:
     * [ ] Thread-safe callback registration with validation
     * [ ] Event-driven notifications with rate limiting
     * [ ] Custom event creation with user-defined triggers
     * [ ] Callback error handling with exception safety
     * [ ] Performance monitoring for callback execution
     * [ ] Event filtering with subscription management
     * [ ] Asynchronous callback execution with queuing
     * [ ] Integration with external event systems
     * [ ] Debugging support with callback tracing
     * [ ] Resource management with proper cleanup
     */
    using DetectionCallback = std::function<void(const VADResult&)>;
    using StateChangeCallback = std::function<void(bool voiceDetected, float confidence)>;
    using ErrorCallback = std::function<void(int errorCode, const std::string& message)>;

    void setDetectionCallback(DetectionCallback callback);
    void setStateChangeCallback(StateChangeCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void clearCallbacks();

    // TODO 2.4.38: Configuration and Parameter Management
    // ---------------------------------------------------
    /**
     * TODO: Implement comprehensive configuration management with:
     * [ ] Dynamic parameter updates with validation and immediate application
     * [ ] Configuration presets with optimized settings for different scenarios
     * [ ] Parameter range validation with detailed error reporting
     * [ ] Configuration persistence with save and load functionality
     * [ ] Parameter optimization with automatic tuning algorithms
     * [ ] Configuration templates with scenario-based recommendations
     * [ ] Real-time parameter monitoring with change tracking
     * [ ] Configuration rollback with previous state restoration
     * [ ] Parameter dependency validation with cross-parameter checking
     * [ ] Configuration migration with version compatibility
     */
    bool setThreshold(float threshold);
    bool setHangTime(uint32_t hangTimeMs);
    bool setAlgorithm(VADConfig::Algorithm algorithm);
    bool enableNoiseCompensation(bool enabled);
    bool setAdaptationRate(float rate);

    VADConfig createOptimizedConfig(const AudioConfig& audioConfig) const;
    std::vector<VADConfig> getConfigurationPresets() const;
    bool saveConfiguration(const std::string& filename) const;
    bool loadConfiguration(const std::string& filename);

    // TODO 2.4.39: Testing and Validation Support
    // -------------------------------------------
    /**
     * TODO: Implement comprehensive testing and validation support with:
     * [ ] Ground truth comparison with accuracy measurement
     * [ ] Performance benchmarking with timing and resource analysis
     * [ ] Algorithm validation with statistical significance testing
     * [ ] Parameter sensitivity analysis with optimization recommendations
     * [ ] Stress testing with high-load scenarios and edge cases
     * [ ] Memory leak testing with allocation tracking and verification
     * [ ] Thread safety testing with race condition detection
     * [ ] Error handling testing with fault injection and recovery
     * [ ] Integration testing with audio processing pipeline validation
     * [ ] Regression testing with automated test suite execution
     */
    bool validateAccuracy(const std::vector<AudioBuffer>& testData,
                          const std::vector<bool>& groundTruth,
                          float& accuracy,
                          float& precision,
                          float& recall) const;
    bool benchmarkPerformance(const AudioBuffer& testBuffer,
                              uint32_t iterations,
                              float& averageTime,
                              float& maxTime) const;
    bool runSelfTest() const;

    // TODO 2.4.40: Error Handling and Diagnostics
    // -------------------------------------------
    /**
     * TODO: Implement comprehensive error handling and diagnostics with:
     * [ ] Detailed error reporting with context and diagnostic information
     * [ ] Error categorization with severity levels and recovery options
     * [ ] Diagnostic information collection with system state capture
     * [ ] Error recovery mechanisms with automatic and manual options
     * [ ] Performance impact analysis for error handling overhead
     * [ ] Integration with external error reporting systems
     * [ ] Error prevention with proactive monitoring and validation
     * [ ] User-friendly error messages with actionable guidance
     * [ ] Developer debugging support with detailed logging
     * [ ] Error pattern analysis with trend identification
     */
    struct ErrorInfo {
        int code;
        std::string message;
        std::string details;
        std::chrono::steady_clock::time_point timestamp;
        std::string component;
    };

    ErrorInfo getLastError() const;
    void clearErrors();
    std::string getDiagnosticInfo() const;
    bool isHealthy() const;

  private:
    // TODO 2.4.41: Internal State and Data Members
    // --------------------------------------------
    /**
     * TODO: Define comprehensive internal state with:
     * [ ] Configuration storage with thread-safe access
     * [ ] Algorithm state with optimized data structures
     * [ ] Performance metrics with atomic operations
     * [ ] Statistical data with efficient accumulation
     * [ ] Error tracking with detailed logging
     * [ ] Memory management with leak detection
     * [ ] Thread synchronization with minimal contention
     * [ ] Platform-specific optimizations with capability detection
     * [ ] Integration interfaces with external systems
     * [ ] Resource monitoring with usage tracking
     */

    // Configuration and State
    VADConfig config_;
    std::atomic<bool> initialized_;
    mutable std::mutex configMutex_;

    // Algorithm State
    std::unique_ptr<class EnergyDetector> energyDetector_;
    std::unique_ptr<class SpectralAnalyzer> spectralAnalyzer_;
    std::unique_ptr<class ZCRAnalyzer> zcrAnalyzer_;
    std::unique_ptr<class CentroidAnalyzer> centroidAnalyzer_;
    std::unique_ptr<class MLDetector> mlDetector_;
    std::unique_ptr<class EnsembleProcessor> ensembleProcessor_;

    // Processing State
    std::vector<float> analysisBuffer_;
    std::vector<float> spectralBuffer_;
    std::unique_ptr<class FFTProcessor> fftProcessor_;

    // Adaptive Processing
    float adaptiveThreshold_;
    float noiseFloor_;
    std::vector<float> noiseHistory_;
    std::chrono::steady_clock::time_point lastAdaptation_;

    // Statistics and Monitoring
    mutable std::mutex statisticsMutex_;
    VADStatistics statistics_;
    std::vector<VADResult> recentResults_;
    std::chrono::steady_clock::time_point startTime_;

    // Performance Monitoring
    mutable std::mutex performanceMutex_;
    std::chrono::high_resolution_clock::time_point lastProcessingTime_;
    std::vector<float> processingTimes_;

    // Callbacks
    mutable std::mutex callbackMutex_;
    DetectionCallback detectionCallback_;
    StateChangeCallback stateChangeCallback_;
    ErrorCallback errorCallback_;

    // Error Handling
    mutable ErrorInfo lastError_;

    // TODO 2.4.42: Internal Processing Methods
    // ---------------------------------------
    /**
     * TODO: Implement internal processing methods with:
     * [ ] Buffer preprocessing with noise reduction and normalization
     * [ ] Feature extraction with optimized algorithms
     * [ ] Decision making with ensemble methods
     * [ ] Post-processing with smoothing and validation
     * [ ] Performance optimization with SIMD and threading
     * [ ] Memory management with efficient allocation and reuse
     * [ ] Error handling with comprehensive logging and recovery
     * [ ] Quality assessment with reliability measurement
     * [ ] State management with proper synchronization
     * [ ] Integration with external processing libraries
     */
    bool preprocessBuffer(const AudioBuffer& input, std::vector<float>& output);
    bool extractFeatures(const std::vector<float>& buffer,
                         float& energy,
                         float& spectralEntropy,
                         float& zcr,
                         float& spectralCentroid);
    bool makeDecision(
        float energy, float spectralEntropy, float zcr, float spectralCentroid, VADResult& result);
    void updateStatistics(const VADResult& result);
    void updateAdaptiveThreshold(const VADResult& result);
    bool validateConfiguration(const VADConfig& config, std::string& errorMessage) const;
    void handleError(int code, const std::string& message, const std::string& details = "");

    // TODO 2.4.43: Algorithm Implementation Helpers
    // ---------------------------------------------
    /**
     * TODO: Implement algorithm-specific helper methods with:
     * [ ] Energy calculation with multiple window functions
     * [ ] Spectral analysis with FFT optimization
     * [ ] Zero crossing analysis with noise compensation
     * [ ] Centroid calculation with harmonic weighting
     * [ ] Feature normalization with adaptive scaling
     * [ ] Decision fusion with weighted combining
     * [ ] Performance optimization with vectorized operations
     * [ ] Error handling with algorithm-specific recovery
     * [ ] Parameter tuning with automatic optimization
     * [ ] Integration with machine learning frameworks
     */
    float calculateEnergy(const std::vector<float>& buffer);
    float calculateSpectralEntropy(const std::vector<float>& spectrum);
    float calculateZeroCrossingRate(const std::vector<float>& buffer);
    float calculateSpectralCentroid(const std::vector<float>& spectrum);
    float normalizeFeature(float value, float min, float max);
    float combineAlgorithmResults(const VADResult::AlgorithmResults& results);

    // TODO 2.4.44: Utility and Helper Functions
    // -----------------------------------------
    /**
     * TODO: Implement utility and helper functions with:
     * [ ] Configuration validation with parameter range checking
     * [ ] Default parameter generation with intelligent defaults
     * [ ] Performance measurement with high-resolution timing
     * [ ] Memory usage calculation with detailed breakdown
     * [ ] String formatting with structured output
     * [ ] File I/O operations with error handling
     * [ ] Mathematical utilities with optimized implementations
     * [ ] Platform detection with capability assessment
     * [ ] Testing utilities with mock data generation
     * [ ] Integration helpers with external system support
     */
    static VADConfig createDefaultConfig();
    static bool isValidThreshold(float threshold);
    static bool isValidHangTime(uint32_t hangTime);
    static std::string formatDiagnosticInfo(const VADStatistics& stats, const VADConfig& config);

    // Prevent copy construction and assignment
    VoiceActivityDetector(const VoiceActivityDetector&) = delete;
    VoiceActivityDetector& operator=(const VoiceActivityDetector&) = delete;
};

// TODO 2.4.45: Utility Functions and Factory Methods
// --------------------------------------------------
/**
 * TODO: Implement comprehensive utility functions with:
 * [ ] Default configuration creation with scenario-based optimization
 * [ ] Configuration validation with detailed error reporting
 * [ ] Performance benchmarking with comparative analysis
 * [ ] Testing utilities with ground truth generation
 * [ ] Integration helpers with audio processing pipeline
 * [ ] Parameter optimization with automatic tuning algorithms
 * [ ] Platform-specific optimizations with capability detection
 * [ ] Error handling utilities with structured logging
 * [ ] Documentation utilities with example generation
 * [ ] Migration utilities with version compatibility support
 */

/**
 * @brief Create optimized VAD configuration for different scenarios
 */
VADConfig createVADConfigForScenario(const std::string& scenario, const AudioConfig& audioConfig);

/**
 * @brief Validate VAD configuration with detailed error reporting
 */
bool validateVADConfig(const VADConfig& config, std::string& errorMessage);

/**
 * @brief Create default VAD configuration with intelligent defaults
 */
VADConfig createDefaultVADConfig();

/**
 * @brief Benchmark VAD performance with different configurations
 */
struct VADPerformanceBenchmark {
    VADConfig config;
    float averageProcessingTime;
    float maxProcessingTime;
    float accuracy;
    float memoryUsage;
    std::string notes;
};

std::vector<VADPerformanceBenchmark>
benchmarkVADPerformance(const std::vector<VADConfig>& configs,
                        const std::vector<AudioBuffer>& testData,
                        const std::vector<bool>& groundTruth);

}  // namespace core
}  // namespace huntmaster
