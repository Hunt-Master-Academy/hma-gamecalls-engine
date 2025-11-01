// [20251029-TEST-001] API Integration Test - End-to-end session workflow via REST endpoints
require('dotenv').config();
const axios = require('axios');
const fs = require('fs');
const path = require('path');

const BASE_URL = 'http://localhost:5005';
const TIMEOUT = 10000;

describe('GameCalls Engine API Integration Test', () => {
  let sessionId;
  let masterCallId; // [20251030-TEST-007] Will be set to longest call dynamically

  // Helper to generate test audio buffer
  function generateTestAudio(samples = 8192) {
    const audioSamples = [];
    for (let i = 0; i < samples; i++) {
      audioSamples.push(Math.sin(2 * Math.PI * 440 * i / 44100));
    }
    return audioSamples;
  }

  test('GET /calls - Should list master calls', async () => {
    const response = await axios.get(`${BASE_URL}/calls?pageSize=20&sortBy=duration&sortOrder=desc`, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('items');
    expect(Array.isArray(response.data.items)).toBe(true);
    expect(response.data.items.length).toBeGreaterThan(0);
    
    // [20251030-TEST-008] Use longest call for subsequent tests
    masterCallId = response.data.items[0].id;
    
    console.log(`✓ Found ${response.data.items.length} master calls`);
    console.log(`✓ Using longest call: ${masterCallId} (${response.data.items[0].duration_seconds || response.data.items[0].durationSeconds}s)`);
  }, TIMEOUT);

  test('GET /calls/:id - Should get master call details', async () => {
    const response = await axios.get(`${BASE_URL}/calls/${masterCallId}`, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('call');
    expect(response.data.call).toHaveProperty('id', masterCallId);
    expect(response.data.call).toHaveProperty('species');
    expect(response.data.call).toHaveProperty('callType');
    
    console.log(`✓ Loaded master call: ${response.data.call.species} - ${response.data.call.callType}`);
  }, TIMEOUT);

  test('POST /sessions - Should create new session', async () => {
    const response = await axios.post(`${BASE_URL}/sessions`, {
      masterCallId: masterCallId,
      options: {
        sampleRate: 44100,
        bufferSize: 512
      }
    }, { timeout: TIMEOUT });
    
    expect(response.status).toBe(201);
    expect(response.data).toHaveProperty('session');
    expect(response.data.session).toHaveProperty('id');
    expect(response.data.session).toHaveProperty('status', 'created');
    expect(response.data.session).toHaveProperty('masterCallId', masterCallId);
    
    sessionId = response.data.session.id;
    console.log(`✓ Created session: ${sessionId}`);
  }, TIMEOUT);

  test('POST /sessions/:id/start - Should start recording', async () => {
    const response = await axios.post(`${BASE_URL}/sessions/${sessionId}/start`, {}, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('session');
    expect(response.data.session).toHaveProperty('id', sessionId);
    expect(response.data.session).toHaveProperty('status', 'recording');
    
    console.log(`✓ Started recording session: ${sessionId}`);
  }, TIMEOUT);

  test('POST /sessions/:id/audio - Should process audio chunk', async () => {
    const audioSamples = generateTestAudio(8192);
    
    const response = await axios.post(
      `${BASE_URL}/sessions/${sessionId}/audio`,
      {
        samples: audioSamples,
        sampleRate: 44100,
        timestamp: new Date().toISOString()
      },
      { timeout: TIMEOUT }
    );
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('analysis');
    expect(response.data.analysis).toHaveProperty('similarity');
    expect(typeof response.data.analysis.similarity.overall).toBe('number');
    
    console.log(`✓ Processed audio chunk - Similarity: ${response.data.analysis.similarity.overall.toFixed(4)}`);
  }, TIMEOUT);

  test.skip('POST /sessions/:id/finalize - Not implemented (use stop instead)', async () => {
    // Finalize functionality is part of stop endpoint
  }, TIMEOUT);

  test('GET /sessions/:id - Should get session status', async () => {
    const response = await axios.get(`${BASE_URL}/sessions/${sessionId}`, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('session');
    expect(response.data.session).toHaveProperty('id', sessionId);
    expect(response.data.session).toHaveProperty('status');
    expect(['recording', 'completed']).toContain(response.data.session.status);
    
    console.log(`✓ Session status: ${response.data.session.status}`);
  }, TIMEOUT);

  test('POST /sessions/:id/stop - Should stop and cleanup session', async () => {
    const response = await axios.post(`${BASE_URL}/sessions/${sessionId}/stop`, {}, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('message');
    
    console.log(`✓ Stopped session: ${sessionId}`);
  }, TIMEOUT);

  test('GET /health - Should return healthy status', async () => {
    const response = await axios.get(`${BASE_URL}/health`, { timeout: TIMEOUT });
    
    expect(response.status).toBe(200);
    expect(response.data).toHaveProperty('status', 'ok');
    expect(response.data).toHaveProperty('service');
    
    console.log(`✓ Service health check passed - Service: ${response.data.service}`);
  }, TIMEOUT);
});
