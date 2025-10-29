/**
 * Sessions Controller
 * Handles REST API requests for audio analysis session operations
 */

const { ApiError } = require('../middleware/errorHandler');
const SessionsService = require('../services/sessionsService');

class SessionsController {

    /**
     * GET /sessions
     * List all active sessions
     */
    static async listSessions(req, res) {
        try {
            const result = await SessionsService.listSessions();

            res.json({
                items: result.sessions,
                total: result.total,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('LIST_SESSIONS_FAILED', `Failed to list sessions: ${error.message}`);
        }
    }

    /**
     * POST /sessions
     * Create new analysis session
     */
    static async createSession(req, res) {
        try {
            const { masterCallId, sampleRate, bufferSize, enableEnhancedAnalysis, ...options } = req.body;

            if (!masterCallId) {
                throw ApiError.badRequest('MISSING_MASTER_CALL', 'Master call ID is required');
            }

            const session = await SessionsService.createSession(masterCallId, {
                sampleRate,
                bufferSize,
                enableEnhancedAnalysis,
                ...options
            });

            res.status(201).json({
                session,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('CREATE_SESSION_FAILED', `Failed to create session: ${error.message}`);
        }
    }

    /**
     * GET /sessions/:id
     * Get specific session details
     */
    static async getSession(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            const session = await SessionsService.getSession(id);

            res.json({
                session,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_SESSION_FAILED', `Failed to get session: ${error.message}`);
        }
    }

    /**
     * POST /sessions/:id/start
     * Start recording/analysis for session
     */
    static async startSession(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            const session = await SessionsService.startSession(id);

            res.json({
                session,
                message: 'Session started successfully',
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('START_SESSION_FAILED', `Failed to start session: ${error.message}`);
        }
    }

    /**
     * POST /sessions/:id/stop
     * Stop and finalize session analysis
     */
    static async stopSession(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            const session = await SessionsService.stopSession(id);

            res.json({
                session,
                message: 'Session completed successfully',
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('STOP_SESSION_FAILED', `Failed to stop session: ${error.message}`);
        }
    }

    /**
     * DELETE /sessions/:id
     * Delete session and cleanup resources
     */
    static async deleteSession(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            await SessionsService.deleteSession(id);

            res.status(204).send();

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('DELETE_SESSION_FAILED', `Failed to delete session: ${error.message}`);
        }
    }

    /**
     * GET /sessions/:id/metrics
     * Get real-time session metrics
     */
    static async getSessionMetrics(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            const metrics = await SessionsService.getSessionMetrics(id);

            res.json(metrics);

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_METRICS_FAILED', `Failed to get session metrics: ${error.message}`);
        }
    }

    /**
     * POST /sessions/:id/audio
     * Process audio data for session (real-time)
     */
    static async processAudio(req, res) {
        try {
            const { id } = req.params;
            const audioData = req.body;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Session ID is required');
            }

            if (!audioData || !audioData.samples) {
                throw ApiError.badRequest('MISSING_AUDIO_DATA', 'Audio samples data is required');
            }

            // [20251029-FIX-001] Extract samples array from request body
            const analysis = await SessionsService.processAudioData(id, audioData.samples);

            res.json({
                analysis,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('PROCESS_AUDIO_FAILED', `Failed to process audio: ${error.message}`);
        }
    }
}

module.exports = SessionsController;