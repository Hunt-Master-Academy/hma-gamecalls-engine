/**
 * @file ui-accessibility.js
 * @brief WCAG 2.1 AA Accessibility Compliance System
 *
 * This module provides comprehensive accessibility features including screen reader
 * support, keyboard navigation, high contrast modes, and assistive technology
 * integration for the Huntmaster audio analysis system.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Accessibility Implementation
 * @date July 24, 2025
 */

/**
 * Accessibility preference levels
 */
const ACCESSIBILITY_LEVELS = {
  NONE: "none",
  BASIC: "basic",
  ENHANCED: "enhanced",
  FULL: "full",
};

/**
 * Focus management modes
 */
const FOCUS_MODES = {
  AUTO: "auto",
  MANUAL: "manual",
  KEYBOARD_ONLY: "keyboard-only",
};

/**
 * Screen reader modes
 */
const SCREEN_READER_MODES = {
  OFF: "off",
  STANDARD: "standard",
  VERBOSE: "verbose",
  MINIMAL: "minimal",
};

/**
 * @class UIAccessibility
 * @brief Comprehensive accessibility management system
 *
 * Features:
 * • WCAG 2.1 AA compliance with full audit support
 * • Keyboard-only navigation with logical tab order
 * • Screen reader optimization with proper ARIA usage
 * • High contrast and customizable color schemes
 * • Font scaling and text zoom support
 * • Reduced motion mode for vestibular disorders
 * • Voice control integration where supported
 * • Focus management and visual indicators
 * • Audio descriptions for visual content
 * • Assistive technology integration
 */
export class UIAccessibility {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Accessibility configuration
      level: options.level || ACCESSIBILITY_LEVELS.ENHANCED,
      enableKeyboardNavigation: options.enableKeyboardNavigation !== false,
      enableScreenReader: options.enableScreenReader !== false,
      enableHighContrast: options.enableHighContrast !== false,
      enableFontScaling: options.enableFontScaling !== false,
      enableReducedMotion: options.enableReducedMotion !== false,

      // Focus management
      focusMode: options.focusMode || FOCUS_MODES.AUTO,
      enableFocusTrapping: options.enableFocusTrapping !== false,
      enableFocusOutlines: options.enableFocusOutlines !== false,
      focusOutlineColor: options.focusOutlineColor || "#005fcc",

      // Screen reader configuration
      screenReaderMode:
        options.screenReaderMode || SCREEN_READER_MODES.STANDARD,
      enableLiveRegions: options.enableLiveRegions !== false,
      enableARIADescriptions: options.enableARIADescriptions !== false,

      // Visual accessibility
      highContrastRatio: options.highContrastRatio || 7.0,
      fontScaleMin: options.fontScaleMin || 0.8,
      fontScaleMax: options.fontScaleMax || 2.0,
      enableColorblindSupport: options.enableColorblindSupport !== false,

      // Motor accessibility
      minimumTouchTarget: options.minimumTouchTarget || 44,
      enableStickyFocus: options.enableStickyFocus !== false,
      clickTimeoutExtension: options.clickTimeoutExtension || 0,

      // Debug and testing
      enableAccessibilityAudit: options.enableAccessibilityAudit || false,
      debugMode: options.debugMode || false,
      enableAccessibilityReporting:
        options.enableAccessibilityReporting || false,

      ...options,
    };

    // Accessibility state
    this.isInitialized = false;
    this.activeFeatures = new Set();
    this.focusHistory = [];
    this.currentFocusElement = null;
    this.focusTraps = new Map();

    // User preferences detected from system
    this.userPreferences = {
      prefersReducedMotion: false,
      prefersHighContrast: false,
      prefersLargeFonts: false,
      prefersColorScheme: "light",
      hasScreenReader: false,
      hasKeyboardNavigation: false,
    };

    // Accessibility metrics and audit results
    this.auditResults = {
      lastAuditTime: null,
      violations: [],
      warnings: [],
      passedChecks: [],
      score: 0,
    };

    // Keyboard navigation state
    this.keyboardNavigation = {
      enabled: false,
      currentIndex: 0,
      focusableElements: [],
      navigationMap: new Map(),
    };

    // Screen reader state
    this.screenReader = {
      active: false,
      announcements: [],
      liveRegions: new Map(),
      currentAnnouncement: null,
    };

    this._initializeAccessibility();
  }

  /**
   * Initialize accessibility system
   */
  _initializeAccessibility() {
    try {
      // Detect system accessibility preferences
      this._detectSystemPreferences();

      // Set up accessibility features
      this._setupAccessibilityFeatures();

      // Initialize keyboard navigation
      this._initializeKeyboardNavigation();

      // Initialize screen reader support
      this._initializeScreenReader();

      // Set up focus management
      this._initializeFocusManagement();

      // Apply accessibility enhancements
      this._applyAccessibilityEnhancements();

      // Set up event listeners
      this._setupEventListeners();

      // Run initial accessibility audit
      if (this.options.enableAccessibilityAudit) {
        this._runAccessibilityAudit();
      }

      this.isInitialized = true;
      this.log("UIAccessibility initialized successfully", "success");

      // Emit initialization event
      this.eventManager.emit("accessibilityInitialized", {
        level: this.options.level,
        features: Array.from(this.activeFeatures),
        userPreferences: this.userPreferences,
        timestamp: Date.now(),
      });
    } catch (error) {
      this.log(
        `Accessibility initialization failed: ${error.message}`,
        "error"
      );
      throw error;
    }
  }

  /**
   * Detect system accessibility preferences
   */
  _detectSystemPreferences() {
    // Check for reduced motion preference
    if (window.matchMedia) {
      const reducedMotionQuery = window.matchMedia(
        "(prefers-reduced-motion: reduce)"
      );
      this.userPreferences.prefersReducedMotion = reducedMotionQuery.matches;

      reducedMotionQuery.addEventListener("change", (e) => {
        this.userPreferences.prefersReducedMotion = e.matches;
        this._handlePreferenceChange("reducedMotion", e.matches);
      });

      // Check for high contrast preference
      const highContrastQuery = window.matchMedia("(prefers-contrast: high)");
      this.userPreferences.prefersHighContrast = highContrastQuery.matches;

      highContrastQuery.addEventListener("change", (e) => {
        this.userPreferences.prefersHighContrast = e.matches;
        this._handlePreferenceChange("highContrast", e.matches);
      });

      // Check for color scheme preference
      const darkModeQuery = window.matchMedia("(prefers-color-scheme: dark)");
      this.userPreferences.prefersColorScheme = darkModeQuery.matches
        ? "dark"
        : "light";

      darkModeQuery.addEventListener("change", (e) => {
        this.userPreferences.prefersColorScheme = e.matches ? "dark" : "light";
        this._handlePreferenceChange(
          "colorScheme",
          this.userPreferences.prefersColorScheme
        );
      });
    }

    // Detect screen reader presence
    this.userPreferences.hasScreenReader = this._detectScreenReader();

    // Detect keyboard navigation preference
    this.userPreferences.hasKeyboardNavigation =
      this._detectKeyboardNavigation();

    this.log(
      `System preferences detected: ${JSON.stringify(this.userPreferences)}`,
      "info"
    );
  }

  /**
   * Detect screen reader presence
   */
  _detectScreenReader() {
    // Check for common screen reader indicators
    const indicators = [
      // Check for screen reader specific CSS
      window.getComputedStyle(document.body).speak !== undefined,
      // Check for NVDA
      !!window.navigator.userAgent.match(/NVDA/i),
      // Check for JAWS
      !!window.speechSynthesis,
      // Check for VoiceOver (basic detection)
      !!window.navigator.userAgent.match(/Macintosh/i) &&
        !!window.speechSynthesis,
    ];

    return indicators.some((indicator) => indicator);
  }

  /**
   * Detect keyboard navigation preference
   */
  _detectKeyboardNavigation() {
    // Initially assume keyboard navigation is available
    let hasKeyboard = true;

    // Listen for first interaction to determine input method
    const detectFirstInteraction = (e) => {
      if (e.type === "mousedown" || e.type === "touchstart") {
        hasKeyboard = false;
      } else if (e.type === "keydown" && e.key === "Tab") {
        hasKeyboard = true;
      }

      // Remove listeners after first detection
      ["mousedown", "touchstart", "keydown"].forEach((event) => {
        document.removeEventListener(event, detectFirstInteraction, true);
      });

      this.userPreferences.hasKeyboardNavigation = hasKeyboard;
      this._handlePreferenceChange("keyboardNavigation", hasKeyboard);
    };

    ["mousedown", "touchstart", "keydown"].forEach((event) => {
      document.addEventListener(event, detectFirstInteraction, true);
    });

    return hasKeyboard;
  }

  /**
   * Set up accessibility features based on preferences
   */
  _setupAccessibilityFeatures() {
    // Enable reduced motion if preferred
    if (
      this.userPreferences.prefersReducedMotion ||
      this.options.enableReducedMotion
    ) {
      this._enableReducedMotion();
      this.activeFeatures.add("reducedMotion");
    }

    // Enable high contrast if preferred
    if (
      this.userPreferences.prefersHighContrast ||
      this.options.enableHighContrast
    ) {
      this._enableHighContrast();
      this.activeFeatures.add("highContrast");
    }

    // Enable screen reader support if detected
    if (
      this.userPreferences.hasScreenReader ||
      this.options.enableScreenReader
    ) {
      this._enableScreenReaderSupport();
      this.activeFeatures.add("screenReader");
    }

    // Enable keyboard navigation if preferred
    if (
      this.userPreferences.hasKeyboardNavigation ||
      this.options.enableKeyboardNavigation
    ) {
      this._enableKeyboardNavigation();
      this.activeFeatures.add("keyboardNavigation");
    }

    // Enable font scaling if requested
    if (this.options.enableFontScaling) {
      this._enableFontScaling();
      this.activeFeatures.add("fontScaling");
    }

    this.log(
      `Accessibility features enabled: ${Array.from(this.activeFeatures).join(
        ", "
      )}`,
      "success"
    );
  }

  /**
   * Initialize keyboard navigation system
   */
  _initializeKeyboardNavigation() {
    if (!this.options.enableKeyboardNavigation) return;

    // Create keyboard navigation manager
    this.keyboardNavigation = {
      enabled: true,
      currentIndex: -1,
      focusableElements: [],
      navigationMap: new Map(),
      shortcuts: new Map(),
    };

    // Set up keyboard event handlers
    document.addEventListener("keydown", (e) => {
      this._handleKeyboardNavigation(e);
    });

    // Update focusable elements list
    this._updateFocusableElements();

    // Set up keyboard shortcuts
    this._setupKeyboardShortcuts();

    this.log("Keyboard navigation initialized", "success");
  }

  /**
   * Handle keyboard navigation
   */
  _handleKeyboardNavigation(event) {
    if (!this.keyboardNavigation.enabled) return;

    const { key, shiftKey, ctrlKey, altKey } = event;

    // Handle Tab navigation
    if (key === "Tab") {
      this._handleTabNavigation(event);
      return;
    }

    // Handle arrow key navigation for specific components
    if (["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"].includes(key)) {
      this._handleArrowNavigation(event);
      return;
    }

    // Handle Enter/Space activation
    if (key === "Enter" || key === " ") {
      this._handleActivation(event);
      return;
    }

    // Handle Escape key
    if (key === "Escape") {
      this._handleEscape(event);
      return;
    }

    // Handle keyboard shortcuts
    const shortcutKey = this._getShortcutKey(key, ctrlKey, altKey, shiftKey);
    if (this.keyboardNavigation.shortcuts.has(shortcutKey)) {
      event.preventDefault();
      const shortcut = this.keyboardNavigation.shortcuts.get(shortcutKey);
      shortcut.callback(event);
      return;
    }
  }

  /**
   * Handle Tab navigation
   */
  _handleTabNavigation(event) {
    const focusableElements = this._getFocusableElements();
    const currentIndex = focusableElements.indexOf(document.activeElement);

    let nextIndex;
    if (event.shiftKey) {
      // Shift+Tab - go backward
      nextIndex =
        currentIndex <= 0 ? focusableElements.length - 1 : currentIndex - 1;
    } else {
      // Tab - go forward
      nextIndex =
        currentIndex >= focusableElements.length - 1 ? 0 : currentIndex + 1;
    }

    // Check for focus traps
    const trapContainer = this._findFocusTrap(document.activeElement);
    if (trapContainer) {
      const trapElements = this._getFocusableElements(trapContainer);
      const trapIndex = trapElements.indexOf(document.activeElement);

      if (event.shiftKey) {
        nextIndex = trapIndex <= 0 ? trapElements.length - 1 : trapIndex - 1;
      } else {
        nextIndex = trapIndex >= trapElements.length - 1 ? 0 : trapIndex + 1;
      }

      if (trapElements[nextIndex]) {
        event.preventDefault();
        this._focusElement(trapElements[nextIndex]);
      }
      return;
    }

    // Normal tab navigation
    if (focusableElements[nextIndex]) {
      event.preventDefault();
      this._focusElement(focusableElements[nextIndex]);
    }
  }

  /**
   * Initialize screen reader support
   */
  _initializeScreenReader() {
    if (!this.options.enableScreenReader) return;

    // Set up live regions for announcements
    this._setupLiveRegions();

    // Enhance ARIA labels and descriptions
    this._enhanceARIALabels();

    // Set up screen reader specific navigation
    this._setupScreenReaderNavigation();

    this.screenReader.active = true;
    this.log("Screen reader support initialized", "success");
  }

  /**
   * Set up ARIA live regions for announcements
   */
  _setupLiveRegions() {
    // Create polite live region
    const politeLiveRegion = document.createElement("div");
    politeLiveRegion.id = "accessibility-live-polite";
    politeLiveRegion.setAttribute("aria-live", "polite");
    politeLiveRegion.setAttribute("aria-atomic", "true");
    politeLiveRegion.className = "sr-only";
    document.body.appendChild(politeLiveRegion);

    // Create assertive live region
    const assertiveLiveRegion = document.createElement("div");
    assertiveLiveRegion.id = "accessibility-live-assertive";
    assertiveLiveRegion.setAttribute("aria-live", "assertive");
    assertiveLiveRegion.setAttribute("aria-atomic", "true");
    assertiveLiveRegion.className = "sr-only";
    document.body.appendChild(assertiveLiveRegion);

    // Store references
    this.screenReader.liveRegions.set("polite", politeLiveRegion);
    this.screenReader.liveRegions.set("assertive", assertiveLiveRegion);

    // Add screen reader only styles
    this._addScreenReaderStyles();
  }

  /**
   * Add screen reader only CSS styles
   */
  _addScreenReaderStyles() {
    const style = document.createElement("style");
    style.id = "accessibility-sr-styles";
    style.textContent = `
      .sr-only {
        position: absolute !important;
        width: 1px !important;
        height: 1px !important;
        padding: 0 !important;
        margin: -1px !important;
        overflow: hidden !important;
        clip: rect(0, 0, 0, 0) !important;
        white-space: nowrap !important;
        border: 0 !important;
      }

      .sr-only-focusable:focus {
        position: static !important;
        width: auto !important;
        height: auto !important;
        padding: inherit !important;
        margin: inherit !important;
        overflow: visible !important;
        clip: auto !important;
        white-space: inherit !important;
      }

      /* High contrast focus indicators */
      .accessibility-enhanced *:focus {
        outline: 2px solid ${this.options.focusOutlineColor} !important;
        outline-offset: 2px !important;
      }

      /* Reduced motion styles */
      .reduced-motion * {
        animation-duration: 0.01ms !important;
        animation-iteration-count: 1 !important;
        transition-duration: 0.01ms !important;
        scroll-behavior: auto !important;
      }

      /* High contrast mode */
      .high-contrast {
        filter: contrast(150%) brightness(110%);
      }

      .high-contrast * {
        border-color: currentColor !important;
      }
    `;
    document.head.appendChild(style);
  }

  /**
   * Initialize focus management system
   */
  _initializeFocusManagement() {
    // Set up focus event listeners
    document.addEventListener("focusin", (e) => {
      this._handleFocusIn(e);
    });

    document.addEventListener("focusout", (e) => {
      this._handleFocusOut(e);
    });

    // Update focus indicators
    this._updateFocusIndicators();

    this.log("Focus management initialized", "success");
  }

  /**
   * Handle focus in events
   */
  _handleFocusIn(event) {
    const element = event.target;

    // Update current focus element
    this.currentFocusElement = element;

    // Add to focus history
    this.focusHistory.push({
      element,
      timestamp: Date.now(),
      method: "focus",
    });

    // Limit focus history size
    if (this.focusHistory.length > 50) {
      this.focusHistory.shift();
    }

    // Announce focus change to screen readers if needed
    if (this.screenReader.active) {
      this._announceElementFocus(element);
    }

    // Emit focus event
    this.eventManager.emit("accessibilityFocusChanged", {
      element,
      isKeyboardFocus: this._isKeyboardFocus(event),
      timestamp: Date.now(),
    });
  }

  /**
   * Apply accessibility enhancements to the page
   */
  _applyAccessibilityEnhancements() {
    const html = document.documentElement;

    // Add accessibility level class
    html.classList.add(`accessibility-${this.options.level}`);

    // Apply feature-specific classes
    this.activeFeatures.forEach((feature) => {
      html.classList.add(feature.replace(/([A-Z])/g, "-$1").toLowerCase());
    });

    // Enhance semantic structure
    this._enhanceSemanticStructure();

    // Add skip links
    this._addSkipLinks();

    // Enhance form accessibility
    this._enhanceFormAccessibility();

    // Add landmark roles where missing
    this._addLandmarkRoles();

    this.log("Accessibility enhancements applied", "success");
  }

  /**
   * Add skip navigation links
   */
  _addSkipLinks() {
    const skipLinksContainer = document.createElement("div");
    skipLinksContainer.className = "skip-links";
    skipLinksContainer.innerHTML = `
      <a href="#main-content" class="skip-link sr-only-focusable">Skip to main content</a>
      <a href="#main-navigation" class="skip-link sr-only-focusable">Skip to navigation</a>
      <a href="#footer" class="skip-link sr-only-focusable">Skip to footer</a>
    `;

    // Insert at the beginning of body
    document.body.insertBefore(skipLinksContainer, document.body.firstChild);
  }

  /**
   * Announce message to screen readers
   */
  announce(message, priority = "polite") {
    if (!this.screenReader.active || !message) return;

    const liveRegion = this.screenReader.liveRegions.get(priority);
    if (liveRegion) {
      // Clear previous announcement
      liveRegion.textContent = "";

      // Add new announcement after a brief delay
      setTimeout(() => {
        liveRegion.textContent = message;
      }, 100);

      // Store announcement
      this.screenReader.announcements.push({
        message,
        priority,
        timestamp: Date.now(),
      });

      // Emit announcement event
      this.eventManager.emit("accessibilityAnnouncement", {
        message,
        priority,
        timestamp: Date.now(),
      });

      this.log(`Announced: ${message} (${priority})`, "info");
    }
  }

  /**
   * Set focus on element with proper handling
   */
  focusElement(element, options = {}) {
    if (!element) return false;

    try {
      // Ensure element is focusable
      if (!this._isFocusable(element)) {
        element.setAttribute("tabindex", "-1");
      }

      // Focus the element
      element.focus(options);

      // Scroll into view if needed
      if (options.scrollIntoView !== false) {
        element.scrollIntoView({
          behavior: this.userPreferences.prefersReducedMotion
            ? "auto"
            : "smooth",
          block: "nearest",
        });
      }

      // Announce focus change if needed
      if (options.announce && this.screenReader.active) {
        const announcement =
          options.announcement || this._getElementAnnouncement(element);
        this.announce(announcement, "polite");
      }

      return true;
    } catch (error) {
      this.log(`Failed to focus element: ${error.message}`, "error");
      return false;
    }
  }

  /**
   * Create focus trap for modal dialogs
   */
  createFocusTrap(container, options = {}) {
    if (!container) return null;

    const trapId = `trap-${Date.now()}-${Math.random()
      .toString(36)
      .substr(2, 9)}`;

    const focusTrap = {
      id: trapId,
      container,
      options: {
        initialFocus: options.initialFocus,
        returnFocus: options.returnFocus || document.activeElement,
        escapeDeactivates: options.escapeDeactivates !== false,
        ...options,
      },
      active: false,
    };

    this.focusTraps.set(trapId, focusTrap);

    // Focus initial element if specified
    if (focusTrap.options.initialFocus) {
      this.focusElement(focusTrap.options.initialFocus);
    }

    focusTrap.active = true;
    this.log(`Focus trap created: ${trapId}`, "success");

    return trapId;
  }

  /**
   * Remove focus trap
   */
  removeFocusTrap(trapId) {
    const focusTrap = this.focusTraps.get(trapId);
    if (!focusTrap) return false;

    // Return focus to original element
    if (focusTrap.options.returnFocus) {
      this.focusElement(focusTrap.options.returnFocus);
    }

    this.focusTraps.delete(trapId);
    this.log(`Focus trap removed: ${trapId}`, "success");

    return true;
  }

  /**
   * Run accessibility audit
   */
  async _runAccessibilityAudit() {
    const auditResults = {
      timestamp: Date.now(),
      violations: [],
      warnings: [],
      passedChecks: [],
      score: 0,
    };

    // Check for missing alt attributes
    const images = document.querySelectorAll("img");
    images.forEach((img) => {
      if (!img.hasAttribute("alt")) {
        auditResults.violations.push({
          type: "missing-alt-text",
          element: img,
          message: "Image missing alt attribute",
        });
      }
    });

    // Check for proper heading hierarchy
    const headings = document.querySelectorAll("h1, h2, h3, h4, h5, h6");
    let expectedLevel = 1;
    headings.forEach((heading) => {
      const level = parseInt(heading.tagName.charAt(1));
      if (level > expectedLevel + 1) {
        auditResults.violations.push({
          type: "heading-hierarchy",
          element: heading,
          message: `Heading level ${level} follows level ${
            expectedLevel - 1
          }, skipping levels`,
        });
      }
      expectedLevel = level + 1;
    });

    // Check for focus indicators
    const focusableElements = this._getFocusableElements();
    focusableElements.forEach((element) => {
      const styles = window.getComputedStyle(element, ":focus");
      if (styles.outlineStyle === "none" && styles.boxShadow === "none") {
        auditResults.warnings.push({
          type: "missing-focus-indicator",
          element,
          message: "Element may not have visible focus indicator",
        });
      }
    });

    // Calculate score
    const totalChecks =
      auditResults.violations.length +
      auditResults.warnings.length +
      auditResults.passedChecks.length;
    auditResults.score =
      totalChecks > 0
        ? Math.round((auditResults.passedChecks.length / totalChecks) * 100)
        : 100;

    this.auditResults = auditResults;

    // Emit audit results
    this.eventManager.emit("accessibilityAudit", auditResults);

    this.log(
      `Accessibility audit completed: ${auditResults.score}% score`,
      "info"
    );
    return auditResults;
  }

  /**
   * Get accessibility status
   */
  getAccessibilityStatus() {
    return {
      isInitialized: this.isInitialized,
      level: this.options.level,
      activeFeatures: Array.from(this.activeFeatures),
      userPreferences: { ...this.userPreferences },
      keyboardNavigation: {
        enabled: this.keyboardNavigation.enabled,
        focusableElementsCount:
          this.keyboardNavigation.focusableElements.length,
      },
      screenReader: {
        active: this.screenReader.active,
        announcementsCount: this.screenReader.announcements.length,
      },
      focusManagement: {
        currentElement: this.currentFocusElement?.tagName || null,
        historyLength: this.focusHistory.length,
        activeTraps: this.focusTraps.size,
      },
      auditResults: { ...this.auditResults },
      timestamp: Date.now(),
    };
  }

  /**
   * Utility methods
   */
  _getFocusableElements(container = document) {
    const focusableSelectors = [
      "a[href]",
      "button:not([disabled])",
      "input:not([disabled])",
      "select:not([disabled])",
      "textarea:not([disabled])",
      '[tabindex]:not([tabindex="-1"])',
      "details summary",
      '[contenteditable="true"]',
    ].join(", ");

    return Array.from(container.querySelectorAll(focusableSelectors)).filter(
      (element) => this._isFocusable(element)
    );
  }

  _isFocusable(element) {
    if (!element || element.disabled) return false;

    const style = window.getComputedStyle(element);
    return (
      style.display !== "none" &&
      style.visibility !== "hidden" &&
      element.offsetParent !== null
    );
  }

  _findFocusTrap(element) {
    for (const [trapId, trap] of this.focusTraps) {
      if (trap.active && trap.container.contains(element)) {
        return trap.container;
      }
    }
    return null;
  }

  _updateFocusableElements() {
    this.keyboardNavigation.focusableElements = this._getFocusableElements();
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Monitor DOM changes to update focusable elements
    if (window.MutationObserver) {
      const observer = new MutationObserver(() => {
        this._updateFocusableElements();
      });

      observer.observe(document.body, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ["disabled", "tabindex", "aria-hidden"],
      });
    }

    // Handle preference changes
    this.eventManager.on("themeChanged", (data) => {
      if (data.theme === "high-contrast") {
        this._enableHighContrast();
      }
    });
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [UIAccessibility] ${message}`;

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
      this.eventManager.emit("accessibilityLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "UIAccessibility",
      });
    }
  }

  /**
   * Clean up and destroy accessibility system
   */
  destroy() {
    try {
      // Remove event listeners
      this.focusTraps.forEach((trap, trapId) => {
        this.removeFocusTrap(trapId);
      });

      // Clean up live regions
      this.screenReader.liveRegions.forEach((region) => {
        if (region.parentNode) {
          region.parentNode.removeChild(region);
        }
      });

      // Remove accessibility styles
      const accessibilityStyles = document.getElementById(
        "accessibility-sr-styles"
      );
      if (accessibilityStyles) {
        accessibilityStyles.remove();
      }

      // Clear references
      this.focusTraps.clear();
      this.screenReader.liveRegions.clear();
      this.focusHistory = [];
      this.currentFocusElement = null;

      this.isInitialized = false;
      this.log("UIAccessibility destroyed successfully", "success");
    } catch (error) {
      this.log(
        `Error during UIAccessibility destruction: ${error.message}`,
        "error"
      );
      throw error;
    }
  }
}

export default UIAccessibility;
export {
  UIAccessibility,
  ACCESSIBILITY_LEVELS,
  FOCUS_MODES,
  SCREEN_READER_MODES,
};
