/**
 * @fileoverview Web Audio Modules - Index and Export Management
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Centralized export management for all Web Audio modules providing
 * clean import interfaces and module coordination.
 *
 * Available Modules:
 * ✅ WebAudioContext - AudioContext management with cross-browser compatibility
 * ✅ WebAudioNodes - Advanced node management with dynamic routing and automation
 * ✅ WebAudioEffects - Comprehensive effects processing with 15+ effect types
 * ✅ WebAudioStreams - Audio stream management with device control
 * ✅ WebAudioWorklets - Custom audio processing with worklet integration
 * ✅ WebAudioPerformance - Real-time performance monitoring and optimization
 * ✅ WebAudioManagerModular - Complete integrated Web Audio management system
 *
 * @example
 * ```javascript
 * // Import individual modules
 * import { WebAudioContext, WebAudioEffects } from './modules/web-audio/index.js';
 *
 * // Import complete manager
 * import { WebAudioManagerModular } from './modules/web-audio/index.js';
 *
 * // Import all modules
 * import * as WebAudio from './modules/web-audio/index.js';
 * ```
 */

// Individual module exports
export { default as WebAudioContext } from "./web-audio-context.js";
export { default as WebAudioNodes } from "./web-audio-nodes.js";
export { default as WebAudioEffects } from "./web-audio-effects.js";
export { default as WebAudioStreams } from "./web-audio-streams.js";
export { default as WebAudioWorklets } from "./web-audio-worklets.js";
export { default as WebAudioPerformance } from "./web-audio-performance.js";

// Integrated manager export
export { default as WebAudioManagerModular } from "./web-audio-manager-modular.js";

// Convenience exports
export { WebAudioManagerModular as WebAudioManager };
export { WebAudioManagerModular as default };

/**
 * Module metadata and information
 */
export const MODULES = {
  WebAudioContext: {
    name: "WebAudioContext",
    description: "AudioContext management with cross-browser compatibility",
    features: [
      "Context initialization",
      "State management",
      "Device enumeration",
      "Error recovery",
    ],
    linesOfCode: "900+",
    status: "complete",
  },

  WebAudioNodes: {
    name: "WebAudioNodes",
    description: "Advanced node management with dynamic routing and automation",
    features: [
      "Node creation",
      "Dynamic routing",
      "Parameter automation",
      "Connection management",
    ],
    linesOfCode: "1500+",
    status: "complete",
  },

  WebAudioEffects: {
    name: "WebAudioEffects",
    description: "Comprehensive effects processing with 15+ effect types",
    features: [
      "Dynamics processing",
      "Filter effects",
      "Time-based effects",
      "Effect chains",
    ],
    linesOfCode: "1200+",
    status: "complete",
  },

  WebAudioStreams: {
    name: "WebAudioStreams",
    description: "Audio stream management with device control",
    features: [
      "Stream capture",
      "Device management",
      "Recording",
      "Monitoring",
    ],
    linesOfCode: "1300+",
    status: "complete",
  },

  WebAudioWorklets: {
    name: "WebAudioWorklets",
    description: "Custom audio processing with worklet integration",
    features: [
      "Worklet registration",
      "Custom processors",
      "Built-in algorithms",
      "Real-time messaging",
    ],
    linesOfCode: "1200+",
    status: "complete",
  },

  WebAudioPerformance: {
    name: "WebAudioPerformance",
    description: "Real-time performance monitoring and optimization",
    features: [
      "Performance metrics",
      "Resource tracking",
      "Alert system",
      "Optimization recommendations",
    ],
    linesOfCode: "1000+",
    status: "complete",
  },

  WebAudioManagerModular: {
    name: "WebAudioManagerModular",
    description: "Complete integrated Web Audio management system",
    features: [
      "Module integration",
      "Error recovery",
      "Cross-module communication",
      "Unified API",
    ],
    linesOfCode: "1300+",
    status: "complete",
  },
};

/**
 * Get module information
 *
 * @param {string} moduleName - Module name
 * @returns {Object|null} Module information
 */
export function getModuleInfo(moduleName) {
  return MODULES[moduleName] || null;
}

/**
 * Get all module information
 *
 * @returns {Object} All modules information
 */
export function getAllModulesInfo() {
  return { ...MODULES };
}

/**
 * Get system statistics
 *
 * @returns {Object} System statistics
 */
export function getSystemStats() {
  const moduleCount = Object.keys(MODULES).length;
  const totalLines = Object.values(MODULES).reduce((total, module) => {
    const lines = parseInt(module.linesOfCode.replace("+", ""));
    return total + lines;
  }, 0);

  return {
    totalModules: moduleCount,
    totalLinesOfCode: totalLines,
    completedModules: Object.values(MODULES).filter(
      (m) => m.status === "complete"
    ).length,
    version: "1.0.0",
    lastUpdated: "2024-01-20",
  };
}

console.log("Web Audio modules index loaded -", getSystemStats());
