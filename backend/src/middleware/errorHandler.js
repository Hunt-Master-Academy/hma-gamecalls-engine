/**
 * Error handling middleware for Game Calls Engine API
 */

class ApiError extends Error {
    constructor(statusCode, code, message, details = null) {
        super(message);
        this.statusCode = statusCode;
        this.code = code;
        this.details = details;
        this.timestamp = new Date().toISOString();
    }
}

// Static factory methods for common error types
ApiError.badRequest = (code, message, details) => new ApiError(400, code, message, details);
ApiError.unauthorized = (code, message, details) => new ApiError(401, code, message, details);
ApiError.forbidden = (code, message, details) => new ApiError(403, code, message, details);
ApiError.notFound = (code, message, details) => new ApiError(404, code, message, details);
ApiError.conflict = (code, message, details) => new ApiError(409, code, message, details);
ApiError.unprocessableEntity = (code, message, details) => new ApiError(422, code, message, details);
ApiError.internal = (code, message, details) => new ApiError(500, code, message, details);
ApiError.serviceUnavailable = (code, message, details) => new ApiError(503, code, message, details);

const errorHandler = (err, req, res, next) => {
    // Log the error
    console.error(`[${new Date().toISOString()}] ERROR ${req.method} ${req.path}:`, err.message);
    
    if (err.stack) {
        console.error(err.stack);
    }

    // Handle ApiError instances
    if (err instanceof ApiError) {
        return res.status(err.statusCode).json({
            code: err.code,
            message: err.message,
            details: err.details,
            timestamp: err.timestamp,
            path: req.path,
            method: req.method
        });
    }

    // Handle Joi validation errors
    if (err.isJoi) {
        return res.status(400).json({
            code: 'VALIDATION_ERROR',
            message: 'Request validation failed',
            details: err.details.map(detail => ({
                field: detail.path.join('.'),
                message: detail.message
            })),
            timestamp: new Date().toISOString(),
            path: req.path,
            method: req.method
        });
    }

    // Handle multer file upload errors
    if (err.code === 'LIMIT_FILE_SIZE') {
        return res.status(413).json({
            code: 'FILE_TOO_LARGE',
            message: 'Audio file size exceeds maximum allowed size',
            timestamp: new Date().toISOString(),
            path: req.path,
            method: req.method
        });
    }

    // Handle unexpected errors
    return res.status(500).json({
        code: 'INTERNAL_SERVER_ERROR',
        message: 'An unexpected error occurred',
        timestamp: new Date().toISOString(),
        path: req.path,
        method: req.method
    });
};

module.exports = { ApiError, errorHandler };