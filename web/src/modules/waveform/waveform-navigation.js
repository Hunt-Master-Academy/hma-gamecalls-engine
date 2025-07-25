/**
 * @fileoverview Waveform Navigation Module - Interactive Navigation and Controls
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Provides comprehensive interactive navigation and control capabilities for
 * waveform visualization including zoom, pan, selection, and playback control.
 *
 * Key Features:
 * - Multi-touch gesture support for zoom and pan operations
 * - Precise selection tools with snap-to-grid functionality
 * - Keyboard shortcuts for efficient navigation
 * - Playback position tracking and visualization
 * - Bookmark and region management
 * - Accessibility support with screen reader compatibility
 *
 * Dependencies:
 * - Touch event handling for mobile support
 * - Keyboard event management
 * - Animation frame scheduling
 * - Accessibility utilities
 *
 * @example
 * ```javascript
 * import { WaveformNavigation } from './modules/waveform/waveform-navigation.js';
 *
 * const navigation = new WaveformNavigation({
 *   canvas: canvasElement,
 *   enableTouch: true,
 *   snapToGrid: true
 * });
 *
 * navigation.setZoomRange(0.1, 10);
 * navigation.enableSelection(true);
 * ```
 */

/**
 * Interactive Waveform Navigation Controller
 *
 * Manages all user interactions with waveform visualizations including
 * navigation, selection, and playback control with accessibility support.
 *
 * @class WaveformNavigation
 */
export class WaveformNavigation {
  /**
   * Create a WaveformNavigation instance
   *
   * @param {Object} options - Configuration options
   * @param {HTMLCanvasElement} options.canvas - Target canvas element
   * @param {boolean} [options.enableTouch=true] - Enable touch gestures
   * @param {boolean} [options.enableKeyboard=true] - Enable keyboard shortcuts
   * @param {boolean} [options.snapToGrid=false] - Enable snap-to-grid
   * @param {number} [options.gridResolution=100] - Grid resolution in pixels
   * @param {Object} [options.zoomLimits] - Zoom limits configuration
   * @param {Object} [options.accessibility] - Accessibility options
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      enableTouch: options.enableTouch !== false,
      enableKeyboard: options.enableKeyboard !== false,
      snapToGrid: options.snapToGrid || false,
      gridResolution: options.gridResolution || 100,
      zoomLimits: {
        min: 0.1,
        max: 10,
        step: 0.1,
        ...options.zoomLimits,
      },
      accessibility: {
        announceChanges: true,
        enableFocusIndicator: true,
        keyboardNavigation: true,
        ...options.accessibility,
      },
      ...options,
    };

    // Canvas reference
    this.canvas = options.canvas;
    if (!this.canvas) {
      throw new Error("Canvas element is required");
    }

    // Navigation state
    this.state = {
      zoom: 1.0,
      pan: { x: 0, y: 0 },
      isNavigating: false,
      isDragging: false,
      isSelecting: false,
      lastInteractionTime: 0,
      activeGesture: null,
      focusedElement: null,
    };

    // Selection management
    this.selection = {
      active: false,
      start: null,
      end: null,
      regions: [],
      snapToGrid: this.config.snapToGrid,
      selectionMode: "range", // 'range', 'point', 'multi'
    };

    // Playback tracking
    this.playback = {
      position: 0,
      duration: 0,
      isPlaying: false,
      markers: [],
      bookmarks: [],
    };

    // Gesture handling
    this.gestures = {
      touches: [],
      lastTouchDistance: 0,
      lastTouchCenter: { x: 0, y: 0 },
      gestureStartZoom: 1.0,
      gestureStartPan: { x: 0, y: 0 },
    };

    // Keyboard shortcuts
    this.keyBindings = {
      // Navigation
      ArrowLeft: { action: "panLeft", shift: "selectLeft" },
      ArrowRight: { action: "panRight", shift: "selectRight" },
      ArrowUp: { action: "zoomIn" },
      ArrowDown: { action: "zoomOut" },

      // Selection
      Space: { action: "togglePlayback" },
      Enter: { action: "confirmSelection" },
      Escape: { action: "clearSelection" },
      KeyA: { ctrl: true, action: "selectAll" },

      // Zoom
      Equal: { ctrl: true, action: "zoomIn" },
      Minus: { ctrl: true, action: "zoomOut" },
      Digit0: { ctrl: true, action: "resetZoom" },

      // Bookmarks
      KeyB: { ctrl: true, action: "addBookmark" },
      KeyG: { ctrl: true, action: "goToBookmark" },
    };

    // Event handlers
    this.eventHandlers = new Map();

    // Animation frame
    this.animationFrame = null;

    this._initialize();
  }

  /**
   * Initialize navigation system
   * @private
   */
  _initialize() {
    try {
      // Setup canvas properties
      this._setupCanvas();

      // Initialize event listeners
      this._setupEventListeners();

      // Initialize accessibility features
      this._setupAccessibility();

      // Initialize gesture recognition
      this._setupGestureRecognition();

      console.log("WaveformNavigation initialized successfully");
    } catch (error) {
      console.error("WaveformNavigation initialization failed:", error);
    }
  }

  /**
   * Setup canvas properties for interaction
   * @private
   */
  _setupCanvas() {
    // Make canvas focusable
    this.canvas.tabIndex = 0;

    // Set ARIA attributes
    this.canvas.setAttribute("role", "application");
    this.canvas.setAttribute(
      "aria-label",
      "Waveform visualization with interactive navigation"
    );

    // Prevent context menu on right-click
    this.canvas.addEventListener("contextmenu", (e) => e.preventDefault());
  }

  /**
   * Setup event listeners
   * @private
   */
  _setupEventListeners() {
    // Mouse events
    this.canvas.addEventListener("mousedown", this._handleMouseDown.bind(this));
    this.canvas.addEventListener("mousemove", this._handleMouseMove.bind(this));
    this.canvas.addEventListener("mouseup", this._handleMouseUp.bind(this));
    this.canvas.addEventListener("wheel", this._handleWheel.bind(this));
    this.canvas.addEventListener(
      "dblclick",
      this._handleDoubleClick.bind(this)
    );

    // Touch events (if enabled)
    if (this.config.enableTouch) {
      this.canvas.addEventListener(
        "touchstart",
        this._handleTouchStart.bind(this),
        { passive: false }
      );
      this.canvas.addEventListener(
        "touchmove",
        this._handleTouchMove.bind(this),
        { passive: false }
      );
      this.canvas.addEventListener(
        "touchend",
        this._handleTouchEnd.bind(this),
        { passive: false }
      );
      this.canvas.addEventListener(
        "touchcancel",
        this._handleTouchCancel.bind(this)
      );
    }

    // Keyboard events (if enabled)
    if (this.config.enableKeyboard) {
      this.canvas.addEventListener("keydown", this._handleKeyDown.bind(this));
      this.canvas.addEventListener("keyup", this._handleKeyUp.bind(this));
      this.canvas.addEventListener("focus", this._handleFocus.bind(this));
      this.canvas.addEventListener("blur", this._handleBlur.bind(this));
    }

    // Window events
    window.addEventListener("resize", this._handleResize.bind(this));
  }

  /**
   * Setup accessibility features
   * @private
   */
  _setupAccessibility() {
    if (!this.config.accessibility.enableFocusIndicator) {
      return;
    }

    // Create focus indicator
    this.focusIndicator = document.createElement("div");
    this.focusIndicator.style.cssText = `
      position: absolute;
      pointer-events: none;
      border: 2px solid #4a9eff;
      border-radius: 4px;
      background: rgba(74, 158, 255, 0.1);
      display: none;
      z-index: 1000;
    `;

    // Insert focus indicator relative to canvas
    this.canvas.parentNode.insertBefore(
      this.focusIndicator,
      this.canvas.nextSibling
    );
  }

  /**
   * Setup gesture recognition
   * @private
   */
  _setupGestureRecognition() {
    this.gestureRecognizer = {
      // Recognize pinch-to-zoom
      recognizePinch: (touches) => {
        if (touches.length !== 2) return null;

        const touch1 = touches[0];
        const touch2 = touches[1];

        const distance = Math.sqrt(
          Math.pow(touch2.clientX - touch1.clientX, 2) +
            Math.pow(touch2.clientY - touch1.clientY, 2)
        );

        const center = {
          x: (touch1.clientX + touch2.clientX) / 2,
          y: (touch1.clientY + touch2.clientY) / 2,
        };

        return { type: "pinch", distance, center };
      },

      // Recognize pan gesture
      recognizePan: (touches) => {
        if (touches.length !== 1) return null;

        return {
          type: "pan",
          position: { x: touches[0].clientX, y: touches[0].clientY },
        };
      },

      // Recognize tap gesture
      recognizeTap: (touch, startTime) => {
        const duration = Date.now() - startTime;
        const maxTapDuration = 300;

        if (duration <= maxTapDuration) {
          return {
            type: "tap",
            position: { x: touch.clientX, y: touch.clientY },
          };
        }

        return null;
      },
    };
  }

  /**
   * Handle mouse down events
   * @private
   */
  _handleMouseDown(event) {
    event.preventDefault();
    this.canvas.focus();

    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    this.state.isDragging = true;
    this.state.lastInteractionTime = Date.now();

    // Handle different mouse buttons
    switch (event.button) {
      case 0: // Left button - selection or pan
        if (event.shiftKey) {
          this._startSelection(x, y);
        } else {
          this._startPan(x, y);
        }
        break;
      case 1: // Middle button - pan
        this._startPan(x, y);
        break;
      case 2: // Right button - context menu
        this._showContextMenu(x, y);
        break;
    }

    this._emitEvent("mouseDown", {
      x,
      y,
      button: event.button,
      modifiers: this._getModifiers(event),
    });
  }

  /**
   * Handle mouse move events
   * @private
   */
  _handleMouseMove(event) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    if (this.state.isDragging) {
      if (this.state.isSelecting) {
        this._updateSelection(x, y);
      } else {
        this._updatePan(x, y);
      }
    }

    // Update cursor based on context
    this._updateCursor(x, y, event);

    this._emitEvent("mouseMove", { x, y, isDragging: this.state.isDragging });
  }

  /**
   * Handle mouse up events
   * @private
   */
  _handleMouseUp(event) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    if (this.state.isSelecting) {
      this._endSelection(x, y);
    } else if (this.state.isDragging) {
      this._endPan();
    }

    this.state.isDragging = false;
    this.state.isSelecting = false;

    this._emitEvent("mouseUp", { x, y });
  }

  /**
   * Handle wheel events for zoom
   * @private
   */
  _handleWheel(event) {
    event.preventDefault();

    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    const zoomFactor = event.deltaY > 0 ? 0.9 : 1.1;
    this._zoomAtPoint(x, y, zoomFactor);

    this._announceChange(`Zoom level: ${Math.round(this.state.zoom * 100)}%`);
    this._emitEvent("zoom", { zoom: this.state.zoom, center: { x, y } });
  }

  /**
   * Handle double-click events
   * @private
   */
  _handleDoubleClick(event) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    // Reset zoom on double-click
    this.resetZoom();

    this._announceChange("Zoom reset to 100%");
    this._emitEvent("doubleClick", { x, y });
  }

  /**
   * Handle touch start events
   * @private
   */
  _handleTouchStart(event) {
    event.preventDefault();

    this.gestures.touches = Array.from(event.touches);
    const gesture = this._recognizeGesture();

    if (gesture) {
      this.state.activeGesture = gesture;
      this.state.lastInteractionTime = Date.now();

      switch (gesture.type) {
        case "pinch":
          this.gestures.lastTouchDistance = gesture.distance;
          this.gestures.lastTouchCenter = gesture.center;
          this.gestures.gestureStartZoom = this.state.zoom;
          break;
        case "pan":
          this.gestures.lastTouchCenter = gesture.position;
          this.gestures.gestureStartPan = { ...this.state.pan };
          break;
      }
    }

    this._emitEvent("touchStart", { touches: this.gestures.touches, gesture });
  }

  /**
   * Handle touch move events
   * @private
   */
  _handleTouchMove(event) {
    event.preventDefault();

    this.gestures.touches = Array.from(event.touches);
    const gesture = this._recognizeGesture();

    if (gesture && this.state.activeGesture) {
      switch (gesture.type) {
        case "pinch":
          this._handlePinchGesture(gesture);
          break;
        case "pan":
          this._handlePanGesture(gesture);
          break;
      }
    }

    this._emitEvent("touchMove", { touches: this.gestures.touches, gesture });
  }

  /**
   * Handle touch end events
   * @private
   */
  _handleTouchEnd(event) {
    event.preventDefault();

    // Check for tap gesture
    if (
      this.gestures.touches.length === 1 &&
      this.state.activeGesture?.type === "pan"
    ) {
      const tapGesture = this.gestureRecognizer.recognizeTap(
        this.gestures.touches[0],
        this.state.lastInteractionTime
      );

      if (tapGesture) {
        this._handleTapGesture(tapGesture);
      }
    }

    this.gestures.touches = Array.from(event.touches);
    this.state.activeGesture = null;

    this._emitEvent("touchEnd", { touches: this.gestures.touches });
  }

  /**
   * Handle pinch gesture
   * @private
   */
  _handlePinchGesture(gesture) {
    const scaleFactor = gesture.distance / this.gestures.lastTouchDistance;
    const newZoom = this.gestures.gestureStartZoom * scaleFactor;

    // Apply zoom limits
    const clampedZoom = Math.max(
      this.config.zoomLimits.min,
      Math.min(this.config.zoomLimits.max, newZoom)
    );

    this.setZoom(clampedZoom);

    // Update pan to keep gesture center stable
    const rect = this.canvas.getBoundingClientRect();
    const centerX = gesture.center.x - rect.left;
    const centerY = gesture.center.y - rect.top;

    this._adjustPanForZoom(
      centerX,
      centerY,
      this.state.zoom / this.gestures.gestureStartZoom
    );
  }

  /**
   * Handle pan gesture
   * @private
   */
  _handlePanGesture(gesture) {
    const rect = this.canvas.getBoundingClientRect();
    const currentX = gesture.position.x - rect.left;
    const currentY = gesture.position.y - rect.top;

    const deltaX = currentX - (this.gestures.lastTouchCenter.x - rect.left);
    const deltaY = currentY - (this.gestures.lastTouchCenter.y - rect.top);

    this.setPan(
      this.gestures.gestureStartPan.x + deltaX,
      this.gestures.gestureStartPan.y + deltaY
    );
  }

  /**
   * Handle tap gesture
   * @private
   */
  _handleTapGesture(gesture) {
    const rect = this.canvas.getBoundingClientRect();
    const x = gesture.position.x - rect.left;
    const y = gesture.position.y - rect.top;

    // Set playback position at tap location
    const relativeX = x / this.canvas.width;
    this.setPlaybackPosition(relativeX * this.playback.duration);

    this._emitEvent("tap", { x, y });
  }

  /**
   * Handle keyboard events
   * @private
   */
  _handleKeyDown(event) {
    const key = event.code;
    const binding = this.keyBindings[key];

    if (!binding) return;

    event.preventDefault();

    // Determine which action to execute based on modifiers
    let action = binding.action;
    if (event.shiftKey && binding.shift) {
      action = binding.shift;
    } else if (event.ctrlKey && binding.ctrl) {
      action = binding.ctrl;
    } else if (event.altKey && binding.alt) {
      action = binding.alt;
    }

    this._executeAction(action, event);
  }

  /**
   * Execute navigation action
   * @private
   */
  _executeAction(action, event) {
    const step = this.config.zoomLimits.step;
    const panStep = 50;

    switch (action) {
      case "panLeft":
        this.setPan(this.state.pan.x + panStep, this.state.pan.y);
        this._announceChange("Panned left");
        break;
      case "panRight":
        this.setPan(this.state.pan.x - panStep, this.state.pan.y);
        this._announceChange("Panned right");
        break;
      case "zoomIn":
        this.setZoom(this.state.zoom + step);
        this._announceChange(
          `Zoomed in to ${Math.round(this.state.zoom * 100)}%`
        );
        break;
      case "zoomOut":
        this.setZoom(this.state.zoom - step);
        this._announceChange(
          `Zoomed out to ${Math.round(this.state.zoom * 100)}%`
        );
        break;
      case "resetZoom":
        this.resetZoom();
        this._announceChange("Zoom reset to 100%");
        break;
      case "togglePlayback":
        this.togglePlayback();
        break;
      case "selectAll":
        this.selectAll();
        this._announceChange("Selected all");
        break;
      case "clearSelection":
        this.clearSelection();
        this._announceChange("Selection cleared");
        break;
      case "addBookmark":
        this.addBookmark();
        this._announceChange("Bookmark added");
        break;
    }

    this._emitEvent("keyAction", { action, event });
  }

  /**
   * Set zoom level
   *
   * @param {number} zoom - Zoom level
   */
  setZoom(zoom) {
    const oldZoom = this.state.zoom;
    this.state.zoom = Math.max(
      this.config.zoomLimits.min,
      Math.min(this.config.zoomLimits.max, zoom)
    );

    if (this.state.zoom !== oldZoom) {
      this._emitEvent("zoomChanged", { zoom: this.state.zoom, oldZoom });
    }
  }

  /**
   * Set pan offset
   *
   * @param {number} x - X offset
   * @param {number} y - Y offset
   */
  setPan(x, y) {
    this.state.pan.x = x;
    this.state.pan.y = y;

    this._emitEvent("panChanged", { pan: this.state.pan });
  }

  /**
   * Reset zoom to default
   */
  resetZoom() {
    this.setZoom(1.0);
    this.setPan(0, 0);
  }

  /**
   * Zoom at specific point
   * @private
   */
  _zoomAtPoint(x, y, factor) {
    const oldZoom = this.state.zoom;
    this.setZoom(this.state.zoom * factor);

    // Adjust pan to keep zoom center stable
    const zoomRatio = this.state.zoom / oldZoom;
    this._adjustPanForZoom(x, y, zoomRatio);
  }

  /**
   * Adjust pan for zoom operation
   * @private
   */
  _adjustPanForZoom(centerX, centerY, zoomRatio) {
    const newPanX = this.state.pan.x - centerX * (zoomRatio - 1);
    const newPanY = this.state.pan.y - centerY * (zoomRatio - 1);

    this.setPan(newPanX, newPanY);
  }

  /**
   * Start selection
   * @private
   */
  _startSelection(x, y) {
    this.state.isSelecting = true;
    this.selection.start = this._snapToGrid(x);
    this.selection.end = this.selection.start;
    this.selection.active = true;

    this._emitEvent("selectionStart", { start: this.selection.start });
  }

  /**
   * Update selection
   * @private
   */
  _updateSelection(x, y) {
    if (!this.state.isSelecting) return;

    this.selection.end = this._snapToGrid(x);

    this._emitEvent("selectionUpdate", {
      start: this.selection.start,
      end: this.selection.end,
    });
  }

  /**
   * End selection
   * @private
   */
  _endSelection(x, y) {
    if (!this.state.isSelecting) return;

    this.selection.end = this._snapToGrid(x);

    // Ensure start is less than end
    if (this.selection.start > this.selection.end) {
      [this.selection.start, this.selection.end] = [
        this.selection.end,
        this.selection.start,
      ];
    }

    const selectionWidth = Math.abs(this.selection.end - this.selection.start);
    const minSelectionWidth = 5; // Minimum selection width in pixels

    if (selectionWidth < minSelectionWidth) {
      this.clearSelection();
    } else {
      this._emitEvent("selectionEnd", {
        start: this.selection.start,
        end: this.selection.end,
        width: selectionWidth,
      });
    }

    this.state.isSelecting = false;
  }

  /**
   * Snap coordinate to grid
   * @private
   */
  _snapToGrid(x) {
    if (!this.config.snapToGrid) {
      return x;
    }

    const gridSize = this.config.gridResolution;
    return Math.round(x / gridSize) * gridSize;
  }

  /**
   * Select all content
   */
  selectAll() {
    this.selection.active = true;
    this.selection.start = 0;
    this.selection.end = this.canvas.width;

    this._emitEvent("selectionChanged", {
      start: this.selection.start,
      end: this.selection.end,
    });
  }

  /**
   * Clear selection
   */
  clearSelection() {
    this.selection.active = false;
    this.selection.start = null;
    this.selection.end = null;

    this._emitEvent("selectionCleared");
  }

  /**
   * Set playback position
   *
   * @param {number} position - Playback position in seconds
   */
  setPlaybackPosition(position) {
    this.playback.position = Math.max(
      0,
      Math.min(this.playback.duration, position)
    );

    this._emitEvent("playbackPositionChanged", {
      position: this.playback.position,
    });
  }

  /**
   * Toggle playback
   */
  togglePlayback() {
    this.playback.isPlaying = !this.playback.isPlaying;

    this._emitEvent("playbackToggled", { isPlaying: this.playback.isPlaying });
  }

  /**
   * Add bookmark at current position
   */
  addBookmark() {
    const bookmark = {
      id: Date.now(),
      position: this.playback.position,
      name: `Bookmark ${this.playback.bookmarks.length + 1}`,
      timestamp: new Date().toISOString(),
    };

    this.playback.bookmarks.push(bookmark);
    this.playback.bookmarks.sort((a, b) => a.position - b.position);

    this._emitEvent("bookmarkAdded", { bookmark });
  }

  /**
   * Recognize current gesture
   * @private
   */
  _recognizeGesture() {
    const touches = this.gestures.touches;

    if (touches.length === 2) {
      return this.gestureRecognizer.recognizePinch(touches);
    } else if (touches.length === 1) {
      return this.gestureRecognizer.recognizePan(touches);
    }

    return null;
  }

  /**
   * Get keyboard modifiers
   * @private
   */
  _getModifiers(event) {
    return {
      shift: event.shiftKey,
      ctrl: event.ctrlKey,
      alt: event.altKey,
      meta: event.metaKey,
    };
  }

  /**
   * Update cursor based on context
   * @private
   */
  _updateCursor(x, y, event) {
    let cursor = "default";

    if (event.shiftKey) {
      cursor = "crosshair";
    } else if (this.state.isDragging) {
      cursor = "grabbing";
    } else {
      cursor = "grab";
    }

    this.canvas.style.cursor = cursor;
  }

  /**
   * Announce change for accessibility
   * @private
   */
  _announceChange(message) {
    if (!this.config.accessibility.announceChanges) return;

    // Use ARIA live region for announcements
    if (!this.ariaLiveRegion) {
      this.ariaLiveRegion = document.createElement("div");
      this.ariaLiveRegion.setAttribute("aria-live", "polite");
      this.ariaLiveRegion.setAttribute("aria-atomic", "true");
      this.ariaLiveRegion.style.cssText = `
        position: absolute;
        left: -10000px;
        width: 1px;
        height: 1px;
        overflow: hidden;
      `;
      document.body.appendChild(this.ariaLiveRegion);
    }

    this.ariaLiveRegion.textContent = message;
  }

  /**
   * Get current navigation state
   *
   * @returns {Object} Navigation state
   */
  getState() {
    return {
      zoom: this.state.zoom,
      pan: { ...this.state.pan },
      selection: this.selection.active
        ? {
            start: this.selection.start,
            end: this.selection.end,
          }
        : null,
      playback: { ...this.playback },
    };
  }

  /**
   * Set navigation state
   *
   * @param {Object} state - Navigation state to restore
   */
  setState(state) {
    if (state.zoom !== undefined) {
      this.setZoom(state.zoom);
    }

    if (state.pan) {
      this.setPan(state.pan.x, state.pan.y);
    }

    if (state.selection) {
      this.selection.active = true;
      this.selection.start = state.selection.start;
      this.selection.end = state.selection.end;
    } else {
      this.clearSelection();
    }

    if (state.playback) {
      Object.assign(this.playback, state.playback);
    }
  }

  /**
   * Setup event handling
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
  _emitEvent(eventName, data) {
    if (this.eventHandlers.has(eventName)) {
      this.eventHandlers.get(eventName).forEach((handler) => {
        try {
          handler(data);
        } catch (error) {
          console.error(`Event handler error for ${eventName}:`, error);
        }
      });
    }
  }

  /**
   * Cleanup resources
   */
  destroy() {
    // Remove event listeners
    this.eventHandlers.clear();

    // Remove accessibility elements
    if (this.ariaLiveRegion) {
      document.body.removeChild(this.ariaLiveRegion);
    }

    if (this.focusIndicator) {
      this.focusIndicator.parentNode.removeChild(this.focusIndicator);
    }

    // Cancel animation frame if active
    if (this.animationFrame) {
      cancelAnimationFrame(this.animationFrame);
    }
  }
}

export default WaveformNavigation;
