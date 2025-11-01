#!/usr/bin/env node
/**
 * [20251229-TEST-003] Generate minimal valid WAV file for testing
 */

const fs = require('fs');

function createWAV(filename, frequency = 440, durationSeconds = 2, sampleRate = 44100) {
    const numSamples = sampleRate * durationSeconds;
    const numChannels = 1;
    const bitsPerSample = 16;
    const byteRate = sampleRate * numChannels * bitsPerSample / 8;
    const blockAlign = numChannels * bitsPerSample / 8;
    const dataSize = numSamples * numChannels * bitsPerSample / 8;
    const fileSize = 44 + dataSize;

    const buffer = Buffer.alloc(fileSize);
    let offset = 0;

    // RIFF header
    buffer.write('RIFF', offset); offset += 4;
    buffer.writeUInt32LE(fileSize - 8, offset); offset += 4;
    buffer.write('WAVE', offset); offset += 4;

    // fmt chunk
    buffer.write('fmt ', offset); offset += 4;
    buffer.writeUInt32LE(16, offset); offset += 4; // Subchunk1Size (16 for PCM)
    buffer.writeUInt16LE(1, offset); offset += 2;  // AudioFormat (1 for PCM)
    buffer.writeUInt16LE(numChannels, offset); offset += 2;
    buffer.writeUInt32LE(sampleRate, offset); offset += 4;
    buffer.writeUInt32LE(byteRate, offset); offset += 4;
    buffer.writeUInt16LE(blockAlign, offset); offset += 2;
    buffer.writeUInt16LE(bitsPerSample, offset); offset += 2;

    // data chunk
    buffer.write('data', offset); offset += 4;
    buffer.writeUInt32LE(dataSize, offset); offset += 4;

    // Generate sine wave audio data
    for (let i = 0; i < numSamples; i++) {
        const t = i / sampleRate;
        const sample = Math.sin(2 * Math.PI * frequency * t) * 0.3; // 30% amplitude
        const value = Math.round(sample * 32767); // Convert to 16-bit PCM
        buffer.writeInt16LE(value, offset);
        offset += 2;
    }

    fs.writeFileSync(filename, buffer);
    console.log(`✓ Created WAV file: ${filename}`);
    console.log(`  Duration: ${durationSeconds}s`);
    console.log(`  Sample rate: ${sampleRate} Hz`);
    console.log(`  Frequency: ${frequency} Hz`);
    console.log(`  File size: ${fileSize} bytes`);
}

// Generate test file
const outputFile = process.argv[2] || '/tmp/test-turkey-purr.wav';
createWAV(outputFile, 440, 2, 44100);
