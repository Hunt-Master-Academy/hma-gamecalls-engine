/**
 * Waveform Service
 * [20251029-WAVEFORM-001] Generates waveform visualization data from audio files
 * Uses decimation algorithm for efficient client-side rendering
 */

const { ApiError } = require('../middleware/errorHandler');
const minioService = require('./minioService');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');
const { promisify } = require('util');

const execAsync = promisify(exec);

class WaveformService {

    /**
     * [20251029-WAVEFORM-002] Generate waveform data from audio file
     * @param {string} audioPath - Path to audio file in MinIO
     * @param {string} bucketName - MinIO bucket name
     * @param {number} samples - Number of waveform samples (default 1000)
     * @returns {Promise<object>} Waveform data with peaks
     */
    static async generateWaveform(audioPath, bucketName, samples = 1000) {
        try {
            // [20251029-WAVEFORM-003] Download audio file to temp location
            const tempDir = '/tmp/gamecalls-waveforms';
            if (!fs.existsSync(tempDir)) {
                fs.mkdirSync(tempDir, { recursive: true });
            }

            const tempFile = path.join(tempDir, `${Date.now()}_${path.basename(audioPath)}`);
            const stream = await minioService.getObjectStream(bucketName, audioPath);

            // Write stream to temp file
            await new Promise((resolve, reject) => {
                const writeStream = fs.createWriteStream(tempFile);
                stream.pipe(writeStream);
                stream.on('error', reject);
                writeStream.on('finish', resolve);
                writeStream.on('error', reject);
            });

            // [20251029-WAVEFORM-004] Extract audio samples using ffmpeg
            const waveformData = await this._extractSamples(tempFile, samples);

            // [20251029-WAVEFORM-005] Clean up temp file
            fs.unlinkSync(tempFile);

            return waveformData;

        } catch (error) {
            throw ApiError.internal('WAVEFORM_GENERATION_FAILED', `Failed to generate waveform: ${error.message}`);
        }
    }

    /**
     * [20251029-WAVEFORM-006] Extract audio samples using ffmpeg
     * Generates min/max peak values for efficient visualization
     */
    static async _extractSamples(audioFile, targetSamples) {
        try {
            // [20251029-WAVEFORM-007] Get audio file info
            const probeCmd = `ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "${audioFile}"`;
            const { stdout: durationStr } = await execAsync(probeCmd);
            const duration = parseFloat(durationStr.trim());

            // [20251029-WAVEFORM-008] Extract raw PCM samples
            const rawFile = `${audioFile}.raw`;
            const extractCmd = `ffmpeg -i "${audioFile}" -f s16le -ac 1 -ar 8000 "${rawFile}" -y`;
            await execAsync(extractCmd);

            // [20251029-WAVEFORM-009] Read raw PCM data
            const pcmData = fs.readFileSync(rawFile);
            const samples = [];
            
            // Convert buffer to 16-bit signed integers
            for (let i = 0; i < pcmData.length; i += 2) {
                const sample = pcmData.readInt16LE(i) / 32768.0; // Normalize to -1.0 to 1.0
                samples.push(sample);
            }

            // [20251029-WAVEFORM-010] Implement decimation algorithm
            const decimatedData = this._decimateSamples(samples, targetSamples);

            // Clean up raw file
            fs.unlinkSync(rawFile);

            return {
                data: decimatedData,
                duration,
                sampleRate: 8000, // Downsampled rate
                samples: decimatedData.length,
                channels: 1
            };

        } catch (error) {
            throw new Error(`Sample extraction failed: ${error.message}`);
        }
    }

    /**
     * [20251029-WAVEFORM-011] Decimate samples for efficient visualization
     * Groups samples into bins and calculates min/max peaks
     */
    static _decimateSamples(samples, targetSamples) {
        if (samples.length <= targetSamples * 2) {
            // Not enough samples to decimate, return as-is
            return samples.map(s => ({ min: s, max: s }));
        }

        const decimated = [];
        const samplesPerBin = Math.floor(samples.length / targetSamples);

        for (let i = 0; i < targetSamples; i++) {
            const start = i * samplesPerBin;
            const end = Math.min(start + samplesPerBin, samples.length);
            const bin = samples.slice(start, end);

            const min = Math.min(...bin);
            const max = Math.max(...bin);

            decimated.push({ min, max });
        }

        return decimated;
    }

    /**
     * [20251029-WAVEFORM-012] Store waveform data in MinIO
     * @param {string} callId - Master call ID
     * @param {object} waveformData - Generated waveform data
     * @returns {Promise<string>} MinIO object path
     */
    static async storeWaveform(callId, waveformData) {
        try {
            const objectName = `waveforms/${callId}.json`;
            const jsonBuffer = Buffer.from(JSON.stringify(waveformData, null, 2));

            await minioService.uploadFile(
                'gamecalls-master-calls',
                objectName,
                jsonBuffer,
                { 'Content-Type': 'application/json' }
            );

            return objectName;

        } catch (error) {
            throw ApiError.internal('WAVEFORM_STORAGE_FAILED', `Failed to store waveform: ${error.message}`);
        }
    }

    /**
     * [20251029-WAVEFORM-013] Load waveform data from MinIO
     * @param {string} waveformPath - MinIO object path
     * @returns {Promise<object>} Waveform data
     */
    static async loadWaveform(waveformPath) {
        try {
            const stream = await minioService.getObjectStream('gamecalls-master-calls', waveformPath);
            
            // Read stream to buffer
            const chunks = [];
            await new Promise((resolve, reject) => {
                stream.on('data', chunk => chunks.push(chunk));
                stream.on('end', resolve);
                stream.on('error', reject);
            });

            const buffer = Buffer.concat(chunks);
            return JSON.parse(buffer.toString());

        } catch (error) {
            throw ApiError.internal('WAVEFORM_LOAD_FAILED', `Failed to load waveform: ${error.message}`);
        }
    }

    /**
     * [20251029-WAVEFORM-014] Check if ffmpeg is available
     */
    static async checkFFmpegAvailability() {
        try {
            await execAsync('ffmpeg -version');
            await execAsync('ffprobe -version');
            return true;
        } catch (error) {
            console.warn('⚠️  FFmpeg not available, waveform generation disabled');
            return false;
        }
    }
}

module.exports = WaveformService;
