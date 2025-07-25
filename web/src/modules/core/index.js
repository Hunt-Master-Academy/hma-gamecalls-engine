/**
 * @file core/index.js
 * @brief Core System Modules Index
 *
 * Essential infrastructure modules that provide foundational functionality
 * for the Huntmaster web application.
 */

export { EventManager } from "./event-manager.js";
export { PerformanceMonitor } from "./performance-monitor.js";
export { WASMEngineManager } from "./wasm-engine-manager.js";

export default {
  EventManager: () => import("./event-manager.js"),
  PerformanceMonitor: () => import("./performance-monitor.js"),
  WASMEngineManager: () => import("./wasm-engine-manager.js"),
};
