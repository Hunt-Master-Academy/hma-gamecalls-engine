// [20251028-API-022] GameCalls Engine REST API with Node-API bindings
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const compression = require('compression');
const path = require('path');

// Import routes
const healthRoutes = require('./routes/health');
const gameCallsRoutes = require('./routes/gamecalls');
const sessionsRoutes = require('./routes/sessions');
const analysisRoutes = require('./routes/analysis');
const { errorHandler } = require('./middleware/errorHandler');
const logger = require('./middleware/logger');

// [20251029-AUDIO-010] Import services for initialization
const minioService = require('./services/minioService');
const databaseService = require('./services/databaseService');

const app = express();
const PORT = process.env.PORT || 5005;  // [20251028-API-023] Changed from 5001 to 5005 per architecture spec

// [20251029-AUDIO-011] Initialize services at startup
async function initializeServices() {
    try {
        console.log('🔧 Initializing services...');
        
        // Initialize database
        await databaseService.initialize();
        console.log('✅ Database service initialized');
        
        // Initialize MinIO
        minioService.initialize();
        await minioService.ensureBuckets();
        console.log('✅ MinIO service initialized');
        
    } catch (error) {
        console.error('❌ Service initialization failed:', error.message);
        // Continue anyway - services will show unavailable in API responses
    }
}

// Middleware
app.use(helmet());
app.use(cors({
  origin: process.env.ALLOWED_ORIGINS?.split(',') || ['http://localhost:8000', 'http://localhost:3000', 'http://localhost:3004'],
  credentials: true
}));
app.use(compression());

// [20251030-FIX-002] Raw body parser for audio uploads (binary data)
app.use('/sessions/:id/audio', express.raw({ 
    type: 'application/octet-stream', 
    limit: '50mb' 
}));

app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true }));
app.use(logger);

// [20251028-API-024] Health check with engine binding status
app.use('/health', healthRoutes);

// API Routes
app.use('/calls', gameCallsRoutes);
app.use('/sessions', sessionsRoutes);
app.use('/analysis', analysisRoutes);

// Legacy endpoint (backwards compatibility)
app.get('/gamecalls/health', (req, res) => {
    res.redirect('/health');
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

// [20251029-AUDIO-012] Start server with service initialization
async function startServer() {
    await initializeServices();
    
    app.listen(PORT, () => {
        console.log(`🚀 Game Calls Engine API running on port ${PORT}`);
        console.log(`📍 Health check: http://localhost:${PORT}/health`);
        console.log(`🎵 API base: http://localhost:${PORT}/calls`);
        console.log(`🎤 Sessions: http://localhost:${PORT}/sessions`);
        console.log(`📊 Analysis: http://localhost:${PORT}/analysis`);
        console.log(`🔧 C++ Engine: Node-API bindings loaded`);
    });
}

// Start the server
startServer().catch(err => {
    console.error('❌ Failed to start server:', err);
    process.exit(1);
});