-- [20251101-FEATURES-013] Materialized Feature Search Candidates
-- Pre-computed normalized vectors for fast ANN lookup

-- Materialized table for fast similarity search
CREATE TABLE IF NOT EXISTS feature_search_materialized (
    id SERIAL PRIMARY KEY,
    master_call_id VARCHAR(255) NOT NULL REFERENCES master_calls(id) ON DELETE CASCADE,
    feature_version VARCHAR(20) NOT NULL DEFAULT 'v1.0',
    
    -- Metadata for filtering
    species VARCHAR(100) NOT NULL,
    call_type VARCHAR(100),
    duration_seconds FLOAT NOT NULL,
    
    -- Normalized feature vector (4D for V1.0, will expand to 64-128D in V1.1)
    feature_vector FLOAT[] NOT NULL,
    
    -- Individual normalized components for targeted queries
    tempo_norm FLOAT,          -- tempo_bpm / 200.0
    rms_norm FLOAT,            -- rms_energy_mean (already 0-1)
    pitch_norm FLOAT,          -- f0_mean / 1000.0
    harmonic_norm FLOAT,       -- harmonicity_mean / 500.0
    
    -- Quality metrics for filtering
    overall_quality_score FLOAT,
    confidence_score FLOAT,
    
    -- Metadata
    materialized_at TIMESTAMPTZ DEFAULT NOW(),
    source_extracted_at TIMESTAMPTZ NOT NULL,
    
    -- Constraints
    UNIQUE(master_call_id, feature_version)
);

-- Indexes for fast lookup
CREATE INDEX idx_search_mat_species ON feature_search_materialized(species, feature_version);
CREATE INDEX idx_search_mat_duration ON feature_search_materialized(duration_seconds)
    WHERE overall_quality_score >= 0.5;
CREATE INDEX idx_search_mat_quality ON feature_search_materialized(overall_quality_score DESC);

-- [20251101-FEATURES-014] Future: pgvector integration placeholder
-- Uncomment when pgvector extension is available:
-- CREATE EXTENSION IF NOT EXISTS vector;
-- ALTER TABLE feature_search_materialized ADD COLUMN embedding vector(4);
-- CREATE INDEX idx_search_mat_vector ON feature_search_materialized 
--     USING ivfflat (embedding vector_cosine_ops) WITH (lists = 100);

COMMENT ON TABLE feature_search_materialized IS
    'Pre-computed normalized feature vectors for fast ANN similarity search. Rebuilt periodically.';

-- Function to rebuild materialized search table
CREATE OR REPLACE FUNCTION rebuild_feature_search_materialized()
RETURNS TABLE(
    inserted INTEGER,
    updated INTEGER,
    deleted INTEGER,
    duration_ms BIGINT
) AS $$
DECLARE
    start_time TIMESTAMPTZ := clock_timestamp();
    ins_count INTEGER := 0;
    upd_count INTEGER := 0;
    del_count INTEGER := 0;
BEGIN
    -- Delete entries for calls that no longer exist or have outdated features
    DELETE FROM feature_search_materialized fsm
    WHERE NOT EXISTS (
        SELECT 1 FROM master_call_features mcf
        WHERE mcf.master_call_id = fsm.master_call_id
          AND mcf.feature_version = fsm.feature_version
          AND mcf.extraction_status = 'completed'
    );
    
    GET DIAGNOSTICS del_count = ROW_COUNT;
    
    -- Upsert from master_call_features
    INSERT INTO feature_search_materialized (
        master_call_id,
        feature_version,
        species,
        call_type,
        duration_seconds,
        feature_vector,
        tempo_norm,
        rms_norm,
        pitch_norm,
        harmonic_norm,
        overall_quality_score,
        confidence_score,
        source_extracted_at
    )
    SELECT 
        mcf.master_call_id,
        mcf.feature_version,
        mc.species,
        mc.call_type,
        mcf.duration_seconds,
        -- 4D normalized vector
        ARRAY[
            COALESCE(mcf.tempo_bpm / 200.0, 0),
            COALESCE(mcf.rms_energy_mean, 0),
            COALESCE(mcf.f0_mean / 1000.0, 0),
            COALESCE(mcf.harmonicity_mean / 500.0, 0)
        ] as feature_vector,
        COALESCE(mcf.tempo_bpm / 200.0, 0) as tempo_norm,
        COALESCE(mcf.rms_energy_mean, 0) as rms_norm,
        COALESCE(mcf.f0_mean / 1000.0, 0) as pitch_norm,
        COALESCE(mcf.harmonicity_mean / 500.0, 0) as harmonic_norm,
        mcf.overall_quality_score,
        mcf.confidence_score,
        mcf.extracted_at
    FROM master_call_features mcf
    JOIN master_calls mc ON mcf.master_call_id = mc.id
    WHERE mcf.extraction_status = 'completed'
      AND mc.deleted_at IS NULL
    ON CONFLICT (master_call_id, feature_version)
    DO UPDATE SET
        species = EXCLUDED.species,
        call_type = EXCLUDED.call_type,
        duration_seconds = EXCLUDED.duration_seconds,
        feature_vector = EXCLUDED.feature_vector,
        tempo_norm = EXCLUDED.tempo_norm,
        rms_norm = EXCLUDED.rms_norm,
        pitch_norm = EXCLUDED.pitch_norm,
        harmonic_norm = EXCLUDED.harmonic_norm,
        overall_quality_score = EXCLUDED.overall_quality_score,
        confidence_score = EXCLUDED.confidence_score,
        source_extracted_at = EXCLUDED.source_extracted_at,
        materialized_at = NOW();
    
    GET DIAGNOSTICS ins_count = ROW_COUNT;
    
    -- Count updates (rows that existed before)
    upd_count := ins_count - (SELECT COUNT(*) FROM feature_search_materialized 
                               WHERE materialized_at = NOW());
    
    RETURN QUERY SELECT 
        ins_count,
        upd_count,
        del_count,
        EXTRACT(EPOCH FROM (clock_timestamp() - start_time) * 1000)::BIGINT;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION rebuild_feature_search_materialized IS
    'Rebuild materialized feature search table from master_call_features. Run nightly or after bulk extractions.';

-- View for stale materialization detection
CREATE OR REPLACE VIEW v_feature_search_freshness AS
SELECT 
    COUNT(*) as total_materialized,
    MAX(materialized_at) as last_materialized,
    COUNT(*) FILTER (WHERE materialized_at < NOW() - INTERVAL '1 day') as stale_entries,
    COUNT(*) FILTER (WHERE source_extracted_at > materialized_at) as needs_update
FROM feature_search_materialized;

COMMENT ON VIEW v_feature_search_freshness IS
    'Monitor freshness of materialized search candidates table';
