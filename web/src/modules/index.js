/**
 * @file index.js
 * @brief Module Indexexport { MasterCallManager } from './audio/master-call-manager.js';
export { NoiseDetector } from './audio/noise-detector.js';
export { QualityAssessor } from './audio/quality-assessor.js';
export { RecordingEnhancer } from './audio/recording-enhancer.js';

// Audio system management
export { AudioContextManager, CONTEXT_STATES, CONTEXT_PRESETS, NODE_TYPES } from './audio/audio-context-manager.js';

// Future audio modules:
// export { AudioNodeFactory } from './audio/audio-node-factory.js';
// export { AudioEffectsChain } from './audio/audio-effects-chain.js';
// export { AudioWorkletProcessor } from './audio/audio-worklet-processor.js';
// export { AudioDeviceManager } from './audio/audio-device-manager.js';
// export { AudioRoutingMatrix } from './audio/audio-routing-matrix.js';ation System
 *
 * This file provides centralized access to all Huntmaster web modules
 * organized by functional category for better maintainability and
 * developer experience.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

// =============================================================================
// CORE SYSTEM MODULES
// =============================================================================
// Essential infrastructure modules that provide foundational functionality

export { EventManager } from "./core/event-manager.js";
export { PerformanceMonitor } from "./core/performance-monitor.js";
export { WASMEngineManager } from "./core/wasm-engine-manager.js";

// =============================================================================
// SESSION MANAGEMENT MODULES
// =============================================================================
// Session lifecycle, state management, and data persistence

export { SessionStorage } from "./session/session-storage.js";
export { SessionState } from "./session/session-state.js";

// Future session modules:
// export { SessionRecovery } from './session/session-recovery.js';
// export { MultiTabSync } from './session/multi-tab-sync.js';
// export { SessionAnalytics } from './session/session-analytics.js';

// =============================================================================
// AUDIO PROCESSING MODULES
// =============================================================================
// Audio analysis, processing, recording, and enhancement functionality

export { AudioLevelMonitor } from "./audio/audio-level-monitor.js";
export { AudioWorkletManager } from "./audio/audio-worklet-manager.js";
export { AutomaticGainControl } from "./audio/automatic-gain-control.js";
export { FormatConverter } from "./audio/format-converter.js";
export { MasterCallManager } from "./audio/master-call-manager.js";
export { NoiseDetector } from "./audio/noise-detector.js";
export { QualityAssessor } from "./audio/quality-assessor.js";
export { RecordingEnhancer } from "./audio/recording-enhancer.js";

// Future audio modules:
// export { AudioContextManager } from './audio/audio-context-manager.js';
// export { AudioNodeFactory } from './audio/audio-node-factory.js';
// export { AudioEffectsChain } from './audio/audio-effects-chain.js';
// export { AudioWorkletProcessor } from './audio/audio-worklet-processor.js';
// export { AudioDeviceManager } from './audio/audio-device-manager.js';
// export { AudioRoutingMatrix } from './audio/audio-routing-matrix.js';

// =============================================================================
// USER INTERFACE MODULES
// =============================================================================
// UI components, layout management, themes, and accessibility

export {
  BaseComponent,
  Button,
  ProgressBar,
  Toggle,
  Slider,
  ComponentFactory,
} from "./ui/ui-components.js";

// Future UI modules:
// export { UILayout } from './ui/ui-layout.js';
// export { UIAccessibility } from './ui/ui-accessibility.js';
// export { UIThemes } from './ui/ui-themes.js';
// export { UIAnimations } from './ui/ui-animations.js';
// export { UIFeedback } from './ui/ui-feedback.js';

// =============================================================================
// VISUALIZATION MODULES
// =============================================================================
// Waveform rendering, spectral analysis, and data visualization

// Future visualization modules:
// export { WaveformRenderer } from './visualization/waveform-renderer.js';
// export { SpectralAnalyzer } from './visualization/spectral-analyzer.js';
// export { WaveformInteractions } from './visualization/waveform-interactions.js';
// export { VisualizationThemes } from './visualization/visualization-themes.js';
// export { AnalysisExport } from './visualization/analysis-export.js';

// =============================================================================
// MODULE COLLECTIONS BY CATEGORY
// =============================================================================
// Convenient collections for importing multiple related modules

export const CoreModules = {
  EventManager: () => import("./core/event-manager.js"),
  PerformanceMonitor: () => import("./core/performance-monitor.js"),
  WASMEngineManager: () => import("./core/wasm-engine-manager.js"),
};

export const SessionModules = {
  SessionStorage: () => import("./session/session-storage.js"),
  SessionState: () => import("./session/session-state.js"),
  // SessionRecovery: () => import('./session/session-recovery.js'),
  // MultiTabSync: () => import('./session/multi-tab-sync.js'),
  // SessionAnalytics: () => import('./session/session-analytics.js'),
};

export const AudioModules = {
  AudioLevelMonitor: () => import("./audio/audio-level-monitor.js"),
  AudioWorkletManager: () => import("./audio/audio-worklet-manager.js"),
  AutomaticGainControl: () => import("./audio/automatic-gain-control.js"),
  FormatConverter: () => import("./audio/format-converter.js"),
  MasterCallManager: () => import("./audio/master-call-manager.js"),
  NoiseDetector: () => import("./audio/noise-detector.js"),
  QualityAssessor: () => import("./audio/quality-assessor.js"),
  RecordingEnhancer: () => import("./audio/recording-enhancer.js"),
  AudioContextManager: () => import("./audio/audio-context-manager.js"),
};

export const UIModules = {
  UIComponents: () => import("./ui/ui-components.js"),
  UILayout: () => import("./ui/ui-layout.js"),
  // UIAccessibility: () => import('./ui/ui-accessibility.js'),
  // UIThemes: () => import('./ui/ui-themes.js'),
  // UIAnimations: () => import('./ui/ui-animations.js'),
  // UIFeedback: () => import('./ui/ui-feedback.js'),
};

export const VisualizationModules = {
  WaveformRenderer: () => import("./visualization/waveform-renderer.js"),
  // SpectralAnalyzer: () => import('./visualization/spectral-analyzer.js'),
  // WaveformInteractions: () => import('./visualization/waveform-interactions.js'),
  // VisualizationThemes: () => import('./visualization/visualization-themes.js'),
  // AnalysisExport: () => import('./visualization/analysis-export.js'),
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * Load all modules in a category dynamically
 */
export async function loadModuleCategory(category) {
  const moduleMap = {
    core: CoreModules,
    session: SessionModules,
    audio: AudioModules,
    ui: UIModules,
    visualization: VisualizationModules,
  };

  const modules = moduleMap[category];
  if (!modules) {
    throw new Error(`Unknown module category: ${category}`);
  }

  const loadedModules = {};
  const loadPromises = Object.entries(modules).map(async ([name, loader]) => {
    try {
      const module = await loader();
      loadedModules[name] = module;
    } catch (error) {
      console.warn(`Failed to load module ${name}:`, error);
      loadedModules[name] = null;
    }
  });

  await Promise.all(loadPromises);
  return loadedModules;
}

/**
 * Load specific modules by name
 */
export async function loadModules(moduleNames) {
  const allModules = {
    ...CoreModules,
    ...SessionModules,
    ...AudioModules,
    ...UIModules,
    ...VisualizationModules,
  };

  const loadedModules = {};
  const loadPromises = moduleNames.map(async (name) => {
    const loader = allModules[name];
    if (!loader) {
      console.warn(`Unknown module: ${name}`);
      return;
    }

    try {
      const module = await loader();
      loadedModules[name] = module;
    } catch (error) {
      console.warn(`Failed to load module ${name}:`, error);
      loadedModules[name] = null;
    }
  });

  await Promise.all(loadPromises);
  return loadedModules;
}

/**
 * Get module information and statistics
 */
export function getModuleInfo() {
  return {
    categories: {
      core: Object.keys(CoreModules).length,
      session: Object.keys(SessionModules).length,
      audio: Object.keys(AudioModules).length,
      ui: Object.keys(UIModules).length,
      visualization: Object.keys(VisualizationModules).length,
    },
    total:
      Object.keys(CoreModules).length +
      Object.keys(SessionModules).length +
      Object.keys(AudioModules).length +
      Object.keys(UIModules).length +
      Object.keys(VisualizationModules).length,
    structure: {
      "core/": Object.keys(CoreModules),
      "session/": Object.keys(SessionModules),
      "audio/": Object.keys(AudioModules),
      "ui/": Object.keys(UIModules),
      "visualization/": Object.keys(VisualizationModules),
    },
  };
}

// =============================================================================
// DEFAULT EXPORT
// =============================================================================

export default {
  // Individual modules
  EventManager,
  PerformanceMonitor,
  WASMEngineManager,
  SessionStorage,
  SessionState,
  AudioLevelMonitor,
  AudioWorkletManager,
  AutomaticGainControl,
  FormatConverter,
  MasterCallManager,
  NoiseDetector,
  QualityAssessor,
  RecordingEnhancer,
  BaseComponent,
  Button,
  ProgressBar,
  Toggle,
  Slider,
  ComponentFactory,

  // Module collections
  CoreModules,
  SessionModules,
  AudioModules,
  UIModules,
  VisualizationModules,

  // Utilities
  loadModuleCategory,
  loadModules,
  getModuleInfo,
};
