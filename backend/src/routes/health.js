// [20251028-API-020] Health check endpoint with engine status
// Extended health check showing C++ engine binding status

const express = require('express');

// [20251030-ENGINE-006] Load real C++ engine (mock removed)
const path = require('path');
let gameCallsEngine;
let engineLoadError;

try {
    // [20251102-DOCKER-002] Support both Docker and host environments for bindings path
    let bindingsPath;
    if (process.env.NODE_ENV === 'production' || process.env.DOCKER_CONTAINER === 'true') {
        // Running in Docker container
        bindingsPath = path.join('/app', 'bindings/node-api/build/Release/gamecalls_engine.node');
    } else {
        // Running on host (for development)
        bindingsPath = path.join(__dirname, '../../../bindings/node-api/build/Release/gamecalls_engine.node');
    }
    
    gameCallsEngine = require(bindingsPath);
} catch (error) {
    engineLoadError = error.message;
    console.error('❌ Failed to load C++ engine for health check:', error.message);
}

const router = express.Router();

/**
 * GET /health
 * Extended health check with engine binding status
 */
router.get('/', async (req, res) => {
    try {
        let engineInfo = null;
        let engineStatus = engineLoadError ? 'unavailable' : 'unknown';
        
        if (gameCallsEngine) {
            try {
                // [20251028-API-021] Try to get engine info to verify bindings work
                engineInfo = await gameCallsEngine.getEngineInfo();
                engineStatus = 'available';
            } catch (error) {
                engineStatus = 'error';
                engineInfo = { error: error.message };
            }
        } else {
            engineInfo = { error: engineLoadError || 'Engine not loaded' };
        }
        
        res.json({
            status: 'ok',
            timestamp: new Date().toISOString(),
            version: '1.0.0',
            service: 'hma-gamecalls-engine',
            engine: {
                status: engineStatus,
                info: engineInfo
            },
            endpoints: {
                calls: '/calls',
                sessions: '/sessions',
                analysis: '/analysis'
            }
        });
    } catch (error) {
        res.status(500).json({
            status: 'error',
            message: error.message,
            timestamp: new Date().toISOString()
        });
    }
});

module.exports = router;
