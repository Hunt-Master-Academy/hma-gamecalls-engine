-- [20251029-DATA-001] Seed master calls for testing and initial library
-- Initial set of master calls across multiple species

INSERT INTO master_calls (
    id, name, species, call_type, difficulty,
    audio_file_path, duration_seconds, sample_rate, file_size_bytes, audio_format,
    description, usage_notes, season, context, tags,
    quality_score, validated, is_public, is_premium, created_by
) VALUES
-- Elk calls
('call_elk_bugle_001', 'Bull Elk Bugle', 'elk', 'bugle', 'advanced',
 'master-calls/elk/call_elk_bugle_001.wav', 4.8, 44100, 423936, 'wav',
 'Mature bull elk bugle with chuckles - challenging call requiring breath control',
 'Practice steady breath support. Start low, crescendo to peak, finish with chuckles.',
 'fall_rut', 'challenge_call', ARRAY['bull', 'rut', 'challenge', 'advanced', 'september'],
 0.92, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_elk_cow_001', 'Cow Elk Mew', 'elk', 'mew', 'beginner',
 'master-calls/elk/call_elk_cow_001.wav', 2.1, 44100, 185220, 'wav',
 'Basic cow elk mew - gentle contact call',
 'Soft, high-pitched mew. Good for locating herd without spooking.',
 'year-round', 'contact_call', ARRAY['cow', 'beginner', 'locate', 'gentle'],
 0.88, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

-- Whitetail deer calls  
('call_deer_grunt_001', 'Basic Buck Grunt', 'whitetail_deer', 'grunt', 'beginner',
 'master-calls/deer/call_deer_grunt_001.wav', 2.5, 44100, 220500, 'wav',
 'Standard buck grunt call for attracting deer during rut season',
 'Short, guttural grunt. 2-3 second intervals. Works best early morning/late evening.',
 'fall_rut', 'attract', ARRAY['rut', 'buck', 'attract', 'basic', 'november'],
 0.90, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_deer_bleat_001', 'Doe Bleat', 'whitetail_deer', 'bleat', 'intermediate',
 'master-calls/deer/call_deer_bleat_001.wav', 3.2, 44100, 282240, 'wav',
 'Estrous doe bleat - highly effective during peak rut',
 'Higher pitch than grunt. Varies length 1-3 seconds. Use sparingly to avoid over-calling.',
 'fall_rut', 'mating_call', ARRAY['doe', 'estrous', 'rut', 'intermediate'],
 0.91, TRUE, TRUE, TRUE, 'info@huntmasteracademy.com'),

-- Wild turkey calls
('call_turkey_yelp_001', 'Hen Turkey Yelp', 'wild_turkey', 'yelp', 'intermediate',
 'master-calls/turkey/call_turkey_yelp_001.wav', 3.2, 44100, 282240, 'wav',
 'Classic hen turkey yelp sequence for spring gobbler hunting',
 'Series of 4-6 yelps with rhythm. Start soft, increase volume mid-sequence.',
 'spring', 'locate_and_attract', ARRAY['spring', 'hen', 'gobbler', 'sequence'],
 0.89, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_turkey_cluck_001', 'Turkey Cluck', 'wild_turkey', 'cluck', 'beginner',
 'master-calls/turkey/call_turkey_cluck_001.wav', 1.5, 44100, 132300, 'wav',
 'Basic turkey cluck - versatile call for various situations',
 'Short, sharp sounds. 1-3 clucks at a time. Use for close-range communication.',
 'year-round', 'contact_call', ARRAY['cluck', 'versatile', 'beginner', 'close-range'],
 0.87, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_turkey_purr_001', 'Turkey Purr', 'wild_turkey', 'purr', 'advanced',
 'master-calls/turkey/call_turkey_purr_001.wav', 4.0, 44100, 352800, 'wav',
 'Rolling purr sound - indicates contentment, used by expert callers',
 'Continuous rolling vibration. Requires specialized technique. Very effective when mastered.',
 'spring', 'finesse', ARRAY['purr', 'advanced', 'expert', 'rolling', 'contentment'],
 0.93, TRUE, TRUE, TRUE, 'info@huntmasteracademy.com'),

-- Waterfowl calls
('call_duck_quack_001', 'Mallard Hen Quack', 'mallard_duck', 'quack', 'beginner',
 'master-calls/duck/call_duck_quack_001.wav', 1.8, 44100, 158760, 'wav',
 'Basic mallard hen quack - foundation waterfowl call',
 'Single quack or 2-3 quack series. Vary volume for different scenarios.',
 'fall', 'attract', ARRAY['waterfowl', 'hen', 'basic', 'mallard'],
 0.86, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_duck_greeting_001', 'Mallard Greeting Call', 'mallard_duck', 'greeting', 'intermediate',
 'master-calls/duck/call_duck_greeting_001.wav', 2.8, 44100, 246960, 'wav',
 'Loud greeting call sequence for attracting passing ducks',
 '5-7 note descending call. High volume. Use when ducks are distant.',
 'fall', 'long_range_attract', ARRAY['greeting', 'loud', 'sequence', 'intermediate'],
 0.88, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

-- Predator calls  
('call_coyote_howl_001', 'Lone Coyote Howl', 'coyote', 'howl', 'intermediate',
 'master-calls/coyote/call_coyote_howl_001.wav', 5.2, 44100, 458640, 'wav',
 'Lone coyote howl for locating or attracting coyotes',
 'Long, mournful howl. Start low, rise to peak, fade out. Wait 2-3 minutes between calls.',
 'year-round', 'locate', ARRAY['predator', 'howl', 'locate', 'lone'],
 0.90, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com'),

('call_rabbit_distress_001', 'Cottontail Distress', 'rabbit', 'distress', 'beginner',
 'master-calls/rabbit/call_rabbit_distress_001.wav', 3.5, 44100, 308700, 'wav',
 'Cottontail rabbit distress call - effective for predators',
 'High-pitched screaming sound. 30-45 seconds of calling, then 3-5 minute silence.',
 'year-round', 'distress_call', ARRAY['distress', 'predator', 'beginner', 'effective'],
 0.85, TRUE, TRUE, FALSE, 'info@huntmasteracademy.com');

-- [20251029-DATA-002] Update usage statistics for popular calls
UPDATE master_calls SET usage_count = 150, success_rate = 72.5 WHERE id = 'call_elk_bugle_001';
UPDATE master_calls SET usage_count = 450, success_rate = 85.2 WHERE id = 'call_deer_grunt_001';
UPDATE master_calls SET usage_count = 320, success_rate = 78.8 WHERE id = 'call_turkey_yelp_001';
UPDATE master_calls SET usage_count = 520, success_rate = 91.3 WHERE id = 'call_duck_quack_001';
