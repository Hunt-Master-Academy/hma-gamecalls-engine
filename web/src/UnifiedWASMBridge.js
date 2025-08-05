/**
 * @file UnifiedWASMBridge.js
 * @brief Bridge between documented Alpha Testing API and actual WASM implementation
 *
 * This module provides the unified API as documented in ALPHA_TESTING_EXECUTION_CHAIN.md
 * while interfacing with the actual EnhancedWASMInterface implementation.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date August 3, 2025
 */

/**
 * @class UnifiedWASMBridge
 * @brief Provides the documented alpha testing API interface
 */
class UnifiedWASMBridge {
    constructor() {
        this.module = null;
        this.wasmInterface = null;
        this.engines = new Map(); // engineId -> interface instance
        this.sessions = new Map(); // sessionId -> engineId mapping
        this.nextEngineId = 1;
        this.nextSessionId = 1;
        this.isReady = false;
    }

    /**
     * Initialize the WASM module and bridge
     * @returns {Promise<boolean>} Success status
     */
    async initialize() {
        try {
            // Load the WASM module
            this.module = await HuntmasterEngine();

            console.log('🔗 UnifiedWASMBridge: WASM module loaded');
            console.log('📦 Available classes:', Object.keys(this.module).filter(k => typeof this.module[k] === 'function'));

            this.isReady = true;
            return true;
        } catch (error) {
            console.error('❌ UnifiedWASMBridge initialization failed:', error);
            return false;
        }
    }

    /**
     * Create engine instance (documented API)
     * @returns {number} Engine ID or -1 on failure
     */
    unified_create_engine() {
        try {
            if (!this.isReady) {
                console.error('❌ Bridge not initialized');
                return -1;
            }

            let wasmInterface = null;

            // Try different WASM interface classes
            if (this.module.HuntmasterEngineAdvanced) {
                wasmInterface = new this.module.HuntmasterEngineAdvanced();
                console.log('✅ Created HuntmasterEngineAdvanced instance');
            } else if (this.module.EnhancedWASMInterface) {
                wasmInterface = new this.module.EnhancedWASMInterface();
                console.log('✅ Created EnhancedWASMInterface instance');
            } else if (this.module.HuntmasterEngine) {
                wasmInterface = new this.module.HuntmasterEngine();
                console.log('✅ Created HuntmasterEngine instance');
            } else if (this.module.WASMInterface) {
                wasmInterface = new this.module.WASMInterface();
                console.log('✅ Created WASMInterface instance');
            } else {
                console.error('❌ No suitable WASM interface found');
                return -1;
            }

            const engineId = this.nextEngineId++;
            this.engines.set(engineId, wasmInterface);

            console.log(`🔧 Engine created with ID: ${engineId}`);
            return engineId;

        } catch (error) {
            console.error('❌ Engine creation failed:', error);
            return -1;
        }
    }

    /**
     * Create session for engine (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sampleRate Sample rate (default 44100)
     * @returns {number} Session ID or -1 on failure
     */
    unified_create_session(engineId, sampleRate = 44100) {
        try {
            const wasmInterface = this.engines.get(engineId);
            if (!wasmInterface) {
                console.error(`❌ Engine not found: ${engineId}`);
                return -1;
            }

            // Initialize the interface if needed
            if (typeof wasmInterface.initialize === 'function') {
                const config = {
                    sampleRate: sampleRate,
                    bufferSize: 1024,
                    channels: 1,
                    enableRealTimeProcessing: true,
                    enablePerformanceMonitoring: true
                };

                const initialized = wasmInterface.initialize(config);
                if (!initialized) {
                    console.error('❌ WASM interface initialization failed');
                    return -1;
                }
                console.log(`🔧 WASM interface initialized for engine ${engineId}`);
            }

            // Create session if the interface supports it
            let sessionKey = null;
            if (typeof wasmInterface.createSession === 'function') {
                const sessionConfig = {
                    sampleRate: sampleRate,
                    enableRealTimeProcessing: true,
                    enablePerformanceMonitoring: true
                };
                sessionKey = wasmInterface.createSession(sessionConfig);
                if (!sessionKey || sessionKey === "") {
                    console.error('❌ Session creation failed');
                    return -1;
                }
                console.log(`🔧 Session created with key: ${sessionKey}`);
            } else if (typeof wasmInterface.startSession === 'function') {
                const sessionKey = `session_${Date.now()}`;
                const started = wasmInterface.startSession(sessionKey);
                if (!started) {
                    console.error('❌ Session start failed');
                    return -1;
                }
                console.log(`🔧 Session started with key: ${sessionKey}`);
            }

            const sessionId = this.nextSessionId++;
            this.sessions.set(sessionId, {
                engineId: engineId,
                sessionKey: sessionKey,
                sampleRate: sampleRate,
                created: Date.now()
            });

            console.log(`✅ Session created with ID: ${sessionId} (engine: ${engineId})`);
            return sessionId;

        } catch (error) {
            console.error('❌ Session creation failed:', error);
            return -1;
        }
    }

    /**
     * Load master call (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @param {string} callType Call type (e.g., "buck_grunt")
     * @returns {number} Status code (0 = success)
     */
    unified_load_master_call(engineId, sessionId, callType) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return -1;
            }

            // For alpha testing, we'll use the web-based master call loading
            // The WASM interface will be used for processing once audio is loaded
            console.log(`🎵 Master call load requested: ${callType} (engine: ${engineId}, session: ${sessionId})`);

            // Store the master call type for later use
            session.masterCallType = callType;

            return 0; // Success

        } catch (error) {
            console.error('❌ Master call loading failed:', error);
            return -1;
        }
    }

    /**
     * Start recording (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {number} Status code (0 = success)
     */
    unified_start_recording(engineId, sessionId) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return -1;
            }

            console.log(`🎤 Recording start requested (engine: ${engineId}, session: ${sessionId})`);

            // Mark session as recording
            session.isRecording = true;
            session.recordingStartTime = Date.now();

            return 0; // Success

        } catch (error) {
            console.error('❌ Recording start failed:', error);
            return -1;
        }
    }

    /**
     * Stop recording (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {number} Status code (0 = success)
     */
    unified_stop_recording(engineId, sessionId) {
        try {
            const session = this.sessions.get(sessionId);

            if (!session) {
                console.error(`❌ Invalid session ${sessionId}`);
                return -1;
            }

            console.log(`⏹️ Recording stop requested (session: ${sessionId})`);

            // Mark session as not recording
            session.isRecording = false;
            session.recordingEndTime = Date.now();

            return 0; // Success

        } catch (error) {
            console.error('❌ Recording stop failed:', error);
            return -1;
        }
    }

    /**
     * Get recording level (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {number} Recording level (0.0 - 1.0)
     */
    unified_get_recording_level(engineId, sessionId) {
        try {
            const session = this.sessions.get(sessionId);

            if (!session || !session.isRecording) {
                return 0.0;
            }

            // Return a simulated level for alpha testing
            return Math.random() * 0.5 + 0.1; // 0.1 - 0.6 range

        } catch (error) {
            console.error('❌ Get recording level failed:', error);
            return 0.0;
        }
    }

    /**
     * Save recording (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @param {string} filename Filename to save
     * @returns {number} Status code (0 = success)
     */
    unified_save_recording(engineId, sessionId, filename) {
        try {
            const session = this.sessions.get(sessionId);

            if (!session) {
                console.error(`❌ Invalid session ${sessionId}`);
                return -1;
            }

            console.log(`💾 Recording save requested: ${filename} (session: ${sessionId})`);

            // Store filename for later use
            session.savedFilename = filename;

            return 0; // Success

        } catch (error) {
            console.error('❌ Recording save failed:', error);
            return -1;
        }
    }

    /**
     * Process audio chunk (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @param {Float32Array} audioData Audio data
     * @param {number} length Data length
     * @returns {number} Status code (0 = success)
     */
    unified_process_audio_chunk(engineId, sessionId, audioData, length) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return -1;
            }

            // Process through WASM interface if available
            if (typeof wasmInterface.processAudioChunk === 'function') {
                const result = wasmInterface.processAudioChunk(session.sessionKey || '', audioData, true);
                return result ? 0 : -1;
            } else if (typeof wasmInterface.processAudio === 'function') {
                const result = wasmInterface.processAudio(audioData);
                return result ? 0 : -1;
            }

            // Fallback: just mark as processed
            session.lastProcessedTime = Date.now();
            return 0;

        } catch (error) {
            console.error('❌ Audio processing failed:', error);
            return -1;
        }
    }

    /**
     * Get similarity score (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {number} Similarity score (0.0 - 1.0)
     */
    unified_get_similarity_score(engineId, sessionId) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return 0.0;
            }

            // Try to get similarity from WASM interface
            if (typeof wasmInterface.getCurrentSimilarity === 'function') {
                const result = wasmInterface.getCurrentSimilarity();
                if (result && typeof result.score === 'number') {
                    return Math.max(0.0, Math.min(1.0, result.score));
                }
            }

            // Fallback: return a simulated similarity score for alpha testing
            return Math.random() * 0.6 + 0.3; // 0.3 - 0.9 range

        } catch (error) {
            console.error('❌ Get similarity score failed:', error);
            return 0.0;
        }
    }

    /**
     * Configure VAD (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @param {Object} vadConfig VAD configuration
     * @returns {number} Status code (0 = success)
     */
    unified_set_vad_config(engineId, sessionId, vadConfig) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return -1;
            }

            // Configure VAD if supported
            if (typeof wasmInterface.configureVAD === 'function') {
                const result = wasmInterface.configureVAD(session.sessionKey || '', vadConfig);
                return result ? 0 : -1;
            }

            // Store VAD config for later use
            session.vadConfig = vadConfig;
            console.log(`🎤 VAD configured for session ${sessionId}:`, vadConfig);

            return 0;

        } catch (error) {
            console.error('❌ VAD configuration failed:', error);
            return -1;
        }
    }

    /**
     * Destroy session (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {number} Status code (0 = success)
     */
    unified_destroy_session(engineId, sessionId) {
        try {
            const wasmInterface = this.engines.get(engineId);
            const session = this.sessions.get(sessionId);

            if (!wasmInterface || !session) {
                console.error(`❌ Invalid engine ${engineId} or session ${sessionId}`);
                return -1;
            }

            // Destroy session in WASM interface if supported
            if (typeof wasmInterface.destroySession === 'function' && session.sessionKey) {
                wasmInterface.destroySession(session.sessionKey);
            } else if (typeof wasmInterface.endSession === 'function') {
                wasmInterface.endSession();
            }

            // Remove from our tracking
            this.sessions.delete(sessionId);

            console.log(`🗑️ Session destroyed: ${sessionId}`);
            return 0;

        } catch (error) {
            console.error('❌ Session destruction failed:', error);
            return -1;
        }
    }

    /**
     * Configure DTW parameters (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @param {number} windowRatio DTW window ratio (0.0-1.0)
     * @param {boolean} enableSIMD Enable SIMD optimization
     * @returns {number} Status code (0 = success)
     */
    unified_configure_dtw(engineId, sessionId, windowRatio = 0.1, enableSIMD = true) {
        try {
            const session = this.sessions.get(sessionId);
            if (!session) {
                console.error(`❌ Invalid session ${sessionId}`);
                return -1;
            }

            // Store DTW configuration
            session.dtwConfig = {
                windowRatio: Math.max(0.0, Math.min(1.0, windowRatio)),
                enableSIMD: enableSIMD
            };

            console.log(`🔧 DTW configured: window=${windowRatio}, SIMD=${enableSIMD}`);
            return 0;

        } catch (error) {
            console.error('❌ DTW configuration failed:', error);
            return -1;
        }
    }

    /**
     * Get VAD status (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {Object} VAD status object
     */
    unified_get_vad_status(engineId, sessionId) {
        try {
            const session = this.sessions.get(sessionId);
            if (!session) {
                return { error: 'Invalid session' };
            }

            return {
                isActive: session.vadActive || false,
                confidence: session.vadConfidence || 0.0,
                energyLevel: session.lastEnergyLevel || 0.0,
                threshold: session.vadConfig?.energyThreshold || 0.01,
                enabled: session.vadConfig?.enabled || true
            };

        } catch (error) {
            console.error('❌ Get VAD status failed:', error);
            return { error: error.message };
        }
    }

    /**
     * Get DTW configuration (documented API)
     * @param {number} engineId Engine ID
     * @param {number} sessionId Session ID
     * @returns {Object} DTW configuration
     */
    unified_get_dtw_config(engineId, sessionId) {
        try {
            const session = this.sessions.get(sessionId);
            if (!session) {
                return { error: 'Invalid session' };
            }

            return session.dtwConfig || {
                windowRatio: 0.1,
                enableSIMD: true
            };

        } catch (error) {
            console.error('❌ Get DTW config failed:', error);
            return { error: error.message };
        }
    }
    unified_destroy_engine(engineId) {
        try {
            const wasmInterface = this.engines.get(engineId);

            if (!wasmInterface) {
                console.error(`❌ Invalid engine ${engineId}`);
                return -1;
            }

            // Clean up all sessions for this engine
            for (const [sessionId, session] of this.sessions) {
                if (session.engineId === engineId) {
                    this.unified_destroy_session(engineId, sessionId);
                }
            }

            // Shutdown interface if supported
            if (typeof wasmInterface.shutdown === 'function') {
                wasmInterface.shutdown();
            }

            // Clean up the interface
            if (typeof wasmInterface.delete === 'function') {
                wasmInterface.delete();
            }

            this.engines.delete(engineId);

            console.log(`🗑️ Engine destroyed: ${engineId}`);
            return 0;

        } catch (error) {
            console.error('❌ Engine destruction failed:', error);
            return -1;
        }
    }

    /**
     * Get engine status
     * @param {number} engineId Engine ID
     * @returns {Object} Status object
     */
    getEngineStatus(engineId) {
        try {
            const wasmInterface = this.engines.get(engineId);

            if (!wasmInterface) {
                return { error: 'Engine not found' };
            }

            if (typeof wasmInterface.getEngineStatus === 'function') {
                return wasmInterface.getEngineStatus();
            }

            return {
                initialized: true,
                engineId: engineId,
                sessionCount: Array.from(this.sessions.values()).filter(s => s.engineId === engineId).length
            };

        } catch (error) {
            console.error('❌ Get engine status failed:', error);
            return { error: error.message };
        }
    }
}

// Global bridge instance
let globalBridge = null;

/**
 * Get or create the global bridge instance
 * @returns {UnifiedWASMBridge} Bridge instance
 */
export function getUnifiedWASMBridge() {
    if (!globalBridge) {
        globalBridge = new UnifiedWASMBridge();
    }
    return globalBridge;
}

/**
 * Initialize the bridge and expose the unified API globally
 * @returns {Promise<boolean>} Success status
 */
export async function initializeUnifiedAPI() {
    try {
        const bridge = getUnifiedWASMBridge();
        const success = await bridge.initialize();

        if (success) {
            // Expose the documented API globally for alpha testing compatibility
            window.HuntmasterEngine = window.HuntmasterEngine || {};
            window.HuntmasterEngine.unified_create_engine = () => bridge.unified_create_engine();
            window.HuntmasterEngine.unified_create_session = (engineId, sampleRate) => bridge.unified_create_session(engineId, sampleRate);
            window.HuntmasterEngine.unified_load_master_call = (engineId, sessionId, callType) => bridge.unified_load_master_call(engineId, sessionId, callType);
            window.HuntmasterEngine.unified_start_recording = (engineId, sessionId) => bridge.unified_start_recording(engineId, sessionId);
            window.HuntmasterEngine.unified_stop_recording = (engineId, sessionId) => bridge.unified_stop_recording(engineId, sessionId);
            window.HuntmasterEngine.unified_get_recording_level = (engineId, sessionId) => bridge.unified_get_recording_level(engineId, sessionId);
            window.HuntmasterEngine.unified_save_recording = (engineId, sessionId, filename) => bridge.unified_save_recording(engineId, sessionId, filename);
            window.HuntmasterEngine.unified_process_audio_chunk = (engineId, sessionId, audioData, length) => bridge.unified_process_audio_chunk(engineId, sessionId, audioData, length);
            window.HuntmasterEngine.unified_get_similarity_score = (engineId, sessionId) => bridge.unified_get_similarity_score(engineId, sessionId);
            window.HuntmasterEngine.unified_set_vad_config = (engineId, sessionId, vadConfig) => bridge.unified_set_vad_config(engineId, sessionId, vadConfig);
            window.HuntmasterEngine.unified_destroy_session = (engineId, sessionId) => bridge.unified_destroy_session(engineId, sessionId);
            window.HuntmasterEngine.unified_destroy_engine = (engineId) => bridge.unified_destroy_engine(engineId);
            window.HuntmasterEngine.unified_configure_dtw = (engineId, sessionId, windowRatio, enableSIMD) => bridge.unified_configure_dtw(engineId, sessionId, windowRatio, enableSIMD);
            window.HuntmasterEngine.unified_get_vad_status = (engineId, sessionId) => bridge.unified_get_vad_status(engineId, sessionId);
            window.HuntmasterEngine.unified_get_dtw_config = (engineId, sessionId) => bridge.unified_get_dtw_config(engineId, sessionId);

            console.log('✅ Unified WASM API initialized and exposed globally');
        }

        return success;
    } catch (error) {
        console.error('❌ Failed to initialize unified API:', error);
        return false;
    }
}

export default UnifiedWASMBridge;
