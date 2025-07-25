/**
 * @file audio/index.js
 * @brief Audio Processing Modules Index
 *
 * Audio analysis, processing, recording, and enhancement functionality
 * modules for the Huntmaster web application.
 */

export { AudioLevelMonitor } from "./audio-level-monitor.js";
export { AudioWorkletManager } from "./audio-worklet-manager.js";
export { AutomaticGainControl } from "./automatic-gain-control.js";
export { FormatConverter } from "./format-converter.js";
export { MasterCallManager } from "./master-call-manager.js";
export { NoiseDetector } from "./noise-detector.js";
export { QualityAssessor } from "./quality-assessor.js";
export { RecordingEnhancer } from "./recording-enhancer.js";

// Audio system management
export {
  AudioContextManager,
  CONTEXT_STATES,
  CONTEXT_PRESETS,
  NODE_TYPES,
} from "./audio-context-manager.js";

// Future exports:
// export { AudioNodeFactory } from './audio-node-factory.js';
// export { AudioEffectsChain } from './audio-effects-chain.js';
// export { AudioWorkletProcessor } from './audio-worklet-processor.js';
// export { AudioDeviceManager } from './audio-device-manager.js';
// export { AudioRoutingMatrix } from './audio-routing-matrix.js';

export default {
  AudioLevelMonitor: () => import("./audio-level-monitor.js"),
  AudioWorkletManager: () => import("./audio-worklet-manager.js"),
  AutomaticGainControl: () => import("./automatic-gain-control.js"),
  FormatConverter: () => import("./format-converter.js"),
  MasterCallManager: () => import("./master-call-manager.js"),
  NoiseDetector: () => import("./noise-detector.js"),
  QualityAssessor: () => import("./quality-assessor.js"),
  RecordingEnhancer: () => import("./recording-enhancer.js"),
  // AudioContextManager: () => import('./audio-context-manager.js'),
  // AudioNodeFactory: () => import('./audio-node-factory.js'),
  // AudioEffectsChain: () => import('./audio-effects-chain.js'),
  // AudioWorkletProcessor: () => import('./audio-worklet-processor.js'),
  // AudioDeviceManager: () => import('./audio-device-manager.js'),
  // AudioRoutingMatrix: () => import('./audio-routing-matrix.js'),
};
