-- [20251029-DB-001] Master Calls table for audio call library
-- Stores metadata and references to audio files in MinIO

CREATE TABLE IF NOT EXISTS master_calls (
    -- Primary identification
    id VARCHAR(255) PRIMARY KEY,
    
    -- Call metadata
    name VARCHAR(255) NOT NULL,
    species VARCHAR(100) NOT NULL,
    call_type VARCHAR(100) NOT NULL,
    difficulty VARCHAR(50) NOT NULL CHECK (difficulty IN ('beginner', 'intermediate', 'advanced', 'expert')),
    
    -- Audio file details
    audio_file_path VARCHAR(500) NOT NULL, -- MinIO bucket path
    duration_seconds DECIMAL(10, 3) NOT NULL,
    sample_rate INTEGER NOT NULL DEFAULT 44100,
    file_size_bytes BIGINT NOT NULL,
    audio_format VARCHAR(20) NOT NULL DEFAULT 'wav',
    
    -- Waveform visualization
    waveform_data_path VARCHAR(500), -- MinIO path to waveform JSON
    waveform_generated BOOLEAN DEFAULT FALSE,
    
    -- Descriptive content
    description TEXT,
    usage_notes TEXT,
    season VARCHAR(100), -- e.g., 'spring', 'fall_rut', 'year-round'
    context VARCHAR(255), -- e.g., 'mating_call', 'distress', 'feeding'
    
    -- Tagging and categorization
    tags TEXT[], -- Array of searchable tags
    
    -- Quality and validation
    quality_score DECIMAL(3, 2) DEFAULT 0.00 CHECK (quality_score >= 0 AND quality_score <= 1),
    validated BOOLEAN DEFAULT FALSE,
    validated_by VARCHAR(255),
    validated_at TIMESTAMP WITH TIME ZONE,
    
    -- Usage statistics
    usage_count INTEGER DEFAULT 0,
    success_rate DECIMAL(5, 2), -- Average user success rate with this call
    
    -- Administrative
    created_by VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    deleted_at TIMESTAMP WITH TIME ZONE, -- Soft delete support
    
    -- Visibility control
    is_public BOOLEAN DEFAULT TRUE,
    is_premium BOOLEAN DEFAULT FALSE
);

-- [20251029-DB-002] Indexes for common query patterns
CREATE INDEX IF NOT EXISTS idx_master_calls_species ON master_calls(species) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_master_calls_call_type ON master_calls(call_type) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_master_calls_difficulty ON master_calls(difficulty) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_master_calls_tags ON master_calls USING GIN(tags) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_master_calls_created_at ON master_calls(created_at DESC) WHERE deleted_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_master_calls_quality ON master_calls(quality_score DESC) WHERE deleted_at IS NULL AND validated = TRUE;

-- [20251029-DB-003] Full-text search index
CREATE INDEX IF NOT EXISTS idx_master_calls_search ON master_calls USING GIN(
    to_tsvector('english', coalesce(name, '') || ' ' || coalesce(description, '') || ' ' || coalesce(usage_notes, ''))
) WHERE deleted_at IS NULL;

-- [20251029-DB-004] Trigger to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_master_calls_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_update_master_calls_updated_at
    BEFORE UPDATE ON master_calls
    FOR EACH ROW
    EXECUTE FUNCTION update_master_calls_updated_at();

-- [20251029-DB-005] Comments for documentation
COMMENT ON TABLE master_calls IS 'Master audio call library for GameCalls Engine - stores metadata and MinIO references';
COMMENT ON COLUMN master_calls.id IS 'Unique identifier (e.g., call_elk_bugle_001)';
COMMENT ON COLUMN master_calls.audio_file_path IS 'MinIO bucket path: master-calls/{species}/{id}.wav';
COMMENT ON COLUMN master_calls.waveform_data_path IS 'MinIO path to pre-generated waveform JSON for visualization';
COMMENT ON COLUMN master_calls.tags IS 'Searchable tags array: {rut, buck, attract, basic}';
COMMENT ON COLUMN master_calls.quality_score IS 'Automated quality score from C++ engine analysis (0.00-1.00)';
COMMENT ON COLUMN master_calls.success_rate IS 'Average user success rate percentage with this master call';
COMMENT ON COLUMN master_calls.deleted_at IS 'Soft delete timestamp - NULL means active';
