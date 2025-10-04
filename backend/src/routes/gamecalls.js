/**
 * Game Calls API Routes
 * REST endpoints for master call operations and management
 */

const express = require('express');
const multer = require('multer');
const router = express.Router();
const { ApiError } = require('../middleware/errorHandler');

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
        const allowedMimeTypes = ['audio/wav', 'audio/mpeg', 'audio/mp4', 'audio/ogg'];
        if (allowedMimeTypes.includes(file.mimetype)) {
            cb(null, true);
        } else {
            cb(new ApiError.badRequest('INVALID_FILE_TYPE', 'Only audio files are allowed'), false);
        }
    }
});

// Validation middleware for call metadata
const validateCallMetadata = (req, res, next) => {
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
};

// Routes

/**
 * GET /calls/categories
 * Get available species, call types, and filtering options
 */
router.get('/categories', GameCallsController.getCategories);

/**
 * GET /calls
 * List all master calls with filtering and pagination
 * Query params: page, pageSize, species, callType, difficulty, tags
 */
router.get('/', GameCallsController.listCalls);

/**
 * POST /calls/upload
 * Upload new master call (admin only)
 * Body: multipart/form-data with audio file and metadata
 */
router.post('/upload', upload.single('audio'), validateCallMetadata, GameCallsController.uploadCall);

/**
 * GET /calls/:id
 * Get specific master call by ID with detailed analysis data
 */
router.get('/:id', GameCallsController.getCall);

/**
 * GET /calls/:id/audio
 * Stream audio file for a master call
 */
router.get('/:id/audio', GameCallsController.streamAudio);

/**
 * GET /calls/:id/waveform
 * Get waveform data for visualization
 * Query params: decimation (default: 1000)
 */
router.get('/:id/waveform', GameCallsController.getWaveform);

module.exports = router;