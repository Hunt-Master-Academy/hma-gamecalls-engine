/**
 * Sessions API Routes
 * REST endpoints for audio analysis session management
 */

const express = require('express');
const router = express.Router();
const { ApiError } = require('../middleware/errorHandler');
const asyncHandler = require('../middleware/asyncHandler');

// Import Controllers
const SessionsController = require('../controllers/sessionsController');

// Validation middleware for session creation
const validateSessionRequest = (req, res, next) => {
    const { masterCallId } = req.body;

    if (!masterCallId || typeof masterCallId !== 'string') {
        throw ApiError.badRequest('INVALID_MASTER_CALL_ID', 'Master call ID is required and must be a string');
    }

    if (req.body.sampleRate && (req.body.sampleRate < 8000 || req.body.sampleRate > 96000)) {
        throw ApiError.badRequest('INVALID_SAMPLE_RATE', 'Sample rate must be between 8000 and 96000 Hz');
    }

    if (req.body.bufferSize && (req.body.bufferSize < 256 || req.body.bufferSize > 8192)) {
        throw ApiError.badRequest('INVALID_BUFFER_SIZE', 'Buffer size must be between 256 and 8192 samples');
    }

    next();
};

// Routes

/**
 * GET /sessions
 * List all active sessions
 */
router.get('/', asyncHandler(SessionsController.listSessions));

/**
 * POST /sessions
 * Create new analysis session
 * Body: { masterCallId, sampleRate?, bufferSize?, enableEnhancedAnalysis?, ...options }
 */
router.post('/', validateSessionRequest, asyncHandler(SessionsController.createSession));

/**
 * GET /sessions/:id
 * Get specific session details
 */
router.get('/:id', asyncHandler(SessionsController.getSession));

/**
 * POST /sessions/:id/start
 * Start recording/analysis for session
 */
router.post('/:id/start', asyncHandler(SessionsController.startSession));

/**
 * POST /sessions/:id/stop
 * Stop and finalize session analysis
 */
router.post('/:id/stop', asyncHandler(SessionsController.stopSession));

/**
 * DELETE /sessions/:id
 * Delete session and cleanup resources
 */
router.delete('/:id', asyncHandler(SessionsController.deleteSession));

/**
 * GET /sessions/:id/metrics
 * Get real-time session metrics
 */
router.get('/:id/metrics', asyncHandler(SessionsController.getSessionMetrics));

/**
 * POST /sessions/:id/audio
 * Process audio data for session (real-time)
 * Body: { samples: Float32Array, sampleRate: number, timestamp: string }
 */
router.post('/:id/audio', asyncHandler(SessionsController.processAudio));

module.exports = router;