// Master Call Configuration for Huntmaster Engine
// This file defines the available master calls and their file paths

export const MASTER_CALLS = {
    "buck_grunt": {
        name: "Buck Grunt",
        description: "Deep, guttural grunt made by mature bucks",
        audioFile: "../data/master_calls/buck_grunt.wav",
        mfccFile: "../data/master_calls/buck_grunt.mfc",
        category: "buck",
        difficulty: "beginner"
    },
    "doe_grunt": {
        name: "Doe Grunt",
        description: "Soft grunt made by does communicating with fawns",
        audioFile: "../data/master_calls/doe_grunt.wav",
        mfccFile: "../data/master_calls/doe_grunt.mfc",
        category: "doe",
        difficulty: "beginner"
    },
    "fawn_bleat": {
        name: "Fawn Bleat",
        description: "High-pitched distress call from young deer",
        audioFile: "../data/master_calls/fawn_bleat.wav",
        mfccFile: "../data/master_calls/fawn_bleat.mfc",
        category: "fawn",
        difficulty: "intermediate"
    },
    "buck_bawl": {
        name: "Buck Bawl",
        description: "Aggressive territorial call from dominant bucks",
        audioFile: "../data/master_calls/buck_bawl.wav",
        mfccFile: "../data/master_calls/buck_bawl.mfc",
        category: "buck",
        difficulty: "advanced"
    },
    "doe_bleat": {
        name: "Doe Bleat",
        description: "Estrus bleat to attract breeding bucks",
        audioFile: "../data/master_calls/doe_bleat.wav",
        mfccFile: "../data/master_calls/doe_bleat.mfc",
        category: "doe",
        difficulty: "intermediate"
    },
    "estrus_bleat": {
        name: "Estrus Bleat",
        description: "Breeding season call from receptive does",
        audioFile: "../data/master_calls/estrus_bleat.wav",
        mfccFile: "../data/master_calls/estrus_bleat.mfc",
        category: "doe",
        difficulty: "advanced"
    },
    "contact_bleat": {
        name: "Contact Bleat",
        description: "Social communication between deer",
        audioFile: "../data/master_calls/contact_bleat.wav",
        mfccFile: "../data/master_calls/contact_bleat.mfc",
        category: "social",
        difficulty: "beginner"
    },
    "tending_grunts": {
        name: "Tending Grunts",
        description: "Buck following a doe during rut",
        audioFile: "../data/master_calls/tending_grunts.wav",
        mfccFile: "../data/master_calls/tending_grunts.mfc",
        category: "buck",
        difficulty: "advanced"
    }
};

// Helper functions for master call management
export class MasterCallManager {
    constructor(audioProcessor) {
        this.audioProcessor = audioProcessor;
        this.loadedCalls = new Map();
        this.currentCall = null;
    }

    /**
     * Get list of available master calls
     * @param {string} category - Filter by category (optional)
     * @param {string} difficulty - Filter by difficulty (optional)
     * @returns {Array} List of master call objects
     */
    getAvailableCalls(category = null, difficulty = null) {
        let calls = Object.entries(MASTER_CALLS).map(([id, call]) => ({
            id,
            ...call
        }));

        if (category) {
            calls = calls.filter(call => call.category === category);
        }

        if (difficulty) {
            calls = calls.filter(call => call.difficulty === difficulty);
        }

        return calls;
    }

    /**
     * Load a master call audio file
     * @param {string} callId - Master call identifier
     * @returns {Promise<AudioBuffer>} Audio buffer
     */
    async loadMasterCallAudio(callId) {
        if (!MASTER_CALLS[callId]) {
            throw new Error(`Master call not found: ${callId}`);
        }

        // Check if already loaded
        if (this.loadedCalls.has(callId)) {
            return this.loadedCalls.get(callId);
        }

        const callConfig = MASTER_CALLS[callId];

        try {
            // Load audio file
            const response = await fetch(callConfig.audioFile);
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }

            const arrayBuffer = await response.arrayBuffer();
            const audioContext = this.audioProcessor.audioContext;
            const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

            // Cache the loaded audio
            this.loadedCalls.set(callId, {
                audioBuffer,
                config: callConfig,
                loadedAt: new Date()
            });

            return this.loadedCalls.get(callId);

        } catch (error) {
            throw new Error(`Failed to load master call ${callId}: ${error.message}`);
        }
    }

    /**
     * Load master call MFCC features (if available)
     * @param {string} callId - Master call identifier
     * @returns {Promise<Object>} MFCC feature data
     */
    async loadMasterCallFeatures(callId) {
        if (!MASTER_CALLS[callId]) {
            throw new Error(`Master call not found: ${callId}`);
        }

        const callConfig = MASTER_CALLS[callId];

        try {
            // Try to load pre-computed MFCC features
            const response = await fetch(callConfig.mfccFile);
            if (!response.ok) {
                // If MFCC file doesn't exist, we'll compute features from audio
                console.warn(`MFCC file not found for ${callId}, will compute from audio`);
                return null;
            }

            const mfccData = await response.json();
            return mfccData;

        } catch (error) {
            console.warn(`Failed to load MFCC features for ${callId}: ${error.message}`);
            return null;
        }
    }

    /**
     * Set current master call for comparison
     * @param {string} callId - Master call identifier
     * @returns {Promise<Object>} Loaded master call data
     */
    async setCurrentMasterCall(callId) {
        const masterCallData = await this.loadMasterCallAudio(callId);
        const mfccFeatures = await this.loadMasterCallFeatures(callId);

        this.currentCall = {
            id: callId,
            ...masterCallData,
            mfccFeatures
        };

        // Load into audio processor engine if MFCC features available
        if (mfccFeatures && this.audioProcessor.engineReady) {
            try {
                const success = this.audioProcessor.loadMasterCall(mfccFeatures);
                if (success) {
                    console.log(`Master call ${callId} loaded into engine successfully`);
                } else {
                    console.warn(`Failed to load master call ${callId} into engine`);
                }
            } catch (error) {
                console.error(`Error loading master call into engine: ${error.message}`);
            }
        }

        return this.currentCall;
    }

    /**
     * Play current master call
     * @returns {Promise<void>}
     */
    async playCurrentMasterCall() {
        if (!this.currentCall) {
            throw new Error('No master call loaded');
        }

        if (!this.audioProcessor.audioContext) {
            throw new Error('Audio context not available');
        }

        const source = this.audioProcessor.audioContext.createBufferSource();
        source.buffer = this.currentCall.audioBuffer;
        source.connect(this.audioProcessor.audioContext.destination);
        source.start();

        console.log(`Playing master call: ${this.currentCall.config.name}`);
    }

    /**
     * Get current master call info
     * @returns {Object|null} Current master call data
     */
    getCurrentMasterCall() {
        return this.currentCall;
    }

    /**
     * Clear loaded master calls from cache
     */
    clearCache() {
        this.loadedCalls.clear();
        this.currentCall = null;
    }

    /**
     * Get cache statistics
     * @returns {Object} Cache information
     */
    getCacheStats() {
        return {
            loadedCalls: this.loadedCalls.size,
            currentCall: this.currentCall ? this.currentCall.id : null,
            memoryUsage: Array.from(this.loadedCalls.values()).reduce((total, call) => {
                return total + (call.audioBuffer.length * call.audioBuffer.numberOfChannels * 4); // 4 bytes per float
            }, 0)
        };
    }
}

// Export default configuration
export default {
    MASTER_CALLS,
    MasterCallManager
};
