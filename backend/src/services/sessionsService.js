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
const GameCallsService = require('./gameCallsService');
const preprocessor = require('./masterCallPreprocessor');

// [20251030-ENGINE-001] Load GameCalls Engine native bindings (REAL C++ ENGINE ONLY)
let gameCallsEngine;
let usingRealEngine = true;

try {
    // [20251102-DOCKER-001] Support both Docker and host environments for bindings path
    // Docker: /app/bindings/... (workdir is /app)
    // Host: /home/xbyooki/projects/hma-gamecalls-engine/bindings/...
    let bindingsPath;
    if (process.env.NODE_ENV === 'production' || process.env.DOCKER_CONTAINER === 'true') {
        // Running in Docker container
        bindingsPath = path.join('/app', 'bindings/node-api/build/Release/gamecalls_engine.node');
    } else {
        // Running on host (for development)
        bindingsPath = path.join(__dirname, '../../../bindings/node-api/build/Release/gamecalls_engine.node');
    }
    
    gameCallsEngine = require(bindingsPath);
    console.log('✅ Using REAL C++ GameCalls Engine bindings');
    console.log(`   Loaded from: ${bindingsPath}`);
} catch (error) {
    console.error('❌ FATAL: Failed to load C++ GameCalls Engine bindings');
    console.error(`   Error: ${error.message}`);
    console.error('   The mock engine has been disabled. Build the Node-API bindings first.');
    console.error('   Run: cd /home/xbyooki/projects/hma-gamecalls-engine/bindings/node-api && npm install');
    throw new Error('C++ GameCalls Engine bindings not found. Mock engine disabled.');
}

// [20251028-STORAGE-011] Load MinIO service for master call storage
const minioService = require('./minioService');

// [20251030-PERSIST-001] Load database service for analysis persistence
const databaseService = require('./databaseService');

// [20251028-CACHE-020] Load Redis service for session caching
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
            // [20251030-ENGINE-003] Real engine uses initializeEngine() method
            await gameCallsEngine.initializeEngine();
            this.engineInitialized = true;
        }
    }

    /**
     * [20251028-API-005] Create new analysis session with C++ engine
     * [20251029-TEST-002] Updated to use GameCallsService for master call retrieval
     */
    static async createSession(masterCallId, options = {}) {
        try {
            await this.ensureEngineInitialized();
            
            // [20251029-TEST-003] Get master call metadata from database
            const masterCall = await GameCallsService.getCall(masterCallId);
            if (!masterCall) {
                throw ApiError.notFound('MASTER_CALL_NOT_FOUND', `Master call ${masterCallId} not found`);
            }
            
            // [20251029-TEST-004] Download master call from MinIO to temp location
            const tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'gamecalls-'));
            const masterCallPath = path.join(tempDir, `${masterCallId}.wav`);
            
            // Get audio file from MinIO using the stored path
            const audioPath = masterCall.audioFilePath.replace('master-calls/', '');
            const stream = await minioService.getObjectStream('gamecalls-master-calls', audioPath);
            
            // Write stream to temp file
            const writeStream = require('fs').createWriteStream(masterCallPath);
            await new Promise((resolve, reject) => {
                stream.pipe(writeStream);
                stream.on('error', reject);
                writeStream.on('finish', resolve);
                writeStream.on('error', reject);
            });
            
            console.log(`📥 Downloaded master call: ${masterCallPath}`);
            
            // [20251028-API-007] Create session in C++ engine via Node-API bindings
            let nativeSessionId;
            try {
                nativeSessionId = await gameCallsEngine.createSession(masterCallPath, {
                    sampleRate: options.sampleRate || 44100,
                    enableEnhancedAnalysis: options.enableEnhancedAnalysis !== false
                });
            } catch (engineError) {
                console.error(`❌ C++ Engine Error Details:`, engineError);
                console.error(`   File path: ${masterCallPath}`);
                console.error(`   File exists: ${require('fs').existsSync(masterCallPath)}`);
                if (require('fs').existsSync(masterCallPath)) {
                    const stats = require('fs').statSync(masterCallPath);
                    console.error(`   File size: ${stats.size} bytes`);
                }
                throw new Error(`Failed to load master call: ${masterCallPath}: ${engineError.message}`);
            }
            
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
            
            // [20251030-PERSIST-004] Insert session into gamecalls_sessions table
            try {
                await databaseService.raw(`
                    INSERT INTO gamecalls_sessions (
                        id, master_call_id, status, sample_rate, created_at, updated_at
                    ) VALUES ($1, $2, $3, $4, $5, $6)
                `, [
                    sessionId,
                    masterCallId,
                    'created',
                    session.sampleRate,
                    now,
                    now
                ]);
                console.log(`💾 Persisted session ${sessionId} to database`);
            } catch (dbError) {
                console.error('Failed to persist session to database:', dbError.message);
                // Continue anyway - session is in Redis
            }
            
            // [20251028-CACHE-016] Cache session in Redis with TTL
            await redisService.setSession(sessionId, session, 3600);

            return session;

        } catch (error) {
            // [20251030-FIX-002] Preserve ApiError status codes
            if (error instanceof ApiError) throw error;
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
            
            // [20251030-PERSIST-005] Update session status in database
            try {
                await databaseService.raw(`
                    UPDATE gamecalls_sessions 
                    SET status = $1, started_at = $2, updated_at = $3
                    WHERE id = $4
                `, ['recording', session.startedAt, session.updatedAt, sessionId]);
            } catch (dbError) {
                console.error('Failed to update session in database:', dbError.message);
            }
            
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
            // [20251030-FIX-004] Handle Int16 PCM from tests, convert to normalized float [-1.0, 1.0]
            let audioBuffer;
            if (audioData instanceof Float32Array) {
                audioBuffer = audioData;
            } else if (Array.isArray(audioData)) {
                // Array of samples - assume already Float32 normalized
                audioBuffer = new Float32Array(audioData);
            } else if (Buffer.isBuffer(audioData)) {
                throw new Error('Raw Buffer not supported - use Int16Array conversion in controller');
            } else {
                throw new Error('Invalid audio data format. Expected Float32Array or Array of floats');
            }

            // [20251030-DEBUG-001] Detailed logging for engine rejection debugging
            console.log(`🔊 Processing audio chunk for session ${sessionId}`);
            console.log(`   Native Session ID: ${session.nativeSessionId}`);
            console.log(`   Master Call: ${session.masterCallId}`);
            console.log(`   Session Status: ${session.status}`);
            console.log(`   Buffer Length: ${audioBuffer.length} samples`);
            console.log(`   Buffer Type: ${audioBuffer.constructor.name}`);
            console.log(`   Buffer Stats: min=${Math.min(...audioBuffer).toFixed(4)}, max=${Math.max(...audioBuffer).toFixed(4)}, mean=${(audioBuffer.reduce((a,b) => a+b, 0) / audioBuffer.length).toFixed(4)}`);
            console.log(`   Sample Rate: ${session.sampleRate} Hz`);
            
            // [20251028-API-012] Process through C++ engine via Node-API bindings
            let results;
            try {
                results = await gameCallsEngine.processAudio(session.nativeSessionId, audioBuffer);
                console.log(`   ✅ Engine processed successfully: similarity=${results.similarityScore?.toFixed(4) || 'N/A'}, readiness=${results.readiness}`);
            } catch (engineError) {
                // [20251030-DEBUG-002] Log detailed engine error context
                console.error(`   ❌ Engine rejected audio chunk:`);
                console.error(`      Error: ${engineError.message}`);
                console.error(`      Session ID: ${sessionId} (native: ${session.nativeSessionId})`);
                console.error(`      Master Call: ${session.masterCallId}`);
                console.error(`      Buffer: ${audioBuffer.length} samples, ${audioBuffer.constructor.name}`);
                console.error(`      Engine Error Type: ${engineError.constructor.name}`);
                if (engineError.stack) {
                    console.error(`      Stack: ${engineError.stack.split('\n').slice(0, 3).join('\n')}`);
                }
                throw engineError;
            }

            // [20251029-TEST-018] Transform engine results to expected API format
            const similarity = {
                overall: results.similarityScore || 0,
                mfcc: results.similarityScore || 0, // Engine returns single score for now
                volume: results.levels?.rms || 0,
                timing: 0, // Not yet implemented
                pitch: results.pitch?.pitch || 0,
                confidence: results.confidence || 0,
                isReliable: results.readiness === 'ready',
                isMatch: results.similarityScore > 0.7,
                samplesAnalyzed: audioBuffer.length
            };

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
                similarity,
                pitch: results.pitch,
                harmonic: results.harmonic,
                cadence: results.cadence,
                levels: results.levels,
                readiness: results.readiness,
                timestamp: new Date().toISOString()
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to process audio data: ${error.message}`);
        }
    }

    /**
     * [20251029-API-017] Stop and finalize session analysis via C++ engine
     */
    static async stopSession(sessionId) {
        try {
            const session = await this.getSession(sessionId);

            if (session.status !== 'recording') {
                throw ApiError.badRequest('INVALID_SESSION_STATE', 'Session must be in recording state to stop');
            }

            // [20251030-ENGINE-004] Try to finalize, but handle insufficient audio gracefully
            let finalAnalysis;
            try {
                finalAnalysis = await gameCallsEngine.finalizeSession(session.nativeSessionId);
            } catch (finalizeError) {
                console.warn(`⚠️  Session ${sessionId} finalization failed: ${finalizeError.message}`);
                console.warn('   Using default analysis (insufficient audio data)');
                
                // [20251030-ENGINE-005] Provide default analysis when finalization fails
                finalAnalysis = {
                    pitch: { pitchHz: 0, confidence: 0, grade: 'F' },
                    harmonic: { fundamental: 0, confidence: 0, grade: 'F' },
                    cadence: { tempoBPM: 0, confidence: 0, grade: 'F' },
                    finalize: { 
                        similarityAtFinalize: 0,
                        normalizationScalar: 1.0,
                        loudnessDeviation: 0,
                        segmentStartMs: 0,
                        segmentDurationMs: 0
                    },
                    valid: false,
                    finalized: false
                };
            }

            // [20251029-API-019] Convert C++ EnhancedAnalysisSummary to API format
            // Real engine returns: pitch, harmonic, cadence, finalize, valid, finalized
            // [20251030-FIX-003] Safer confidence calculation with fallbacks
            const pitchConf = finalAnalysis.pitch?.confidence ?? 0;
            const harmonicConf = finalAnalysis.harmonic?.confidence ?? 0;
            const cadenceConf = finalAnalysis.cadence?.confidence ?? 0;
            const avgConfidence = pitchConf || harmonicConf || cadenceConf 
                ? (pitchConf + harmonicConf + cadenceConf) / 3 
                : 0;
            
            const analysis = {
                // Use finalize similarity as overall score if available
                overallScore: finalAnalysis.finalize?.similarityAtFinalize || 0,
                similarityScore: finalAnalysis.finalize?.similarityAtFinalize || 0,
                confidence: avgConfidence,
                segments: finalAnalysis.finalize ? [{
                    startTime: finalAnalysis.finalize.segmentStartMs / 1000,
                    endTime: (finalAnalysis.finalize.segmentStartMs + finalAnalysis.finalize.segmentDurationMs) / 1000,
                    duration: finalAnalysis.finalize.segmentDurationMs / 1000,
                    isBestSegment: true
                }] : [],
                enhanced: {
                    pitch: finalAnalysis.pitch || {},
                    harmonic: finalAnalysis.harmonic || {},
                    cadence: finalAnalysis.cadence || {},
                    loudness: finalAnalysis.finalize ? {
                        normalizationScalar: finalAnalysis.finalize.normalizationScalar,
                        loudnessDeviation: finalAnalysis.finalize.loudnessDeviation
                    } : {}
                },
                valid: finalAnalysis.valid || false,
                finalized: finalAnalysis.finalized || false,
                feedback: {
                    strengths: [],
                    improvements: [],
                    grade: this.calculateGrade(finalAnalysis.finalize?.similarityAtFinalize || 0)
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
            
            // [20251030-PERSIST-006] Update session status to completed in database
            try {
                await databaseService.raw(`
                    UPDATE gamecalls_sessions 
                    SET status = $1, completed_at = $2, updated_at = $3
                    WHERE id = $4
                `, ['completed', session.completedAt, session.updatedAt, sessionId]);
                console.log(`💾 Updated session ${sessionId} status to completed in database`);
            } catch (dbError) {
                console.error('Failed to update session completion in database:', dbError.message);
            }
            
            // [20251028-STORAGE-013] Store analysis results in MinIO
            try {
                await minioService.storeAnalysisResults(sessionId, analysis);
                console.log(`💾 Stored analysis results for session: ${sessionId}`);
            } catch (minioError) {
                console.warn(`⚠️  Failed to store analysis in MinIO: ${minioError.message}`);
            }
            
            // [20251030-PERSIST-002] Persist analysis to PostgreSQL
            try {
                const minioResultsKey = `sessions/${sessionId}/results.json`;
                await this.persistAnalysisToDatabase(sessionId, analysis, minioResultsKey);
                console.log(`💾 Persisted analysis to database for session: ${sessionId}`);
            } catch (persistError) {
                console.warn(`⚠️  Failed to persist analysis to database: ${persistError.message}`);
            }
            
            // [20251028-CACHE-021] Update Redis with final state and cache analysis
            try {
                await redisService.updateSession(sessionId, {
                    status: 'completed',
                    completedAt: session.completedAt,
                    analysis: session.analysis
                }, 7200); // Cache completed sessions for 2 hours
            } catch (cacheError) {
                console.warn(`⚠️  Failed to cache session: ${cacheError.message}`);
            }

            return {
                sessionId: session.id,
                status: session.status,
                analysis: session.analysis
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to stop session: ${error.message}`);
        }
    }
    
    /**
     * [20251029-API-020] Get real-time similarity score (for testing and monitoring)
     */
    static async getSimilarityScore(sessionId) {
        try {
            const session = await this.getSession(sessionId);
            const similarity = await gameCallsEngine.getSimilarityScore(session.nativeSessionId);
            
            return {
                sessionId,
                similarity,
                timestamp: new Date().toISOString()
            };
        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to get similarity score: ${error.message}`);
        }
    }
    
    /**
     * [20251029-API-021] Finalize session without stopping (for testing)
     */
    static async finalizeSession(sessionId) {
        try {
            const session = await this.getSession(sessionId);
            const rawAnalysis = await gameCallsEngine.finalizeSession(session.nativeSessionId);
            
            // [20251030-FIX-001] Normalize analysis format - C++ engine returns fields at root level
            // Extract enhanced fields if they exist, otherwise use raw fields
            const analysis = {
                pitch: rawAnalysis.pitch || rawAnalysis.enhanced?.pitch || {},
                harmonic: rawAnalysis.harmonic || rawAnalysis.enhanced?.harmonic || {},
                cadence: rawAnalysis.cadence || rawAnalysis.enhanced?.cadence || {},
                finalize: rawAnalysis.finalize || rawAnalysis || {},
                valid: rawAnalysis.valid !== undefined ? rawAnalysis.valid : true,
                finalized: rawAnalysis.finalized !== undefined ? rawAnalysis.finalized : true
            };
            
            // [20251102-FIX-017] Extract similarity from finalize.similarityAtFinalize for pairwise matrix
            const similarity = rawAnalysis.finalize?.similarityAtFinalize || 0;
            
            return {
                sessionId,
                analysis,
                similarity,  // Top-level similarity for easy access
                timestamp: new Date().toISOString()
            };
        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to finalize session: ${error.message}`);
        }
    }
    
    /**
     * [20251029-API-022] Process audio with alias for compatibility
     */
    static async processAudio(sessionId, audioBuffer) {
        return this.processAudioData(sessionId, audioBuffer);
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
    
    /**
     * [20251030-PERSIST-003] Persist analysis to PostgreSQL gamecalls_analysis table
     */
    static async persistAnalysisToDatabase(sessionId, analysis, minioResultsKey) {
        try {
            const query = `
                INSERT INTO gamecalls_analysis (
                    session_id,
                    overall_score,
                    similarity_score,
                    confidence,
                    grade,
                    pitch_score,
                    pitch_frequency,
                    pitch_confidence,
                    harmonic_score,
                    harmonic_spectral_centroid,
                    harmonic_confidence,
                    cadence_score,
                    cadence_bpm,
                    cadence_rhythm_strength,
                    loudness_rms,
                    loudness_peak,
                    loudness_normalization,
                    processing_time_ms,
                    minio_results_key,
                    minio_recording_key,
                    analyzed_at
                ) VALUES (
                    $1, $2, $3, $4, $5, $6, $7, $8, $9, $10,
                    $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21
                )
                ON CONFLICT (session_id) DO UPDATE SET
                    overall_score = EXCLUDED.overall_score,
                    similarity_score = EXCLUDED.similarity_score,
                    confidence = EXCLUDED.confidence,
                    grade = EXCLUDED.grade,
                    pitch_score = EXCLUDED.pitch_score,
                    pitch_frequency = EXCLUDED.pitch_frequency,
                    pitch_confidence = EXCLUDED.pitch_confidence,
                    harmonic_score = EXCLUDED.harmonic_score,
                    harmonic_spectral_centroid = EXCLUDED.harmonic_spectral_centroid,
                    harmonic_confidence = EXCLUDED.harmonic_confidence,
                    cadence_score = EXCLUDED.cadence_score,
                    cadence_bpm = EXCLUDED.cadence_bpm,
                    cadence_rhythm_strength = EXCLUDED.cadence_rhythm_strength,
                    loudness_rms = EXCLUDED.loudness_rms,
                    loudness_peak = EXCLUDED.loudness_peak,
                    loudness_normalization = EXCLUDED.loudness_normalization,
                    processing_time_ms = EXCLUDED.processing_time_ms,
                    minio_results_key = EXCLUDED.minio_results_key,
                    minio_recording_key = EXCLUDED.minio_recording_key,
                    analyzed_at = EXCLUDED.analyzed_at
                RETURNING id
            `;
            
            const now = new Date().toISOString();
            
            const values = [
                sessionId,
                analysis.overallScore || 0,
                analysis.similarityScore || 0,
                analysis.confidence || 0,
                analysis.feedback?.grade || null,
                analysis.enhanced?.pitch?.score || null,
                analysis.enhanced?.pitch?.frequency || null,
                analysis.enhanced?.pitch?.confidence || null,
                analysis.enhanced?.harmonic?.score || null,
                analysis.enhanced?.harmonic?.spectralCentroid || null,
                analysis.enhanced?.harmonic?.confidence || null,
                analysis.enhanced?.cadence?.score || null,
                analysis.enhanced?.cadence?.bpm || null,
                analysis.enhanced?.cadence?.rhythmStrength || null,
                analysis.enhanced?.loudness?.rms || null,
                analysis.enhanced?.loudness?.peak || null,
                analysis.enhanced?.loudness?.normalizationScalar || null,
                analysis.processingTimeMs || null,
                minioResultsKey,
                null, // minio_recording_key - TODO: get from session data
                now
            ];
            
            const result = await databaseService.raw(query, values);
            
            return result.rows[0];
            
        } catch (error) {
            console.error('Failed to persist analysis to database:', error);
            // Don't throw - analysis is already stored in MinIO, DB is secondary
        }
    }
}

module.exports = SessionsService;