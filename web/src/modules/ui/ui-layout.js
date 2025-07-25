/**
 * @file ui-layout.js
 * @brief Responsive layout management system for adaptive UI design
 *
 * This module provides a comprehensive layout management system that handles:
 * - Responsive breakpoints and media queries
 * - Container sizing and positioning
 * - Grid and flexbox layout utilities
 * - Viewport management and orientation detection
 * - Dynamic layout adaptation
 */

/**
 * Layout breakpoint definitions following common responsive design patterns
 */
const BREAKPOINTS = {
  xs: 0, // Extra small devices (portrait phones)
  sm: 576, // Small devices (landscape phones)
  md: 768, // Medium devices (tablets)
  lg: 992, // Large devices (desktops)
  xl: 1200, // Extra large devices (large desktops)
  xxl: 1400, // Extra extra large devices (ultra-wide)
};

/**
 * Layout orientation constants
 */
const ORIENTATIONS = {
  PORTRAIT: "portrait",
  LANDSCAPE: "landscape",
};

/**
 * Layout container types
 */
const CONTAINER_TYPES = {
  FLUID: "fluid",
  FIXED: "fixed",
  RESPONSIVE: "responsive",
};

/**
 * Layout manager that handles responsive design and adaptive layouts
 */
export class UILayout {
  constructor(options = {}) {
    this.options = {
      // Breakpoint configuration
      breakpoints: { ...BREAKPOINTS, ...options.breakpoints },

      // Container options
      containerMaxWidths: {
        sm: 540,
        md: 720,
        lg: 960,
        xl: 1140,
        xxl: 1320,
        ...options.containerMaxWidths,
      },

      // Grid system options
      gridColumns: options.gridColumns || 12,
      gridGutter: options.gridGutter || 16,

      // Layout adaptation options
      adaptiveMode: options.adaptiveMode || "responsive", // 'responsive' | 'adaptive' | 'static'
      orientationHandling: options.orientationHandling !== false,

      // Performance options
      throttleDelay: options.throttleDelay || 16, // ~60fps
      enableLayoutCaching: options.enableLayoutCaching !== false,

      // Event handling
      autoInitialize: options.autoInitialize !== false,
      eventManager: options.eventManager || null,

      // Debug options
      debugMode: options.debugMode || false,
      showBreakpointIndicator: options.showBreakpointIndicator || false,

      ...options,
    };

    // Internal state
    this.currentBreakpoint = null;
    this.currentOrientation = null;
    this.viewportDimensions = { width: 0, height: 0 };
    this.containers = new Map();
    this.mediaQueries = new Map();
    this.layoutCache = new Map();
    this.observers = new Set();

    // Performance tracking
    this.metrics = {
      layoutUpdates: 0,
      resizeEvents: 0,
      breakpointChanges: 0,
      orientationChanges: 0,
      lastUpdateTime: 0,
      averageUpdateTime: 0,
    };

    // Throttled functions
    this.throttledResize = this._throttle(
      this._handleResize.bind(this),
      this.options.throttleDelay
    );
    this.throttledOrientationChange = this._throttle(
      this._handleOrientationChange.bind(this),
      this.options.throttleDelay
    );

    if (this.options.autoInitialize) {
      this.initialize();
    }
  }

  /**
   * Initialize the layout system
   */
  async initialize() {
    try {
      // Set up initial viewport and breakpoint detection
      this._updateViewportDimensions();
      this._detectBreakpoint();
      this._detectOrientation();

      // Create media query listeners
      this._setupMediaQueries();

      // Set up event listeners
      this._setupEventListeners();

      // Initialize debug features if enabled
      if (this.options.debugMode) {
        this._setupDebugFeatures();
      }

      // Cache initial layout state
      if (this.options.enableLayoutCaching) {
        this._cacheLayoutState();
      }

      // Emit initialization event
      this._emitEvent("layoutInitialized", {
        breakpoint: this.currentBreakpoint,
        orientation: this.currentOrientation,
        viewport: this.viewportDimensions,
      });

      return true;
    } catch (error) {
      console.error("UILayout initialization failed:", error);
      return false;
    }
  }

  /**
   * Create a responsive container with automatic sizing
   */
  createContainer(element, options = {}) {
    if (!element) {
      throw new Error("Container element is required");
    }

    const containerConfig = {
      type: options.type || CONTAINER_TYPES.RESPONSIVE,
      maxWidths: { ...this.options.containerMaxWidths, ...options.maxWidths },
      padding: options.padding || this.options.gridGutter,
      centerContent: options.centerContent !== false,
      element: element,
      id: options.id || this._generateId("container"),
      ...options,
    };

    // Apply initial container styling
    this._applyContainerStyles(element, containerConfig);

    // Register container for updates
    this.containers.set(containerConfig.id, containerConfig);

    // Update container sizing
    this._updateContainer(containerConfig);

    return containerConfig.id;
  }

  /**
   * Create a responsive grid system
   */
  createGrid(element, options = {}) {
    if (!element) {
      throw new Error("Grid element is required");
    }

    const gridConfig = {
      columns: options.columns || this.options.gridColumns,
      gutter: options.gutter || this.options.gridGutter,
      rowGap: options.rowGap || options.gutter || this.options.gridGutter,
      responsive: options.responsive !== false,
      autoFit: options.autoFit || false,
      minColumnWidth: options.minColumnWidth || 200,
      element: element,
      id: options.id || this._generateId("grid"),
      ...options,
    };

    // Apply grid styles
    this._applyGridStyles(element, gridConfig);

    return gridConfig.id;
  }

  /**
   * Register a breakpoint change callback
   */
  onBreakpointChange(callback) {
    if (typeof callback !== "function") {
      throw new Error("Callback must be a function");
    }

    const observer = {
      type: "breakpoint",
      callback: callback,
      id: this._generateId("observer"),
    };

    this.observers.add(observer);
    return observer.id;
  }

  /**
   * Register an orientation change callback
   */
  onOrientationChange(callback) {
    if (typeof callback !== "function") {
      throw new Error("Callback must be a function");
    }

    const observer = {
      type: "orientation",
      callback: callback,
      id: this._generateId("observer"),
    };

    this.observers.add(observer);
    return observer.id;
  }

  /**
   * Register a viewport change callback
   */
  onViewportChange(callback) {
    if (typeof callback !== "function") {
      throw new Error("Callback must be a function");
    }

    const observer = {
      type: "viewport",
      callback: callback,
      id: this._generateId("observer"),
    };

    this.observers.add(observer);
    return observer.id;
  }

  /**
   * Remove an observer by ID
   */
  removeObserver(observerId) {
    for (const observer of this.observers) {
      if (observer.id === observerId) {
        this.observers.delete(observer);
        return true;
      }
    }
    return false;
  }

  /**
   * Get current layout state
   */
  getLayoutState() {
    return {
      breakpoint: this.currentBreakpoint,
      orientation: this.currentOrientation,
      viewport: { ...this.viewportDimensions },
      containerCount: this.containers.size,
      metrics: { ...this.metrics },
    };
  }

  /**
   * Check if current viewport matches a breakpoint condition
   */
  matchesBreakpoint(breakpoint) {
    const currentWidth = this.viewportDimensions.width;

    if (typeof breakpoint === "string") {
      const bp = this.options.breakpoints[breakpoint];
      if (bp === undefined) {
        throw new Error(`Unknown breakpoint: ${breakpoint}`);
      }
      return currentWidth >= bp;
    }

    if (typeof breakpoint === "object") {
      const { min, max } = breakpoint;
      const minWidth =
        min !== undefined ? this.options.breakpoints[min] || min : 0;
      const maxWidth =
        max !== undefined ? this.options.breakpoints[max] || max : Infinity;
      return currentWidth >= minWidth && currentWidth < maxWidth;
    }

    return false;
  }

  /**
   * Get CSS media query string for a breakpoint
   */
  getMediaQuery(breakpoint) {
    if (this.mediaQueries.has(breakpoint)) {
      return this.mediaQueries.get(breakpoint).media;
    }

    const bp = this.options.breakpoints[breakpoint];
    if (bp === undefined) {
      throw new Error(`Unknown breakpoint: ${breakpoint}`);
    }

    return `(min-width: ${bp}px)`;
  }

  /**
   * Force layout update (useful after dynamic content changes)
   */
  updateLayout() {
    const startTime = performance.now();

    try {
      // Update viewport dimensions
      this._updateViewportDimensions();

      // Check for breakpoint changes
      const oldBreakpoint = this.currentBreakpoint;
      this._detectBreakpoint();

      // Check for orientation changes
      const oldOrientation = this.currentOrientation;
      this._detectOrientation();

      // Update all containers
      for (const [id, container] of this.containers) {
        this._updateContainer(container);
      }

      // Emit change events if needed
      if (oldBreakpoint !== this.currentBreakpoint) {
        this._notifyBreakpointChange(oldBreakpoint, this.currentBreakpoint);
      }

      if (oldOrientation !== this.currentOrientation) {
        this._notifyOrientationChange(oldOrientation, this.currentOrientation);
      }

      // Update performance metrics
      const updateTime = performance.now() - startTime;
      this.metrics.layoutUpdates++;
      this.metrics.lastUpdateTime = updateTime;
      this.metrics.averageUpdateTime =
        (this.metrics.averageUpdateTime + updateTime) / 2;

      // Cache updated state
      if (this.options.enableLayoutCaching) {
        this._cacheLayoutState();
      }

      // Emit layout update event
      this._emitEvent("layoutUpdated", this.getLayoutState());
    } catch (error) {
      console.error("Layout update failed:", error);
    }
  }

  /**
   * Clean up resources and event listeners
   */
  destroy() {
    try {
      // Remove event listeners
      window.removeEventListener("resize", this.throttledResize);
      window.removeEventListener(
        "orientationchange",
        this.throttledOrientationChange
      );

      // Clean up media query listeners
      for (const [breakpoint, mediaQuery] of this.mediaQueries) {
        if (mediaQuery.removeListener) {
          mediaQuery.removeListener(mediaQuery.handler);
        }
      }

      // Clear containers and observers
      this.containers.clear();
      this.observers.clear();
      this.mediaQueries.clear();
      this.layoutCache.clear();

      // Remove debug features
      if (this.options.debugMode) {
        this._removeDebugFeatures();
      }

      // Emit cleanup event
      this._emitEvent("layoutDestroyed");
    } catch (error) {
      console.error("Layout cleanup failed:", error);
    }
  }

  // Private methods

  /**
   * Update viewport dimensions
   */
  _updateViewportDimensions() {
    this.viewportDimensions = {
      width: window.innerWidth,
      height: window.innerHeight,
    };
  }

  /**
   * Detect current breakpoint based on viewport width
   */
  _detectBreakpoint() {
    const width = this.viewportDimensions.width;
    const breakpoints = Object.entries(this.options.breakpoints).sort(
      ([, a], [, b]) => b - a
    ); // Sort by value, descending

    for (const [name, minWidth] of breakpoints) {
      if (width >= minWidth) {
        this.currentBreakpoint = name;
        return;
      }
    }

    this.currentBreakpoint = "xs"; // Fallback
  }

  /**
   * Detect current orientation
   */
  _detectOrientation() {
    const { width, height } = this.viewportDimensions;
    this.currentOrientation =
      width > height ? ORIENTATIONS.LANDSCAPE : ORIENTATIONS.PORTRAIT;
  }

  /**
   * Set up media query listeners
   */
  _setupMediaQueries() {
    for (const [name, minWidth] of Object.entries(this.options.breakpoints)) {
      if (minWidth > 0) {
        // Skip 'xs' breakpoint
        const mediaQuery = window.matchMedia(`(min-width: ${minWidth}px)`);
        const handler = () => this.updateLayout();

        if (mediaQuery.addListener) {
          mediaQuery.addListener(handler);
        } else {
          mediaQuery.addEventListener("change", handler);
        }

        this.mediaQueries.set(name, {
          mediaQuery,
          handler,
          media: `(min-width: ${minWidth}px)`,
        });
      }
    }
  }

  /**
   * Set up window event listeners
   */
  _setupEventListeners() {
    window.addEventListener("resize", this.throttledResize, { passive: true });

    if (this.options.orientationHandling) {
      window.addEventListener(
        "orientationchange",
        this.throttledOrientationChange,
        { passive: true }
      );
    }
  }

  /**
   * Handle window resize events
   */
  _handleResize() {
    this.metrics.resizeEvents++;
    this.updateLayout();
  }

  /**
   * Handle orientation change events
   */
  _handleOrientationChange() {
    // Use setTimeout to ensure viewport dimensions are updated after orientation change
    setTimeout(() => {
      this.updateLayout();
    }, 100);
  }

  /**
   * Apply container styles based on configuration
   */
  _applyContainerStyles(element, config) {
    const styles = {
      width: "100%",
      marginLeft: "auto",
      marginRight: "auto",
      paddingLeft: `${config.padding}px`,
      paddingRight: `${config.padding}px`,
    };

    if (config.type === CONTAINER_TYPES.FIXED) {
      const maxWidth = config.maxWidths[this.currentBreakpoint];
      if (maxWidth) {
        styles.maxWidth = `${maxWidth}px`;
      }
    }

    Object.assign(element.style, styles);
  }

  /**
   * Apply grid styles to an element
   */
  _applyGridStyles(element, config) {
    const styles = {
      display: "grid",
      gap: `${config.rowGap}px ${config.gutter}px`,
    };

    if (config.autoFit) {
      styles.gridTemplateColumns = `repeat(auto-fit, minmax(${config.minColumnWidth}px, 1fr))`;
    } else {
      styles.gridTemplateColumns = `repeat(${config.columns}, 1fr)`;
    }

    Object.assign(element.style, styles);
  }

  /**
   * Update a container's styling based on current breakpoint
   */
  _updateContainer(config) {
    if (
      config.type === CONTAINER_TYPES.RESPONSIVE ||
      config.type === CONTAINER_TYPES.FIXED
    ) {
      const maxWidth = config.maxWidths[this.currentBreakpoint];
      if (maxWidth) {
        config.element.style.maxWidth = `${maxWidth}px`;
      } else {
        config.element.style.maxWidth = "none";
      }
    }
  }

  /**
   * Notify observers of breakpoint changes
   */
  _notifyBreakpointChange(oldBreakpoint, newBreakpoint) {
    this.metrics.breakpointChanges++;

    for (const observer of this.observers) {
      if (observer.type === "breakpoint") {
        try {
          observer.callback(newBreakpoint, oldBreakpoint);
        } catch (error) {
          console.error("Breakpoint observer error:", error);
        }
      }
    }

    this._emitEvent("breakpointChanged", {
      from: oldBreakpoint,
      to: newBreakpoint,
      viewport: this.viewportDimensions,
    });
  }

  /**
   * Notify observers of orientation changes
   */
  _notifyOrientationChange(oldOrientation, newOrientation) {
    this.metrics.orientationChanges++;

    for (const observer of this.observers) {
      if (observer.type === "orientation") {
        try {
          observer.callback(newOrientation, oldOrientation);
        } catch (error) {
          console.error("Orientation observer error:", error);
        }
      }
    }

    this._emitEvent("orientationChanged", {
      from: oldOrientation,
      to: newOrientation,
      viewport: this.viewportDimensions,
    });
  }

  /**
   * Cache current layout state for performance
   */
  _cacheLayoutState() {
    const state = {
      breakpoint: this.currentBreakpoint,
      orientation: this.currentOrientation,
      viewport: { ...this.viewportDimensions },
      timestamp: Date.now(),
    };

    this.layoutCache.set("current", state);
  }

  /**
   * Set up debug features
   */
  _setupDebugFeatures() {
    if (this.options.showBreakpointIndicator) {
      this._createBreakpointIndicator();
    }
  }

  /**
   * Create visual breakpoint indicator
   */
  _createBreakpointIndicator() {
    const indicator = document.createElement("div");
    indicator.id = "layout-breakpoint-indicator";
    indicator.style.cssText = `
      position: fixed;
      top: 10px;
      right: 10px;
      background: rgba(0, 0, 0, 0.8);
      color: white;
      padding: 8px 12px;
      border-radius: 4px;
      font-family: monospace;
      font-size: 12px;
      z-index: 10000;
      pointer-events: none;
    `;

    document.body.appendChild(indicator);
    this.debugIndicator = indicator;
    this._updateBreakpointIndicator();
  }

  /**
   * Update breakpoint indicator display
   */
  _updateBreakpointIndicator() {
    if (this.debugIndicator) {
      this.debugIndicator.textContent = `${this.currentBreakpoint} (${this.viewportDimensions.width}×${this.viewportDimensions.height}) ${this.currentOrientation}`;
    }
  }

  /**
   * Remove debug features
   */
  _removeDebugFeatures() {
    if (this.debugIndicator) {
      this.debugIndicator.remove();
      this.debugIndicator = null;
    }
  }

  /**
   * Emit events through event manager if available
   */
  _emitEvent(eventName, data = {}) {
    if (
      this.options.eventManager &&
      typeof this.options.eventManager.emit === "function"
    ) {
      this.options.eventManager.emit(eventName, {
        source: "UILayout",
        timestamp: Date.now(),
        ...data,
      });
    }

    // Also dispatch as DOM event
    const event = new CustomEvent(`layout:${eventName}`, { detail: data });
    window.dispatchEvent(event);
  }

  /**
   * Throttle function execution
   */
  _throttle(func, delay) {
    let timeoutId;
    let lastExecTime = 0;

    return function (...args) {
      const currentTime = Date.now();

      if (currentTime - lastExecTime > delay) {
        func.apply(this, args);
        lastExecTime = currentTime;
      } else {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => {
          func.apply(this, args);
          lastExecTime = Date.now();
        }, delay - (currentTime - lastExecTime));
      }
    };
  }

  /**
   * Generate unique IDs
   */
  _generateId(prefix) {
    return `${prefix}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  }
}

// Export constants
export { BREAKPOINTS, ORIENTATIONS, CONTAINER_TYPES };

// Export default
export default UILayout;
