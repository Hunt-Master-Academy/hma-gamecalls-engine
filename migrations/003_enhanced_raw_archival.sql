-- [20251101-FEATURES-012] Enhanced Raw Archival Strategy
-- Manages JSONB storage lifecycle to prevent table bloat

-- Add archive pointer column for offloaded enhanced_raw data
ALTER TABLE master_call_features
ADD COLUMN enhanced_raw_archive_url TEXT DEFAULT NULL,
ADD COLUMN enhanced_raw_archived_at TIMESTAMPTZ DEFAULT NULL;

COMMENT ON COLUMN master_call_features.enhanced_raw_archive_url IS
    'S3/MinIO URL for archived enhanced_raw JSONB when offloaded from database';
COMMENT ON COLUMN master_call_features.enhanced_raw_archived_at IS
    'Timestamp when enhanced_raw was archived and removed from this table';

-- Create archive status view
CREATE OR REPLACE VIEW v_feature_archive_status AS
SELECT 
    master_call_id,
    feature_version,
    CASE 
        WHEN enhanced_raw IS NOT NULL AND enhanced_raw_archive_url IS NULL THEN 'active'
        WHEN enhanced_raw IS NULL AND enhanced_raw_archive_url IS NOT NULL THEN 'archived'
        WHEN enhanced_raw IS NOT NULL AND enhanced_raw_archive_url IS NOT NULL THEN 'dual'
        ELSE 'missing'
    END as archive_status,
    pg_column_size(enhanced_raw) as enhanced_raw_bytes,
    extracted_at,
    enhanced_raw_archived_at,
    enhanced_raw_archive_url
FROM master_call_features;

COMMENT ON VIEW v_feature_archive_status IS
    'Track enhanced_raw lifecycle: active (in DB), archived (in S3), dual (both), missing (lost)';

-- Function to mark enhanced_raw for archival (keeps record for 90 days, then eligible for archive)
CREATE OR REPLACE FUNCTION mark_old_enhanced_raw_for_archive(retention_days INTEGER DEFAULT 90)
RETURNS TABLE(
    master_call_id TEXT,
    feature_version TEXT,
    enhanced_raw_size INTEGER,
    extracted_at TIMESTAMPTZ
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        mcf.master_call_id::TEXT,
        mcf.feature_version::TEXT,
        pg_column_size(mcf.enhanced_raw) as enhanced_raw_size,
        mcf.extracted_at
    FROM master_call_features mcf
    WHERE mcf.enhanced_raw IS NOT NULL
      AND mcf.enhanced_raw_archive_url IS NULL
      AND mcf.extracted_at < NOW() - (retention_days || ' days')::INTERVAL
    ORDER BY mcf.extracted_at ASC;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION mark_old_enhanced_raw_for_archive IS
    'Identify enhanced_raw records older than retention_days eligible for S3 archive offload';

-- Index for archive queries
CREATE INDEX idx_features_archive_candidates 
ON master_call_features(extracted_at)
WHERE enhanced_raw IS NOT NULL AND enhanced_raw_archive_url IS NULL;

-- Statistics view for archive monitoring
CREATE OR REPLACE VIEW v_feature_archive_stats AS
SELECT 
    COUNT(*) as total_features,
    COUNT(*) FILTER (WHERE enhanced_raw IS NOT NULL) as active_in_db,
    COUNT(*) FILTER (WHERE enhanced_raw_archive_url IS NOT NULL) as archived_to_s3,
    SUM(pg_column_size(enhanced_raw)) FILTER (WHERE enhanced_raw IS NOT NULL) as total_jsonb_bytes,
    AVG(pg_column_size(enhanced_raw)) FILTER (WHERE enhanced_raw IS NOT NULL) as avg_jsonb_bytes,
    MAX(pg_column_size(enhanced_raw)) FILTER (WHERE enhanced_raw IS NOT NULL) as max_jsonb_bytes,
    COUNT(*) FILTER (WHERE extracted_at < NOW() - INTERVAL '90 days' 
                     AND enhanced_raw IS NOT NULL 
                     AND enhanced_raw_archive_url IS NULL) as eligible_for_archive
FROM master_call_features;

COMMENT ON VIEW v_feature_archive_stats IS
    'Dashboard metrics for enhanced_raw storage and archive eligibility';
