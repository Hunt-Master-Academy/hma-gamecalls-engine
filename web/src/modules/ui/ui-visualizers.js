/**
 * @file ui-visualizers.js
 * @brief Advanced Data Visualization and Visual Feedback System
 *
 * This module provides comprehensive visualization capabilities including
 * waveform displays, real-time audio analysis, interactive charts, progress
 * indicators, and advanced visual feedback for the Huntmaster engine.
 *
 * @author Huntmaster Engine Team
 * @version 1.0 - Visualization System Implementation
 * @date July 24, 2025
 */

/**
 * Visualization types
 */
const VISUALIZATION_TYPES = {
  WAVEFORM: "waveform",
  SPECTROGRAM: "spectrogram",
  FREQUENCY: "frequency",
  PROGRESS: "progress",
  CIRCULAR_PROGRESS: "circular-progress",
  CHART: "chart",
  HEATMAP: "heatmap",
  PARTICLE: "particle",
  ANIMATED_ICON: "animated-icon",
};

/**
 * Chart types
 */
const CHART_TYPES = {
  LINE: "line",
  BAR: "bar",
  AREA: "area",
  SCATTER: "scatter",
  PIE: "pie",
  DONUT: "donut",
};

/**
 * Animation types
 */
const ANIMATION_TYPES = {
  FADE: "fade",
  SLIDE: "slide",
  SCALE: "scale",
  ROTATE: "rotate",
  PULSE: "pulse",
  BOUNCE: "bounce",
  ELASTIC: "elastic",
};

/**
 * @class UIVisualizers
 * @brief Comprehensive visualization and visual feedback system
 *
 * Features:
 * • Real-time waveform visualization with multiple display modes
 * • Advanced spectrogram and frequency analysis displays
 * • Interactive progress indicators and loading animations
 * • Dynamic chart generation with various chart types
 * • Particle systems for enhanced visual effects
 * • Animated icons and visual feedback elements
 * • Performance-optimized Canvas and WebGL rendering
 * • Customizable color schemes and themes
 * • Accessibility-compliant visual designs
 * • Export capabilities for visualizations
 */
export class UIVisualizers {
  constructor(eventManager, options = {}) {
    this.eventManager = eventManager;
    this.options = {
      // General visualization settings
      enableVisualizations: options.enableVisualizations !== false,
      defaultColorScheme: options.defaultColorScheme || "default",
      enableAnimations: options.enableAnimations !== false,
      animationDuration: options.animationDuration || 300,

      // Canvas settings
      enableCanvasRendering: options.enableCanvasRendering !== false,
      enableWebGL: options.enableWebGL !== false,
      canvasScale: options.canvasScale || window.devicePixelRatio || 1,

      // Waveform settings
      waveformHeight: options.waveformHeight || 200,
      waveformColor: options.waveformColor || "#4A90E2",
      waveformBackground: options.waveformBackground || "#1a1a1a",
      waveformLineWidth: options.waveformLineWidth || 2,

      // Spectrogram settings
      spectrogramHeight: options.spectrogramHeight || 300,
      spectrogramColors: options.spectrogramColors || [
        "#000080",
        "#0000FF",
        "#00FFFF",
        "#FFFF00",
        "#FF0000",
      ],

      // Progress settings
      progressBarHeight: options.progressBarHeight || 4,
      circularProgressSize: options.circularProgressSize || 40,
      progressColor: options.progressColor || "#4A90E2",

      // Chart settings
      chartPadding: options.chartPadding || 20,
      chartGridColor: options.chartGridColor || "#333333",
      chartTextColor: options.chartTextColor || "#ffffff",

      // Performance settings
      maxFPS: options.maxFPS || 60,
      enablePerformanceMode: options.enablePerformanceMode || false,
      reducedMotion: options.reducedMotion || false,

      // Export settings
      enableExport: options.enableExport !== false,
      exportFormat: options.exportFormat || "png",

      // Debug settings
      debugMode: options.debugMode || false,
      showPerformanceStats: options.showPerformanceStats || false,

      ...options,
    };

    // Visualization components
    this.visualizers = new Map();
    this.activeAnimations = new Map();
    this.canvasContexts = new Map();

    // Color schemes
    this.colorSchemes = new Map();
    this._initializeColorSchemes();

    // Performance tracking
    this.performanceMetrics = {
      frameRate: 0,
      renderTime: 0,
      droppedFrames: 0,
      activeVisualizations: 0,
      memoryUsage: 0,
    };

    // Animation control
    this.animationState = {
      isRunning: false,
      requestId: null,
      lastFrameTime: 0,
      deltaTime: 0,
    };

    this.isInitialized = false;
    this._initializeVisualizers();
  }

  /**
   * Initialize visualization system
   */
  _initializeVisualizers() {
    try {
      // Initialize color schemes
      this._setupColorSchemes();

      // Set up canvas contexts
      this._setupCanvasSupport();

      // Initialize built-in visualizers
      this._initializeBuiltInVisualizers();

      // Set up animation system
      this._setupAnimationSystem();

      // Set up performance monitoring
      this._setupPerformanceMonitoring();

      // Set up event listeners
      this._setupEventListeners();

      this.isInitialized = true;
      this.log("UIVisualizers initialized successfully", "success");

      // Emit initialization event
      this.eventManager.emit("visualizersInitialized", {
        supportedTypes: this._getSupportedTypes(),
        enabledFeatures: this._getEnabledFeatures(),
        colorSchemes: Array.from(this.colorSchemes.keys()),
        timestamp: Date.now(),
      });
    } catch (error) {
      this.log(`Visualizers initialization failed: ${error.message}`, "error");
      throw error;
    }
  }

  /**
   * Initialize color schemes
   */
  _initializeColorSchemes() {
    // Default color scheme
    this.colorSchemes.set("default", {
      primary: "#4A90E2",
      secondary: "#50C878",
      accent: "#FF6B6B",
      background: "#1a1a1a",
      surface: "#2a2a2a",
      text: "#ffffff",
      textSecondary: "#cccccc",
      grid: "#333333",
      waveform: "#4A90E2",
      frequency: "#50C878",
      spectrogram: ["#000080", "#0000FF", "#00FFFF", "#FFFF00", "#FF0000"],
    });

    // Dark theme
    this.colorSchemes.set("dark", {
      primary: "#BB86FC",
      secondary: "#03DAC6",
      accent: "#CF6679",
      background: "#121212",
      surface: "#1F1F1F",
      text: "#FFFFFF",
      textSecondary: "#AAAAAA",
      grid: "#333333",
      waveform: "#BB86FC",
      frequency: "#03DAC6",
      spectrogram: ["#1A1A2E", "#16213E", "#0F3460", "#533483", "#E94560"],
    });

    // Light theme
    this.colorSchemes.set("light", {
      primary: "#1976D2",
      secondary: "#388E3C",
      accent: "#D32F2F",
      background: "#FFFFFF",
      surface: "#F5F5F5",
      text: "#212121",
      textSecondary: "#757575",
      grid: "#E0E0E0",
      waveform: "#1976D2",
      frequency: "#388E3C",
      spectrogram: ["#E3F2FD", "#BBDEFB", "#90CAF9", "#64B5F6", "#42A5F5"],
    });

    // High contrast theme
    this.colorSchemes.set("high-contrast", {
      primary: "#FFFF00",
      secondary: "#00FF00",
      accent: "#FF0000",
      background: "#000000",
      surface: "#000000",
      text: "#FFFFFF",
      textSecondary: "#FFFFFF",
      grid: "#FFFFFF",
      waveform: "#FFFF00",
      frequency: "#00FF00",
      spectrogram: ["#000000", "#333333", "#666666", "#999999", "#FFFFFF"],
    });
  }

  /**
   * Set up color schemes
   */
  _setupColorSchemes() {
    const currentScheme =
      this.colorSchemes.get(this.options.defaultColorScheme) ||
      this.colorSchemes.get("default");

    // Apply CSS custom properties for current scheme
    this._applyCSSColorScheme(currentScheme);
  }

  /**
   * Apply CSS color scheme
   */
  _applyCSSColorScheme(scheme) {
    const root = document.documentElement;

    Object.entries(scheme).forEach(([key, value]) => {
      if (Array.isArray(value)) {
        // Handle array values (like spectrogram colors)
        value.forEach((color, index) => {
          root.style.setProperty(`--viz-${key}-${index}`, color);
        });
      } else {
        root.style.setProperty(`--viz-${key}`, value);
      }
    });
  }

  /**
   * Set up canvas support
   */
  _setupCanvasSupport() {
    if (!this.options.enableCanvasRendering) return;

    // Check for WebGL support
    const testCanvas = document.createElement("canvas");
    const webglSupported = !!(
      testCanvas.getContext("webgl") ||
      testCanvas.getContext("experimental-webgl")
    );

    if (this.options.enableWebGL && !webglSupported) {
      this.log("WebGL not supported, falling back to 2D canvas", "warning");
      this.options.enableWebGL = false;
    }

    this.log(
      `Canvas support initialized (WebGL: ${this.options.enableWebGL})`,
      "success"
    );
  }

  /**
   * Initialize built-in visualizers
   */
  _initializeBuiltInVisualizers() {
    // Register built-in visualizer types
    this._registerVisualizer(
      VISUALIZATION_TYPES.WAVEFORM,
      this._createWaveformVisualizer.bind(this)
    );
    this._registerVisualizer(
      VISUALIZATION_TYPES.SPECTROGRAM,
      this._createSpectrogramVisualizer.bind(this)
    );
    this._registerVisualizer(
      VISUALIZATION_TYPES.FREQUENCY,
      this._createFrequencyVisualizer.bind(this)
    );
    this._registerVisualizer(
      VISUALIZATION_TYPES.PROGRESS,
      this._createProgressVisualizer.bind(this)
    );
    this._registerVisualizer(
      VISUALIZATION_TYPES.CIRCULAR_PROGRESS,
      this._createCircularProgressVisualizer.bind(this)
    );
    this._registerVisualizer(
      VISUALIZATION_TYPES.CHART,
      this._createChartVisualizer.bind(this)
    );

    this.log("Built-in visualizers registered", "success");
  }

  /**
   * Register visualizer type
   */
  _registerVisualizer(type, creator) {
    this.visualizers.set(type, creator);
  }

  /**
   * Create waveform visualizer
   */
  _createWaveformVisualizer(container, options = {}) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    // Set up canvas
    const width = options.width || container.clientWidth || 800;
    const height = options.height || this.options.waveformHeight;

    canvas.width = width * this.options.canvasScale;
    canvas.height = height * this.options.canvasScale;
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;

    ctx.scale(this.options.canvasScale, this.options.canvasScale);

    container.appendChild(canvas);

    const visualizer = {
      type: VISUALIZATION_TYPES.WAVEFORM,
      canvas: canvas,
      context: ctx,
      width: width,
      height: height,
      data: null,
      options: {
        color: options.color || this.options.waveformColor,
        backgroundColor:
          options.backgroundColor || this.options.waveformBackground,
        lineWidth: options.lineWidth || this.options.waveformLineWidth,
        ...options,
      },

      update: (audioData) => {
        if (!audioData) return;

        visualizer.data = audioData;
        this._renderWaveform(visualizer);
      },

      destroy: () => {
        if (canvas.parentNode) {
          canvas.parentNode.removeChild(canvas);
        }
      },
    };

    return visualizer;
  }

  /**
   * Render waveform visualization
   */
  _renderWaveform(visualizer) {
    const { context: ctx, width, height, data, options } = visualizer;

    // Clear canvas
    ctx.fillStyle = options.backgroundColor;
    ctx.fillRect(0, 0, width, height);

    if (!data || data.length === 0) return;

    // Set up drawing style
    ctx.strokeStyle = options.color;
    ctx.lineWidth = options.lineWidth;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";

    // Draw waveform
    ctx.beginPath();

    const samples = data.length;
    const step = width / samples;
    const centerY = height / 2;
    const amplitude = height / 2 - 10; // Leave some padding

    for (let i = 0; i < samples; i++) {
      const x = i * step;
      const y = centerY + data[i] * amplitude;

      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }

    ctx.stroke();

    // Add center line
    ctx.strokeStyle = options.color + "40"; // Semi-transparent
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, centerY);
    ctx.lineTo(width, centerY);
    ctx.stroke();
  }

  /**
   * Create spectrogram visualizer
   */
  _createSpectrogramVisualizer(container, options = {}) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    const width = options.width || container.clientWidth || 800;
    const height = options.height || this.options.spectrogramHeight;

    canvas.width = width * this.options.canvasScale;
    canvas.height = height * this.options.canvasScale;
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;

    ctx.scale(this.options.canvasScale, this.options.canvasScale);

    container.appendChild(canvas);

    const visualizer = {
      type: VISUALIZATION_TYPES.SPECTROGRAM,
      canvas: canvas,
      context: ctx,
      width: width,
      height: height,
      spectrogramData: [],
      options: {
        colors: options.colors || this.options.spectrogramColors,
        ...options,
      },

      update: (frequencyData) => {
        if (!frequencyData) return;

        // Add new frequency data column
        visualizer.spectrogramData.push([...frequencyData]);

        // Limit history
        const maxColumns = Math.floor(width / 2);
        if (visualizer.spectrogramData.length > maxColumns) {
          visualizer.spectrogramData.shift();
        }

        this._renderSpectrogram(visualizer);
      },

      destroy: () => {
        if (canvas.parentNode) {
          canvas.parentNode.removeChild(canvas);
        }
      },
    };

    return visualizer;
  }

  /**
   * Render spectrogram visualization
   */
  _renderSpectrogram(visualizer) {
    const {
      context: ctx,
      width,
      height,
      spectrogramData,
      options,
    } = visualizer;

    if (!spectrogramData || spectrogramData.length === 0) return;

    // Clear canvas
    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, width, height);

    const columnWidth = width / spectrogramData.length;
    const colors = options.colors;

    spectrogramData.forEach((column, x) => {
      const pixelHeight = height / column.length;

      column.forEach((value, y) => {
        // Map value to color
        const colorIndex = Math.floor(value * (colors.length - 1));
        const color =
          colors[Math.max(0, Math.min(colorIndex, colors.length - 1))];

        ctx.fillStyle = color;
        ctx.fillRect(
          x * columnWidth,
          height - (y + 1) * pixelHeight,
          columnWidth,
          pixelHeight
        );
      });
    });
  }

  /**
   * Create frequency analyzer visualizer
   */
  _createFrequencyVisualizer(container, options = {}) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    const width = options.width || container.clientWidth || 800;
    const height = options.height || 200;

    canvas.width = width * this.options.canvasScale;
    canvas.height = height * this.options.canvasScale;
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;

    ctx.scale(this.options.canvasScale, this.options.canvasScale);

    container.appendChild(canvas);

    const visualizer = {
      type: VISUALIZATION_TYPES.FREQUENCY,
      canvas: canvas,
      context: ctx,
      width: width,
      height: height,
      options: {
        barColor: options.barColor || "#50C878",
        backgroundColor: options.backgroundColor || "#1a1a1a",
        ...options,
      },

      update: (frequencyData) => {
        if (!frequencyData) return;
        this._renderFrequencyBars(visualizer, frequencyData);
      },

      destroy: () => {
        if (canvas.parentNode) {
          canvas.parentNode.removeChild(canvas);
        }
      },
    };

    return visualizer;
  }

  /**
   * Render frequency bars
   */
  _renderFrequencyBars(visualizer, frequencyData) {
    const { context: ctx, width, height, options } = visualizer;

    // Clear canvas
    ctx.fillStyle = options.backgroundColor;
    ctx.fillRect(0, 0, width, height);

    if (!frequencyData || frequencyData.length === 0) return;

    const barWidth = width / frequencyData.length;
    const maxHeight = height - 20; // Leave some padding

    frequencyData.forEach((value, index) => {
      const barHeight = value * maxHeight;
      const x = index * barWidth;
      const y = height - barHeight;

      // Create gradient
      const gradient = ctx.createLinearGradient(0, y, 0, height);
      gradient.addColorStop(0, options.barColor);
      gradient.addColorStop(1, options.barColor + "80");

      ctx.fillStyle = gradient;
      ctx.fillRect(x, y, barWidth - 1, barHeight);
    });
  }

  /**
   * Create progress visualizer
   */
  _createProgressVisualizer(container, options = {}) {
    const progressContainer = document.createElement("div");
    progressContainer.className = "progress-visualizer";
    progressContainer.style.cssText = `
      width: 100%;
      height: ${options.height || this.options.progressBarHeight}px;
      background-color: var(--viz-surface);
      border-radius: ${
        (options.height || this.options.progressBarHeight) / 2
      }px;
      overflow: hidden;
      position: relative;
    `;

    const progressBar = document.createElement("div");
    progressBar.className = "progress-bar";
    progressBar.style.cssText = `
      height: 100%;
      width: 0%;
      background-color: ${options.color || this.options.progressColor};
      transition: width 0.3s ease;
      border-radius: inherit;
    `;

    const progressText = document.createElement("span");
    progressText.className = "progress-text";
    progressText.style.cssText = `
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      color: var(--viz-text);
      font-size: 12px;
      font-weight: bold;
    `;

    progressContainer.appendChild(progressBar);
    if (options.showText !== false) {
      progressContainer.appendChild(progressText);
    }
    container.appendChild(progressContainer);

    const visualizer = {
      type: VISUALIZATION_TYPES.PROGRESS,
      container: progressContainer,
      bar: progressBar,
      text: progressText,
      options: options,

      update: (progress, text) => {
        const percentage = Math.max(0, Math.min(100, progress * 100));
        progressBar.style.width = `${percentage}%`;

        if (progressText && text !== undefined) {
          progressText.textContent = text;
        } else if (progressText) {
          progressText.textContent = `${Math.round(percentage)}%`;
        }
      },

      destroy: () => {
        if (progressContainer.parentNode) {
          progressContainer.parentNode.removeChild(progressContainer);
        }
      },
    };

    return visualizer;
  }

  /**
   * Create circular progress visualizer
   */
  _createCircularProgressVisualizer(container, options = {}) {
    const size = options.size || this.options.circularProgressSize;
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    canvas.width = size * this.options.canvasScale;
    canvas.height = size * this.options.canvasScale;
    canvas.style.width = `${size}px`;
    canvas.style.height = `${size}px`;

    ctx.scale(this.options.canvasScale, this.options.canvasScale);

    container.appendChild(canvas);

    const visualizer = {
      type: VISUALIZATION_TYPES.CIRCULAR_PROGRESS,
      canvas: canvas,
      context: ctx,
      size: size,
      options: {
        color: options.color || this.options.progressColor,
        backgroundColor: options.backgroundColor || "rgba(255,255,255,0.1)",
        lineWidth: options.lineWidth || 4,
        ...options,
      },

      update: (progress) => {
        this._renderCircularProgress(visualizer, progress);
      },

      destroy: () => {
        if (canvas.parentNode) {
          canvas.parentNode.removeChild(canvas);
        }
      },
    };

    return visualizer;
  }

  /**
   * Render circular progress
   */
  _renderCircularProgress(visualizer, progress) {
    const { context: ctx, size, options } = visualizer;
    const centerX = size / 2;
    const centerY = size / 2;
    const radius = (size - options.lineWidth) / 2;

    // Clear canvas
    ctx.clearRect(0, 0, size, size);

    // Draw background circle
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    ctx.strokeStyle = options.backgroundColor;
    ctx.lineWidth = options.lineWidth;
    ctx.stroke();

    // Draw progress arc
    if (progress > 0) {
      const angle = progress * 2 * Math.PI - Math.PI / 2; // Start from top

      ctx.beginPath();
      ctx.arc(centerX, centerY, radius, -Math.PI / 2, angle);
      ctx.strokeStyle = options.color;
      ctx.lineWidth = options.lineWidth;
      ctx.lineCap = "round";
      ctx.stroke();
    }
  }

  /**
   * Create chart visualizer
   */
  _createChartVisualizer(container, options = {}) {
    const canvas = document.createElement("canvas");
    const ctx = canvas.getContext("2d");

    const width = options.width || container.clientWidth || 400;
    const height = options.height || 300;

    canvas.width = width * this.options.canvasScale;
    canvas.height = height * this.options.canvasScale;
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;

    ctx.scale(this.options.canvasScale, this.options.canvasScale);

    container.appendChild(canvas);

    const visualizer = {
      type: VISUALIZATION_TYPES.CHART,
      canvas: canvas,
      context: ctx,
      width: width,
      height: height,
      data: null,
      options: {
        chartType: options.chartType || CHART_TYPES.LINE,
        padding: options.padding || this.options.chartPadding,
        gridColor: options.gridColor || this.options.chartGridColor,
        textColor: options.textColor || this.options.chartTextColor,
        ...options,
      },

      update: (data) => {
        visualizer.data = data;
        this._renderChart(visualizer);
      },

      destroy: () => {
        if (canvas.parentNode) {
          canvas.parentNode.removeChild(canvas);
        }
      },
    };

    return visualizer;
  }

  /**
   * Render chart
   */
  _renderChart(visualizer) {
    const { context: ctx, width, height, data, options } = visualizer;

    if (!data || !data.series || data.series.length === 0) return;

    // Clear canvas
    ctx.fillStyle = "rgba(0,0,0,0)";
    ctx.clearRect(0, 0, width, height);

    const padding = options.padding;
    const chartWidth = width - padding * 2;
    const chartHeight = height - padding * 2;

    // Draw grid
    this._drawChartGrid(ctx, padding, chartWidth, chartHeight, options);

    // Draw data based on chart type
    switch (options.chartType) {
      case CHART_TYPES.LINE:
        this._drawLineChart(
          ctx,
          data,
          padding,
          chartWidth,
          chartHeight,
          options
        );
        break;
      case CHART_TYPES.BAR:
        this._drawBarChart(
          ctx,
          data,
          padding,
          chartWidth,
          chartHeight,
          options
        );
        break;
      case CHART_TYPES.AREA:
        this._drawAreaChart(
          ctx,
          data,
          padding,
          chartWidth,
          chartHeight,
          options
        );
        break;
    }
  }

  /**
   * Draw chart grid
   */
  _drawChartGrid(ctx, padding, width, height, options) {
    ctx.strokeStyle = options.gridColor;
    ctx.lineWidth = 1;
    ctx.setLineDash([2, 2]);

    // Vertical lines
    for (let i = 0; i <= 10; i++) {
      const x = padding + (i * width) / 10;
      ctx.beginPath();
      ctx.moveTo(x, padding);
      ctx.lineTo(x, padding + height);
      ctx.stroke();
    }

    // Horizontal lines
    for (let i = 0; i <= 5; i++) {
      const y = padding + (i * height) / 5;
      ctx.beginPath();
      ctx.moveTo(padding, y);
      ctx.lineTo(padding + width, y);
      ctx.stroke();
    }

    ctx.setLineDash([]);
  }

  /**
   * Draw line chart
   */
  _drawLineChart(ctx, data, padding, width, height, options) {
    data.series.forEach((series, seriesIndex) => {
      if (!series.data || series.data.length === 0) return;

      ctx.strokeStyle = series.color || `hsl(${seriesIndex * 60}, 70%, 50%)`;
      ctx.lineWidth = 2;
      ctx.beginPath();

      const step = width / (series.data.length - 1);
      const maxValue = Math.max(...data.series.map((s) => Math.max(...s.data)));

      series.data.forEach((value, index) => {
        const x = padding + index * step;
        const y = padding + height - (value / maxValue) * height;

        if (index === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });

      ctx.stroke();
    });
  }

  /**
   * Set up animation system
   */
  _setupAnimationSystem() {
    if (!this.options.enableAnimations) return;

    const animate = (currentTime) => {
      if (!this.animationState.isRunning) return;

      // Calculate delta time
      if (this.animationState.lastFrameTime === 0) {
        this.animationState.lastFrameTime = currentTime;
      }

      this.animationState.deltaTime =
        currentTime - this.animationState.lastFrameTime;
      this.animationState.lastFrameTime = currentTime;

      // Update performance metrics
      this.performanceMetrics.frameRate = 1000 / this.animationState.deltaTime;

      // Update active animations
      this._updateAnimations(currentTime);

      // Continue animation loop
      this.animationState.requestId = requestAnimationFrame(animate);
    };

    // Start animation loop
    this.animationState.isRunning = true;
    this.animationState.requestId = requestAnimationFrame(animate);

    this.log("Animation system initialized", "success");
  }

  /**
   * Update active animations
   */
  _updateAnimations(currentTime) {
    this.activeAnimations.forEach((animation, id) => {
      const elapsed = currentTime - animation.startTime;
      const progress = Math.min(elapsed / animation.duration, 1);

      // Apply easing function
      const easedProgress = this._applyEasing(progress, animation.easing);

      // Update animation values
      animation.update(easedProgress);

      // Remove completed animations
      if (progress >= 1) {
        if (animation.onComplete) {
          animation.onComplete();
        }
        this.activeAnimations.delete(id);
      }
    });
  }

  /**
   * Apply easing function
   */
  _applyEasing(t, easing = "ease") {
    switch (easing) {
      case "linear":
        return t;
      case "ease-in":
        return t * t;
      case "ease-out":
        return 1 - Math.pow(1 - t, 2);
      case "ease-in-out":
        return t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
      case "bounce":
        if (t < 1 / 2.75) {
          return 7.5625 * t * t;
        } else if (t < 2 / 2.75) {
          return 7.5625 * (t -= 1.5 / 2.75) * t + 0.75;
        } else if (t < 2.5 / 2.75) {
          return 7.5625 * (t -= 2.25 / 2.75) * t + 0.9375;
        } else {
          return 7.5625 * (t -= 2.625 / 2.75) * t + 0.984375;
        }
      default:
        return t;
    }
  }

  /**
   * Set up performance monitoring
   */
  _setupPerformanceMonitoring() {
    if (!this.options.showPerformanceStats) return;

    setInterval(() => {
      this.performanceMetrics.activeVisualizations = this.visualizers.size;
      this.performanceMetrics.memoryUsage = performance.memory
        ? performance.memory.usedJSHeapSize / 1024 / 1024
        : 0;

      if (this.options.debugMode) {
        this.log(
          `Performance - FPS: ${this.performanceMetrics.frameRate.toFixed(
            1
          )}, ` +
            `Active: ${this.performanceMetrics.activeVisualizations}, ` +
            `Memory: ${this.performanceMetrics.memoryUsage.toFixed(2)}MB`,
          "info"
        );
      }
    }, 1000);
  }

  /**
   * Set up event listeners
   */
  _setupEventListeners() {
    // Handle visibility changes
    document.addEventListener("visibilitychange", () => {
      if (document.hidden) {
        this._pauseAnimations();
      } else {
        this._resumeAnimations();
      }
    });

    // Handle reduced motion preference
    if (window.matchMedia) {
      const mediaQuery = window.matchMedia("(prefers-reduced-motion: reduce)");
      const handleReducedMotion = (e) => {
        this.options.reducedMotion = e.matches;
        if (e.matches) {
          this._pauseAnimations();
        }
      };

      mediaQuery.addListener(handleReducedMotion);
      handleReducedMotion(mediaQuery);
    }
  }

  /**
   * Pause animations
   */
  _pauseAnimations() {
    this.animationState.isRunning = false;
    if (this.animationState.requestId) {
      cancelAnimationFrame(this.animationState.requestId);
    }
  }

  /**
   * Resume animations
   */
  _resumeAnimations() {
    if (!this.options.reducedMotion && this.options.enableAnimations) {
      this.animationState.isRunning = true;
      this.animationState.lastFrameTime = 0;
      this.animationState.requestId = requestAnimationFrame(
        this._updateAnimations.bind(this)
      );
    }
  }

  /**
   * Create visualizer instance
   */
  createVisualizer(type, container, options = {}) {
    const creator = this.visualizers.get(type);
    if (!creator) {
      this.log(`Visualizer type not found: ${type}`, "error");
      return null;
    }

    try {
      const visualizer = creator(container, options);
      this.performanceMetrics.activeVisualizations++;

      this.log(`Visualizer created: ${type}`, "success");
      return visualizer;
    } catch (error) {
      this.log(
        `Failed to create visualizer ${type}: ${error.message}`,
        "error"
      );
      return null;
    }
  }

  /**
   * Change color scheme
   */
  setColorScheme(schemeName) {
    const scheme = this.colorSchemes.get(schemeName);
    if (!scheme) {
      this.log(`Color scheme not found: ${schemeName}`, "warning");
      return false;
    }

    this._applyCSSColorScheme(scheme);
    this.options.defaultColorScheme = schemeName;

    this.eventManager.emit("colorSchemeChanged", {
      scheme: schemeName,
      colors: scheme,
      timestamp: Date.now(),
    });

    return true;
  }

  /**
   * Get supported visualization types
   */
  _getSupportedTypes() {
    return Array.from(this.visualizers.keys());
  }

  /**
   * Get enabled features
   */
  _getEnabledFeatures() {
    const features = [];

    if (this.options.enableVisualizations) features.push("visualizations");
    if (this.options.enableAnimations) features.push("animations");
    if (this.options.enableCanvasRendering) features.push("canvas");
    if (this.options.enableWebGL) features.push("webgl");
    if (this.options.enableExport) features.push("export");

    return features;
  }

  /**
   * Get visualization system status
   */
  getVisualizationStatus() {
    return {
      isInitialized: this.isInitialized,
      supportedTypes: this._getSupportedTypes(),
      colorSchemes: Array.from(this.colorSchemes.keys()),
      currentColorScheme: this.options.defaultColorScheme,
      activeAnimations: this.activeAnimations.size,
      performanceMetrics: { ...this.performanceMetrics },
      enabledFeatures: this._getEnabledFeatures(),
      timestamp: Date.now(),
    };
  }

  /**
   * Logging utility
   */
  log(message, level = "info") {
    const timestamp = new Date().toISOString();
    const logMessage = `[${timestamp}] [UIVisualizers] ${message}`;

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
      this.eventManager.emit("visualizersLog", {
        message,
        level,
        timestamp: Date.now(),
        source: "UIVisualizers",
      });
    }
  }

  /**
   * Clean up and destroy visualization system
   */
  destroy() {
    try {
      // Stop animations
      this._pauseAnimations();

      // Clear active animations
      this.activeAnimations.clear();

      // Clear visualizers
      this.visualizers.clear();

      // Clear canvas contexts
      this.canvasContexts.clear();

      // Remove CSS custom properties
      const root = document.documentElement;
      this.colorSchemes.forEach((scheme) => {
        Object.keys(scheme).forEach((key) => {
          if (Array.isArray(scheme[key])) {
            scheme[key].forEach((_, index) => {
              root.style.removeProperty(`--viz-${key}-${index}`);
            });
          } else {
            root.style.removeProperty(`--viz-${key}`);
          }
        });
      });

      this.isInitialized = false;
      this.log("UIVisualizers destroyed successfully", "success");
    } catch (error) {
      this.log(
        `Error during UIVisualizers destruction: ${error.message}`,
        "error"
      );
      throw error;
    }
  }
}

export default UIVisualizers;
export { UIVisualizers, VISUALIZATION_TYPES, CHART_TYPES, ANIMATION_TYPES };
