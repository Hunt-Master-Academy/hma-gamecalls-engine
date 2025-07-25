/**
 * @fileoverview Waveform Module Index - Exports all waveform analysis modules
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Central export point for all waveform analysis and visualization modules
 * providing a clean API for importing specific components or the entire suite.
 *
 * Available Modules:
 * - WaveformAnalysis: Real-time waveform analysis with FFT and feature extraction
 * - WaveformVisualization: Advanced visualization rendering with WebGL support
 * - WaveformNavigation: Interactive navigation and controls
 * - WaveformFeatures: Comprehensive audio feature extraction
 * - WaveformPerformance: Performance monitoring and optimization
 * - WaveformUtils: Utility functions and helper methods
 * - WaveformProcessor: Advanced DSP processing with Web Workers support
 * - WaveformInteraction: Comprehensive user interaction handling
 * - WaveformData: Data management and storage with multi-resolution support
 * - WaveformSettings: Settings and configuration management
 * - WaveformExport: Export functionality for multiple formats
 *
 * @example
 * ```javascript
 * // Import specific modules
 * import { WaveformAnalysis, WaveformVisualization } from './modules/waveform/index.js';
 *
 * // Or import all modules
 * import * as WaveformModules from './modules/waveform/index.js';
 *
 * const analyzer = new WaveformAnalysis();
 * const visualizer = new WaveformVisualization({ canvas: canvasElement });
 * ```
 */

// Import all waveform modules
import WaveformAnalysis from "./waveform-analysis.js";
import WaveformVisualization from "./waveform-visualization.js";
import WaveformNavigation from "./waveform-navigation.js";
import WaveformFeatures from "./waveform-features.js";
import WaveformPerformance from "./waveform-performance.js";
import WaveformUtils from "./waveform-utils.js";
import WaveformProcessor from "./waveform-processor.js";
import WaveformInteraction from "./waveform-interaction.js";
import WaveformData from "./waveform-data.js";
import WaveformSettings from "./waveform-settings.js";
import WaveformExport from "./waveform-export.js";

// Export individual modules
export {
  WaveformAnalysis,
  WaveformVisualization,
  WaveformNavigation,
  WaveformFeatures,
  WaveformPerformance,
  WaveformUtils,
  WaveformProcessor,
  WaveformInteraction,
  WaveformData,
  WaveformSettings,
  WaveformExport,
};

// Export as default object for convenience
export default {
  WaveformAnalysis,
  WaveformVisualization,
  WaveformNavigation,
  WaveformFeatures,
  WaveformPerformance,
  WaveformUtils,
};

/**
 * Module version information
 */
export const WAVEFORM_MODULE_VERSION = "1.0.0";

/**
 * Module metadata
 */
export const WAVEFORM_MODULE_INFO = {
  version: WAVEFORM_MODULE_VERSION,
  modules: [
    "WaveformAnalysis",
    "WaveformVisualization",
    "WaveformNavigation",
    "WaveformFeatures",
    "WaveformPerformance",
    "WaveformUtils",
  ],
  description: "Comprehensive waveform analysis and visualization module suite",
  author: "Huntmaster Development Team",
  created: "2024-01-20",
};
