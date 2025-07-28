/**
 * Privacy Compliance Module for Session Recording
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This module provides comprehensive privacy compliance for session recording,
 * including GDPR, CCPA compliance, data filtering, anonymization,
 * and user consent management.
 *
 * @fileoverview Privacy compliance and data filtering for session recording
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires DataValidator - For privacy data validation
 * @requires DataEncryption - For sensitive data protection
 */

import { DataValidator } from "../validation/data-validator.js";
import { DataEncryption } from "./data-encryption.js";

/**
 * PrivacyCompliance class for comprehensive privacy protection during session recording
 * Provides GDPR/CCPA compliance, data filtering, anonymization, and consent management
 */
class PrivacyCompliance {
  constructor(options = {}) {
    this.config = {
      gdprEnabled: options.gdprEnabled !== false,
      ccpaEnabled: options.ccpaEnabled !== false,
      anonymizationEnabled: options.anonymizationEnabled !== false,
      consentRequired: options.consentRequired !== false,
      dataRetentionDays: options.dataRetentionDays || 365,
      allowDataExport: options.allowDataExport !== false,
      allowDataDeletion: options.allowDataDeletion !== false,
      logComplianceEvents: options.logComplianceEvents !== false,
      strictMode: options.strictMode || false,
      minimumAge: options.minimumAge || 13,
      ...options,
    };

    this.validator = new DataValidator();
    this.encryption = new DataEncryption();

    this.consentStatus = new Map(); // userId -> consent details
    this.dataSubjects = new Map(); // userId -> subject rights data
    this.complianceLog = [];

    this.sensitiveDataPatterns = {
      email: /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b/g,
      phone: /(\+\d{1,2}\s?)?\(?\d{3}\)?[\s.-]?\d{3}[\s.-]?\d{4}/g,
      ssn: /\b\d{3}-?\d{2}-?\d{4}\b/g,
      creditCard: /\b\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4}\b/g,
      ipAddress: /\b(?:\d{1,3}\.){3}\d{1,3}\b/g,
      personalName: /\b[A-Z][a-z]+ [A-Z][a-z]+\b/g,
    };

    this.blockedSelectors = [
      'input[type="password"]',
      'input[type="email"]',
      'input[name*="password"]',
      'input[name*="email"]',
      'input[name*="ssn"]',
      'input[name*="credit"]',
      ".sensitive-data",
      ".private-content",
      "[data-private]",
    ];

    this.complianceFrameworks = {
      gdpr: {
        lawfulBases: [
          "consent",
          "legitimate_interest",
          "contract",
          "legal_obligation",
        ],
        dataSubjectRights: [
          "access",
          "rectification",
          "erasure",
          "portability",
          "restriction",
          "objection",
        ],
        retentionPolicies: new Map(),
      },
      ccpa: {
        consumerRights: ["know", "delete", "opt_out", "non_discrimination"],
        categories: [
          "identifiers",
          "personal_info",
          "protected_class",
          "commercial_info",
        ],
      },
    };

    this.initializePrivacyCompliance();
  }

  /**
   * Initialize privacy compliance system
   * Set up privacy protection mechanisms and compliance monitoring
   */
  initializePrivacyCompliance() {
    try {
      this.loadConsentRecords();

      this.initializeDataSubjectRegistry();

      this.setupComplianceMonitoring();

      this.setupPrivacyEventHandlers();

      console.log("PrivacyCompliance: Initialized successfully");
    } catch (error) {
      console.error("PrivacyCompliance: Initialization failed:", error);
    }
  }

  /**
   * Load existing consent records from storage
   * Retrieve and validate stored consent information
   */
  loadConsentRecords() {
    try {
      const storedConsent = localStorage.getItem("huntmaster_consent_records");
      if (storedConsent) {
        const consentData = JSON.parse(storedConsent);

        Object.entries(consentData).forEach(([userId, consent]) => {
          if (this.validateConsentRecord(consent)) {
            this.consentStatus.set(userId, consent);
          }
        });
      }

      console.log(
        `PrivacyCompliance: Loaded ${this.consentStatus.size} consent records`
      );
    } catch (error) {
      console.error(
        "PrivacyCompliance: Failed to load consent records:",
        error
      );
    }
  }

  /**
   * Initialize data subject registry for rights management
   * Set up registry for tracking data subject rights and requests
   */
  initializeDataSubjectRegistry() {
    try {
      const storedSubjects = localStorage.getItem("huntmaster_data_subjects");
      if (storedSubjects) {
        const subjectData = JSON.parse(storedSubjects);

        Object.entries(subjectData).forEach(([userId, subject]) => {
          this.dataSubjects.set(userId, subject);
        });
      }

      console.log(
        `PrivacyCompliance: Loaded ${this.dataSubjects.size} data subject records`
      );
    } catch (error) {
      console.error(
        "PrivacyCompliance: Failed to initialize data subject registry:",
        error
      );
    }
  }

  /**
   * Set up compliance monitoring and logging
   * Initialize monitoring for compliance violations and events
   */
  setupComplianceMonitoring() {
    if (this.config.logComplianceEvents) {
      this.startComplianceLogging();
    }

    setInterval(() => {
      this.performComplianceAudit();
    }, 60000); // Check every minute

    console.log("PrivacyCompliance: Compliance monitoring initialized");
  }

  /**
   * Set up privacy-aware event handlers
   * Initialize event handlers that respect privacy settings
   */
  setupPrivacyEventHandlers() {
    document.addEventListener(
      "input",
      (event) => {
        if (this.shouldBlockElement(event.target)) {
          event.stopPropagation();
          this.logComplianceEvent("blocked_sensitive_input", {
            element: event.target.tagName,
            reason: "sensitive_data_protection",
          });
        }
      },
      true
    );

    document.addEventListener("copy", (event) => {
      if (this.containsSensitiveData(event.clipboardData?.getData("text"))) {
        this.logComplianceEvent("sensitive_data_copy", {
          reason: "clipboard_contains_sensitive_data",
        });
      }
    });

    console.log("PrivacyCompliance: Privacy event handlers initialized");
  }

  /**
   * Check if user has provided valid consent
   * Validate consent status for data processing
   */
  hasValidConsent(userId, purpose = "session_recording") {
    try {
      const consent = this.consentStatus.get(userId);
      if (!consent) {
        return false;
      }

      const now = Date.now();
      if (consent.expiresAt && now > consent.expiresAt) {
        return false;
      }

      if (!consent.purposes.includes(purpose)) {
        return false;
      }

      if (consent.withdrawn) {
        return false;
      }

      return true;
    } catch (error) {
      console.error("PrivacyCompliance: Consent validation failed:", error);
      return false;
    }
  }

  /**
   * Record user consent for data processing
   * Store and validate user consent information
   */
  recordConsent(userId, consentDetails) {
    try {
      const validatedConsent = this.validateConsentRecord(consentDetails);
      if (!validatedConsent) {
        throw new Error("Invalid consent details");
      }

      const consentRecord = {
        userId,
        timestamp: Date.now(),
        purposes: consentDetails.purposes || ["session_recording"],
        lawfulBasis: consentDetails.lawfulBasis || "consent",
        consentMethod: consentDetails.method || "web_form",
        ipAddress: this.anonymizeIP(consentDetails.ipAddress),
        userAgent: consentDetails.userAgent,
        expiresAt: consentDetails.expiresAt,
        withdrawn: false,
        version: "1.0",
      };

      this.consentStatus.set(userId, consentRecord);
      this.saveConsentRecords();

      this.logComplianceEvent("consent_recorded", {
        userId,
        purposes: consentRecord.purposes,
        lawfulBasis: consentRecord.lawfulBasis,
      });

      console.log(`PrivacyCompliance: Consent recorded for user ${userId}`);
      return true;
    } catch (error) {
      console.error("PrivacyCompliance: Failed to record consent:", error);
      return false;
    }
  }

  /**
   * Withdraw user consent
   * Process consent withdrawal and stop data processing
   */
  withdrawConsent(userId, reason = "user_request") {
    try {
      const consent = this.consentStatus.get(userId);
      if (!consent) {
        console.warn(
          `PrivacyCompliance: No consent record found for user ${userId}`
        );
        return false;
      }

      consent.withdrawn = true;
      consent.withdrawnAt = Date.now();
      consent.withdrawalReason = reason;

      this.consentStatus.set(userId, consent);
      this.saveConsentRecords();

      this.stopDataProcessing(userId);

      this.logComplianceEvent("consent_withdrawn", {
        userId,
        reason,
        timestamp: consent.withdrawnAt,
      });

      console.log(`PrivacyCompliance: Consent withdrawn for user ${userId}`);
      return true;
    } catch (error) {
      console.error("PrivacyCompliance: Failed to withdraw consent:", error);
      return false;
    }
  }

  /**
   * Filter data to remove or anonymize sensitive information
   * Apply privacy filters and anonymization to data
   */
  filterData(data) {
    try {
      let filteredData = JSON.parse(JSON.stringify(data));

      if (this.config.anonymizationEnabled) {
        filteredData = this.anonymizeData(filteredData);
      }

      filteredData = this.removeSensitiveFields(filteredData);

      filteredData = this.applyDataMinimization(filteredData);

      return this.validator.validate(filteredData);
    } catch (error) {
      console.error("PrivacyCompliance: Data filtering failed:", error);
      return data;
    }
  }

  /**
   * Anonymize personally identifiable information
   * Apply anonymization techniques to protect privacy
   */
  anonymizeData(data) {
    try {
      let dataStr = JSON.stringify(data);

      Object.entries(this.sensitiveDataPatterns).forEach(([type, pattern]) => {
        dataStr = dataStr.replace(pattern, (match) => {
          return this.anonymizeValue(match, type);
        });
      });

      return JSON.parse(dataStr);
    } catch (error) {
      console.error("PrivacyCompliance: Data anonymization failed:", error);
      return data;
    }
  }

  /**
   * Anonymize a specific value based on its type
   * Apply type-specific anonymization techniques
   */
  anonymizeValue(value, type) {
    switch (type) {
      case "email":
        const emailParts = value.split("@");
        return `${emailParts[0].substr(0, 2)}***@${emailParts[1]}`;

      case "phone":
        return value.replace(/\d/g, (digit, index) =>
          index < 3 || index > value.length - 4 ? digit : "*"
        );

      case "ssn":
        return value.replace(/\d/g, "*");

      case "creditCard":
        return value.replace(/\d(?=\d{4})/g, "*");

      case "ipAddress":
        return this.anonymizeIP(value);

      case "personalName":
        return value.replace(
          /\b[A-Z][a-z]+/g,
          (name) => name.charAt(0) + "*".repeat(name.length - 1)
        );

      default:
        return "*".repeat(value.length);
    }
  }

  /**
   * Anonymize IP address for privacy protection
   * Apply IP anonymization while preserving analytical value
   */
  anonymizeIP(ip) {
    if (!ip) return ip;

    try {
      if (ip.includes(".")) {
        const parts = ip.split(".");
        if (parts.length === 4) {
          return `${parts[0]}.${parts[1]}.${parts[2]}.0`;
        }
      }

      if (ip.includes(":")) {
        const parts = ip.split(":");
        return parts.slice(0, Math.ceil(parts.length / 2)).join(":") + "::";
      }

      return ip;
    } catch (error) {
      console.error("PrivacyCompliance: IP anonymization failed:", error);
      return ip;
    }
  }

  /**
   * Remove sensitive fields from data objects
   * TODO: Strip out predefined sensitive data fields
   */
  removeSensitiveFields(data) {
    const sensitiveFields = [
      "password",
      "creditCard",
      "ssn",
      "pin",
      "cvv",
      "bankAccount",
      "routingNumber",
      "socialSecurity",
      "driversLicense",
      "passport",
    ];

    const removeSensitive = (obj) => {
      if (Array.isArray(obj)) {
        return obj.map(removeSensitive);
      } else if (obj && typeof obj === "object") {
        const cleaned = {};
        Object.keys(obj).forEach((key) => {
          const lowerKey = key.toLowerCase();
          if (!sensitiveFields.some((field) => lowerKey.includes(field))) {
            cleaned[key] = removeSensitive(obj[key]);
          }
        });
        return cleaned;
      }
      return obj;
    };

    return removeSensitive(data);
  }

  /**
   * Apply data minimization principles
   * Remove unnecessary data to minimize privacy exposure
   */
  applyDataMinimization(data) {
    const essentialFields = [
      "timestamp",
      "eventType",
      "elementType",
      "sessionId",
      "userAction",
      "pageUrl",
      "performance",
      "errors",
    ];

    if (this.config.strictMode) {
      return this.keepOnlyEssentialFields(data, essentialFields);
    }

    return data;
  }

  /**
   * Check if an element should be blocked from recording
   * Determine if element contains sensitive data
   */
  shouldBlockElement(element) {
    if (!element) return false;

    try {
      for (const selector of this.blockedSelectors) {
        if (element.matches && element.matches(selector)) {
          return true;
        }
      }

      const sensitiveAttributes = [
        "data-sensitive",
        "data-private",
        "data-personal",
      ];
      for (const attr of sensitiveAttributes) {
        if (element.hasAttribute && element.hasAttribute(attr)) {
          return true;
        }
      }

      const content = element.value || element.textContent || "";
      if (this.containsSensitiveData(content)) {
        return true;
      }

      return false;
    } catch (error) {
      console.error("PrivacyCompliance: Element blocking check failed:", error);
      return false;
    }
  }

  /**
   * Check if text contains sensitive data patterns
   * TODO: Scan text for sensitive information patterns
   */
  containsSensitiveData(text) {
    if (!text || typeof text !== "string") return false;

    for (const pattern of Object.values(this.sensitiveDataPatterns)) {
      if (pattern.test(text)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Handle data subject rights requests (GDPR/CCPA)
   * Process data subject rights requests
   */
  async handleDataSubjectRequest(userId, requestType, details = {}) {
    try {
      const validRequestTypes = [
        "access",
        "rectification",
        "erasure",
        "portability",
        "restriction",
      ];
      if (!validRequestTypes.includes(requestType)) {
        throw new Error(`Invalid request type: ${requestType}`);
      }

      const request = {
        requestId: `req_${Date.now()}_${Math.random()
          .toString(36)
          .substr(2, 9)}`,
        userId,
        requestType,
        timestamp: Date.now(),
        status: "received",
        details,
        response: null,
      };

      let response;
      switch (requestType) {
        case "access":
          response = await this.processAccessRequest(userId);
          break;
        case "erasure":
          response = await this.processErasureRequest(userId);
          break;
        case "portability":
          response = await this.processPortabilityRequest(userId);
          break;
        default:
          response = { message: "Request type not yet implemented" };
      }

      request.status = "completed";
      request.response = response;
      request.completedAt = Date.now();

      this.logComplianceEvent("data_subject_request", {
        requestId: request.requestId,
        userId,
        requestType,
        status: request.status,
      });

      return request;
    } catch (error) {
      console.error("PrivacyCompliance: Data subject request failed:", error);
      return null;
    }
  }

  /**
   * Log compliance events for audit trail
   * Create comprehensive compliance audit log
   */
  logComplianceEvent(eventType, details) {
    if (!this.config.logComplianceEvents) return;

    try {
      const logEntry = {
        timestamp: Date.now(),
        eventType,
        details,
        sessionId: this.getCurrentSessionId(),
        userAgent: navigator.userAgent,
        url: window.location.href,
      };

      this.complianceLog.push(logEntry);

      if (this.complianceLog.length > 1000) {
        this.complianceLog = this.complianceLog.slice(-500);
      }

      this.saveComplianceLog();

      console.log(`PrivacyCompliance: Logged event ${eventType}`);
    } catch (error) {
      console.error(
        "PrivacyCompliance: Failed to log compliance event:",
        error
      );
    }
  }

  /**
   * Perform automated compliance audit
   * Check for compliance violations and issues
   */
  performComplianceAudit() {
    try {
      const auditResults = {
        timestamp: Date.now(),
        consentRecords: this.consentStatus.size,
        expiredConsents: 0,
        withdrawnConsents: 0,
        dataSubjects: this.dataSubjects.size,
        issues: [],
      };

      for (const [userId, consent] of this.consentStatus.entries()) {
        if (consent.expiresAt && Date.now() > consent.expiresAt) {
          auditResults.expiredConsents++;
          auditResults.issues.push({
            type: "expired_consent",
            userId,
            details: "Consent has expired",
          });
        }

        if (consent.withdrawn) {
          auditResults.withdrawnConsents++;
        }
      }

      this.logComplianceEvent("compliance_audit", auditResults);

      return auditResults;
    } catch (error) {
      console.error("PrivacyCompliance: Compliance audit failed:", error);
      return null;
    }
  }

  /**
   * Save consent records to storage
   * Persist consent records for compliance
   */
  saveConsentRecords() {
    try {
      const consentData = Object.fromEntries(this.consentStatus);
      localStorage.setItem(
        "huntmaster_consent_records",
        JSON.stringify(consentData)
      );
    } catch (error) {
      console.error(
        "PrivacyCompliance: Failed to save consent records:",
        error
      );
    }
  }

  /**
   * Save compliance log to storage
   * Persist compliance events for audit trail
   */
  saveComplianceLog() {
    try {
      localStorage.setItem(
        "huntmaster_compliance_log",
        JSON.stringify(this.complianceLog)
      );
    } catch (error) {
      console.error("PrivacyCompliance: Failed to save compliance log:", error);
    }
  }

  /**
   * Cleanup and destroy privacy compliance system
   * Clean up privacy protection and save final state
   */
  destroy() {
    try {
      this.saveConsentRecords();
      this.saveComplianceLog();

      this.consentStatus.clear();
      this.dataSubjects.clear();
      this.complianceLog = [];

      console.log("PrivacyCompliance: Destroyed successfully");
    } catch (error) {
      console.error("PrivacyCompliance: Destruction failed:", error);
    }
  }
}

export { PrivacyCompliance };

export const createPrivacyCompliance = (options) =>
  new PrivacyCompliance(options);
export const checkGDPRCompliance = (data, userId) => {
  const privacy = new PrivacyCompliance({ gdprEnabled: true });
  return privacy.hasValidConsent(userId) ? privacy.filterData(data) : null;
};

export const PrivacyUtils = {
  anonymizeEmail: (email) => {
    const parts = email.split("@");
    return `${parts[0].substr(0, 2)}***@${parts[1]}`;
  },

  anonymizePhone: (phone) => {
    return phone.replace(/\d(?=\d{4})/g, "*");
  },

  isValidConsent: (consent) => {
    return (
      consent &&
      consent.timestamp &&
      consent.purposes &&
      Array.isArray(consent.purposes) &&
      !consent.withdrawn
    );
  },

  generateConsentId: () => {
    return `consent_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  },
};

console.log("PrivacyCompliance module loaded successfully");
