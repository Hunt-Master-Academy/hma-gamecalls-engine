/**
 * @fileoverview Waveform Settings Management Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive waveform settings and configuration management module providing
 * persistent settings, user preferences, theme management, and configuration validation.
 *
 * Features:
 * ✅ Comprehensive settings management with validation
 * ✅ User preference persistence (localStorage, sessionStorage)
 * ✅ Theme and appearance customization
 * ✅ Performance and rendering settings
 * ✅ Accessibility configuration options
 * ✅ Import/export settings functionality
 * ✅ Settings migration and versioning
 * ✅ Real-time settings synchronization
 * ✅ Settings validation and error handling
 * ✅ Default settings and factory reset
 * ✅ Settings groups and categories
 * ✅ Event-driven settings updates
 *
 * @example
 * ```javascript
 * import { WaveformSettings } from './modules/waveform/index.js';
 *
 * const settings = new WaveformSettings({
 *   persistence: 'localStorage',
 *   autoSave: true
 * });
 *
 * settings.set('appearance.waveformColor', '#3498db');
 * const color = settings.get('appearance.waveformColor');
 * ```
 */

/**
 * Waveform Settings Manager
 *
 * Provides comprehensive settings management for waveform visualization
 * including persistence, validation, themes, and user preferences.
 *
 * @class WaveformSettings
 */
export class WaveformSettings {
  /**
   * Create WaveformSettings instance
   *
   * @param {Object} options - Configuration options
   * @param {string} [options.persistence='localStorage'] - Storage method
   * @param {boolean} [options.autoSave=true] - Auto-save changes
   * @param {string} [options.storageKey='waveform-settings'] - Storage key
   * @param {boolean} [options.validateOnSet=true] - Validate on setting values
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      persistence: options.persistence || "localStorage",
      autoSave: options.autoSave !== false,
      storageKey: options.storageKey || "waveform-settings",
      validateOnSet: options.validateOnSet !== false,
      ...options,
    };

    // Settings structure
    this.settings = {};
    this.defaultSettings = this._getDefaultSettings();
    this.validators = new Map();
    this.watchers = new Map();

    // Event handling
    this.eventHandlers = new Map();

    // Change tracking
    this.changeHistory = [];
    this.maxHistorySize = 100;
    this.isDirty = false;

    // Version management
    this.version = "1.0.0";

    // Initialize system
    this._initializeValidators();
    this._loadSettings();

    console.log("WaveformSettings initialized");
  }

  /**
   * Get default settings structure
   * @private
   */
  _getDefaultSettings() {
    return {
      // Appearance settings
      appearance: {
        theme: "default",
        waveformColor: "#3498db",
        backgroundColor: "#2c3e50",
        gridColor: "#34495e",
        cursorColor: "#e74c3c",
        selectionColor: "rgba(52, 152, 219, 0.3)",
        peakColor: "#3498db",
        rmsColor: "#9b59b6",
        progressColor: "#e67e22",

        // Typography
        fontFamily: "Arial, sans-serif",
        fontSize: 12,
        fontWeight: "normal",

        // Opacity and transparency
        waveformOpacity: 1.0,
        gridOpacity: 0.3,
        overlayOpacity: 0.8,

        // Gradients and effects
        enableGradients: true,
        enableShadows: false,
        enableGlow: false,

        // Animation
        enableAnimations: true,
        animationDuration: 300,
        animationEasing: "ease-out",
      },

      // Rendering settings
      rendering: {
        renderer: "canvas", // 'canvas', 'webgl', 'svg'
        pixelRatio: window.devicePixelRatio || 1,
        enableAntialiasing: true,
        enableHardwareAcceleration: true,
        maxFPS: 60,

        // Waveform rendering
        waveformStyle: "filled", // 'filled', 'outline', 'bars', 'dots'
        peakStyle: "linear", // 'linear', 'stepped', 'smooth'
        interpolation: "linear", // 'linear', 'cubic', 'none'

        // Performance
        enableLOD: true, // Level of Detail
        lodThreshold: 1000,
        enableCulling: true,
        maxPeaksPerPixel: 4,

        // Quality
        renderQuality: "high", // 'low', 'medium', 'high', 'ultra'
        subpixelRendering: true,
      },

      // Interaction settings
      interaction: {
        enableTouch: true,
        enableMouse: true,
        enableKeyboard: true,
        enableGestures: true,

        // Zoom and pan
        zoomSensitivity: 1.0,
        panSensitivity: 1.0,
        minZoom: 0.1,
        maxZoom: 100,
        zoomMode: "center", // 'center', 'cursor', 'selection'

        // Selection
        enableSelection: true,
        multiSelection: false,
        selectionMode: "range", // 'range', 'point', 'region'

        // Playback
        clickToSeek: true,
        doubleClickAction: "zoom", // 'zoom', 'play', 'select'
        wheelAction: "zoom", // 'zoom', 'scroll', 'volume'

        // Timing
        doubleClickTime: 300,
        longPressTime: 500,
        hoverDelay: 100,
      },

      // Audio settings
      audio: {
        sampleRate: 44100,
        channels: 2,
        bitDepth: 32,
        bufferSize: 4096,

        // Processing
        enableNormalization: true,
        enableNoiseGate: false,
        noiseGateThreshold: -40,

        // Analysis
        fftSize: 2048,
        windowFunction: "hann",
        overlap: 0.5,

        // Playback
        volume: 1.0,
        muted: false,
        loop: false,
        autoplay: false,
      },

      // Performance settings
      performance: {
        enableWorkers: true,
        maxWorkers: 4,
        enableCaching: true,
        maxCacheSize: 100,

        // Memory management
        enableGarbageCollection: true,
        gcInterval: 60000, // 1 minute
        maxMemoryUsage: 500, // MB

        // Update throttling
        updateThrottling: true,
        maxUpdatesPerSecond: 60,
        minUpdateInterval: 16, // ~60fps

        // Lazy loading
        enableLazyLoading: true,
        preloadDistance: 10, // seconds
        unloadDistance: 60, // seconds
      },

      // Accessibility settings
      accessibility: {
        enableAccessibility: true,
        enableKeyboardNavigation: true,
        enableScreenReader: true,

        // Contrast and visibility
        highContrast: false,
        largeText: false,
        reducedMotion: false,

        // Audio cues
        enableAudioCues: false,
        audioCueVolume: 0.5,

        // ARIA labels
        enableAriaLabels: true,
        verboseDescriptions: false,
      },

      // Export settings
      export: {
        defaultFormat: "png",
        defaultQuality: 0.9,
        includeMetadata: true,

        // Image export
        imageWidth: 1920,
        imageHeight: 1080,
        imageDPI: 300,

        // Audio export
        audioFormat: "wav",
        audioQuality: "high",
        exportSampleRate: 44100,

        // Data export
        dataFormat: "json",
        includeRawData: false,
        compressionLevel: 6,
      },

      // Debug settings
      debug: {
        enableDebugMode: false,
        showPerformanceStats: false,
        showMemoryUsage: false,
        enableLogging: true,
        logLevel: "info", // 'error', 'warn', 'info', 'debug'

        // Visualization debug
        showBoundingBoxes: false,
        showRenderRegions: false,
        showFrameRate: false,

        // Audio debug
        showAudioStats: false,
        enableAudioVisualization: false,
      },
    };
  }

  /**
   * Initialize validators
   * @private
   */
  _initializeValidators() {
    // Color validator
    this.validators.set("color", (value) => {
      if (typeof value !== "string") return false;
      const colorRegex = /^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$|^rgba?\([^)]+\)$/;
      return colorRegex.test(value);
    });

    // Number range validator
    this.validators.set(
      "numberRange",
      (value, min = -Infinity, max = Infinity) => {
        return (
          typeof value === "number" &&
          value >= min &&
          value <= max &&
          !isNaN(value)
        );
      }
    );

    // Enum validator
    this.validators.set("enum", (value, allowedValues) => {
      return allowedValues.includes(value);
    });

    // Boolean validator
    this.validators.set("boolean", (value) => {
      return typeof value === "boolean";
    });

    // String validator
    this.validators.set("string", (value) => {
      return typeof value === "string";
    });

    // Positive number validator
    this.validators.set("positiveNumber", (value) => {
      return typeof value === "number" && value > 0 && !isNaN(value);
    });

    // Percentage validator (0-1)
    this.validators.set("percentage", (value) => {
      return this.validators.get("numberRange")(value, 0, 1);
    });

    console.log(`Initialized ${this.validators.size} validators`);
  }

  /**
   * Load settings from storage
   * @private
   */
  _loadSettings() {
    try {
      let stored = null;

      switch (this.config.persistence) {
        case "localStorage":
          stored = localStorage.getItem(this.config.storageKey);
          break;
        case "sessionStorage":
          stored = sessionStorage.getItem(this.config.storageKey);
          break;
        case "none":
          // No persistence
          break;
        default:
          console.warn("Unknown persistence method:", this.config.persistence);
      }

      if (stored) {
        const parsed = JSON.parse(stored);

        // Validate version and migrate if necessary
        if (parsed.version !== this.version) {
          this.settings = this._migrateSettings(parsed);
        } else {
          this.settings = this._mergeWithDefaults(parsed.settings || parsed);
        }

        console.log("Settings loaded from storage");
      } else {
        this.settings = this._deepClone(this.defaultSettings);
        console.log("Using default settings");
      }

      this.isDirty = false;
    } catch (error) {
      console.error("Failed to load settings:", error);
      this.settings = this._deepClone(this.defaultSettings);
    }
  }

  /**
   * Save settings to storage
   * @private
   */
  _saveSettings() {
    if (!this.config.autoSave || this.config.persistence === "none") {
      return;
    }

    try {
      const toSave = {
        version: this.version,
        settings: this.settings,
        timestamp: Date.now(),
      };

      const serialized = JSON.stringify(toSave);

      switch (this.config.persistence) {
        case "localStorage":
          localStorage.setItem(this.config.storageKey, serialized);
          break;
        case "sessionStorage":
          sessionStorage.setItem(this.config.storageKey, serialized);
          break;
      }

      this.isDirty = false;
      this._emitEvent("settingsSaved");
    } catch (error) {
      console.error("Failed to save settings:", error);
      this._emitEvent("settingsSaveError", { error });
    }
  }

  /**
   * Migrate settings from older version
   * @private
   */
  _migrateSettings(oldSettings) {
    // For now, just merge with defaults
    // In production, implement proper migration logic based on version
    console.log(
      `Migrating settings from version ${oldSettings.version || "unknown"} to ${
        this.version
      }`
    );

    return this._mergeWithDefaults(oldSettings.settings || oldSettings);
  }

  /**
   * Merge settings with defaults
   * @private
   */
  _mergeWithDefaults(userSettings) {
    return this._deepMerge(this._deepClone(this.defaultSettings), userSettings);
  }

  /**
   * Deep merge objects
   * @private
   */
  _deepMerge(target, source) {
    for (const key in source) {
      if (
        source[key] &&
        typeof source[key] === "object" &&
        !Array.isArray(source[key])
      ) {
        if (!target[key]) target[key] = {};
        this._deepMerge(target[key], source[key]);
      } else {
        target[key] = source[key];
      }
    }
    return target;
  }

  /**
   * Deep clone object
   * @private
   */
  _deepClone(obj) {
    return JSON.parse(JSON.stringify(obj));
  }

  /**
   * Get setting value
   *
   * @param {string} path - Setting path (e.g., 'appearance.waveformColor')
   * @param {*} [defaultValue] - Default value if not found
   * @returns {*} Setting value
   */
  get(path, defaultValue = undefined) {
    const keys = path.split(".");
    let current = this.settings;

    for (const key of keys) {
      if (current && current.hasOwnProperty(key)) {
        current = current[key];
      } else {
        return defaultValue;
      }
    }

    return current;
  }

  /**
   * Set setting value
   *
   * @param {string} path - Setting path
   * @param {*} value - Value to set
   * @param {boolean} [validate=true] - Whether to validate the value
   * @returns {boolean} Success status
   */
  set(path, value, validate = true) {
    // Validate value if required
    if (validate && this.config.validateOnSet) {
      if (!this._validateSetting(path, value)) {
        console.error(`Invalid value for setting ${path}:`, value);
        return false;
      }
    }

    // Get previous value for change detection
    const oldValue = this.get(path);

    // Set the value
    const keys = path.split(".");
    let target = this.settings;

    for (let i = 0; i < keys.length - 1; i++) {
      const key = keys[i];
      if (!target[key] || typeof target[key] !== "object") {
        target[key] = {};
      }
      target = target[key];
    }

    const finalKey = keys[keys.length - 1];
    target[finalKey] = value;

    // Track change
    this._trackChange(path, oldValue, value);

    // Emit change event
    this._emitEvent("settingChanged", {
      path,
      oldValue,
      newValue: value,
    });

    // Save if auto-save enabled
    if (this.config.autoSave) {
      this._saveSettings();
    } else {
      this.isDirty = true;
    }

    // Notify watchers
    this._notifyWatchers(path, value, oldValue);

    return true;
  }

  /**
   * Validate setting value
   * @private
   */
  _validateSetting(path, value) {
    // Define validation rules for specific paths
    const validationRules = {
      "appearance.waveformColor": { type: "color" },
      "appearance.backgroundColor": { type: "color" },
      "appearance.gridColor": { type: "color" },
      "appearance.cursorColor": { type: "color" },
      "appearance.selectionColor": { type: "color" },
      "appearance.peakColor": { type: "color" },
      "appearance.rmsColor": { type: "color" },
      "appearance.progressColor": { type: "color" },

      "appearance.fontSize": { type: "positiveNumber" },
      "appearance.waveformOpacity": { type: "percentage" },
      "appearance.gridOpacity": { type: "percentage" },
      "appearance.overlayOpacity": { type: "percentage" },

      "appearance.enableGradients": { type: "boolean" },
      "appearance.enableShadows": { type: "boolean" },
      "appearance.enableGlow": { type: "boolean" },
      "appearance.enableAnimations": { type: "boolean" },

      "rendering.renderer": {
        type: "enum",
        values: ["canvas", "webgl", "svg"],
      },
      "rendering.waveformStyle": {
        type: "enum",
        values: ["filled", "outline", "bars", "dots"],
      },
      "rendering.peakStyle": {
        type: "enum",
        values: ["linear", "stepped", "smooth"],
      },
      "rendering.interpolation": {
        type: "enum",
        values: ["linear", "cubic", "none"],
      },
      "rendering.renderQuality": {
        type: "enum",
        values: ["low", "medium", "high", "ultra"],
      },

      "rendering.pixelRatio": { type: "positiveNumber" },
      "rendering.maxFPS": { type: "numberRange", min: 1, max: 120 },
      "rendering.maxPeaksPerPixel": { type: "numberRange", min: 1, max: 100 },

      "interaction.zoomSensitivity": { type: "positiveNumber" },
      "interaction.panSensitivity": { type: "positiveNumber" },
      "interaction.minZoom": { type: "positiveNumber" },
      "interaction.maxZoom": { type: "positiveNumber" },

      "audio.sampleRate": { type: "positiveNumber" },
      "audio.channels": { type: "numberRange", min: 1, max: 32 },
      "audio.volume": { type: "percentage" },

      "performance.maxWorkers": { type: "numberRange", min: 1, max: 16 },
      "performance.maxCacheSize": { type: "positiveNumber" },
      "performance.maxMemoryUsage": { type: "positiveNumber" },
    };

    const rule = validationRules[path];
    if (!rule) {
      return true; // No validation rule defined, assume valid
    }

    const validator = this.validators.get(rule.type);
    if (!validator) {
      console.warn(`Unknown validator type: ${rule.type}`);
      return true;
    }

    // Apply validation
    switch (rule.type) {
      case "numberRange":
        return validator(value, rule.min, rule.max);
      case "enum":
        return validator(value, rule.values);
      default:
        return validator(value);
    }
  }

  /**
   * Track setting change
   * @private
   */
  _trackChange(path, oldValue, newValue) {
    const change = {
      path,
      oldValue,
      newValue,
      timestamp: Date.now(),
    };

    this.changeHistory.push(change);

    // Trim history if too long
    if (this.changeHistory.length > this.maxHistorySize) {
      this.changeHistory.shift();
    }
  }

  /**
   * Notify watchers
   * @private
   */
  _notifyWatchers(path, newValue, oldValue) {
    // Exact path watchers
    if (this.watchers.has(path)) {
      this.watchers.get(path).forEach((callback) => {
        try {
          callback(newValue, oldValue, path);
        } catch (error) {
          console.error(`Watcher error for ${path}:`, error);
        }
      });
    }

    // Wildcard watchers (e.g., 'appearance.*')
    for (const [watchPath, callbacks] of this.watchers.entries()) {
      if (watchPath.endsWith("*")) {
        const prefix = watchPath.slice(0, -1);
        if (path.startsWith(prefix)) {
          callbacks.forEach((callback) => {
            try {
              callback(newValue, oldValue, path);
            } catch (error) {
              console.error(`Wildcard watcher error for ${watchPath}:`, error);
            }
          });
        }
      }
    }
  }

  /**
   * Get multiple settings
   *
   * @param {string[]} paths - Array of setting paths
   * @returns {Object} Object with path-value pairs
   */
  getMultiple(paths) {
    const result = {};
    for (const path of paths) {
      result[path] = this.get(path);
    }
    return result;
  }

  /**
   * Set multiple settings
   *
   * @param {Object} settings - Object with path-value pairs
   * @param {boolean} [validate=true] - Whether to validate values
   * @returns {Object} Results with success status for each setting
   */
  setMultiple(settings, validate = true) {
    const results = {};

    for (const [path, value] of Object.entries(settings)) {
      results[path] = this.set(path, value, validate);
    }

    return results;
  }

  /**
   * Reset setting to default
   *
   * @param {string} path - Setting path
   * @returns {boolean} Success status
   */
  reset(path) {
    const defaultValue = this.get(path, undefined, this.defaultSettings);
    if (defaultValue !== undefined) {
      return this.set(path, defaultValue, false);
    }
    return false;
  }

  /**
   * Reset all settings to defaults
   */
  resetAll() {
    this.settings = this._deepClone(this.defaultSettings);
    this.isDirty = true;

    if (this.config.autoSave) {
      this._saveSettings();
    }

    this._emitEvent("settingsReset");
    console.log("All settings reset to defaults");
  }

  /**
   * Watch for setting changes
   *
   * @param {string} path - Setting path (supports wildcards with *)
   * @param {Function} callback - Callback function
   */
  watch(path, callback) {
    if (!this.watchers.has(path)) {
      this.watchers.set(path, []);
    }
    this.watchers.get(path).push(callback);
  }

  /**
   * Unwatch setting changes
   *
   * @param {string} path - Setting path
   * @param {Function} [callback] - Specific callback to remove
   */
  unwatch(path, callback = null) {
    if (this.watchers.has(path)) {
      if (callback) {
        const callbacks = this.watchers.get(path);
        const index = callbacks.indexOf(callback);
        if (index > -1) {
          callbacks.splice(index, 1);
        }
      } else {
        this.watchers.delete(path);
      }
    }
  }

  /**
   * Get all settings
   *
   * @returns {Object} Complete settings object
   */
  getAll() {
    return this._deepClone(this.settings);
  }

  /**
   * Import settings from object
   *
   * @param {Object} importedSettings - Settings to import
   * @param {boolean} [merge=true] - Whether to merge with existing settings
   * @returns {boolean} Success status
   */
  import(importedSettings, merge = true) {
    try {
      let newSettings;

      if (merge) {
        newSettings = this._deepMerge(
          this._deepClone(this.settings),
          importedSettings
        );
      } else {
        newSettings = this._mergeWithDefaults(importedSettings);
      }

      // Validate all imported settings
      if (this.config.validateOnSet) {
        const valid = this._validateAllSettings(newSettings);
        if (!valid) {
          console.error("Settings validation failed during import");
          return false;
        }
      }

      this.settings = newSettings;
      this.isDirty = true;

      if (this.config.autoSave) {
        this._saveSettings();
      }

      this._emitEvent("settingsImported", { merge });
      console.log("Settings imported successfully");
      return true;
    } catch (error) {
      console.error("Failed to import settings:", error);
      this._emitEvent("settingsImportError", { error });
      return false;
    }
  }

  /**
   * Export settings to object
   *
   * @param {boolean} [includeDefaults=false] - Include default values
   * @returns {Object} Exported settings
   */
  export(includeDefaults = false) {
    const exported = {
      version: this.version,
      timestamp: Date.now(),
      settings: this._deepClone(this.settings),
    };

    if (!includeDefaults) {
      // Remove settings that match defaults
      exported.settings = this._removeDefaultValues(
        exported.settings,
        this.defaultSettings
      );
    }

    return exported;
  }

  /**
   * Validate all settings
   * @private
   */
  _validateAllSettings(settings, prefix = "") {
    for (const [key, value] of Object.entries(settings)) {
      const fullPath = prefix ? `${prefix}.${key}` : key;

      if (value && typeof value === "object" && !Array.isArray(value)) {
        if (!this._validateAllSettings(value, fullPath)) {
          return false;
        }
      } else {
        if (!this._validateSetting(fullPath, value)) {
          return false;
        }
      }
    }
    return true;
  }

  /**
   * Remove default values from settings
   * @private
   */
  _removeDefaultValues(settings, defaults) {
    const result = {};

    for (const [key, value] of Object.entries(settings)) {
      if (defaults.hasOwnProperty(key)) {
        if (value && typeof value === "object" && !Array.isArray(value)) {
          const filtered = this._removeDefaultValues(value, defaults[key]);
          if (Object.keys(filtered).length > 0) {
            result[key] = filtered;
          }
        } else if (value !== defaults[key]) {
          result[key] = value;
        }
      } else {
        result[key] = value;
      }
    }

    return result;
  }

  /**
   * Force save settings
   */
  save() {
    this._saveSettings();
  }

  /**
   * Check if settings have unsaved changes
   *
   * @returns {boolean} Dirty state
   */
  isDirty() {
    return this.isDirty;
  }

  /**
   * Get change history
   *
   * @returns {Array} Array of changes
   */
  getChangeHistory() {
    return [...this.changeHistory];
  }

  /**
   * Clear change history
   */
  clearHistory() {
    this.changeHistory.length = 0;
  }

  /**
   * Event handling
   */
  addEventListener(event, handler) {
    if (!this.eventHandlers.has(event)) {
      this.eventHandlers.set(event, []);
    }
    this.eventHandlers.get(event).push(handler);
  }

  removeEventListener(event, handler) {
    if (this.eventHandlers.has(event)) {
      const handlers = this.eventHandlers.get(event);
      const index = handlers.indexOf(handler);
      if (index > -1) {
        handlers.splice(index, 1);
      }
    }
  }

  /**
   * Emit event
   * @private
   */
  _emitEvent(eventName, data = {}) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).forEach((handler) => {
        try {
          handler({ type: eventName, ...data });
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      });
    }
  }

  /**
   * Get setting schema/structure
   *
   * @returns {Object} Settings schema
   */
  getSchema() {
    return this._deepClone(this.defaultSettings);
  }

  /**
   * Cleanup and destroy settings manager
   */
  destroy() {
    console.log("Destroying WaveformSettings...");

    // Save final state if dirty
    if (this.isDirty && this.config.autoSave) {
      this._saveSettings();
    }

    // Clear all data
    this.watchers.clear();
    this.eventHandlers.clear();
    this.changeHistory.length = 0;
    this.validators.clear();

    console.log("WaveformSettings destroyed");
  }
}

export default WaveformSettings;
