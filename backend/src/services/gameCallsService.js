/**
 * Game Calls Service
 * Handles master call management, loading, and metadata operations
 * Interfaces with the C++ Huntmaster Audio Engine for actual processing
 */

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
     * Get specific master call by ID
     */
    static async getCall(callId) {
        try {
            // TODO: Interface with C++ engine UnifiedAudioEngine for call loading
            
            const mockCalls = await this.listCalls({ pageSize: 1000 });
            const call = mockCalls.calls.find(c => c.id === callId);

            if (!call) {
                throw ApiError.notFound('CALL_NOT_FOUND', `Master call with ID ${callId} not found`);
            }

            // Add extended metadata for individual call requests
            return {
                ...call,
                analysis: {
                    fundamentalFrequency: call.species === 'whitetail_deer' ? 120 : call.species === 'wild_turkey' ? 300 : 200,
                    harmonics: call.difficulty === 'advanced' ? 5 : 3,
                    cadencePattern: call.callType === 'sequence' ? [0.5, 0.3, 0.7, 0.4] : [1.0],
                    pitchRange: {
                        min: call.species === 'elk' ? 80 : 150,
                        max: call.species === 'elk' ? 400 : 800
                    }
                },
                waveform: {
                    peaks: Array.from({length: 100}, (_, i) => Math.sin(i * 0.1) * Math.random()),
                    sampleRate: call.sampleRate,
                    duration: call.duration
                }
            };

        } catch (error) {
            if (error instanceof ApiError) throw error;
            throw new Error(`Failed to get call: ${error.message}`);
        }
    }

    /**
     * Get call species and categories for filtering
     */
    static async getCallCategories() {
        try {
            // TODO: Load from actual call database
            
            return {
                species: [
                    { id: 'whitetail_deer', name: 'Whitetail Deer', callCount: 12 },
                    { id: 'wild_turkey', name: 'Wild Turkey', callCount: 18 },
                    { id: 'elk', name: 'Elk', callCount: 8 },
                    { id: 'mallard_duck', name: 'Mallard Duck', callCount: 15 },
                    { id: 'canada_goose', name: 'Canada Goose', callCount: 6 },
                    { id: 'coyote', name: 'Coyote', callCount: 10 }
                ],
                callTypes: [
                    { id: 'grunt', name: 'Grunt', description: 'Low frequency vocalizations' },
                    { id: 'yelp', name: 'Yelp', description: 'Sharp, quick calls' },
                    { id: 'bugle', name: 'Bugle', description: 'Long, melodic calls' },
                    { id: 'quack', name: 'Quack', description: 'Duck vocalizations' },
                    { id: 'honk', name: 'Honk', description: 'Goose calls' },
                    { id: 'howl', name: 'Howl', description: 'Long predator calls' }
                ],
                difficulties: [
                    { id: 'beginner', name: 'Beginner', description: 'Easy to learn and practice' },
                    { id: 'intermediate', name: 'Intermediate', description: 'Moderate complexity' },
                    { id: 'advanced', name: 'Advanced', description: 'Challenging techniques required' }
                ],
                tags: [
                    'rut', 'spring', 'winter', 'basic', 'advanced', 'sequence', 
                    'attract', 'challenge', 'hen', 'buck', 'bull', 'gobbler'
                ]
            };

        } catch (error) {
            throw new Error(`Failed to get call categories: ${error.message}`);
        }
    }

    /**
     * Upload and process a new master call (admin function)
     */
    static async uploadCall(audioFile, metadata) {
        try {
            // TODO: Interface with C++ engine for audio processing and MFCC cache generation
            
            const callId = `call_${metadata.species}_${metadata.callType}_${uuidv4().substr(0, 8)}`;
            
            const newCall = {
                id: callId,
                name: metadata.name,
                species: metadata.species,
                callType: metadata.callType,
                difficulty: metadata.difficulty || 'intermediate',
                duration: metadata.duration || 0,
                sampleRate: 44100,
                description: metadata.description || '',
                tags: metadata.tags || [],
                audioFile: `/audio/${callId}.wav`,
                waveformData: `/waveforms/${callId}.json`,
                createdAt: new Date().toISOString(),
                updatedAt: new Date().toISOString(),
                status: 'processing' // Would be updated after C++ processing
            };

            // TODO: Save to database and trigger C++ processing
            
            return newCall;

        } catch (error) {
            throw new Error(`Failed to upload call: ${error.message}`);
        }
    }
}

module.exports = GameCallsService;