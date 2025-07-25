/**
 * @file session-state.js
 * @brief Session State Management System
 *
 * This module provides comprehensive state management for audio sessions,
 * including state transitions, validation, event handling, and real-time
 * state synchronization across the application.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

/**
 * @enum SessionStatus
 * @brief Valid session status values
 */
export const SessionStatus = {
  INACTIVE: "inactive",
  INITIALIZING: "initializing",
  ACTIVE: "active",
  RECORDING: "recording",
  ANALYZING: "analyzing",
  PAUSED: "paused",
  COMPLETE: "complete",
  ERROR: "error",
  TERMINATED: "terminated",
};

/**
 * @enum StateTransition
 * @brief Valid state transition mappings
 */
export const StateTransitions = {
  [SessionStatus.INACTIVE]: [SessionStatus.INITIALIZING],
  [SessionStatus.INITIALIZING]: [SessionStatus.ACTIVE, SessionStatus.ERROR],
  [SessionStatus.ACTIVE]: [
    SessionStatus.RECORDING,
    SessionStatus.ANALYZING,
    SessionStatus.PAUSED,
    SessionStatus.TERMINATED,
  ],
  [SessionStatus.RECORDING]: [
    SessionStatus.ACTIVE,
    SessionStatus.ANALYZING,
    SessionStatus.PAUSED,
    SessionStatus.ERROR,
  ],
  [SessionStatus.ANALYZING]: [
    SessionStatus.ACTIVE,
    SessionStatus.COMPLETE,
    SessionStatus.ERROR,
  ],
  [SessionStatus.PAUSED]: [
    SessionStatus.ACTIVE,
    SessionStatus.RECORDING,
    SessionStatus.TERMINATED,
  ],
  [SessionStatus.COMPLETE]: [SessionStatus.ACTIVE, SessionStatus.TERMINATED],
  [SessionStatus.ERROR]: [SessionStatus.ACTIVE, SessionStatus.TERMINATED],
  [SessionStatus.TERMINATED]: [],
};

/**
 * @class SessionState
 * @brief Comprehensive session state management
 *
 * Manages session lifecycle, state transitions, validation, and
 * real-time state updates with event-driven architecture.
 */
export class SessionState {
  constructor(initialState = {}, options = {}) {
    this.options = {
      enableValidation: true,
      enableHistory: true,
      maxHistorySize: 100,
      enableAutoSave: true,
      autoSaveInterval: 5000,
      ...options,
    };

    // Core state properties
    this.currentState = {
      id: null,
      status: SessionStatus.INACTIVE,
      startTime: null,
      lastActivity: Date.now(),
      duration: 0,
      isRecording: false,
      isPlaying: false,
      audioData: null,
      analysisResults: [],
      recordings: [],
      annotations: [],
      userSettings: {},
      metadata: {},
      ...initialState,
    };

    // State history for undo/redo functionality
    this.stateHistory = [];
    this.historyIndex = -1;

    // Event listeners for state changes
    this.eventListeners = new Map();

    // State validation rules
    this.validationRules = new Map();
    this.setupDefaultValidationRules();

    // Performance metrics
    this.performanceMetrics = {
      stateChanges: 0,
      validationErrors: 0,
      transitionErrors: 0,
      averageTransitionTime: 0,
      lastTransitionTime: 0,
    };

    // Auto-save timer
    this.autoSaveTimer = null;
    this.isDirty = false;

    console.log("SessionState initialized");
  }

  /**
   * Set up default validation rules for state properties
   */
  setupDefaultValidationRules() {
    // Session ID validation
    this.addValidationRule(
      "id",
      (value) => {
        return typeof value === "string" && value.length > 0;
      },
      "Session ID must be a non-empty string"
    );

    // Status validation
    this.addValidationRule(
      "status",
      (value) => {
        return Object.values(SessionStatus).includes(value);
      },
      "Status must be a valid SessionStatus value"
    );

    // Duration validation
    this.addValidationRule(
      "duration",
      (value) => {
        return typeof value === "number" && value >= 0;
      },
      "Duration must be a non-negative number"
    );

    // Audio data validation
    this.addValidationRule(
      "audioData",
      (value) => {
        return value === null || (value && typeof value === "object");
      },
      "Audio data must be null or an object"
    );

    // Arrays validation
    this.addValidationRule(
      "analysisResults",
      (value) => {
        return Array.isArray(value);
      },
      "Analysis results must be an array"
    );

    this.addValidationRule(
      "recordings",
      (value) => {
        return Array.isArray(value);
      },
      "Recordings must be an array"
    );

    this.addValidationRule(
      "annotations",
      (value) => {
        return Array.isArray(value);
      },
      "Annotations must be an array"
    );
  }

  /**
   * Add a custom validation rule for a state property
   */
  addValidationRule(property, validator, errorMessage) {
    if (typeof validator !== "function") {
      throw new Error("Validator must be a function");
    }

    this.validationRules.set(property, {
      validator,
      errorMessage: errorMessage || `Invalid value for ${property}`,
    });
  }

  /**
   * Validate the current state against all rules
   */
  validateState(state = this.currentState) {
    if (!this.options.enableValidation) {
      return { isValid: true, errors: [] };
    }

    const errors = [];

    for (const [property, rule] of this.validationRules) {
      if (state.hasOwnProperty(property)) {
        try {
          if (!rule.validator(state[property])) {
            errors.push({
              property,
              value: state[property],
              message: rule.errorMessage,
            });
          }
        } catch (error) {
          errors.push({
            property,
            value: state[property],
            message: `Validation error: ${error.message}`,
          });
        }
      }
    }

    if (errors.length > 0) {
      this.performanceMetrics.validationErrors += errors.length;
    }

    return {
      isValid: errors.length === 0,
      errors,
    };
  }

  /**
   * Update state with validation and event emission
   */
  updateState(updates, options = {}) {
    const startTime = performance.now();

    try {
      // Create new state with updates
      const newState = {
        ...this.currentState,
        ...updates,
        lastActivity: Date.now(),
      };

      // Validate new state
      if (this.options.enableValidation) {
        const validation = this.validateState(newState);
        if (!validation.isValid) {
          const error = new Error(
            `State validation failed: ${validation.errors
              .map((e) => e.message)
              .join(", ")}`
          );
          error.validationErrors = validation.errors;
          throw error;
        }
      }

      // Check state transition validity if status is changing
      if (updates.status && updates.status !== this.currentState.status) {
        if (!this.isValidTransition(this.currentState.status, updates.status)) {
          const error = new Error(
            `Invalid state transition from ${this.currentState.status} to ${updates.status}`
          );
          this.performanceMetrics.transitionErrors++;
          throw error;
        }
      }

      // Store previous state in history
      if (this.options.enableHistory) {
        this.addToHistory(this.currentState);
      }

      const previousState = { ...this.currentState };
      this.currentState = newState;

      // Update duration if session is active
      if (
        this.currentState.startTime &&
        this.currentState.status !== SessionStatus.INACTIVE
      ) {
        this.currentState.duration = Date.now() - this.currentState.startTime;
      }

      // Mark as dirty for auto-save
      this.isDirty = true;

      // Update performance metrics
      this.performanceMetrics.stateChanges++;
      const transitionTime = performance.now() - startTime;
      this.performanceMetrics.lastTransitionTime = transitionTime;
      this.performanceMetrics.averageTransitionTime =
        (this.performanceMetrics.averageTransitionTime + transitionTime) / 2;

      // Emit state change events
      this.emitStateChange(previousState, this.currentState, updates);

      // Start auto-save timer if enabled
      if (this.options.enableAutoSave && !options.skipAutoSave) {
        this.scheduleAutoSave();
      }

      return true;
    } catch (error) {
      console.error("Failed to update state:", error);
      this.emitError("STATE_UPDATE_FAILED", error);
      return false;
    }
  }

  /**
   * Check if a state transition is valid
   */
  isValidTransition(fromStatus, toStatus) {
    const validTransitions = StateTransitions[fromStatus];
    return validTransitions && validTransitions.includes(toStatus);
  }

  /**
   * Transition to a new status with validation
   */
  transitionTo(newStatus, additionalUpdates = {}) {
    if (!this.isValidTransition(this.currentState.status, newStatus)) {
      throw new Error(
        `Invalid transition from ${this.currentState.status} to ${newStatus}`
      );
    }

    const updates = {
      status: newStatus,
      ...additionalUpdates,
    };

    // Add status-specific updates
    switch (newStatus) {
      case SessionStatus.INITIALIZING:
        updates.startTime = Date.now();
        break;
      case SessionStatus.ACTIVE:
        if (!this.currentState.startTime) {
          updates.startTime = Date.now();
        }
        break;
      case SessionStatus.RECORDING:
        updates.isRecording = true;
        break;
      case SessionStatus.COMPLETE:
      case SessionStatus.TERMINATED:
        updates.isRecording = false;
        updates.isPlaying = false;
        break;
      case SessionStatus.ERROR:
        updates.isRecording = false;
        updates.isPlaying = false;
        break;
    }

    return this.updateState(updates);
  }

  /**
   * Add current state to history
   */
  addToHistory(state) {
    // Remove future history if we're not at the end
    if (this.historyIndex < this.stateHistory.length - 1) {
      this.stateHistory = this.stateHistory.slice(0, this.historyIndex + 1);
    }

    // Add new state to history
    this.stateHistory.push({ ...state });
    this.historyIndex++;

    // Limit history size
    if (this.stateHistory.length > this.options.maxHistorySize) {
      this.stateHistory.shift();
      this.historyIndex--;
    }
  }

  /**
   * Undo last state change
   */
  undo() {
    if (!this.canUndo()) {
      return false;
    }

    this.historyIndex--;
    const previousState = this.stateHistory[this.historyIndex];
    const currentState = { ...this.currentState };

    this.currentState = { ...previousState };
    this.emitStateChange(currentState, this.currentState, { source: "undo" });

    return true;
  }

  /**
   * Redo last undone state change
   */
  redo() {
    if (!this.canRedo()) {
      return false;
    }

    this.historyIndex++;
    const nextState = this.stateHistory[this.historyIndex];
    const currentState = { ...this.currentState };

    this.currentState = { ...nextState };
    this.emitStateChange(currentState, this.currentState, { source: "redo" });

    return true;
  }

  /**
   * Check if undo is available
   */
  canUndo() {
    return this.historyIndex > 0;
  }

  /**
   * Check if redo is available
   */
  canRedo() {
    return this.historyIndex < this.stateHistory.length - 1;
  }

  /**
   * Get current state (read-only copy)
   */
  getState() {
    return { ...this.currentState };
  }

  /**
   * Get specific state property
   */
  getProperty(property) {
    return this.currentState[property];
  }

  /**
   * Set specific state property
   */
  setProperty(property, value) {
    return this.updateState({ [property]: value });
  }

  /**
   * Add event listener for state changes
   */
  addEventListener(event, callback) {
    if (typeof callback !== "function") {
      throw new Error("Callback must be a function");
    }

    if (!this.eventListeners.has(event)) {
      this.eventListeners.set(event, new Set());
    }

    this.eventListeners.get(event).add(callback);
  }

  /**
   * Remove event listener
   */
  removeEventListener(event, callback) {
    const listeners = this.eventListeners.get(event);
    if (listeners) {
      listeners.delete(callback);
      if (listeners.size === 0) {
        this.eventListeners.delete(event);
      }
    }
  }

  /**
   * Emit state change event
   */
  emitStateChange(previousState, newState, updates) {
    const event = {
      type: "stateChange",
      previousState: { ...previousState },
      newState: { ...newState },
      updates,
      timestamp: Date.now(),
    };

    this.emitEvent("stateChange", event);

    // Emit specific property change events
    for (const [key, value] of Object.entries(updates)) {
      if (previousState[key] !== value) {
        this.emitEvent(`${key}Changed`, {
          type: `${key}Changed`,
          property: key,
          previousValue: previousState[key],
          newValue: value,
          timestamp: Date.now(),
        });
      }
    }

    // Emit status-specific events
    if (updates.status) {
      this.emitEvent(`status:${updates.status}`, {
        type: "statusChange",
        previousStatus: previousState.status,
        newStatus: updates.status,
        state: { ...newState },
        timestamp: Date.now(),
      });
    }
  }

  /**
   * Emit error event
   */
  emitError(errorType, error) {
    this.emitEvent("error", {
      type: "error",
      errorType,
      error,
      state: { ...this.currentState },
      timestamp: Date.now(),
    });
  }

  /**
   * Generic event emission
   */
  emitEvent(eventType, eventData) {
    const listeners = this.eventListeners.get(eventType);
    if (listeners) {
      listeners.forEach((callback) => {
        try {
          callback(eventData);
        } catch (error) {
          console.error(`Error in event listener for ${eventType}:`, error);
        }
      });
    }
  }

  /**
   * Schedule auto-save operation
   */
  scheduleAutoSave() {
    if (this.autoSaveTimer) {
      clearTimeout(this.autoSaveTimer);
    }

    this.autoSaveTimer = setTimeout(() => {
      if (this.isDirty) {
        this.emitEvent("autoSave", {
          type: "autoSave",
          state: { ...this.currentState },
          timestamp: Date.now(),
        });
        this.isDirty = false;
      }
    }, this.options.autoSaveInterval);
  }

  /**
   * Reset session state to initial values
   */
  reset(preserveId = true) {
    const initialState = {
      id: preserveId ? this.currentState.id : null,
      status: SessionStatus.INACTIVE,
      startTime: null,
      lastActivity: Date.now(),
      duration: 0,
      isRecording: false,
      isPlaying: false,
      audioData: null,
      analysisResults: [],
      recordings: [],
      annotations: [],
      userSettings: {},
      metadata: {},
    };

    // Clear history
    this.stateHistory = [];
    this.historyIndex = -1;

    // Clear auto-save timer
    if (this.autoSaveTimer) {
      clearTimeout(this.autoSaveTimer);
      this.autoSaveTimer = null;
    }

    this.isDirty = false;

    return this.updateState(initialState, { skipAutoSave: true });
  }

  /**
   * Get performance metrics
   */
  getPerformanceMetrics() {
    return { ...this.performanceMetrics };
  }

  /**
   * Get state history
   */
  getHistory() {
    return this.stateHistory.map((state) => ({ ...state }));
  }

  /**
   * Export state for persistence
   */
  exportState() {
    return {
      currentState: { ...this.currentState },
      history: this.options.enableHistory ? this.getHistory() : [],
      historyIndex: this.historyIndex,
      performanceMetrics: { ...this.performanceMetrics },
      options: { ...this.options },
      timestamp: Date.now(),
    };
  }

  /**
   * Import state from persistence
   */
  importState(exportedState) {
    try {
      if (exportedState.currentState) {
        this.currentState = { ...exportedState.currentState };
      }

      if (exportedState.history && this.options.enableHistory) {
        this.stateHistory = exportedState.history.map((state) => ({
          ...state,
        }));
        this.historyIndex = exportedState.historyIndex || -1;
      }

      if (exportedState.performanceMetrics) {
        this.performanceMetrics = { ...exportedState.performanceMetrics };
      }

      // Validate imported state
      const validation = this.validateState(this.currentState);
      if (!validation.isValid) {
        throw new Error(
          `Imported state validation failed: ${validation.errors
            .map((e) => e.message)
            .join(", ")}`
        );
      }

      this.emitEvent("stateImported", {
        type: "stateImported",
        state: { ...this.currentState },
        timestamp: Date.now(),
      });

      return true;
    } catch (error) {
      console.error("Failed to import state:", error);
      this.emitError("STATE_IMPORT_FAILED", error);
      return false;
    }
  }

  /**
   * Cleanup resources and stop timers
   */
  destroy() {
    // Clear auto-save timer
    if (this.autoSaveTimer) {
      clearTimeout(this.autoSaveTimer);
      this.autoSaveTimer = null;
    }

    // Clear event listeners
    this.eventListeners.clear();

    // Clear history
    this.stateHistory = [];
    this.historyIndex = -1;

    console.log("SessionState destroyed");
  }
}

export default SessionState;
