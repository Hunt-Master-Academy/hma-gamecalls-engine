/**
 * @file ui-mobile.js
 * @brief Mobile Optimization and Touch Interaction System
 *
 * This module provides comprehensive mobile optimization including responsive
 * design, touch gesture recognition, mobile-specific UI patterns, and
 * performance optimization for mobile devices.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Mobile Optimization Implementation
 * @date July 24, 2025
 */

/**
 * Device types and detection
 */
const DEVICE_TYPES = {
  MOBILE: "mobile",
  TABLET: "tablet",
  DESKTOP: "desktop",
};

/**
 * Touch gesture types
 */
const GESTURE_TYPES = {
  TAP: "tap",
  DOUBLE_TAP: "doubletap",
  LONG_PRESS: "longpress",
  SWIPE: "swipe",
  PINCH: "pinch",
  PAN: "pan",
  ROTATE: "rotate",
};

/**
 * Swipe directions
 */
const SWIPE_DIRECTIONS = {
  UP: "up",
  DOWN: "down",
  LEFT: "left",
  RIGHT: "right",
};

/**
 * @class UIMobile
 * @brief Comprehensive mobile optimization and touch interaction system
 *
 * Features:
 * • Device detection and capability assessment
 * • Touch gesture recognition and handling
 * • Mobile-specific UI patterns and components
 * • Responsive design optimization for small screens
 * • Performance optimization for mobile devices
 * • Touch-friendly control sizing and spacing
 * • Haptic feedback integration where available
 * • Mobile keyboard handling and input optimization
 * • Orientation change detection and handling
 * • Mobile-specific accessibility features
 */
export class UIMobile {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Device detection
      enableDeviceDetection: options.enableDeviceDetection !== false,
      enableCapabilityDetection: options.enableCapabilityDetection !== false,

      // Touch gestures
      enableGestures: options.enableGestures !== false,
      gestureThreshold: options.gestureThreshold || 10,
      longPressDelay: options.longPressDelay || 500,
      doubleTapDelay: options.doubleTapDelay || 300,

      // Touch targets
      minimumTouchTarget: options.minimumTouchTarget || 44,
      touchPadding: options.touchPadding || 8,
      enableTouchTargetOptimization:
        options.enableTouchTargetOptimization !== false,

      // Mobile UI
      enableMobileUI: options.enableMobileUI !== false,
      enableBottomNavigation: options.enableBottomNavigation !== false,
      enablePullToRefresh: options.enablePullToRefresh || false,
      enableInfiniteScroll: options.enableInfiniteScroll || false,

      // Performance
      enableMobileOptimizations: options.enableMobileOptimizations !== false,
      reducedAnimations: options.reducedAnimations || false,
      lazyLoadImages: options.lazyLoadImages !== false,

      // Haptics
      enableHaptics: options.enableHaptics !== false,
      hapticIntensity: options.hapticIntensity || "medium",

      // Debug and testing
      debugMode: options.debugMode || false,
      enableTouchVisualization: options.enableTouchVisualization || false,

      ...options,
    };

    // Device information
    this.deviceInfo = {
      type: DEVICE_TYPES.DESKTOP,
      isMobile: false,
      isTablet: false,
      isAndroid: false,
      isIOS: false,
      hasTouch: false,
      hasHaptics: false,
      screenSize: {
        width: window.innerWidth,
        height: window.innerHeight,
      },
      orientation: "portrait",
      pixelRatio: window.devicePixelRatio || 1,
    };

    // Touch and gesture state
    this.touchState = {
      active: false,
      touches: new Map(),
      currentGesture: null,
      gestureHistory: [],
    };

    // Gesture recognition
    this.gestureRecognizers = new Map();
    this.activeGestures = new Set();

    // Mobile UI state
    this.mobileUI = {
      bottomNavigation: null,
      pullToRefresh: null,
      infiniteScroll: null,
      activeModals: new Set(),
    };

    // Performance tracking
    this.performanceMetrics = {
      touchEvents: 0,
      gesturesRecognized: 0,
      averageResponseTime: 0,
      frameDrops: 0,
    };

    this.isInitialized = false;
    this._initializeMobile();
  }

  /**
   * Initialize mobile optimization system
   */
  _initializeMobile() {
    try {
      // Detect device capabilities
      this._detectDevice();

      // Set up touch event handling
      this._setupTouchHandling();

      // Initialize gesture recognition
      this._initializeGestures();

      // Apply mobile optimizations
      this._applyMobileOptimizations();

      // Set up orientation change handling
      this._setupOrientationHandling();

      // Initialize mobile UI patterns
      this._initializeMobileUI();

      // Set up event listeners
      this._setupEventListeners();

      this.isInitialized = true;
      this.log("UIMobile initialized successfully", "success");

      // Emit initialization event
      this.eventManager.emit("mobileInitialized", {
        deviceInfo: this.deviceInfo,
        enabledFeatures: this._getEnabledFeatures(),
        timestamp: Date.now(),
      });
    } catch (error) {
      this.log(`Mobile initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Detect device type and capabilities
   */
  _detectDevice() {
    const userAgent = navigator.userAgent;
    const platform = navigator.platform;

    // Detect mobile devices
    this.deviceInfo.isMobile =
      /Android|iPhone|iPod|BlackBerry|IEMobile|Opera Mini/i.test(userAgent);

    // Detect tablets
    this.deviceInfo.isTablet =
      /iPad|Android.*(?!Mobile)/i.test(userAgent) ||
      (platform === "MacIntel" && navigator.maxTouchPoints > 1);

    // Determine device type
    if (this.deviceInfo.isMobile) {
      this.deviceInfo.type = DEVICE_TYPES.MOBILE;
    } else if (this.deviceInfo.isTablet) {
      this.deviceInfo.type = DEVICE_TYPES.TABLET;
    } else {
      this.deviceInfo.type = DEVICE_TYPES.DESKTOP;
    }

    // Detect specific platforms
    this.deviceInfo.isAndroid = /Android/i.test(userAgent);
    this.deviceInfo.isIOS = /iPhone|iPad|iPod/i.test(userAgent);

    // Detect touch capability
    this.deviceInfo.hasTouch =
      "ontouchstart" in window ||
      navigator.maxTouchPoints > 0 ||
      navigator.msMaxTouchPoints > 0;

    // Detect haptics capability
    this.deviceInfo.hasHaptics = "vibrate" in navigator;

    // Get screen information
    this.deviceInfo.screenSize = {
      width: window.innerWidth,
      height: window.innerHeight,
    };

    // Detect orientation
    this.deviceInfo.orientation =
      window.innerWidth > window.innerHeight ? "landscape" : "portrait";

    // Get pixel ratio
    this.deviceInfo.pixelRatio = window.devicePixelRatio || 1;

    this.log(`Device detected: ${JSON.stringify(this.deviceInfo)}`, "info");
  }

  /**
   * Set up touch event handling
   */
  _setupTouchHandling() {
    if (!this.deviceInfo.hasTouch || !this.options.enableGestures) return;

    // Passive event listeners for better performance
    const passiveOptions = { passive: false };

    document.addEventListener(
      "touchstart",
      (e) => {
        this._handleTouchStart(e);
      },
      passiveOptions
    );

    document.addEventListener(
      "touchmove",
      (e) => {
        this._handleTouchMove(e);
      },
      passiveOptions
    );

    document.addEventListener(
      "touchend",
      (e) => {
        this._handleTouchEnd(e);
      },
      passiveOptions
    );

    document.addEventListener(
      "touchcancel",
      (e) => {
        this._handleTouchCancel(e);
      },
      passiveOptions
    );

    this.log("Touch event handling initialized", "success");
  }

  /**
   * Handle touch start events
   */
  _handleTouchStart(event) {
    this.performanceMetrics.touchEvents++;
    this.touchState.active = true;

    // Store touch information
    Array.from(event.changedTouches).forEach((touch) => {
      this.touchState.touches.set(touch.identifier, {
        id: touch.identifier,
        startX: touch.clientX,
        startY: touch.clientY,
        currentX: touch.clientX,
        currentY: touch.clientY,
        startTime: Date.now(),
        target: event.target,
        moved: false,
      });
    });

    // Start gesture recognition
    this._recognizeGesture(event);

    // Add touch visualization if enabled
    if (this.options.enableTouchVisualization) {
      this._visualizeTouch(event);
    }

    // Emit touch start event
    this.eventManager.emit("touchStart", {
      touches: event.touches.length,
      target: event.target,
      timestamp: Date.now(),
    });
  }

  /**
   * Handle touch move events
   */
  _handleTouchMove(event) {
    // Update touch information
    Array.from(event.changedTouches).forEach((touch) => {
      const touchData = this.touchState.touches.get(touch.identifier);
      if (touchData) {
        touchData.currentX = touch.clientX;
        touchData.currentY = touch.clientY;
        touchData.moved = true;
      }
    });

    // Continue gesture recognition
    this._recognizeGesture(event);

    // Prevent default behavior for certain gestures
    if (
      this.touchState.currentGesture &&
      ["swipe", "pinch", "pan"].includes(this.touchState.currentGesture.type)
    ) {
      event.preventDefault();
    }
  }

  /**
   * Handle touch end events
   */
  _handleTouchEnd(event) {
    // Complete gesture recognition
    this._recognizeGesture(event);

    // Remove ended touches
    Array.from(event.changedTouches).forEach((touch) => {
      this.touchState.touches.delete(touch.identifier);
    });

    // Check if all touches ended
    if (this.touchState.touches.size === 0) {
      this.touchState.active = false;
      this.touchState.currentGesture = null;
    }

    // Emit touch end event
    this.eventManager.emit("touchEnd", {
      touches: event.touches.length,
      timestamp: Date.now(),
    });
  }

  /**
   * Handle touch cancel events
   */
  _handleTouchCancel(event) {
    // Clear all touch data
    this.touchState.touches.clear();
    this.touchState.active = false;
    this.touchState.currentGesture = null;

    // Emit touch cancel event
    this.eventManager.emit("touchCancel", {
      timestamp: Date.now(),
    });
  }

  /**
   * Initialize gesture recognition system
   */
  _initializeGestures() {
    // Register built-in gesture recognizers
    this._registerGestureRecognizer("tap", this._recognizeTap.bind(this));
    this._registerGestureRecognizer(
      "doubletap",
      this._recognizeDoubleTap.bind(this)
    );
    this._registerGestureRecognizer(
      "longpress",
      this._recognizeLongPress.bind(this)
    );
    this._registerGestureRecognizer("swipe", this._recognizeSwipe.bind(this));
    this._registerGestureRecognizer("pinch", this._recognizePinch.bind(this));
    this._registerGestureRecognizer("pan", this._recognizePan.bind(this));

    this.log("Gesture recognition system initialized", "success");
  }

  /**
   * Register new gesture recognizer
   */
  _registerGestureRecognizer(name, recognizer) {
    this.gestureRecognizers.set(name, recognizer);
  }

  /**
   * Recognize gestures from touch events
   */
  _recognizeGesture(event) {
    // Run all gesture recognizers
    this.gestureRecognizers.forEach((recognizer, name) => {
      const gesture = recognizer(event);
      if (gesture) {
        this._handleGesture(gesture);
      }
    });
  }

  /**
   * Recognize tap gesture
   */
  _recognizeTap(event) {
    if (event.type === "touchend" && this.touchState.touches.size === 0) {
      const touches = Array.from(event.changedTouches);
      if (touches.length === 1) {
        const touch = touches[0];
        const touchData = this.touchState.touches.get(touch.identifier);

        if (touchData && !touchData.moved) {
          const duration = Date.now() - touchData.startTime;
          if (duration < this.options.longPressDelay) {
            return {
              type: GESTURE_TYPES.TAP,
              x: touch.clientX,
              y: touch.clientY,
              target: touchData.target,
              duration: duration,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Recognize swipe gesture
   */
  _recognizeSwipe(event) {
    if (event.type === "touchend" && this.touchState.touches.size === 0) {
      const touches = Array.from(event.changedTouches);
      if (touches.length === 1) {
        const touch = touches[0];
        const touchData = this.touchState.touches.get(touch.identifier);

        if (touchData && touchData.moved) {
          const deltaX = touch.clientX - touchData.startX;
          const deltaY = touch.clientY - touchData.startY;
          const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);

          if (distance > this.options.gestureThreshold) {
            let direction;
            if (Math.abs(deltaX) > Math.abs(deltaY)) {
              direction =
                deltaX > 0 ? SWIPE_DIRECTIONS.RIGHT : SWIPE_DIRECTIONS.LEFT;
            } else {
              direction =
                deltaY > 0 ? SWIPE_DIRECTIONS.DOWN : SWIPE_DIRECTIONS.UP;
            }

            return {
              type: GESTURE_TYPES.SWIPE,
              direction: direction,
              distance: distance,
              deltaX: deltaX,
              deltaY: deltaY,
              startX: touchData.startX,
              startY: touchData.startY,
              endX: touch.clientX,
              endY: touch.clientY,
              target: touchData.target,
              duration: Date.now() - touchData.startTime,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Recognize long press gesture
   */
  _recognizeLongPress(event) {
    if (event.type === "touchstart") {
      const touches = Array.from(event.touches);
      if (touches.length === 1) {
        const touch = touches[0];

        setTimeout(() => {
          const touchData = this.touchState.touches.get(touch.identifier);
          if (touchData && !touchData.moved && this.touchState.active) {
            const gesture = {
              type: GESTURE_TYPES.LONG_PRESS,
              x: touch.clientX,
              y: touch.clientY,
              target: event.target,
              duration: this.options.longPressDelay,
              timestamp: Date.now(),
            };

            this._handleGesture(gesture);
          }
        }, this.options.longPressDelay);
      }
    }
    return null;
  }

  /**
   * Recognize pinch gesture
   */
  _recognizePinch(event) {
    if (event.touches && event.touches.length === 2) {
      const touch1 = event.touches[0];
      const touch2 = event.touches[1];

      const distance = Math.sqrt(
        Math.pow(touch2.clientX - touch1.clientX, 2) +
          Math.pow(touch2.clientY - touch1.clientY, 2)
      );

      if (event.type === "touchstart") {
        this._lastPinchDistance = distance;
      } else if (event.type === "touchmove" && this._lastPinchDistance) {
        const scale = distance / this._lastPinchDistance;

        if (Math.abs(scale - 1) > 0.1) {
          // Minimum scale change threshold
          return {
            type: GESTURE_TYPES.PINCH,
            scale: scale,
            distance: distance,
            centerX: (touch1.clientX + touch2.clientX) / 2,
            centerY: (touch1.clientY + touch2.clientY) / 2,
            timestamp: Date.now(),
          };
        }
      }
    }
    return null;
  }

  /**
   * Recognize pan gesture
   */
  _recognizePan(event) {
    if (event.type === "touchmove" && event.touches.length === 1) {
      const touch = event.touches[0];
      const touchData = this.touchState.touches.get(touch.identifier);

      if (touchData && touchData.moved) {
        return {
          type: GESTURE_TYPES.PAN,
          deltaX: touch.clientX - touchData.startX,
          deltaY: touch.clientY - touchData.startY,
          currentX: touch.clientX,
          currentY: touch.clientY,
          startX: touchData.startX,
          startY: touchData.startY,
          target: touchData.target,
          timestamp: Date.now(),
        };
      }
    }
    return null;
  }

  /**
   * Handle recognized gesture
   */
  _handleGesture(gesture) {
    this.performanceMetrics.gesturesRecognized++;

    // Store gesture in history
    this.touchState.gestureHistory.push(gesture);
    if (this.touchState.gestureHistory.length > 50) {
      this.touchState.gestureHistory.shift();
    }

    // Set current gesture
    this.touchState.currentGesture = gesture;

    // Provide haptic feedback if available
    if (this.options.enableHaptics && this.deviceInfo.hasHaptics) {
      this._provideHapticFeedback(gesture);
    }

    // Emit gesture event
    this.eventManager.emit("gesture", gesture);
    this.eventManager.emit(`gesture${gesture.type}`, gesture);

    this.log(`Gesture recognized: ${gesture.type}`, "info");
  }

  /**
   * Apply mobile optimizations
   */
  _applyMobileOptimizations() {
    if (!this.options.enableMobileOptimizations) return;

    const html = document.documentElement;
    const body = document.body;

    // Add mobile device classes
    html.classList.add(`device-${this.deviceInfo.type}`);

    if (this.deviceInfo.isMobile) {
      html.classList.add("mobile-device");
    }

    if (this.deviceInfo.isTablet) {
      html.classList.add("tablet-device");
    }

    if (this.deviceInfo.hasTouch) {
      html.classList.add("touch-device");
    }

    // Apply mobile-specific CSS
    this._applyMobileStyles();

    // Optimize touch targets
    if (this.options.enableTouchTargetOptimization) {
      this._optimizeTouchTargets();
    }

    // Disable hover effects on touch devices
    if (this.deviceInfo.hasTouch) {
      html.classList.add("no-hover");
    }

    this.log("Mobile optimizations applied", "success");
  }

  /**
   * Apply mobile-specific CSS styles
   */
  _applyMobileStyles() {
    const style = document.createElement("style");
    style.id = "ui-mobile-styles";
    style.textContent = `
      /* Mobile-specific styles */
      .mobile-device {
        -webkit-text-size-adjust: 100%;
        -webkit-tap-highlight-color: transparent;
      }

      /* Touch target optimization */
      .touch-device button,
      .touch-device input,
      .touch-device select,
      .touch-device textarea,
      .touch-device .touch-target {
        min-height: ${this.options.minimumTouchTarget}px;
        min-width: ${this.options.minimumTouchTarget}px;
        padding: ${this.options.touchPadding}px;
      }

      /* Disable hover effects on touch devices */
      .no-hover *:hover {
        background-color: initial !important;
        color: initial !important;
        border-color: initial !important;
      }

      /* Mobile UI patterns */
      .mobile-bottom-nav {
        position: fixed;
        bottom: 0;
        left: 0;
        right: 0;
        background: var(--color-background);
        border-top: 1px solid var(--color-border);
        padding: 8px;
        z-index: 1000;
      }

      /* Touch visualization */
      .touch-visualization {
        position: absolute;
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background: rgba(0, 100, 200, 0.3);
        border: 2px solid rgba(0, 100, 200, 0.6);
        pointer-events: none;
        transform: translate(-50%, -50%);
        animation: touch-ripple 0.3s ease-out;
        z-index: 10000;
      }

      @keyframes touch-ripple {
        0% {
          transform: translate(-50%, -50%) scale(0);
          opacity: 1;
        }
        100% {
          transform: translate(-50%, -50%) scale(1);
          opacity: 0;
        }
      }

      /* Responsive adjustments */
      @media (max-width: 768px) {
        .mobile-hide {
          display: none !important;
        }

        .mobile-full-width {
          width: 100% !important;
        }

        .mobile-stack {
          flex-direction: column !important;
        }
      }
    `;
    document.head.appendChild(style);
  }

  /**
   * Optimize touch targets for better accessibility
   */
  _optimizeTouchTargets() {
    const touchableElements = document.querySelectorAll(
      'button, input, select, textarea, a, [role="button"], [onclick]'
    );

    touchableElements.forEach((element) => {
      const rect = element.getBoundingClientRect();
      const isSmall =
        rect.width < this.options.minimumTouchTarget ||
        rect.height < this.options.minimumTouchTarget;

      if (isSmall) {
        element.classList.add("touch-target-optimized");
      }
    });
  }

  /**
   * Set up orientation change handling
   */
  _setupOrientationHandling() {
    const handleOrientationChange = () => {
      const newOrientation =
        window.innerWidth > window.innerHeight ? "landscape" : "portrait";

      if (newOrientation !== this.deviceInfo.orientation) {
        const oldOrientation = this.deviceInfo.orientation;
        this.deviceInfo.orientation = newOrientation;

        // Update screen size
        this.deviceInfo.screenSize = {
          width: window.innerWidth,
          height: window.innerHeight,
        };

        // Emit orientation change event
        this.eventManager.emit("orientationChanged", {
          oldOrientation,
          newOrientation,
          screenSize: this.deviceInfo.screenSize,
          timestamp: Date.now(),
        });

        this.log(
          `Orientation changed: ${oldOrientation} → ${newOrientation}`,
          "info"
        );
      }
    };

    // Handle orientation change
    window.addEventListener("orientationchange", () => {
      setTimeout(handleOrientationChange, 100); // Delay to ensure correct dimensions
    });

    // Also handle resize as fallback
    window.addEventListener("resize", handleOrientationChange);
  }

  /**
   * Initialize mobile UI patterns
   */
  _initializeMobileUI() {
    if (!this.deviceInfo.isMobile || !this.options.enableMobileUI) return;

    // Initialize bottom navigation if enabled
    if (this.options.enableBottomNavigation) {
      this._initializeBottomNavigation();
    }

    // Initialize pull to refresh if enabled
    if (this.options.enablePullToRefresh) {
      this._initializePullToRefresh();
    }

    this.log("Mobile UI patterns initialized", "success");
  }

  /**
   * Initialize bottom navigation
   */
  _initializeBottomNavigation() {
    const bottomNav = document.createElement("nav");
    bottomNav.className = "mobile-bottom-nav";
    bottomNav.setAttribute("role", "navigation");
    bottomNav.setAttribute("aria-label", "Bottom navigation");

    // Add navigation items
    const navItems = [
      { id: "setup", icon: "⚙️", label: "Setup" },
      { id: "practice", icon: "🎯", label: "Practice" },
      { id: "results", icon: "📊", label: "Results" },
    ];

    navItems.forEach((item) => {
      const navItem = document.createElement("button");
      navItem.className = "bottom-nav-item";
      navItem.dataset.navId = item.id;
      navItem.innerHTML = `
        <span class="nav-icon">${item.icon}</span>
        <span class="nav-label">${item.label}</span>
      `;

      navItem.addEventListener("click", () => {
        this._handleBottomNavigation(item.id);
      });

      bottomNav.appendChild(navItem);
    });

    document.body.appendChild(bottomNav);
    this.mobileUI.bottomNavigation = bottomNav;
  }

  /**
   * Handle bottom navigation
   */
  _handleBottomNavigation(navId) {
    // Update active state
    const navItems =
      this.mobileUI.bottomNavigation.querySelectorAll(".bottom-nav-item");
    navItems.forEach((item) => {
      if (item.dataset.navId === navId) {
        item.classList.add("active");
      } else {
        item.classList.remove("active");
      }
    });

    // Emit navigation event
    this.eventManager.emit("bottomNavigation", {
      navId,
      timestamp: Date.now(),
    });
  }

  /**
   * Provide haptic feedback
   */
  _provideHapticFeedback(gesture) {
    if (!this.deviceInfo.hasHaptics) return;

    let vibrationPattern;

    switch (gesture.type) {
      case GESTURE_TYPES.TAP:
        vibrationPattern = [10];
        break;
      case GESTURE_TYPES.LONG_PRESS:
        vibrationPattern = [50];
        break;
      case GESTURE_TYPES.SWIPE:
        vibrationPattern = [20];
        break;
      default:
        vibrationPattern = [15];
    }

    navigator.vibrate(vibrationPattern);
  }

  /**
   * Visualize touch for debugging
   */
  _visualizeTouch(event) {
    Array.from(event.touches).forEach((touch) => {
      const visualization = document.createElement("div");
      visualization.className = "touch-visualization";
      visualization.style.left = `${touch.clientX}px`;
      visualization.style.top = `${touch.clientY}px`;

      document.body.appendChild(visualization);

      setTimeout(() => {
        if (visualization.parentNode) {
          visualization.parentNode.removeChild(visualization);
        }
      }, 300);
    });
  }

  /**
   * Get enabled features
   */
  _getEnabledFeatures() {
    const features = [];

    if (this.options.enableGestures) features.push("gestures");
    if (this.options.enableMobileUI) features.push("mobileUI");
    if (this.options.enableHaptics) features.push("haptics");
    if (this.options.enableTouchTargetOptimization)
      features.push("touchOptimization");

    return features;
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Handle viewport changes
    window.addEventListener("resize", () => {
      this.deviceInfo.screenSize = {
        width: window.innerWidth,
        height: window.innerHeight,
      };
    });

    // Handle focus events for touch devices
    if (this.deviceInfo.hasTouch) {
      document.addEventListener("focusin", (e) => {
        // Scroll focused element into view on mobile
        if (this.deviceInfo.isMobile) {
          setTimeout(() => {
            e.target.scrollIntoView({ behavior: "smooth", block: "center" });
          }, 300);
        }
      });
    }
  }

  /**
   * Get mobile optimization status
   */
  getMobileStatus() {
    return {
      isInitialized: this.isInitialized,
      deviceInfo: { ...this.deviceInfo },
      enabledFeatures: this._getEnabledFeatures(),
      touchState: {
        active: this.touchState.active,
        activeTouches: this.touchState.touches.size,
        currentGesture: this.touchState.currentGesture?.type || null,
      },
      performanceMetrics: { ...this.performanceMetrics },
      timestamp: Date.now(),
    };
  }

  /**
   * Register custom gesture recognizer
   */
  registerGesture(name, recognizer) {
    this._registerGestureRecognizer(name, recognizer);
    this.log(`Custom gesture registered: ${name}`, "success");
  }

  /**
   * Trigger haptic feedback manually
   */
  triggerHaptic(intensity = "medium") {
    if (!this.deviceInfo.hasHaptics) return false;

    let pattern;
    switch (intensity) {
      case "light":
        pattern = [10];
        break;
      case "medium":
        pattern = [25];
        break;
      case "heavy":
        pattern = [50];
        break;
      default:
        pattern = [25];
    }

    navigator.vibrate(pattern);
    return true;
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [UIMobile] ${message}`;

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
      this.eventManager.emit("mobileLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "UIMobile",
      });
    }
  }

  /**
   * Clean up and destroy mobile system
   */
  destroy() {
    try {
      // Remove event listeners
      document.removeEventListener("touchstart", this._handleTouchStart);
      document.removeEventListener("touchmove", this._handleTouchMove);
      document.removeEventListener("touchend", this._handleTouchEnd);
      document.removeEventListener("touchcancel", this._handleTouchCancel);

      // Remove mobile UI elements
      if (this.mobileUI.bottomNavigation) {
        this.mobileUI.bottomNavigation.remove();
      }

      // Remove mobile styles
      const mobileStyles = document.getElementById("ui-mobile-styles");
      if (mobileStyles) {
        mobileStyles.remove();
      }

      // Clear touch state
      this.touchState.touches.clear();
      this.touchState.gestureHistory = [];

      // Clear gesture recognizers
      this.gestureRecognizers.clear();
      this.activeGestures.clear();

      this.isInitialized = false;
      this.log("UIMobile destroyed successfully", "success");
    } catch (error) {
      this.log(`Error during UIMobile destruction: ${error.message}`, "error");
      throw error;
    }
  }
}

export default UIMobile;
export { UIMobile, DEVICE_TYPES, GESTURE_TYPES, SWIPE_DIRECTIONS };
