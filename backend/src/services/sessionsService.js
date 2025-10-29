// [20251028-API-001] Sessions Service with Node-API bindings integration
/**
 * Sessions Service
 * Manages audio analysis sessions and real-time processing
 * Interfaces with the C++ UnifiedAudioEngine via Node-API bindings
 */

const { ApiError } = require('../middleware/errorHandler');
const { v4: uuidv4 } = require('uuid');
const path = require('path');
const fs = require('fs').promises;
const os = require('os');

// [20251028-API-002] Load GameCalls Engine native bindings
const gameCallsEngine = require('../../../bindings/node-api/lib/index');

// [20251028-STORAGE-011] Load MinIO service for master call storage
const minioService = require('./minioService');

// [20251028-CACHE-014] Load Redis service for distributed session caching
const redisService = require('./redisService');

// [20251028-API-003] Session state cache (maps UUID to native session ID)
// [20251028-CACHE-015] Using Redis for distributed deployment, Map as fallback
class SessionsService {

    // In-memory fallback (used when Redis unavailable)
    static sessions = new Map();
    
    // Track if engine is initialized
    static engineInitialized = false;

    // [20251028-API-004] Initialize engine on first use
    static async ensureEngineInitialized() {
        if (!this.engineInitialized) {
            await gameCallsEngine.initialize();
            this.engineInitialized = true;
        }
    }

    /**
     * [20251028-API-005] Create new analysis session with C++ engine
     */
    static async createSession(masterCallId, options = {}) {
        try {
            await this.ensureEngineInitialized();
            
            // [20251028-STORAGE-012] Download master call from MinIO to temp location
            const tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'gamecalls-'));
            const masterCallPath = path.join(tempDir, `${masterCallId}.wav`);
            
            await minioService.downloadMasterCall(masterCallId, masterCallPath);
            console.log(`📥 Downloaded master call: ${masterCallPath}`);
            
            // [20251028-API-007] Create session in C++ engine via Node-API bindings
            const nativeSessionId = await gameCallsEngine.createSession(masterCallPath, {
                sampleRate: options.sampleRate || 44100,
                enableEnhancedAnalysis: options.enableEnhancedAnalysis !== false
            });
            
            // [20251028-API-008] Generate public UUID for API consumers
            const sessionId = uuidv4();
            const now = new Date().toISOString();

            const session = {
                id: sessionId,
                nativeSessionId, // Internal C++ engine session ID
                masterCallId,
                masterCallPath, // Temp file location
                tempDir, // For cleanup
                status: 'created',
                sampleRate: options.sampleRate || 44100,
                bufferSize: options.bufferSize || 1024,
                enableEnhancedAnalysis: options.enableEnhancedAnalysis !== false,
                settings: {
                    pitchAnalysis: options.pitchAnalysis !== false,
                    harmonicAnalysis: options.harmonicAnalysis !== false,
                    cadenceAnalysis: options.cadenceAnalysis !== false,
                    realTimeScoring: options.realTimeScoring !== false
                },
                metrics: {
                    similarity: 0,
                    pitch: { score: 0, confidence: 0 },
                    harmonic: { score: 0, confidence: 0 },
                    cadence: { score: 0, confidence: 0 },
                    loudness: { rms: 0, peak: 0, normalization: 1.0 },
                    readiness: 'not_ready'
                },
                analysis: {
                    segments: [],
                    vadActive: false,
                    bestSegment: null,
                    overallScore: 0
                },
                createdAt: now,
                updatedAt: now,
                startedAt: null,
                completedAt: null
            };

            this.sessions.set(sessionId, session);
            
            // [20251028-CACHE-016] Cache session in Redis with TTL
            await redisService.setSession(sessionId, session, 3600);

            return session;

        } catch (error) {
            throw new Error(`Failed to create session: ${error.message}`);
        }
    }

    /**
     * Get session by ID
     */
    static async getSession(sessionId) {
        try {
            // [20251028-CACHE-017] Try Redis first
            let session = await redisService.getSession(sessionId);
            
            if (session) {
                console.log(`✅ Session cache HIT: ${sessionId}`);
                return session;
            }
            
            // [20251028-CACHE-018] Fallback to in-memory Map
            session = this.sessions.get(sessionId);

            if (!session) {
                throw ApiError.notFound('SESSION_NOT_FOUND', `Session with ID ${sessionId} not found`);
            }
            
            // Repopulate Redis cache
            await redisService.setSession(sessionId, session, 3600);

            return session;

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to get session: ${error.message}`);
        }
    }

    /**
     * Start recording/analysis session
     */
    static async startSession(sessionId) {
        try {
            // TODO: Interface with C++ engine to start real-time processing
            
            const session = await this.getSession(sessionId);

            if (session.status !== 'created') {
                throw ApiError.badRequest('INVALID_SESSION_STATE', 'Session must be in created state to start');
            }

            session.status = 'recording';
            session.startedAt = new Date().toISOString();
            session.updatedAt = new Date().toISOString();

            this.sessions.set(sessionId, session);
            
            // [20251028-CACHE-019] Update Redis cache
            await redisService.updateSession(sessionId, { 
                status: 'recording', 
                startedAt: session.startedAt 
            });

            return session;

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to start session: ${error.message}`);
        }
    }

    /**
     * [20251028-API-010] Process audio data for session via C++ engine
     */
    static async processAudioData(sessionId, audioData) {
        try {
            const session = await this.getSession(sessionId);

            if (session.status !== 'recording') {
                throw ApiError.badRequest('INVALID_SESSION_STATE', 'Session must be in recording state to process audio');
            }

            // [20251028-API-011] Convert audio data to Float32Array if needed
            let audioBuffer;
            if (audioData instanceof Float32Array) {
                audioBuffer = audioData;
            } else if (Buffer.isBuffer(audioData)) {
                // Convert Buffer to Float32Array (assumes 32-bit float PCM)
                audioBuffer = new Float32Array(audioData.buffer, audioData.byteOffset, audioData.length / 4);
            } else if (Array.isArray(audioData)) {
                audioBuffer = new Float32Array(audioData);
            } else {
                throw new Error('Invalid audio data format. Expected Float32Array, Buffer, or Array');
            }

            // [20251028-API-012] Process through C++ engine via Node-API bindings
            const results = await gameCallsEngine.processAudio(session.nativeSessionId, audioBuffer);

            // [20251028-API-013] Update session metrics with real analysis
            session.metrics = {
                similarity: results.similarityScore,
                pitch: {
                    score: results.pitch.pitch,
                    confidence: results.pitch.confidence,
                    frequency: results.pitch.pitch
                },
                harmonic: {
                    score: results.harmonic.harmonicity,
                    confidence: results.harmonic.harmonicity,
                    spectralCentroid: results.harmonic.spectralCentroid
                },
                cadence: {
                    score: results.cadence.tempo,
                    confidence: results.cadence.rhythmStrength,
                    bpm: results.cadence.tempo
                },
                loudness: {
                    rms: results.levels.rms,
                    peak: results.levels.peak,
                    normalization: 1.0
                },
                readiness: results.readiness,
                vadActive: results.readiness === 'ready'
            };

            session.updatedAt = new Date().toISOString();
            this.sessions.set(sessionId, session);
            
            // [20251028-CACHE-020] Update metrics in Redis cache
            await redisService.updateSession(sessionId, { metrics: session.metrics });

            return {
                ...results,
                timestamp: new Date().toISOString()
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to process audio data: ${error.message}`);
        }
    }

    /**
     * [20251028-API-014] Stop and finalize session analysis via C++ engine
     */
    static async stopSession(sessionId) {
        try {
            const session = await this.getSession(sessionId);

            if (session.status !== 'recording') {
                throw ApiError.badRequest('INVALID_SESSION_STATE', 'Session must be in recording state to stop');
            }

            // [20251028-API-015] Run finalization in C++ engine (segment selection, refined DTW)
            const finalAnalysis = await gameCallsEngine.finalizeSession(session.nativeSessionId);

            // [20251028-API-016] Convert C++ analysis to API format
            const analysis = {
                overallScore: finalAnalysis.overallScore,
                similarityScore: finalAnalysis.similarityScore,
                confidence: finalAnalysis.confidence,
                segments: [{
                    startTime: finalAnalysis.segment.startMs / 1000,
                    endTime: finalAnalysis.segment.endMs / 1000,
                    duration: finalAnalysis.segment.durationMs / 1000,
                    isBestSegment: true
                }],
                enhanced: {
                    pitch: finalAnalysis.enhanced.pitch,
                    harmonic: finalAnalysis.enhanced.harmonic,
                    cadence: finalAnalysis.enhanced.cadence,
                    loudness: finalAnalysis.enhanced.loudness
                },
                processingTimeMs: finalAnalysis.processingTimeMs,
                feedback: {
                    // TODO: Generate feedback based on scores
                    strengths: [],
                    improvements: [],
                    grade: this.calculateGrade(finalAnalysis.overallScore)
                }
            };

            session.status = 'completed';
            session.completedAt = new Date().toISOString();
            session.updatedAt = new Date().toISOString();
            session.analysis = {
                ...session.analysis,
                ...analysis
            };

            this.sessions.set(sessionId, session);
            
            // [20251028-STORAGE-013] Store analysis results in MinIO
            await minioService.storeAnalysisResults(sessionId, analysis);
            console.log(`💾 Stored analysis results for session: ${sessionId}`);
            
            // [20251028-CACHE-021] Update Redis with final state and cache analysis
            await redisService.updateSession(sessionId, {
                status: 'completed',
                completedAt: session.completedAt,
                analysis: session.analysis
            });
            await redisService.setAnalysisResults(sessionId, analysis, 86400); // 24 hour cache

            return session;

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to stop session: ${error.message}`);
        }
    }
    
    /**
     * [20251028-API-017] Calculate letter grade from score
     */
    static calculateGrade(score) {
        if (score >= 0.9) return 'A';
        if (score >= 0.8) return 'B';
        if (score >= 0.7) return 'C';
        if (score >= 0.6) return 'D';
        return 'F';
    }

    /**
     * [20251028-API-018] Delete session and cleanup C++ engine resources
     */
    static async deleteSession(sessionId) {
        try {
            const session = await this.getSession(sessionId);
            
            // [20251028-API-019] Destroy C++ engine session
            await gameCallsEngine.destroySession(session.nativeSessionId);
            
            // [20251028-STORAGE-014] Cleanup temp files
            if (session.tempDir) {
                try {
                    await fs.rm(session.tempDir, { recursive: true, force: true });
                    console.log(`🗑️  Cleaned up temp directory: ${session.tempDir}`);
                } catch (error) {
                    console.error(`Warning: Failed to cleanup temp directory: ${error.message}`);
                }
            }
            
            this.sessions.delete(sessionId);
            
            // [20251028-CACHE-022] Remove from Redis cache
            await redisService.deleteSession(sessionId);

            return { success: true };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to delete session: ${error.message}`);
        }
    }

    /**
     * Get session metrics (real-time)
     */
    static async getSessionMetrics(sessionId) {
        try {
            const session = await this.getSession(sessionId);

            return {
                sessionId,
                status: session.status,
                metrics: session.metrics,
                analysis: session.analysis,
                timestamp: new Date().toISOString()
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to get session metrics: ${error.message}`);
        }
    }

    /**
     * List active sessions
     */
    static async listSessions() {
        try {
            // [20251028-CACHE-023] Try Redis first for distributed deployment
            const redisSessionIds = await redisService.listSessions();
            
            if (redisSessionIds.length > 0) {
                const sessions = await Promise.all(
                    redisSessionIds.map(id => redisService.getSession(id))
                );
                
                const sessionList = sessions.filter(Boolean).map(session => ({
                    id: session.id,
                    masterCallId: session.masterCallId,
                    status: session.status,
                    createdAt: session.createdAt,
                    startedAt: session.startedAt,
                    completedAt: session.completedAt
                }));
                
                return {
                    sessions: sessionList,
                    total: sessionList.length
                };
            }
            
            // Fallback to in-memory Map
            const sessionList = Array.from(this.sessions.values()).map(session => ({
                id: session.id,
                masterCallId: session.masterCallId,
                status: session.status,
                createdAt: session.createdAt,
                startedAt: session.startedAt,
                completedAt: session.completedAt
            }));

            return {
                sessions: sessionList,
                total: sessionList.length
            };

        } catch (error) {
            throw new Error(`Failed to list sessions: ${error.message}`);
        }
    }
}

module.exports = SessionsService;