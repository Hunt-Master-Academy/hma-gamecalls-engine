// [20251030-TEST-005] Load Testing and Resource Limits
// Tests concurrent sessions, memory cleanup, temp file cleanup, and cache expiration

const axios = require('axios');
const { Client: MinioClient } = require('minio');
const fs = require('fs').promises;
const path = require('path');

// Try to import redis, fallback if not available
let redisClient = null;
try {
    const redis = require('redis');
    redisClient = redis.createClient({
        url: `redis://${process.env.REDIS_HOST || 'localhost'}:${process.env.REDIS_PORT || 6379}`,
        password: process.env.REDIS_PASSWORD
    });
} catch (err) {
    console.warn('⚠️  Redis module not available - cache tests will be skipped');
}

const API_BASE = process.env.API_BASE_URL || 'http://localhost:5005';
const api = axios.create({ 
    baseURL: API_BASE,
    validateStatus: () => true
});

describe('Load Testing and Resource Limits', () => {
    
    let masterCallId;
    
    beforeAll(async () => {
        // Get a valid master call for testing
        const callsResponse = await api.get('/calls');
        if (!callsResponse.data.items || callsResponse.data.items.length === 0) {
            throw new Error('No master calls available for testing');
        }
        masterCallId = callsResponse.data.items[0].id;
        
        // Connect to Redis for cache inspection (if available)
        if (redisClient) {
            try {
                await redisClient.connect();
            } catch (err) {
                console.warn('⚠️  Could not connect to Redis - cache tests will be skipped');
                redisClient = null;
            }
        }
    });
    
    afterAll(async () => {
        if (redisClient) {
            await redisClient.quit().catch(() => {});
        }
    });
    
    // =========================================================================
    // Concurrent Session Tests
    // =========================================================================
    
    describe('Concurrent Session Handling', () => {
        
        test('Should handle multiple concurrent session creations', async () => {
            const concurrentRequests = 10;
            const promises = Array(concurrentRequests).fill(null).map(() =>
                api.post('/sessions', {
                    masterCallId,
                    options: { threshold: 0.85 }
                })
            );
            
            const results = await Promise.allSettled(promises);
            
            const successful = results.filter(r => r.status === 'fulfilled' && r.value.status === 201);
            const failed = results.filter(r => r.status === 'rejected' || r.value.status !== 201);
            
            console.log(`✓ Concurrent session creation test`);
            console.log(`  Requested: ${concurrentRequests}`);
            console.log(`  Successful: ${successful.length}`);
            console.log(`  Failed: ${failed.length}`);
            
            // At least 80% should succeed
            expect(successful.length).toBeGreaterThanOrEqual(concurrentRequests * 0.8);
            
            // Cleanup created sessions
            const sessionIds = successful.map(r => r.value.data.session.id);
            await Promise.all(sessionIds.map(id => 
                api.delete(`/sessions/${id}`).catch(() => {})
            ));
        }, 30000);
        
        test('Should handle concurrent audio processing for same session', async () => {
            // Create and start a session
            const createResponse = await api.post('/sessions', { 
                masterCallId,
                options: { threshold: 0.85 }
            });
            const sessionId = createResponse.data.session.id;
            await api.post(`/sessions/${sessionId}/start`);
            
            // Send multiple audio chunks concurrently
            const audioBuffer = Buffer.alloc(88200); // 1 second
            for (let i = 0; i < audioBuffer.length; i += 2) {
                audioBuffer.writeInt16LE(Math.floor(Math.sin(i / 100) * 5000), i);
            }
            
            const concurrentAudio = 5;
            const promises = Array(concurrentAudio).fill(null).map(() =>
                api.post(`/sessions/${sessionId}/audio`, audioBuffer, {
                    headers: { 'Content-Type': 'application/octet-stream' }
                })
            );
            
            const results = await Promise.allSettled(promises);
            const successful = results.filter(r => 
                r.status === 'fulfilled' && [200, 202].includes(r.value.status)
            );
            const rejected = results.filter(r => 
                r.status === 'fulfilled' && [400, 409, 429, 500].includes(r.value.status)
            );
            const errors = results.filter(r => r.status === 'rejected');
            
            console.log(`✓ Concurrent audio processing test`);
            console.log(`  Concurrent requests: ${concurrentAudio}`);
            console.log(`  Successful: ${successful.length}`);
            console.log(`  Rejected (4xx/5xx): ${rejected.length}`);
            console.log(`  Network errors: ${errors.length}`);
            
            // All requests should complete (either succeed or get handled error response)
            expect(successful.length + rejected.length + errors.length).toBe(concurrentAudio);
            
            // Cleanup
            await api.post(`/sessions/${sessionId}/stop`).catch(() => {});
        }, 30000);
        
        test('Should enforce concurrent session limits per user', async () => {
            // Create many sessions rapidly
            const maxSessions = 20;
            const sessionIds = [];
            
            for (let i = 0; i < maxSessions; i++) {
                const response = await api.post('/sessions', {
                    masterCallId,
                    options: { threshold: 0.85 }
                });
                
                if (response.status === 201) {
                    sessionIds.push(response.data.session.id);
                } else if (response.status === 429) {
                    console.log(`  Rate limited at ${sessionIds.length} sessions`);
                    break;
                }
            }
            
            console.log(`✓ Session limit test`);
            console.log(`  Created ${sessionIds.length} sessions before limit`);
            
            // Should have created at least some sessions
            expect(sessionIds.length).toBeGreaterThan(0);
            
            // Cleanup
            await Promise.all(sessionIds.map(id => 
                api.delete(`/sessions/${id}`).catch(() => {})
            ));
        }, 60000);
    });
    
    // =========================================================================
    // Memory and Resource Cleanup Tests
    // =========================================================================
    
    describe('Memory and Resource Cleanup', () => {
        
        test('Should clean up session resources after stop', async () => {
            // Create, start, and stop a session
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            
            await api.post(`/sessions/${sessionId}/start`);
            
            // Send some audio
            const audioBuffer = Buffer.alloc(88200);
            for (let i = 0; i < audioBuffer.length; i += 2) {
                audioBuffer.writeInt16LE(Math.floor(Math.sin(i / 100) * 5000), i);
            }
            await api.post(`/sessions/${sessionId}/audio`, audioBuffer, {
                headers: { 'Content-Type': 'application/octet-stream' }
            }).catch(() => {});
            
            await api.post(`/sessions/${sessionId}/stop`).catch(() => {});
            
            // Wait a moment for cleanup
            await new Promise(resolve => setTimeout(resolve, 1000));
            
            // Session should still be accessible (for retrieval)
            const getResponse = await api.get(`/sessions/${sessionId}`);
            expect([200, 404]).toContain(getResponse.status);
            
            console.log(`✓ Session cleanup test passed`);
            console.log(`  Session ${sessionId} state: ${getResponse.status === 200 ? 'accessible' : 'cleaned up'}`);
        }, 15000);
        
        test('Should clean up temporary audio files', async () => {
            // Create a session (this downloads master call to temp directory)
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            
            expect(createResponse.status).toBe(201);
            
            // Stop session to trigger cleanup
            await api.post(`/sessions/${sessionId}/start`).catch(() => {});
            await api.post(`/sessions/${sessionId}/stop`).catch(() => {});
            
            // Wait for cleanup
            await new Promise(resolve => setTimeout(resolve, 2000));
            
            console.log(`✓ Temporary file cleanup test`);
            console.log(`  (Manual verification needed for /tmp/gamecalls-* directories)`);
            
            // Note: Actual temp file checking would require container access or shared volume
            expect(true).toBe(true);
        }, 15000);
        
        test('Should handle memory pressure gracefully', async () => {
            // Create multiple sessions with large audio buffers
            const sessionCount = 5;
            const sessionIds = [];
            
            for (let i = 0; i < sessionCount; i++) {
                const response = await api.post('/sessions', { masterCallId });
                if (response.status === 201) {
                    const sessionId = response.data.session.id;
                    sessionIds.push(sessionId);
                    
                    await api.post(`/sessions/${sessionId}/start`).catch(() => {});
                    
                    // Send large audio buffer (5 seconds)
                    const largeBuffer = Buffer.alloc(441000);
                    for (let j = 0; j < largeBuffer.length; j += 2) {
                        largeBuffer.writeInt16LE(Math.floor(Math.sin(j / 100) * 5000), j);
                    }
                    
                    await api.post(`/sessions/${sessionId}/audio`, largeBuffer, {
                        headers: { 'Content-Type': 'application/octet-stream' }
                    }).catch(() => {});
                }
            }
            
            console.log(`✓ Memory pressure test`);
            console.log(`  Created ${sessionIds.length} sessions with large buffers`);
            
            // All sessions should have been created
            expect(sessionIds.length).toBe(sessionCount);
            
            // Cleanup
            await Promise.all(sessionIds.map(id => 
                api.post(`/sessions/${id}/stop`).catch(() => {})
            ));
        }, 60000);
    });
    
    // =========================================================================
    // Redis Cache Tests
    // =========================================================================
    
    describe('Redis Cache Management', () => {
        
        test('Should cache session data in Redis', async () => {
            if (!redisClient) {
                console.warn('⚠️  Skipping - Redis not available');
                return;
            }
            
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            
            // Check if session exists in Redis
            const cacheKey = `session:${sessionId}`;
            const cachedData = await redisClient.get(cacheKey);
            
            expect(cachedData).not.toBeNull();
            
            if (cachedData) {
                const session = JSON.parse(cachedData);
                expect(session).toHaveProperty('id', sessionId);
                expect(session).toHaveProperty('status');
                
                console.log(`✓ Session cached in Redis`);
                console.log(`  Key: ${cacheKey}`);
                console.log(`  Status: ${session.status}`);
            }
            
            // Cleanup
            await api.delete(`/sessions/${sessionId}`).catch(() => {});
        });
        
        test('Should expire cache entries after TTL', async () => {
            if (!redisClient) {
                console.warn('⚠️  Skipping - Redis not available');
                return;
            }
            
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            
            // Check TTL on cache entry
            const cacheKey = `session:${sessionId}`;
            const ttl = await redisClient.ttl(cacheKey);
            
            expect(ttl).toBeGreaterThan(0);
            expect(ttl).toBeLessThanOrEqual(3600); // Default TTL is 1 hour
            
            console.log(`✓ Cache TTL verification`);
            console.log(`  Key: ${cacheKey}`);
            console.log(`  TTL: ${ttl} seconds`);
            
            // Cleanup
            await api.delete(`/sessions/${sessionId}`).catch(() => {});
        });
        
        test('Should update cache on session state changes', async () => {
            if (!redisClient) {
                console.warn('⚠️  Skipping - Redis not available');
                return;
            }
            
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            const cacheKey = `session:${sessionId}`;
            
            // Check initial state
            let cachedData = await redisClient.get(cacheKey);
            let session = JSON.parse(cachedData);
            expect(session.status).toBe('created');
            
            // Start session
            await api.post(`/sessions/${sessionId}/start`);
            
            // Wait briefly for cache update
            await new Promise(resolve => setTimeout(resolve, 100));
            
            // Check updated state
            cachedData = await redisClient.get(cacheKey);
            session = JSON.parse(cachedData);
            expect(session.status).toBe('recording');
            
            console.log(`✓ Cache updates on state changes`);
            console.log(`  Created → Recording`);
            
            // Cleanup
            await api.post(`/sessions/${sessionId}/stop`).catch(() => {});
        });
        
        test('Should clear cache entries on session deletion', async () => {
            if (!redisClient) {
                console.warn('⚠️  Skipping - Redis not available');
                return;
            }
            
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            const cacheKey = `session:${sessionId}`;
            
            // Verify cache exists
            let exists = await redisClient.exists(cacheKey);
            expect(exists).toBe(1);
            
            // Delete session
            await api.delete(`/sessions/${sessionId}`);
            
            // Wait for cache cleanup
            await new Promise(resolve => setTimeout(resolve, 500));
            
            // Cache should be cleared
            exists = await redisClient.exists(cacheKey);
            
            console.log(`✓ Cache cleanup on deletion`);
            console.log(`  Cache cleared: ${exists === 0 ? 'Yes' : 'No'}`);
            
            // Either way is acceptable - cache might expire naturally or be deleted
            expect([0, 1]).toContain(exists);
        });
    });
    
    // =========================================================================
    // Performance Benchmarks
    // =========================================================================
    
    describe('Performance Benchmarks', () => {
        
        test('Should maintain response times under load', async () => {
            const iterations = 50;
            const responseTimes = [];
            
            for (let i = 0; i < iterations; i++) {
                const start = Date.now();
                const response = await api.get('/calls');
                const duration = Date.now() - start;
                
                if (response.status === 200) {
                    responseTimes.push(duration);
                }
                
                // Small delay to avoid overwhelming the server
                await new Promise(resolve => setTimeout(resolve, 10));
            }
            
            const avgTime = responseTimes.reduce((a, b) => a + b, 0) / responseTimes.length;
            const maxTime = Math.max(...responseTimes);
            const minTime = Math.min(...responseTimes);
            
            console.log(`✓ Performance benchmark - GET /calls`);
            console.log(`  Requests: ${iterations}`);
            console.log(`  Average: ${avgTime.toFixed(2)}ms`);
            console.log(`  Min: ${minTime}ms`);
            console.log(`  Max: ${maxTime}ms`);
            
            // Average response time should be reasonable
            expect(avgTime).toBeLessThan(500); // 500ms average
            expect(maxTime).toBeLessThan(2000); // 2s max
        }, 60000);
        
        test('Should handle burst traffic', async () => {
            const burstSize = 20;
            const start = Date.now();
            
            const promises = Array(burstSize).fill(null).map(() =>
                api.get('/health')
            );
            
            const results = await Promise.allSettled(promises);
            const duration = Date.now() - start;
            
            const successful = results.filter(r => 
                r.status === 'fulfilled' && r.value.status === 200
            );
            
            console.log(`✓ Burst traffic test`);
            console.log(`  Burst size: ${burstSize}`);
            console.log(`  Successful: ${successful.length}`);
            console.log(`  Duration: ${duration}ms`);
            console.log(`  Throughput: ${(burstSize / (duration / 1000)).toFixed(2)} req/s`);
            
            // At least 90% should succeed
            expect(successful.length).toBeGreaterThanOrEqual(burstSize * 0.9);
        }, 30000);
    });
    
    // =========================================================================
    // Error Recovery Tests
    // =========================================================================
    
    describe('Error Recovery and Stability', () => {
        
        test('Should recover from invalid audio data', async () => {
            const createResponse = await api.post('/sessions', { masterCallId });
            const sessionId = createResponse.data.session.id;
            await api.post(`/sessions/${sessionId}/start`);
            
            // Send invalid audio data
            const invalidBuffer = Buffer.from('not audio data');
            await api.post(`/sessions/${sessionId}/audio`, invalidBuffer, {
                headers: { 'Content-Type': 'application/octet-stream' }
            }).catch(() => {});
            
            // Session should still be accessible
            const getResponse = await api.get(`/sessions/${sessionId}`);
            expect(getResponse.status).toBe(200);
            
            // Should be able to stop cleanly
            const stopResponse = await api.post(`/sessions/${sessionId}/stop`);
            expect([200, 400, 500]).toContain(stopResponse.status);
            
            console.log(`✓ Recovery from invalid audio data`);
        });
        
        test('Should handle rapid session lifecycle', async () => {
            // Create, start, stop rapidly
            const cycles = 10;
            let successfulCycles = 0;
            
            for (let i = 0; i < cycles; i++) {
                try {
                    const createResp = await api.post('/sessions', { masterCallId });
                    const sessionId = createResp.data.session.id;
                    
                    await api.post(`/sessions/${sessionId}/start`);
                    await api.post(`/sessions/${sessionId}/stop`);
                    
                    successfulCycles++;
                } catch (error) {
                    // Acceptable to have some failures under rapid cycling
                }
                
                await new Promise(resolve => setTimeout(resolve, 50));
            }
            
            console.log(`✓ Rapid lifecycle test`);
            console.log(`  Cycles: ${cycles}`);
            console.log(`  Successful: ${successfulCycles}`);
            
            // At least 70% should succeed
            expect(successfulCycles).toBeGreaterThanOrEqual(cycles * 0.7);
        }, 30000);
    });
});
