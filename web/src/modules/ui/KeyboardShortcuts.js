/**
 * KeyboardShortcuts.js - Comprehensive Keyboard Control System
 *
 * Advanced keyboard shortcut management system with configurable key bindings,
 * context-aware shortcuts, help overlay system, full accessibility support,
 * and international keyboard layout compatibility.
 *
 * Features:
 * - Configurable keyboard shortcuts with conflict detection
 * - Context-aware key bindings for different application states
 * - Interactive help overlay with shortcut visualization
 * - Full accessibility compliance with screen reader support
 * - International keyboard layout support (QWERTY, AZERTY, etc.)
 * - Modifier key combinations (Ctrl, Alt, Shift, Meta)
 * - Sequential key combinations and chords
 * - Customizable shortcut themes and presets
 *
 * Dependencies: EventManager, PerformanceMonitor
 */

import { EventManager } from "../core/EventManager.js";
import { PerformanceMonitor } from "../core/PerformanceMonitor.js";

export class KeyboardShortcuts {
  constructor(options = {}) {
    this.options = {
      // General settings
      enabled: options.enabled !== false,
      globalCapture: options.globalCapture || false,
      preventDefault: options.preventDefault !== false,

      // Context management
      enableContexts: options.enableContexts !== false,
      defaultContext: options.defaultContext || "global",
      contextSwitchDelay: options.contextSwitchDelay || 100,

      // Key combination settings
      enableSequences: options.enableSequences || false,
      sequenceTimeout: options.sequenceTimeout || 1000,
      enableChords: options.enableChords || false,
      chordTimeout: options.chordTimeout || 500,

      // Modifier key settings
      modifierKeys: options.modifierKeys || ["ctrl", "alt", "shift", "meta"],
      caseSensitive: options.caseSensitive || false,

      // Help system
      enableHelpOverlay: options.enableHelpOverlay !== false,
      helpKey: options.helpKey || "F1",
      helpTheme: options.helpTheme || "dark",

      // Accessibility
      enableAccessibility: options.enableAccessibility !== false,
      announceShortcuts: options.announceShortcuts || false,
      highContrastMode: options.highContrastMode || false,

      // International support
      keyboardLayout: options.keyboardLayout || "qwerty",
      enableLayoutDetection: options.enableLayoutDetection || false,

      // Performance
      maxShortcuts: options.maxShortcuts || 1000,
      debounceDelay: options.debounceDelay || 50,

      ...options,
    };

    // Initialize state
    this.isInitialized = false;
    this.isActive = false;
    this.currentContext = this.options.defaultContext;

    // Shortcut storage
    this.shortcuts = new Map(); // Map<context, Map<keyCombo, action>>
    this.globalShortcuts = new Map();
    this.sequenceBuffer = [];
    this.chordBuffer = new Set();

    // Key state tracking
    this.pressedKeys = new Set();
    this.modifierState = {
      ctrl: false,
      alt: false,
      shift: false,
      meta: false,
    };

    // Context management
    this.contextHistory = [];
    this.contextSwitchTimeout = null;

    // Help system
    this.helpOverlay = null;
    this.helpVisible = false;

    // Sequence and chord state
    this.sequenceTimeout = null;
    this.chordTimeout = null;
    this.lastKeyTime = 0;

    // Performance tracking
    this.shortcutCount = 0;
    this.recognitionTime = 0;

    // Accessibility
    this.screenReaderAnnouncer = null;

    // Keyboard layout detection
    this.detectedLayout = null;
    this.layoutMappings = new Map();

    // Event system
    this.eventManager = EventManager.getInstance();
    this.performanceMonitor = PerformanceMonitor.getInstance();

    // Initialize component
    this.init();
  }

  /**
   * Initialize the keyboard shortcuts system
   * TODO: Set up keyboard event listeners
   * TODO: Initialize keyboard layout detection
   * TODO: Create help overlay system
   * TODO: Set up accessibility features
   * TODO: Register default shortcuts
   */
  async init() {
    try {
      this.performanceMonitor.startOperation("KeyboardShortcuts.init");

      // TODO: Set up keyboard event listeners
      this.setupKeyboardEvents();

      // TODO: Initialize keyboard layout detection
      if (this.options.enableLayoutDetection) {
        await this.detectKeyboardLayout();
      }

      // TODO: Initialize layout mappings
      this.initLayoutMappings();

      // TODO: Set up help overlay system
      if (this.options.enableHelpOverlay) {
        this.initHelpSystem();
      }

      // TODO: Set up accessibility features
      if (this.options.enableAccessibility) {
        this.setupAccessibilityFeatures();
      }

      // TODO: Register default shortcuts
      this.registerDefaultShortcuts();

      // TODO: Initialize contexts
      this.initContextSystem();

      this.isInitialized = true;
      this.isActive = true;

      this.eventManager.emit("keyboardShortcuts:initialized", {
        component: "KeyboardShortcuts",
        layout: this.detectedLayout,
        contexts: Array.from(this.shortcuts.keys()),
        options: this.options,
      });

      this.performanceMonitor.endOperation("KeyboardShortcuts.init");
    } catch (error) {
      console.error("KeyboardShortcuts initialization failed:", error);
      this.eventManager.emit("keyboardShortcuts:error", {
        error: error.message,
        component: "KeyboardShortcuts",
      });
      throw error;
    }
  }

  /**
   * Set up keyboard event listeners
   * TODO: Register keydown, keyup, keypress events
   * TODO: Configure event capturing options
   * TODO: Handle special keys and combinations
   * TODO: Set up global vs local capture
   */
  setupKeyboardEvents() {
    const target = this.options.globalCapture ? document : document.body;
    const options = { capture: true, passive: false };

    // TODO: Keydown event handler
    this.keydownHandler = (event) => {
      this.handleKeyDown(event);
    };
    target.addEventListener("keydown", this.keydownHandler, options);

    // TODO: Keyup event handler
    this.keyupHandler = (event) => {
      this.handleKeyUp(event);
    };
    target.addEventListener("keyup", this.keyupHandler, options);

    // TODO: Focus/blur handlers for context management
    window.addEventListener("focus", () => {
      this.resetKeyState();
    });

    window.addEventListener("blur", () => {
      this.resetKeyState();
    });
  }

  /**
   * Detect keyboard layout for international support
   * TODO: Use Keyboard API if available
   * TODO: Implement layout detection heuristics
   * TODO: Handle layout switching
   * TODO: Store layout preferences
   */
  async detectKeyboardLayout() {
    try {
      // TODO: Use Keyboard API if available
      if ("keyboard" in navigator && "getLayoutMap" in navigator.keyboard) {
        const layoutMap = await navigator.keyboard.getLayoutMap();
        this.detectedLayout = this.analyzeLayoutMap(layoutMap);
      } else {
        // TODO: Fallback to heuristic detection
        this.detectedLayout = this.detectLayoutHeuristically();
      }

      this.eventManager.emit("keyboardShortcuts:layoutDetected", {
        layout: this.detectedLayout,
      });
    } catch (error) {
      console.warn("Keyboard layout detection failed:", error);
      this.detectedLayout = this.options.keyboardLayout;
    }
  }

  /**
   * Analyze keyboard layout map to determine layout type
   * TODO: Check key positions for QWERTY vs AZERTY vs DVORAK
   * TODO: Identify modifier key positions
   * TODO: Handle regional variations
   */
  analyzeLayoutMap(layoutMap) {
    // TODO: Check key positions to identify layout
    const qKey = layoutMap.get("KeyQ");
    const aKey = layoutMap.get("KeyA");
    const zKey = layoutMap.get("KeyZ");

    if (qKey === "a" && aKey === "q") {
      return "azerty";
    } else if (qKey === "'" && aKey === "a") {
      return "dvorak";
    } else {
      return "qwerty";
    }
  }

  /**
   * Initialize keyboard layout mappings for different layouts
   * TODO: Create mapping tables for different layouts
   * TODO: Handle special character mappings
   * TODO: Set up modifier key variations
   */
  initLayoutMappings() {
    // TODO: QWERTY layout (default)
    this.layoutMappings.set(
      "qwerty",
      new Map([
        ["KeyQ", "q"],
        ["KeyW", "w"],
        ["KeyE", "e"],
        ["KeyR", "r"],
        ["KeyA", "a"],
        ["KeyS", "s"],
        ["KeyD", "d"],
        ["KeyF", "f"],
        ["KeyZ", "z"],
        ["KeyX", "x"],
        ["KeyC", "c"],
        ["KeyV", "v"],
      ])
    );

    // TODO: AZERTY layout
    this.layoutMappings.set(
      "azerty",
      new Map([
        ["KeyQ", "a"],
        ["KeyW", "z"],
        ["KeyE", "e"],
        ["KeyR", "r"],
        ["KeyA", "q"],
        ["KeyS", "s"],
        ["KeyD", "d"],
        ["KeyF", "f"],
        ["KeyZ", "w"],
        ["KeyX", "x"],
        ["KeyC", "c"],
        ["KeyV", "v"],
      ])
    );

    // TODO: DVORAK layout
    this.layoutMappings.set(
      "dvorak",
      new Map([
        ["KeyQ", "'"],
        ["KeyW", ","],
        ["KeyE", "."],
        ["KeyR", "p"],
        ["KeyA", "a"],
        ["KeyS", "o"],
        ["KeyD", "e"],
        ["KeyF", "u"],
        ["KeyZ", ";"],
        ["KeyX", "q"],
        ["KeyC", "j"],
        ["KeyV", "k"],
      ])
    );
  }

  /**
   * Initialize help overlay system
   * TODO: Create help overlay DOM structure
   * TODO: Set up help content templates
   * TODO: Configure help overlay styling
   * TODO: Add keyboard navigation for help
   */
  initHelpSystem() {
    // TODO: Create help overlay container
    this.helpOverlay = document.createElement("div");
    this.helpOverlay.className = `keyboard-help-overlay theme-${this.options.helpTheme}`;
    this.helpOverlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.8);
            z-index: 10000;
            display: none;
            overflow-y: auto;
        `;

    // TODO: Create help content container
    const helpContent = document.createElement("div");
    helpContent.className = "help-content";
    this.helpOverlay.appendChild(helpContent);

    // TODO: Add to document
    document.body.appendChild(this.helpOverlay);

    // TODO: Set up help overlay event handlers
    this.helpOverlay.addEventListener("click", (event) => {
      if (event.target === this.helpOverlay) {
        this.hideHelp();
      }
    });
  }

  /**
   * Set up accessibility features
   * TODO: Create screen reader announcer
   * TODO: Set up high contrast mode
   * TODO: Configure ARIA attributes
   * TODO: Add focus management
   */
  setupAccessibilityFeatures() {
    // TODO: Create screen reader announcer
    if (this.options.announceShortcuts) {
      this.screenReaderAnnouncer = document.createElement("div");
      this.screenReaderAnnouncer.setAttribute("aria-live", "polite");
      this.screenReaderAnnouncer.setAttribute("aria-atomic", "true");
      this.screenReaderAnnouncer.style.cssText = `
                position: absolute;
                left: -10000px;
                width: 1px;
                height: 1px;
                overflow: hidden;
            `;
      document.body.appendChild(this.screenReaderAnnouncer);
    }

    // TODO: High contrast mode
    if (this.options.highContrastMode) {
      document.body.classList.add("keyboard-shortcuts-high-contrast");
    }
  }

  /**
   * Register default shortcuts for audio application
   * TODO: Register playback control shortcuts
   * TODO: Set up navigation shortcuts
   * TODO: Configure editing shortcuts
   * TODO: Add accessibility shortcuts
   */
  registerDefaultShortcuts() {
    // TODO: Playback controls (global context)
    this.register("space", "audio:togglePlayback", "global");
    this.register("ctrl+space", "audio:stop", "global");
    this.register("ctrl+r", "audio:record", "global");

    // TODO: Navigation shortcuts
    this.register("left", "navigation:previous", "global");
    this.register("right", "navigation:next", "global");
    this.register("up", "navigation:volumeUp", "global");
    this.register("down", "navigation:volumeDown", "global");

    // TODO: View shortcuts
    this.register("ctrl+=", "view:zoomIn", "global");
    this.register("ctrl+-", "view:zoomOut", "global");
    this.register("ctrl+0", "view:resetZoom", "global");
    this.register("ctrl+f", "view:toggleFullscreen", "global");

    // TODO: Editing shortcuts (editing context)
    this.register("ctrl+z", "edit:undo", "editing");
    this.register("ctrl+y", "edit:redo", "editing");
    this.register("ctrl+x", "edit:cut", "editing");
    this.register("ctrl+c", "edit:copy", "editing");
    this.register("ctrl+v", "edit:paste", "editing");

    // TODO: Help and accessibility
    this.register(this.options.helpKey, "help:toggle", "global");
    this.register("alt+h", "help:show", "global");
    this.register("escape", "ui:escape", "global");
  }

  /**
   * Initialize context system for context-aware shortcuts
   * TODO: Set up context switching logic
   * TODO: Create context inheritance
   * TODO: Configure context priority
   */
  initContextSystem() {
    if (!this.options.enableContexts) return;

    // TODO: Initialize default contexts
    this.shortcuts.set("global", new Map());
    this.shortcuts.set("editing", new Map());
    this.shortcuts.set("playback", new Map());
    this.shortcuts.set("recording", new Map());
    this.shortcuts.set("analysis", new Map());

    // TODO: Set up context inheritance
    this.contextInheritance = new Map([
      ["editing", ["global"]],
      ["playback", ["global"]],
      ["recording", ["global"]],
      ["analysis", ["global"]],
    ]);
  }

  /**
   * Handle keydown events
   * TODO: Track pressed keys and modifiers
   * TODO: Build key combination string
   * TODO: Check for matching shortcuts
   * TODO: Handle sequences and chords
   */
  handleKeyDown(event) {
    if (!this.isActive) return;

    try {
      const currentTime = Date.now();
      this.performanceMonitor.startFrame();

      // TODO: Update modifier state
      this.updateModifierState(event);

      // TODO: Add key to pressed keys set
      this.pressedKeys.add(event.code);

      // TODO: Build key combination string
      const keyCombo = this.buildKeyCombo(event);

      // TODO: Handle sequences if enabled
      if (this.options.enableSequences) {
        this.handleSequence(keyCombo, currentTime);
      }

      // TODO: Handle chords if enabled
      if (this.options.enableChords) {
        this.handleChord(keyCombo, currentTime);
      }

      // TODO: Check for direct shortcut match
      const shortcut = this.findShortcut(keyCombo);
      if (shortcut) {
        if (this.options.preventDefault) {
          event.preventDefault();
        }
        this.executeShortcut(shortcut, keyCombo, event);
      }

      this.lastKeyTime = currentTime;
      this.performanceMonitor.endFrame();
    } catch (error) {
      console.error("Keydown handling failed:", error);
    }
  }

  /**
   * Handle keyup events
   * TODO: Remove keys from pressed set
   * TODO: Update modifier state
   * TODO: Handle chord completion
   */
  handleKeyUp(event) {
    if (!this.isActive) return;

    try {
      // TODO: Update modifier state
      this.updateModifierState(event);

      // TODO: Remove key from pressed keys set
      this.pressedKeys.delete(event.code);

      // TODO: Handle chord completion
      if (this.options.enableChords && this.chordBuffer.size > 0) {
        this.processChordCompletion();
      }
    } catch (error) {
      console.error("Keyup handling failed:", error);
    }
  }

  /**
   * Build key combination string from event
   * TODO: Include modifier keys in correct order
   * TODO: Map physical keys to logical keys
   * TODO: Handle keyboard layout differences
   * TODO: Normalize key names
   */
  buildKeyCombo(event) {
    const parts = [];

    // TODO: Add modifiers in consistent order
    if (event.ctrlKey) parts.push("ctrl");
    if (event.altKey) parts.push("alt");
    if (event.shiftKey) parts.push("shift");
    if (event.metaKey) parts.push("meta");

    // TODO: Map physical key to logical key
    let key = event.code;
    const layoutMapping = this.layoutMappings.get(
      this.detectedLayout || "qwerty"
    );
    if (layoutMapping && layoutMapping.has(event.code)) {
      key = layoutMapping.get(event.code);
    } else {
      // TODO: Fallback to event.key with normalization
      key = event.key.toLowerCase();
    }

    // TODO: Handle special cases
    if (key.startsWith("digit")) {
      key = key.replace("digit", "");
    } else if (key.startsWith("key")) {
      key = key.replace("key", "").toLowerCase();
    }

    parts.push(key);

    return parts.join("+");
  }

  /**
   * Find shortcut matching key combination
   * TODO: Check current context first
   * TODO: Check inherited contexts
   * TODO: Check global shortcuts
   * TODO: Handle context priority
   */
  findShortcut(keyCombo) {
    // TODO: Check current context
    const currentContextShortcuts = this.shortcuts.get(this.currentContext);
    if (currentContextShortcuts && currentContextShortcuts.has(keyCombo)) {
      return currentContextShortcuts.get(keyCombo);
    }

    // TODO: Check inherited contexts
    const inheritedContexts =
      this.contextInheritance.get(this.currentContext) || [];
    for (const context of inheritedContexts) {
      const contextShortcuts = this.shortcuts.get(context);
      if (contextShortcuts && contextShortcuts.has(keyCombo)) {
        return contextShortcuts.get(keyCombo);
      }
    }

    // TODO: Check global shortcuts
    if (this.globalShortcuts.has(keyCombo)) {
      return this.globalShortcuts.get(keyCombo);
    }

    return null;
  }

  /**
   * Register a keyboard shortcut
   * TODO: Validate key combination format
   * TODO: Check for conflicts
   * TODO: Store in appropriate context
   * TODO: Update help system
   */
  register(keyCombo, action, context = "global", options = {}) {
    try {
      // TODO: Validate parameters
      if (!keyCombo || !action) {
        throw new Error("Key combination and action are required");
      }

      // TODO: Normalize key combination
      const normalizedCombo = this.normalizeKeyCombo(keyCombo);

      // TODO: Check for conflicts
      if (options.overwrite !== true) {
        const existingShortcut = this.findShortcut(normalizedCombo);
        if (existingShortcut) {
          console.warn(
            `Shortcut conflict: ${normalizedCombo} already mapped to ${existingShortcut.action}`
          );
        }
      }

      // TODO: Create shortcut object
      const shortcut = {
        keyCombo: normalizedCombo,
        action: action,
        context: context,
        description: options.description || action,
        enabled: options.enabled !== false,
        preventDefault: options.preventDefault !== false,
        category: options.category || "general",
        priority: options.priority || 1,
      };

      // TODO: Store in appropriate context
      if (context === "global") {
        this.globalShortcuts.set(normalizedCombo, shortcut);
      } else {
        if (!this.shortcuts.has(context)) {
          this.shortcuts.set(context, new Map());
        }
        this.shortcuts.get(context).set(normalizedCombo, shortcut);
      }

      // TODO: Update help system
      this.updateHelpContent();

      this.eventManager.emit("keyboardShortcuts:registered", {
        shortcut: shortcut,
      });

      return shortcut;
    } catch (error) {
      console.error("Shortcut registration failed:", error);
      throw error;
    }
  }

  /**
   * Execute a matched shortcut
   * TODO: Check if shortcut is enabled
   * TODO: Emit action event
   * TODO: Update usage statistics
   * TODO: Announce to screen reader if enabled
   */
  executeShortcut(shortcut, keyCombo, event) {
    try {
      const startTime = performance.now();

      // TODO: Check if shortcut is enabled
      if (!shortcut.enabled) return;

      // TODO: Emit action event
      this.eventManager.emit(shortcut.action, {
        keyCombo: keyCombo,
        shortcut: shortcut,
        originalEvent: event,
        context: this.currentContext,
        timestamp: Date.now(),
      });

      // TODO: Emit generic shortcut executed event
      this.eventManager.emit("keyboardShortcuts:executed", {
        shortcut: shortcut,
        keyCombo: keyCombo,
        context: this.currentContext,
      });

      // TODO: Announce to screen reader
      if (this.options.announceShortcuts && this.screenReaderAnnouncer) {
        this.announceShortcut(shortcut);
      }

      // TODO: Update statistics
      this.shortcutCount++;
      this.recognitionTime += performance.now() - startTime;
    } catch (error) {
      console.error("Shortcut execution failed:", error);
    }
  }

  /**
   * Switch to a different shortcut context
   * TODO: Validate context exists
   * TODO: Store context history
   * TODO: Emit context change event
   * TODO: Update help content
   */
  switchContext(newContext, options = {}) {
    try {
      if (newContext === this.currentContext) return;

      // TODO: Store in context history
      if (options.addToHistory !== false) {
        this.contextHistory.push(this.currentContext);
      }

      const previousContext = this.currentContext;
      this.currentContext = newContext;

      // TODO: Clear any active sequences or chords
      this.clearSequenceBuffer();
      this.clearChordBuffer();

      // TODO: Update help content
      this.updateHelpContent();

      this.eventManager.emit("keyboardShortcuts:contextChanged", {
        previousContext: previousContext,
        newContext: newContext,
        history: this.contextHistory,
      });
    } catch (error) {
      console.error("Context switching failed:", error);
    }
  }

  /**
   * Show help overlay with current shortcuts
   * TODO: Generate help content for current context
   * TODO: Show overlay with animation
   * TODO: Set focus for keyboard navigation
   */
  showHelp() {
    if (!this.helpOverlay) return;

    try {
      // TODO: Update help content
      this.updateHelpContent();

      // TODO: Show overlay
      this.helpOverlay.style.display = "flex";
      this.helpVisible = true;

      // TODO: Set focus for keyboard navigation
      this.helpOverlay.focus();

      this.eventManager.emit("keyboardShortcuts:helpShown");
    } catch (error) {
      console.error("Help display failed:", error);
    }
  }

  /**
   * Hide help overlay
   * TODO: Hide overlay with animation
   * TODO: Restore focus to previous element
   * TODO: Clear help state
   */
  hideHelp() {
    if (!this.helpOverlay) return;

    try {
      // TODO: Hide overlay
      this.helpOverlay.style.display = "none";
      this.helpVisible = false;

      this.eventManager.emit("keyboardShortcuts:helpHidden");
    } catch (error) {
      console.error("Help hiding failed:", error);
    }
  }

  /**
   * Export shortcut configuration
   * TODO: Export all registered shortcuts
   * TODO: Include context information
   * TODO: Export usage statistics
   */
  exportConfiguration() {
    const config = {
      contexts: {},
      globalShortcuts: Object.fromEntries(this.globalShortcuts),
      currentContext: this.currentContext,
      detectedLayout: this.detectedLayout,
      statistics: {
        shortcutCount: this.shortcutCount,
        averageRecognitionTime: this.recognitionTime / this.shortcutCount || 0,
      },
    };

    // TODO: Export context shortcuts
    for (const [context, shortcuts] of this.shortcuts) {
      config.contexts[context] = Object.fromEntries(shortcuts);
    }

    return config;
  }

  /**
   * Clean up resources and event listeners
   * TODO: Remove keyboard event listeners
   * TODO: Clear timeouts and intervals
   * TODO: Remove help overlay
   * TODO: Clean up accessibility elements
   */
  destroy() {
    try {
      this.isActive = false;

      // TODO: Remove event listeners
      if (this.keydownHandler) {
        document.removeEventListener("keydown", this.keydownHandler);
      }
      if (this.keyupHandler) {
        document.removeEventListener("keyup", this.keyupHandler);
      }

      // TODO: Clear timeouts
      if (this.sequenceTimeout) {
        clearTimeout(this.sequenceTimeout);
      }
      if (this.chordTimeout) {
        clearTimeout(this.chordTimeout);
      }

      // TODO: Remove help overlay
      if (this.helpOverlay && this.helpOverlay.parentNode) {
        this.helpOverlay.parentNode.removeChild(this.helpOverlay);
      }

      // TODO: Remove screen reader announcer
      if (this.screenReaderAnnouncer && this.screenReaderAnnouncer.parentNode) {
        this.screenReaderAnnouncer.parentNode.removeChild(
          this.screenReaderAnnouncer
        );
      }

      this.eventManager.emit("keyboardShortcuts:destroyed");
    } catch (error) {
      console.error("Keyboard shortcuts cleanup failed:", error);
    }
  }

  // Helper methods (TODO: Implement these)
  normalizeKeyCombo(keyCombo) {
    return keyCombo.toLowerCase();
  }
  updateModifierState(event) {
    /* TODO */
  }
  handleSequence(keyCombo, time) {
    /* TODO */
  }
  handleChord(keyCombo, time) {
    /* TODO */
  }
  clearSequenceBuffer() {
    this.sequenceBuffer = [];
  }
  clearChordBuffer() {
    this.chordBuffer.clear();
  }
  updateHelpContent() {
    /* TODO */
  }
  announceShortcut(shortcut) {
    /* TODO */
  }
  resetKeyState() {
    this.pressedKeys.clear();
  }

  // Getter methods for external access
  get isReady() {
    return this.isInitialized;
  }
  get currentShortcuts() {
    return this.shortcuts.get(this.currentContext) || new Map();
  }
  get allContexts() {
    return Array.from(this.shortcuts.keys());
  }
  get usageStats() {
    return {
      count: this.shortcutCount,
      averageTime: this.recognitionTime / this.shortcutCount || 0,
    };
  }
}

export default KeyboardShortcuts;
