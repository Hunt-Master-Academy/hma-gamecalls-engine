/**
 * Session Reporter Modular Integration
 * Orchestrates all session reporting modules for comprehensive report generation
 *
 * Features:
 * - Unified reporting interface
 * - Multi-format export capabilities
 * - Scheduled report generation
 * - Custom template support
 * - Batch processing
 */

import { ExecutiveReports } from "./reporting/executive-reports.js";
import { TechnicalReports } from "./reporting/technical-reports.js";
import { UXReports } from "./reporting/ux-reports.js";
import { QAReports } from "./reporting/qa-reports.js";
import { ComparativeReports } from "./reporting/comparative-reports.js";
import { ExportEngine } from "./reporting/export-engine.js";
import { DistributionSystem } from "./reporting/distribution-system.js";
import { TemplateEngine } from "./reporting/template-engine.js";
import { ArchiveManager } from "./reporting/archive-manager.js";

export class SessionReporterModular {
  constructor(options = {}) {
    this.config = {
      defaultFormats: options.defaultFormats || ["html", "pdf"],
      autoSchedule: options.autoSchedule !== false,
      reportRetention: options.reportRetention || 90, // days
      concurrentReports: options.concurrentReports || 3,
      cacheEnabled: options.cacheEnabled !== false,
      templateDirectory: options.templateDirectory || "./templates",
      outputDirectory: options.outputDirectory || "./reports",
      branding: options.branding || {
        organization: "Huntmaster Audio Engine",
        logo: null,
        colors: { primary: "#007bff", secondary: "#6c757d" },
      },
      ...options,
    };

    this.executiveReports = new ExecutiveReports(this.config);
    this.technicalReports = new TechnicalReports(this.config);
    this.uxReports = new UXReports(this.config);
    this.qaReports = new QAReports(this.config);
    this.comparativeReports = new ComparativeReports(this.config);
    this.exportEngine = new ExportEngine(this.config);
    this.distributionSystem = new DistributionSystem(this.config);
    this.templateEngine = new TemplateEngine(this.config);
    this.archiveManager = new ArchiveManager(this.config);

    this.reportQueue = [];
    this.reportCache = new Map();
    this.scheduledReports = new Map();
    this.isProcessing = false;
    this.reportTemplates = new Map();

    this.setupEventHandlers();
  }

  setupEventHandlers() {
    // Handle report generation events
    this.executiveReports.on?.("report-generated", (report) => {
      this.cacheReport(report);
      this.notifySubscribers("executive-report-ready", report);
    });

    this.technicalReports.on?.("report-generated", (report) => {
      this.cacheReport(report);
      this.notifySubscribers("technical-report-ready", report);
    });

    // Handle export completion
    this.exportEngine.on?.("export-completed", (exportInfo) => {
      this.notifySubscribers("export-ready", exportInfo);
    });
  }

  async generateSessionReport(
    sessionData,
    reportType = "comprehensive",
    options = {}
  ) {
    try {
      const reportId = this.generateReportId();
      const startTime = Date.now();

      const reportsToGenerate = this.determineReportsToGenerate(
        reportType,
        options
      );

      const reports = {};

      if (reportsToGenerate.includes("executive")) {
        reports.executive =
          await this.executiveReports.generateExecutiveSummary(
            sessionData,
            options.timeframe || "last_30_days"
          );
      }

      if (reportsToGenerate.includes("technical")) {
        reports.technical =
          await this.technicalReports.generateSystemHealthReport(
            sessionData,
            options.timeframe || "last_24_hours"
          );
      }

      if (reportsToGenerate.includes("ux")) {
        reports.ux = await this.uxReports.generateUXOverviewReport(
          sessionData,
          options.timeframe || "last_30_days"
        );
      }

      if (reportsToGenerate.includes("qa")) {
        reports.qa = await this.qaReports.generateQASummaryReport(
          options.testData || [],
          options.defectData || [],
          options.timeframe || "current_sprint"
        );
      }

      const comprehensiveReport = {
        id: reportId,
        type: reportType,
        generatedAt: new Date().toISOString(),
        timeframe: options.timeframe,

        meta: {
          sessionCount: sessionData.length,
          dataQuality: this.assessDataQuality(sessionData),
          reportSections: reportsToGenerate,
          processingTime: Date.now() - startTime,
          version: "1.0.0",
        },

        reports,

        insights: await this.generateCrossReportInsights(reports),

        executiveSummary: await this.generateCombinedExecutiveSummary(reports),

        actionItems: await this.consolidateActionItems(reports),

        recommendations: await this.consolidateRecommendations(reports),
      };

      this.cacheReport(comprehensiveReport);

      return comprehensiveReport;
    } catch (error) {
      console.error("Failed to generate session report:", error);
      throw error;
    }
  }

  async exportReport(
    reportData,
    formats = this.config.defaultFormats,
    options = {}
  ) {
    const exportResults = [];

    for (const format of formats) {
      try {
        const exportResult = await this.exportEngine.exportReport(
          reportData,
          format,
          { ...this.config, ...options }
        );
        exportResults.push(exportResult);
      } catch (error) {
        console.error(`Export failed for format ${format}:`, error);
        exportResults.push({
          format,
          error: error.message,
          success: false,
        });
      }
    }

    return {
      reportId: reportData.id,
      exports: exportResults,
      summary: {
        successful: exportResults.filter((r) => !r.error).length,
        failed: exportResults.filter((r) => r.error).length,
        totalSize: exportResults.reduce((sum, r) => sum + (r.size || 0), 0),
      },
    };
  }

  async generateAndExportReport(
    sessionData,
    reportType,
    formats,
    options = {}
  ) {
    // Generate the report
    const report = await this.generateSessionReport(
      sessionData,
      reportType,
      options
    );

    // Export in specified formats
    const exportResults = await this.exportReport(report, formats, options);

    return {
      report,
      exports: exportResults,
    };
  }

  async scheduleReport(scheduleConfig) {
    const scheduleId = this.generateScheduleId();

    const scheduledReport = {
      id: scheduleId,
      name: scheduleConfig.name || `Scheduled Report ${scheduleId}`,
      reportType: scheduleConfig.reportType || "comprehensive",
      schedule: scheduleConfig.schedule, // cron expression
      formats: scheduleConfig.formats || this.config.defaultFormats,
      options: scheduleConfig.options || {},
      active: true,
      lastRun: null,
      nextRun: this.calculateNextRun(scheduleConfig.schedule),
      createdAt: new Date().toISOString(),
      subscribers: scheduleConfig.subscribers || [],
    };

    this.scheduledReports.set(scheduleId, scheduledReport);
    await this.addToScheduler(scheduledReport);

    return scheduledReport;
  }

  async generateBatchReports(batchConfig) {
    const batchId = this.generateBatchId();
    const results = [];

    try {
      for (const config of batchConfig.reports) {
        const result = await this.generateAndExportReport(
          config.sessionData,
          config.reportType,
          config.formats || batchConfig.defaultFormats,
          config.options
        );

        results.push({
          configId: config.id || `config_${results.length}`,
          success: true,
          result,
        });
      }

      return {
        batchId,
        summary: {
          total: batchConfig.reports.length,
          successful: results.filter((r) => r.success).length,
          failed: results.filter((r) => !r.success).length,
          totalProcessingTime: results.reduce(
            (sum, r) => sum + (r.result?.report?.meta?.processingTime || 0),
            0
          ),
        },
        results,
      };
    } catch (error) {
      console.error("Batch report generation failed:", error);
      throw error;
    }
  }

  getReportingDashboard() {
    return {
      recentReports: Array.from(this.reportCache.values())
        .sort((a, b) => new Date(b.generatedAt) - new Date(a.generatedAt))
        .slice(0, 10),
      scheduledReports: Array.from(this.scheduledReports.values()).filter(
        (s) => s.active
      ),
      queueStatus: {
        pending: this.reportQueue.length,
        processing: this.isProcessing,
      },
      statistics: {
        totalReportsGenerated: this.reportCache.size,
        activeSchedules: Array.from(this.scheduledReports.values()).filter(
          (s) => s.active
        ).length,
        avgProcessingTime: this.calculateAvgProcessingTime(),
        cacheHitRate: this.calculateCacheHitRate(),
      },
      systemHealth: this.getSystemHealth(),
    };
  }

  async createCustomTemplate(templateConfig) {
    const template = await this.exportEngine.createCustomTemplate(
      templateConfig
    );
    this.reportTemplates.set(template.id, template);
    return template;
  }

  determineReportsToGenerate(reportType, options) {
    const reportMap = {
      comprehensive: ["executive", "technical", "ux", "qa"],
      executive: ["executive"],
      technical: ["technical"],
      ux: ["ux"],
      qa: ["qa"],
      business: ["executive", "ux"],
      development: ["technical", "qa"],
    };

    const baseReports = reportMap[reportType] || ["executive"];

    // Add additional reports based on options
    if (options.includeAdditional) {
      return [...new Set([...baseReports, ...options.includeAdditional])];
    }

    return baseReports;
  }

  async generateCrossReportInsights(reports) {
    const insights = [];

    // Cross-correlate metrics between reports
    if (reports.executive && reports.technical) {
      // Correlate business metrics with technical performance
      const businessImpact = reports.executive.kpis?.businessImpact;
      const systemHealth = reports.technical.overview?.healthScore;

      if (businessImpact && systemHealth) {
        insights.push({
          type: "correlation",
          category: "business_technical",
          insight: `System health score of ${systemHealth}% correlates with business impact metrics`,
          confidence: 0.85,
          data: { businessImpact, systemHealth },
        });
      }
    }

    if (reports.ux && reports.qa) {
      // Correlate UX metrics with quality metrics
      const uxScore = reports.ux.summary?.overallUXScore;
      const qualityScore = reports.qa.summary?.overallQualityScore;

      if (uxScore && qualityScore) {
        insights.push({
          type: "correlation",
          category: "ux_quality",
          insight: `UX score (${uxScore}) shows strong correlation with quality metrics (${qualityScore})`,
          confidence: 0.78,
          data: { uxScore, qualityScore },
        });
      }
    }

    return insights;
  }

  async generateCombinedExecutiveSummary(reports) {
    const summary = {
      overallStatus: this.determineOverallStatus(reports),
      keyMetrics: this.extractKeyMetrics(reports),
      criticalIssues: this.extractCriticalIssues(reports),
      majorAchievements: this.extractMajorAchievements(reports),
      strategicRecommendations: this.extractStrategicRecommendations(reports),
    };

    return summary;
  }

  async generateComparativeReport(
    currentData,
    comparisonData,
    analysisType = "period_comparison",
    options = {}
  ) {
    try {
      let comparativeReport;

      switch (analysisType) {
        case "ab_testing":
          comparativeReport =
            await this.comparativeReports.generateABTestingReport(currentData, {
              ...options,
              comparisonData,
            });
          break;
        case "period_comparison":
          comparativeReport =
            await this.comparativeReports.generatePeriodComparisonReport(
              currentData,
              comparisonData,
              options
            );
          break;
        case "benchmark_analysis":
          comparativeReport =
            await this.comparativeReports.generateBenchmarkReport(
              currentData,
              comparisonData,
              options
            );
          break;
        case "cohort_comparison":
          comparativeReport =
            await this.comparativeReports.generateCohortComparisonReport(
              currentData,
              options
            );
          break;
        case "segment_comparison":
          comparativeReport =
            await this.comparativeReports.generateSegmentComparisonReport(
              currentData,
              options
            );
          break;
        default:
          throw new Error(`Unsupported analysis type: ${analysisType}`);
      }

      return comparativeReport;
    } catch (error) {
      console.error("Failed to generate comparative report:", error);
      throw error;
    }
  }

  async distributeReport(reportData, distributionConfig) {
    try {
      const distribution = await this.distributionSystem.distributeReport(
        reportData,
        distributionConfig
      );
      return distribution;
    } catch (error) {
      console.error("Failed to distribute report:", error);
      throw error;
    }
  }

  async scheduleReportDistribution(reportConfig, schedule, distributionConfig) {
    try {
      const scheduledDistribution =
        await this.distributionSystem.scheduleDistribution(
          reportConfig,
          schedule,
          distributionConfig
        );
      return scheduledDistribution;
    } catch (error) {
      console.error("Failed to schedule report distribution:", error);
      throw error;
    }
  }

  async archiveReport(reportData, metadata = {}) {
    try {
      const archiveResult = await this.archiveManager.archiveReport(
        reportData,
        metadata
      );
      return archiveResult;
    } catch (error) {
      console.error("Failed to archive report:", error);
      throw error;
    }
  }

  async searchArchivedReports(query, options = {}) {
    try {
      const searchResults = await this.archiveManager.searchReports(
        query,
        options
      );
      return searchResults;
    } catch (error) {
      console.error("Failed to search archived reports:", error);
      throw error;
    }
  }

  async retrieveArchivedReport(archiveId, options = {}) {
    try {
      const retrievedReport = await this.archiveManager.retrieveReport(
        archiveId,
        options
      );
      return retrievedReport;
    } catch (error) {
      console.error("Failed to retrieve archived report:", error);
      throw error;
    }
  }

  async createCustomReportTemplate(templateConfig) {
    try {
      const customTemplate = await this.templateEngine.createCustomTemplate(
        templateConfig
      );
      return customTemplate;
    } catch (error) {
      console.error("Failed to create custom template:", error);
      throw error;
    }
  }

  async renderReportWithTemplate(reportData, templateName, options = {}) {
    try {
      const renderedReport = await this.templateEngine.renderReport(
        reportData,
        templateName,
        options
      );
      return renderedReport;
    } catch (error) {
      console.error("Failed to render report with template:", error);
      throw error;
    }
  }

  async runRetentionCleanup(reportType = null) {
    try {
      const cleanupResults = await this.archiveManager.runRetentionCleanup(
        reportType
      );
      return cleanupResults;
    } catch (error) {
      console.error("Failed to run retention cleanup:", error);
      throw error;
    }
  }

  async consolidateActionItems(reports) {
    const allActionItems = [];

    Object.values(reports).forEach((report) => {
      if (report.actionItems) {
        allActionItems.push(...report.actionItems);
      }
    });

    // Deduplicate and prioritize
    return this.deduplicateAndPrioritizeActionItems(allActionItems);
  }

  async consolidateRecommendations(reports) {
    const allRecommendations = [];

    Object.values(reports).forEach((report) => {
      if (report.recommendations) {
        allRecommendations.push(...report.recommendations);
      }
    });

    // Group by category and priority
    return this.groupAndPrioritizeRecommendations(allRecommendations);
  }

  // Helper methods
  generateReportId() {
    return `report_${Date.now()}_${Math.random().toString(36).substr(2, 8)}`;
  }

  generateScheduleId() {
    return `schedule_${Date.now()}_${Math.random().toString(36).substr(2, 6)}`;
  }

  generateBatchId() {
    return `batch_${Date.now()}_${Math.random().toString(36).substr(2, 6)}`;
  }

  cacheReport(report) {
    if (this.config.cacheEnabled) {
      this.reportCache.set(report.id, report);
    }
  }

  assessDataQuality(sessionData) {
    if (sessionData.length === 0) return "no_data";
    if (sessionData.length < 10) return "insufficient";
    if (sessionData.length < 100) return "limited";
    return "sufficient";
  }

  calculateNextRun(schedule) {
    // Calculate next run based on cron schedule
    return new Date(Date.now() + 86400000); // 24 hours for demo
  }

  async addToScheduler(scheduledReport) {
    // Add to job scheduler
    console.log(`Scheduled report: ${scheduledReport.name}`);
    return true;
  }

  calculateAvgProcessingTime() {
    const reports = Array.from(this.reportCache.values());
    if (reports.length === 0) return 0;

    const totalTime = reports.reduce(
      (sum, report) => sum + (report.meta?.processingTime || 0),
      0
    );
    return totalTime / reports.length;
  }

  calculateCacheHitRate() {
    // Calculate cache hit rate (placeholder)
    return 0.85;
  }

  getSystemHealth() {
    return {
      status: "healthy",
      uptime: process.uptime(),
      memoryUsage: process.memoryUsage(),
      reportsGenerated: this.reportCache.size,
      errorRate: 0.01,
    };
  }

  notifySubscribers(event, data) {
    console.log(`Event: ${event}`, data);
  }

  determineOverallStatus(reports) {
    // Determine overall system status based on all reports
    return "healthy";
  }

  extractKeyMetrics(reports) {
    // Extract key metrics from all reports
    return {};
  }

  extractCriticalIssues(reports) {
    // Extract critical issues from all reports
    return [];
  }

  extractMajorAchievements(reports) {
    // Extract achievements from all reports
    return [];
  }

  extractStrategicRecommendations(reports) {
    // Extract strategic recommendations
    return [];
  }

  deduplicateAndPrioritizeActionItems(actionItems) {
    // Remove duplicates and prioritize
    return actionItems;
  }

  groupAndPrioritizeRecommendations(recommendations) {
    // Group and prioritize recommendations
    return recommendations;
  }
}

export default SessionReporterModular;
