// [20251030-TEST-001] Error handling and edge case tests
// Validates API error responses and failure scenarios

const axios = require('axios');

const BASE_URL = 'http://localhost:5005';
const api = axios.create({ baseURL: BASE_URL, validateStatus: () => true });

describe('GameCalls Engine Error Handling', () => {
    
    // =========================================================================
    // Invalid Master Call Tests
    // =========================================================================
    
    describe('Invalid Master Call Operations', () => {
        test('GET /calls/:id - Should return 404 for non-existent call', async () => {
            const response = await api.get('/calls/nonexistent-call-id');
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code');
            expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error for invalid call ID');
        });
        
        test('GET /calls/:id - Should return 404 for malformed UUID', async () => {
            const response = await api.get('/calls/not-a-valid-uuid');
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code');
            expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error for malformed UUID');
        });
    });
    
    // =========================================================================
    // Invalid Session Tests
    // =========================================================================
    
    describe('Invalid Session Operations', () => {
        test('POST /sessions - Should return 400 for missing masterCallId', async () => {
            const response = await api.post('/sessions', {
                // Missing required masterCallId
                options: { threshold: 0.85 }
            });
            
            expect(response.status).toBe(400);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 400 error for missing masterCallId');
        });
        
        test('POST /sessions - Should return 404 for non-existent master call', async () => {
            const response = await api.post('/sessions', {
                masterCallId: 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa',
                options: { threshold: 0.85 }
            });
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error for non-existent master call in session creation');
        });
        
        test('GET /sessions/:id - Should return 404 for non-existent session', async () => {
            const response = await api.get('/sessions/bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb');
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error for non-existent session');
        });
        
        test('POST /sessions/:id/start - Should return 404 for non-existent session', async () => {
            const response = await api.post('/sessions/cccccccc-cccc-cccc-cccc-cccccccccccc/start');
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error when starting non-existent session');
        });
        
        test('POST /sessions/:id/audio - Should return 404 for non-existent session', async () => {
            const audioBuffer = Buffer.alloc(1024);
            const response = await api.post(
                '/sessions/dddddddd-dddd-dddd-dddd-dddddddddddd/audio',
                audioBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error when sending audio to non-existent session');
        });
        
        test('POST /sessions/:id/stop - Should return 404 for non-existent session', async () => {
            const response = await api.post('/sessions/eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee/stop');
            
            expect(response.status).toBe(404);
            expect(response.data).toHaveProperty('code'); expect(response.data).toHaveProperty('message');
            console.log('✓ 404 error when stopping non-existent session');
        });
    });
    
    // =========================================================================
    // Session State Tests
    // =========================================================================
    
    describe('Session State Violations', () => {
        let sessionId;
        let masterCallId;
        
        beforeAll(async () => {
            // [20251030-TEST-005] Get the longest master call for reliable C++ engine finalization
            const callsResponse = await api.get('/calls?pageSize=20&sortBy=duration&sortOrder=desc');
            if (!callsResponse.data.items || callsResponse.data.items.length === 0) {
                throw new Error('No master calls available for testing. Run data seed first.');
            }
            // Use longest call (should be call_coyote_howl_001 at 5.2s)
            masterCallId = callsResponse.data.items[0].id;
            
            // Create a session
            const sessionResponse = await api.post('/sessions', {
                masterCallId,
                options: { threshold: 0.85 }
            });
            sessionId = sessionResponse.data.session.id;
        });
        
        afterAll(async () => {
            // Cleanup: stop session if it exists
            try {
                await api.post(`/sessions/${sessionId}/stop`);
            } catch (error) {
                // Ignore cleanup errors
            }
        });
        
        test('POST /sessions/:id/audio - Should return 400 when session not started', async () => {
            const audioBuffer = Buffer.alloc(1024);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                audioBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            // Should fail because session hasn't been started yet
            expect([400, 404]).toContain(response.status);
            console.log('✓ Error when sending audio to non-started session');
        });
        
        test('POST /sessions/:id/start - Should start successfully', async () => {
            const response = await api.post(`/sessions/${sessionId}/start`);
            expect(response.status).toBe(200);
            console.log('✓ Session started successfully');
        });
        
        test('POST /sessions/:id/start - Should return 400 for already-started session', async () => {
            const response = await api.post(`/sessions/${sessionId}/start`);
            
            // Should fail because session is already recording
            expect([400, 409]).toContain(response.status);
            console.log('✓ Error when starting already-started session');
        });
        
        test('POST /sessions/:id/stop - Should stop successfully', async () => {
            // [20251030-TEST-003] Ensure session is still in recording state
            // Process a small amount of audio to satisfy engine requirements
            const audioBuffer = Buffer.alloc(8192); // Minimum audio for finalization
            await api.post(
                `/sessions/${sessionId}/audio`,
                audioBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            const response = await api.post(`/sessions/${sessionId}/stop`);
            if (response.status !== 200) {
                console.log(`Stop failed with ${response.status}: ${JSON.stringify(response.data)}`);
            }
            expect(response.status).toBe(200);
            console.log('✓ Session stopped successfully');
        });
        
        test('POST /sessions/:id/audio - Should return 404 after session stopped', async () => {
            const audioBuffer = Buffer.alloc(1024);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                audioBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            // Should fail because session has been stopped
            expect(response.status).toBe(404);
            console.log('✓ Error when sending audio to stopped session');
        });
    });
    
    // =========================================================================
    // Malformed Audio Data Tests
    // =========================================================================
    
    describe('Malformed Audio Data', () => {
        let sessionId;
        
        beforeAll(async () => {
            // Get a valid master call and create/start a session
            const callsResponse = await api.get('/calls');
            if (!callsResponse.data.items || callsResponse.data.items.length === 0) {
                throw new Error('No master calls available for testing. Run data seed first.');
            }
            const masterCallId = callsResponse.data.items[0].id;
            
            const sessionResponse = await api.post('/sessions', {
                masterCallId,
                options: { threshold: 0.85 }
            });
            sessionId = sessionResponse.data.session.id;
            
            await api.post(`/sessions/${sessionId}/start`);
        });
        
        afterAll(async () => {
            // Cleanup
            try {
                await api.post(`/sessions/${sessionId}/stop`);
            } catch (error) {
                // Ignore
            }
        });
        
        test('POST /sessions/:id/audio - Should handle empty buffer', async () => {
            const emptyBuffer = Buffer.alloc(0);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                emptyBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            // Should either reject (400) or handle gracefully (200 with low score)
            expect([200, 400]).toContain(response.status);
            console.log('✓ Handled empty audio buffer');
        });
        
        test('POST /sessions/:id/audio - Should handle very small buffer', async () => {
            const tinyBuffer = Buffer.alloc(16); // Too small for meaningful audio
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                tinyBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            // Should either reject or handle gracefully
            expect([200, 400]).toContain(response.status);
            console.log('✓ Handled very small audio buffer');
        });
        
        test('POST /sessions/:id/audio - Should handle missing Content-Type', async () => {
            const audioBuffer = Buffer.alloc(1024);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                audioBuffer
                // No Content-Type header
            );
            
            // Should handle missing header
            expect([200, 400, 415]).toContain(response.status);
            console.log('✓ Handled missing Content-Type header');
        });
        
        test('POST /sessions/:id/audio - Should handle very large buffer', async () => {
            // 10MB buffer - might exceed limits
            const largeBuffer = Buffer.alloc(10 * 1024 * 1024);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                largeBuffer,
                { 
                    headers: { 'Content-Type': 'application/octet-stream' },
                    maxContentLength: 20 * 1024 * 1024 // Allow up to 20MB
                }
            );
            
            // Should either process or reject with appropriate error
            expect([200, 400, 413]).toContain(response.status);
            if (response.status === 413) {
                console.log('✓ Rejected oversized audio buffer (413 Payload Too Large)');
            } else {
                console.log('✓ Processed large audio buffer');
            }
        });
    });
    
    // =========================================================================
    // Concurrent Session Limits
    // =========================================================================
    
    describe('Concurrent Session Limits', () => {
        test('Should handle multiple concurrent sessions', async () => {
            // [20251030-TEST-006] Get longest master call for reliable processing
            const callsResponse = await api.get('/calls?pageSize=20&sortBy=duration&sortOrder=desc');
            if (!callsResponse.data.items || callsResponse.data.items.length === 0) {
                console.log('⚠️  No master calls available, skipping test');
                return;
            }
            const masterCallId = callsResponse.data.items[0].id;
            
            // Create 5 concurrent sessions
            const sessionPromises = Array(5).fill(null).map(() => 
                api.post('/sessions', {
                    masterCallId,
                    options: { threshold: 0.85 }
                })
            );
            
            const responses = await Promise.all(sessionPromises);
            const successfulSessions = responses.filter(r => r.status === 200 || r.status === 201);
            
            // [20251030-TEST-002] Debug concurrent session creation
            console.log(`Concurrent session results: ${responses.map(r => r.status).join(', ')}`);
            
            expect(successfulSessions.length).toBeGreaterThan(0);
            console.log(`✓ Created ${successfulSessions.length}/5 concurrent sessions`);
            
            // Cleanup all sessions
            await Promise.all(
                successfulSessions.map(r => 
                    api.post(`/sessions/${r.data.session.id}/stop`).catch(() => {})
                )
            );
        });
    });
    
    // =========================================================================
    // Invalid Query Parameters
    // =========================================================================
    
    describe('Invalid Query Parameters', () => {
        test('GET /calls - Should handle invalid filters gracefully', async () => {
            const response = await api.get('/calls', {
                params: {
                    species: 'invalid-species-name',
                    type: 'invalid-type'
                }
            });
            
            // Should return 200 with empty results or ignore invalid filters
            expect(response.status).toBe(200);
            expect(response.data).toHaveProperty('items');
            console.log('✓ Handled invalid query filters');
        });
    });
    
    // =========================================================================
    // Content-Type Validation
    // =========================================================================
    
    describe('Content-Type Validation', () => {
        let sessionId;
        
        beforeAll(async () => {
            const callsResponse = await api.get('/calls');
            if (!callsResponse.data.items || callsResponse.data.items.length === 0) {
                throw new Error('No master calls available for testing. Run data seed first.');
            }
            const masterCallId = callsResponse.data.items[0].id;
            
            const sessionResponse = await api.post('/sessions', {
                masterCallId,
                options: { threshold: 0.85 }
            });
            sessionId = sessionResponse.data.session.id;
            await api.post(`/sessions/${sessionId}/start`);
        });
        
        afterAll(async () => {
            try {
                await api.post(`/sessions/${sessionId}/stop`);
            } catch (error) {
                // Ignore
            }
        });
        
        test('POST /sessions - Should reject non-JSON body', async () => {
            const response = await api.post('/sessions', 'not-json', {
                headers: { 'Content-Type': 'text/plain' }
            });
            
            expect([400, 415]).toContain(response.status);
            console.log('✓ Rejected non-JSON request body');
        });
        
        test('POST /sessions/:id/audio - Should accept application/octet-stream', async () => {
            const audioBuffer = Buffer.alloc(1024);
            const response = await api.post(
                `/sessions/${sessionId}/audio`,
                audioBuffer,
                { headers: { 'Content-Type': 'application/octet-stream' } }
            );
            
            expect(response.status).toBe(200);
            console.log('✓ Accepted application/octet-stream for audio');
        });
    });
});
