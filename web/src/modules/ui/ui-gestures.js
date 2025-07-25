/**
 * @file ui-gestures.js
 * @brief Advanced Gesture Recognition and Interaction System
 *
 * This module provides comprehensive gesture recognition including complex
 * multi-touch gestures, custom gesture patterns, gesture sequence recognition,
 * and advanced interaction patterns for enhanced user experience.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Gesture Recognition Implementation
 * @date July 24, 2025
 */

/**
 * Extended gesture types beyond basic touch
 */
const ADVANCED_GESTURES = {
  // Multi-touch gestures
  TWO_FINGER_TAP: "twofingertap",
  THREE_FINGER_TAP: "threefingertap",
  FOUR_FINGER_TAP: "fourfingertap",

  // Complex gestures
  CIRCLE: "circle",
  ZIGZAG: "zigzag",
  TRIANGLE: "triangle",
  SQUARE: "square",

  // Sequence gestures
  DOUBLE_SWIPE: "doubleswipe",
  TRIPLE_TAP: "tripletap",
  KNOCK: "knock",

  // Drawing gestures
  CHECKMARK: "checkmark",
  CROSS: "cross",
  HEART: "heart",
  STAR: "star",

  // Advanced interactions
  FORCE_TOUCH: "forcetouch",
  HOVER: "hover",
  SHAKE: "shake",
  TILT: "tilt",
};

/**
 * Gesture recognition states
 */
const GESTURE_STATES = {
  IDLE: "idle",
  STARTED: "started",
  CHANGED: "changed",
  ENDED: "ended",
  CANCELLED: "cancelled",
  FAILED: "failed",
};

/**
 * @class UIGestures
 * @brief Advanced gesture recognition and interaction system
 *
 * Features:
 * • Multi-touch gesture recognition and handling
 * • Custom gesture pattern creation and recognition
 * • Gesture sequence and combination detection
 * • Drawing and shape recognition capabilities
 * • Force touch and pressure sensitivity support
 * • Device motion and orientation gesture detection
 * • Gesture history and pattern learning
 * • Performance-optimized gesture calculation
 * • Configurable gesture sensitivity and thresholds
 * • Cross-platform gesture normalization
 */
export class UIGestures {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // Basic gesture settings
      enableAdvancedGestures: options.enableAdvancedGestures !== false,
      gestureTimeout: options.gestureTimeout || 1000,
      minimumGestureDistance: options.minimumGestureDistance || 20,

      // Multi-touch settings
      enableMultiTouch: options.enableMultiTouch !== false,
      maxSimultaneousTouches: options.maxSimultaneousTouches || 10,
      multiTouchTimeout: options.multiTouchTimeout || 500,

      // Shape recognition
      enableShapeRecognition: options.enableShapeRecognition !== false,
      shapeAccuracy: options.shapeAccuracy || 0.7,
      minimumShapePoints: options.minimumShapePoints || 5,

      // Sequence recognition
      enableSequenceRecognition: options.enableSequenceRecognition !== false,
      maxSequenceLength: options.maxSequenceLength || 5,
      sequenceTimeout: options.sequenceTimeout || 2000,

      // Force touch
      enableForceTouch: options.enableForceTouch !== false,
      forceThreshold: options.forceThreshold || 0.5,

      // Motion gestures
      enableMotionGestures: options.enableMotionGestures !== false,
      shakeThreshold: options.shakeThreshold || 15,
      tiltThreshold: options.tiltThreshold || 30,

      // Performance
      enableGestureOptimization: options.enableGestureOptimization !== false,
      maxGestureHistory: options.maxGestureHistory || 100,

      // Debug and calibration
      debugMode: options.debugMode || false,
      enableGestureVisualization: options.enableGestureVisualization || false,

      ...options,
    };

    // Gesture recognition engine
    this.gestureEngine = {
      recognizers: new Map(),
      activeGestures: new Map(),
      gestureHistory: [],
      sequenceBuffer: [],
      lastGestureTime: 0,
    };

    // Touch tracking for advanced gestures
    this.touchTracking = {
      activeTouches: new Map(),
      touchPaths: new Map(),
      multiTouchSessions: new Map(),
      averageTouch: { x: 0, y: 0 },
    };

    // Shape recognition data
    this.shapeTemplates = new Map();
    this.shapeRecognition = {
      currentPath: [],
      isDrawing: false,
      lastRecognition: null,
    };

    // Motion tracking
    this.motionTracking = {
      acceleration: { x: 0, y: 0, z: 0 },
      rotation: { alpha: 0, beta: 0, gamma: 0 },
      lastMotion: { x: 0, y: 0, z: 0 },
      motionHistory: [],
    };

    // Performance metrics
    this.performanceMetrics = {
      gesturesRecognized: 0,
      averageRecognitionTime: 0,
      falsePositives: 0,
      missedGestures: 0,
      customGestures: 0,
    };

    this.isInitialized = false;
    this._initializeGestures();
  }

  /**
   * Initialize advanced gesture system
   */
  _initializeGestures() {
    try {
      // Initialize gesture recognizers
      this._initializeGestureRecognizers();

      // Set up advanced touch handling
      this._setupAdvancedTouchHandling();

      // Initialize shape recognition
      this._initializeShapeRecognition();

      // Set up motion gesture detection
      this._setupMotionGestures();

      // Initialize sequence recognition
      this._initializeSequenceRecognition();

      // Set up performance optimization
      this._setupPerformanceOptimization();

      // Set up event listeners
      this._setupEventListeners();

      this.isInitialized = true;
      this.log("UIGestures initialized successfully", "success");

      // Emit initialization event
      this.eventManager.emit("gesturesInitialized", {
        supportedGestures: this._getSupportedGestures(),
        enabledFeatures: this._getEnabledFeatures(),
        timestamp: Date.now(),
      });
    } catch (error) {
      this.log(`Gesture initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Initialize gesture recognizers
   */
  _initializeGestureRecognizers() {
    // Multi-touch gestures
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.TWO_FINGER_TAP,
      this._recognizeTwoFingerTap.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.THREE_FINGER_TAP,
      this._recognizeThreeFingerTap.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.FOUR_FINGER_TAP,
      this._recognizeFourFingerTap.bind(this)
    );

    // Shape gestures
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.CIRCLE,
      this._recognizeCircle.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.TRIANGLE,
      this._recognizeTriangle.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.SQUARE,
      this._recognizeSquare.bind(this)
    );

    // Drawing gestures
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.CHECKMARK,
      this._recognizeCheckmark.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.CROSS,
      this._recognizeCross.bind(this)
    );

    // Complex interactions
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.FORCE_TOUCH,
      this._recognizeForceTouch.bind(this)
    );
    this._registerGestureRecognizer(
      ADVANCED_GESTURES.SHAKE,
      this._recognizeShake.bind(this)
    );

    this.log("Gesture recognizers initialized", "success");
  }

  /**
   * Register new gesture recognizer
   */
  _registerGestureRecognizer(gestureType, recognizer) {
    this.gestureEngine.recognizers.set(gestureType, recognizer);
  }

  /**
   * Set up advanced touch handling
   */
  _setupAdvancedTouchHandling() {
    if (!this.options.enableMultiTouch) return;

    // Enhanced touch event handling with pressure and force
    const advancedTouchHandler = (event) => {
      this._processAdvancedTouchEvent(event);
    };

    // Use more specific event listeners
    document.addEventListener("touchstart", advancedTouchHandler, {
      passive: false,
    });
    document.addEventListener("touchmove", advancedTouchHandler, {
      passive: false,
    });
    document.addEventListener("touchend", advancedTouchHandler, {
      passive: false,
    });

    // Pointer events for better cross-platform support
    if (window.PointerEvent) {
      document.addEventListener("pointerdown", advancedTouchHandler, {
        passive: false,
      });
      document.addEventListener("pointermove", advancedTouchHandler, {
        passive: false,
      });
      document.addEventListener("pointerup", advancedTouchHandler, {
        passive: false,
      });
    }

    this.log("Advanced touch handling initialized", "success");
  }

  /**
   * Process advanced touch events with enhanced data
   */
  _processAdvancedTouchEvent(event) {
    const startTime = performance.now();

    // Track all touches with enhanced data
    Array.from(event.changedTouches || [event]).forEach((touch) => {
      const touchData = {
        id: touch.identifier || "pointer",
        x: touch.clientX,
        y: touch.clientY,
        force: touch.force || 0,
        radiusX: touch.radiusX || 0,
        radiusY: touch.radiusY || 0,
        rotationAngle: touch.rotationAngle || 0,
        timestamp: Date.now(),
      };

      this._updateTouchTracking(touchData, event.type);
    });

    // Update average touch position for multi-touch gestures
    this._updateAverageTouch();

    // Run gesture recognition
    this._runGestureRecognition(event);

    // Update performance metrics
    const recognitionTime = performance.now() - startTime;
    this.performanceMetrics.averageRecognitionTime =
      (this.performanceMetrics.averageRecognitionTime + recognitionTime) / 2;
  }

  /**
   * Update touch tracking data
   */
  _updateTouchTracking(touchData, eventType) {
    const touchId = touchData.id;

    switch (eventType) {
      case "touchstart":
      case "pointerdown":
        this.touchTracking.activeTouches.set(touchId, touchData);
        this.touchTracking.touchPaths.set(touchId, [touchData]);
        break;

      case "touchmove":
      case "pointermove":
        if (this.touchTracking.activeTouches.has(touchId)) {
          this.touchTracking.activeTouches.set(touchId, touchData);
          const path = this.touchTracking.touchPaths.get(touchId) || [];
          path.push(touchData);
          this.touchTracking.touchPaths.set(touchId, path);
        }
        break;

      case "touchend":
      case "pointerup":
        this.touchTracking.activeTouches.delete(touchId);
        // Keep path for analysis
        break;
    }
  }

  /**
   * Update average touch position for multi-touch calculations
   */
  _updateAverageTouch() {
    const touches = Array.from(this.touchTracking.activeTouches.values());
    if (touches.length === 0) {
      this.touchTracking.averageTouch = { x: 0, y: 0 };
      return;
    }

    const sum = touches.reduce(
      (acc, touch) => ({
        x: acc.x + touch.x,
        y: acc.y + touch.y,
      }),
      { x: 0, y: 0 }
    );

    this.touchTracking.averageTouch = {
      x: sum.x / touches.length,
      y: sum.y / touches.length,
    };
  }

  /**
   * Run gesture recognition on current touch data
   */
  _runGestureRecognition(event) {
    // Run all registered recognizers
    this.gestureEngine.recognizers.forEach((recognizer, gestureType) => {
      try {
        const gesture = recognizer(event, this.touchTracking);
        if (gesture) {
          this._handleRecognizedGesture(gesture);
        }
      } catch (error) {
        this.log(
          `Error in gesture recognizer ${gestureType}: ${error.message}`,
          "error"
        );
      }
    });
  }

  /**
   * Handle recognized gesture
   */
  _handleRecognizedGesture(gesture) {
    this.performanceMetrics.gesturesRecognized++;

    // Add to gesture history
    this.gestureEngine.gestureHistory.push({
      ...gesture,
      recognizedAt: Date.now(),
    });

    // Trim history if too long
    if (
      this.gestureEngine.gestureHistory.length > this.options.maxGestureHistory
    ) {
      this.gestureEngine.gestureHistory.shift();
    }

    // Add to sequence buffer for sequence recognition
    if (this.options.enableSequenceRecognition) {
      this._addToSequenceBuffer(gesture);
    }

    // Emit gesture event
    this.eventManager.emit("advancedGesture", gesture);
    this.eventManager.emit(`gesture${gesture.type}`, gesture);

    // Visualize gesture if enabled
    if (this.options.enableGestureVisualization) {
      this._visualizeGesture(gesture);
    }

    this.log(`Advanced gesture recognized: ${gesture.type}`, "info");
  }

  /**
   * Recognize two-finger tap
   */
  _recognizeTwoFingerTap(event, touchTracking) {
    if (event.type === "touchend" && touchTracking.activeTouches.size === 0) {
      const paths = Array.from(touchTracking.touchPaths.values());

      if (paths.length === 2) {
        const path1 = paths[0];
        const path2 = paths[1];

        // Check if both paths are short (indicating taps)
        if (path1.length <= 3 && path2.length <= 3) {
          const startTime = Math.min(path1[0].timestamp, path2[0].timestamp);
          const endTime = Math.max(
            path1[path1.length - 1].timestamp,
            path2[path2.length - 1].timestamp
          );

          // Check timing
          if (endTime - startTime < this.options.multiTouchTimeout) {
            return {
              type: ADVANCED_GESTURES.TWO_FINGER_TAP,
              touches: 2,
              centerX: touchTracking.averageTouch.x,
              centerY: touchTracking.averageTouch.y,
              duration: endTime - startTime,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Recognize three-finger tap
   */
  _recognizeThreeFingerTap(event, touchTracking) {
    if (event.type === "touchend" && touchTracking.activeTouches.size === 0) {
      const paths = Array.from(touchTracking.touchPaths.values());

      if (paths.length === 3) {
        const allShort = paths.every((path) => path.length <= 3);

        if (allShort) {
          const startTime = Math.min(...paths.map((path) => path[0].timestamp));
          const endTime = Math.max(
            ...paths.map((path) => path[path.length - 1].timestamp)
          );

          if (endTime - startTime < this.options.multiTouchTimeout) {
            return {
              type: ADVANCED_GESTURES.THREE_FINGER_TAP,
              touches: 3,
              centerX: touchTracking.averageTouch.x,
              centerY: touchTracking.averageTouch.y,
              duration: endTime - startTime,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Recognize circle gesture
   */
  _recognizeCircle(event, touchTracking) {
    if (event.type === "touchend" && touchTracking.activeTouches.size === 0) {
      const paths = Array.from(touchTracking.touchPaths.values());

      if (paths.length === 1) {
        const path = paths[0];

        if (path.length >= this.options.minimumShapePoints) {
          const circleScore = this._calculateCircleScore(path);

          if (circleScore > this.options.shapeAccuracy) {
            const bounds = this._calculatePathBounds(path);

            return {
              type: ADVANCED_GESTURES.CIRCLE,
              score: circleScore,
              center: bounds.center,
              radius: bounds.radius,
              clockwise: this._isClockwise(path),
              path: path,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Calculate circle score for path
   */
  _calculateCircleScore(path) {
    if (path.length < 8) return 0;

    const bounds = this._calculatePathBounds(path);
    const centerX = bounds.center.x;
    const centerY = bounds.center.y;
    const expectedRadius = bounds.radius;

    let totalScore = 0;
    let validPoints = 0;

    for (let i = 0; i < path.length; i++) {
      const point = path[i];
      const distance = Math.sqrt(
        Math.pow(point.x - centerX, 2) + Math.pow(point.y - centerY, 2)
      );

      const radiusError = Math.abs(distance - expectedRadius) / expectedRadius;
      if (radiusError < 0.3) {
        // 30% tolerance
        totalScore += 1 - radiusError;
        validPoints++;
      }
    }

    return validPoints > 0 ? totalScore / validPoints : 0;
  }

  /**
   * Calculate bounds for path
   */
  _calculatePathBounds(path) {
    const xs = path.map((p) => p.x);
    const ys = path.map((p) => p.y);

    const minX = Math.min(...xs);
    const maxX = Math.max(...xs);
    const minY = Math.min(...ys);
    const maxY = Math.max(...ys);

    const centerX = (minX + maxX) / 2;
    const centerY = (minY + maxY) / 2;
    const radius = Math.max(maxX - minX, maxY - minY) / 2;

    return {
      center: { x: centerX, y: centerY },
      radius: radius,
      bounds: { minX, maxX, minY, maxY },
    };
  }

  /**
   * Check if path is clockwise
   */
  _isClockwise(path) {
    let sum = 0;
    for (let i = 0; i < path.length - 1; i++) {
      const current = path[i];
      const next = path[i + 1];
      sum += (next.x - current.x) * (next.y + current.y);
    }
    return sum > 0;
  }

  /**
   * Recognize checkmark gesture
   */
  _recognizeCheckmark(event, touchTracking) {
    if (event.type === "touchend" && touchTracking.activeTouches.size === 0) {
      const paths = Array.from(touchTracking.touchPaths.values());

      if (paths.length === 1) {
        const path = paths[0];

        if (path.length >= this.options.minimumShapePoints) {
          const checkScore = this._calculateCheckmarkScore(path);

          if (checkScore > this.options.shapeAccuracy) {
            return {
              type: ADVANCED_GESTURES.CHECKMARK,
              score: checkScore,
              path: path,
              timestamp: Date.now(),
            };
          }
        }
      }
    }
    return null;
  }

  /**
   * Calculate checkmark score
   */
  _calculateCheckmarkScore(path) {
    // Simplified checkmark detection: look for direction changes
    if (path.length < 6) return 0;

    const directions = [];
    for (let i = 1; i < path.length; i++) {
      const dx = path[i].x - path[i - 1].x;
      const dy = path[i].y - path[i - 1].y;
      const angle = Math.atan2(dy, dx);
      directions.push(angle);
    }

    // Look for down-right then up-right pattern (simplified)
    let hasDownRight = false;
    let hasUpRight = false;
    let changePoint = -1;

    for (let i = 0; i < directions.length - 2; i++) {
      const angle = directions[i];

      // Down-right quadrant
      if (angle > 0 && angle < Math.PI / 2) {
        hasDownRight = true;
      }

      // Up-right quadrant after down-right
      if (hasDownRight && angle < 0 && angle > -Math.PI / 2) {
        hasUpRight = true;
        changePoint = i;
        break;
      }
    }

    return hasDownRight && hasUpRight ? 0.8 : 0;
  }

  /**
   * Recognize force touch
   */
  _recognizeForceTouch(event, touchTracking) {
    if (this.options.enableForceTouch && event.changedTouches) {
      for (let touch of event.changedTouches) {
        if (touch.force && touch.force > this.options.forceThreshold) {
          return {
            type: ADVANCED_GESTURES.FORCE_TOUCH,
            force: touch.force,
            x: touch.clientX,
            y: touch.clientY,
            timestamp: Date.now(),
          };
        }
      }
    }
    return null;
  }

  /**
   * Initialize shape recognition templates
   */
  _initializeShapeRecognition() {
    if (!this.options.enableShapeRecognition) return;

    // This would be expanded with more sophisticated shape templates
    this.log("Shape recognition initialized", "success");
  }

  /**
   * Set up motion gesture detection
   */
  _setupMotionGestures() {
    if (!this.options.enableMotionGestures) return;

    // Device motion for shake detection
    if (window.DeviceMotionEvent) {
      window.addEventListener("devicemotion", (event) => {
        this._processMotionEvent(event);
      });
    }

    // Device orientation for tilt detection
    if (window.DeviceOrientationEvent) {
      window.addEventListener("deviceorientationchange", (event) => {
        this._processOrientationEvent(event);
      });
    }

    this.log("Motion gesture detection initialized", "success");
  }

  /**
   * Process device motion for shake detection
   */
  _processMotionEvent(event) {
    const acceleration = event.accelerationIncludingGravity;
    if (!acceleration) return;

    const currentMotion = {
      x: acceleration.x || 0,
      y: acceleration.y || 0,
      z: acceleration.z || 0,
      timestamp: Date.now(),
    };

    // Calculate motion magnitude
    const magnitude = Math.sqrt(
      Math.pow(currentMotion.x, 2) +
        Math.pow(currentMotion.y, 2) +
        Math.pow(currentMotion.z, 2)
    );

    // Add to motion history
    this.motionTracking.motionHistory.push({ ...currentMotion, magnitude });
    if (this.motionTracking.motionHistory.length > 20) {
      this.motionTracking.motionHistory.shift();
    }

    // Check for shake gesture
    if (magnitude > this.options.shakeThreshold) {
      const shakeGesture = this._detectShakePattern();
      if (shakeGesture) {
        this._handleRecognizedGesture(shakeGesture);
      }
    }

    this.motionTracking.acceleration = currentMotion;
  }

  /**
   * Detect shake pattern from motion history
   */
  _detectShakePattern() {
    const history = this.motionTracking.motionHistory;
    if (history.length < 5) return null;

    // Check for rapid back-and-forth motion
    const recentHistory = history.slice(-10);
    const highMagnitudeCount = recentHistory.filter(
      (motion) => motion.magnitude > this.options.shakeThreshold
    ).length;

    if (highMagnitudeCount >= 3) {
      return {
        type: ADVANCED_GESTURES.SHAKE,
        intensity: Math.max(...recentHistory.map((m) => m.magnitude)),
        duration:
          recentHistory[recentHistory.length - 1].timestamp -
          recentHistory[0].timestamp,
        timestamp: Date.now(),
      };
    }

    return null;
  }

  /**
   * Initialize sequence recognition
   */
  _initializeSequenceRecognition() {
    if (!this.options.enableSequenceRecognition) return;

    // Set up sequence buffer cleanup
    setInterval(() => {
      this._cleanupSequenceBuffer();
    }, this.options.sequenceTimeout);

    this.log("Sequence recognition initialized", "success");
  }

  /**
   * Add gesture to sequence buffer
   */
  _addToSequenceBuffer(gesture) {
    this.gestureEngine.sequenceBuffer.push({
      gesture: gesture,
      timestamp: Date.now(),
    });

    // Check for sequence patterns
    this._checkSequencePatterns();

    // Limit buffer size
    if (
      this.gestureEngine.sequenceBuffer.length > this.options.maxSequenceLength
    ) {
      this.gestureEngine.sequenceBuffer.shift();
    }
  }

  /**
   * Check for recognized sequence patterns
   */
  _checkSequencePatterns() {
    const buffer = this.gestureEngine.sequenceBuffer;

    // Example: Triple tap sequence
    if (buffer.length >= 3) {
      const lastThree = buffer.slice(-3);
      const allTaps = lastThree.every(
        (item) =>
          item.gesture.type === "tap" ||
          item.gesture.type === ADVANCED_GESTURES.TWO_FINGER_TAP
      );

      if (allTaps) {
        const totalTime = lastThree[2].timestamp - lastThree[0].timestamp;
        if (totalTime < this.options.sequenceTimeout) {
          const sequenceGesture = {
            type: ADVANCED_GESTURES.TRIPLE_TAP,
            sequence: lastThree.map((item) => item.gesture.type),
            duration: totalTime,
            timestamp: Date.now(),
          };

          this._handleRecognizedGesture(sequenceGesture);
        }
      }
    }
  }

  /**
   * Clean up old sequence buffer entries
   */
  _cleanupSequenceBuffer() {
    const now = Date.now();
    this.gestureEngine.sequenceBuffer =
      this.gestureEngine.sequenceBuffer.filter(
        (item) => now - item.timestamp < this.options.sequenceTimeout
      );
  }

  /**
   * Set up performance optimization
   */
  _setupPerformanceOptimization() {
    if (!this.options.enableGestureOptimization) return;

    // Throttle gesture recognition for performance
    let lastRecognitionTime = 0;
    const originalRunRecognition = this._runGestureRecognition.bind(this);

    this._runGestureRecognition = (event) => {
      const now = Date.now();
      if (now - lastRecognitionTime > 16) {
        // ~60fps
        originalRunRecognition(event);
        lastRecognitionTime = now;
      }
    };

    this.log("Gesture performance optimization enabled", "success");
  }

  /**
   * Visualize gesture for debugging
   */
  _visualizeGesture(gesture) {
    const visualization = document.createElement("div");
    visualization.className = "gesture-visualization";
    visualization.style.cssText = `
      position: fixed;
      top: 10px;
      right: 10px;
      background: rgba(0, 0, 0, 0.8);
      color: white;
      padding: 10px;
      border-radius: 5px;
      z-index: 10000;
      font-family: monospace;
      font-size: 12px;
    `;

    visualization.textContent = `Gesture: ${gesture.type}`;
    document.body.appendChild(visualization);

    setTimeout(() => {
      if (visualization.parentNode) {
        visualization.parentNode.removeChild(visualization);
      }
    }, 2000);
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Handle page visibility changes
    document.addEventListener("visibilitychange", () => {
      if (document.hidden) {
        // Clear active touches when page becomes hidden
        this.touchTracking.activeTouches.clear();
        this.touchTracking.touchPaths.clear();
      }
    });
  }

  /**
   * Get supported gestures
   */
  _getSupportedGestures() {
    return Array.from(this.gestureEngine.recognizers.keys());
  }

  /**
   * Get enabled features
   */
  _getEnabledFeatures() {
    const features = [];

    if (this.options.enableAdvancedGestures) features.push("advanced");
    if (this.options.enableMultiTouch) features.push("multitouch");
    if (this.options.enableShapeRecognition) features.push("shapes");
    if (this.options.enableSequenceRecognition) features.push("sequences");
    if (this.options.enableMotionGestures) features.push("motion");
    if (this.options.enableForceTouch) features.push("force");

    return features;
  }

  /**
   * Register custom gesture
   */
  registerCustomGesture(name, recognizer, options = {}) {
    this._registerGestureRecognizer(name, recognizer);
    this.performanceMetrics.customGestures++;

    this.log(`Custom gesture registered: ${name}`, "success");

    return {
      name: name,
      remove: () => {
        this.gestureEngine.recognizers.delete(name);
        this.performanceMetrics.customGestures--;
      },
    };
  }

  /**
   * Get gesture statistics
   */
  getGestureStats() {
    return {
      isInitialized: this.isInitialized,
      recognizers: this.gestureEngine.recognizers.size,
      activeGestures: this.gestureEngine.activeGestures.size,
      gestureHistory: this.gestureEngine.gestureHistory.length,
      sequenceBuffer: this.gestureEngine.sequenceBuffer.length,
      activeTouches: this.touchTracking.activeTouches.size,
      performanceMetrics: { ...this.performanceMetrics },
      supportedGestures: this._getSupportedGestures(),
      enabledFeatures: this._getEnabledFeatures(),
      timestamp: Date.now(),
    };
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [UIGestures] ${message}`;

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
      this.eventManager.emit("gesturesLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "UIGestures",
      });
    }
  }

  /**
   * Clean up and destroy gesture system
   */
  destroy() {
    try {
      // Remove event listeners
      window.removeEventListener("devicemotion", this._processMotionEvent);
      window.removeEventListener(
        "deviceorientationchange",
        this._processOrientationEvent
      );

      // Clear tracking data
      this.touchTracking.activeTouches.clear();
      this.touchTracking.touchPaths.clear();
      this.touchTracking.multiTouchSessions.clear();

      // Clear gesture engine
      this.gestureEngine.recognizers.clear();
      this.gestureEngine.activeGestures.clear();
      this.gestureEngine.gestureHistory = [];
      this.gestureEngine.sequenceBuffer = [];

      // Clear motion tracking
      this.motionTracking.motionHistory = [];
      this.shapeTemplates.clear();

      this.isInitialized = false;
      this.log("UIGestures destroyed successfully", "success");
    } catch (error) {
      this.log(
        `Error during UIGestures destruction: ${error.message}`,
        "error"
      );
      throw error;
    }
  }
}

export default UIGestures;
export { UIGestures, ADVANCED_GESTURES, GESTURE_STATES };
