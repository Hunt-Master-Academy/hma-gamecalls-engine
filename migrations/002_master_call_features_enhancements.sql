-- [20251101-FEATURES-006] Enhanced feature table schema improvements
-- Adds: JSONB raw analysis, engine versioning, extraction params, performance indexes

-- Add JSONB column for full enhanced analysis preservation
ALTER TABLE master_call_features
ADD COLUMN enhanced_raw JSONB DEFAULT NULL;

-- Add engine build tracking for reproducibility
ALTER TABLE master_call_features
ADD COLUMN engine_build_sha TEXT DEFAULT NULL,
ADD COLUMN engine_version TEXT DEFAULT '1.0.0';

-- Add extraction parameters for audit trail
ALTER TABLE master_call_features
ADD COLUMN extraction_params JSONB DEFAULT NULL;

-- Add status column for error handling and QA
ALTER TABLE master_call_features
ADD COLUMN extraction_status VARCHAR(50) DEFAULT 'completed'
    CHECK (extraction_status IN ('completed', 'partial', 'failed', 'needs_review'));

-- Add confidence/quality metrics
ALTER TABLE master_call_features
ADD COLUMN overall_quality_score FLOAT,
ADD COLUMN confidence_score FLOAT;

-- Performance indexes for similarity search queries
-- 1. Species + feature version (most common filter)
CREATE INDEX idx_features_species_version ON master_call_features(master_call_id)
    INCLUDE (feature_version, extraction_status);

-- 2. Temporal filters (duration, tempo)
CREATE INDEX idx_features_temporal ON master_call_features(duration_seconds, tempo_bpm)
    WHERE tempo_bpm IS NOT NULL AND extraction_status = 'completed';

-- 3. Energy/RMS band queries
CREATE INDEX idx_features_energy ON master_call_features(rms_energy_mean)
    WHERE rms_energy_mean IS NOT NULL AND extraction_status = 'completed';

-- 4. Pitch range queries
CREATE INDEX idx_features_pitch ON master_call_features(f0_mean)
    WHERE f0_mean IS NOT NULL AND extraction_status = 'completed';

-- 5. GIN index on JSONB for flexible enhanced_raw queries
CREATE INDEX idx_features_enhanced_raw ON master_call_features USING GIN(enhanced_raw)
    WHERE enhanced_raw IS NOT NULL;

-- 6. Extraction timestamp for cache invalidation
CREATE INDEX idx_features_extracted_at ON master_call_features(extracted_at DESC);

-- Add comments for new columns
COMMENT ON COLUMN master_call_features.enhanced_raw IS 
    'Complete JSON snapshot of C++ engine enhanced analysis output for future feature extraction without schema changes';
COMMENT ON COLUMN master_call_features.engine_build_sha IS 
    'Git SHA or build identifier of C++ engine binary used for extraction (reproducibility)';
COMMENT ON COLUMN master_call_features.engine_version IS 
    'Semantic version of engine API (e.g., 1.0.0, 1.1.0) for feature compatibility tracking';
COMMENT ON COLUMN master_call_features.extraction_params IS 
    'JSON of extraction configuration: {sampleRate, enableEnhancedAnalysis, vadEnabled, etc.}';
COMMENT ON COLUMN master_call_features.extraction_status IS 
    'Extraction outcome: completed (success), partial (some features missing), failed (error), needs_review (QA flag)';
COMMENT ON COLUMN master_call_features.overall_quality_score IS 
    'Composite quality metric from engine (0-1): blend of similarity, confidence, component scores';
COMMENT ON COLUMN master_call_features.confidence_score IS 
    'Engine confidence in feature extraction quality (0-1)';

-- Create view for fast similarity search candidates
CREATE OR REPLACE VIEW v_feature_search_candidates AS
SELECT 
    mcf.id,
    mcf.master_call_id,
    mc.name,
    mc.species,
    mc.call_type,
    mcf.duration_seconds,
    mcf.tempo_bpm,
    mcf.rms_energy_mean,
    mcf.f0_mean,
    mcf.harmonicity_mean,
    mcf.overall_quality_score,
    mcf.confidence_score,
    mcf.feature_version,
    -- Normalized feature vector for distance calculations
    ARRAY[
        COALESCE(mcf.tempo_bpm / 200.0, 0),  -- Normalize to [0,1] assuming max 200 BPM
        COALESCE(mcf.rms_energy_mean, 0),     -- Already 0-1 range
        COALESCE(mcf.f0_mean / 1000.0, 0),    -- Normalize to [0,1] assuming max 1000 Hz
        COALESCE(mcf.harmonicity_mean / 500.0, 0)  -- Normalize harmonic freq
    ] AS feature_vector
FROM master_call_features mcf
JOIN master_calls mc ON mcf.master_call_id = mc.id
WHERE mcf.extraction_status = 'completed'
  AND mcf.feature_version = 'v1.0'
  AND mc.deleted_at IS NULL;

COMMENT ON VIEW v_feature_search_candidates IS 
    'Pre-filtered, pre-normalized feature vectors for fast similarity search queries';
