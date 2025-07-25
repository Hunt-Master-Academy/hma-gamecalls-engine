/**
 * SpectrogramVisualizer.js - Advanced 3D Spectrogram Visualization
 *
 * Advanced spectrogram visualization component with WebGL-accelerated 3D waterfall display,
 * real-time frequency analysis, interactive frequency band selection, and export capabilities.
 *
 * Features:
 * - Real-time frequency analysis display with FFT processing
 * - 3D waterfall visualization using WebGL for performance
 * - Interactive frequency band selection and zooming
 * - Configurable color mapping and intensity scaling
 * - Export capabilities (PNG, SVG, raw data)
 * - GPU-accelerated rendering for smooth 60fps performance
 * - Responsive design for mobile and desktop
 * - Accessibility features for screen readers
 *
 * Dependencies: WebGL, Web Audio API, EventManager, PerformanceMonitor
 */

import { EventManager } from "../core/EventManager.js";
import { PerformanceMonitor } from "../core/PerformanceMonitor.js";

export class SpectrogramVisualizer {
  constructor(container, options = {}) {
    this.container = container;
    this.options = {
      // Display configuration
      width: options.width || 800,
      height: options.height || 600,
      fftSize: options.fftSize || 2048,
      smoothingTimeConstant: options.smoothingTimeConstant || 0.8,

      // 3D visualization settings
      waterfallHeight: options.waterfallHeight || 200,
      perspectiveAngle: options.perspectiveAngle || 45,
      rotationSpeed: options.rotationSpeed || 0.5,
      enableAutoRotation: options.enableAutoRotation || false,

      // Color mapping
      colorMap: options.colorMap || "viridis",
      intensityRange: options.intensityRange || [-80, 0], // dB range
      gammaCorrection: options.gammaCorrection || 2.2,

      // Interaction settings
      enableZoom: options.enableZoom !== false,
      enablePan: options.enablePan !== false,
      enableFrequencySelection: options.enableFrequencySelection !== false,

      // Performance settings
      maxFrameRate: options.maxFrameRate || 60,
      enableGPUAcceleration: options.enableGPUAcceleration !== false,

      // Accessibility
      enableScreenReader: options.enableScreenReader || false,
      highContrastMode: options.highContrastMode || false,

      ...options,
    };

    // Initialize state
    this.isInitialized = false;
    this.isPlaying = false;
    this.audioContext = null;
    this.analyserNode = null;
    this.frequencyData = null;
    this.spectrogramData = [];

    // WebGL context and resources
    this.gl = null;
    this.shaderProgram = null;
    this.buffers = {};
    this.textures = {};

    // Interaction state
    this.camera = {
      position: { x: 0, y: 0, z: 5 },
      rotation: { x: 0, y: 0, z: 0 },
      zoom: 1.0,
    };
    this.mouseState = {
      isDown: false,
      lastX: 0,
      lastY: 0,
    };
    this.selectedFrequencyBand = null;

    // Performance monitoring
    this.frameCount = 0;
    this.lastFrameTime = 0;
    this.renderStats = {
      fps: 0,
      frameTime: 0,
      gpuTime: 0,
    };

    // Event system
    this.eventManager = EventManager.getInstance();
    this.performanceMonitor = PerformanceMonitor.getInstance();

    // Initialize component
    this.init();
  }

  /**
   * Initialize the spectrogram visualizer
   * TODO: Set up WebGL context and shaders
   * TODO: Create frequency analysis pipeline
   * TODO: Initialize 3D scene and camera
   * TODO: Set up interaction handlers
   * TODO: Configure accessibility features
   */
  async init() {
    try {
      this.performanceMonitor.startOperation("SpectrogramVisualizer.init");

      // TODO: Validate WebGL support
      if (!this.checkWebGLSupport()) {
        throw new Error("WebGL not supported - falling back to 2D canvas");
      }

      // TODO: Create canvas and WebGL context
      await this.createCanvas();
      await this.initWebGL();

      // TODO: Load and compile shaders
      await this.loadShaders();

      // TODO: Initialize 3D scene
      await this.initScene();

      // TODO: Set up audio analysis pipeline
      await this.initAudioAnalysis();

      // TODO: Configure interaction handlers
      this.setupInteractionHandlers();

      // TODO: Initialize accessibility features
      this.setupAccessibility();

      // TODO: Start render loop
      this.startRenderLoop();

      this.isInitialized = true;
      this.eventManager.emit("spectrogramVisualizer:initialized", {
        component: "SpectrogramVisualizer",
        options: this.options,
      });

      this.performanceMonitor.endOperation("SpectrogramVisualizer.init");
    } catch (error) {
      console.error("SpectrogramVisualizer initialization failed:", error);
      this.eventManager.emit("spectrogramVisualizer:error", {
        error: error.message,
        component: "SpectrogramVisualizer",
      });
      throw error;
    }
  }

  /**
   * Check WebGL support and capabilities
   * TODO: Detect WebGL version and extensions
   * TODO: Check for required shader capabilities
   * TODO: Validate texture size limits
   * TODO: Test floating-point texture support
   */
  checkWebGLSupport() {
    try {
      const canvas = document.createElement("canvas");
      const gl = canvas.getContext("webgl2") || canvas.getContext("webgl");

      if (!gl) {
        console.warn("WebGL not supported, falling back to 2D rendering");
        return false;
      }

      // TODO: Check for required extensions
      const requiredExtensions = [
        "OES_texture_float",
        "OES_texture_float_linear",
        "WEBGL_color_buffer_float",
      ];

      const supportedExtensions = {};
      for (const ext of requiredExtensions) {
        supportedExtensions[ext] = gl.getExtension(ext) !== null;
      }

      // TODO: Validate texture size limits
      const maxTextureSize = gl.getParameter(gl.MAX_TEXTURE_SIZE);
      if (maxTextureSize < 1024) {
        console.warn("WebGL texture size too small for optimal performance");
      }

      return true;
    } catch (error) {
      console.error("WebGL support check failed:", error);
      return false;
    }
  }

  /**
   * Create canvas element and configure for WebGL
   * TODO: Create responsive canvas with proper DPI handling
   * TODO: Configure WebGL context attributes
   * TODO: Set up proper pixel ratio handling
   * TODO: Add canvas to container with proper styling
   */
  async createCanvas() {
    // TODO: Create canvas element
    this.canvas = document.createElement("canvas");
    this.canvas.className = "spectrogram-canvas";

    // TODO: Set up responsive sizing with DPI awareness
    const pixelRatio = window.devicePixelRatio || 1;
    this.canvas.width = this.options.width * pixelRatio;
    this.canvas.height = this.options.height * pixelRatio;
    this.canvas.style.width = `${this.options.width}px`;
    this.canvas.style.height = `${this.options.height}px`;

    // TODO: Add accessibility attributes
    this.canvas.setAttribute("role", "img");
    this.canvas.setAttribute(
      "aria-label",
      "Real-time audio spectrogram visualization"
    );
    this.canvas.setAttribute("tabindex", "0");

    // TODO: Add to container
    this.container.appendChild(this.canvas);

    // TODO: Handle resize events
    window.addEventListener("resize", this.handleResize.bind(this));
  }

  /**
   * Initialize WebGL context and set up rendering pipeline
   * TODO: Create WebGL context with optimal settings
   * TODO: Configure blending and depth testing
   * TODO: Set up framebuffers for off-screen rendering
   * TODO: Initialize uniform buffer objects
   */
  async initWebGL() {
    try {
      // TODO: Create WebGL context with performance settings
      const contextAttributes = {
        alpha: false,
        depth: true,
        stencil: false,
        antialias: true,
        premultipliedAlpha: false,
        preserveDrawingBuffer: false,
        powerPreference: "high-performance",
        failIfMajorPerformanceCaveat: false,
      };

      this.gl =
        this.canvas.getContext("webgl2", contextAttributes) ||
        this.canvas.getContext("webgl", contextAttributes);

      if (!this.gl) {
        throw new Error("Failed to create WebGL context");
      }

      // TODO: Configure WebGL state
      this.gl.enable(this.gl.DEPTH_TEST);
      this.gl.enable(this.gl.BLEND);
      this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
      this.gl.clearColor(0.0, 0.0, 0.0, 1.0);

      // TODO: Set viewport
      this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);

      // TODO: Initialize framebuffers for multipass rendering
      this.initFramebuffers();
    } catch (error) {
      console.error("WebGL initialization failed:", error);
      throw error;
    }
  }

  /**
   * Load and compile WebGL shaders for 3D spectrogram rendering
   * TODO: Load vertex shader for 3D waterfall geometry
   * TODO: Load fragment shader for color mapping and lighting
   * TODO: Compile and link shader program
   * TODO: Cache uniform and attribute locations
   * TODO: Set up shader variations for different rendering modes
   */
  async loadShaders() {
    try {
      // TODO: Vertex shader for 3D waterfall mesh
      const vertexShaderSource = `
                attribute vec3 a_position;
                attribute vec2 a_texCoord;
                attribute float a_intensity;

                uniform mat4 u_projectionMatrix;
                uniform mat4 u_viewMatrix;
                uniform mat4 u_modelMatrix;
                uniform float u_time;
                uniform vec2 u_resolution;

                varying vec2 v_texCoord;
                varying float v_intensity;
                varying vec3 v_worldPosition;

                void main() {
                    vec3 position = a_position;

                    // Apply waterfall effect
                    position.z += sin(u_time * 0.001 + a_position.x * 0.1) * 0.1;

                    gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(position, 1.0);

                    v_texCoord = a_texCoord;
                    v_intensity = a_intensity;
                    v_worldPosition = (u_modelMatrix * vec4(position, 1.0)).xyz;
                }
            `;

      // TODO: Fragment shader for color mapping and effects
      const fragmentShaderSource = `
                precision mediump float;

                varying vec2 v_texCoord;
                varying float v_intensity;
                varying vec3 v_worldPosition;

                uniform sampler2D u_colorMap;
                uniform vec2 u_intensityRange;
                uniform float u_gamma;
                uniform float u_time;
                uniform bool u_highContrast;

                vec3 applyColorMap(float intensity) {
                    // Normalize intensity to 0-1 range
                    float normalized = (intensity - u_intensityRange.x) / (u_intensityRange.y - u_intensityRange.x);
                    normalized = clamp(normalized, 0.0, 1.0);

                    // Apply gamma correction
                    normalized = pow(normalized, 1.0 / u_gamma);

                    // Sample color map texture
                    return texture2D(u_colorMap, vec2(normalized, 0.5)).rgb;
                }

                void main() {
                    vec3 color = applyColorMap(v_intensity);

                    // Apply high contrast mode if enabled
                    if (u_highContrast) {
                        float luminance = dot(color, vec3(0.299, 0.587, 0.114));
                        color = luminance > 0.5 ? vec3(1.0) : vec3(0.0);
                    }

                    // Add subtle animation effect
                    color += sin(u_time * 0.002 + v_worldPosition.x * 0.5) * 0.05;

                    gl_FragColor = vec4(color, 1.0);
                }
            `;

      // TODO: Compile shaders
      const vertexShader = this.compileShader(
        this.gl.VERTEX_SHADER,
        vertexShaderSource
      );
      const fragmentShader = this.compileShader(
        this.gl.FRAGMENT_SHADER,
        fragmentShaderSource
      );

      // TODO: Create and link program
      this.shaderProgram = this.gl.createProgram();
      this.gl.attachShader(this.shaderProgram, vertexShader);
      this.gl.attachShader(this.shaderProgram, fragmentShader);
      this.gl.linkProgram(this.shaderProgram);

      if (
        !this.gl.getProgramParameter(this.shaderProgram, this.gl.LINK_STATUS)
      ) {
        throw new Error(
          "Shader program linking failed: " +
            this.gl.getProgramInfoLog(this.shaderProgram)
        );
      }

      // TODO: Cache uniform and attribute locations
      this.shaderLocations = {
        attributes: {
          position: this.gl.getAttribLocation(this.shaderProgram, "a_position"),
          texCoord: this.gl.getAttribLocation(this.shaderProgram, "a_texCoord"),
          intensity: this.gl.getAttribLocation(
            this.shaderProgram,
            "a_intensity"
          ),
        },
        uniforms: {
          projectionMatrix: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_projectionMatrix"
          ),
          viewMatrix: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_viewMatrix"
          ),
          modelMatrix: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_modelMatrix"
          ),
          time: this.gl.getUniformLocation(this.shaderProgram, "u_time"),
          resolution: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_resolution"
          ),
          colorMap: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_colorMap"
          ),
          intensityRange: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_intensityRange"
          ),
          gamma: this.gl.getUniformLocation(this.shaderProgram, "u_gamma"),
          highContrast: this.gl.getUniformLocation(
            this.shaderProgram,
            "u_highContrast"
          ),
        },
      };
    } catch (error) {
      console.error("Shader loading failed:", error);
      throw error;
    }
  }

  /**
   * Compile individual shader
   * TODO: Compile shader source code
   * TODO: Check for compilation errors
   * TODO: Provide detailed error reporting
   */
  compileShader(type, source) {
    const shader = this.gl.createShader(type);
    this.gl.shaderSource(shader, source);
    this.gl.compileShader(shader);

    if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
      const error = this.gl.getShaderInfoLog(shader);
      this.gl.deleteShader(shader);
      throw new Error(`Shader compilation failed: ${error}`);
    }

    return shader;
  }

  /**
   * Initialize 3D scene geometry and matrices
   * TODO: Create waterfall mesh geometry
   * TODO: Set up projection and view matrices
   * TODO: Initialize camera controls
   * TODO: Create color map textures
   */
  async initScene() {
    try {
      // TODO: Create waterfall mesh geometry
      this.createWaterfallMesh();

      // TODO: Initialize matrices
      this.projectionMatrix = this.createProjectionMatrix();
      this.viewMatrix = this.createViewMatrix();
      this.modelMatrix = this.createModelMatrix();

      // TODO: Create color map textures
      await this.createColorMapTextures();

      // TODO: Initialize framebuffers for multipass rendering
      this.initFramebuffers();
    } catch (error) {
      console.error("Scene initialization failed:", error);
      throw error;
    }
  }

  /**
   * Create waterfall mesh geometry for 3D spectrogram
   * TODO: Generate vertex positions for waterfall grid
   * TODO: Create texture coordinates for frequency mapping
   * TODO: Set up index buffer for efficient rendering
   * TODO: Configure vertex array objects
   */
  createWaterfallMesh() {
    const width = this.options.fftSize / 2;
    const height = this.options.waterfallHeight;
    const vertices = [];
    const texCoords = [];
    const indices = [];

    // TODO: Generate waterfall grid vertices
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const normalizedX = (x / (width - 1)) * 2 - 1;
        const normalizedY = (y / (height - 1)) * 2 - 1;

        vertices.push(normalizedX, normalizedY, 0);
        texCoords.push(x / (width - 1), y / (height - 1));
      }
    }

    // TODO: Generate indices for triangle strips
    for (let y = 0; y < height - 1; y++) {
      for (let x = 0; x < width - 1; x++) {
        const i = y * width + x;

        // Triangle 1
        indices.push(i, i + 1, i + width);
        // Triangle 2
        indices.push(i + 1, i + width + 1, i + width);
      }
    }

    // TODO: Create and bind vertex buffers
    this.buffers.vertices = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffers.vertices);
    this.gl.bufferData(
      this.gl.ARRAY_BUFFER,
      new Float32Array(vertices),
      this.gl.STATIC_DRAW
    );

    this.buffers.texCoords = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.buffers.texCoords);
    this.gl.bufferData(
      this.gl.ARRAY_BUFFER,
      new Float32Array(texCoords),
      this.gl.STATIC_DRAW
    );

    this.buffers.indices = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.buffers.indices);
    this.gl.bufferData(
      this.gl.ELEMENT_ARRAY_BUFFER,
      new Uint16Array(indices),
      this.gl.STATIC_DRAW
    );

    this.meshData = {
      vertexCount: vertices.length / 3,
      indexCount: indices.length,
    };
  }

  /**
   * Initialize audio analysis pipeline for real-time frequency data
   * TODO: Create Web Audio API analyser node
   * TODO: Configure FFT parameters
   * TODO: Set up real-time data collection
   * TODO: Initialize frequency data buffers
   */
  async initAudioAnalysis() {
    try {
      // TODO: Create analyser node if not provided
      if (!this.analyserNode && this.audioContext) {
        this.analyserNode = this.audioContext.createAnalyser();
        this.analyserNode.fftSize = this.options.fftSize;
        this.analyserNode.smoothingTimeConstant =
          this.options.smoothingTimeConstant;
      }

      // TODO: Initialize frequency data arrays
      const bufferLength = this.analyserNode
        ? this.analyserNode.frequencyBinCount
        : this.options.fftSize / 2;
      this.frequencyData = new Float32Array(bufferLength);
      this.timeData = new Float32Array(this.options.fftSize);

      // TODO: Initialize spectrogram history buffer
      this.spectrogramData = Array(this.options.waterfallHeight)
        .fill(null)
        .map(() => new Float32Array(bufferLength));

      this.eventManager.emit("spectrogramVisualizer:audioAnalysisReady", {
        bufferLength,
        fftSize: this.options.fftSize,
      });
    } catch (error) {
      console.error("Audio analysis initialization failed:", error);
      throw error;
    }
  }

  /**
   * Set up interaction handlers for 3D navigation and frequency selection
   * TODO: Configure mouse/touch controls for camera movement
   * TODO: Implement zoom and pan functionality
   * TODO: Add frequency band selection capability
   * TODO: Set up keyboard shortcuts
   */
  setupInteractionHandlers() {
    // TODO: Mouse interaction handlers
    this.canvas.addEventListener("mousedown", this.handleMouseDown.bind(this));
    this.canvas.addEventListener("mousemove", this.handleMouseMove.bind(this));
    this.canvas.addEventListener("mouseup", this.handleMouseUp.bind(this));
    this.canvas.addEventListener("wheel", this.handleWheel.bind(this));

    // TODO: Touch interaction handlers for mobile
    this.canvas.addEventListener(
      "touchstart",
      this.handleTouchStart.bind(this)
    );
    this.canvas.addEventListener("touchmove", this.handleTouchMove.bind(this));
    this.canvas.addEventListener("touchend", this.handleTouchEnd.bind(this));

    // TODO: Keyboard shortcuts
    this.canvas.addEventListener("keydown", this.handleKeyDown.bind(this));

    // TODO: Context menu for export options
    this.canvas.addEventListener(
      "contextmenu",
      this.handleContextMenu.bind(this)
    );
  }

  /**
   * Configure accessibility features
   * TODO: Set up screen reader support
   * TODO: Implement keyboard navigation
   * TODO: Add high contrast mode
   * TODO: Configure ARIA attributes
   */
  setupAccessibility() {
    if (this.options.enableScreenReader) {
      // TODO: Create text description of spectrogram data
      this.createAccessibleDescription();

      // TODO: Set up periodic updates for screen readers
      setInterval(() => {
        this.updateAccessibleDescription();
      }, 1000);
    }

    // TODO: Add keyboard navigation support
    this.canvas.setAttribute("tabindex", "0");
    this.canvas.setAttribute("role", "application");
    this.canvas.setAttribute("aria-label", "Interactive 3D audio spectrogram");

    if (this.options.highContrastMode) {
      this.canvas.classList.add("high-contrast");
    }
  }

  /**
   * Start the main render loop
   * TODO: Implement frame rate limiting
   * TODO: Set up performance monitoring
   * TODO: Handle visibility changes for optimization
   * TODO: Implement adaptive quality based on performance
   */
  startRenderLoop() {
    let lastTime = 0;
    const targetFrameTime = 1000 / this.options.maxFrameRate;

    const renderFrame = (currentTime) => {
      // TODO: Frame rate limiting
      if (currentTime - lastTime >= targetFrameTime) {
        this.performanceMonitor.startFrame();

        try {
          this.updateSpectrogramData();
          this.render(currentTime);
          this.updatePerformanceStats(currentTime);
        } catch (error) {
          console.error("Render error:", error);
          this.eventManager.emit("spectrogramVisualizer:renderError", {
            error,
          });
        }

        this.performanceMonitor.endFrame();
        lastTime = currentTime;
      }

      if (this.isPlaying) {
        requestAnimationFrame(renderFrame);
      }
    };

    this.isPlaying = true;
    requestAnimationFrame(renderFrame);
  }

  /**
   * Update spectrogram data with latest frequency analysis
   * TODO: Get latest frequency data from analyser
   * TODO: Update waterfall history buffer
   * TODO: Apply smoothing and filtering
   * TODO: Update GPU textures with new data
   */
  updateSpectrogramData() {
    if (!this.analyserNode || !this.frequencyData) return;

    try {
      // TODO: Get current frequency data
      this.analyserNode.getFloatFrequencyData(this.frequencyData);

      // TODO: Shift waterfall data down
      for (let i = this.spectrogramData.length - 1; i > 0; i--) {
        this.spectrogramData[i].set(this.spectrogramData[i - 1]);
      }

      // TODO: Add new frequency data at the top
      this.spectrogramData[0].set(this.frequencyData);

      // TODO: Update GPU texture with new data
      this.updateSpectrogramTexture();
    } catch (error) {
      console.error("Spectrogram data update failed:", error);
    }
  }

  /**
   * Main render function
   * TODO: Clear framebuffer and set up rendering state
   * TODO: Update matrices and uniforms
   * TODO: Render 3D waterfall mesh
   * TODO: Apply post-processing effects
   */
  render(currentTime) {
    if (!this.gl || !this.shaderProgram) return;

    try {
      // TODO: Clear and set up render state
      this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
      this.gl.useProgram(this.shaderProgram);

      // TODO: Update matrices
      this.updateMatrices(currentTime);

      // TODO: Set uniforms
      this.setUniforms(currentTime);

      // TODO: Bind vertex attributes
      this.bindVertexAttributes();

      // TODO: Render waterfall mesh
      this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.buffers.indices);
      this.gl.drawElements(
        this.gl.TRIANGLES,
        this.meshData.indexCount,
        this.gl.UNSIGNED_SHORT,
        0
      );

      this.frameCount++;
    } catch (error) {
      console.error("Render failed:", error);
      throw error;
    }
  }

  // TODO: Implement remaining methods for matrix updates, texture management,
  //       interaction handling, export functionality, and cleanup

  /**
   * Connect to audio source for real-time analysis
   * TODO: Connect analyser node to audio graph
   * TODO: Validate audio source compatibility
   * TODO: Configure analysis parameters
   */
  connectAudioSource(audioNode) {
    try {
      if (!audioNode || !this.analyserNode) {
        throw new Error("Invalid audio node or analyser not initialized");
      }

      audioNode.connect(this.analyserNode);
      this.eventManager.emit("spectrogramVisualizer:audioConnected", {
        sourceType: audioNode.constructor.name,
      });
    } catch (error) {
      console.error("Audio source connection failed:", error);
      throw error;
    }
  }

  /**
   * Export spectrogram as image or data
   * TODO: Implement PNG export with current view
   * TODO: Add SVG export for vector graphics
   * TODO: Provide raw data export functionality
   * TODO: Configure export quality and format options
   */
  async exportSpectrogram(format = "png", options = {}) {
    try {
      this.performanceMonitor.startOperation("SpectrogramVisualizer.export");

      switch (format.toLowerCase()) {
        case "png":
          return await this.exportAsPNG(options);
        case "svg":
          return await this.exportAsSVG(options);
        case "data":
          return await this.exportAsData(options);
        default:
          throw new Error(`Unsupported export format: ${format}`);
      }
    } catch (error) {
      console.error("Export failed:", error);
      throw error;
    } finally {
      this.performanceMonitor.endOperation("SpectrogramVisualizer.export");
    }
  }

  /**
   * Clean up resources and remove event listeners
   * TODO: Dispose of WebGL resources
   * TODO: Clean up audio connections
   * TODO: Remove event listeners
   * TODO: Clear animation loops
   */
  destroy() {
    try {
      this.isPlaying = false;

      // TODO: Clean up WebGL resources
      if (this.gl) {
        Object.values(this.buffers).forEach((buffer) => {
          if (buffer) this.gl.deleteBuffer(buffer);
        });
        Object.values(this.textures).forEach((texture) => {
          if (texture) this.gl.deleteTexture(texture);
        });
        if (this.shaderProgram) {
          this.gl.deleteProgram(this.shaderProgram);
        }
      }

      // TODO: Remove event listeners
      window.removeEventListener("resize", this.handleResize);

      // TODO: Clean up DOM
      if (this.canvas && this.canvas.parentNode) {
        this.canvas.parentNode.removeChild(this.canvas);
      }

      this.eventManager.emit("spectrogramVisualizer:destroyed");
    } catch (error) {
      console.error("Cleanup failed:", error);
    }
  }

  // TODO: Implement getter methods for external access to visualization state
  get isReady() {
    return this.isInitialized;
  }
  get currentFrequencyData() {
    return this.frequencyData;
  }
  get renderingStats() {
    return this.renderStats;
  }
  get selectedBand() {
    return this.selectedFrequencyBand;
  }
}

export default SpectrogramVisualizer;
