// [20251030-FIX-008] Async handler wrapper to catch errors in async route handlers
// Wraps async route handlers to automatically catch errors and pass to Express error middleware

/**
 * Wraps async route handlers to catch promise rejections
 * @param {Function} fn - Async route handler function
 * @returns {Function} - Wrapped handler that passes errors to next()
 */
const asyncHandler = (fn) => {
    return (req, res, next) => {
        Promise.resolve(fn(req, res, next)).catch(next);
    };
};

module.exports = asyncHandler;
