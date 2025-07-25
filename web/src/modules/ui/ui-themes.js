/**
 * @file ui-themes.js
 * @brief Theme Management and Customization System
 *
 * This module provides comprehensive theme management with customizable color
 * schemes, dark/light modes, high contrast options, and user preference
 * persistence for the Huntmaster audio analysis system.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Theme Management Implementation
 * @date July 24, 2025
 */

/**
 * Available theme types
 */
const THEME_TYPES = {
  LIGHT: "light",
  DARK: "dark",
  HIGH_CONTRAST: "high-contrast",
  CUSTOM: "custom",
};

/**
 * Color scheme definitions
 */
const COLOR_SCHEMES = {
  LIGHT: "light",
  DARK: "dark",
  AUTO: "auto",
};

/**
 * Theme categories for organization
 */
const THEME_CATEGORIES = {
  SYSTEM: "system",
  ACCESSIBILITY: "accessibility",
  CUSTOM: "custom",
  SEASONAL: "seasonal",
};

/**
 * @class UIThemes
 * @brief Comprehensive theme management system
 *
 * Features:
 * • Multiple built-in themes with consistent design language
 * • Dark/light mode with automatic system preference detection
 * • High contrast and accessibility-focused themes
 * • Custom theme creation and modification tools
 * • User preference persistence and synchronization
 * • Smooth theme transitions with performance optimization
 * • CSS custom properties integration
 * • Real-time theme preview and switching
 * • Import/export functionality for theme sharing
 * • Seasonal and time-based theme switching
 */
export class UIThemes {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Theme configuration
      defaultTheme: options.defaultTheme || THEME_TYPES.LIGHT,
      enableAutoDetection: options.enableAutoDetection !== false,
      enableTransitions: options.enableTransitions !== false,
      transitionDuration: options.transitionDuration || 300,

      // Theme persistence
      enablePersistence: options.enablePersistence !== false,
      storageKey: options.storageKey || "huntmaster_theme_preferences",
      enableSync: options.enableSync || false,

      // Custom themes
      enableCustomThemes: options.enableCustomThemes !== false,
      maxCustomThemes: options.maxCustomThemes || 10,
      enableThemeSharing: options.enableThemeSharing || false,

      // Performance optimization
      enableLazyLoading: options.enableLazyLoading !== false,
      enableCSSOptimization: options.enableCSSOptimization !== false,

      // Debug and development
      debugMode: options.debugMode || false,
      enableThemeAnalytics: options.enableThemeAnalytics || false,

      ...options,
    };

    // Theme state
    this.currentTheme = null;
    this.availableThemes = new Map();
    this.customThemes = new Map();
    this.isInitialized = false;
    this.themeHistory = [];

    // System preferences
    this.systemPreferences = {
      prefersColorScheme: "light",
      prefersHighContrast: false,
      prefersReducedMotion: false,
    };

    // Theme application state
    this.appliedStyles = new Set();
    this.transitionTimeouts = new Map();
    this.cssCustomProperties = new Map();

    // Performance tracking
    this.performanceMetrics = {
      themeChanges: 0,
      averageTransitionTime: 0,
      cacheHits: 0,
      cacheMisses: 0,
    };

    this._initializeThemes();
  }

  /**
   * Initialize theme management system
   */
  _initializeThemes() {
    try {
      // Detect system preferences
      this._detectSystemPreferences();

      // Load built-in themes
      this._loadBuiltInThemes();

      // Load custom themes from storage
      this._loadCustomThemes();

      // Load user preferences
      this._loadUserPreferences();

      // Apply initial theme
      this._applyInitialTheme();

      // Set up event listeners
      this._setupEventListeners();

      // Set up CSS custom properties
      this._setupCSSCustomProperties();

      this.isInitialized = true;
      this.log("UIThemes initialized successfully", "success");

      // Emit initialization event
      this.eventManager.emit("themesInitialized", {
        currentTheme: this.currentTheme?.id,
        availableThemes: Array.from(this.availableThemes.keys()),
        systemPreferences: this.systemPreferences,
        timestamp: Date.now(),
      });
    } catch (error) {
      this.log(`Theme initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Detect system theme preferences
   */
  _detectSystemPreferences() {
    if (window.matchMedia) {
      // Color scheme preference
      const darkModeQuery = window.matchMedia("(prefers-color-scheme: dark)");
      this.systemPreferences.prefersColorScheme = darkModeQuery.matches
        ? "dark"
        : "light";

      darkModeQuery.addEventListener("change", (e) => {
        this.systemPreferences.prefersColorScheme = e.matches
          ? "dark"
          : "light";
        this._handleSystemPreferenceChange(
          "colorScheme",
          this.systemPreferences.prefersColorScheme
        );
      });

      // High contrast preference
      const highContrastQuery = window.matchMedia("(prefers-contrast: high)");
      this.systemPreferences.prefersHighContrast = highContrastQuery.matches;

      highContrastQuery.addEventListener("change", (e) => {
        this.systemPreferences.prefersHighContrast = e.matches;
        this._handleSystemPreferenceChange("highContrast", e.matches);
      });

      // Reduced motion preference
      const reducedMotionQuery = window.matchMedia(
        "(prefers-reduced-motion: reduce)"
      );
      this.systemPreferences.prefersReducedMotion = reducedMotionQuery.matches;

      reducedMotionQuery.addEventListener("change", (e) => {
        this.systemPreferences.prefersReducedMotion = e.matches;
        this._handleSystemPreferenceChange("reducedMotion", e.matches);
      });
    }

    this.log(
      `System preferences detected: ${JSON.stringify(this.systemPreferences)}`,
      "info"
    );
  }

  /**
   * Load built-in themes
   */
  _loadBuiltInThemes() {
    // Light theme
    const lightTheme = {
      id: "light",
      name: "Light",
      type: THEME_TYPES.LIGHT,
      category: THEME_CATEGORIES.SYSTEM,
      properties: {
        // Primary colors
        "--color-primary": "#0066cc",
        "--color-primary-hover": "#0052a3",
        "--color-primary-active": "#003d7a",
        "--color-primary-light": "#e6f2ff",

        // Secondary colors
        "--color-secondary": "#6c757d",
        "--color-secondary-hover": "#545b62",
        "--color-secondary-active": "#3d4449",

        // Success, warning, error
        "--color-success": "#28a745",
        "--color-warning": "#ffc107",
        "--color-error": "#dc3545",
        "--color-info": "#17a2b8",

        // Background colors
        "--color-background": "#ffffff",
        "--color-background-secondary": "#f8f9fa",
        "--color-background-tertiary": "#e9ecef",
        "--color-background-overlay": "rgba(0, 0, 0, 0.5)",

        // Text colors
        "--color-text": "#212529",
        "--color-text-secondary": "#6c757d",
        "--color-text-muted": "#868e96",
        "--color-text-inverse": "#ffffff",

        // Border colors
        "--color-border": "#dee2e6",
        "--color-border-light": "#e9ecef",
        "--color-border-focus": "#80bdff",

        // Shadow
        "--shadow-sm": "0 0.125rem 0.25rem rgba(0, 0, 0, 0.075)",
        "--shadow": "0 0.5rem 1rem rgba(0, 0, 0, 0.15)",
        "--shadow-lg": "0 1rem 3rem rgba(0, 0, 0, 0.175)",

        // Typography
        "--font-family-sans":
          'system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif',
        "--font-family-mono":
          '"SF Mono", Monaco, Inconsolata, "Roboto Mono", monospace',
        "--font-size-base": "1rem",
        "--line-height-base": "1.5",

        // Spacing
        "--spacing-xs": "0.25rem",
        "--spacing-sm": "0.5rem",
        "--spacing-md": "1rem",
        "--spacing-lg": "1.5rem",
        "--spacing-xl": "3rem",

        // Border radius
        "--border-radius-sm": "0.25rem",
        "--border-radius": "0.375rem",
        "--border-radius-lg": "0.5rem",
        "--border-radius-xl": "1rem",

        // Transitions
        "--transition-fast": "0.15s ease-in-out",
        "--transition-base": "0.3s ease-in-out",
        "--transition-slow": "0.5s ease-in-out",
      },
    };

    // Dark theme
    const darkTheme = {
      id: "dark",
      name: "Dark",
      type: THEME_TYPES.DARK,
      category: THEME_CATEGORIES.SYSTEM,
      properties: {
        ...lightTheme.properties,

        // Override colors for dark theme
        "--color-primary": "#4dabf7",
        "--color-primary-hover": "#339af0",
        "--color-primary-active": "#228be6",
        "--color-primary-light": "#1a365d",

        "--color-background": "#1a1a1a",
        "--color-background-secondary": "#2d2d2d",
        "--color-background-tertiary": "#404040",
        "--color-background-overlay": "rgba(0, 0, 0, 0.8)",

        "--color-text": "#ffffff",
        "--color-text-secondary": "#cccccc",
        "--color-text-muted": "#999999",
        "--color-text-inverse": "#000000",

        "--color-border": "#404040",
        "--color-border-light": "#333333",
        "--color-border-focus": "#4dabf7",

        "--shadow-sm": "0 0.125rem 0.25rem rgba(0, 0, 0, 0.3)",
        "--shadow": "0 0.5rem 1rem rgba(0, 0, 0, 0.4)",
        "--shadow-lg": "0 1rem 3rem rgba(0, 0, 0, 0.5)",
      },
    };

    // High contrast theme
    const highContrastTheme = {
      id: "high-contrast",
      name: "High Contrast",
      type: THEME_TYPES.HIGH_CONTRAST,
      category: THEME_CATEGORIES.ACCESSIBILITY,
      properties: {
        ...lightTheme.properties,

        // High contrast colors
        "--color-primary": "#0000ff",
        "--color-primary-hover": "#0000cc",
        "--color-primary-active": "#000099",

        "--color-background": "#ffffff",
        "--color-background-secondary": "#f0f0f0",
        "--color-background-tertiary": "#e0e0e0",

        "--color-text": "#000000",
        "--color-text-secondary": "#000000",
        "--color-text-muted": "#333333",

        "--color-border": "#000000",
        "--color-border-light": "#666666",
        "--color-border-focus": "#ff0000",

        "--color-success": "#008000",
        "--color-warning": "#ff8c00",
        "--color-error": "#ff0000",

        // Enhanced shadows and borders
        "--shadow-sm": "0 0 0 2px #000000",
        "--shadow": "0 0 0 3px #000000",
        "--shadow-lg": "0 0 0 4px #000000",
      },
    };

    // Register built-in themes
    this.availableThemes.set(lightTheme.id, lightTheme);
    this.availableThemes.set(darkTheme.id, darkTheme);
    this.availableThemes.set(highContrastTheme.id, highContrastTheme);

    this.log(`Built-in themes loaded: ${this.availableThemes.size}`, "success");
  }

  /**
   * Load custom themes from storage
   */
  _loadCustomThemes() {
    if (!this.options.enableCustomThemes || !this.options.enablePersistence)
      return;

    try {
      const customThemesData = localStorage.getItem(
        `${this.options.storageKey}_custom`
      );
      if (customThemesData) {
        const customThemes = JSON.parse(customThemesData);

        Object.entries(customThemes).forEach(([id, theme]) => {
          this.customThemes.set(id, theme);
          this.availableThemes.set(id, theme);
        });

        this.log(`Custom themes loaded: ${this.customThemes.size}`, "success");
      }
    } catch (error) {
      this.log(`Failed to load custom themes: ${error.message}`, "error");
    }
  }

  /**
   * Load user theme preferences
   */
  _loadUserPreferences() {
    if (!this.options.enablePersistence) return;

    try {
      const preferences = localStorage.getItem(this.options.storageKey);
      if (preferences) {
        const data = JSON.parse(preferences);

        // Load theme history
        if (data.themeHistory) {
          this.themeHistory = data.themeHistory;
        }

        return data;
      }
    } catch (error) {
      this.log(`Failed to load user preferences: ${error.message}`, "error");
    }

    return null;
  }

  /**
   * Apply initial theme based on preferences and system settings
   */
  _applyInitialTheme() {
    let themeToApply = this.options.defaultTheme;

    // Load user preference
    const userPrefs = this._loadUserPreferences();
    if (userPrefs && userPrefs.currentTheme) {
      themeToApply = userPrefs.currentTheme;
    }

    // Auto-detect system preference if enabled
    if (this.options.enableAutoDetection && themeToApply === "auto") {
      if (this.systemPreferences.prefersHighContrast) {
        themeToApply = "high-contrast";
      } else {
        themeToApply = this.systemPreferences.prefersColorScheme;
      }
    }

    // Apply the determined theme
    this.applyTheme(themeToApply);
  }

  /**
   * Apply a theme to the interface
   */
  async applyTheme(themeId, options = {}) {
    try {
      const theme = this.availableThemes.get(themeId);
      if (!theme) {
        throw new Error(`Theme not found: ${themeId}`);
      }

      const startTime = Date.now();

      // Prepare for theme transition
      if (
        this.options.enableTransitions &&
        !this.systemPreferences.prefersReducedMotion
      ) {
        await this._prepareThemeTransition();
      }

      // Apply theme properties
      this._applyThemeProperties(theme);

      // Update theme state
      const previousTheme = this.currentTheme;
      this.currentTheme = theme;

      // Add theme to history
      this.themeHistory.unshift({
        id: themeId,
        timestamp: Date.now(),
        applied: true,
      });

      // Limit history size
      if (this.themeHistory.length > 50) {
        this.themeHistory = this.themeHistory.slice(0, 50);
      }

      // Save preferences
      if (this.options.enablePersistence) {
        this._saveUserPreferences();
      }

      // Complete transition
      if (
        this.options.enableTransitions &&
        !this.systemPreferences.prefersReducedMotion
      ) {
        await this._completeThemeTransition();
      }

      // Update performance metrics
      const transitionTime = Date.now() - startTime;
      this.performanceMetrics.themeChanges++;
      this.performanceMetrics.averageTransitionTime =
        (this.performanceMetrics.averageTransitionTime + transitionTime) / 2;

      // Emit theme change event
      this.eventManager.emit("themeChanged", {
        theme: theme.id,
        previousTheme: previousTheme?.id,
        transitionTime,
        timestamp: Date.now(),
      });

      this.log(`Theme applied: ${theme.name} (${transitionTime}ms)`, "success");
      return { success: true, theme, transitionTime };
    } catch (error) {
      this.log(`Failed to apply theme: ${error.message}`, "error");
      return { success: false, error: error.message };
    }
  }

  /**
   * Apply theme CSS custom properties
   */
  _applyThemeProperties(theme) {
    const root = document.documentElement;

    // Apply each CSS custom property
    Object.entries(theme.properties).forEach(([property, value]) => {
      root.style.setProperty(property, value);
      this.cssCustomProperties.set(property, value);
    });

    // Add theme-specific classes
    document.body.className = document.body.className
      .split(" ")
      .filter((cls) => !cls.startsWith("theme-"))
      .concat(`theme-${theme.id}`)
      .join(" ");

    // Add theme type class
    document.body.classList.add(`theme-type-${theme.type}`);

    // Add theme category class
    document.body.classList.add(`theme-category-${theme.category}`);
  }

  /**
   * Create a custom theme
   */
  createCustomTheme(themeData) {
    if (!this.options.enableCustomThemes) {
      throw new Error("Custom themes are disabled");
    }

    if (this.customThemes.size >= this.options.maxCustomThemes) {
      throw new Error(
        `Maximum custom themes limit reached (${this.options.maxCustomThemes})`
      );
    }

    const theme = {
      id: themeData.id || `custom-${Date.now()}`,
      name: themeData.name || `Custom Theme ${this.customThemes.size + 1}`,
      type: THEME_TYPES.CUSTOM,
      category: THEME_CATEGORIES.CUSTOM,
      properties: { ...themeData.properties },
      createdAt: Date.now(),
      modifiedAt: Date.now(),
      version: "1.0",
    };

    // Validate theme properties
    if (!this._validateThemeProperties(theme.properties)) {
      throw new Error("Invalid theme properties");
    }

    // Store custom theme
    this.customThemes.set(theme.id, theme);
    this.availableThemes.set(theme.id, theme);

    // Save to storage
    if (this.options.enablePersistence) {
      this._saveCustomThemes();
    }

    // Emit theme created event
    this.eventManager.emit("customThemeCreated", {
      theme: theme.id,
      name: theme.name,
      timestamp: Date.now(),
    });

    this.log(`Custom theme created: ${theme.name}`, "success");
    return theme;
  }

  /**
   * Modify an existing custom theme
   */
  modifyCustomTheme(themeId, modifications) {
    const theme = this.customThemes.get(themeId);
    if (!theme) {
      throw new Error(`Custom theme not found: ${themeId}`);
    }

    // Apply modifications
    if (modifications.name) {
      theme.name = modifications.name;
    }

    if (modifications.properties) {
      Object.assign(theme.properties, modifications.properties);
    }

    theme.modifiedAt = Date.now();

    // Validate modified properties
    if (!this._validateThemeProperties(theme.properties)) {
      throw new Error("Invalid theme properties after modification");
    }

    // Update in available themes
    this.availableThemes.set(themeId, theme);

    // Save to storage
    if (this.options.enablePersistence) {
      this._saveCustomThemes();
    }

    // If this is the current theme, re-apply it
    if (this.currentTheme?.id === themeId) {
      this.applyTheme(themeId);
    }

    // Emit theme modified event
    this.eventManager.emit("customThemeModified", {
      theme: themeId,
      modifications,
      timestamp: Date.now(),
    });

    this.log(`Custom theme modified: ${theme.name}`, "success");
    return theme;
  }

  /**
   * Delete a custom theme
   */
  deleteCustomTheme(themeId) {
    const theme = this.customThemes.get(themeId);
    if (!theme) {
      throw new Error(`Custom theme not found: ${themeId}`);
    }

    // If this is the current theme, switch to default
    if (this.currentTheme?.id === themeId) {
      this.applyTheme(this.options.defaultTheme);
    }

    // Remove from collections
    this.customThemes.delete(themeId);
    this.availableThemes.delete(themeId);

    // Save to storage
    if (this.options.enablePersistence) {
      this._saveCustomThemes();
    }

    // Emit theme deleted event
    this.eventManager.emit("customThemeDeleted", {
      theme: themeId,
      name: theme.name,
      timestamp: Date.now(),
    });

    this.log(`Custom theme deleted: ${theme.name}`, "success");
    return true;
  }

  /**
   * Export theme data
   */
  exportTheme(themeId) {
    const theme = this.availableThemes.get(themeId);
    if (!theme) {
      throw new Error(`Theme not found: ${themeId}`);
    }

    const exportData = {
      ...theme,
      exportedAt: Date.now(),
      exportVersion: "1.0",
      sourceApplication: "Huntmaster Audio Engine",
    };

    return JSON.stringify(exportData, null, 2);
  }

  /**
   * Import theme data
   */
  importTheme(themeData) {
    try {
      const theme =
        typeof themeData === "string" ? JSON.parse(themeData) : themeData;

      // Validate imported theme
      if (!theme.id || !theme.name || !theme.properties) {
        throw new Error("Invalid theme format");
      }

      // Generate new ID if it conflicts
      let themeId = theme.id;
      let counter = 1;
      while (this.availableThemes.has(themeId)) {
        themeId = `${theme.id}-${counter}`;
        counter++;
      }

      // Create imported theme
      const importedTheme = {
        ...theme,
        id: themeId,
        type: THEME_TYPES.CUSTOM,
        category: THEME_CATEGORIES.CUSTOM,
        importedAt: Date.now(),
        imported: true,
      };

      // Validate theme properties
      if (!this._validateThemeProperties(importedTheme.properties)) {
        throw new Error("Invalid theme properties");
      }

      // Add to custom themes
      this.customThemes.set(themeId, importedTheme);
      this.availableThemes.set(themeId, importedTheme);

      // Save to storage
      if (this.options.enablePersistence) {
        this._saveCustomThemes();
      }

      // Emit theme imported event
      this.eventManager.emit("themeImported", {
        theme: themeId,
        name: importedTheme.name,
        timestamp: Date.now(),
      });

      this.log(`Theme imported: ${importedTheme.name}`, "success");
      return importedTheme;
    } catch (error) {
      this.log(`Failed to import theme: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Get current theme information
   */
  getCurrentTheme() {
    return this.currentTheme ? { ...this.currentTheme } : null;
  }

  /**
   * Get all available themes
   */
  getAvailableThemes() {
    return Array.from(this.availableThemes.values()).map((theme) => ({
      id: theme.id,
      name: theme.name,
      type: theme.type,
      category: theme.category,
    }));
  }

  /**
   * Get theme management status
   */
  getThemeStatus() {
    return {
      isInitialized: this.isInitialized,
      currentTheme: this.currentTheme?.id,
      availableThemes: this.availableThemes.size,
      customThemes: this.customThemes.size,
      systemPreferences: { ...this.systemPreferences },
      performanceMetrics: { ...this.performanceMetrics },
      themeHistory: this.themeHistory.slice(0, 10), // Last 10 changes
      timestamp: Date.now(),
    };
  }

  /**
   * Validate theme properties
   */
  _validateThemeProperties(properties) {
    const requiredProperties = [
      "--color-primary",
      "--color-background",
      "--color-text",
      "--color-border",
    ];

    return requiredProperties.every((prop) => properties.hasOwnProperty(prop));
  }

  /**
   * Save user preferences to storage
   */
  _saveUserPreferences() {
    if (!this.options.enablePersistence) return;

    try {
      const preferences = {
        currentTheme: this.currentTheme?.id,
        themeHistory: this.themeHistory,
        lastSaved: Date.now(),
      };

      localStorage.setItem(
        this.options.storageKey,
        JSON.stringify(preferences)
      );
    } catch (error) {
      this.log(`Failed to save preferences: ${error.message}`, "error");
    }
  }

  /**
   * Save custom themes to storage
   */
  _saveCustomThemes() {
    if (!this.options.enablePersistence) return;

    try {
      const customThemesData = {};
      this.customThemes.forEach((theme, id) => {
        customThemesData[id] = theme;
      });

      localStorage.setItem(
        `${this.options.storageKey}_custom`,
        JSON.stringify(customThemesData)
      );
    } catch (error) {
      this.log(`Failed to save custom themes: ${error.message}`, "error");
    }
  }

  /**
   * Set up CSS custom properties system
   */
  _setupCSSCustomProperties() {
    // Create CSS custom properties style element
    const styleElement = document.createElement("style");
    styleElement.id = "ui-themes-custom-properties";
    document.head.appendChild(styleElement);

    this.log("CSS custom properties system initialized", "success");
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Handle system preference changes
    this.eventManager.on("systemPreferenceChanged", (data) => {
      this._handleSystemPreferenceChange(data.preference, data.value);
    });

    // Handle accessibility changes
    this.eventManager.on("accessibilityChanged", (data) => {
      if (data.feature === "highContrast" && data.enabled) {
        this.applyTheme("high-contrast");
      }
    });
  }

  /**
   * Handle system preference changes
   */
  _handleSystemPreferenceChange(preference, value) {
    this.log(`System preference changed: ${preference} = ${value}`, "info");

    // Auto-switch theme based on system preferences
    if (this.options.enableAutoDetection) {
      if (
        preference === "colorScheme" &&
        this.currentTheme?.type !== THEME_TYPES.HIGH_CONTRAST
      ) {
        this.applyTheme(value);
      } else if (preference === "highContrast" && value) {
        this.applyTheme("high-contrast");
      }
    }

    // Emit system preference change event
    this.eventManager.emit("systemPreferenceChanged", {
      preference,
      value,
      timestamp: Date.now(),
    });
  }

  /**
   * Prepare theme transition animation
   */
  async _prepareThemeTransition() {
    return new Promise((resolve) => {
      document.body.classList.add("theme-transitioning");

      // Add transition styles
      const style = document.createElement("style");
      style.id = "theme-transition-styles";
      style.textContent = `
        .theme-transitioning * {
          transition: background-color ${this.options.transitionDuration}ms ease-in-out,
                     color ${this.options.transitionDuration}ms ease-in-out,
                     border-color ${this.options.transitionDuration}ms ease-in-out,
                     box-shadow ${this.options.transitionDuration}ms ease-in-out !important;
        }
      `;
      document.head.appendChild(style);

      setTimeout(resolve, 50); // Brief delay to ensure styles are applied
    });
  }

  /**
   * Complete theme transition animation
   */
  async _completeThemeTransition() {
    return new Promise((resolve) => {
      setTimeout(() => {
        document.body.classList.remove("theme-transitioning");

        // Remove transition styles
        const transitionStyles = document.getElementById(
          "theme-transition-styles"
        );
        if (transitionStyles) {
          transitionStyles.remove();
        }

        resolve();
      }, this.options.transitionDuration);
    });
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [UIThemes] ${message}`;

    switch (level) {
      case "error":
        console.error(logMessage);
        break;
      case "warning":
        console.warn(logMessage);
        break;
      case "success":
        console.log(`✅ ${logMessage}`);
        break;
      default:
        if (this.options.debugMode) {
          console.log(logMessage);
        }
    }

    // Emit log event
    if (this.eventManager) {
      this.eventManager.emit("themesLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "UIThemes",
      });
    }
  }

  /**
   * Clean up and destroy theme system
   */
  destroy() {
    try {
      // Clear transition timeouts
      this.transitionTimeouts.forEach((timeout) => clearTimeout(timeout));
      this.transitionTimeouts.clear();

      // Remove theme-specific styles
      const customPropertiesStyle = document.getElementById(
        "ui-themes-custom-properties"
      );
      if (customPropertiesStyle) {
        customPropertiesStyle.remove();
      }

      const transitionStyles = document.getElementById(
        "theme-transition-styles"
      );
      if (transitionStyles) {
        transitionStyles.remove();
      }

      // Clear CSS custom properties
      this.cssCustomProperties.forEach((value, property) => {
        document.documentElement.style.removeProperty(property);
      });

      // Remove theme classes from body
      document.body.classList.remove(
        ...Array.from(document.body.classList).filter(
          (cls) =>
            cls.startsWith("theme-") ||
            cls.startsWith("theme-type-") ||
            cls.startsWith("theme-category-")
        )
      );

      // Clear references
      this.availableThemes.clear();
      this.customThemes.clear();
      this.cssCustomProperties.clear();
      this.appliedStyles.clear();
      this.themeHistory = [];
      this.currentTheme = null;

      this.isInitialized = false;
      this.log("UIThemes destroyed successfully", "success");
    } catch (error) {
      this.log(`Error during UIThemes destruction: ${error.message}`, "error");
      throw error;
    }
  }
}

export default UIThemes;
export { UIThemes, THEME_TYPES, COLOR_SCHEMES, THEME_CATEGORIES };
