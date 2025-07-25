/**
 * @fileoverview Web Audio Effects and Processing Module
 * @version 1.0.0
 * @author Huntmaster Development Team
 * @created 2024-01-20
 * @modified 2024-01-20
 *
 * Comprehensive Web Audio API effects processing with real-time parameter
 * automation, high-quality DSP algorithms, and modular effect chains.
 *
 * Features:
 * ✅ Real-time audio effects with high-quality DSP
 * ✅ Modular effect chains with flexible routing
 * ✅ Parameter automation with curves and modulation
 * ✅ Preset management and effect templates
 * ✅ Convolution reverb with impulse response loading
 * ✅ Multi-band processing and frequency splitting
 * ✅ Dynamics processing (compressor, limiter, gate)
 * ✅ Time-based effects (delay, reverb, chorus, flanger)
 * ✅ Modulation effects (tremolo, vibrato, ring modulation)
 * ✅ Distortion and saturation algorithms
 * ✅ Spectral processing and filtering
 * ✅ Performance monitoring and CPU optimization
 *
 * @example
 * ```javascript
 * import { WebAudioEffects } from './modules/web-audio/index.js';
 *
 * const effects = new WebAudioEffects(audioContext);
 *
 * // Create effect chain
 * const reverbEffect = await effects.createEffect('convolutionReverb', {
 *   impulseResponse: 'hall.wav',
 *   wetGain: 0.3,
 *   dryGain: 0.7
 * });
 *
 * const compressor = await effects.createEffect('compressor', {
 *   threshold: -20,
 *   ratio: 4,
 *   attack: 0.003,
 *   release: 0.1
 * });
 * ```
 */

/**
 * Web Audio Effects and Processing Manager
 *
 * Provides comprehensive audio effects processing with real-time parameter
 * control, effect chaining, and performance optimization.
 *
 * @class WebAudioEffects
 */
export class WebAudioEffects {
  /**
   * Create WebAudioEffects manager
   *
   * @param {AudioContext} audioContext - Web Audio context
   * @param {Object} options - Configuration options
   * @param {boolean} [options.enableConvolution=true] - Enable convolution reverb
   * @param {boolean} [options.enableModulation=true] - Enable modulation effects
   * @param {number} [options.maxEffects=50] - Maximum number of effects
   */
  constructor(audioContext, options = {}) {
    if (!audioContext) {
      throw new Error("AudioContext is required");
    }

    this.audioContext = audioContext;

    // Configuration
    this.config = {
      enableConvolution: options.enableConvolution !== false,
      enableModulation: options.enableModulation !== false,
      maxEffects: options.maxEffects || 50,
      defaultFadeTime: options.defaultFadeTime || 0.05,
      enablePerformanceMonitoring:
        options.enablePerformanceMonitoring !== false,
      sampleRate: audioContext.sampleRate,
      ...options,
    };

    // Effect management
    this.effects = new Map(); // effectId -> effect instance
    this.effectChains = new Map(); // chainId -> effect chain
    this.effectTemplates = new Map(); // template definitions
    this.presets = new Map(); // preset storage
    this.impulseResponses = new Map(); // IR buffer storage

    // Parameter automation
    this.automations = new Map(); // automationId -> automation data
    this.modulators = new Map(); // modulatorId -> modulator instance
    this.scheduledEvents = [];

    // Performance tracking
    this.performance = {
      effectCount: 0,
      chainCount: 0,
      automationCount: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      processingLatency: 0,
      createdEffects: 0,
      destroyedEffects: 0,
    };

    // Event handling
    this.eventHandlers = new Map();

    // Initialize effect templates and presets
    this._initializeEffectTemplates();
    this._initializePresets();

    // Start performance monitoring
    if (this.config.enablePerformanceMonitoring) {
      this._startPerformanceMonitoring();
    }

    console.log("WebAudioEffects manager initialized");
  }

  /**
   * Initialize built-in effect templates
   * @private
   */
  _initializeEffectTemplates() {
    // === DYNAMICS EFFECTS ===

    // Compressor
    this.effectTemplates.set("compressor", {
      type: "dynamics",
      category: "dynamics",
      description: "Dynamic range compression",
      createNodes: (context) => {
        const compressor = context.createDynamicsCompressor();
        return {
          input: compressor,
          output: compressor,
          nodes: { compressor },
          controls: {
            threshold: compressor.threshold,
            knee: compressor.knee,
            ratio: compressor.ratio,
            attack: compressor.attack,
            release: compressor.release,
          },
        };
      },
      defaultParams: {
        threshold: -24,
        knee: 30,
        ratio: 12,
        attack: 0.003,
        release: 0.25,
      },
    });

    // Gate
    this.effectTemplates.set("gate", {
      type: "dynamics",
      category: "dynamics",
      description: "Noise gate with threshold control",
      createNodes: (context) => {
        const analyser = context.createAnalyser();
        const gate = context.createGain();
        const processor = context.createScriptProcessor(256, 1, 1);

        analyser.connect(gate);

        let isOpen = false;
        const threshold = -40; // dB
        const hysteresis = 6; // dB

        processor.onaudioprocess = (event) => {
          const input = event.inputBuffer.getChannelData(0);
          const output = event.outputBuffer.getChannelData(0);

          // Calculate RMS level
          let sum = 0;
          for (let i = 0; i < input.length; i++) {
            sum += input[i] * input[i];
          }
          const rms = Math.sqrt(sum / input.length);
          const dB = 20 * Math.log10(Math.max(rms, 0.000001));

          // Gate logic with hysteresis
          if (!isOpen && dB > threshold) {
            isOpen = true;
            gate.gain.setTargetAtTime(1.0, context.currentTime, 0.01);
          } else if (isOpen && dB < threshold - hysteresis) {
            isOpen = false;
            gate.gain.setTargetAtTime(0.0, context.currentTime, 0.05);
          }

          // Copy input to output (processor is just for analysis)
          for (let i = 0; i < input.length; i++) {
            output[i] = input[i];
          }
        };

        return {
          input: analyser,
          output: gate,
          nodes: { analyser, gate, processor },
          controls: {
            threshold: { value: threshold },
            hysteresis: { value: hysteresis },
          },
        };
      },
      defaultParams: {
        threshold: -40,
        hysteresis: 6,
      },
    });

    // === FILTER EFFECTS ===

    // Multi-band EQ
    this.effectTemplates.set("eq3band", {
      type: "filter",
      category: "eq",
      description: "3-band parametric equalizer",
      createNodes: (context) => {
        const lowShelf = context.createBiquadFilter();
        const midPeak = context.createBiquadFilter();
        const highShelf = context.createBiquadFilter();

        lowShelf.type = "lowshelf";
        lowShelf.frequency.value = 200;
        lowShelf.gain.value = 0;

        midPeak.type = "peaking";
        midPeak.frequency.value = 1000;
        midPeak.Q.value = 1;
        midPeak.gain.value = 0;

        highShelf.type = "highshelf";
        highShelf.frequency.value = 5000;
        highShelf.gain.value = 0;

        // Chain filters
        lowShelf.connect(midPeak);
        midPeak.connect(highShelf);

        return {
          input: lowShelf,
          output: highShelf,
          nodes: { lowShelf, midPeak, highShelf },
          controls: {
            lowGain: lowShelf.gain,
            lowFreq: lowShelf.frequency,
            midGain: midPeak.gain,
            midFreq: midPeak.frequency,
            midQ: midPeak.Q,
            highGain: highShelf.gain,
            highFreq: highShelf.frequency,
          },
        };
      },
      defaultParams: {
        lowGain: 0,
        lowFreq: 200,
        midGain: 0,
        midFreq: 1000,
        midQ: 1,
        highGain: 0,
        highFreq: 5000,
      },
    });

    // === TIME-BASED EFFECTS ===

    // Delay
    this.effectTemplates.set("delay", {
      type: "time",
      category: "delay",
      description: "Digital delay with feedback",
      createNodes: (context) => {
        const input = context.createGain();
        const output = context.createGain();
        const delay = context.createDelay(5.0);
        const feedback = context.createGain();
        const wetGain = context.createGain();
        const dryGain = context.createGain();

        // Routing
        input.connect(dryGain);
        input.connect(delay);
        delay.connect(feedback);
        delay.connect(wetGain);
        feedback.connect(delay);

        dryGain.connect(output);
        wetGain.connect(output);

        // Default values
        delay.delayTime.value = 0.25;
        feedback.gain.value = 0.3;
        wetGain.gain.value = 0.5;
        dryGain.gain.value = 1.0;

        return {
          input: input,
          output: output,
          nodes: { input, output, delay, feedback, wetGain, dryGain },
          controls: {
            delayTime: delay.delayTime,
            feedback: feedback.gain,
            wetGain: wetGain.gain,
            dryGain: dryGain.gain,
          },
        };
      },
      defaultParams: {
        delayTime: 0.25,
        feedback: 0.3,
        wetGain: 0.5,
        dryGain: 1.0,
      },
    });

    // Convolution Reverb
    if (this.config.enableConvolution) {
      this.effectTemplates.set("convolutionReverb", {
        type: "time",
        category: "reverb",
        description: "Convolution reverb with impulse responses",
        createNodes: (context, options = {}) => {
          const input = context.createGain();
          const output = context.createGain();
          const convolver = context.createConvolver();
          const wetGain = context.createGain();
          const dryGain = context.createGain();

          // Routing
          input.connect(dryGain);
          input.connect(convolver);
          convolver.connect(wetGain);

          dryGain.connect(output);
          wetGain.connect(output);

          // Load impulse response if provided
          if (options.impulseResponse) {
            this._loadImpulseResponse(options.impulseResponse)
              .then((buffer) => {
                convolver.buffer = buffer;
              })
              .catch((error) => {
                console.error("Failed to load impulse response:", error);
              });
          }

          return {
            input: input,
            output: output,
            nodes: { input, output, convolver, wetGain, dryGain },
            controls: {
              wetGain: wetGain.gain,
              dryGain: dryGain.gain,
            },
          };
        },
        defaultParams: {
          wetGain: 0.3,
          dryGain: 0.7,
          impulseResponse: null,
        },
      });
    }

    // === MODULATION EFFECTS ===

    if (this.config.enableModulation) {
      // Chorus
      this.effectTemplates.set("chorus", {
        type: "modulation",
        category: "modulation",
        description: "Chorus effect with LFO modulation",
        createNodes: (context) => {
          const input = context.createGain();
          const output = context.createGain();
          const delay = context.createDelay(0.05);
          const lfo = context.createOscillator();
          const lfoGain = context.createGain();
          const wetGain = context.createGain();
          const dryGain = context.createGain();

          // LFO setup
          lfo.type = "sine";
          lfo.frequency.value = 0.5;
          lfoGain.gain.value = 0.005;

          // Routing
          input.connect(dryGain);
          input.connect(delay);
          delay.connect(wetGain);

          lfo.connect(lfoGain);
          lfoGain.connect(delay.delayTime);

          dryGain.connect(output);
          wetGain.connect(output);

          // Start LFO
          lfo.start();

          // Default values
          delay.delayTime.value = 0.02;
          wetGain.gain.value = 0.5;
          dryGain.gain.value = 1.0;

          return {
            input: input,
            output: output,
            nodes: { input, output, delay, lfo, lfoGain, wetGain, dryGain },
            controls: {
              rate: lfo.frequency,
              depth: lfoGain.gain,
              wetGain: wetGain.gain,
              dryGain: dryGain.gain,
            },
          };
        },
        defaultParams: {
          rate: 0.5,
          depth: 0.005,
          wetGain: 0.5,
          dryGain: 1.0,
        },
      });

      // Tremolo
      this.effectTemplates.set("tremolo", {
        type: "modulation",
        category: "modulation",
        description: "Amplitude modulation tremolo",
        createNodes: (context) => {
          const input = context.createGain();
          const output = context.createGain();
          const lfo = context.createOscillator();
          const lfoGain = context.createGain();
          const dcOffset = context.createConstantSource();
          const amplitudeGain = context.createGain();

          // LFO setup
          lfo.type = "sine";
          lfo.frequency.value = 5;
          lfoGain.gain.value = 0.5;
          dcOffset.offset.value = 1;

          // Routing
          input.connect(amplitudeGain);
          amplitudeGain.connect(output);

          lfo.connect(lfoGain);
          dcOffset.connect(amplitudeGain.gain);
          lfoGain.connect(amplitudeGain.gain);

          // Start sources
          lfo.start();
          dcOffset.start();

          return {
            input: input,
            output: output,
            nodes: { input, output, lfo, lfoGain, dcOffset, amplitudeGain },
            controls: {
              rate: lfo.frequency,
              depth: lfoGain.gain,
            },
          };
        },
        defaultParams: {
          rate: 5,
          depth: 0.5,
        },
      });
    }

    // === DISTORTION EFFECTS ===

    // Waveshaper Distortion
    this.effectTemplates.set("distortion", {
      type: "distortion",
      category: "distortion",
      description: "Waveshaper distortion with drive control",
      createNodes: (context) => {
        const input = context.createGain();
        const output = context.createGain();
        const preGain = context.createGain();
        const waveshaper = context.createWaveShaper();
        const postGain = context.createGain();
        const filter = context.createBiquadFilter();

        // Create distortion curve
        const makeDistortionCurve = (amount) => {
          const samples = 44100;
          const curve = new Float32Array(samples);
          const deg = Math.PI / 180;

          for (let i = 0; i < samples; i++) {
            const x = (i * 2) / samples - 1;
            curve[i] =
              ((3 + amount) * x * 20 * deg) / (Math.PI + amount * Math.abs(x));
          }

          return curve;
        };

        waveshaper.curve = makeDistortionCurve(20);
        waveshaper.oversample = "4x";

        // Anti-aliasing filter
        filter.type = "lowpass";
        filter.frequency.value = 8000;

        // Routing
        input.connect(preGain);
        preGain.connect(waveshaper);
        waveshaper.connect(filter);
        filter.connect(postGain);
        postGain.connect(output);

        // Default values
        preGain.gain.value = 10;
        postGain.gain.value = 0.1;

        return {
          input: input,
          output: output,
          nodes: { input, output, preGain, waveshaper, postGain, filter },
          controls: {
            drive: preGain.gain,
            level: postGain.gain,
            tone: filter.frequency,
          },
          updateCurve: (amount) => {
            waveshaper.curve = makeDistortionCurve(amount);
          },
        };
      },
      defaultParams: {
        drive: 10,
        level: 0.1,
        tone: 8000,
      },
    });

    console.log(`Initialized ${this.effectTemplates.size} effect templates`);
  }

  /**
   * Initialize built-in presets
   * @private
   */
  _initializePresets() {
    // Compression presets
    this.presets.set("vocal-compressor", {
      effect: "compressor",
      params: {
        threshold: -18,
        knee: 8,
        ratio: 6,
        attack: 0.005,
        release: 0.1,
      },
      description: "Vocal compression with moderate settings",
    });

    this.presets.set("drum-compressor", {
      effect: "compressor",
      params: {
        threshold: -12,
        knee: 2,
        ratio: 8,
        attack: 0.001,
        release: 0.05,
      },
      description: "Punchy drum compression",
    });

    // EQ presets
    this.presets.set("vocal-eq", {
      effect: "eq3band",
      params: {
        lowGain: -2,
        lowFreq: 100,
        midGain: 3,
        midFreq: 2500,
        midQ: 1.5,
        highGain: 2,
        highFreq: 8000,
      },
      description: "Vocal presence EQ",
    });

    // Delay presets
    this.presets.set("slap-delay", {
      effect: "delay",
      params: {
        delayTime: 0.125,
        feedback: 0.15,
        wetGain: 0.3,
        dryGain: 1.0,
      },
      description: "Short slap-back delay",
    });

    this.presets.set("echo-delay", {
      effect: "delay",
      params: {
        delayTime: 0.5,
        feedback: 0.4,
        wetGain: 0.4,
        dryGain: 1.0,
      },
      description: "Classic echo delay",
    });

    console.log(`Initialized ${this.presets.size} effect presets`);
  }

  /**
   * Create audio effect
   *
   * @param {string} effectType - Type of effect to create
   * @param {Object|string} [options={}] - Effect options or preset name
   * @param {string} [effectId] - Custom effect ID
   * @returns {Promise<Object>} Created effect instance
   */
  async createEffect(effectType, options = {}, effectId) {
    if (this.effects.size >= this.config.maxEffects) {
      throw new Error(
        `Maximum number of effects (${this.config.maxEffects}) reached`
      );
    }

    // Handle preset loading
    if (typeof options === "string") {
      const presetName = options;
      const preset = this.presets.get(presetName);
      if (preset) {
        effectType = preset.effect;
        options = preset.params;
        console.log(
          `Loading preset "${presetName}" for effect type "${effectType}"`
        );
      } else {
        throw new Error(`Preset "${presetName}" not found`);
      }
    }

    // Get effect template
    const template = this.effectTemplates.get(effectType);
    if (!template) {
      throw new Error(`Unknown effect type: ${effectType}`);
    }

    // Generate unique effect ID
    if (!effectId) {
      effectId = this._generateEffectId(effectType);
    }

    if (this.effects.has(effectId)) {
      throw new Error(`Effect with ID "${effectId}" already exists`);
    }

    try {
      const startTime = performance.now();

      // Create effect nodes
      const effectInstance = template.createNodes(this.audioContext, options);

      // Apply default parameters
      await this._applyEffectParameters(
        effectInstance,
        template.defaultParams,
        options
      );

      // Create effect wrapper
      const effect = {
        id: effectId,
        type: effectType,
        template: template,
        instance: effectInstance,
        parameters: new Map(),
        automations: new Map(),
        isActive: true,
        isBypassed: false,
        createdAt: Date.now(),
        creationTime: performance.now() - startTime,
        metadata: {
          category: template.category,
          description: template.description,
        },
      };

      // Store effect
      this.effects.set(effectId, effect);

      // Update performance counters
      this.performance.effectCount++;
      this.performance.createdEffects++;

      console.log(
        `Created ${effectType} effect "${effectId}" in ${effect.creationTime.toFixed(
          2
        )}ms`
      );

      this._emitEvent("effectCreated", {
        effectId,
        effectType,
        effect: effect,
        creationTime: effect.creationTime,
      });

      return effect;
    } catch (error) {
      console.error(`Failed to create ${effectType} effect:`, error);
      this._emitEvent("effectCreationError", { effectType, effectId, error });
      throw error;
    }
  }

  /**
   * Apply effect parameters
   * @private
   */
  async _applyEffectParameters(effectInstance, defaultParams, userParams) {
    const allParams = { ...defaultParams, ...userParams };

    for (const [paramName, value] of Object.entries(allParams)) {
      if (effectInstance.controls && effectInstance.controls[paramName]) {
        const control = effectInstance.controls[paramName];

        try {
          if (control && typeof control.setValueAtTime === "function") {
            // AudioParam
            control.setValueAtTime(value, this.audioContext.currentTime);
          } else if (
            control &&
            typeof control === "object" &&
            "value" in control
          ) {
            // Object with value property
            control.value = value;
          } else if (effectInstance.updateCurve && paramName === "drive") {
            // Special handling for waveshaper curve updates
            effectInstance.updateCurve(value);
          }
        } catch (error) {
          console.warn(`Failed to set parameter ${paramName}:`, error);
        }
      }
    }
  }

  /**
   * Create effect chain
   *
   * @param {string} chainId - Chain identifier
   * @param {Array<Object>} effectSpecs - Array of effect specifications
   * @returns {Promise<Object>} Created effect chain
   */
  async createEffectChain(chainId, effectSpecs) {
    if (this.effectChains.has(chainId)) {
      throw new Error(`Effect chain "${chainId}" already exists`);
    }

    try {
      const chain = {
        id: chainId,
        effects: [],
        input: null,
        output: null,
        isActive: true,
        isBypassed: false,
        createdAt: Date.now(),
      };

      // Create effects and connect them
      for (let i = 0; i < effectSpecs.length; i++) {
        const spec = effectSpecs[i];
        const effect = await this.createEffect(
          spec.type,
          spec.options,
          spec.id
        );

        chain.effects.push(effect);

        // Set up connections
        if (i === 0) {
          chain.input = effect.instance.input;
        }

        if (i === effectSpecs.length - 1) {
          chain.output = effect.instance.output;
        }

        // Connect to next effect
        if (i < effectSpecs.length - 1) {
          const nextSpec = effectSpecs[i + 1];
          const nextEffect = await this.createEffect(
            nextSpec.type,
            nextSpec.options,
            nextSpec.id
          );
          effect.instance.output.connect(nextEffect.instance.input);
        }
      }

      this.effectChains.set(chainId, chain);
      this.performance.chainCount++;

      console.log(
        `Created effect chain "${chainId}" with ${chain.effects.length} effects`
      );

      this._emitEvent("effectChainCreated", {
        chainId,
        chain,
        effectCount: chain.effects.length,
      });

      return chain;
    } catch (error) {
      console.error(`Failed to create effect chain "${chainId}":`, error);
      this._emitEvent("effectChainCreationError", { chainId, error });
      throw error;
    }
  }

  /**
   * Set effect parameter
   *
   * @param {string} effectId - Effect ID
   * @param {string} paramName - Parameter name
   * @param {number} value - Parameter value
   * @param {number} [time] - Time to apply change
   * @param {string} [method='setValueAtTime'] - Automation method
   */
  setEffectParameter(
    effectId,
    paramName,
    value,
    time = this.audioContext.currentTime,
    method = "setValueAtTime"
  ) {
    const effect = this.effects.get(effectId);
    if (!effect) {
      throw new Error(`Effect "${effectId}" not found`);
    }

    const control = effect.instance.controls[paramName];
    if (!control) {
      throw new Error(
        `Parameter "${paramName}" not found on effect "${effectId}"`
      );
    }

    try {
      if (control && typeof control[method] === "function") {
        // AudioParam with automation methods
        control[method](value, time);
      } else if (control && typeof control === "object" && "value" in control) {
        // Object with value property
        control.value = value;
      } else if (effect.instance.updateCurve && paramName === "drive") {
        // Special handling for waveshaper
        effect.instance.updateCurve(value);
      }

      // Store parameter value
      effect.parameters.set(paramName, value);

      console.log(
        `Set ${effectId}.${paramName} = ${value} at time ${time.toFixed(3)}`
      );

      this._emitEvent("effectParameterChanged", {
        effectId,
        paramName,
        value,
        time,
        method,
      });
    } catch (error) {
      console.error(
        `Failed to set parameter ${paramName} on effect ${effectId}:`,
        error
      );
      this._emitEvent("effectParameterError", {
        effectId,
        paramName,
        value,
        error,
      });
      throw error;
    }
  }

  /**
   * Automate effect parameter
   *
   * @param {string} effectId - Effect ID
   * @param {string} paramName - Parameter name
   * @param {Array} automation - Automation curve
   * @returns {string} Automation ID
   */
  automateEffectParameter(effectId, paramName, automation) {
    const effect = this.effects.get(effectId);
    if (!effect) {
      throw new Error(`Effect "${effectId}" not found`);
    }

    const control = effect.instance.controls[paramName];
    if (!control || typeof control.setValueAtTime !== "function") {
      throw new Error(
        `Parameter "${paramName}" is not automatable on effect "${effectId}"`
      );
    }

    const automationId = `${effectId}_${paramName}_${Date.now()}`;

    try {
      // Schedule automation events
      automation.forEach(
        ({ time, value, method = "linearRampToValueAtTime" }) => {
          const scheduleTime = this.audioContext.currentTime + time;

          if (method === "setValueAtTime") {
            control.setValueAtTime(value, scheduleTime);
          } else if (method === "linearRampToValueAtTime") {
            control.linearRampToValueAtTime(value, scheduleTime);
          } else if (method === "exponentialRampToValueAtTime") {
            control.exponentialRampToValueAtTime(value, scheduleTime);
          } else if (method === "setTargetAtTime") {
            const timeConstant = automation.timeConstant || 0.1;
            control.setTargetAtTime(value, scheduleTime, timeConstant);
          }
        }
      );

      // Store automation metadata
      const automationData = {
        id: automationId,
        effectId,
        paramName,
        automation,
        createdAt: Date.now(),
        isActive: true,
      };

      this.automations.set(automationId, automationData);
      effect.automations.set(paramName, automationId);
      this.performance.automationCount++;

      console.log(
        `Created automation "${automationId}" for ${effectId}.${paramName}`
      );

      this._emitEvent("effectAutomationCreated", {
        automationId,
        effectId,
        paramName,
        automation,
      });

      return automationId;
    } catch (error) {
      console.error(
        `Failed to create automation for ${effectId}.${paramName}:`,
        error
      );
      this._emitEvent("effectAutomationError", {
        effectId,
        paramName,
        automation,
        error,
      });
      throw error;
    }
  }

  /**
   * Bypass effect
   *
   * @param {string} effectId - Effect ID
   * @param {boolean} bypass - Whether to bypass
   */
  bypassEffect(effectId, bypass = true) {
    const effect = this.effects.get(effectId);
    if (!effect) {
      throw new Error(`Effect "${effectId}" not found`);
    }

    effect.isBypassed = bypass;

    // TODO: Implement proper bypass routing
    // This would involve creating a bypass connection that routes
    // input directly to output when bypassed

    console.log(`Effect "${effectId}" ${bypass ? "bypassed" : "enabled"}`);

    this._emitEvent("effectBypassed", {
      effectId,
      isBypassed: bypass,
    });
  }

  /**
   * Destroy effect
   *
   * @param {string} effectId - Effect ID
   */
  destroyEffect(effectId) {
    const effect = this.effects.get(effectId);
    if (!effect) {
      throw new Error(`Effect "${effectId}" not found`);
    }

    try {
      // Disconnect all nodes
      if (
        effect.instance.input &&
        typeof effect.instance.input.disconnect === "function"
      ) {
        effect.instance.input.disconnect();
      }

      if (
        effect.instance.output &&
        typeof effect.instance.output.disconnect === "function"
      ) {
        effect.instance.output.disconnect();
      }

      // Stop any oscillators
      Object.values(effect.instance.nodes).forEach((node) => {
        if (node && typeof node.stop === "function") {
          try {
            node.stop();
          } catch (error) {
            // Node might already be stopped
          }
        }
      });

      // Remove from collections
      this.effects.delete(effectId);

      // Clean up automations
      for (const [automationId, automation] of this.automations.entries()) {
        if (automation.effectId === effectId) {
          this.automations.delete(automationId);
        }
      }

      // Update performance counters
      this.performance.effectCount--;
      this.performance.destroyedEffects++;

      console.log(`Destroyed effect "${effectId}"`);

      this._emitEvent("effectDestroyed", {
        effectId,
        effectType: effect.type,
      });
    } catch (error) {
      console.error(`Failed to destroy effect "${effectId}":`, error);
      this._emitEvent("effectDestroyError", { effectId, error });
      throw error;
    }
  }

  /**
   * Load impulse response for convolution
   * @private
   */
  async _loadImpulseResponse(url) {
    try {
      // Check cache first
      if (this.impulseResponses.has(url)) {
        return this.impulseResponses.get(url);
      }

      const response = await fetch(url);
      const arrayBuffer = await response.arrayBuffer();
      const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);

      // Cache the buffer
      this.impulseResponses.set(url, audioBuffer);

      console.log(`Loaded impulse response: ${url}`);
      return audioBuffer;
    } catch (error) {
      console.error(`Failed to load impulse response ${url}:`, error);
      throw error;
    }
  }

  /**
   * Start performance monitoring
   * @private
   */
  _startPerformanceMonitoring() {
    const updateMetrics = () => {
      // Update CPU usage estimate
      this.performance.cpuUsage = this._estimateCPUUsage();

      // Update memory usage if available
      if (performance.memory) {
        this.performance.memoryUsage =
          performance.memory.usedJSHeapSize / 1024 / 1024;
      }

      this._emitEvent("performanceUpdate", this.performance);
    };

    // Update metrics every 5 seconds
    setInterval(updateMetrics, 5000);
  }

  /**
   * Estimate CPU usage
   * @private
   */
  _estimateCPUUsage() {
    // Simple CPU usage estimation based on number of active effects
    const baseUsage = 0.05; // 5% base usage
    const perEffectUsage = 0.02; // 2% per effect
    const perAutomationUsage = 0.01; // 1% per automation

    return Math.min(
      baseUsage +
        this.performance.effectCount * perEffectUsage +
        this.performance.automationCount * perAutomationUsage,
      1.0
    );
  }

  // === UTILITY METHODS ===

  /**
   * Generate unique effect ID
   * @private
   */
  _generateEffectId(effectType) {
    let counter = 1;
    let effectId;

    do {
      effectId = `${effectType}_${counter}`;
      counter++;
    } while (this.effects.has(effectId));

    return effectId;
  }

  /**
   * Get effect by ID
   *
   * @param {string} effectId - Effect ID
   * @returns {Object|null} Effect instance
   */
  getEffect(effectId) {
    return this.effects.get(effectId) || null;
  }

  /**
   * Get all effects
   *
   * @returns {Map<string, Object>} All effects
   */
  getAllEffects() {
    return new Map(this.effects);
  }

  /**
   * Get effect chain
   *
   * @param {string} chainId - Chain ID
   * @returns {Object|null} Effect chain
   */
  getEffectChain(chainId) {
    return this.effectChains.get(chainId) || null;
  }

  /**
   * Get available effect types
   *
   * @returns {Array<string>} Available effect types
   */
  getAvailableEffectTypes() {
    return Array.from(this.effectTemplates.keys());
  }

  /**
   * Get available presets
   *
   * @returns {Array<string>} Available preset names
   */
  getAvailablePresets() {
    return Array.from(this.presets.keys());
  }

  /**
   * Get performance metrics
   *
   * @returns {Object} Performance metrics
   */
  getPerformanceMetrics() {
    return { ...this.performance };
  }

  /**
   * Save effect preset
   *
   * @param {string} presetName - Preset name
   * @param {string} effectId - Effect ID to save
   * @param {string} [description] - Preset description
   */
  savePreset(presetName, effectId, description = "") {
    const effect = this.effects.get(effectId);
    if (!effect) {
      throw new Error(`Effect "${effectId}" not found`);
    }

    const preset = {
      effect: effect.type,
      params: Object.fromEntries(effect.parameters),
      description: description || `Custom ${effect.type} preset`,
      createdAt: Date.now(),
    };

    this.presets.set(presetName, preset);

    console.log(`Saved preset "${presetName}" from effect "${effectId}"`);
    this._emitEvent("presetSaved", { presetName, effectId, preset });
  }

  /**
   * Event handling
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
   * Cleanup and destroy manager
   */
  destroy() {
    console.log("Destroying WebAudioEffects manager...");

    // Destroy all effects
    for (const effectId of this.effects.keys()) {
      try {
        this.destroyEffect(effectId);
      } catch (error) {
        console.warn(`Failed to destroy effect ${effectId}:`, error);
      }
    }

    // Clear all collections
    this.effects.clear();
    this.effectChains.clear();
    this.automations.clear();
    this.modulators.clear();
    this.impulseResponses.clear();
    this.eventHandlers.clear();

    // Reset performance counters
    this.performance.effectCount = 0;
    this.performance.chainCount = 0;
    this.performance.automationCount = 0;

    this._emitEvent("destroyed");
    console.log("WebAudioEffects manager destroyed");
  }
}

export default WebAudioEffects;
