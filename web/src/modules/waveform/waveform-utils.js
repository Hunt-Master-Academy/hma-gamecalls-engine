/**
 * @fileoverview Waveform Utilities Module - Utility Methods and Helpers
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Provides comprehensive utility functions and helper methods for waveform
 * processing, mathematical operations, data conversion, and common tasks.
 *
 * Key Features:
 * - Mathematical utilities for DSP operations
 * - Data format conversion and validation
 * - Color manipulation and theme utilities
 * - File format handling and export capabilities
 * - Statistical analysis functions
 * - Performance optimization helpers
 *
 * Dependencies:
 * - Standard JavaScript Math library
 * - Web APIs for file handling
 * - Canvas API for color operations
 *
 * @example
 * ```javascript
 * import { WaveformUtils } from './modules/waveform/waveform-utils.js';
 *
 * const utils = new WaveformUtils();
 * const normalized = utils.normalizeArray(audioData);
 * const color = utils.interpolateColor('#ff0000', '#0000ff', 0.5);
 * ```
 */

/**
 * Comprehensive Waveform Utility Collection
 *
 * Provides essential utility functions for waveform processing, data manipulation,
 * mathematical operations, and common helper tasks used throughout the system.
 *
 * @class WaveformUtils
 */
export class WaveformUtils {
  /**
   * Create a WaveformUtils instance
   *
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableCaching=true] - Enable result caching
   * @param {number} [options.precision=6] - Floating point precision
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      enableCaching: options.enableCaching !== false,
      precision: options.precision || 6,
      ...options,
    };

    // Utility cache for expensive operations
    this.cache = new Map();

    // Mathematical constants
    this.constants = {
      PI: Math.PI,
      TWO_PI: 2 * Math.PI,
      HALF_PI: Math.PI / 2,
      E: Math.E,
      LN2: Math.LN2,
      LN10: Math.LN10,
      LOG2E: Math.LOG2E,
      LOG10E: Math.LOG10E,
      SQRT2: Math.SQRT2,
      SQRT1_2: Math.SQRT1_2,
    };

    // Common color palettes
    this.colorPalettes = {
      viridis: [
        "#440154",
        "#482777",
        "#3f4a8a",
        "#31678e",
        "#26838f",
        "#1f9d8a",
        "#6cce5a",
        "#b6de2b",
        "#fee825",
      ],
      plasma: [
        "#0c0786",
        "#40039c",
        "#6a00a7",
        "#8f0da4",
        "#b02a8f",
        "#ca4c79",
        "#df7767",
        "#f0a58e",
        "#f7d23c",
      ],
      inferno: [
        "#000003",
        "#1b0c41",
        "#4a0c6b",
        "#781c6d",
        "#a52c60",
        "#cf4446",
        "#ed6925",
        "#fb9b06",
        "#f7d03c",
      ],
      cool: ["#00ffff", "#0080ff", "#0000ff", "#8000ff", "#ff00ff"],
      hot: ["#000000", "#800000", "#ff0000", "#ff8000", "#ffff00", "#ffffff"],
      grayscale: ["#000000", "#404040", "#808080", "#c0c0c0", "#ffffff"],
    };

    this._initialize();
  }

  /**
   * Initialize utility system
   * @private
   */
  _initialize() {
    // Pre-calculate common values for performance
    this._precalculateCommonValues();
  }

  /**
   * Pre-calculate commonly used values
   * @private
   */
  _precalculateCommonValues() {
    // Common window functions
    this.windowFunctions = {
      rectangular: (n, N) => 1,
      hann: (n, N) =>
        0.5 * (1 - Math.cos((this.constants.TWO_PI * n) / (N - 1))),
      hamming: (n, N) =>
        0.54 - 0.46 * Math.cos((this.constants.TWO_PI * n) / (N - 1)),
      blackman: (n, N) =>
        0.42 -
        0.5 * Math.cos((this.constants.TWO_PI * n) / (N - 1)) +
        0.08 * Math.cos((4 * this.constants.PI * n) / (N - 1)),
      kaiser: (n, N, beta = 8.6) =>
        this._modifiedBesselI0(
          beta * Math.sqrt(1 - Math.pow((2 * n) / (N - 1) - 1, 2))
        ) / this._modifiedBesselI0(beta),
    };
  }

  // === MATHEMATICAL UTILITIES ===

  /**
   * Normalize array values to range [0, 1]
   *
   * @param {Array|Float32Array} array - Input array
   * @param {Object} [options={}] - Normalization options
   * @returns {Float32Array} Normalized array
   */
  normalizeArray(array, options = {}) {
    const { min = null, max = null, method = "minmax" } = options;

    const cacheKey = `normalize_${method}_${array.length}_${JSON.stringify(
      options
    )}`;
    if (this.config.enableCaching && this.cache.has(cacheKey)) {
      return this.cache.get(cacheKey);
    }

    const result = new Float32Array(array.length);

    let arrayMin = min;
    let arrayMax = max;

    // Calculate min/max if not provided
    if (arrayMin === null || arrayMax === null) {
      arrayMin = arrayMax = array[0];
      for (let i = 1; i < array.length; i++) {
        if (array[i] < arrayMin) arrayMin = array[i];
        if (array[i] > arrayMax) arrayMax = array[i];
      }
    }

    const range = arrayMax - arrayMin;

    switch (method) {
      case "minmax":
        if (range === 0) {
          result.fill(0.5);
        } else {
          for (let i = 0; i < array.length; i++) {
            result[i] = (array[i] - arrayMin) / range;
          }
        }
        break;

      case "zscore":
        const mean = this.calculateMean(array);
        const std = this.calculateStandardDeviation(array, mean);
        if (std === 0) {
          result.fill(0);
        } else {
          for (let i = 0; i < array.length; i++) {
            result[i] = (array[i] - mean) / std;
          }
        }
        break;

      case "robust":
        const median = this.calculateMedian(array);
        const mad = this.calculateMAD(array, median);
        if (mad === 0) {
          result.fill(0);
        } else {
          for (let i = 0; i < array.length; i++) {
            result[i] = (array[i] - median) / (1.4826 * mad);
          }
        }
        break;
    }

    if (this.config.enableCaching) {
      this.cache.set(cacheKey, result);
    }

    return result;
  }

  /**
   * Calculate mean of array
   *
   * @param {Array|Float32Array} array - Input array
   * @returns {number} Mean value
   */
  calculateMean(array) {
    let sum = 0;
    for (let i = 0; i < array.length; i++) {
      sum += array[i];
    }
    return sum / array.length;
  }

  /**
   * Calculate standard deviation
   *
   * @param {Array|Float32Array} array - Input array
   * @param {number} [mean] - Pre-calculated mean (optional)
   * @returns {number} Standard deviation
   */
  calculateStandardDeviation(array, mean = null) {
    if (mean === null) {
      mean = this.calculateMean(array);
    }

    let sumSquaredDiffs = 0;
    for (let i = 0; i < array.length; i++) {
      const diff = array[i] - mean;
      sumSquaredDiffs += diff * diff;
    }

    return Math.sqrt(sumSquaredDiffs / (array.length - 1));
  }

  /**
   * Calculate median value
   *
   * @param {Array|Float32Array} array - Input array
   * @returns {number} Median value
   */
  calculateMedian(array) {
    const sorted = Array.from(array).sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);

    return sorted.length % 2 === 0
      ? (sorted[mid - 1] + sorted[mid]) / 2
      : sorted[mid];
  }

  /**
   * Calculate Median Absolute Deviation (MAD)
   *
   * @param {Array|Float32Array} array - Input array
   * @param {number} [median] - Pre-calculated median (optional)
   * @returns {number} MAD value
   */
  calculateMAD(array, median = null) {
    if (median === null) {
      median = this.calculateMedian(array);
    }

    const deviations = new Array(array.length);
    for (let i = 0; i < array.length; i++) {
      deviations[i] = Math.abs(array[i] - median);
    }

    return this.calculateMedian(deviations);
  }

  /**
   * Apply window function to array
   *
   * @param {Array|Float32Array} array - Input array
   * @param {string} windowType - Window function type
   * @param {Object} [options={}] - Window options
   * @returns {Float32Array} Windowed array
   */
  applyWindow(array, windowType = "hann", options = {}) {
    const windowFunc = this.windowFunctions[windowType];
    if (!windowFunc) {
      throw new Error(`Unknown window type: ${windowType}`);
    }

    const result = new Float32Array(array.length);
    const N = array.length;

    for (let n = 0; n < N; n++) {
      const windowValue = windowFunc(n, N, options.beta);
      result[n] = array[n] * windowValue;
    }

    return result;
  }

  /**
   * Modified Bessel function I0 (for Kaiser window)
   * @private
   */
  _modifiedBesselI0(x) {
    let sum = 1;
    let term = 1;
    const xSquaredOver4 = (x * x) / 4;

    for (let k = 1; k <= 50; k++) {
      term *= xSquaredOver4 / (k * k);
      sum += term;

      if (term < 1e-12) break;
    }

    return sum;
  }

  /**
   * Interpolate between two values
   *
   * @param {number} start - Start value
   * @param {number} end - End value
   * @param {number} t - Interpolation parameter [0, 1]
   * @param {string} [method='linear'] - Interpolation method
   * @returns {number} Interpolated value
   */
  interpolate(start, end, t, method = "linear") {
    t = Math.max(0, Math.min(1, t)); // Clamp to [0, 1]

    switch (method) {
      case "linear":
        return start + (end - start) * t;

      case "cosine":
        const ft = (1 - Math.cos(t * this.constants.PI)) * 0.5;
        return start + (end - start) * ft;

      case "cubic":
        const t2 = t * t;
        const t3 = t2 * t;
        return start + (end - start) * (3 * t2 - 2 * t3);

      case "quintic":
        const t3q = t * t * t;
        return start + (end - start) * (t3q * (6 * t * t - 15 * t + 10));

      default:
        return this.interpolate(start, end, t, "linear");
    }
  }

  /**
   * Resample array to new length
   *
   * @param {Array|Float32Array} array - Input array
   * @param {number} newLength - Target length
   * @param {string} [method='linear'] - Interpolation method
   * @returns {Float32Array} Resampled array
   */
  resampleArray(array, newLength, method = "linear") {
    if (array.length === newLength) {
      return new Float32Array(array);
    }

    const result = new Float32Array(newLength);
    const ratio = (array.length - 1) / (newLength - 1);

    for (let i = 0; i < newLength; i++) {
      const srcIndex = i * ratio;
      const srcIndexFloor = Math.floor(srcIndex);
      const srcIndexCeil = Math.min(srcIndexFloor + 1, array.length - 1);
      const t = srcIndex - srcIndexFloor;

      result[i] = this.interpolate(
        array[srcIndexFloor],
        array[srcIndexCeil],
        t,
        method
      );
    }

    return result;
  }

  // === COLOR UTILITIES ===

  /**
   * Convert hex color to RGB
   *
   * @param {string} hex - Hex color string
   * @returns {Object} RGB values {r, g, b}
   */
  hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result
      ? {
          r: parseInt(result[1], 16),
          g: parseInt(result[2], 16),
          b: parseInt(result[3], 16),
        }
      : null;
  }

  /**
   * Convert RGB to hex color
   *
   * @param {number} r - Red component [0-255]
   * @param {number} g - Green component [0-255]
   * @param {number} b - Blue component [0-255]
   * @returns {string} Hex color string
   */
  rgbToHex(r, g, b) {
    return (
      "#" +
      Math.round(r).toString(16).padStart(2, "0") +
      Math.round(g).toString(16).padStart(2, "0") +
      Math.round(b).toString(16).padStart(2, "0")
    );
  }

  /**
   * Convert HSL to RGB
   *
   * @param {number} h - Hue [0-360]
   * @param {number} s - Saturation [0-1]
   * @param {number} l - Lightness [0-1]
   * @returns {Object} RGB values {r, g, b}
   */
  hslToRgb(h, s, l) {
    h = h / 360;
    const c = (1 - Math.abs(2 * l - 1)) * s;
    const x = c * (1 - Math.abs(((h * 6) % 2) - 1));
    const m = l - c / 2;

    let r, g, b;

    if (h < 1 / 6) {
      [r, g, b] = [c, x, 0];
    } else if (h < 2 / 6) {
      [r, g, b] = [x, c, 0];
    } else if (h < 3 / 6) {
      [r, g, b] = [0, c, x];
    } else if (h < 4 / 6) {
      [r, g, b] = [0, x, c];
    } else if (h < 5 / 6) {
      [r, g, b] = [x, 0, c];
    } else {
      [r, g, b] = [c, 0, x];
    }

    return {
      r: Math.round((r + m) * 255),
      g: Math.round((g + m) * 255),
      b: Math.round((b + m) * 255),
    };
  }

  /**
   * Interpolate between two colors
   *
   * @param {string} color1 - Start color (hex)
   * @param {string} color2 - End color (hex)
   * @param {number} t - Interpolation parameter [0, 1]
   * @returns {string} Interpolated color (hex)
   */
  interpolateColor(color1, color2, t) {
    const rgb1 = this.hexToRgb(color1);
    const rgb2 = this.hexToRgb(color2);

    if (!rgb1 || !rgb2) {
      throw new Error("Invalid color format");
    }

    const r = this.interpolate(rgb1.r, rgb2.r, t);
    const g = this.interpolate(rgb1.g, rgb2.g, t);
    const b = this.interpolate(rgb1.b, rgb2.b, t);

    return this.rgbToHex(r, g, b);
  }

  /**
   * Generate color palette
   *
   * @param {string} paletteName - Palette name
   * @param {number} steps - Number of color steps
   * @returns {Array<string>} Array of hex colors
   */
  generateColorPalette(paletteName, steps) {
    const basePalette = this.colorPalettes[paletteName];
    if (!basePalette) {
      throw new Error(`Unknown palette: ${paletteName}`);
    }

    if (steps <= basePalette.length) {
      return basePalette.slice(0, steps);
    }

    const result = [];
    const segmentSize = (basePalette.length - 1) / (steps - 1);

    for (let i = 0; i < steps; i++) {
      const position = i * segmentSize;
      const index1 = Math.floor(position);
      const index2 = Math.min(index1 + 1, basePalette.length - 1);
      const t = position - index1;

      result.push(
        this.interpolateColor(basePalette[index1], basePalette[index2], t)
      );
    }

    return result;
  }

  /**
   * Map value to color using palette
   *
   * @param {number} value - Value to map [0, 1]
   * @param {string} paletteName - Palette name
   * @returns {string} Hex color
   */
  valueToColor(value, paletteName = "viridis") {
    const palette = this.colorPalettes[paletteName];
    if (!palette) {
      throw new Error(`Unknown palette: ${paletteName}`);
    }

    value = Math.max(0, Math.min(1, value)); // Clamp to [0, 1]

    const position = value * (palette.length - 1);
    const index1 = Math.floor(position);
    const index2 = Math.min(index1 + 1, palette.length - 1);
    const t = position - index1;

    return this.interpolateColor(palette[index1], palette[index2], t);
  }

  // === DATA CONVERSION UTILITIES ===

  /**
   * Convert audio buffer to different format
   *
   * @param {Float32Array} audioBuffer - Input audio buffer
   * @param {string} format - Target format ('int16', 'int32', 'float64')
   * @returns {TypedArray} Converted buffer
   */
  convertAudioFormat(audioBuffer, format) {
    switch (format.toLowerCase()) {
      case "int16":
        const int16Buffer = new Int16Array(audioBuffer.length);
        for (let i = 0; i < audioBuffer.length; i++) {
          int16Buffer[i] = Math.max(
            -32768,
            Math.min(32767, Math.round(audioBuffer[i] * 32767))
          );
        }
        return int16Buffer;

      case "int32":
        const int32Buffer = new Int32Array(audioBuffer.length);
        for (let i = 0; i < audioBuffer.length; i++) {
          int32Buffer[i] = Math.max(
            -2147483648,
            Math.min(2147483647, Math.round(audioBuffer[i] * 2147483647))
          );
        }
        return int32Buffer;

      case "float64":
        return new Float64Array(audioBuffer);

      default:
        return new Float32Array(audioBuffer);
    }
  }

  /**
   * Validate audio buffer
   *
   * @param {*} buffer - Buffer to validate
   * @returns {Object} Validation result
   */
  validateAudioBuffer(buffer) {
    const result = {
      isValid: false,
      type: null,
      length: 0,
      issues: [],
    };

    // Check if buffer exists
    if (!buffer) {
      result.issues.push("Buffer is null or undefined");
      return result;
    }

    // Check if buffer is typed array
    if (
      !(
        buffer instanceof Float32Array ||
        buffer instanceof Float64Array ||
        buffer instanceof Int16Array ||
        buffer instanceof Int32Array
      )
    ) {
      result.issues.push("Buffer is not a typed array");
      return result;
    }

    result.type = buffer.constructor.name;
    result.length = buffer.length;

    // Check length
    if (buffer.length === 0) {
      result.issues.push("Buffer is empty");
      return result;
    }

    // Check for NaN or Infinity
    let hasNaN = false;
    let hasInfinity = false;
    let minValue = buffer[0];
    let maxValue = buffer[0];

    for (let i = 0; i < buffer.length; i++) {
      const value = buffer[i];

      if (isNaN(value)) {
        hasNaN = true;
      } else if (!isFinite(value)) {
        hasInfinity = true;
      } else {
        minValue = Math.min(minValue, value);
        maxValue = Math.max(maxValue, value);
      }
    }

    if (hasNaN) {
      result.issues.push("Buffer contains NaN values");
    }

    if (hasInfinity) {
      result.issues.push("Buffer contains Infinity values");
    }

    // For floating point, check if values are in reasonable range
    if (buffer instanceof Float32Array || buffer instanceof Float64Array) {
      if (maxValue > 10 || minValue < -10) {
        result.issues.push("Float buffer values outside typical range [-1, 1]");
      }
    }

    result.isValid = result.issues.length === 0;
    result.minValue = minValue;
    result.maxValue = maxValue;

    return result;
  }

  // === FILE UTILITIES ===

  /**
   * Export data as JSON file
   *
   * @param {*} data - Data to export
   * @param {string} filename - Output filename
   * @param {Object} [options={}] - Export options
   */
  exportAsJSON(data, filename, options = {}) {
    const { indent = 2, replacer = null } = options;

    try {
      const jsonString = JSON.stringify(data, replacer, indent);
      const blob = new Blob([jsonString], { type: "application/json" });
      this._downloadBlob(blob, filename);
    } catch (error) {
      console.error("JSON export failed:", error);
      throw error;
    }
  }

  /**
   * Export audio buffer as WAV file
   *
   * @param {Float32Array} audioBuffer - Audio data
   * @param {number} sampleRate - Sample rate
   * @param {string} filename - Output filename
   */
  exportAsWAV(audioBuffer, sampleRate, filename) {
    const length = audioBuffer.length;
    const buffer = new ArrayBuffer(44 + length * 2);
    const view = new DataView(buffer);

    // WAV header
    const writeString = (offset, string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };

    writeString(0, "RIFF");
    view.setUint32(4, 36 + length * 2, true);
    writeString(8, "WAVE");
    writeString(12, "fmt ");
    view.setUint32(16, 16, true);
    view.setUint16(20, 1, true);
    view.setUint16(22, 1, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * 2, true);
    view.setUint16(32, 2, true);
    view.setUint16(34, 16, true);
    writeString(36, "data");
    view.setUint32(40, length * 2, true);

    // Convert float to 16-bit PCM
    let offset = 44;
    for (let i = 0; i < length; i++) {
      const sample = Math.max(-1, Math.min(1, audioBuffer[i]));
      view.setInt16(offset, sample * 0x7fff, true);
      offset += 2;
    }

    const blob = new Blob([buffer], { type: "audio/wav" });
    this._downloadBlob(blob, filename);
  }

  /**
   * Download blob as file
   * @private
   */
  _downloadBlob(blob, filename) {
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  }

  // === PERFORMANCE UTILITIES ===

  /**
   * Debounce function execution
   *
   * @param {Function} func - Function to debounce
   * @param {number} delay - Delay in milliseconds
   * @returns {Function} Debounced function
   */
  debounce(func, delay) {
    let timeoutId;
    return function (...args) {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
  }

  /**
   * Throttle function execution
   *
   * @param {Function} func - Function to throttle
   * @param {number} limit - Limit in milliseconds
   * @returns {Function} Throttled function
   */
  throttle(func, limit) {
    let inThrottle;
    return function (...args) {
      if (!inThrottle) {
        func.apply(this, args);
        inThrottle = true;
        setTimeout(() => (inThrottle = false), limit);
      }
    };
  }

  /**
   * Create batch processor for large datasets
   *
   * @param {Function} processor - Processing function
   * @param {number} batchSize - Batch size
   * @returns {Function} Batch processor
   */
  createBatchProcessor(processor, batchSize = 1000) {
    return async function (data, progressCallback = null) {
      const results = [];
      const totalBatches = Math.ceil(data.length / batchSize);

      for (let i = 0; i < totalBatches; i++) {
        const start = i * batchSize;
        const end = Math.min(start + batchSize, data.length);
        const batch = data.slice(start, end);

        const batchResult = await processor(batch, i);
        results.push(...batchResult);

        if (progressCallback) {
          progressCallback({
            batch: i + 1,
            totalBatches,
            progress: (i + 1) / totalBatches,
            processed: end,
            total: data.length,
          });
        }

        // Yield control to prevent blocking
        await new Promise((resolve) => setTimeout(resolve, 0));
      }

      return results;
    };
  }

  // === VALIDATION UTILITIES ===

  /**
   * Check if value is a valid number
   *
   * @param {*} value - Value to check
   * @returns {boolean} Is valid number
   */
  isValidNumber(value) {
    return typeof value === "number" && isFinite(value);
  }

  /**
   * Clamp value to range
   *
   * @param {number} value - Value to clamp
   * @param {number} min - Minimum value
   * @param {number} max - Maximum value
   * @returns {number} Clamped value
   */
  clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  /**
   * Round to specified precision
   *
   * @param {number} value - Value to round
   * @param {number} precision - Number of decimal places
   * @returns {number} Rounded value
   */
  roundToPrecision(value, precision = this.config.precision) {
    const factor = Math.pow(10, precision);
    return Math.round(value * factor) / factor;
  }

  /**
   * Format duration in human-readable format
   *
   * @param {number} seconds - Duration in seconds
   * @returns {string} Formatted duration
   */
  formatDuration(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);
    const ms = Math.floor((seconds % 1) * 1000);

    if (hours > 0) {
      return `${hours}:${minutes.toString().padStart(2, "0")}:${secs
        .toString()
        .padStart(2, "0")}`;
    } else if (minutes > 0) {
      return `${minutes}:${secs.toString().padStart(2, "0")}`;
    } else if (secs > 0) {
      return `${secs}.${ms.toString().padStart(3, "0").substr(0, 2)}s`;
    } else {
      return `${ms}ms`;
    }
  }

  /**
   * Generate UUID
   *
   * @returns {string} UUID string
   */
  generateUUID() {
    return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(
      /[xy]/g,
      function (c) {
        const r = (Math.random() * 16) | 0;
        const v = c === "x" ? r : (r & 0x3) | 0x8;
        return v.toString(16);
      }
    );
  }

  /**
   * Clear utility cache
   */
  clearCache() {
    this.cache.clear();
  }

  /**
   * Get cache statistics
   *
   * @returns {Object} Cache statistics
   */
  getCacheStats() {
    return {
      size: this.cache.size,
      enabled: this.config.enableCaching,
    };
  }
}

export default WaveformUtils;
