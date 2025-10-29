// [20251028-API-020] Health check endpoint with engine status
// Extended health check showing C++ engine binding status

const express = require('express');

// [20251029-MOCK-002] Load engine with mock fallback
let gameCallsEngine;
try {
    gameCallsEngine = require('../../../bindings/node-api/lib/index');
} catch (error) {
    gameCallsEngine = require('../services/mockGameCallsEngine');
}

const router = express.Router();

/**
 * GET /health
 * Extended health check with engine binding status
 */
router.get('/', async (req, res) => {
    try {
        let engineInfo = null;
        let engineStatus = 'unknown';
        
        try {
            // [20251028-API-021] Try to get engine info to verify bindings work
            engineInfo = await gameCallsEngine.getEngineInfo();
            engineStatus = 'available';
        } catch (error) {
            engineStatus = 'unavailable';
            engineInfo = { error: error.message };
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
