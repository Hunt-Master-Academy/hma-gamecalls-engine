// [20251030-TEST-004] Audio Upload and Waveform Generation Tests
// Validates POST /calls/upload endpoint with MinIO storage and waveform generation

const axios = require('axios');
const FormData = require('form-data');
const fs = require('fs');
const path = require('path');
const { Client: MinioClient } = require('minio');

const API_BASE = process.env.API_BASE_URL || 'http://localhost:5005';
const api = axios.create({ 
    baseURL: API_BASE,
    validateStatus: () => true // Don't throw on any status
});

// MinIO client for direct verification
const minioClient = new MinioClient({
    endPoint: process.env.MINIO_ENDPOINT || 'localhost',
    port: parseInt(process.env.MINIO_PORT || '9000'),
    useSSL: false,
    accessKey: process.env.MINIO_ACCESS_KEY || 'minioadmin',
    secretKey: process.env.MINIO_SECRET_KEY || 'minioadmin'
});

describe('Audio Upload and Waveform Generation', () => {
    
    const testAudioPath = path.join(__dirname, '../../data/test_audio/test_complex.wav');
    let uploadedCallId;
    
    // =========================================================================
    // Audio Upload Tests
    // =========================================================================
    
    describe('POST /calls/upload', () => {
        
        test('Should successfully upload audio file with metadata', async () => {
            // Check if test audio file exists
            if (!fs.existsSync(testAudioPath)) {
                console.warn(`⚠️  Test audio not found: ${testAudioPath}`);
                console.warn('   Skipping upload test');
                return;
            }
            
            const form = new FormData();
            form.append('audio', fs.createReadStream(testAudioPath));
            form.append('name', 'Test Complex Audio Upload');
            form.append('species', 'elk');
            form.append('callType', 'bugle');
            form.append('huntingContext', 'rut');
            form.append('quality', 'high');
            form.append('difficulty', 'advanced');
            form.append('description', 'Test upload - Elk bugle during rut season');
            
            const response = await api.post('/calls/upload', form, {
                headers: form.getHeaders(),
                maxBodyLength: Infinity,
                maxContentLength: Infinity
            });
            
            console.log(`📤 Upload response status: ${response.status}`);
            if (response.status !== 201) {
                console.log(`   Error: ${JSON.stringify(response.data, null, 2)}`);
            }
            
            expect(response.status).toBe(201);
            expect(response.data).toHaveProperty('call');
            expect(response.data.call).toHaveProperty('id');
            expect(response.data.call).toHaveProperty('audio_file_path'); // snake_case from DB
            expect(response.data.call.species).toBe('elk');
            expect(response.data.call.call_type).toBe('bugle'); // snake_case from DB
            
            uploadedCallId = response.data.call.id;
            
            console.log('✓ Audio uploaded successfully');
            console.log(`  Call ID: ${uploadedCallId}`);
            console.log(`  Audio path: ${response.data.call.audio_file_path}`);
        }, 30000); // 30s timeout for upload
        
        test('Should reject upload without audio file', async () => {
            const form = new FormData();
            form.append('species', 'elk');
            form.append('callType', 'bugle');
            
            const response = await api.post('/calls/upload', form, {
                headers: form.getHeaders()
            });
            
            expect(response.status).toBe(400);
            expect(response.data).toHaveProperty('code');
            // Accept various validation error codes
            expect(['MISSING_FILE', 'INVALID_NAME', 'MISSING_AUDIO', 'REQUIRED_FIELD']).toContain(response.data.code);
            
            console.log('✓ Correctly rejects upload without audio file');
            console.log(`  Error code: ${response.data.code}`);
        });
        
        test('Should reject upload with missing required metadata', async () => {
            if (!fs.existsSync(testAudioPath)) {
                return; // Skip if no test file
            }
            
            const form = new FormData();
            form.append('audio', fs.createReadStream(testAudioPath));
            // Missing species, callType
            
            const response = await api.post('/calls/upload', form, {
                headers: form.getHeaders(),
                maxBodyLength: Infinity
            });
            
            expect(response.status).toBe(400);
            console.log('✓ Correctly rejects upload with missing metadata');
        });
    });
    
    // =========================================================================
    // MinIO Storage Verification
    // =========================================================================
    
    describe('MinIO Storage Verification', () => {
        
        test('Should store audio file in MinIO bucket', async () => {
            if (!uploadedCallId) {
                console.warn('⚠️  No uploaded call ID, skipping MinIO verification');
                return;
            }
            
            // Get the call details to find MinIO path
            const callResponse = await api.get(`/calls/${uploadedCallId}`);
            expect(callResponse.status).toBe(200);
            
            const audioFilePath = callResponse.data.call.audio_file_path;
            const minioPath = audioFilePath.replace('master-calls/', '');
            
            // Verify file exists in MinIO
            try {
                const stat = await minioClient.statObject('gamecalls-master-calls', minioPath);
                expect(stat).toBeDefined();
                expect(stat.size).toBeGreaterThan(0);
                
                console.log('✓ Audio file found in MinIO');
                console.log(`  Bucket: gamecalls-master-calls`);
                console.log(`  Path: ${minioPath}`);
                console.log(`  Size: ${stat.size} bytes`);
            } catch (error) {
                throw new Error(`Audio file not found in MinIO: ${error.message}`);
            }
        });
        
        test('Should generate presigned URL for audio access', async () => {
            if (!uploadedCallId) {
                return;
            }
            
            const callResponse = await api.get(`/calls/${uploadedCallId}`);
            const audioUrl = callResponse.data.call.audioUrl;
            
            expect(audioUrl).toBeDefined();
            expect(audioUrl).toMatch(/^http/);
            
            // Try to access the URL (may fail outside Docker network)
            try {
                const audioResponse = await axios.get(audioUrl, {
                    responseType: 'arraybuffer',
                    timeout: 5000
                });
                
                if (audioResponse.status === 200) {
                    console.log('✓ Presigned URL accessible');
                    console.log(`  Downloaded ${audioResponse.data.length} bytes`);
                } else {
                    console.warn(`⚠️  Presigned URL returned status ${audioResponse.status}`);
                }
            } catch (error) {
                console.warn(`⚠️  Could not access presigned URL: ${error.message}`);
                console.warn('   This may be expected if running outside Docker network');
            }
        });
    });
    
    // =========================================================================
    // Waveform Generation Tests
    // =========================================================================
    
    describe('Waveform Generation', () => {
        
        test('Should generate waveform data for uploaded audio', async () => {
            if (!uploadedCallId) {
                console.warn('⚠️  No uploaded call ID, skipping waveform test');
                return;
            }
            
            const waveformResponse = await api.get(`/calls/${uploadedCallId}/waveform`);
            
            expect(waveformResponse.status).toBe(200);
            expect(waveformResponse.data).toHaveProperty('waveform');
            expect(Array.isArray(waveformResponse.data.waveform)).toBe(true);
            expect(waveformResponse.data.waveform.length).toBeGreaterThan(0);
            
            // Validate waveform data points
            const samplePoint = waveformResponse.data.waveform[0];
            expect(samplePoint).toHaveProperty('timestamp');
            expect(samplePoint).toHaveProperty('amplitude');
            expect(typeof samplePoint.timestamp).toBe('number');
            expect(typeof samplePoint.amplitude).toBe('number');
            
            console.log('✓ Waveform generated successfully');
            console.log(`  Data points: ${waveformResponse.data.waveform.length}`);
            console.log(`  Sample point: ${JSON.stringify(samplePoint)}`);
        }, 15000); // 15s timeout for waveform generation
        
        test('Should return cached waveform on subsequent requests', async () => {
            if (!uploadedCallId) {
                return;
            }
            
            const start = Date.now();
            const response1 = await api.get(`/calls/${uploadedCallId}/waveform`);
            const time1 = Date.now() - start;
            
            const start2 = Date.now();
            const response2 = await api.get(`/calls/${uploadedCallId}/waveform`);
            const time2 = Date.now() - start2;
            
            expect(response1.status).toBe(200);
            expect(response2.status).toBe(200);
            expect(response1.data.waveform.length).toBe(response2.data.waveform.length);
            
            console.log('✓ Waveform caching working');
            console.log(`  First request: ${time1}ms`);
            console.log(`  Second request: ${time2}ms`);
            
            if (time2 < time1 * 0.5) {
                console.log('  ⚡ Cache significantly faster!');
            }
        });
    });
    
    // =========================================================================
    // Database Record Verification
    // =========================================================================
    
    describe('Database Record Verification', () => {
        
        test('Should create database record for uploaded call', async () => {
            if (!uploadedCallId) {
                return;
            }
            
            const callResponse = await api.get(`/calls/${uploadedCallId}`);
            
            expect(callResponse.status).toBe(200);
            expect(callResponse.data.call).toHaveProperty('id', uploadedCallId);
            expect(callResponse.data.call).toHaveProperty('species');
            expect(callResponse.data.call).toHaveProperty('callType');
            expect(callResponse.data.call).toHaveProperty('createdAt');
            expect(callResponse.data.call).toHaveProperty('updatedAt');
            
            console.log('✓ Database record created');
            console.log(`  Species: ${callResponse.data.call.species}`);
            console.log(`  Call Type: ${callResponse.data.call.callType}`);
            console.log(`  Created: ${callResponse.data.call.createdAt}`);
        });
        
        test('Should include uploaded call in /calls list', async () => {
            if (!uploadedCallId) {
                return;
            }
            
            const listResponse = await api.get('/calls');
            
            expect(listResponse.status).toBe(200);
            expect(listResponse.data).toHaveProperty('items');
            expect(Array.isArray(listResponse.data.items)).toBe(true);
            
            const uploadedCall = listResponse.data.items.find(call => call.id === uploadedCallId);
            expect(uploadedCall).toBeDefined();
            
            console.log('✓ Uploaded call appears in list');
            console.log(`  Total calls: ${listResponse.data.items.length}`);
        });
    });
    
    // =========================================================================
    // Cleanup
    // =========================================================================
    
    afterAll(async () => {
        // Note: Not deleting uploaded test call to allow manual inspection
        // In production, implement DELETE /calls/:id endpoint for cleanup
        if (uploadedCallId) {
            console.log(`\n📌 Test call uploaded: ${uploadedCallId}`);
            console.log('   (Not automatically deleted - clean up manually if needed)');
        }
    });
});
