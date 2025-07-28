/**
 * Template Engine for Session Reporting
 * Provides customizable report templates with branding and layout options
 *
 * Features:
 * - Multiple template engines (Handlebars, Mustache, EJS)
 * - Custom branding and styling
 * - Dynamic layout generation
 * - Component-based templates
 * - Theme management
 */

export class TemplateEngine {
  constructor(options = {}) {
    this.config = {
      defaultEngine: options.defaultEngine || "handlebars",
      supportedEngines: options.supportedEngines || [
        "handlebars",
        "mustache",
        "ejs",
      ],
      templatesPath: options.templatesPath || "./templates",
      cachingEnabled: options.cachingEnabled !== false,
      compressionEnabled: options.compressionEnabled !== false,
      branding: options.branding || {
        logo: null,
        companyName: "Huntmaster Engine",
        colors: {
          primary: "#007bff",
          secondary: "#6c757d",
          success: "#28a745",
          warning: "#ffc107",
          danger: "#dc3545",
        },
        fonts: {
          primary: "Arial, sans-serif",
          secondary: "Georgia, serif",
          monospace: "Courier New, monospace",
        },
      },
      ...options,
    };

    this.engines = new Map();
    this.templates = new Map();
    this.partials = new Map();
    this.helpers = new Map();
    this.themes = new Map();
    this.templateCache = new Map();

    this.initializeEngines();
    this.loadBaseTemplates();
    this.registerHelpers();
    this.loadThemes();
  }

  initializeEngines() {
    // Handlebars engine
    this.engines.set("handlebars", {
      name: "Handlebars",
      compile: this.compileHandlebars.bind(this),
      supports: ["partials", "helpers", "layouts"],
      fileExtension: ".hbs",
    });

    // Mustache engine
    this.engines.set("mustache", {
      name: "Mustache",
      compile: this.compileMustache.bind(this),
      supports: ["partials"],
      fileExtension: ".mustache",
    });

    // EJS engine
    this.engines.set("ejs", {
      name: "EJS",
      compile: this.compileEJS.bind(this),
      supports: ["includes", "functions"],
      fileExtension: ".ejs",
    });

    // Plain text engine
    this.engines.set("text", {
      name: "Plain Text",
      compile: this.compileText.bind(this),
      supports: ["variables"],
      fileExtension: ".txt",
    });
  }

  loadBaseTemplates() {
    // Executive report template
    this.templates.set("executive_report", {
      name: "Executive Report",
      description:
        "High-level executive summary with KPIs and business insights",
      engine: "handlebars",
      layout: "executive_layout",
      sections: ["header", "summary", "kpis", "trends", "recommendations"],
      styling: "executive_theme",
      template: `
        {{> report_header}}

        <div class="executive-summary">
          <h1>Executive Summary</h1>
          <p class="report-date">{{formatDate generatedAt}}</p>

          {{#if summary}}
          <section class="overview">
            <h2>Overview</h2>
            <p>{{summary.overview}}</p>
          </section>
          {{/if}}

          {{#if kpis}}
          <section class="kpis">
            <h2>Key Performance Indicators</h2>
            <div class="kpi-grid">
              {{#each kpis}}
              <div class="kpi-card {{status}}">
                <h3>{{name}}</h3>
                <div class="kpi-value">{{formatNumber value}}</div>
                <div class="kpi-change {{trendClass trend}}">
                  {{formatPercentage change}}
                </div>
              </div>
              {{/each}}
            </div>
          </section>
          {{/if}}

          {{#if recommendations}}
          <section class="recommendations">
            <h2>Strategic Recommendations</h2>
            <ul>
              {{#each recommendations}}
              <li class="recommendation {{priority}}">
                <strong>{{title}}</strong>: {{description}}
              </li>
              {{/each}}
            </ul>
          </section>
          {{/if}}
        </div>

        {{> report_footer}}
      `,
    });

    // Technical report template
    this.templates.set("technical_report", {
      name: "Technical Report",
      description: "Detailed technical analysis for development teams",
      engine: "handlebars",
      layout: "technical_layout",
      sections: [
        "header",
        "system_metrics",
        "performance",
        "errors",
        "recommendations",
      ],
      styling: "technical_theme",
      template: `
        {{> report_header}}

        <div class="technical-report">
          <h1>Technical Analysis Report</h1>
          <p class="report-date">{{formatDate generatedAt}}</p>

          {{#if systemMetrics}}
          <section class="system-metrics">
            <h2>System Metrics</h2>
            <div class="metrics-grid">
              {{#each systemMetrics}}
              <div class="metric-card">
                <h3>{{name}}</h3>
                <div class="metric-value">{{formatTechnicalValue value unit}}</div>
                <div class="metric-status {{healthStatus}}">{{healthStatus}}</div>
              </div>
              {{/each}}
            </div>
          </section>
          {{/if}}

          {{#if performance}}
          <section class="performance-analysis">
            <h2>Performance Analysis</h2>
            {{#each performance}}
            <div class="performance-section">
              <h3>{{category}}</h3>
              <p>{{analysis}}</p>
              {{#if bottlenecks}}
              <ul class="bottlenecks">
                {{#each bottlenecks}}
                <li class="bottleneck {{severity}}">{{description}}</li>
                {{/each}}
              </ul>
              {{/if}}
            </div>
            {{/each}}
          </section>
          {{/if}}

          {{#if errors}}
          <section class="error-analysis">
            <h2>Error Analysis</h2>
            <div class="error-summary">
              <p>Total Errors: <strong>{{errors.total}}</strong></p>
              <p>Error Rate: <strong>{{formatPercentage errors.rate}}</strong></p>
            </div>

            {{#if errors.categories}}
            <div class="error-categories">
              {{#each errors.categories}}
              <div class="error-category">
                <h3>{{name}} ({{count}})</h3>
                {{#if samples}}
                <ul class="error-samples">
                  {{#each samples}}
                  <li class="error-sample">
                    <code>{{message}}</code>
                    <span class="error-frequency">{{frequency}}x</span>
                  </li>
                  {{/each}}
                </ul>
                {{/if}}
              </div>
              {{/each}}
            </div>
            {{/if}}
          </section>
          {{/if}}
        </div>

        {{> report_footer}}
      `,
    });

    // UX report template
    this.templates.set("ux_report", {
      name: "UX Analysis Report",
      description:
        "User experience analysis with journey maps and usability metrics",
      engine: "handlebars",
      layout: "ux_layout",
      sections: [
        "header",
        "journey_analysis",
        "usability_metrics",
        "accessibility",
        "recommendations",
      ],
      styling: "ux_theme",
      template: `
        {{> report_header}}

        <div class="ux-report">
          <h1>User Experience Analysis</h1>
          <p class="report-date">{{formatDate generatedAt}}</p>

          {{#if journeyAnalysis}}
          <section class="journey-analysis">
            <h2>User Journey Analysis</h2>
            <div class="journey-overview">
              <p>Analyzed {{journeyAnalysis.totalSessions}} user sessions</p>
              <p>Average session duration: {{formatDuration journeyAnalysis.avgDuration}}</p>
            </div>

            {{#if journeyAnalysis.commonPaths}}
            <div class="common-paths">
              <h3>Most Common User Paths</h3>
              {{#each journeyAnalysis.commonPaths}}
              <div class="path-item">
                <div class="path-steps">{{join steps ' → '}}</div>
                <div class="path-frequency">{{percentage}}% of users</div>
              </div>
              {{/each}}
            </div>
            {{/if}}
          </section>
          {{/if}}

          {{#if usabilityMetrics}}
          <section class="usability-metrics">
            <h2>Usability Metrics</h2>
            <div class="metrics-grid">
              {{#each usabilityMetrics}}
              <div class="usability-metric">
                <h3>{{name}}</h3>
                <div class="metric-score {{scoreClass score}}">{{score}}/10</div>
                <p class="metric-description">{{description}}</p>
              </div>
              {{/each}}
            </div>
          </section>
          {{/if}}

          {{#if accessibility}}
          <section class="accessibility-audit">
            <h2>Accessibility Compliance</h2>
            <div class="compliance-overview">
              <div class="compliance-score">
                <h3>WCAG 2.1 Compliance</h3>
                <div class="score-circle {{accessibility.wcag.level}}">
                  {{accessibility.wcag.score}}%
                </div>
              </div>

              {{#if accessibility.issues}}
              <div class="accessibility-issues">
                <h3>Issues by Severity</h3>
                {{#each accessibility.issues}}
                <div class="issue-category {{severity}}">
                  <h4>{{severity}} ({{count}})</h4>
                  <ul>
                    {{#each items}}
                    <li>{{description}}</li>
                    {{/each}}
                  </ul>
                </div>
                {{/each}}
              </div>
              {{/if}}
            </div>
          </section>
          {{/if}}
        </div>

        {{> report_footer}}
      `,
    });

    // QA report template
    this.templates.set("qa_report", {
      name: "Quality Assurance Report",
      description: "Testing metrics, coverage analysis, and defect tracking",
      engine: "handlebars",
      layout: "qa_layout",
      sections: [
        "header",
        "test_summary",
        "coverage",
        "defects",
        "quality_gates",
      ],
      styling: "qa_theme",
      template: `
        {{> report_header}}

        <div class="qa-report">
          <h1>Quality Assurance Report</h1>
          <p class="report-date">{{formatDate generatedAt}}</p>

          {{#if testSummary}}
          <section class="test-summary">
            <h2>Test Execution Summary</h2>
            <div class="test-overview">
              <div class="test-stat">
                <h3>Total Tests</h3>
                <div class="stat-value">{{testSummary.total}}</div>
              </div>
              <div class="test-stat passed">
                <h3>Passed</h3>
                <div class="stat-value">{{testSummary.passed}}</div>
              </div>
              <div class="test-stat failed">
                <h3>Failed</h3>
                <div class="stat-value">{{testSummary.failed}}</div>
              </div>
              <div class="test-stat skipped">
                <h3>Skipped</h3>
                <div class="stat-value">{{testSummary.skipped}}</div>
              </div>
            </div>

            <div class="pass-rate">
              <h3>Pass Rate</h3>
              <div class="progress-bar">
                <div class="progress-fill" style="width: {{testSummary.passRate}}%"></div>
              </div>
              <span class="progress-text">{{formatPercentage testSummary.passRate}}</span>
            </div>
          </section>
          {{/if}}

          {{#if coverage}}
          <section class="coverage-analysis">
            <h2>Test Coverage</h2>
            <div class="coverage-metrics">
              <div class="coverage-metric">
                <h3>Line Coverage</h3>
                <div class="coverage-percent">{{formatPercentage coverage.line}}</div>
              </div>
              <div class="coverage-metric">
                <h3>Branch Coverage</h3>
                <div class="coverage-percent">{{formatPercentage coverage.branch}}</div>
              </div>
              <div class="coverage-metric">
                <h3>Function Coverage</h3>
                <div class="coverage-percent">{{formatPercentage coverage.function}}</div>
              </div>
            </div>
          </section>
          {{/if}}

          {{#if defects}}
          <section class="defect-analysis">
            <h2>Defect Analysis</h2>
            <div class="defect-summary">
              <p>Total Defects: <strong>{{defects.total}}</strong></p>
              <p>Open Defects: <strong>{{defects.open}}</strong></p>
              <p>Average Resolution Time: <strong>{{formatDuration defects.avgResolutionTime}}</strong></p>
            </div>

            {{#if defects.bySeverity}}
            <div class="defects-by-severity">
              <h3>Defects by Severity</h3>
              {{#each defects.bySeverity}}
              <div class="severity-group {{severity}}">
                <h4>{{severity}} ({{count}})</h4>
                <div class="severity-bar">
                  <div class="bar-fill" style="width: {{percentage}}%"></div>
                </div>
              </div>
              {{/each}}
            </div>
            {{/if}}
          </section>
          {{/if}}
        </div>

        {{> report_footer}}
      `,
    });
  }

  registerHelpers() {
    // Date formatting helper
    this.helpers.set("formatDate", (date) => {
      return new Date(date).toLocaleDateString("en-US", {
        year: "numeric",
        month: "long",
        day: "numeric",
        hour: "2-digit",
        minute: "2-digit",
      });
    });

    // Number formatting helper
    this.helpers.set("formatNumber", (value, options) => {
      if (typeof value !== "number") return value;
      return value.toLocaleString("en-US", options || {});
    });

    // Percentage formatting helper
    this.helpers.set("formatPercentage", (value) => {
      if (typeof value !== "number") return value;
      return `${(value * 100).toFixed(1)}%`;
    });

    // Duration formatting helper
    this.helpers.set("formatDuration", (seconds) => {
      if (typeof seconds !== "number") return seconds;
      const hours = Math.floor(seconds / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      const secs = Math.floor(seconds % 60);

      if (hours > 0) return `${hours}h ${minutes}m ${secs}s`;
      if (minutes > 0) return `${minutes}m ${secs}s`;
      return `${secs}s`;
    });

    // Technical value formatting helper
    this.helpers.set("formatTechnicalValue", (value, unit) => {
      if (typeof value !== "number") return value;

      switch (unit) {
        case "bytes":
          return this.formatBytes(value);
        case "ms":
          return `${value}ms`;
        case "percent":
          return `${value.toFixed(1)}%`;
        default:
          return `${value} ${unit || ""}`;
      }
    });

    // Array join helper
    this.helpers.set("join", (array, separator) => {
      return Array.isArray(array) ? array.join(separator || ", ") : array;
    });

    // Trend class helper
    this.helpers.set("trendClass", (trend) => {
      if (trend > 0) return "trend-up";
      if (trend < 0) return "trend-down";
      return "trend-neutral";
    });

    // Score class helper
    this.helpers.set("scoreClass", (score) => {
      if (score >= 8) return "score-excellent";
      if (score >= 6) return "score-good";
      if (score >= 4) return "score-fair";
      return "score-poor";
    });
  }

  loadThemes() {
    // Executive theme
    this.themes.set("executive_theme", {
      name: "Executive Theme",
      description: "Professional theme for executive reports",
      colors: {
        primary: this.config.branding.colors.primary,
        secondary: this.config.branding.colors.secondary,
        background: "#ffffff",
        text: "#333333",
        accent: "#f8f9fa",
      },
      typography: {
        headingFont: this.config.branding.fonts.primary,
        bodyFont: this.config.branding.fonts.primary,
        codeFont: this.config.branding.fonts.monospace,
      },
      layout: {
        maxWidth: "1200px",
        padding: "2rem",
        borderRadius: "8px",
      },
    });

    // Technical theme
    this.themes.set("technical_theme", {
      name: "Technical Theme",
      description: "Monospace theme for technical reports",
      colors: {
        primary: "#2c3e50",
        secondary: "#34495e",
        background: "#f8f9fa",
        text: "#2c3e50",
        accent: "#ecf0f1",
      },
      typography: {
        headingFont: this.config.branding.fonts.monospace,
        bodyFont: this.config.branding.fonts.primary,
        codeFont: this.config.branding.fonts.monospace,
      },
      layout: {
        maxWidth: "1400px",
        padding: "1.5rem",
        borderRadius: "4px",
      },
    });

    // UX theme
    this.themes.set("ux_theme", {
      name: "UX Theme",
      description: "User-friendly theme for UX reports",
      colors: {
        primary: "#6f42c1",
        secondary: "#6610f2",
        background: "#ffffff",
        text: "#495057",
        accent: "#f8f9fa",
      },
      typography: {
        headingFont: this.config.branding.fonts.primary,
        bodyFont: this.config.branding.fonts.primary,
        codeFont: this.config.branding.fonts.monospace,
      },
      layout: {
        maxWidth: "1100px",
        padding: "2rem",
        borderRadius: "12px",
      },
    });
  }

  async renderReport(reportData, templateName, options = {}) {
    try {
      const template = this.templates.get(templateName);
      if (!template) {
        throw new Error(`Template not found: ${templateName}`);
      }

      const engine = this.engines.get(
        template.engine || this.config.defaultEngine
      );
      if (!engine) {
        throw new Error(`Template engine not found: ${template.engine}`);
      }

      const context = await this.prepareTemplateContext(
        reportData,
        template,
        options
      );

      const themedContext = await this.applyTheme(
        context,
        template.styling,
        options
      );

      const rendered = await engine.compile(template.template, themedContext);

      return {
        html: rendered,
        template: templateName,
        engine: template.engine,
        generatedAt: new Date().toISOString(),
        metadata: {
          reportId: reportData.id,
          reportType: reportData.type,
          theme: template.styling,
        },
      };
    } catch (error) {
      console.error("Error rendering report:", error);
      throw error;
    }
  }

  async createCustomTemplate(templateConfig) {
    const templateId = this.generateTemplateId();

    const customTemplate = {
      id: templateId,
      name: templateConfig.name,
      description: templateConfig.description,
      engine: templateConfig.engine || this.config.defaultEngine,
      layout: templateConfig.layout,
      sections: templateConfig.sections || [],
      styling: templateConfig.styling,
      template: templateConfig.template,
      createdAt: new Date().toISOString(),
      custom: true,
    };

    const validation = await this.validateTemplate(customTemplate);
    if (!validation.isValid) {
      throw new Error(`Invalid template: ${validation.errors.join(", ")}`);
    }

    this.templates.set(templateId, customTemplate);

    return customTemplate;
  }

  async compileHandlebars(template, context) {
    for (const [name, helper] of this.helpers) {
      // Register helper with Handlebars instance
    }

    return template; // Placeholder - would use actual Handlebars compilation
  }

  async compileMustache(template, context) {
    return template; // Placeholder
  }

  async compileEJS(template, context) {
    return template; // Placeholder
  }

  async compileText(template, context) {
    // Simple variable replacement
    let result = template;
    for (const [key, value] of Object.entries(context)) {
      const regex = new RegExp(`{{${key}}}`, "g");
      result = result.replace(regex, String(value));
    }
    return result;
  }

  // Helper methods
  generateTemplateId() {
    return `template_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  formatBytes(bytes) {
    if (bytes === 0) return "0 Bytes";
    const k = 1024;
    const sizes = ["Bytes", "KB", "MB", "GB", "TB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i];
  }

  async prepareTemplateContext(reportData, template, options) {
    return reportData;
  }
  async applyTheme(context, themeName, options) {
    return context;
  }
  async validateTemplate(template) {
    return { isValid: true, errors: [] };
  }
}

export default TemplateEngine;
