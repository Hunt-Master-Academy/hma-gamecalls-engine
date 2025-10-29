// [20251029-MOCK-001] Mock GameCalls Engine bindings for development
/**
 * Mock implementation of C++ engine bindings
 * Use this when native bindings are not available
 * Replace with real bindings from ../../../bindings/node-api/lib/index when available
 */

class MockGameCallsEngine {
    constructor() {
        this.initialized = false;
        this.sessions = new Map();
        console.log('⚠️  Using MOCK GameCalls Engine bindings');
    }

    async initialize() {
        this.initialized = true;
        console.log('✅ Mock engine initialized');
        return { success: true };
    }

    async getEngineInfo() {
        return {
            version: '1.0.0-mock',
            mode: 'mock',
            features: ['session-management', 'audio-processing', 'mfcc', 'dtw'],
            timestamp: new Date().toISOString()
        };
    }

    async createSession(masterCallPath, options = {}) {
        const sessionId = Date.now();
        this.sessions.set(sessionId, {
            id: sessionId,
            masterCallPath,
            options,
            created: Date.now()
        });
        console.log(`📝 Mock session created: ${sessionId}`);
        return sessionId;
    }

    async processAudio(sessionId, audioBuffer) {
        if (!this.sessions.has(sessionId)) {
            throw new Error(`Session ${sessionId} not found`);
        }

        // Mock analysis results
        return {
            similarityScore: 0.75 + Math.random() * 0.2,
            readiness: 'ready',
            levels: {
                rms: 0.3 + Math.random() * 0.1,
                peak: 0.8 + Math.random() * 0.15
            },
            pitch: {
                pitch: 440 + Math.random() * 100,
                confidence: 0.8 + Math.random() * 0.15
            },
            harmonic: {
                harmonicity: 0.7 + Math.random() * 0.2,
                spectralCentroid: 2000 + Math.random() * 500
            },
            cadence: {
                tempo: 120 + Math.random() * 30,
                rhythmStrength: 0.6 + Math.random() * 0.3
            }
        };
    }

    async finalizeSession(sessionId) {
        if (!this.sessions.has(sessionId)) {
            throw new Error(`Session ${sessionId} not found`);
        }

        return {
            overallScore: 0.8 + Math.random() * 0.15,
            similarityScore: 0.75 + Math.random() * 0.2,
            confidence: 0.85 + Math.random() * 0.1,
            segment: {
                startMs: 500,
                endMs: 3500,
                durationMs: 3000
            },
            enhanced: {
                pitch: {
                    pitch: 440 + Math.random() * 100,
                    confidence: 0.8 + Math.random() * 0.15
                },
                harmonic: {
                    harmonicity: 0.7 + Math.random() * 0.2,
                    spectralCentroid: 2000 + Math.random() * 500
                },
                cadence: {
                    tempo: 120 + Math.random() * 30,
                    rhythmStrength: 0.6 + Math.random() * 0.3
                },
                loudness: {
                    rms: 0.3 + Math.random() * 0.1,
                    peak: 0.8 + Math.random() * 0.15
                }
            },
            processingTimeMs: 50 + Math.random() * 100
        };
    }

    async destroySession(sessionId) {
        this.sessions.delete(sessionId);
        console.log(`🗑️  Mock session destroyed: ${sessionId}`);
        return { success: true };
    }
}

// Export mock instance
module.exports = new MockGameCallsEngine();
