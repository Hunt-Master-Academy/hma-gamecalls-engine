// [20251029-DB-006] Database service singleton for PostgreSQL queries
const { Pool } = require('pg');

class DatabaseService {
    constructor() {
        this.pool = null;
    }

    initialize() {
        if (this.pool) return;

        this.pool = new Pool({
            host: process.env.DB_HOST || 'localhost',
            port: parseInt(process.env.DB_PORT || '5432'),
            database: process.env.DB_NAME || 'hma_academy',
            user: process.env.DB_USER || 'hma_admin',
            password: process.env.DB_PASSWORD || 'development_password',
            max: 20,
            idleTimeoutMillis: 30000,
            connectionTimeoutMillis: 2000,
        });

        console.log(`✅ PostgreSQL connected: ${this.pool.options.host}:${this.pool.options.port}/${this.pool.options.database}`);
    }

    async raw(query, params = []) {
        if (!this.pool) this.initialize();
        return await this.pool.query(query, params);
    }

    async close() {
        if (this.pool) {
            await this.pool.end();
            this.pool = null;
        }
    }
}

module.exports = new DatabaseService();
