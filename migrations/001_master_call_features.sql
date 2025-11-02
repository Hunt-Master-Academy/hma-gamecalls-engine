-- [20251101-FEATURES-001] Master call acoustic features table
-- Stores pre-computed acoustic features for master calls
-- Used for similarity matching, ML training, and analytics

CREATE TABLE IF NOT EXISTS master_call_features (
    id SERIAL PRIMARY KEY,
    master_call_id VARCHAR(255) NOT NULL REFERENCES master_calls(id) ON DELETE CASCADE,
    
    -- Feature version for schema evolution
    feature_version VARCHAR(20) NOT NULL DEFAULT 'v1.0',
    
    -- Temporal features (from MFCC analysis)
    mfcc_mean FLOAT[] NOT NULL,           -- Mean MFCC coefficients (13 dimensions)
    mfcc_std FLOAT[] NOT NULL,            -- Standard deviation of MFCCs
    mfcc_delta_mean FLOAT[],              -- Mean of MFCC deltas (temporal change)
    mfcc_delta_std FLOAT[],               -- Std of MFCC deltas
    
    -- Spectral features
    spectral_centroid_mean FLOAT,        -- Average frequency center of mass
    spectral_centroid_std FLOAT,
    spectral_bandwidth_mean FLOAT,       -- Frequency spread
    spectral_bandwidth_std FLOAT,
    spectral_rolloff_mean FLOAT,         -- Frequency below which 85% of energy lies
    spectral_rolloff_std FLOAT,
    zero_crossing_rate_mean FLOAT,       -- Audio texture indicator
    zero_crossing_rate_std FLOAT,
    
    -- Energy features
    rms_energy_mean FLOAT NOT NULL,      -- Overall loudness
    rms_energy_std FLOAT NOT NULL,
    rms_energy_max FLOAT NOT NULL,
    rms_energy_min FLOAT NOT NULL,
    
    -- Pitch/fundamental frequency features
    f0_mean FLOAT,                        -- Mean fundamental frequency (Hz)
    f0_std FLOAT,
    f0_max FLOAT,
    f0_min FLOAT,
    f0_range FLOAT,                       -- Max - Min pitch range
    
    -- Harmonic features
    harmonicity_mean FLOAT,              -- Harmonic-to-noise ratio
    harmonicity_std FLOAT,
    
    -- Temporal features
    duration_seconds FLOAT NOT NULL,
    num_frames INTEGER NOT NULL,
    tempo_bpm FLOAT,                      -- Rhythm/cadence
    
    -- Silence/activity features
    silence_ratio FLOAT,                  -- Percentage of silent frames
    voice_activity_ratio FLOAT,           -- Percentage of active audio
    
    -- Statistical features for discrimination
    entropy FLOAT,                        -- Signal randomness
    kurtosis FLOAT,                       -- Distribution peakedness
    skewness FLOAT,                       -- Distribution asymmetry
    
    -- Metadata
    extraction_method VARCHAR(100) DEFAULT 'UnifiedAudioEngine',
    extracted_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    computation_time_ms INTEGER,
    
    -- Indexes for fast similarity search
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    CONSTRAINT unique_call_version UNIQUE(master_call_id, feature_version)
);

-- Indexes for similarity queries
CREATE INDEX idx_master_call_features_call_id ON master_call_features(master_call_id);
CREATE INDEX idx_master_call_features_version ON master_call_features(feature_version);
CREATE INDEX idx_master_call_features_extracted_at ON master_call_features(extracted_at DESC);

-- GIN index for MFCC similarity search (requires pg_similarity extension or manual implementation)
-- CREATE INDEX idx_mfcc_mean_similarity ON master_call_features USING GIN(mfcc_mean);

-- Trigger to update updated_at
CREATE OR REPLACE FUNCTION update_master_call_features_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_update_master_call_features_updated_at
    BEFORE UPDATE ON master_call_features
    FOR EACH ROW
    EXECUTE FUNCTION update_master_call_features_updated_at();

-- Comments for documentation
COMMENT ON TABLE master_call_features IS 'Pre-computed acoustic features for master calls used in similarity matching and ML';
COMMENT ON COLUMN master_call_features.mfcc_mean IS 'Mean of Mel-frequency cepstral coefficients (13-dimensional vector)';
COMMENT ON COLUMN master_call_features.feature_version IS 'Version tag for feature schema evolution (v1.0, v1.1, etc)';
COMMENT ON COLUMN master_call_features.extraction_method IS 'Tool used to extract features (UnifiedAudioEngine, librosa, etc)';
