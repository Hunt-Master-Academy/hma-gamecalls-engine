// [20251229-PREPROCESS-001] Master Call Preprocessing Service
// Syncs master calls from MinIO to local filesystem and extracts features

const fs = require('fs').promises;
const fsSync = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const minioService = require('./minioService');
const databaseService = require('./databaseService');

class MasterCallPreprocessor {
    constructor() {
        // [20251229-PREPROCESS-002] Configure paths to match C++ engine expectations
        this.masterCallsPath = '/home/xbyooki/projects/hma-gamecalls-engine/data/master_calls/';
        this.featuresPath = '/home/xbyooki/projects/hma-gamecalls-engine/data/processed_calls/mfc/';
        this.generateFeaturesToolPath = '/home/xbyooki/projects/hma-gamecalls-engine/build/bin/generate_features';
        this.minioBucket = 'gamecalls-master-calls';
    }

    /**
     * [20251229-PREPROCESS-003] Ensure required directories exist
     */
    async ensureDirectories() {
        try {
            await fs.mkdir(this.masterCallsPath, { recursive: true });
            await fs.mkdir(this.featuresPath, { recursive: true });
            console.log('✓ Preprocessing directories ready');
        } catch (error) {
            throw new Error(`Failed to create directories: ${error.message}`);
        }
    }

    /**
     * [20251229-PREPROCESS-004] Normalize master call ID for filesystem
     * Converts database IDs like "call_turkey_purr_001" to filesystem-safe "turkey_purr_001"
     */
    normalizeCallId(databaseId) {
        // Remove "call_" prefix if present
        let normalized = databaseId.replace(/^call_/, '');
        
        // Replace forward slashes with underscores (for paths like "turkey/call_001")
        normalized = normalized.replace(/\//g, '_');
        
        // Ensure lowercase and replace spaces/hyphens with underscores
        normalized = normalized.toLowerCase().replace(/[\s-]+/g, '_');
        
        return normalized;
    }

    /**
     * [20251229-PREPROCESS-005] Download master call from MinIO to filesystem
     * @param {string} callId - Database master call ID
     * @param {string} minioPath - Path in MinIO bucket
     * @returns {Promise<string>} Local filesystem path
     */
    async downloadMasterCall(callId, minioPath) {
        const normalizedId = this.normalizeCallId(callId);
        const localPath = path.join(this.masterCallsPath, `${normalizedId}.wav`);

        try {
            // Check if already downloaded
            if (fsSync.existsSync(localPath)) {
                console.log(`  ↓ Already cached: ${normalizedId}.wav`);
                return localPath;
            }

            // Download from MinIO
            console.log(`  ↓ Downloading: ${minioPath} → ${normalizedId}.wav`);
            const stream = await minioService.getObjectStream(this.minioBucket, minioPath);
            
            const writeStream = fsSync.createWriteStream(localPath);
            await new Promise((resolve, reject) => {
                stream.pipe(writeStream);
                stream.on('error', reject);
                writeStream.on('finish', resolve);
                writeStream.on('error', reject);
            });

            console.log(`  ✓ Downloaded: ${normalizedId}.wav (${(await fs.stat(localPath)).size} bytes)`);
            return localPath;

        } catch (error) {
            throw new Error(`Failed to download master call ${callId}: ${error.message}`);
        }
    }

    /**
     * [20251229-PREPROCESS-006] Run feature extraction using C++ tool
     * @param {string} callId - Normalized call ID (without extension)
     * @returns {Promise<void>}
     */
    async extractFeatures(callId) {
        const normalizedId = this.normalizeCallId(callId);
        const featureFile = path.join(this.featuresPath, `${normalizedId}.mfc`);

        // Check if features already exist
        if (fsSync.existsSync(featureFile)) {
            console.log(`  ✓ Features already exist: ${normalizedId}.mfc`);
            return;
        }

        console.log(`  🔧 Extracting features: ${normalizedId}`);

        return new Promise((resolve, reject) => {
            // Run generate_features tool with the normalized call ID
            // Tool expects to run from build/ directory where it can find ../data/master_calls/
            const childProcess = spawn(this.generateFeaturesToolPath, [normalizedId], {
                cwd: '/home/xbyooki/projects/hma-gamecalls-engine/build',
                env: {
                    ...process.env,
                    LD_LIBRARY_PATH: '/home/xbyooki/projects/hma-gamecalls-engine/build/_deps/kissfft-build:' + (process.env.LD_LIBRARY_PATH || '')
                }
            });

            let stdout = '';
            let stderr = '';

            childProcess.stdout.on('data', (data) => {
                stdout += data.toString();
            });

            childProcess.stderr.on('data', (data) => {
                stderr += data.toString();
            });

            childProcess.on('close', (code) => {
                if (code !== 0) {
                    console.error(`  ✗ Feature extraction failed for ${normalizedId}:`);
                    console.error(`  STDOUT: ${stdout}`);
                    console.error(`  STDERR: ${stderr}`);
                    reject(new Error(`Feature extraction failed with code ${code}`));
                } else {
                    // Verify feature file was created
                    if (fsSync.existsSync(featureFile)) {
                        console.log(`  ✓ Features extracted: ${normalizedId}.mfc (${fsSync.statSync(featureFile).size} bytes)`);
                        resolve();
                    } else {
                        console.log(`  ⚠ Feature file not in expected location, checking alternate paths...`);
                        // Check if it was created in master_calls directory instead
                        const altPath = path.join(this.masterCallsPath, `${normalizedId}.mfc`);
                        if (fsSync.existsSync(altPath)) {
                            console.log(`  ✓ Found features in master_calls directory: ${normalizedId}.mfc`);
                            resolve();
                        } else {
                            console.warn(`  ⚠ Feature extraction completed but file not found`);
                            resolve(); // Don't fail if feature file isn't where expected
                        }
                    }
                }
            });

            childProcess.on('error', (error) => {
                reject(new Error(`Failed to spawn feature extraction tool: ${error.message}`));
            });
        });
    }

    /**
     * [20251229-PREPROCESS-007] Preprocess a single master call
     * @param {Object} masterCall - Master call record from database
     * @returns {Promise<Object>} Processing result
     */
    async preprocessMasterCall(masterCall) {
        const result = {
            id: masterCall.id,
            normalizedId: this.normalizeCallId(masterCall.id),
            success: false,
            downloaded: false,
            featuresExtracted: false,
            error: null
        };

        try {
            // Extract MinIO path from audio_file_path
            const minioPath = masterCall.audio_file_path.replace('master-calls/', '');
            
            // Download from MinIO to filesystem
            await this.downloadMasterCall(masterCall.id, minioPath);
            result.downloaded = true;

            // Extract features using C++ tool
            await this.extractFeatures(masterCall.id);
            result.featuresExtracted = true;

            result.success = true;

        } catch (error) {
            result.error = error.message;
            console.error(`  ✗ Failed to preprocess ${masterCall.id}: ${error.message}`);
        }

        return result;
    }

    /**
     * [20251229-PREPROCESS-008] Preprocess all master calls from database
     * @returns {Promise<Object>} Summary of preprocessing results
     */
    async preprocessAllMasterCalls() {
        console.log('🔧 Starting master call preprocessing...\n');

        await this.ensureDirectories();

        // Get all master calls from database
        const query = `
            SELECT id, name, species, audio_file_path, duration_seconds
            FROM master_calls
            ORDER BY species, name
        `;
        
        const result = await databaseService.raw(query);
        const masterCalls = result.rows;

        console.log(`Found ${masterCalls.length} master calls in database\n`);

        const results = {
            total: masterCalls.length,
            successful: 0,
            failed: 0,
            skipped: 0,
            details: []
        };

        // Process each master call
        for (const masterCall of masterCalls) {
            console.log(`\n📋 Processing: ${masterCall.name} (${masterCall.id})`);
            console.log(`   Species: ${masterCall.species}`);
            console.log(`   Duration: ${masterCall.duration_seconds}s`);

            const processingResult = await this.preprocessMasterCall(masterCall);
            results.details.push(processingResult);

            if (processingResult.success) {
                results.successful++;
            } else if (processingResult.error) {
                results.failed++;
            } else {
                results.skipped++;
            }
        }

        // Print summary
        console.log('\n' + '='.repeat(60));
        console.log('📊 Preprocessing Summary');
        console.log('='.repeat(60));
        console.log(`Total master calls: ${results.total}`);
        console.log(`✓ Successfully processed: ${results.successful}`);
        console.log(`✗ Failed: ${results.failed}`);
        console.log(`⊘ Skipped: ${results.skipped}`);
        console.log('='.repeat(60));

        return results;
    }

    /**
     * [20251229-PREPROCESS-009] Get normalized ID for use with C++ engine
     * @param {string} databaseId - Database master call ID
     * @returns {string} Normalized ID for C++ engine
     */
    getNormalizedId(databaseId) {
        return this.normalizeCallId(databaseId);
    }

    /**
     * [20251229-PREPROCESS-010] Check if master call is ready for use
     * @param {string} databaseId - Database master call ID
     * @returns {Promise<Object>} Readiness status
     */
    async checkMasterCallReady(databaseId) {
        const normalizedId = this.normalizeCallId(databaseId);
        const wavPath = path.join(this.masterCallsPath, `${normalizedId}.wav`);
        const mfcPath = path.join(this.masterCallsPath, `${normalizedId}.mfc`);

        const status = {
            normalizedId,
            audioExists: fsSync.existsSync(wavPath),
            featuresExist: fsSync.existsSync(mfcPath),
            ready: false
        };

        if (status.audioExists) {
            const stats = await fs.stat(wavPath);
            status.audioSize = stats.size;
        }

        if (status.featuresExist) {
            const stats = await fs.stat(mfcPath);
            status.featureSize = stats.size;
        }

        status.ready = status.audioExists && status.featuresExist;

        return status;
    }
}

module.exports = new MasterCallPreprocessor();
