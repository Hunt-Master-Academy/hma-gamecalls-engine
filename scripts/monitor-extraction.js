#!/usr/bin/env node
/**
 * [20251101-FEATURES-015] Feature Extraction Monitoring & Telemetry
 * 
 * Provides operational metrics for extraction pipeline:
 * - Extraction success/failure rates
 * - Performance metrics (P50, P95, P99 latencies)
 * - Queue depth and throughput
 * - Feature completeness and quality
 * - Anomaly detection (NaN, overflow, extreme values)
 */

const path = require('path');
const servicesPath = path.join(__dirname, '../src/services');
const databaseService = require(path.join(servicesPath, 'databaseService'));

class ExtractionMonitor {
    async getMetrics(timeWindowHours = 24) {
        console.log(`\n📊 Feature Extraction Telemetry (Last ${timeWindowHours}h)\n`);
        console.log('═══════════════════════════════════════════════════════════════\n');

        const metrics = {
            extraction: await this.getExtractionMetrics(timeWindowHours),
            performance: await this.getPerformanceMetrics(timeWindowHours),
            quality: await this.getQualityMetrics(),
            anomalies: await this.getAnomalyMetrics(),
            storage: await this.getStorageMetrics()
        };

        this.printMetrics(metrics);
        return metrics;
    }

    async getExtractionMetrics(hours) {
        const query = `
            SELECT 
                COUNT(*) as total_extractions,
                COUNT(*) FILTER (WHERE extraction_status = 'completed') as successful,
                COUNT(*) FILTER (WHERE extraction_status = 'failed') as failed,
                COUNT(*) FILTER (WHERE extraction_status = 'partial') as partial,
                COUNT(*) FILTER (WHERE extraction_status = 'needs_review') as needs_review,
                ROUND(COUNT(*) FILTER (WHERE extraction_status = 'completed')::numeric / 
                      NULLIF(COUNT(*), 0) * 100, 2) as success_rate_pct
            FROM master_call_features
            WHERE extracted_at >= NOW() - ($1 || ' hours')::INTERVAL
        `;
        const result = await databaseService.raw(query, [hours]);
        return result.rows[0];
    }

    async getPerformanceMetrics(hours) {
        const query = `
            SELECT 
                COUNT(*) as sample_size,
                ROUND(AVG(computation_time_ms)::numeric, 2) as mean_ms,
                PERCENTILE_CONT(0.5) WITHIN GROUP (ORDER BY computation_time_ms) as p50_ms,
                PERCENTILE_CONT(0.95) WITHIN GROUP (ORDER BY computation_time_ms) as p95_ms,
                PERCENTILE_CONT(0.99) WITHIN GROUP (ORDER BY computation_time_ms) as p99_ms,
                MAX(computation_time_ms) as max_ms,
                MIN(computation_time_ms) as min_ms,
                ROUND(AVG(computation_time_ms / (duration_seconds * 1000) * 100)::numeric, 2) 
                    as avg_realtime_ratio_pct
            FROM master_call_features
            WHERE extracted_at >= NOW() - ($1 || ' hours')::INTERVAL
              AND extraction_status = 'completed'
              AND computation_time_ms IS NOT NULL
              AND duration_seconds > 0
        `;
        const result = await databaseService.raw(query, [hours]);
        return result.rows[0];
    }

    async getQualityMetrics() {
        const query = `
            SELECT 
                COUNT(*) as total_features,
                COUNT(*) FILTER (WHERE overall_quality_score IS NOT NULL) as with_quality_score,
                ROUND(AVG(overall_quality_score)::numeric, 4) as avg_quality_score,
                ROUND(STDDEV(overall_quality_score)::numeric, 4) as quality_stddev,
                PERCENTILE_CONT(0.25) WITHIN GROUP (ORDER BY overall_quality_score) as q1_quality,
                PERCENTILE_CONT(0.5) WITHIN GROUP (ORDER BY overall_quality_score) as median_quality,
                PERCENTILE_CONT(0.75) WITHIN GROUP (ORDER BY overall_quality_score) as q3_quality,
                COUNT(*) FILTER (WHERE confidence_score IS NOT NULL) as with_confidence,
                ROUND(AVG(confidence_score)::numeric, 4) as avg_confidence
            FROM master_call_features
            WHERE extraction_status = 'completed'
        `;
        const result = await databaseService.raw(query, []);
        return result.rows[0];
    }

    async getAnomalyMetrics() {
        const query = `
            SELECT 
                COUNT(*) FILTER (
                    WHERE duration_seconds IS NULL 
                       OR duration_seconds < 0 
                       OR duration_seconds = 'NaN'::float
                ) as invalid_duration,
                COUNT(*) FILTER (
                    WHERE num_frames IS NULL 
                       OR num_frames < 0
                ) as invalid_frames,
                COUNT(*) FILTER (
                    WHERE rms_energy_mean IS NULL 
                       OR rms_energy_mean = 'NaN'::float 
                       OR rms_energy_mean < 0
                       OR rms_energy_mean > 10
                ) as invalid_rms,
                COUNT(*) FILTER (
                    WHERE tempo_bpm IS NOT NULL 
                      AND (tempo_bpm < 0 OR tempo_bpm > 500)
                ) as invalid_tempo,
                COUNT(*) FILTER (
                    WHERE f0_mean IS NOT NULL 
                      AND (f0_mean < 0 OR f0_mean > 5000)
                ) as invalid_pitch,
                COUNT(*) FILTER (
                    WHERE enhanced_raw IS NULL
                ) as missing_enhanced_raw,
                COUNT(*) FILTER (
                    WHERE engine_build_sha IS NULL
                ) as missing_build_sha
            FROM master_call_features
        `;
        const result = await databaseService.raw(query, []);
        return result.rows[0];
    }

    async getStorageMetrics() {
        const query = `
            SELECT 
                COUNT(*) as total_rows,
                pg_size_pretty(pg_total_relation_size('master_call_features')) as table_size,
                pg_size_pretty(SUM(pg_column_size(enhanced_raw))) as jsonb_size,
                ROUND(AVG(pg_column_size(enhanced_raw))::numeric, 0) as avg_jsonb_bytes,
                MAX(pg_column_size(enhanced_raw)) as max_jsonb_bytes,
                COUNT(*) FILTER (WHERE enhanced_raw IS NOT NULL) as with_jsonb,
                COUNT(*) FILTER (WHERE enhanced_raw_archive_url IS NOT NULL) as archived_count
            FROM master_call_features
        `;
        const result = await databaseService.raw(query, []);
        return result.rows[0];
    }

    printMetrics(metrics) {
        // Extraction Metrics
        console.log('🔄 Extraction Status:');
        console.log(`  Total: ${metrics.extraction.total_extractions}`);
        console.log(`  ✅ Completed: ${metrics.extraction.successful} (${metrics.extraction.success_rate_pct}%)`);
        if (metrics.extraction.failed > 0) {
            console.log(`  ❌ Failed: ${metrics.extraction.failed}`);
        }
        if (metrics.extraction.partial > 0) {
            console.log(`  ⚠️  Partial: ${metrics.extraction.partial}`);
        }
        if (metrics.extraction.needs_review > 0) {
            console.log(`  🔍 Needs Review: ${metrics.extraction.needs_review}`);
        }
        console.log('');

        // Performance Metrics
        if (metrics.performance.sample_size > 0) {
            console.log('⚡ Performance (Extraction Time):');
            console.log(`  Sample Size: ${metrics.performance.sample_size}`);
            console.log(`  Mean: ${metrics.performance.mean_ms}ms`);
            console.log(`  P50: ${metrics.performance.p50_ms}ms`);
            console.log(`  P95: ${metrics.performance.p95_ms}ms`);
            console.log(`  P99: ${metrics.performance.p99_ms}ms`);
            console.log(`  Range: ${metrics.performance.min_ms}ms - ${metrics.performance.max_ms}ms`);
            console.log(`  Avg Real-time Ratio: ${metrics.performance.avg_realtime_ratio_pct}% (${metrics.performance.avg_realtime_ratio_pct < 100 ? 'faster than playback ✓' : 'slower than playback ⚠️'})`);
            console.log('');
        }

        // Quality Metrics
        console.log('📈 Quality Metrics:');
        console.log(`  Total Features: ${metrics.quality.total_features}`);
        console.log(`  With Quality Score: ${metrics.quality.with_quality_score}`);
        if (metrics.quality.avg_quality_score) {
            console.log(`  Avg Quality: ${metrics.quality.avg_quality_score} ± ${metrics.quality.quality_stddev}`);
            console.log(`  Quality Range: Q1=${metrics.quality.q1_quality} | Median=${metrics.quality.median_quality} | Q3=${metrics.quality.q3_quality}`);
        }
        if (metrics.quality.avg_confidence) {
            console.log(`  Avg Confidence: ${metrics.quality.avg_confidence}`);
        }
        console.log('');

        // Anomaly Detection
        const totalAnomalies = Object.values(metrics.anomalies).reduce((sum, val) => sum + parseInt(val || 0), 0);
        console.log(`🚨 Anomaly Detection: ${totalAnomalies === 0 ? '✅ No anomalies' : '⚠️ ' + totalAnomalies + ' issues'}`);
        if (totalAnomalies > 0) {
            if (metrics.anomalies.invalid_duration > 0) {
                console.log(`  ❌ Invalid Duration: ${metrics.anomalies.invalid_duration}`);
            }
            if (metrics.anomalies.invalid_frames > 0) {
                console.log(`  ❌ Invalid Frames: ${metrics.anomalies.invalid_frames}`);
            }
            if (metrics.anomalies.invalid_rms > 0) {
                console.log(`  ❌ Invalid RMS: ${metrics.anomalies.invalid_rms}`);
            }
            if (metrics.anomalies.invalid_tempo > 0) {
                console.log(`  ⚠️  Invalid Tempo: ${metrics.anomalies.invalid_tempo}`);
            }
            if (metrics.anomalies.invalid_pitch > 0) {
                console.log(`  ⚠️  Invalid Pitch: ${metrics.anomalies.invalid_pitch}`);
            }
            if (metrics.anomalies.missing_enhanced_raw > 0) {
                console.log(`  ⚠️  Missing enhanced_raw: ${metrics.anomalies.missing_enhanced_raw}`);
            }
            if (metrics.anomalies.missing_build_sha > 0) {
                console.log(`  ⚠️  Missing build_sha: ${metrics.anomalies.missing_build_sha}`);
            }
        }
        console.log('');

        // Storage Metrics
        console.log('💾 Storage:');
        console.log(`  Table Size: ${metrics.storage.table_size}`);
        console.log(`  JSONB Total: ${metrics.storage.jsonb_size}`);
        console.log(`  JSONB Avg/Max: ${metrics.storage.avg_jsonb_bytes}B / ${metrics.storage.max_jsonb_bytes}B`);
        console.log(`  Rows with JSONB: ${metrics.storage.with_jsonb}/${metrics.storage.total_rows}`);
        if (metrics.storage.archived_count > 0) {
            console.log(`  Archived to S3: ${metrics.storage.archived_count}`);
        }
        console.log('');
    }

    // Prometheus-compatible metrics export
    async exportPrometheusMetrics() {
        const metrics = {
            extraction: await this.getExtractionMetrics(24),
            performance: await this.getPerformanceMetrics(24),
            quality: await this.getQualityMetrics(),
            anomalies: await this.getAnomalyMetrics()
        };

        const lines = [
            '# HELP gamecalls_extraction_total Total feature extractions',
            '# TYPE gamecalls_extraction_total counter',
            `gamecalls_extraction_total{status="completed"} ${metrics.extraction.successful}`,
            `gamecalls_extraction_total{status="failed"} ${metrics.extraction.failed}`,
            `gamecalls_extraction_total{status="partial"} ${metrics.extraction.partial}`,
            '',
            '# HELP gamecalls_extraction_duration_ms Feature extraction duration in milliseconds',
            '# TYPE gamecalls_extraction_duration_ms summary',
            `gamecalls_extraction_duration_ms{quantile="0.5"} ${metrics.performance.p50_ms || 0}`,
            `gamecalls_extraction_duration_ms{quantile="0.95"} ${metrics.performance.p95_ms || 0}`,
            `gamecalls_extraction_duration_ms{quantile="0.99"} ${metrics.performance.p99_ms || 0}`,
            '',
            '# HELP gamecalls_quality_score Average feature quality score',
            '# TYPE gamecalls_quality_score gauge',
            `gamecalls_quality_score ${metrics.quality.avg_quality_score || 0}`,
            '',
            '# HELP gamecalls_anomalies_total Total anomalies detected',
            '# TYPE gamecalls_anomalies_total gauge',
            `gamecalls_anomalies_total{type="invalid_duration"} ${metrics.anomalies.invalid_duration}`,
            `gamecalls_anomalies_total{type="invalid_rms"} ${metrics.anomalies.invalid_rms}`,
            `gamecalls_anomalies_total{type="missing_enhanced_raw"} ${metrics.anomalies.missing_enhanced_raw}`
        ];

        return lines.join('\n');
    }
}

async function main() {
    const args = process.argv.slice(2);
    const format = args.includes('--prometheus') ? 'prometheus' : 'console';
    const timeWindow = parseInt(args[args.indexOf('--window') + 1]) || 24;

    try {
        const monitor = new ExtractionMonitor();
        
        if (format === 'prometheus') {
            const output = await monitor.exportPrometheusMetrics();
            console.log(output);
        } else {
            await monitor.getMetrics(timeWindow);
        }
        
        process.exit(0);
    } catch (error) {
        console.error('\n❌ Error:', error.message);
        process.exit(1);
    }
}

if (require.main === module) {
    main();
}

module.exports = ExtractionMonitor;
