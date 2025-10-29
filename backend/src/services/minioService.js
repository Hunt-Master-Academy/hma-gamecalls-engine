// [20251028-STORAGE-001] MinIO Service for GameCalls master calls and analysis results
/**
 * MinIO Service
 * Handles S3-compatible object storage for audio files
 * Stores master calls, user recordings, and analysis results
 */

const Minio = require('minio');
const { ApiError } = require('../middleware/errorHandler');

class MinIOService {
    constructor() {
        this.client = null;
        this.buckets = {
            masterCalls: 'gamecalls-master-calls',
            userRecordings: 'gamecalls-user-recordings',
            analysisResults: 'gamecalls-analysis-results'
        };
    }

    /**
     * [20251028-STORAGE-002] Initialize MinIO client
     */
    initialize() {
        if (this.client) {
            return; // Already initialized
        }

        const endPoint = process.env.MINIO_ENDPOINT || 'minio';
        const port = parseInt(process.env.MINIO_PORT || '9000', 10);
        const useSSL = process.env.MINIO_USE_SSL === 'true';
        const accessKey = process.env.MINIO_ACCESS_KEY || 'minioadmin';
        const secretKey = process.env.MINIO_SECRET_KEY || 'minioadmin';

        this.client = new Minio.Client({
            endPoint,
            port,
            useSSL,
            accessKey,
            secretKey
        });

        console.log(`✅ MinIO client initialized: ${endPoint}:${port}`);
    }

    /**
     * [20251028-STORAGE-003] Ensure all required buckets exist
     */
    async ensureBuckets() {
        this.initialize();

        for (const [name, bucket] of Object.entries(this.buckets)) {
            try {
                const exists = await this.client.bucketExists(bucket);
                if (!exists) {
                    await this.client.makeBucket(bucket, 'us-east-1');
                    console.log(`✅ Created MinIO bucket: ${bucket}`);
                }
            } catch (error) {
                console.error(`❌ Failed to create bucket ${bucket}:`, error.message);
                throw new Error(`MinIO bucket creation failed: ${error.message}`);
            }
        }
    }

    /**
     * [20251028-STORAGE-004] Upload master call audio file
     * @param {string} callId - Unique identifier for the call
     * @param {Buffer} audioBuffer - Audio file data
     * @param {object} metadata - Call metadata (species, callType, etc.)
     * @returns {Promise<string>} Object key in MinIO
     */
    async uploadMasterCall(callId, audioBuffer, metadata = {}) {
        try {
            this.initialize();

            const objectName = `master-calls/${callId}.wav`;
            const metaData = {
                'Content-Type': 'audio/wav',
                'x-amz-meta-species': metadata.species || 'unknown',
                'x-amz-meta-call-type': metadata.callType || 'unknown',
                'x-amz-meta-difficulty': metadata.difficulty || 'beginner',
                'x-amz-meta-uploaded-at': new Date().toISOString()
            };

            await this.client.putObject(
                this.buckets.masterCalls,
                objectName,
                audioBuffer,
                audioBuffer.length,
                metaData
            );

            console.log(`✅ Uploaded master call: ${objectName}`);
            return objectName;

        } catch (error) {
            throw ApiError.internal('UPLOAD_FAILED', `Failed to upload master call: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-005] Get presigned URL for master call download
     * @param {string} callId - Master call ID
     * @param {number} expirySeconds - URL expiry time (default 1 hour)
     * @returns {Promise<string>} Presigned download URL
     */
    async getMasterCallUrl(callId, expirySeconds = 3600) {
        try {
            this.initialize();

            const objectName = `master-calls/${callId}.wav`;
            
            // Check if object exists
            await this.client.statObject(this.buckets.masterCalls, objectName);

            // Generate presigned URL
            const url = await this.client.presignedGetObject(
                this.buckets.masterCalls,
                objectName,
                expirySeconds
            );

            return url;

        } catch (error) {
            if (error.code === 'NotFound') {
                throw ApiError.notFound('MASTER_CALL_NOT_FOUND', `Master call ${callId} not found`);
            }
            throw ApiError.internal('URL_GENERATION_FAILED', `Failed to generate URL: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-006] Download master call to local path (for C++ engine)
     * @param {string} callId - Master call ID
     * @param {string} localPath - Where to save the file
     * @returns {Promise<string>} Local file path
     */
    async downloadMasterCall(callId, localPath) {
        try {
            this.initialize();

            const objectName = `master-calls/${callId}.wav`;
            await this.client.fGetObject(this.buckets.masterCalls, objectName, localPath);

            console.log(`✅ Downloaded master call to: ${localPath}`);
            return localPath;

        } catch (error) {
            if (error.code === 'NotFound') {
                throw ApiError.notFound('MASTER_CALL_NOT_FOUND', `Master call ${callId} not found`);
            }
            throw ApiError.internal('DOWNLOAD_FAILED', `Failed to download master call: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-007] Upload user recording for analysis
     * @param {string} sessionId - Session ID
     * @param {Buffer} audioBuffer - Recorded audio data
     * @returns {Promise<string>} Object key in MinIO
     */
    async uploadUserRecording(sessionId, audioBuffer) {
        try {
            this.initialize();

            const objectName = `recordings/${sessionId}/recording.wav`;
            const metaData = {
                'Content-Type': 'audio/wav',
                'x-amz-meta-session-id': sessionId,
                'x-amz-meta-recorded-at': new Date().toISOString()
            };

            await this.client.putObject(
                this.buckets.userRecordings,
                objectName,
                audioBuffer,
                audioBuffer.length,
                metaData
            );

            console.log(`✅ Uploaded user recording: ${objectName}`);
            return objectName;

        } catch (error) {
            throw ApiError.internal('UPLOAD_FAILED', `Failed to upload recording: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-008] Store analysis results as JSON
     * @param {string} sessionId - Session ID
     * @param {object} results - Analysis results object
     * @returns {Promise<string>} Object key in MinIO
     */
    async storeAnalysisResults(sessionId, results) {
        try {
            this.initialize();

            const objectName = `analysis/${sessionId}/results.json`;
            const jsonBuffer = Buffer.from(JSON.stringify(results, null, 2));
            const metaData = {
                'Content-Type': 'application/json',
                'x-amz-meta-session-id': sessionId,
                'x-amz-meta-analyzed-at': new Date().toISOString()
            };

            await this.client.putObject(
                this.buckets.analysisResults,
                objectName,
                jsonBuffer,
                jsonBuffer.length,
                metaData
            );

            console.log(`✅ Stored analysis results: ${objectName}`);
            return objectName;

        } catch (error) {
            throw ApiError.internal('STORAGE_FAILED', `Failed to store analysis: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-009] List all master calls with metadata
     * @param {object} filters - Optional filters (species, callType, difficulty)
     * @returns {Promise<Array>} List of master calls
     */
    async listMasterCalls(filters = {}) {
        try {
            this.initialize();

            const stream = this.client.listObjects(this.buckets.masterCalls, 'master-calls/', true);
            const calls = [];

            return new Promise((resolve, reject) => {
                stream.on('data', async (obj) => {
                    try {
                        // Get metadata for filtering
                        const stat = await this.client.statObject(this.buckets.masterCalls, obj.name);
                        const metadata = stat.metaData;

                        // Apply filters
                        if (filters.species && metadata['x-amz-meta-species'] !== filters.species) {
                            return;
                        }
                        if (filters.callType && metadata['x-amz-meta-call-type'] !== filters.callType) {
                            return;
                        }
                        if (filters.difficulty && metadata['x-amz-meta-difficulty'] !== filters.difficulty) {
                            return;
                        }

                        calls.push({
                            id: obj.name.replace('master-calls/', '').replace('.wav', ''),
                            objectName: obj.name,
                            size: obj.size,
                            species: metadata['x-amz-meta-species'],
                            callType: metadata['x-amz-meta-call-type'],
                            difficulty: metadata['x-amz-meta-difficulty'],
                            uploadedAt: metadata['x-amz-meta-uploaded-at']
                        });
                    } catch (error) {
                        console.error(`Error processing object ${obj.name}:`, error.message);
                    }
                });

                stream.on('end', () => resolve(calls));
                stream.on('error', (error) => reject(error));
            });

        } catch (error) {
            throw ApiError.internal('LIST_FAILED', `Failed to list master calls: ${error.message}`);
        }
    }

    /**
     * [20251028-STORAGE-010] Delete master call
     * @param {string} callId - Master call ID
     */
    async deleteMasterCall(callId) {
        try {
            this.initialize();

            const objectName = `master-calls/${callId}.wav`;
            await this.client.removeObject(this.buckets.masterCalls, objectName);

            console.log(`✅ Deleted master call: ${objectName}`);

        } catch (error) {
            throw ApiError.internal('DELETE_FAILED', `Failed to delete master call: ${error.message}`);
        }
    }

    /**
     * [20251029-AUDIO-007] Generic method to get object stream from MinIO
     * @param {string} bucketName - Bucket name
     * @param {string} objectName - Object path/key
     * @returns {Promise<Stream>} Readable stream of the object
     */
    async getObjectStream(bucketName, objectName) {
        try {
            this.initialize();

            // Check if object exists first
            await this.client.statObject(bucketName, objectName);

            // Get object as stream
            const stream = await this.client.getObject(bucketName, objectName);
            
            return stream;

        } catch (error) {
            if (error.code === 'NotFound') {
                throw ApiError.notFound('OBJECT_NOT_FOUND', `Object ${objectName} not found in bucket ${bucketName}`);
            }
            throw ApiError.internal('STREAM_FAILED', `Failed to get object stream: ${error.message}`);
        }
    }

    /**
     * [20251029-AUDIO-008] Generic method to upload file to MinIO
     * @param {string} bucketName - Bucket name
     * @param {string} objectName - Object path/key
     * @param {Buffer} buffer - File data
     * @param {object} metadata - Optional metadata
     * @returns {Promise<string>} Object name
     */
    async uploadFile(bucketName, objectName, buffer, metadata = {}) {
        try {
            this.initialize();

            await this.client.putObject(
                bucketName,
                objectName,
                buffer,
                buffer.length,
                metadata
            );

            console.log(`✅ Uploaded file: ${bucketName}/${objectName}`);
            return objectName;

        } catch (error) {
            throw ApiError.internal('UPLOAD_FAILED', `Failed to upload file: ${error.message}`);
        }
    }

    /**
     * [20251029-AUDIO-009] Generic method to get presigned URL
     * @param {string} bucketName - Bucket name
     * @param {string} objectName - Object path/key
     * @param {number} expirySeconds - URL expiry time
     * @returns {Promise<string>} Presigned URL
     */
    async getPresignedUrl(bucketName, objectName, expirySeconds = 3600) {
        try {
            this.initialize();

            // Check if object exists
            await this.client.statObject(bucketName, objectName);

            // Generate presigned URL
            const url = await this.client.presignedGetObject(
                bucketName,
                objectName,
                expirySeconds
            );

            return url;

        } catch (error) {
            if (error.code === 'NotFound') {
                throw ApiError.notFound('OBJECT_NOT_FOUND', `Object ${objectName} not found`);
            }
            throw ApiError.internal('URL_GENERATION_FAILED', `Failed to generate URL: ${error.message}`);
        }
    }

    /**
     * Check if MinIO service is available
     */
    isAvailable() {
        return this.client !== null;
    }
}

// Export singleton instance
module.exports = new MinIOService();
