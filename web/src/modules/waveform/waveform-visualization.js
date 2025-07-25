/**
 * @fileoverview Waveform Visualization Module - Advanced Visualization Rendering
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Provides comprehensive waveform visualization capabilities with advanced
 * rendering techniques, WebGL acceleration, and interactive displays.
 *
 * Key Features:
 * - High-performance Canvas 2D and WebGL rendering
 * - Real-time waveform, spectrogram, and frequency visualization
 * - Customizable themes, colors, and visual styles
 * - Interactive zoom, pan, and navigation controls
 * - Multi-layer visualization with transparency support
 * - Performance-optimized rendering with frame rate control
 *
 * Dependencies:
 * - Canvas API for 2D rendering
 * - WebGL for hardware-accelerated graphics
 * - Animation frame management
 * - Theme management system
 *
 * @example
 * ```javascript
 * import { WaveformVisualization } from './modules/waveform/waveform-visualization.js';
 *
 * const visualizer = new WaveformVisualization({
 *   canvas: canvasElement,
 *   theme: 'dark',
 *   enableWebGL: true
 * });
 *
 * visualizer.renderWaveform(audioData);
 * visualizer.renderSpectrogram(spectrogramData);
 * ```
 */

/**
 * Advanced Waveform Visualization Engine
 *
 * Provides high-performance visualization of audio waveforms, spectrograms,
 * and frequency analysis with WebGL acceleration and customizable themes.
 *
 * @class WaveformVisualization
 */
export class WaveformVisualization {
  /**
   * Create a WaveformVisualization instance
   *
   * @param {Object} options - Configuration options
   * @param {HTMLCanvasElement} options.canvas - Target canvas element
   * @param {string} [options.theme='dark'] - Visual theme
   * @param {boolean} [options.enableWebGL=true] - Enable WebGL rendering
   * @param {number} [options.width=800] - Visualization width
   * @param {number} [options.height=400] - Visualization height
   * @param {Object} [options.colors] - Custom color configuration
   * @param {number} [options.frameRate=60] - Target frame rate
   */
  constructor(options = {}) {
    // Configuration
    this.config = {
      theme: options.theme || "dark",
      enableWebGL: options.enableWebGL !== false,
      width: options.width || 800,
      height: options.height || 400,
      frameRate: options.frameRate || 60,
      colors: options.colors || {},
      ...options,
    };

    // Canvas and rendering contexts
    this.canvas = options.canvas;
    this.ctx2d = null;
    this.webglCtx = null;
    this.activeRenderer = null;

    // Visualization state
    this.state = {
      isRendering: false,
      lastFrameTime: 0,
      frameCount: 0,
      fps: 0,
      zoom: 1.0,
      pan: { x: 0, y: 0 },
      selection: null,
      viewPort: {
        x: 0,
        y: 0,
        width: this.config.width,
        height: this.config.height,
      },
    };

    // Rendering components
    this.renderers = {
      waveform: null,
      spectrogram: null,
      frequency: null,
      overlay: null,
    };

    // Theme and styling
    this.themes = {
      dark: {
        background: "#1a1a1a",
        waveform: "#4a9eff",
        grid: "#333333",
        text: "#ffffff",
        accent: "#ff6b35",
        selection: "rgba(74, 158, 255, 0.3)",
      },
      light: {
        background: "#ffffff",
        waveform: "#2196f3",
        grid: "#e0e0e0",
        text: "#333333",
        accent: "#ff5722",
        selection: "rgba(33, 150, 243, 0.3)",
      },
      neon: {
        background: "#0a0a0a",
        waveform: "#00ff41",
        grid: "#1a2332",
        text: "#00ff41",
        accent: "#ff0080",
        selection: "rgba(0, 255, 65, 0.3)",
      },
    };

    // WebGL shaders
    this.shaders = {
      vertex: {
        waveform: `
          attribute vec2 a_position;
          attribute float a_amplitude;
          uniform vec2 u_resolution;
          uniform float u_zoom;
          uniform vec2 u_pan;
          varying float v_amplitude;

          void main() {
            vec2 position = (a_position + u_pan) * u_zoom;
            vec2 clipSpace = ((position / u_resolution) * 2.0) - 1.0;
            gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);
            v_amplitude = a_amplitude;
          }
        `,
        spectrogram: `
          attribute vec2 a_position;
          attribute vec2 a_texCoord;
          uniform vec2 u_resolution;
          varying vec2 v_texCoord;

          void main() {
            vec2 clipSpace = ((a_position / u_resolution) * 2.0) - 1.0;
            gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);
            v_texCoord = a_texCoord;
          }
        `,
      },
      fragment: {
        waveform: `
          precision mediump float;
          varying float v_amplitude;
          uniform vec3 u_color;
          uniform float u_alpha;

          void main() {
            float intensity = abs(v_amplitude);
            vec3 color = u_color * intensity;
            gl_FragColor = vec4(color, u_alpha);
          }
        `,
        spectrogram: `
          precision mediump float;
          varying vec2 v_texCoord;
          uniform sampler2D u_texture;
          uniform float u_brightness;
          uniform float u_contrast;

          void main() {
            vec4 color = texture2D(u_texture, v_texCoord);
            color.rgb = (color.rgb - 0.5) * u_contrast + 0.5;
            color.rgb *= u_brightness;
            gl_FragColor = color;
          }
        `,
      },
    };

    // Animation frame management
    this.animation = {
      requestId: null,
      isAnimating: false,
      frameCallback: null,
    };

    // Event handlers
    this.eventHandlers = new Map();

    this._initialize();
  }

  /**
   * Initialize visualization components
   * @private
   */
  async _initialize() {
    try {
      // Setup canvas
      this._setupCanvas();

      // Initialize rendering contexts
      await this._initializeRenderingContexts();

      // Initialize renderers
      await this._initializeRenderers();

      // Setup event listeners
      this._setupEventListeners();

      // Apply initial theme
      this._applyTheme(this.config.theme);

      console.log("WaveformVisualization initialized successfully");
    } catch (error) {
      console.error("WaveformVisualization initialization failed:", error);
      this._handleInitializationError(error);
    }
  }

  /**
   * Setup canvas properties
   * @private
   */
  _setupCanvas() {
    if (!this.canvas) {
      throw new Error("Canvas element is required");
    }

    // Set canvas dimensions
    this.canvas.width = this.config.width;
    this.canvas.height = this.config.height;

    // Set CSS dimensions for proper scaling
    this.canvas.style.width = `${this.config.width}px`;
    this.canvas.style.height = `${this.config.height}px`;

    // Configure canvas for high DPI displays
    const devicePixelRatio = window.devicePixelRatio || 1;
    if (devicePixelRatio > 1) {
      this.canvas.width = this.config.width * devicePixelRatio;
      this.canvas.height = this.config.height * devicePixelRatio;
      this.canvas.style.width = `${this.config.width}px`;
      this.canvas.style.height = `${this.config.height}px`;
    }
  }

  /**
   * Initialize rendering contexts
   * @private
   */
  async _initializeRenderingContexts() {
    // Initialize 2D context
    this.ctx2d = this.canvas.getContext("2d");
    if (!this.ctx2d) {
      throw new Error("Failed to get 2D rendering context");
    }

    // Configure 2D context
    this.ctx2d.imageSmoothingEnabled = true;
    this.ctx2d.imageSmoothingQuality = "high";

    // Initialize WebGL context if enabled
    if (this.config.enableWebGL) {
      try {
        this.webglCtx =
          this.canvas.getContext("webgl") ||
          this.canvas.getContext("experimental-webgl");

        if (this.webglCtx) {
          await this._setupWebGL();
          this.activeRenderer = "webgl";
          console.log("WebGL rendering enabled");
        } else {
          console.warn("WebGL not available, falling back to 2D canvas");
          this.activeRenderer = "2d";
        }
      } catch (error) {
        console.warn("WebGL initialization failed, using 2D canvas:", error);
        this.activeRenderer = "2d";
      }
    } else {
      this.activeRenderer = "2d";
    }
  }

  /**
   * Setup WebGL context and shaders
   * @private
   */
  async _setupWebGL() {
    const gl = this.webglCtx;

    // Configure WebGL settings
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    gl.clearColor(0.0, 0.0, 0.0, 1.0);

    // Compile shaders
    this.shaderPrograms = {
      waveform: await this._createShaderProgram(
        this.shaders.vertex.waveform,
        this.shaders.fragment.waveform
      ),
      spectrogram: await this._createShaderProgram(
        this.shaders.vertex.spectrogram,
        this.shaders.fragment.spectrogram
      ),
    };

    // Create buffers
    this._createWebGLBuffers();
  }

  /**
   * Create WebGL shader program
   * @private
   */
  async _createShaderProgram(vertexSource, fragmentSource) {
    const gl = this.webglCtx;

    const vertexShader = this._compileShader(gl.VERTEX_SHADER, vertexSource);
    const fragmentShader = this._compileShader(
      gl.FRAGMENT_SHADER,
      fragmentSource
    );

    const program = gl.createProgram();
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);

    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      throw new Error(
        "Shader program failed to link: " + gl.getProgramInfoLog(program)
      );
    }

    return program;
  }

  /**
   * Compile WebGL shader
   * @private
   */
  _compileShader(type, source) {
    const gl = this.webglCtx;
    const shader = gl.createShader(type);

    gl.shaderSource(shader, source);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      throw new Error(
        "Shader compilation failed: " + gl.getShaderInfoLog(shader)
      );
    }

    return shader;
  }

  /**
   * Create WebGL buffers
   * @private
   */
  _createWebGLBuffers() {
    const gl = this.webglCtx;

    this.buffers = {
      position: gl.createBuffer(),
      amplitude: gl.createBuffer(),
      texCoord: gl.createBuffer(),
      index: gl.createBuffer(),
    };
  }

  /**
   * Initialize specialized renderers
   * @private
   */
  async _initializeRenderers() {
    // Waveform renderer
    this.renderers.waveform = {
      render: (audioData, options = {}) => {
        if (this.activeRenderer === "webgl") {
          return this._renderWaveformWebGL(audioData, options);
        } else {
          return this._renderWaveform2D(audioData, options);
        }
      },
    };

    // Spectrogram renderer
    this.renderers.spectrogram = {
      render: (spectrogramData, options = {}) => {
        if (this.activeRenderer === "webgl") {
          return this._renderSpectrogramWebGL(spectrogramData, options);
        } else {
          return this._renderSpectrogram2D(spectrogramData, options);
        }
      },
    };

    // Frequency spectrum renderer
    this.renderers.frequency = {
      render: (frequencyData, options = {}) => {
        return this._renderFrequencySpectrum(frequencyData, options);
      },
    };

    // Overlay renderer for UI elements
    this.renderers.overlay = {
      render: (overlayData, options = {}) => {
        return this._renderOverlay(overlayData, options);
      },
    };
  }

  /**
   * Render waveform visualization
   *
   * @param {Float32Array} audioData - Audio waveform data
   * @param {Object} [options={}] - Rendering options
   * @returns {Promise<void>}
   */
  async renderWaveform(audioData, options = {}) {
    if (!audioData || audioData.length === 0) {
      return;
    }

    try {
      this.state.isRendering = true;

      // Clear canvas
      this._clearCanvas();

      // Apply zoom and pan transformations
      this._applyTransformations();

      // Render waveform
      await this.renderers.waveform.render(audioData, {
        color: this._getCurrentTheme().waveform,
        lineWidth: options.lineWidth || 2,
        fillMode: options.fillMode || false,
        ...options,
      });

      // Render grid if enabled
      if (options.showGrid !== false) {
        this._renderGrid();
      }

      // Render selection if present
      if (this.state.selection) {
        this._renderSelection();
      }

      // Update frame count
      this._updateFrameStats();
    } catch (error) {
      console.error("Waveform rendering failed:", error);
    } finally {
      this.state.isRendering = false;
    }
  }

  /**
   * Render spectrogram visualization
   *
   * @param {Array} spectrogramData - Spectrogram data matrix
   * @param {Object} [options={}] - Rendering options
   * @returns {Promise<void>}
   */
  async renderSpectrogram(spectrogramData, options = {}) {
    if (!spectrogramData || spectrogramData.length === 0) {
      return;
    }

    try {
      this.state.isRendering = true;

      // Clear canvas
      this._clearCanvas();

      // Render spectrogram
      await this.renderers.spectrogram.render(spectrogramData, {
        colormap: options.colormap || "viridis",
        brightness: options.brightness || 1.0,
        contrast: options.contrast || 1.0,
        ...options,
      });

      // Render frequency axis labels
      if (options.showFrequencyAxis !== false) {
        this._renderFrequencyAxis();
      }

      // Render time axis labels
      if (options.showTimeAxis !== false) {
        this._renderTimeAxis();
      }

      this._updateFrameStats();
    } catch (error) {
      console.error("Spectrogram rendering failed:", error);
    } finally {
      this.state.isRendering = false;
    }
  }

  /**
   * Render frequency spectrum
   *
   * @param {Object} frequencyData - Frequency spectrum data
   * @param {Object} [options={}] - Rendering options
   */
  renderFrequencySpectrum(frequencyData, options = {}) {
    if (!frequencyData?.magnitude) {
      return;
    }

    try {
      this.state.isRendering = true;

      this._clearCanvas();
      this.renderers.frequency.render(frequencyData, options);
      this._updateFrameStats();
    } catch (error) {
      console.error("Frequency spectrum rendering failed:", error);
    } finally {
      this.state.isRendering = false;
    }
  }

  /**
   * Render waveform using WebGL
   * @private
   */
  _renderWaveformWebGL(audioData, options) {
    const gl = this.webglCtx;
    const program = this.shaderPrograms.waveform;

    gl.useProgram(program);

    // Prepare vertex data
    const positions = new Float32Array(audioData.length * 2);
    const amplitudes = new Float32Array(audioData.length);

    for (let i = 0; i < audioData.length; i++) {
      positions[i * 2] = (i / audioData.length) * this.config.width;
      positions[i * 2 + 1] = this.config.height / 2;
      amplitudes[i] = audioData[i];
    }

    // Upload vertex data
    gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers.position);
    gl.bufferData(gl.ARRAY_BUFFER, positions, gl.DYNAMIC_DRAW);

    gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers.amplitude);
    gl.bufferData(gl.ARRAY_BUFFER, amplitudes, gl.DYNAMIC_DRAW);

    // Set uniforms
    const resolutionLocation = gl.getUniformLocation(program, "u_resolution");
    gl.uniform2f(resolutionLocation, this.config.width, this.config.height);

    const zoomLocation = gl.getUniformLocation(program, "u_zoom");
    gl.uniform1f(zoomLocation, this.state.zoom);

    const panLocation = gl.getUniformLocation(program, "u_pan");
    gl.uniform2f(panLocation, this.state.pan.x, this.state.pan.y);

    // Draw waveform
    gl.drawArrays(gl.LINE_STRIP, 0, audioData.length);
  }

  /**
   * Render waveform using Canvas 2D
   * @private
   */
  _renderWaveform2D(audioData, options) {
    const ctx = this.ctx2d;
    const width = this.config.width;
    const height = this.config.height;
    const centerY = height / 2;

    ctx.save();

    // Set drawing properties
    ctx.strokeStyle = options.color;
    ctx.lineWidth = options.lineWidth || 2;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";

    // Begin path
    ctx.beginPath();

    // Draw waveform
    const samplesPerPixel = audioData.length / width;

    for (let x = 0; x < width; x++) {
      const sampleIndex = Math.floor(x * samplesPerPixel);
      const amplitude = audioData[sampleIndex] || 0;
      const y = centerY + amplitude * centerY * 0.8;

      if (x === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }

    ctx.stroke();

    // Fill mode if enabled
    if (options.fillMode) {
      ctx.lineTo(width, centerY);
      ctx.lineTo(0, centerY);
      ctx.closePath();
      ctx.fillStyle = options.color + "40"; // Add transparency
      ctx.fill();
    }

    ctx.restore();
  }

  /**
   * Clear canvas
   * @private
   */
  _clearCanvas() {
    if (this.activeRenderer === "webgl") {
      const gl = this.webglCtx;
      const theme = this._getCurrentTheme();
      const bgColor = this._hexToRgb(theme.background);
      gl.clearColor(bgColor.r, bgColor.g, bgColor.b, 1.0);
      gl.clear(gl.COLOR_BUFFER_BIT);
    } else {
      const ctx = this.ctx2d;
      ctx.fillStyle = this._getCurrentTheme().background;
      ctx.fillRect(0, 0, this.config.width, this.config.height);
    }
  }

  /**
   * Apply zoom and pan transformations
   * @private
   */
  _applyTransformations() {
    if (this.activeRenderer === "2d") {
      const ctx = this.ctx2d;
      ctx.save();
      ctx.translate(this.state.pan.x, this.state.pan.y);
      ctx.scale(this.state.zoom, this.state.zoom);
    }
  }

  /**
   * Get current theme
   * @private
   */
  _getCurrentTheme() {
    return this.themes[this.config.theme] || this.themes.dark;
  }

  /**
   * Apply theme to visualization
   * @private
   */
  _applyTheme(themeName) {
    this.config.theme = themeName;

    // Emit theme change event
    this._emitEvent("themeChanged", { theme: themeName });
  }

  /**
   * Convert hex color to RGB
   * @private
   */
  _hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result
      ? {
          r: parseInt(result[1], 16) / 255,
          g: parseInt(result[2], 16) / 255,
          b: parseInt(result[3], 16) / 255,
        }
      : { r: 0, g: 0, b: 0 };
  }

  /**
   * Setup event listeners
   * @private
   */
  _setupEventListeners() {
    // Canvas mouse events
    this.canvas.addEventListener("mousedown", this._handleMouseDown.bind(this));
    this.canvas.addEventListener("mousemove", this._handleMouseMove.bind(this));
    this.canvas.addEventListener("mouseup", this._handleMouseUp.bind(this));
    this.canvas.addEventListener("wheel", this._handleWheel.bind(this));

    // Keyboard events
    window.addEventListener("keydown", this._handleKeyDown.bind(this));

    // Resize events
    window.addEventListener("resize", this._handleResize.bind(this));
  }

  /**
   * Handle mouse down events
   * @private
   */
  _handleMouseDown(event) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    this._emitEvent("mouseDown", { x, y, event });
  }

  /**
   * Handle mouse move events
   * @private
   */
  _handleMouseMove(event) {
    const rect = this.canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    this._emitEvent("mouseMove", { x, y, event });
  }

  /**
   * Handle wheel events for zoom
   * @private
   */
  _handleWheel(event) {
    event.preventDefault();

    const zoomFactor = event.deltaY > 0 ? 0.9 : 1.1;
    this.setZoom(this.state.zoom * zoomFactor);

    this._emitEvent("zoom", { zoom: this.state.zoom, event });
  }

  /**
   * Set zoom level
   *
   * @param {number} zoom - Zoom level
   */
  setZoom(zoom) {
    this.state.zoom = Math.max(0.1, Math.min(10, zoom));
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
  }

  /**
   * Update frame statistics
   * @private
   */
  _updateFrameStats() {
    const now = performance.now();
    const deltaTime = now - this.state.lastFrameTime;

    this.state.frameCount++;
    this.state.fps = 1000 / deltaTime;
    this.state.lastFrameTime = now;
  }

  /**
   * Start animation loop
   */
  startAnimation(callback) {
    if (this.animation.isAnimating) {
      return;
    }

    this.animation.isAnimating = true;
    this.animation.frameCallback = callback;

    const animate = () => {
      if (!this.animation.isAnimating) {
        return;
      }

      if (this.animation.frameCallback) {
        this.animation.frameCallback();
      }

      this.animation.requestId = requestAnimationFrame(animate);
    };

    animate();
  }

  /**
   * Stop animation loop
   */
  stopAnimation() {
    this.animation.isAnimating = false;

    if (this.animation.requestId) {
      cancelAnimationFrame(this.animation.requestId);
      this.animation.requestId = null;
    }
  }

  /**
   * Set theme
   *
   * @param {string} themeName - Theme name
   */
  setTheme(themeName) {
    if (this.themes[themeName]) {
      this._applyTheme(themeName);
    }
  }

  /**
   * Get available themes
   *
   * @returns {Array<string>} Available theme names
   */
  getAvailableThemes() {
    return Object.keys(this.themes);
  }

  /**
   * Get performance statistics
   *
   * @returns {Object} Performance stats
   */
  getPerformanceStats() {
    return {
      fps: this.state.fps,
      frameCount: this.state.frameCount,
      isRendering: this.state.isRendering,
      activeRenderer: this.activeRenderer,
    };
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
    this.stopAnimation();
    this.eventHandlers.clear();

    // Clean up WebGL resources
    if (this.webglCtx) {
      Object.values(this.buffers || {}).forEach((buffer) => {
        this.webglCtx.deleteBuffer(buffer);
      });

      Object.values(this.shaderPrograms || {}).forEach((program) => {
        this.webglCtx.deleteProgram(program);
      });
    }
  }
}

export default WaveformVisualization;
