/**
 * Game Calls API Routes
 * REST endpoints for master call operations and management
 */

const express = require('express');
const multer = require('multer');
const router = express.Router();
const { ApiError } = require('../middleware/errorHandler');
const asyncHandler = require('../middleware/asyncHandler');

// Import Controllers
const GameCallsController = require('../controllers/gameCallsController');

// Configure multer for audio file uploads
const upload = multer({
    storage: multer.memoryStorage(),
    limits: {
        fileSize: 10 * 1024 * 1024, // 10MB max file size
        files: 1
    },
    fileFilter: (req, file, cb) => {
        console.log(`[Upload] Received file: ${file.originalname}, mimetype: ${file.mimetype}`);
        // [20251029-UPLOAD-001] Accept common audio MIME types + octet-stream (for CLI uploads)
        // [20251030-UPLOAD-002] Added audio/wave (some systems use this instead of audio/wav)
        const allowedMimeTypes = [
            'audio/wav', 'audio/x-wav', 'audio/wave', 'audio/mpeg', 'audio/mp3', 
            'audio/mp4', 'audio/m4a', 'audio/ogg', 'audio/flac',
            'application/octet-stream'  // CLI tools often use this
        ];
        
        // Additional check: validate WAV by file extension if mimetype is generic
        const isWavFile = file.originalname.toLowerCase().endsWith('.wav');
        const isAudioMime = allowedMimeTypes.includes(file.mimetype);
        
        if (isAudioMime || (file.mimetype === 'application/octet-stream' && isWavFile)) {
            cb(null, true);
        } else {
            cb(ApiError.badRequest('INVALID_FILE_TYPE', `Only audio files are allowed. Received: ${file.mimetype}`), false);
        }
    }
});

// Validation middleware for call metadata
const validateCallMetadata = (req, res, next) => {
    try {
        const { name, species, callType } = req.body;

        if (!name || typeof name !== 'string') {
            throw ApiError.badRequest('INVALID_NAME', 'Call name is required and must be a string');
        }

        if (name.length > 100) {
            throw ApiError.badRequest('NAME_TOO_LONG', 'Call name cannot exceed 100 characters');
        }

        if (!species || typeof species !== 'string') {
            throw ApiError.badRequest('INVALID_SPECIES', 'Species is required and must be a string');
        }

        if (!callType || typeof callType !== 'string') {
            throw ApiError.badRequest('INVALID_CALL_TYPE', 'Call type is required and must be a string');
        }

        const validDifficulties = ['beginner', 'intermediate', 'advanced'];
        if (req.body.difficulty && !validDifficulties.includes(req.body.difficulty)) {
            throw ApiError.badRequest('INVALID_DIFFICULTY', `Difficulty must be one of: ${validDifficulties.join(', ')}`);
        }

        next();
    } catch (error) {
        next(error);
    }
};

// Routes

/**
 * GET /calls/categories
 * Get available species, call types, and filtering options
 */
router.get('/categories', asyncHandler(GameCallsController.getCategories));

/**
 * GET /calls
 * List all master calls with filtering and pagination
 * Query params: page, pageSize, species, callType, difficulty, tags
 */
router.get('/', asyncHandler(GameCallsController.listCalls));

/**
 * POST /calls/upload
 * Upload new master call (admin only)
 * Body: multipart/form-data with audio file and metadata
 */
router.post('/upload', upload.single('audio'), validateCallMetadata, asyncHandler(GameCallsController.uploadCall));

/**
 * GET /calls/:id
 * Get specific master call by ID with detailed analysis data
 */
router.get('/:id', asyncHandler(GameCallsController.getCall));

/**
 * GET /calls/:id/audio
 * Stream audio file for a master call
 */
router.get('/:id/audio', asyncHandler(GameCallsController.streamAudio));

/**
 * GET /calls/:id/waveform
 * Get waveform data for visualization
 * Query params: decimation (default: 1000)
 */
router.get('/:id/waveform', asyncHandler(GameCallsController.getWaveform));

module.exports = router;