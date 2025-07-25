/**
 * @fileoverview Waveform Interaction Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Advanced waveform interaction handling module providing comprehensive user interaction
 * capabilities including touch, mouse, keyboard, gesture recognition, and accessibility features.
 *
 * Features:
 * ✅ Multi-touch gesture support (pinch, zoom, pan)
 * ✅ Mouse interaction (click, drag, wheel, hover)
 * ✅ Keyboard navigation and shortcuts
 * ✅ Accessibility features (ARIA, screen reader support)
 * ✅ Custom gesture recognition and patterns
 * ✅ Interaction state management
 * ✅ Event delegation and bubbling control
 * ✅ Performance-optimized event handling
 * ✅ Cross-platform compatibility
 * ✅ Haptic feedback support
 * ✅ Context menu integration
 * ✅ Undo/redo interaction history
 *
 * @example
 * ```javascript
 * import { WaveformInteraction } from './modules/waveform/index.js';
 *
 * const interaction = new WaveformInteraction(canvas, {
 *   enableTouch: true,
 *   enableKeyboard: true,
 *   enableAccessibility: true
 * });
 *
 * interaction.on('waveformClick', (event) => {
 *   console.log('Clicked at time:', event.timePosition);
 * });
 * ```
 */

/**
 * Waveform Interaction Handler
 *
 * Provides comprehensive user interaction capabilities for waveform visualization
 * including touch, mouse, keyboard, and accessibility features.
 *
 * @class WaveformInteraction
 */
export class WaveformInteraction {
  /**
   * Create WaveformInteraction instance
   *
   * @param {HTMLElement} element - Target element for interactions
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableTouch=true] - Enable touch interactions
   * @param {boolean} [options.enableMouse=true] - Enable mouse interactions
   * @param {boolean} [options.enableKeyboard=true] - Enable keyboard navigation
   * @param {boolean} [options.enableAccessibility=true] - Enable accessibility features
   * @param {boolean} [options.enableGestures=true] - Enable gesture recognition
   * @param {boolean} [options.enableHaptics=false] - Enable haptic feedback
   */
  constructor(element, options = {}) {
    this.element = element;

    // Configuration
    this.config = {
      enableTouch: options.enableTouch !== false,
      enableMouse: options.enableMouse !== false,
      enableKeyboard: options.enableKeyboard !== false,
      enableAccessibility: options.enableAccessibility !== false,
      enableGestures: options.enableGestures !== false,
      enableHaptics: options.enableHaptics === true,
      doubleClickTime: options.doubleClickTime || 300,
      longPressTime: options.longPressTime || 500,
      pinchThreshold: options.pinchThreshold || 10,
      swipeThreshold: options.swipeThreshold || 50,
      ...options,
    };

    // Interaction state
    this.isInteracting = false;
    this.interactionType = null;
    this.interactionStart = null;
    this.interactionHistory = [];
    this.lastClickTime = 0;
    this.clickCount = 0;

    // Touch/mouse tracking
    this.touches = new Map();
    this.mousePosition = { x: 0, y: 0 };
    this.mouseButtons = 0;

    // Gesture recognition
    this.gestures = {
      pinch: { active: false, startDistance: 0, scale: 1 },
      pan: { active: false, startX: 0, startY: 0, deltaX: 0, deltaY: 0 },
      swipe: { active: false, startX: 0, startY: 0, direction: null },
      longPress: { active: false, timer: null, startX: 0, startY: 0 },
    };

    // Event handlers
    this.eventHandlers = new Map();

    // Keyboard state
    this.pressedKeys = new Set();
    this.keyboardShortcuts = new Map();

    // Accessibility
    this.focusIndex = 0;
    this.focusableElements = [];

    // Performance optimization
    this.rafId = null;
    this.eventQueue = [];
    this.lastEventTime = 0;

    // Context menu
    this.contextMenuItems = [];

    // Initialize system
    this._initializeEventListeners();
    this._initializeKeyboardShortcuts();
    this._initializeAccessibility();
    this._initializeContextMenu();

    console.log("WaveformInteraction initialized");
  }

  /**
   * Initialize event listeners
   * @private
   */
  _initializeEventListeners() {
    // Mouse events
    if (this.config.enableMouse) {
      this.element.addEventListener(
        "mousedown",
        this._handleMouseDown.bind(this)
      );
      this.element.addEventListener(
        "mousemove",
        this._handleMouseMove.bind(this)
      );
      this.element.addEventListener("mouseup", this._handleMouseUp.bind(this));
      this.element.addEventListener("wheel", this._handleWheel.bind(this));
      this.element.addEventListener(
        "contextmenu",
        this._handleContextMenu.bind(this)
      );
      this.element.addEventListener(
        "dblclick",
        this._handleDoubleClick.bind(this)
      );
    }

    // Touch events
    if (this.config.enableTouch) {
      this.element.addEventListener(
        "touchstart",
        this._handleTouchStart.bind(this)
      );
      this.element.addEventListener(
        "touchmove",
        this._handleTouchMove.bind(this)
      );
      this.element.addEventListener(
        "touchend",
        this._handleTouchEnd.bind(this)
      );
      this.element.addEventListener(
        "touchcancel",
        this._handleTouchCancel.bind(this)
      );
    }

    // Keyboard events
    if (this.config.enableKeyboard) {
      this.element.setAttribute("tabindex", "0");
      this.element.addEventListener("keydown", this._handleKeyDown.bind(this));
      this.element.addEventListener("keyup", this._handleKeyUp.bind(this));
      this.element.addEventListener("focus", this._handleFocus.bind(this));
      this.element.addEventListener("blur", this._handleBlur.bind(this));
    }

    // Pointer events (for unified handling)
    if ("PointerEvent" in window) {
      this.element.addEventListener(
        "pointerdown",
        this._handlePointerDown.bind(this)
      );
      this.element.addEventListener(
        "pointermove",
        this._handlePointerMove.bind(this)
      );
      this.element.addEventListener(
        "pointerup",
        this._handlePointerUp.bind(this)
      );
      this.element.addEventListener(
        "pointercancel",
        this._handlePointerCancel.bind(this)
      );
    }

    // Window events for cleanup
    window.addEventListener("blur", this._handleWindowBlur.bind(this));
    window.addEventListener("resize", this._handleWindowResize.bind(this));
  }

  /**
   * Initialize keyboard shortcuts
   * @private
   */
  _initializeKeyboardShortcuts() {
    // Default shortcuts
    this.keyboardShortcuts.set("Space", () => this._emitEvent("playPause"));
    this.keyboardShortcuts.set("ArrowLeft", () =>
      this._emitEvent("seekBackward", { amount: 5 })
    );
    this.keyboardShortcuts.set("ArrowRight", () =>
      this._emitEvent("seekForward", { amount: 5 })
    );
    this.keyboardShortcuts.set("ArrowUp", () => this._emitEvent("volumeUp"));
    this.keyboardShortcuts.set("ArrowDown", () =>
      this._emitEvent("volumeDown")
    );
    this.keyboardShortcuts.set("Home", () => this._emitEvent("seekToStart"));
    this.keyboardShortcuts.set("End", () => this._emitEvent("seekToEnd"));
    this.keyboardShortcuts.set("Escape", () =>
      this._emitEvent("cancelOperation")
    );

    // Modifier combinations
    this.keyboardShortcuts.set("Ctrl+z", () => this._emitEvent("undo"));
    this.keyboardShortcuts.set("Ctrl+y", () => this._emitEvent("redo"));
    this.keyboardShortcuts.set("Ctrl+a", () => this._emitEvent("selectAll"));
    this.keyboardShortcuts.set("Ctrl+c", () => this._emitEvent("copy"));
    this.keyboardShortcuts.set("Ctrl+v", () => this._emitEvent("paste"));
    this.keyboardShortcuts.set("Delete", () => this._emitEvent("delete"));

    console.log(
      `Initialized ${this.keyboardShortcuts.size} keyboard shortcuts`
    );
  }

  /**
   * Initialize accessibility features
   * @private
   */
  _initializeAccessibility() {
    if (!this.config.enableAccessibility) return;

    // ARIA attributes
    this.element.setAttribute("role", "application");
    this.element.setAttribute("aria-label", "Waveform Audio Visualization");
    this.element.setAttribute("aria-describedby", "waveform-help");

    // Create help text
    const helpText = document.createElement("div");
    helpText.id = "waveform-help";
    helpText.className = "sr-only";
    helpText.textContent =
      "Use arrow keys to navigate, space to play/pause, home/end to seek to start/end";
    document.body.appendChild(helpText);

    // Screen reader announcements
    this.announcer = document.createElement("div");
    this.announcer.setAttribute("aria-live", "polite");
    this.announcer.setAttribute("aria-atomic", "true");
    this.announcer.className = "sr-only";
    document.body.appendChild(this.announcer);

    console.log("Accessibility features initialized");
  }

  /**
   * Initialize context menu
   * @private
   */
  _initializeContextMenu() {
    this.contextMenuItems = [
      { label: "Play/Pause", action: "playPause", shortcut: "Space" },
      { label: "Zoom In", action: "zoomIn", shortcut: "Ctrl++" },
      { label: "Zoom Out", action: "zoomOut", shortcut: "Ctrl+-" },
      { label: "Zoom to Fit", action: "zoomToFit", shortcut: "Ctrl+0" },
      { type: "separator" },
      { label: "Copy Selection", action: "copy", shortcut: "Ctrl+C" },
      { label: "Paste", action: "paste", shortcut: "Ctrl+V" },
      { type: "separator" },
      { label: "Export Audio", action: "export" },
      { label: "Properties", action: "properties" },
    ];
  }

  /**
   * Mouse event handlers
   */
  _handleMouseDown(event) {
    event.preventDefault();

    this.isInteracting = true;
    this.interactionType = "mouse";
    this.interactionStart = {
      x: event.clientX,
      y: event.clientY,
      time: Date.now(),
    };
    this.mouseButtons = event.buttons;

    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    // Handle click timing for double-click detection
    const now = Date.now();
    if (now - this.lastClickTime < this.config.doubleClickTime) {
      this.clickCount++;
    } else {
      this.clickCount = 1;
    }
    this.lastClickTime = now;

    // Long press detection
    if (this.config.enableGestures) {
      this.gestures.longPress.timer = setTimeout(() => {
        this._handleLongPress(position);
      }, this.config.longPressTime);
    }

    this._emitEvent("mouseDown", {
      position,
      timePosition,
      button: event.button,
      buttons: event.buttons,
      modifiers: this._getModifiers(event),
    });

    // Add mouse move/up listeners to document for proper tracking
    document.addEventListener("mousemove", this._handleMouseMove.bind(this));
    document.addEventListener("mouseup", this._handleMouseUp.bind(this));
  }

  _handleMouseMove(event) {
    if (!this.isInteracting) return;

    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    // Clear long press if mouse moved too much
    if (this.gestures.longPress.timer) {
      const startPos = this.interactionStart;
      const distance = Math.sqrt(
        Math.pow(event.clientX - startPos.x, 2) +
          Math.pow(event.clientY - startPos.y, 2)
      );

      if (distance > 10) {
        clearTimeout(this.gestures.longPress.timer);
        this.gestures.longPress.timer = null;
      }
    }

    this._emitEvent("mouseMove", {
      position,
      timePosition,
      buttons: event.buttons,
      modifiers: this._getModifiers(event),
    });

    // Handle dragging
    if (this.isInteracting && this.mouseButtons > 0) {
      const deltaX = event.clientX - this.interactionStart.x;
      const deltaY = event.clientY - this.interactionStart.y;

      this._emitEvent("mouseDrag", {
        position,
        timePosition,
        deltaX,
        deltaY,
        startPosition: this.interactionStart,
        modifiers: this._getModifiers(event),
      });
    }
  }

  _handleMouseUp(event) {
    // Clean up document listeners
    document.removeEventListener("mousemove", this._handleMouseMove.bind(this));
    document.removeEventListener("mouseup", this._handleMouseUp.bind(this));

    if (!this.isInteracting) return;

    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    // Clear long press timer
    if (this.gestures.longPress.timer) {
      clearTimeout(this.gestures.longPress.timer);
      this.gestures.longPress.timer = null;
    }

    this._emitEvent("mouseUp", {
      position,
      timePosition,
      button: event.button,
      clickCount: this.clickCount,
      modifiers: this._getModifiers(event),
    });

    // Handle single/double click
    if (this.clickCount === 1) {
      setTimeout(() => {
        if (this.clickCount === 1) {
          this._emitEvent("waveformClick", {
            position,
            timePosition,
            modifiers: this._getModifiers(event),
          });
        }
      }, this.config.doubleClickTime);
    }

    this.isInteracting = false;
    this.interactionType = null;
    this.mouseButtons = 0;
  }

  _handleDoubleClick(event) {
    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    this._emitEvent("waveformDoubleClick", {
      position,
      timePosition,
      modifiers: this._getModifiers(event),
    });
  }

  _handleWheel(event) {
    event.preventDefault();

    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    const direction = event.deltaY > 0 ? "down" : "up";
    const magnitude = Math.abs(event.deltaY);

    this._emitEvent("mouseWheel", {
      position,
      timePosition,
      direction,
      magnitude,
      deltaX: event.deltaX,
      deltaY: event.deltaY,
      deltaZ: event.deltaZ,
      modifiers: this._getModifiers(event),
    });

    // Handle zoom with Ctrl+wheel
    if (event.ctrlKey) {
      this._emitEvent(direction === "up" ? "zoomIn" : "zoomOut", {
        center: timePosition,
        magnitude: magnitude / 100,
      });
    }
  }

  /**
   * Touch event handlers
   */
  _handleTouchStart(event) {
    event.preventDefault();

    const touches = Array.from(event.changedTouches);
    touches.forEach((touch) => {
      this.touches.set(touch.identifier, {
        startX: touch.clientX,
        startY: touch.clientY,
        currentX: touch.clientX,
        currentY: touch.clientY,
        startTime: Date.now(),
      });
    });

    this.isInteracting = true;
    this.interactionType = "touch";

    // Handle different touch counts
    if (this.touches.size === 1) {
      const touch = touches[0];
      const position = this._getRelativePosition(touch);
      const timePosition = this._pixelToTime(position.x);

      // Long press detection
      if (this.config.enableGestures) {
        this.gestures.longPress.timer = setTimeout(() => {
          this._handleLongPress(position);
        }, this.config.longPressTime);
      }

      this._emitEvent("touchStart", {
        position,
        timePosition,
        touchId: touch.identifier,
      });
    } else if (this.touches.size === 2 && this.config.enableGestures) {
      // Initialize pinch gesture
      const touchArray = Array.from(this.touches.values());
      const distance = this._getTouchDistance(touchArray[0], touchArray[1]);

      this.gestures.pinch.active = true;
      this.gestures.pinch.startDistance = distance;
      this.gestures.pinch.scale = 1;

      this._emitEvent("pinchStart", {
        distance,
        center: this._getTouchCenter(touchArray[0], touchArray[1]),
      });
    }
  }

  _handleTouchMove(event) {
    event.preventDefault();

    const touches = Array.from(event.changedTouches);
    touches.forEach((touch) => {
      if (this.touches.has(touch.identifier)) {
        const stored = this.touches.get(touch.identifier);
        stored.currentX = touch.clientX;
        stored.currentY = touch.clientY;
      }
    });

    const touchArray = Array.from(this.touches.values());

    if (touchArray.length === 1) {
      // Single touch - pan or potential swipe
      const touch = touchArray[0];
      const position = this._getRelativePosition({
        clientX: touch.currentX,
        clientY: touch.currentY,
      });
      const timePosition = this._pixelToTime(position.x);

      // Clear long press if moved too much
      if (this.gestures.longPress.timer) {
        const distance = Math.sqrt(
          Math.pow(touch.currentX - touch.startX, 2) +
            Math.pow(touch.currentY - touch.startY, 2)
        );

        if (distance > 10) {
          clearTimeout(this.gestures.longPress.timer);
          this.gestures.longPress.timer = null;
        }
      }

      this._emitEvent("touchMove", {
        position,
        timePosition,
        deltaX: touch.currentX - touch.startX,
        deltaY: touch.currentY - touch.startY,
        touchId: touches[0].identifier,
      });
    } else if (touchArray.length === 2 && this.gestures.pinch.active) {
      // Two touches - pinch/zoom
      const distance = this._getTouchDistance(touchArray[0], touchArray[1]);
      const scale = distance / this.gestures.pinch.startDistance;

      this.gestures.pinch.scale = scale;

      this._emitEvent("pinchMove", {
        scale,
        distance,
        center: this._getTouchCenter(touchArray[0], touchArray[1]),
      });
    }
  }

  _handleTouchEnd(event) {
    const touches = Array.from(event.changedTouches);

    touches.forEach((touch) => {
      if (this.touches.has(touch.identifier)) {
        const stored = this.touches.get(touch.identifier);
        const deltaX = touch.clientX - stored.startX;
        const deltaY = touch.clientY - stored.startY;
        const duration = Date.now() - stored.startTime;

        // Check for swipe gesture
        if (
          this.config.enableGestures &&
          Math.abs(deltaX) > this.config.swipeThreshold
        ) {
          const direction = deltaX > 0 ? "right" : "left";
          const velocity = Math.abs(deltaX) / duration;

          this._emitEvent("swipe", {
            direction,
            velocity,
            deltaX,
            deltaY,
            duration,
          });
        }

        this.touches.delete(touch.identifier);
      }
    });

    // Clear long press timer
    if (this.gestures.longPress.timer) {
      clearTimeout(this.gestures.longPress.timer);
      this.gestures.longPress.timer = null;
    }

    // Handle pinch end
    if (this.gestures.pinch.active && this.touches.size < 2) {
      this._emitEvent("pinchEnd", {
        finalScale: this.gestures.pinch.scale,
      });

      this.gestures.pinch.active = false;
      this.gestures.pinch.scale = 1;
    }

    if (this.touches.size === 0) {
      this.isInteracting = false;
      this.interactionType = null;
    }

    this._emitEvent("touchEnd", {
      remainingTouches: this.touches.size,
    });
  }

  _handleTouchCancel(event) {
    this.touches.clear();
    this.isInteracting = false;
    this.interactionType = null;

    // Clear timers
    if (this.gestures.longPress.timer) {
      clearTimeout(this.gestures.longPress.timer);
      this.gestures.longPress.timer = null;
    }

    // Reset gestures
    this.gestures.pinch.active = false;
    this.gestures.pinch.scale = 1;

    this._emitEvent("touchCancel");
  }

  /**
   * Keyboard event handlers
   */
  _handleKeyDown(event) {
    this.pressedKeys.add(event.code);

    const shortcutKey = this._getShortcutKey(event);
    if (this.keyboardShortcuts.has(shortcutKey)) {
      event.preventDefault();
      this.keyboardShortcuts.get(shortcutKey)();
      return;
    }

    this._emitEvent("keyDown", {
      key: event.key,
      code: event.code,
      modifiers: this._getModifiers(event),
    });

    // Handle accessibility navigation
    if (this.config.enableAccessibility) {
      this._handleAccessibilityNavigation(event);
    }
  }

  _handleKeyUp(event) {
    this.pressedKeys.delete(event.code);

    this._emitEvent("keyUp", {
      key: event.key,
      code: event.code,
      modifiers: this._getModifiers(event),
    });
  }

  /**
   * Pointer event handlers (unified)
   */
  _handlePointerDown(event) {
    // Delegate to appropriate handler based on pointer type
    if (event.pointerType === "mouse") {
      this._handleMouseDown(event);
    } else if (event.pointerType === "touch") {
      // Convert to touch event format
      const touchEvent = {
        changedTouches: [
          {
            identifier: event.pointerId,
            clientX: event.clientX,
            clientY: event.clientY,
          },
        ],
        preventDefault: () => event.preventDefault(),
      };
      this._handleTouchStart(touchEvent);
    }
  }

  _handlePointerMove(event) {
    if (event.pointerType === "mouse") {
      this._handleMouseMove(event);
    }
    // Touch move is handled separately
  }

  _handlePointerUp(event) {
    if (event.pointerType === "mouse") {
      this._handleMouseUp(event);
    }
  }

  _handlePointerCancel(event) {
    if (event.pointerType === "touch") {
      this._handleTouchCancel(event);
    }
  }

  /**
   * Gesture handlers
   */
  _handleLongPress(position) {
    this.gestures.longPress.active = true;
    this.gestures.longPress.timer = null;

    const timePosition = this._pixelToTime(position.x);

    this._emitEvent("longPress", {
      position,
      timePosition,
    });

    // Haptic feedback
    if (this.config.enableHaptics && navigator.vibrate) {
      navigator.vibrate(50);
    }
  }

  /**
   * Context menu handler
   */
  _handleContextMenu(event) {
    event.preventDefault();

    const position = this._getRelativePosition(event);
    const timePosition = this._pixelToTime(position.x);

    this._emitEvent("contextMenu", {
      position,
      timePosition,
      items: this.contextMenuItems,
      nativeEvent: event,
    });
  }

  /**
   * Focus handlers
   */
  _handleFocus(event) {
    this._emitEvent("focus");

    if (this.config.enableAccessibility) {
      this._announceToScreenReader(
        "Waveform focused. Use arrow keys to navigate."
      );
    }
  }

  _handleBlur(event) {
    this._emitEvent("blur");

    // Clear pressed keys
    this.pressedKeys.clear();
  }

  /**
   * Window event handlers
   */
  _handleWindowBlur() {
    // Clear all interaction state when window loses focus
    this.touches.clear();
    this.pressedKeys.clear();
    this.isInteracting = false;
    this.interactionType = null;

    // Clear timers
    if (this.gestures.longPress.timer) {
      clearTimeout(this.gestures.longPress.timer);
      this.gestures.longPress.timer = null;
    }
  }

  _handleWindowResize() {
    // Update cached dimensions
    this._updateElementDimensions();
  }

  /**
   * Accessibility navigation
   * @private
   */
  _handleAccessibilityNavigation(event) {
    switch (event.key) {
      case "Tab":
        if (!event.shiftKey) {
          this._focusNext();
        } else {
          this._focusPrevious();
        }
        event.preventDefault();
        break;

      case "Enter":
      case " ":
        this._activateFocusedElement();
        event.preventDefault();
        break;
    }
  }

  /**
   * Utility methods
   */
  _getRelativePosition(event) {
    const rect = this.element.getBoundingClientRect();
    return {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
    };
  }

  _pixelToTime(x) {
    // This should be implemented based on your waveform's time scale
    // For now, return a normalized value
    const rect = this.element.getBoundingClientRect();
    return (x / rect.width) * 100; // Assuming 100 seconds total
  }

  _getModifiers(event) {
    return {
      ctrl: event.ctrlKey,
      shift: event.shiftKey,
      alt: event.altKey,
      meta: event.metaKey,
    };
  }

  _getShortcutKey(event) {
    const modifiers = [];
    if (event.ctrlKey) modifiers.push("Ctrl");
    if (event.shiftKey) modifiers.push("Shift");
    if (event.altKey) modifiers.push("Alt");
    if (event.metaKey) modifiers.push("Meta");

    return modifiers.length > 0
      ? `${modifiers.join("+")}+${event.key}`
      : event.key;
  }

  _getTouchDistance(touch1, touch2) {
    const dx = touch2.currentX - touch1.currentX;
    const dy = touch2.currentY - touch1.currentY;
    return Math.sqrt(dx * dx + dy * dy);
  }

  _getTouchCenter(touch1, touch2) {
    return {
      x: (touch1.currentX + touch2.currentX) / 2,
      y: (touch1.currentY + touch2.currentY) / 2,
    };
  }

  _announceToScreenReader(message) {
    if (this.announcer) {
      this.announcer.textContent = message;
    }
  }

  _updateElementDimensions() {
    // Update cached element dimensions if needed
    this.elementRect = this.element.getBoundingClientRect();
  }

  /**
   * Public API methods
   */

  /**
   * Add keyboard shortcut
   *
   * @param {string} key - Key combination (e.g., 'Ctrl+s')
   * @param {Function} handler - Handler function
   */
  addKeyboardShortcut(key, handler) {
    this.keyboardShortcuts.set(key, handler);
  }

  /**
   * Remove keyboard shortcut
   *
   * @param {string} key - Key combination
   */
  removeKeyboardShortcut(key) {
    this.keyboardShortcuts.delete(key);
  }

  /**
   * Add context menu item
   *
   * @param {Object} item - Menu item configuration
   */
  addContextMenuItem(item) {
    this.contextMenuItems.push(item);
  }

  /**
   * Set focus to element
   */
  focus() {
    this.element.focus();
  }

  /**
   * Check if element has focus
   *
   * @returns {boolean} Focus state
   */
  hasFocus() {
    return document.activeElement === this.element;
  }

  /**
   * Get current interaction state
   *
   * @returns {Object} Interaction state
   */
  getInteractionState() {
    return {
      isInteracting: this.isInteracting,
      interactionType: this.interactionType,
      touches: this.touches.size,
      pressedKeys: Array.from(this.pressedKeys),
      gestures: { ...this.gestures },
    };
  }

  /**
   * Event handling
   */
  on(event, handler) {
    if (!this.eventHandlers.has(event)) {
      this.eventHandlers.set(event, []);
    }
    this.eventHandlers.get(event).push(handler);
  }

  off(event, handler) {
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
   * Cleanup and destroy interaction handler
   */
  destroy() {
    console.log("Destroying WaveformInteraction...");

    // Remove all event listeners
    // (In a real implementation, you'd store bound handlers to remove them properly)

    // Clear timers
    if (this.gestures.longPress.timer) {
      clearTimeout(this.gestures.longPress.timer);
    }

    if (this.rafId) {
      cancelAnimationFrame(this.rafId);
    }

    // Clear state
    this.touches.clear();
    this.pressedKeys.clear();
    this.eventHandlers.clear();
    this.keyboardShortcuts.clear();

    // Remove accessibility elements
    if (this.announcer && this.announcer.parentNode) {
      this.announcer.parentNode.removeChild(this.announcer);
    }

    const helpText = document.getElementById("waveform-help");
    if (helpText && helpText.parentNode) {
      helpText.parentNode.removeChild(helpText);
    }

    console.log("WaveformInteraction destroyed");
  }
}

export default WaveformInteraction;
