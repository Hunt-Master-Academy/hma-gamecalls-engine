/**
 * @file WaveformAnalyzer.h
 * @brief Advanced Waveform Analysis and Visualization System
 *
 * This header defines comprehensive waveform analysis for sophisticated
 * audio visualization, multi-resolution processing, and advanced analytics.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <kiss_fft.h>

// Forward declarations
namespace huntmaster {
namespace core {
class AudioBuffer;
struct AudioConfig;
}  // namespace core
using core::AudioBuffer;
using core::AudioConfig;
}  // namespace huntmaster

namespace huntmaster {

// Constants
constexpr int MAX_ZOOM_LEVELS = 16;
constexpr size_t DEFAULT_SAMPLES_PER_LEVEL = 4096;

// Enumerations
enum class WindowFunction { HANN, HAMMING, BLACKMAN };

// Data structures
struct ColorValue {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

struct SpectralPeak {
    float frequency = 0.0f;
    float magnitude = 0.0f;
    size_t bin_index = 0;
};

struct WaveformPeak {
    size_t sample_index = 0;
    float magnitude = 0.0f;
    float time = 0.0f;
    float prominence = 0.0f;
    float width = 0.0f;
    float refined_position = 0.0f;
    float refined_magnitude = 0.0f;
};

struct WaveformLevel {
    int level = 0;
    size_t samples_per_pixel = 1;
    size_t decimation_factor = 1;
    std::vector<float> min_samples;
    std::vector<float> max_samples;
    std::vector<float> rms_samples;
};

struct WaveformData {
    std::vector<float> min_values;
    std::vector<float> max_values;
    std::vector<float> rms_values;
    float start_time = 0.0f;
    float end_time = 0.0f;
    float sample_rate = 0.0f;
    int resolution_level = 0;
    size_t samples_per_pixel = 1;
    bool is_valid = false;
};

struct SpectrumData {
    std::vector<float> frequencies;
    std::vector<float> magnitudes;
    std::vector<float> phases;
    std::vector<SpectralPeak> peaks;
    float start_time = 0.0f;
    float duration = 0.0f;
    float sample_rate = 0.0f;
    size_t fft_size = 0;
    WindowFunction window_function = WindowFunction::HANN;
    float spectral_centroid = 0.0f;
    float spectral_bandwidth = 0.0f;
    bool is_valid = false;
};

struct WaveformStatistics {
    size_t peak_count = 0;
    float max_amplitude = 0.0f;
    float min_amplitude = 0.0f;
    float rms_level = 0.0f;
    float dynamic_range = 0.0f;
    float spectral_centroid = 0.0f;
    float spectral_bandwidth = 0.0f;
    float zero_crossing_rate = 0.0f;
};

struct PerformanceStatistics {
    double analysis_time = 0.0;
    double fft_time = 0.0;
    double peak_detection_time = 0.0;
    double color_mapping_time = 0.0;
    size_t memory_usage = 0;
};

/**
 * @class WaveformAnalyzer
 * @brief Advanced waveform analysis and visualization system
 *
 * This class provides comprehensive waveform analysis capabilities including:
 * - Multi-resolution waveform data generation with zoom and pan support
 * - Color-coding algorithms for similarity region visualization
 * - Frequency spectrum analysis for spectrogram display capabilities
 * - Peak detection and amplitude scaling algorithms
 * - Real-time waveform processing with efficient rendering optimization
 * - Cross-platform visualization data generation
 * - Memory-efficient waveform caching and management
 * - Thread-safe analysis with concurrent processing support
 * - Advanced filtering and signal conditioning
 * - Statistical analysis and feature extraction from waveforms
 */
class WaveformAnalyzer {
  public:
    /**
     * @brief Constructor
     * @param config Audio configuration settings
     */
    explicit WaveformAnalyzer(const AudioConfig& config);

    /**
     * @brief Destructor
     */
    ~WaveformAnalyzer();

    // Disable copy constructor and assignment operator
    WaveformAnalyzer(const WaveformAnalyzer&) = delete;
    WaveformAnalyzer& operator=(const WaveformAnalyzer&) = delete;

    /**
     * @brief Initialize the waveform analyzer
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Generate multi-resolution waveform data from audio buffer
     * @param audio_buffer Input audio buffer
     * @return true if generation was successful, false otherwise
     */
    bool generateWaveformData(const AudioBuffer& audio_buffer);

    /**
     * @brief Get waveform data for specified time range and display width
     * @param start_time Start time in seconds
     * @param end_time End time in seconds
     * @param target_width Target display width in pixels
     * @return WaveformData structure containing the requested data
     */
    WaveformData getWaveformData(float start_time, float end_time, int target_width) const;

    /**
     * @brief Analyze frequency spectrum of audio segment
     * @param audio_buffer Input audio buffer
     * @param start_time Start time in seconds
     * @param duration Duration in seconds
     * @return SpectrumData structure containing spectrum analysis results
     */
    SpectrumData analyzeSpectrum(const AudioBuffer& audio_buffer, float start_time, float duration);

    /**
     * @brief Generate similarity-based colors for visualization
     * @param similarity_values Vector of similarity values (0.0 to 1.0)
     * @return Vector of ColorValue structures for rendering
     */
    std::vector<ColorValue>
    generateSimilarityColors(const std::vector<float>& similarity_values) const;

    /**
     * @brief Detect peaks in waveform data
     * @param data Input waveform data
     * @param threshold Peak detection threshold (0.0 uses default)
     * @return Vector of detected peaks with metadata
     */
    std::vector<WaveformPeak> detectPeaks(const std::vector<float>& data,
                                          float threshold = 0.0f) const;

    /**
     * @brief Set window function for spectrum analysis
     * @param window_function Window function type
     */
    void setWindowFunction(WindowFunction window_function) {
        window_function_ = window_function;
    }

    /**
     * @brief Set spectrum analysis size
     * @param size FFT size (must be power of 2)
     */
    void setSpectrumSize(size_t size) {
        spectrum_size_ = size;
    }

    /**
     * @brief Set peak detection threshold
     * @param threshold Threshold value (0.0 to 1.0)
     */
    void setPeakThreshold(float threshold) {
        peak_threshold_ = threshold;
    }

    /**
     * @brief Set color sensitivity for visualization
     * @param sensitivity Sensitivity value (0.0 to 2.0)
     */
    void setColorSensitivity(float sensitivity) {
        color_sensitivity_ = sensitivity;
    }

    /**
     * @brief Enable or disable logarithmic scale for spectrum
     * @param enable true to enable log scale, false for linear
     */
    void setLogScale(bool enable) {
        use_log_scale_ = enable;
    }

    /**
     * @brief Get current waveform statistics
     * @return WaveformStatistics structure
     */
    const WaveformStatistics& getStatistics() const {
        return statistics_;
    }

    /**
     * @brief Get performance statistics
     * @return PerformanceStatistics structure
     */
    const PerformanceStatistics& getPerformanceStats() const {
        return performance_stats_;
    }

    /**
     * @brief Check if analyzer is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const {
        return is_initialized_;
    }

  private:
    // Core initialization methods
    bool initializeFFT();
    void initializeWindowFunctions();
    void initializeMemoryPools();
    void initializePerformanceMonitoring();
    bool validateConfiguration() const;

    // Waveform generation methods
    bool generateWaveformLevel(const AudioBuffer& audio_buffer, int level);
    int selectOptimalLevel(float start, float end, int width) const;

    // Spectrum analysis methods
    void findSpectralPeaks(SpectrumData& spectrum_data);
    void calculateSpectralFeatures(SpectrumData& data);

    // Peak detection methods
    float calculatePeakProminence(const std::vector<float>& data, size_t peak_index) const;
    float calculatePeakWidth(const std::vector<float>& data,
                             size_t peak_index,
                             float relative_threshold) const;

    // Color mapping methods
    ColorValue mapSimilarityToColor(float normalized_similarity) const;
    void initializeColorMap();

    // Utility methods
    void clearWaveformData();
    void updateWaveformStatistics();
    void resetStatistics();
    void cleanup();

    // Logging methods
    void console_log(const std::string& msg) const;
    void console_warn(const std::string& msg) const;
    void console_error(const std::string& msg) const;

  private:
    // Configuration
    const AudioConfig& config_;
    float sample_rate_;
    bool is_initialized_;

    // FFT resources (using KissFFT)
    kiss_fft_cfg fft_plan_;
    float* fft_input_;
    kiss_fft_cpx* fft_output_;

    // Analysis parameters
    WindowFunction window_function_;
    size_t spectrum_size_;
    float overlap_factor_;
    float zoom_level_;
    double pan_offset_;
    float color_sensitivity_;
    float peak_threshold_;
    bool use_log_scale_;
    bool enable_smoothing_;
    float smoothing_factor_;

    // Data storage
    std::vector<WaveformLevel> waveform_levels_;
    std::vector<float> analysis_buffer_;
    std::vector<float> windowed_buffer_;
    std::vector<float> spectrum_data_;
    std::vector<float> magnitude_spectrum_;
    std::vector<float> phase_spectrum_;
    std::vector<WaveformPeak> peaks_;
    std::vector<float> peak_magnitudes_;
    std::vector<ColorValue> similarity_colors_;

    // Window functions cache
    std::unordered_map<size_t, std::map<WindowFunction, std::vector<float>>> window_functions_;

    // Thread pool for parallel processing
    std::vector<std::thread> thread_pool_;

    // Statistics and performance tracking
    WaveformStatistics statistics_;
    PerformanceStatistics performance_stats_;

    // Audio data
    std::unique_ptr<AudioBuffer> original_audio_;
    float audio_duration_;
};

}  // namespace huntmaster
