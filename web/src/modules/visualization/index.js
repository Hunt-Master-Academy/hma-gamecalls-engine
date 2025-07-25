/**
 * @file visualization/index.js
 * @brief Data Visualization and Graphics Modules
 *
 * This index provides access to all visualization modules including:
 * - High-performance waveform rendering
 * - Spectral analysis and FFT visualization
 * - Interactive waveform controls
 * - Visualization themes and styling
 * - Export and sharing functionality
 */

// Waveform visualization
export {
  WaveformRenderer,
  RENDER_STYLES,
  COLOR_SCHEMES,
} from "./waveform-renderer.js";

// Phase 2.3A: Advanced Visualization Components
export { SpectrogramVisualizer } from "./SpectrogramVisualizer.js";
export { AudioSpectrumAnalyzer } from "./AudioSpectrumAnalyzer.js";
export { WaveformEnhancer } from "./WaveformEnhancer.js";

// Planned exports (will be available in future releases)
// export { SpectralAnalyzer } from './spectral-analyzer.js';
// export { WaveformInteractions } from './waveform-interactions.js';
// export { VisualizationThemes } from './visualization-themes.js';
// export { AnalysisExport } from './analysis-export.js';

// Default export for convenience
export default {
  WaveformRenderer,
  SpectrogramVisualizer,
  AudioSpectrumAnalyzer,
  WaveformEnhancer,
};
