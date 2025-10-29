-- [20251028-DB-001] GameCalls Engine Database Schema
-- Stores session metadata, analysis results, and user progress

-- Sessions table: Core session metadata
CREATE TABLE IF NOT EXISTS gamecalls_sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    master_call_id VARCHAR(255) NOT NULL,
    master_call_path TEXT,
    
    -- Session state
    status VARCHAR(50) NOT NULL DEFAULT 'created',
    -- Possible values: 'created', 'recording', 'completed', 'error'
    
    -- Configuration
    sample_rate INTEGER NOT NULL DEFAULT 44100,
    buffer_size INTEGER NOT NULL DEFAULT 1024,
    enable_enhanced_analysis BOOLEAN NOT NULL DEFAULT true,
    
    -- Analysis settings
    pitch_analysis BOOLEAN NOT NULL DEFAULT true,
    harmonic_analysis BOOLEAN NOT NULL DEFAULT true,
    cadence_analysis BOOLEAN NOT NULL DEFAULT true,
    real_time_scoring BOOLEAN NOT NULL DEFAULT true,
    
    -- Timestamps
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    started_at TIMESTAMP WITH TIME ZONE,
    completed_at TIMESTAMP WITH TIME ZONE,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    
    -- Indexing
    CONSTRAINT check_status CHECK (status IN ('created', 'recording', 'completed', 'error'))
);

-- Analysis results table: Stores final analysis output
CREATE TABLE IF NOT EXISTS gamecalls_analysis (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID NOT NULL REFERENCES gamecalls_sessions(id) ON DELETE CASCADE,
    
    -- Overall scores
    overall_score NUMERIC(5,4) NOT NULL,
    similarity_score NUMERIC(5,4) NOT NULL,
    confidence NUMERIC(5,4) NOT NULL,
    grade VARCHAR(2), -- A, B, C, D, F
    
    -- Enhanced analysis scores
    pitch_score NUMERIC(5,4),
    pitch_frequency NUMERIC(8,2),
    pitch_confidence NUMERIC(5,4),
    
    harmonic_score NUMERIC(5,4),
    harmonic_spectral_centroid NUMERIC(10,2),
    harmonic_confidence NUMERIC(5,4),
    
    cadence_score NUMERIC(5,4),
    cadence_bpm NUMERIC(6,2),
    cadence_rhythm_strength NUMERIC(5,4),
    
    loudness_rms NUMERIC(8,4),
    loudness_peak NUMERIC(8,4),
    loudness_normalization NUMERIC(5,4),
    
    -- Performance metrics
    processing_time_ms INTEGER,
    
    -- Storage references
    minio_results_key TEXT, -- Path to full JSON results in MinIO
    minio_recording_key TEXT, -- Path to user recording in MinIO
    
    -- Timestamps
    analyzed_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    
    -- Unique constraint: one analysis per session
    CONSTRAINT unique_session_analysis UNIQUE (session_id)
);

-- Segments table: Stores detected audio segments
CREATE TABLE IF NOT EXISTS gamecalls_segments (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID NOT NULL REFERENCES gamecalls_sessions(id) ON DELETE CASCADE,
    
    -- Segment timing
    start_time NUMERIC(10,3) NOT NULL, -- seconds
    end_time NUMERIC(10,3) NOT NULL,
    duration NUMERIC(10,3) NOT NULL,
    
    -- Segment quality
    is_best_segment BOOLEAN NOT NULL DEFAULT false,
    segment_score NUMERIC(5,4),
    vad_confidence NUMERIC(5,4),
    
    -- Timestamps
    detected_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

-- Feedback table: Coaching feedback for sessions
CREATE TABLE IF NOT EXISTS gamecalls_feedback (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID NOT NULL REFERENCES gamecalls_sessions(id) ON DELETE CASCADE,
    
    -- Feedback content
    strengths TEXT[], -- Array of strength descriptions
    improvements TEXT[], -- Array of improvement suggestions
    tips TEXT[], -- Array of coaching tips
    
    -- Feedback metadata
    generated_by VARCHAR(50) DEFAULT 'system', -- 'system', 'ai', 'instructor'
    generated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    
    -- Unique constraint: one feedback per session
    CONSTRAINT unique_session_feedback UNIQUE (session_id)
);

-- User progress table: Track user competency over time
CREATE TABLE IF NOT EXISTS gamecalls_user_progress (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    
    -- Competency tracking
    species VARCHAR(100) NOT NULL,
    call_type VARCHAR(100) NOT NULL,
    difficulty VARCHAR(50) NOT NULL,
    
    -- Performance metrics
    attempts INTEGER NOT NULL DEFAULT 0,
    completions INTEGER NOT NULL DEFAULT 0,
    best_score NUMERIC(5,4),
    average_score NUMERIC(5,4),
    latest_score NUMERIC(5,4),
    
    -- Progress indicators
    proficiency_level VARCHAR(50), -- 'beginner', 'intermediate', 'advanced', 'expert'
    mastery_percentage NUMERIC(5,2), -- 0-100%
    
    -- Timestamps
    first_attempt_at TIMESTAMP WITH TIME ZONE,
    last_attempt_at TIMESTAMP WITH TIME ZONE,
    mastered_at TIMESTAMP WITH TIME ZONE, -- When they achieved mastery
    
    -- Indexing for common queries
    CONSTRAINT unique_user_species_calltype UNIQUE (user_id, species, call_type, difficulty)
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON gamecalls_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_status ON gamecalls_sessions(status);
CREATE INDEX IF NOT EXISTS idx_sessions_created_at ON gamecalls_sessions(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_analysis_session_id ON gamecalls_analysis(session_id);
CREATE INDEX IF NOT EXISTS idx_segments_session_id ON gamecalls_segments(session_id);
CREATE INDEX IF NOT EXISTS idx_feedback_session_id ON gamecalls_feedback(session_id);
CREATE INDEX IF NOT EXISTS idx_progress_user_id ON gamecalls_user_progress(user_id);
CREATE INDEX IF NOT EXISTS idx_progress_species ON gamecalls_user_progress(species, call_type);

-- Triggers for updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER update_gamecalls_sessions_updated_at
    BEFORE UPDATE ON gamecalls_sessions
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Comments for documentation
COMMENT ON TABLE gamecalls_sessions IS 'Core session metadata for GameCalls analysis';
COMMENT ON TABLE gamecalls_analysis IS 'Final analysis results and scores';
COMMENT ON TABLE gamecalls_segments IS 'Detected audio segments within sessions';
COMMENT ON TABLE gamecalls_feedback IS 'Coaching feedback generated for sessions';
COMMENT ON TABLE gamecalls_user_progress IS 'User competency tracking over time';
