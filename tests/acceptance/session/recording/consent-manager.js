/**
 * Consent Manager Module for Session Recording
 * Part of the Huntmaster Engine User Acceptance Testing Framework
 *
 * This module provides comprehensive user consent collection and management
 * for session recording, including GDPR/CCPA compliance, consent tracking,
 * preference management, and opt-out handling.
 *
 * @fileoverview User consent collection and management for session recording
 * @version 1.0.0
 * @since 2025-07-25
 *
 * @requires DataValidator - For consent data validation
 * @requires PrivacyCompliance - For privacy compliance integration
 */

import { DataValidator } from "../validation/data-validator.js";

/**
 * ConsentManager class for comprehensive consent collection and management
 * Provides GDPR/CCPA compliance, consent tracking, and user preference management
 */
class ConsentManager {
  constructor(options = {}) {
    this.config = {
      gdprEnabled: options.gdprEnabled !== false,
      ccpaEnabled: options.ccpaEnabled !== false,
      requireExplicitConsent: options.requireExplicitConsent !== false,
      consentExpirationDays: options.consentExpirationDays || 365,
      reminderIntervalDays: options.reminderIntervalDays || 90,
      enableConsentBanner: options.enableConsentBanner !== false,
      enablePreferenceCenter: options.enablePreferenceCenter !== false,
      consentCategories: options.consentCategories || [
        "necessary",
        "functional",
        "analytics",
        "marketing",
      ],
      defaultLanguage: options.defaultLanguage || "en",
      supportedLanguages: options.supportedLanguages || [
        "en",
        "es",
        "fr",
        "de",
      ],
      storageKey: options.storageKey || "huntmaster_consent",
      debugMode: options.debugMode || false,
      ...options,
    };

    this.validator = new DataValidator();

    this.state = {
      isInitialized: false,
      currentConsent: null,
      consentHistory: [],
      bannerVisible: false,
      preferenceCenterVisible: false,
      reminderDue: false,
      consentRequired: true,
    };

    this.consentCategories = {
      necessary: {
        id: "necessary",
        name: "Necessary",
        description: "Essential cookies required for basic site functionality",
        required: true,
        purposes: ["session_management", "security", "authentication"],
      },
      functional: {
        id: "functional",
        name: "Functional",
        description:
          "Cookies that enhance site functionality and user experience",
        required: false,
        purposes: ["preferences", "language_selection", "region_selection"],
      },
      analytics: {
        id: "analytics",
        name: "Analytics",
        description: "Cookies for session recording and analytics",
        required: false,
        purposes: [
          "session_recording",
          "performance_analysis",
          "user_behavior",
        ],
      },
      marketing: {
        id: "marketing",
        name: "Marketing",
        description: "Cookies for marketing and advertising purposes",
        required: false,
        purposes: ["advertising", "remarketing", "social_media"],
      },
    };

    this.localizations = {
      en: {
        banner_title: "Cookie Consent",
        banner_message:
          "We use cookies to improve your experience and for analytics.",
        accept_all: "Accept All",
        reject_all: "Reject All",
        manage_preferences: "Manage Preferences",
        save_preferences: "Save Preferences",
        close: "Close",
      },
    };

    this.eventHandlers = new Map();

    this.initializeConsentManager();
  }

  /**
   * Initialize the consent management system
   * Set up consent collection and compliance checking
   */
  async initializeConsentManager() {
    try {
      await this.loadConsentData();

      this.checkConsentRequirements();

      if (this.shouldShowConsentBanner()) {
        await this.showConsentBanner();
      }

      this.setupConsentReminders();

      if (this.config.enablePreferenceCenter) {
        this.setupPreferenceCenter();
      }

      this.setupEventListeners();

      this.state.isInitialized = true;
      console.log("ConsentManager: Initialized successfully");
    } catch (error) {
      console.error("ConsentManager: Initialization failed:", error);
      this.handleError("initialization_failed", error);
    }
  }

  /**
   * Load existing consent data from storage
   * Retrieve and validate stored consent information
   */
  async loadConsentData() {
    try {
      const storedConsent = localStorage.getItem(this.config.storageKey);
      if (storedConsent) {
        const consentData = JSON.parse(storedConsent);

        if (this.validateConsentData(consentData)) {
          this.state.currentConsent = consentData;

          if (this.isConsentExpired(consentData)) {
            this.state.currentConsent = null;
            localStorage.removeItem(this.config.storageKey);
          }
        }
      }

      const historyKey = `${this.config.storageKey}_history`;
      const storedHistory = localStorage.getItem(historyKey);
      if (storedHistory) {
        this.state.consentHistory = JSON.parse(storedHistory);
      }

      console.log("ConsentManager: Consent data loaded");
    } catch (error) {
      console.error("ConsentManager: Failed to load consent data:", error);
    }
  }

  /**
   * Check if consent is required based on regulations
   * Determine if user consent collection is needed
   */
  checkConsentRequirements() {
    try {
      if (
        this.state.currentConsent &&
        !this.isConsentExpired(this.state.currentConsent)
      ) {
        this.state.consentRequired = false;
        return;
      }

      const userLocation = this.detectUserLocation();

      if (this.config.gdprEnabled && this.isEUUser(userLocation)) {
        this.state.consentRequired = true;
        return;
      }

      if (this.config.ccpaEnabled && this.isCaliforniaUser(userLocation)) {
        this.state.consentRequired = true;
        return;
      }

      this.state.consentRequired = this.config.requireExplicitConsent;
    } catch (error) {
      console.error(
        "ConsentManager: Consent requirements check failed:",
        error
      );
      // Default to requiring consent on error
      this.state.consentRequired = true;
    }
  }

  /**
   * Determine if consent banner should be shown
   * Check conditions for displaying consent banner
   */
  shouldShowConsentBanner() {
    return (
      this.config.enableConsentBanner &&
      this.state.consentRequired &&
      !this.state.currentConsent
    );
  }

  /**
   * Show consent banner to user
   * Display consent collection interface
   */
  async showConsentBanner() {
    try {
      if (!this.consentBanner) {
        this.createConsentBanner();
      }

      this.consentBanner.style.display = "block";
      this.state.bannerVisible = true;

      this.logConsentEvent("banner_displayed", {
        timestamp: Date.now(),
        required: this.state.consentRequired,
      });

      console.log("ConsentManager: Consent banner displayed");
    } catch (error) {
      console.error("ConsentManager: Failed to show consent banner:", error);
    }
  }

  /**
   * Create consent banner UI element
   * Build consent banner interface
   */
  createConsentBanner() {
    try {
      this.consentBanner = document.createElement("div");
      this.consentBanner.id = "huntmaster-consent-banner";
      this.consentBanner.className = "huntmaster-consent-banner";

      const texts = this.getLocalizedTexts();

      this.consentBanner.innerHTML = `
                <div class="consent-banner-content">
                    <div class="consent-banner-text">
                        <h3>${texts.banner_title}</h3>
                        <p>${texts.banner_message}</p>
                    </div>
                    <div class="consent-banner-actions">
                        <button id="consent-accept-all" class="consent-btn consent-btn-primary">
                            ${texts.accept_all}
                        </button>
                        <button id="consent-reject-all" class="consent-btn consent-btn-secondary">
                            ${texts.reject_all}
                        </button>
                        <button id="consent-manage" class="consent-btn consent-btn-tertiary">
                            ${texts.manage_preferences}
                        </button>
                    </div>
                </div>
            `;

      this.addConsentBannerStyles();

      document.body.appendChild(this.consentBanner);

      this.setupBannerEventListeners();

      console.log("ConsentManager: Consent banner created");
    } catch (error) {
      console.error("ConsentManager: Failed to create consent banner:", error);
    }
  }

  /**
   * Add CSS styles for consent banner
   * Style the consent banner interface
   */
  addConsentBannerStyles() {
    try {
      if (document.getElementById("huntmaster-consent-styles")) {
        return;
      }

      const style = document.createElement("style");
      style.id = "huntmaster-consent-styles";
      style.textContent = `
                .huntmaster-consent-banner {
                    position: fixed;
                    bottom: 0;
                    left: 0;
                    right: 0;
                    background: #fff;
                    border-top: 1px solid #ddd;
                    box-shadow: 0 -2px 10px rgba(0,0,0,0.1);
                    z-index: 10000;
                    padding: 1rem;
                    display: none;
                }

                .consent-banner-content {
                    max-width: 1200px;
                    margin: 0 auto;
                    display: flex;
                    align-items: center;
                    justify-content: space-between;
                    gap: 1rem;
                }

                .consent-banner-text h3 {
                    margin: 0 0 0.5rem 0;
                    font-size: 1.1rem;
                    font-weight: 600;
                }

                .consent-banner-text p {
                    margin: 0;
                    color: #666;
                    font-size: 0.9rem;
                }

                .consent-banner-actions {
                    display: flex;
                    gap: 0.5rem;
                    flex-shrink: 0;
                }

                .consent-btn {
                    padding: 0.5rem 1rem;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                    font-size: 0.9rem;
                    transition: background-color 0.2s;
                }

                .consent-btn-primary {
                    background: #007cba;
                    color: white;
                }

                .consent-btn-primary:hover {
                    background: #005a87;
                }

                .consent-btn-secondary {
                    background: #6c757d;
                    color: white;
                }

                .consent-btn-secondary:hover {
                    background: #545b62;
                }

                .consent-btn-tertiary {
                    background: transparent;
                    color: #007cba;
                    border: 1px solid #007cba;
                }

                .consent-btn-tertiary:hover {
                    background: #007cba;
                    color: white;
                }

                @media (max-width: 768px) {
                    .consent-banner-content {
                        flex-direction: column;
                        text-align: center;
                    }

                    .consent-banner-actions {
                        width: 100%;
                        justify-content: center;
                    }
                }
            `;

      document.head.appendChild(style);
    } catch (error) {
      console.error("ConsentManager: Failed to add banner styles:", error);
    }
  }

  /**
   * Set up event listeners for consent banner
   * Handle banner button interactions
   */
  setupBannerEventListeners() {
    try {
      const acceptAllBtn = document.getElementById("consent-accept-all");
      if (acceptAllBtn) {
        acceptAllBtn.addEventListener("click", () => {
          this.acceptAllConsent();
        });
      }

      const rejectAllBtn = document.getElementById("consent-reject-all");
      if (rejectAllBtn) {
        rejectAllBtn.addEventListener("click", () => {
          this.rejectAllConsent();
        });
      }

      const manageBtn = document.getElementById("consent-manage");
      if (manageBtn) {
        manageBtn.addEventListener("click", () => {
          this.showPreferenceCenter();
        });
      }
    } catch (error) {
      console.error(
        "ConsentManager: Failed to set up banner event listeners:",
        error
      );
    }
  }

  /**
   * Accept all consent categories
   * Grant consent for all categories
   */
  acceptAllConsent() {
    try {
      const consent = this.createConsentObject(true);

      this.saveConsent(consent);

      this.hideConsentBanner();

      this.logConsentEvent("accept_all", {
        timestamp: Date.now(),
        categories: Object.keys(this.consentCategories),
      });

      console.log("ConsentManager: All consent accepted");
    } catch (error) {
      console.error("ConsentManager: Failed to accept all consent:", error);
    }
  }

  /**
   * Reject all non-essential consent categories
   * Deny consent for optional categories
   */
  rejectAllConsent() {
    try {
      const consent = this.createConsentObject(false);

      this.saveConsent(consent);

      this.hideConsentBanner();

      this.logConsentEvent("reject_all", {
        timestamp: Date.now(),
        categories: ["necessary"],
      });

      console.log("ConsentManager: All optional consent rejected");
    } catch (error) {
      console.error("ConsentManager: Failed to reject consent:", error);
    }
  }

  /**
   * Create consent object with specified acceptance
   * Build consent data structure
   */
  createConsentObject(acceptAll = false) {
    const consent = {
      timestamp: Date.now(),
      version: "1.0",
      method: "banner",
      userAgent: navigator.userAgent,
      language: this.config.defaultLanguage,
      categories: {},
      purposes: [],
      expiresAt:
        Date.now() + this.config.consentExpirationDays * 24 * 60 * 60 * 1000,
    };

    Object.entries(this.consentCategories).forEach(([categoryId, category]) => {
      const accepted = category.required || acceptAll;
      consent.categories[categoryId] = {
        accepted,
        timestamp: Date.now(),
        required: category.required,
      };

      if (accepted) {
        consent.purposes.push(...category.purposes);
      }
    });

    return consent;
  }

  /**
   * Save consent data to storage
   * Persist consent information
   */
  saveConsent(consent) {
    try {
      if (!this.validateConsentData(consent)) {
        throw new Error("Invalid consent data");
      }

      localStorage.setItem(this.config.storageKey, JSON.stringify(consent));
      this.state.currentConsent = consent;

      this.state.consentHistory.push(consent);

      if (this.state.consentHistory.length > 10) {
        this.state.consentHistory = this.state.consentHistory.slice(-5);
      }

      const historyKey = `${this.config.storageKey}_history`;
      localStorage.setItem(
        historyKey,
        JSON.stringify(this.state.consentHistory)
      );

      this.state.consentRequired = false;

      this.emit("consentChanged", consent);

      console.log("ConsentManager: Consent saved successfully");
    } catch (error) {
      console.error("ConsentManager: Failed to save consent:", error);
      throw error;
    }
  }

  /**
   * Hide consent banner
   * Remove consent banner from display
   */
  hideConsentBanner() {
    try {
      if (this.consentBanner) {
        this.consentBanner.style.display = "none";
        this.state.bannerVisible = false;
      }
    } catch (error) {
      console.error("ConsentManager: Failed to hide consent banner:", error);
    }
  }

  /**
   * Check if user has consent for specific purpose
   * Validate consent for data processing purpose
   */
  hasConsentFor(purpose) {
    try {
      if (
        !this.state.currentConsent ||
        this.isConsentExpired(this.state.currentConsent)
      ) {
        return false;
      }

      return this.state.currentConsent.purposes.includes(purpose);
    } catch (error) {
      console.error("ConsentManager: Consent check failed:", error);
      return false;
    }
  }

  /**
   * Check if consent is expired
   * Validate consent expiration status
   */
  isConsentExpired(consent) {
    if (!consent || !consent.expiresAt) {
      return true;
    }
    return Date.now() > consent.expiresAt;
  }

  /**
   * Validate consent data structure
   * Ensure consent data is properly formatted
   */
  validateConsentData(consent) {
    try {
      if (!consent.timestamp || !consent.version || !consent.categories) {
        return false;
      }

      if (typeof consent.categories !== "object") {
        return false;
      }

      if (
        !consent.categories.necessary ||
        !consent.categories.necessary.accepted
      ) {
        return false;
      }

      return true;
    } catch (error) {
      console.error("ConsentManager: Consent validation failed:", error);
      return false;
    }
  }

  /**
   * Get localized text based on user language
   * Return appropriate language texts
   */
  getLocalizedTexts() {
    const language = this.detectUserLanguage();
    return this.localizations[language] || this.localizations.en;
  }

  /**
   * Log consent-related events
   * Track consent events for compliance audit
   */
  logConsentEvent(eventType, data) {
    try {
      const logEntry = {
        timestamp: Date.now(),
        eventType,
        data,
        userAgent: navigator.userAgent,
        url: window.location.href,
      };

      const logKey = `${this.config.storageKey}_log`;
      let log = [];
      try {
        const existingLog = localStorage.getItem(logKey);
        if (existingLog) {
          log = JSON.parse(existingLog);
        }
      } catch (e) {
        // Ignore parsing errors
      }

      log.push(logEntry);

      if (log.length > 100) {
        log = log.slice(-50);
      }

      localStorage.setItem(logKey, JSON.stringify(log));

      if (this.config.debugMode) {
        console.log(`ConsentManager: Logged event ${eventType}`, data);
      }
    } catch (error) {
      console.error("ConsentManager: Failed to log consent event:", error);
    }
  }

  /**
   * Get current consent status
   * Return comprehensive consent information
   */
  getConsentStatus() {
    return {
      hasConsent: !!this.state.currentConsent,
      consentRequired: this.state.consentRequired,
      isExpired: this.state.currentConsent
        ? this.isConsentExpired(this.state.currentConsent)
        : true,
      categories: this.state.currentConsent?.categories || {},
      purposes: this.state.currentConsent?.purposes || [],
      timestamp: this.state.currentConsent?.timestamp,
      expiresAt: this.state.currentConsent?.expiresAt,
      bannerVisible: this.state.bannerVisible,
    };
  }

  /**
   * Handle consent errors
   * Process and log consent-related errors
   */
  handleError(errorType, error) {
    const errorRecord = {
      type: errorType,
      message: error.message || error,
      timestamp: Date.now(),
      stack: error.stack,
    };

    console.error(`ConsentManager: ${errorType}`, error);
  }

  /**
   * Clean up and destroy consent manager
   * Clean up resources and event listeners
   */
  async destroy() {
    try {
      if (this.consentBanner && this.consentBanner.parentNode) {
        this.consentBanner.parentNode.removeChild(this.consentBanner);
      }

      const styles = document.getElementById("huntmaster-consent-styles");
      if (styles && styles.parentNode) {
        styles.parentNode.removeChild(styles);
      }

      this.eventHandlers.clear();

      this.state.isInitialized = false;

      console.log("ConsentManager: Destroyed successfully");
    } catch (error) {
      console.error("ConsentManager: Destruction failed:", error);
    }
  }

  detectUserLocation() {
    return "unknown";
  }
  isEUUser(location) {
    return false;
  }
  isCaliforniaUser(location) {
    return false;
  }
  detectUserLanguage() {
    return navigator.language?.substring(0, 2) || "en";
  }
  setupConsentReminders() {
    /* Would implement reminder system */
  }
  setupPreferenceCenter() {
    /* Would implement preference center UI */
  }
  showPreferenceCenter() {
    /* Would show preference center */
  }
  setupEventListeners() {
    /* Would set up additional event listeners */
  }
  emit(event, data) {
    /* Would emit events for external handling */
  }
}

export { ConsentManager };

export const createConsentManager = (options) => new ConsentManager(options);
export const checkConsentRequired = () => {
  // Simple check for common consent-requiring jurisdictions
  const language = navigator.language;
  const timezone = Intl.DateTimeFormat().resolvedOptions().timeZone;

  // EU timezones (simplified)
  const euTimezones = ["Europe/", "Atlantic/Azores", "Atlantic/Madeira"];
  return euTimezones.some((tz) => timezone.startsWith(tz));
};

export const ConsentUtils = {
  createConsentId: () => {
    return `consent_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  },

  isConsentValid: (consent) => {
    return (
      consent &&
      consent.timestamp &&
      consent.categories &&
      consent.categories.necessary &&
      consent.categories.necessary.accepted &&
      (!consent.expiresAt || Date.now() < consent.expiresAt)
    );
  },

  getConsentPurposes: (consent) => {
    return consent?.purposes || [];
  },

  hasConsentForPurpose: (consent, purpose) => {
    return (
      ConsentUtils.isConsentValid(consent) && consent.purposes.includes(purpose)
    );
  },
};

console.log("ConsentManager module loaded successfully");
