/**
 * [20251101-FEATURES-002] Feature Extraction Service
 * Extracts and stores acoustic features from master calls
 * Integrates with C++ engine for computation
 */

const databaseService = require('./databaseService');
const minioService = require('./minioService');
const gameCallsEngine = require('../../bindings/node-api/build/Release/gamecalls_engine.node');
const fs = require('fs').promises;
const path = require('path');
const os = require('os');

class FeatureExtractionService {
    
    /**
     * Extract features from a master call audio file
     * @param {string} masterCallId - ID of the master call
     * @returns {Promise<Object>} Extracted features
     */
    static async extractFeatures(masterCallId) {
        const startTime = Date.now();
        
        try {
            // Get master call metadata from database
            const callQuery = `
                SELECT id, name, audio_file_path, duration_seconds, sample_rate
                FROM master_calls
                WHERE id = $1 AND deleted_at IS NULL
            `;
            const callResult = await databaseService.raw(callQuery, [masterCallId]);
            
            if (callResult.rows.length === 0) {
                throw new Error(`Master call ${masterCallId} not found`);
            }
            
            const call = callResult.rows[0];
            
            // Download audio file from MinIO to temp location
            const audioKey = call.audio_file_path.replace('master-calls/', '');
            const tempFile = path.join(os.tmpdir(), `feature-extract-${masterCallId}-${Date.now()}.wav`);
            
            const stream = await minioService.getObjectStream('gamecalls-master-calls', audioKey);
            const writeStream = require('fs').createWriteStream(tempFile);
            
            await new Promise((resolve, reject) => {
                stream.pipe(writeStream);
                stream.on('error', reject);
                writeStream.on('finish', resolve);
                writeStream.on('error', reject);
            });
            
            console.log(`[FeatureExtraction] Processing ${call.name} (${masterCallId})`);
            
            // Create temporary analysis session in engine
            const sessionId = await gameCallsEngine.createSession(tempFile, {
                sampleRate: call.sample_rate || 44100,
                enableEnhancedAnalysis: true // Enable all analyzers
            });
            
            // Load audio through engine to trigger feature extraction
            const audioData = await this._loadAudioFile(tempFile);
            const processResult = await gameCallsEngine.processAudio(sessionId, audioData);
            
            // [20251101-FEATURES-004] Get enhanced analysis with acoustic features from V1.0 engine
            const analysis = await gameCallsEngine.getEnhancedAnalysis(sessionId);
            
            // [20251101-FEATURES-008] Get engine info for reproducibility tracking
            const engineInfo = await gameCallsEngine.getEngineInfo();
            
            // Extract features from analysis
            const features = this._buildFeatureVector(analysis, call);
            
            // [20251101-FEATURES-009] Store enhanced raw analysis and engine metadata
            features.enhancedRaw = analysis;  // Full JSON snapshot
            features.engineBuildSha = engineInfo.fixVersion || 'unknown';
            features.engineVersion = engineInfo.version || '1.0.0';
            features.extractionParams = {
                sampleRate: call.sample_rate || 44100,
                enableEnhancedAnalysis: true,
                vadEnabled: false,
                extractedAt: new Date().toISOString()
            };
            features.overallQualityScore = analysis.quality?.overallScore || null;
            features.confidenceScore = analysis.quality?.confidence || null;
            
            // Store features in database
            await this._storeFeatures(masterCallId, features, Date.now() - startTime);
            
            // Cleanup
            await gameCallsEngine.destroySession(sessionId);
            await fs.unlink(tempFile);
            
            console.log(`[FeatureExtraction] Completed ${call.name} in ${Date.now() - startTime}ms`);
            
            return features;
            
        } catch (error) {
            console.error(`[FeatureExtraction] Error extracting features for ${masterCallId}:`, error);
            throw error;
        }
    }
    
    /**
     * Build feature vector from engine enhanced analysis (V1.0 with real acoustic features)
     * @private
     */
    static _buildFeatureVector(analysis, callMetadata) {
        // [20251101-FEATURES-005] Extract real acoustic features from V1.0 enhanced analysis
        
        // MFCC features - V1.0 provides MFCC similarity score but not vectors yet
        // TODO: Export actual MFCC coefficient vectors in V1.1
        const mfccMean = new Array(13).fill(0);  // Placeholder for now
        const mfccStd = new Array(13).fill(0);
        const mfccDeltaMean = null;
        const mfccDeltaStd = null;
        
        // Spectral features - not yet exported from V1.0
        const spectral = {
            centroidMean: null,
            centroidStd: null,
            bandwidthMean: null,
            bandwidthStd: null,
            rolloffMean: null,
            rolloffStd: null,
            zcrMean: null,
            zcrStd: null
        };
        
        // Energy features - extract from V1.0 analysis
        const energy = {
            rmsMean: analysis.energy?.normalizationScalar || 0,
            rmsStd: 0,  // Not provided
            rmsMax: 0,  // Not provided
            rmsMin: 0   // Not provided
        };
        
        // Pitch/F0 features - extract from V1.0 enhanced analysis
        const pitch = {
            f0Mean: analysis.pitch?.f0Mean || null,
            f0Std: null,  // Not provided in V1.0
            f0Max: null,  // Not provided
            f0Min: null,  // Not provided
            f0Range: null // Not provided
        };
        
        // Harmonic features - extract from V1.0 enhanced analysis
        const harmonic = {
            harmonicityMean: analysis.harmonic?.fundamentalFreq || null,
            harmonicityStd: null  // Not provided
        };
        
        // Temporal features - extract from V1.0 enhanced analysis
        const segmentDurationSec = (analysis.temporal?.segmentDurationMs || 0) / 1000.0;
        const temporal = {
            durationSeconds: segmentDurationSec || callMetadata.duration_seconds,
            numFrames: Math.floor((analysis.samplesAnalyzed || 0) / 512),  // Estimate frames
            tempoBpm: analysis.temporal?.tempoBpm || null,
            silenceRatio: null,  // Not provided in V1.0
            voiceActivityRatio: null  // Not provided
        };
        
        // Statistical features - placeholders (need per-frame stats for these)
        const stats = {
            entropy: analysis.stats?.entropy || null,
            kurtosis: analysis.stats?.kurtosis || null,
            skewness: analysis.stats?.skewness || null
        };
        
        return {
            featureVersion: 'v1.0',
            mfccMean,
            mfccStd,
            mfccDeltaMean,
            mfccDeltaStd,
            ...spectral,
            ...energy,
            ...pitch,
            ...harmonic,
            ...temporal,
            ...stats,
            extractionMethod: 'UnifiedAudioEngine'
        };
    }
    
    /**
     * Store extracted features in database
     * @private
     */
    static async _storeFeatures(masterCallId, features, computationTimeMs) {
        const query = `
            INSERT INTO master_call_features (
                master_call_id, feature_version,
                mfcc_mean, mfcc_std, mfcc_delta_mean, mfcc_delta_std,
                spectral_centroid_mean, spectral_centroid_std,
                spectral_bandwidth_mean, spectral_bandwidth_std,
                spectral_rolloff_mean, spectral_rolloff_std,
                zero_crossing_rate_mean, zero_crossing_rate_std,
                rms_energy_mean, rms_energy_std, rms_energy_max, rms_energy_min,
                f0_mean, f0_std, f0_max, f0_min, f0_range,
                harmonicity_mean, harmonicity_std,
                duration_seconds, num_frames, tempo_bpm,
                silence_ratio, voice_activity_ratio,
                entropy, kurtosis, skewness,
                extraction_method, computation_time_ms,
                enhanced_raw, engine_build_sha, engine_version,
                extraction_params, overall_quality_score, confidence_score,
                extraction_status
            ) VALUES (
                $1, $2,
                $3, $4, $5, $6,
                $7, $8, $9, $10, $11, $12, $13, $14,
                $15, $16, $17, $18,
                $19, $20, $21, $22, $23,
                $24, $25,
                $26, $27, $28,
                $29, $30,
                $31, $32, $33,
                $34, $35,
                $36, $37, $38,
                $39, $40, $41,
                $42
            )
            ON CONFLICT (master_call_id, feature_version)
            DO UPDATE SET
                mfcc_mean = EXCLUDED.mfcc_mean,
                mfcc_std = EXCLUDED.mfcc_std,
                mfcc_delta_mean = EXCLUDED.mfcc_delta_mean,
                mfcc_delta_std = EXCLUDED.mfcc_delta_std,
                spectral_centroid_mean = EXCLUDED.spectral_centroid_mean,
                spectral_centroid_std = EXCLUDED.spectral_centroid_std,
                spectral_bandwidth_mean = EXCLUDED.spectral_bandwidth_mean,
                spectral_bandwidth_std = EXCLUDED.spectral_bandwidth_std,
                spectral_rolloff_mean = EXCLUDED.spectral_rolloff_mean,
                spectral_rolloff_std = EXCLUDED.spectral_rolloff_std,
                zero_crossing_rate_mean = EXCLUDED.zero_crossing_rate_mean,
                zero_crossing_rate_std = EXCLUDED.zero_crossing_rate_std,
                rms_energy_mean = EXCLUDED.rms_energy_mean,
                rms_energy_std = EXCLUDED.rms_energy_std,
                rms_energy_max = EXCLUDED.rms_energy_max,
                rms_energy_min = EXCLUDED.rms_energy_min,
                f0_mean = EXCLUDED.f0_mean,
                f0_std = EXCLUDED.f0_std,
                f0_max = EXCLUDED.f0_max,
                f0_min = EXCLUDED.f0_min,
                f0_range = EXCLUDED.f0_range,
                harmonicity_mean = EXCLUDED.harmonicity_mean,
                harmonicity_std = EXCLUDED.harmonicity_std,
                duration_seconds = EXCLUDED.duration_seconds,
                num_frames = EXCLUDED.num_frames,
                tempo_bpm = EXCLUDED.tempo_bpm,
                silence_ratio = EXCLUDED.silence_ratio,
                voice_activity_ratio = EXCLUDED.voice_activity_ratio,
                entropy = EXCLUDED.entropy,
                kurtosis = EXCLUDED.kurtosis,
                skewness = EXCLUDED.skewness,
                extraction_method = EXCLUDED.extraction_method,
                computation_time_ms = EXCLUDED.computation_time_ms,
                enhanced_raw = EXCLUDED.enhanced_raw,
                engine_build_sha = EXCLUDED.engine_build_sha,
                engine_version = EXCLUDED.engine_version,
                extraction_params = EXCLUDED.extraction_params,
                overall_quality_score = EXCLUDED.overall_quality_score,
                confidence_score = EXCLUDED.confidence_score,
                extraction_status = EXCLUDED.extraction_status,
                extracted_at = NOW()
            RETURNING id
        `;
        
        const params = [
            masterCallId, features.featureVersion,
            features.mfccMean, features.mfccStd, features.mfccDeltaMean, features.mfccDeltaStd,
            features.centroidMean, features.centroidStd,
            features.bandwidthMean, features.bandwidthStd,
            features.rolloffMean, features.rolloffStd,
            features.zcrMean, features.zcrStd,
            features.rmsMean, features.rmsStd, features.rmsMax, features.rmsMin,
            features.f0Mean, features.f0Std, features.f0Max, features.f0Min, features.f0Range,
            features.harmonicityMean, features.harmonicityStd,
            features.durationSeconds, features.numFrames, features.tempoBpm,
            features.silenceRatio, features.voiceActivityRatio,
            features.entropy, features.kurtosis, features.skewness,
            features.extractionMethod, computationTimeMs,
            // [20251101-FEATURES-010] New V1.0 schema enhancements
            JSON.stringify(features.enhancedRaw || null),
            features.engineBuildSha || null,
            features.engineVersion || '1.0.0',
            JSON.stringify(features.extractionParams || null),
            features.overallQualityScore || null,
            features.confidenceScore || null,
            'completed'
        ];
        
        const result = await databaseService.raw(query, params);
        return result.rows[0].id;
    }
    
    /**
     * Load audio file as Float32Array
     * @private
     */
    static async _loadAudioFile(filePath) {
        const fileHandle = await fs.open(filePath, 'r');
        const fileStats = await fileHandle.stat();
        const fileSize = fileStats.size - 44; // Skip WAV header
        
        const buffer = Buffer.alloc(fileSize);
        await fileHandle.read(buffer, 0, fileSize, 44);
        await fileHandle.close();
        
        // Convert Int16 PCM to Float32
        const float32Data = new Float32Array(fileSize / 2);
        for (let i = 0; i < float32Data.length; i++) {
            float32Data[i] = buffer.readInt16LE(i * 2) / 32768.0;
        }
        
        return float32Data;
    }
    
    /**
     * Get features for a master call
     * @param {string} masterCallId - ID of the master call
     * @param {string} version - Feature version (default: v1.0)
     * @returns {Promise<Object|null>} Features or null if not extracted
     */
    static async getFeatures(masterCallId, version = 'v1.0') {
        const query = `
            SELECT * FROM master_call_features
            WHERE master_call_id = $1 AND feature_version = $2
        `;
        
        const result = await databaseService.raw(query, [masterCallId, version]);
        return result.rows[0] || null;
    }
    
    /**
     * Batch extract features for multiple master calls
     * @param {string[]} masterCallIds - Array of master call IDs
     * @returns {Promise<Object>} Results summary
     */
    static async batchExtract(masterCallIds) {
        const results = {
            total: masterCallIds.length,
            successful: 0,
            failed: 0,
            errors: []
        };
        
        for (const callId of masterCallIds) {
            try {
                await this.extractFeatures(callId);
                results.successful++;
            } catch (error) {
                results.failed++;
                results.errors.push({ callId, error: error.message });
            }
        }
        
        return results;
    }
    
    /**
     * Extract features for all master calls without features
     * @returns {Promise<Object>} Results summary
     */
    static async extractMissingFeatures() {
        // Find calls without features
        const query = `
            SELECT mc.id
            FROM master_calls mc
            LEFT JOIN master_call_features mcf ON mc.id = mcf.master_call_id
            WHERE mc.deleted_at IS NULL AND mcf.id IS NULL
        `;
        
        const result = await databaseService.raw(query);
        const missingIds = result.rows.map(row => row.id);
        
        console.log(`[FeatureExtraction] Found ${missingIds.length} calls without features`);
        
        return this.batchExtract(missingIds);
    }
}

module.exports = FeatureExtractionService;
