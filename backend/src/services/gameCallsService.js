/**
 * Game Calls Service
 * Handles master call management, loading, and metadata operations
 * Interfaces with PostgreSQL database and MinIO storage
 */

const { ApiError } = require('../middleware/errorHandler');
const { v4: uuidv4 } = require('uuid');
const minioService = require('./minioService');
const databaseService = require('./databaseService');

class GameCallsService {

    /**
     * List available master calls with filtering and pagination
     * [20251029-API-004] Real database implementation
     */
    static async listCalls({ page = 1, pageSize = 50, species, callType, difficulty, tags }) {
        try {
            const offset = (page - 1) * pageSize;
            const limit = Math.min(pageSize, 100); // Max 100 per page

            // [20251029-API-005] Build dynamic query with filters
            let query = `
                SELECT 
                    id, name, species, call_type, difficulty,
                    duration_seconds, sample_rate, description,
                    tags, season, context, quality_score,
                    usage_count, success_rate, is_premium,
                    created_at, updated_at
                FROM master_calls
                WHERE deleted_at IS NULL
            `;
            
            const params = [];
            let paramIndex = 1;

            if (species) {
                query += ` AND species = $${paramIndex++}`;
                params.push(species);
            }

            if (callType) {
                query += ` AND call_type = $${paramIndex++}`;
                params.push(callType);
            }

            if (difficulty) {
                query += ` AND difficulty = $${paramIndex++}`;
                params.push(difficulty);
            }

            if (tags && tags.length > 0) {
                query += ` AND tags && $${paramIndex++}`; // Array overlap operator
                params.push(tags);
            }

            // [20251029-API-006] Order by quality and usage
            query += ` ORDER BY quality_score DESC, usage_count DESC`;
            query += ` LIMIT $${paramIndex++} OFFSET $${paramIndex++}`;
            params.push(limit, offset);

            // [20251029-API-007] Execute query and count total
            const result = await databaseService.raw(query, params);
            
            const countQuery = `
                SELECT COUNT(*) as total
                FROM master_calls
                WHERE deleted_at IS NULL
                ${species ? 'AND species = $1' : ''}
                ${callType ? `AND call_type = $${species ? 2 : 1}` : ''}
                ${difficulty ? `AND difficulty = $${[species, callType].filter(Boolean).length + 1}` : ''}
            `;
            
            const countParams = [species, callType, difficulty].filter(Boolean);
            const countResult = await databaseService.raw(countQuery, countParams);
            const total = parseInt(countResult.rows[0].total);

            return {
                calls: result.rows,
                total,
                page,
                pageSize: limit,
                totalPages: Math.ceil(total / limit)
            };

        } catch (error) {
            throw ApiError.internal('LIST_CALLS_FAILED', `Failed to list calls: ${error.message}`);
        }
    }

    /**
     * Get individual master call by ID with full details
     * [20251029-API-008] Real database implementation
     */
    static async getCall(callId) {
        try {
            const query = `
                SELECT 
                    id, name, species, call_type, difficulty,
                    audio_file_path, duration_seconds, sample_rate, file_size_bytes, audio_format,
                    waveform_data_path, waveform_generated,
                    description, usage_notes, season, context, tags,
                    quality_score, validated, usage_count, success_rate,
                    is_public, is_premium,
                    created_at, updated_at
                FROM master_calls
                WHERE id = $1 AND deleted_at IS NULL
            `;

            const result = await databaseService.raw(query, [callId]);

            if (result.rows.length === 0) {
                throw ApiError.notFound('CALL_NOT_FOUND', `Master call with ID ${callId} not found`);
            }

            const call = result.rows[0];

            // [20251029-API-009] Generate presigned URL for audio file
            let audioUrl = null;
            if (minioService.isAvailable()) {
                try {
                    audioUrl = await minioService.getPresignedUrl(
                        'gamecalls-master-calls',
                        call.audio_file_path.replace('master-calls/', ''),
                        60 * 60 // 1 hour expiry
                    );
                } catch (error) {
                    console.warn(`Failed to generate audio URL for ${callId}:`, error.message);
                }
            }

            // [20251029-API-010] Increment usage count
            await databaseService.raw(
                'UPDATE master_calls SET usage_count = usage_count + 1 WHERE id = $1',
                [callId]
            );

            return {
                ...call,
                audioUrl,
                // Convert snake_case to camelCase for API response
                callType: call.call_type,
                audioFilePath: call.audio_file_path,
                durationSeconds: parseFloat(call.duration_seconds),
                sampleRate: call.sample_rate,
                fileSizeBytes: parseInt(call.file_size_bytes),
                audioFormat: call.audio_format,
                waveformDataPath: call.waveform_data_path,
                waveformGenerated: call.waveform_generated,
                usageNotes: call.usage_notes,
                qualityScore: parseFloat(call.quality_score),
                usageCount: call.usage_count,
                successRate: parseFloat(call.success_rate),
                isPremium: call.is_premium,
                isPublic: call.is_public,
                createdAt: call.created_at,
                updatedAt: call.updated_at
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('GET_CALL_FAILED', `Failed to get call: ${error.message}`);
        }
    }

    /**
     * Stream audio file for a master call
     * [20251029-AUDIO-002] Implemented audio streaming from MinIO with Range support
     */
    static async streamAudio(callId) {
        try {
            // [20251029-AUDIO-003] Get call metadata from database
            const query = `
                SELECT 
                    id, name, audio_file_path, file_size_bytes, audio_format
                FROM master_calls
                WHERE id = $1 AND deleted_at IS NULL
            `;

            const result = await databaseService.raw(query, [callId]);

            if (result.rows.length === 0) {
                throw ApiError.notFound('CALL_NOT_FOUND', `Master call with ID ${callId} not found`);
            }

            const call = result.rows[0];

            // [20251029-AUDIO-004] Check MinIO availability
            if (!minioService.isAvailable()) {
                throw ApiError.serviceUnavailable('MINIO_UNAVAILABLE', 'Storage service is not available');
            }

            // [20251029-AUDIO-005] Get audio stream from MinIO
            const objectPath = call.audio_file_path.replace('master-calls/', '');
            const stream = await minioService.getObjectStream('gamecalls-master-calls', objectPath);

            // [20251029-AUDIO-006] Determine content type based on audio format
            const audioFormatMap = {
                'wav': 'audio/wav',
                'mp3': 'audio/mpeg',
                'mpeg': 'audio/mpeg',
                'm4a': 'audio/mp4',
                'ogg': 'audio/ogg',
                'flac': 'audio/flac'
            };

            const contentType = audioFormatMap[call.audio_format] || 'audio/wav';
            const filename = `${call.name.replace(/\s+/g, '_')}.${call.audio_format}`;

            return {
                stream,
                contentType,
                contentLength: parseInt(call.file_size_bytes),
                filename
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('STREAM_AUDIO_FAILED', `Failed to stream audio: ${error.message}`);
        }
    }

    /**
     * Get call species and categories for filtering
     * [20251029-API-011] Real database implementation
     */
    static async getCallCategories() {
        try {
            // [20251029-API-012] Query actual species counts
            const speciesQuery = `
                SELECT 
                    species, 
                    COUNT(*) as call_count
                FROM master_calls
                WHERE deleted_at IS NULL AND is_public = TRUE
                GROUP BY species
                ORDER BY call_count DESC
            `;
            const speciesResult = await databaseService.raw(speciesQuery);

            // [20251029-API-013] Query actual call types
            const callTypesQuery = `
                SELECT 
                    call_type, 
                    COUNT(*) as call_count
                FROM master_calls
                WHERE deleted_at IS NULL AND is_public = TRUE
                GROUP BY call_type
                ORDER BY call_count DESC
            `;
            const callTypesResult = await databaseService.raw(callTypesQuery);

            // [20251029-API-014] Query all unique tags
            const tagsQuery = `
                SELECT DISTINCT unnest(tags) as tag
                FROM master_calls
                WHERE deleted_at IS NULL AND is_public = TRUE
                ORDER BY tag
            `;
            const tagsResult = await databaseService.raw(tagsQuery);

            return {
                species: speciesResult.rows.map(row => ({
                    id: row.species,
                    name: this._formatSpeciesName(row.species),
                    callCount: parseInt(row.call_count)
                })),
                callTypes: callTypesResult.rows.map(row => ({
                    id: row.call_type,
                    name: this._formatCallTypeName(row.call_type),
                    callCount: parseInt(row.call_count)
                })),
                difficulties: [
                    { id: 'beginner', name: 'Beginner', description: 'Easy to learn and practice' },
                    { id: 'intermediate', name: 'Intermediate', description: 'Moderate complexity' },
                    { id: 'advanced', name: 'Advanced', description: 'Challenging techniques required' },
                    { id: 'expert', name: 'Expert', description: 'Master-level techniques' }
                ],
                tags: tagsResult.rows.map(row => row.tag)
            };

        } catch (error) {
            throw ApiError.internal('GET_CATEGORIES_FAILED', `Failed to get categories: ${error.message}`);
        }
    }

    /**
     * Upload and process a new master call (admin function)
     * [20251029-API-015] Real implementation with MinIO and database
     */
    static async uploadCall(audioFile, metadata) {
        try {
            const callId = `call_${metadata.species}_${metadata.callType}_${Date.now()}`;
            
            // [20251029-API-016] Upload audio file to MinIO
            const audioPath = `${metadata.species}/${callId}.${metadata.audioFormat || 'wav'}`;
            
            if (minioService.isAvailable()) {
                await minioService.uploadFile(
                    'gamecalls-master-calls',
                    audioPath,
                    audioFile.buffer,
                    {
                        'Content-Type': `audio/${metadata.audioFormat || 'wav'}`,
                        'x-amz-meta-species': metadata.species,
                        'x-amz-meta-call-type': metadata.callType
                    }
                );
            } else {
                throw ApiError.serviceUnavailable('MINIO_UNAVAILABLE', 'Storage service is not available');
            }

            // [20251029-API-017] Insert into database
            const insertQuery = `
                INSERT INTO master_calls (
                    id, name, species, call_type, difficulty,
                    audio_file_path, duration_seconds, sample_rate, file_size_bytes, audio_format,
                    description, usage_notes, season, context, tags,
                    created_by
                ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16)
                RETURNING *
            `;

            const values = [
                callId,
                metadata.name,
                metadata.species,
                metadata.callType,
                metadata.difficulty || 'intermediate',
                `master-calls/${audioPath}`,
                metadata.durationSeconds || 0,
                metadata.sampleRate || 44100,
                audioFile.size,
                metadata.audioFormat || 'wav',
                metadata.description || '',
                metadata.usageNotes || '',
                metadata.season || null,
                metadata.context || null,
                metadata.tags || [],
                metadata.createdBy || 'system'
            ];

            const result = await databaseService.raw(insertQuery, values);

            return result.rows[0];

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw ApiError.internal('UPLOAD_CALL_FAILED', `Failed to upload call: ${error.message}`);
        }
    }

    /**
     * Update waveform data path in database
     * [20251029-WAVEFORM-020] Store waveform path after generation
     */
    static async updateWaveformPath(callId, waveformPath) {
        try {
            const query = `
                UPDATE master_calls
                SET waveform_data_path = $1, waveform_generated = TRUE, updated_at = NOW()
                WHERE id = $2
            `;

            await databaseService.raw(query, [waveformPath, callId]);

        } catch (error) {
            throw ApiError.internal('UPDATE_WAVEFORM_FAILED', `Failed to update waveform path: ${error.message}`);
        }
    }

    // [20251029-API-018] Helper functions for formatting
    static _formatSpeciesName(species) {
        return species.split('_').map(word => 
            word.charAt(0).toUpperCase() + word.slice(1)
        ).join(' ');
    }

    static _formatCallTypeName(callType) {
        return callType.split('_').map(word =>
            word.charAt(0).toUpperCase() + word.slice(1)
        ).join(' ');
    }
}

module.exports = GameCallsService;
