const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const compression = require('compression');
const path = require('path');

// Import routes
const gameCallsRoutes = require('./routes/gamecalls');
const sessionsRoutes = require('./routes/sessions');
const analysisRoutes = require('./routes/analysis');
const { errorHandler } = require('./middleware/errorHandler');
const logger = require('./middleware/logger');

const app = express();
const PORT = process.env.PORT || 5001;

// Middleware
app.use(helmet());
app.use(cors({
  origin: process.env.ALLOWED_ORIGINS?.split(',') || ['http://localhost:8000', 'http://localhost:3000'],
  credentials: true
}));
app.use(compression());
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true }));
app.use(logger);

// Health check
app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: new Date().toISOString(),
    version: '1.0.0',
    service: 'hma-gamecalls-engine'
  });
});

// API Routes
app.use('/calls', gameCallsRoutes);
app.use('/sessions', sessionsRoutes);
app.use('/analysis', analysisRoutes);

// Game Calls API health check with endpoints
app.get('/gamecalls/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'hma-gamecalls-engine',
        version: '1.0.0',
        endpoints: {
            calls: '/calls',
            sessions: '/sessions',
            analysis: '/analysis'
        },
        timestamp: new Date().toISOString()
    });
});

// 404 handler
app.use('*', (req, res) => {
  res.status(404).json({
    code: 'ENDPOINT_NOT_FOUND',
    message: `Endpoint ${req.method} ${req.originalUrl} not found`,
    timestamp: new Date().toISOString()
  });
});

// Error handling middleware
app.use(errorHandler);

// Start server
app.listen(PORT, () => {
  console.log(`🚀 Game Calls Engine API running on port ${PORT}`);
  console.log(`📍 Health check: http://localhost:${PORT}/health`);
  console.log(`🎵 API base: http://localhost:${PORT}/calls`);
  console.log(`🎤 Sessions: http://localhost:${PORT}/sessions`);
  console.log(`📊 Analysis: http://localhost:${PORT}/analysis`);
});