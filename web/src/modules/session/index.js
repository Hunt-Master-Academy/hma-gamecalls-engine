/**
 * @file session/index.js
 * @brief Session Management Modules Index
 *
 * Complete session lifecycle, state management, data persistence, recovery,
 * multi-tab sync, and analytics modules for the Huntmaster web application.
 *
 * ✅ COMPLETE SESSION MODULE SUITE - All 5 modules implemented
 */

export { SessionStorage } from "./session-storage.js";
export { SessionState } from "./session-state.js";
export {
  SessionRecovery,
  RECOVERY_STATES,
  RECOVERY_PRIORITIES,
} from "./session-recovery.js";
export {
  MultiTabSync,
  SYNC_STATES,
  MESSAGE_TYPES,
  CONFLICT_STRATEGIES,
} from "./multi-tab-sync.js";
export {
  SessionAnalytics,
  ANALYTICS_EVENTS,
  AGGREGATION_TYPES,
  PRIVACY_LEVELS,
} from "./session-analytics.js";

export default {
  SessionStorage: () => import("./session-storage.js"),
  SessionState: () => import("./session-state.js"),
  SessionRecovery: () => import("./session-recovery.js"),
  MultiTabSync: () => import("./multi-tab-sync.js"),
  SessionAnalytics: () => import("./session-analytics.js"),
};
