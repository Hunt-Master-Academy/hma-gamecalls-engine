/**
 * Logging middleware for Game Calls Engine API
 */

const logger = (req, res, next) => {
    const startTime = Date.now();
    const timestamp = new Date().toISOString();
    
    console.log(`[${timestamp}] ${req.method} ${req.path} - Started`);
    
    // Log request body for POST/PUT requests (but not file uploads)
    if ((req.method === 'POST' || req.method === 'PUT') && 
        req.body && 
        !req.file && 
        !req.files &&
        Object.keys(req.body).length > 0) {
        console.log(`[${timestamp}] Request body:`, JSON.stringify(req.body, null, 2));
    }

    // Override res.end to log completion
    const originalEnd = res.end;
    res.end = function(...args) {
        const duration = Date.now() - startTime;
        const logLevel = res.statusCode >= 400 ? 'ERROR' : 'INFO';
        console.log(`[${timestamp}] ${req.method} ${req.path} - ${res.statusCode} (${duration}ms) [${logLevel}]`);
        originalEnd.apply(this, args);
    };

    next();
};

module.exports = logger;