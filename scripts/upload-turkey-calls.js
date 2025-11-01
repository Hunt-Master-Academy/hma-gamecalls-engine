#!/usr/bin/env node
// [20251030-SCRIPT-001] Upload turkey calls from hma-content to MinIO and database

require('dotenv').config({ path: '../backend/.env' });
const fs = require('fs');
const path = require('path');
const { Client } = require('minio');
const { Pool } = require('pg');
const { spawn } = require('child_process');

// MinIO client
const minioClient = new Client({
    endPoint: process.env.MINIO_ENDPOINT || 'localhost',
    port: parseInt(process.env.MINIO_PORT) || 9000,
    useSSL: process.env.MINIO_USE_SSL === 'true',
    accessKey: process.env.MINIO_ACCESS_KEY || 'minioadmin',
    secretKey: process.env.MINIO_SECRET_KEY
});

// PostgreSQL pool
const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: parseInt(process.env.DB_PORT) || 5432,
    database: process.env.DB_NAME || 'hma_academy',
    user: process.env.DB_USER || 'hma_admin',
    password: process.env.DB_PASSWORD
});

const TURKEY_DIR = '/home/xbyooki/projects/hma-content/audio/game-calls/processed/normalized/turkey';
const BUCKET = 'gamecalls-master-calls';

// Map filename to call type
const callTypeMap = {
    'Kee_Kee_Run.wav': 'kee_kee',
    'Gobbling.wav': 'gobble',
    'Cutting.wav': 'cutting',
    'Tree_Calling.wav': 'tree_call',
    'Cluck_and_Purr.wav': 'cluck_purr',
    'Purrs.wav': 'purr',
    'Excited_Hen_Yelps.wav': 'yelp',
    'Plain_Hen_Yelp.wav': 'yelp',
    'Putts.wav': 'putt',
    'Fly_Down_Cackles.wav': 'cackle',
    'Clucks.wav': 'cluck',
    'Old_Hen_Assembly_Yelp.wav': 'assembly_yelp'
};

// Get audio duration using ffprobe
function getAudioDuration(filePath) {
    return new Promise((resolve, reject) => {
        const ffprobe = spawn('ffprobe', [
            '-v', 'error',
            '-show_entries', 'format=duration',
            '-of', 'default=noprint_wrappers=1:nokey=1',
            filePath
        ]);

        let output = '';
        ffprobe.stdout.on('data', (data) => { output += data.toString(); });
        ffprobe.on('close', (code) => {
            if (code === 0) {
                resolve(parseFloat(output.trim()));
            } else {
                reject(new Error(`ffprobe failed with code ${code}`));
            }
        });
    });
}

async function uploadTurkeyCalls() {
    try {
        console.log('🦃 Starting turkey call upload...\n');

        // Check if bucket exists
        const bucketExists = await minioClient.bucketExists(BUCKET);
        if (!bucketExists) {
            console.log(`Creating bucket: ${BUCKET}`);
            await minioClient.makeBucket(BUCKET, 'us-east-1');
        }

        // Get all WAV files
        const files = fs.readdirSync(TURKEY_DIR).filter(f => f.endsWith('.wav'));
        console.log(`Found ${files.length} turkey call WAV files\n`);

        for (const filename of files) {
            const filePath = path.join(TURKEY_DIR, filename);
            const callType = callTypeMap[filename] || 'unknown';
            const displayName = filename.replace('.wav', '').replace(/_/g, ' ');
            const callId = `call_turkey_${callType}_${filename.replace('.wav', '').toLowerCase().replace(/[^a-z0-9]/g, '_')}`;
            
            // Get duration
            const duration = await getAudioDuration(filePath);
            console.log(`📄 ${filename}`);
            console.log(`   Duration: ${duration.toFixed(2)}s`);
            console.log(`   Call ID: ${callId}`);
            console.log(`   Call Type: ${callType}`);

            // Upload to MinIO
            const objectName = `turkey/${callId}.wav`;
            await minioClient.fPutObject(BUCKET, objectName, filePath, {
                'Content-Type': 'audio/wav',
                'x-amz-meta-species': 'wild_turkey',
                'x-amz-meta-call-type': callType
            });
            console.log(`   ✅ Uploaded to MinIO: ${BUCKET}/${objectName}`);

            // Insert into database
            const query = `
                INSERT INTO master_calls (
                    id, name, species, call_type, difficulty, duration_seconds,
                    audio_file_path, quality_score, file_size_bytes, created_by
                ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)
                ON CONFLICT (id) DO UPDATE SET
                    duration_seconds = EXCLUDED.duration_seconds,
                    audio_file_path = EXCLUDED.audio_file_path,
                    file_size_bytes = EXCLUDED.file_size_bytes,
                    updated_at = NOW()
            `;
            
            const fileStats = fs.statSync(filePath);
            await pool.query(query, [
                callId,
                displayName,
                'wild_turkey',
                callType,
                'intermediate', // Difficulty
                duration,
                objectName,
                0.95, // High quality score
                fileStats.size,
                'system'
            ]);
            console.log(`   ✅ Added to database\n`);
        }

        console.log('✅ All turkey calls uploaded successfully!');
        
        // Summary
        const result = await pool.query(
            "SELECT COUNT(*) as count, AVG(duration_seconds) as avg_duration FROM master_calls WHERE species = 'wild_turkey'"
        );
        console.log(`\n📊 Turkey Calls Summary:`);
        console.log(`   Total: ${result.rows[0].count}`);
        console.log(`   Average Duration: ${parseFloat(result.rows[0].avg_duration).toFixed(2)}s`);

    } catch (error) {
        console.error('❌ Error:', error.message);
        process.exit(1);
    } finally {
        await pool.end();
    }
}

uploadTurkeyCalls();
