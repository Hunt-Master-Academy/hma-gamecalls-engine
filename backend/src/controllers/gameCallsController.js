/**
 * Game Calls Controller
 * Handles REST API requests for master call operations
 * Interfaces with GameCallsService for business logic
 */

const { ApiError } = require('../middleware/errorHandler');
const GameCallsService = require('../services/gameCallsService');

class GameCallsController {

    /**
     * GET /calls
     * List all master calls with filtering and pagination
     */
    static async listCalls(req, res) {
        try {
            const {
                page = 1,
                pageSize = 50,
                species,
                callType,
                difficulty,
                tags
            } = req.query;

            const pageNum = parseInt(page);
            const pageSizeNum = Math.min(parseInt(pageSize), 100); // Max 100 per page

            if (pageNum < 1 || pageSizeNum < 1) {
                throw ApiError.badRequest('INVALID_PAGINATION', 'Page and pageSize must be positive integers');
            }

            const result = await GameCallsService.listCalls({
                page: pageNum,
                pageSize: pageSizeNum,
                species,
                callType,
                difficulty,
                tags: tags ? tags.split(',') : undefined
            });

            res.json({
                items: result.calls,
                total: result.total,
                page: pageNum,
                pageSize: pageSizeNum,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('LIST_CALLS_FAILED', `Failed to list calls: ${error.message}`);
        }
    }

    /**
     * GET /calls/:id
     * Get specific master call by ID with detailed analysis data
     */
    static async getCall(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Call ID is required');
            }

            const call = await GameCallsService.getCall(id);

            res.json({
                call,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_CALL_FAILED', `Failed to get call: ${error.message}`);
        }
    }

    /**
     * GET /calls/categories
     * Get available species, call types, and filtering options
     */
    static async getCategories(req, res) {
        try {
            const categories = await GameCallsService.getCallCategories();

            res.json({
                categories,
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_CATEGORIES_FAILED', `Failed to get categories: ${error.message}`);
        }
    }

    /**
     * POST /calls/upload
     * Upload new master call (admin only)
     */
    static async uploadCall(req, res) {
        try {
            if (!req.file) {
                throw ApiError.badRequest('MISSING_AUDIO_FILE', 'Audio file is required');
            }

            const metadata = req.body;

            // Validate required metadata
            if (!metadata.name || !metadata.species || !metadata.callType) {
                throw ApiError.badRequest('MISSING_METADATA', 'Name, species, and callType are required');
            }

            // [20251029-UPLOAD-002] File already validated by multer fileFilter

            const newCall = await GameCallsService.uploadCall(req.file, metadata);

            res.status(201).json({
                call: newCall,
                message: 'Call uploaded successfully and is being processed',
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('UPLOAD_CALL_FAILED', `Failed to upload call: ${error.message}`);
        }
    }

    /**
     * GET /calls/:id/audio
     * Stream audio file for a master call
     * [20251029-AUDIO-001] Implemented audio streaming with MinIO backend
     */
    static async streamAudio(req, res) {
        try {
            const { id } = req.params;

            if (!id) {
                throw ApiError.badRequest('MISSING_ID', 'Call ID is required');
            }

            // Get audio stream and metadata from service
            const audioData = await GameCallsService.streamAudio(id);

            // Set appropriate headers for audio streaming
            res.setHeader('Content-Type', audioData.contentType || 'audio/wav');
            res.setHeader('Content-Length', audioData.contentLength);
            res.setHeader('Accept-Ranges', 'bytes');
            res.setHeader('Cache-Control', 'public, max-age=31536000'); // Cache for 1 year
            res.setHeader('Content-Disposition', `inline; filename="${audioData.filename}"`);

            // Handle Range requests for seeking (HTTP 206 Partial Content)
            const range = req.headers.range;
            if (range) {
                const parts = range.replace(/bytes=/, '').split('-');
                const start = parseInt(parts[0], 10);
                const end = parts[1] ? parseInt(parts[1], 10) : audioData.contentLength - 1;
                const chunkSize = (end - start) + 1;

                res.status(206);
                res.setHeader('Content-Range', `bytes ${start}-${end}/${audioData.contentLength}`);
                res.setHeader('Content-Length', chunkSize);

                // MinIO will handle the byte range automatically
                audioData.stream.pipe(res);
            } else {
                // Stream full file
                audioData.stream.pipe(res);
            }

            // Handle stream errors
            audioData.stream.on('error', (error) => {
                console.error('[streamAudio] Stream error:', error);
                if (!res.headersSent) {
                    res.status(500).json({
                        code: 'STREAM_ERROR',
                        message: 'Error streaming audio file'
                    });
                }
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('STREAM_AUDIO_FAILED', `Failed to stream audio: ${error.message}`);
        }
    }

    /**
     * GET /calls/:id/waveform
     * Get waveform data for visualization
     */
    static async getWaveform(req, res) {
        try {
            const { id } = req.params;
            const { decimation = 1000 } = req.query;

            const call = await GameCallsService.getCall(id);

            res.json({
                waveform: call.waveform,
                decimation: parseInt(decimation),
                timestamp: new Date().toISOString()
            });

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_WAVEFORM_FAILED', `Failed to get waveform: ${error.message}`);
        }
    }
}

module.exports = GameCallsController;