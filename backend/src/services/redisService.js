// [20251028-CACHE-001] Redis Service for distributed session caching
/**
 * Redis Service
 * Handles distributed session state caching for GameCalls Engine
 * Replaces in-memory Map for production multi-instance deployment
 */

const Redis = require('ioredis');
const { ApiError } = require('../middleware/errorHandler');

class RedisService {
    constructor() {
        this.client = null;
        this.isConnected = false;
    }

    /**
     * [20251028-CACHE-002] Initialize Redis client
     */
    initialize() {
        if (this.client) {
            return; // Already initialized
        }

        const host = process.env.REDIS_HOST || 'redis';
        const port = parseInt(process.env.REDIS_PORT || '6379', 10);
        const password = process.env.REDIS_PASSWORD || 'development_redis';
        const db = parseInt(process.env.REDIS_DB || '0', 10);

        this.client = new Redis({
            host,
            port,
            password,
            db,
            retryStrategy: (times) => {
                const delay = Math.min(times * 50, 2000);
                return delay;
            },
            maxRetriesPerRequest: 3,
            enableReadyCheck: true,
            lazyConnect: false
        });

        this.client.on('connect', () => {
            console.log(`✅ Redis connected: ${host}:${port}`);
            this.isConnected = true;
        });

        this.client.on('error', (error) => {
            console.error(`❌ Redis error:`, error.message);
            this.isConnected = false;
        });

        this.client.on('close', () => {
            console.log('⚠️  Redis connection closed');
            this.isConnected = false;
        });
    }

    /**
     * [20251028-CACHE-003] Store session data with TTL
     * @param {string} sessionId - Session UUID
     * @param {object} sessionData - Session metadata object
     * @param {number} ttlSeconds - Time to live (default 1 hour)
     */
    async setSession(sessionId, sessionData, ttlSeconds = 3600) {
        try {
            this.initialize();

            const key = `gamecalls:session:${sessionId}`;
            const value = JSON.stringify(sessionData);

            await this.client.setex(key, ttlSeconds, value);
            console.log(`💾 Cached session: ${sessionId} (TTL: ${ttlSeconds}s)`);

        } catch (error) {
            console.error(`Failed to cache session ${sessionId}:`, error.message);
            // Don't throw - Redis is cache, not primary storage
        }
    }

    /**
     * [20251028-CACHE-004] Retrieve session data from cache
     * @param {string} sessionId - Session UUID
     * @returns {Promise<object|null>} Session data or null if not found
     */
    async getSession(sessionId) {
        try {
            this.initialize();

            const key = `gamecalls:session:${sessionId}`;
            const value = await this.client.get(key);

            if (!value) {
                return null;
            }

            return JSON.parse(value);

        } catch (error) {
            console.error(`Failed to retrieve session ${sessionId}:`, error.message);
            return null; // Cache miss is not an error
        }
    }

    /**
     * [20251028-CACHE-005] Update session data in cache
     * @param {string} sessionId - Session UUID
     * @param {object} updates - Partial session data to merge
     * @param {number} ttlSeconds - Reset TTL (default 1 hour)
     */
    async updateSession(sessionId, updates, ttlSeconds = 3600) {
        try {
            this.initialize();

            const existing = await this.getSession(sessionId);
            if (!existing) {
                console.warn(`Cannot update non-existent session: ${sessionId}`);
                return false;
            }

            const merged = { ...existing, ...updates, updatedAt: new Date().toISOString() };
            await this.setSession(sessionId, merged, ttlSeconds);

            return true;

        } catch (error) {
            console.error(`Failed to update session ${sessionId}:`, error.message);
            return false;
        }
    }

    /**
     * [20251028-CACHE-006] Delete session from cache
     * @param {string} sessionId - Session UUID
     */
    async deleteSession(sessionId) {
        try {
            this.initialize();

            const key = `gamecalls:session:${sessionId}`;
            await this.client.del(key);
            console.log(`🗑️  Removed session from cache: ${sessionId}`);

        } catch (error) {
            console.error(`Failed to delete session ${sessionId}:`, error.message);
        }
    }

    /**
     * [20251028-CACHE-007] List all active session IDs
     * @returns {Promise<string[]>} Array of session UUIDs
     */
    async listSessions() {
        try {
            this.initialize();

            const pattern = 'gamecalls:session:*';
            const keys = await this.client.keys(pattern);

            // Extract session IDs from keys
            const sessionIds = keys.map(key => key.replace('gamecalls:session:', ''));

            return sessionIds;

        } catch (error) {
            console.error('Failed to list sessions:', error.message);
            return [];
        }
    }

    /**
     * [20251028-CACHE-008] Extend session TTL (keep-alive)
     * @param {string} sessionId - Session UUID
     * @param {number} ttlSeconds - New TTL in seconds
     */
    async extendSessionTTL(sessionId, ttlSeconds = 3600) {
        try {
            this.initialize();

            const key = `gamecalls:session:${sessionId}`;
            await this.client.expire(key, ttlSeconds);

        } catch (error) {
            console.error(`Failed to extend TTL for ${sessionId}:`, error.message);
        }
    }

    /**
     * [20251028-CACHE-009] Get session TTL remaining
     * @param {string} sessionId - Session UUID
     * @returns {Promise<number>} Seconds remaining or -1 if not found
     */
    async getSessionTTL(sessionId) {
        try {
            this.initialize();

            const key = `gamecalls:session:${sessionId}`;
            return await this.client.ttl(key);

        } catch (error) {
            console.error(`Failed to get TTL for ${sessionId}:`, error.message);
            return -1;
        }
    }

    /**
     * [20251028-CACHE-010] Store analysis results (longer TTL)
     * @param {string} sessionId - Session UUID
     * @param {object} analysis - Analysis results object
     * @param {number} ttlSeconds - TTL (default 24 hours)
     */
    async setAnalysisResults(sessionId, analysis, ttlSeconds = 86400) {
        try {
            this.initialize();

            const key = `gamecalls:analysis:${sessionId}`;
            const value = JSON.stringify(analysis);

            await this.client.setex(key, ttlSeconds, value);
            console.log(`💾 Cached analysis results: ${sessionId}`);

        } catch (error) {
            console.error(`Failed to cache analysis ${sessionId}:`, error.message);
        }
    }

    /**
     * [20251028-CACHE-011] Get analysis results from cache
     * @param {string} sessionId - Session UUID
     * @returns {Promise<object|null>} Analysis data or null
     */
    async getAnalysisResults(sessionId) {
        try {
            this.initialize();

            const key = `gamecalls:analysis:${sessionId}`;
            const value = await this.client.get(key);

            if (!value) {
                return null;
            }

            return JSON.parse(value);

        } catch (error) {
            console.error(`Failed to retrieve analysis ${sessionId}:`, error.message);
            return null;
        }
    }

    /**
     * [20251028-CACHE-012] Health check
     * @returns {Promise<boolean>} True if Redis is responsive
     */
    async healthCheck() {
        try {
            this.initialize();
            await this.client.ping();
            return true;
        } catch (error) {
            return false;
        }
    }

    /**
     * Check if Redis is connected
     */
    isAvailable() {
        return this.isConnected;
    }

    /**
     * [20251028-CACHE-013] Graceful shutdown
     */
    async disconnect() {
        if (this.client) {
            await this.client.quit();
            console.log('✅ Redis disconnected gracefully');
        }
    }
}

// Export singleton instance
module.exports = new RedisService();
