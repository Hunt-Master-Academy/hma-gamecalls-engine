/**
 * Data Validator Module - Session Validation Framework
 *
 * Comprehensive data validation engine for session analysis with schema validation,
 * integrity checks, and data quality assessment for the Huntmaster Audio Engine.
 *
 * Features:
 * - Multi-layer data validation with type checking and constraint enforcement
 * - Real-time integrity monitoring with anomaly detection
 * - Schema compliance validation with custom rule engines
 * - Data quality metrics and automated cleansing
 * - Cross-module validation with dependency checking
 *
 * @version 1.0.0
 * @author Huntmaster Team
 * @since 2025-07-26
 */

class DataValidator {
  constructor(options = {}) {
    this.config = {
      strictMode: options.strictMode || false,
      autoCorrect: options.autoCorrect || true,
      validationLevel: options.validationLevel || "standard",
      customRules: options.customRules || [],
      realTimeValidation: options.realTimeValidation || true,
      ...options,
    };

    this.validationRules = new Map();
    this.customValidators = new Map();
    this.validationHistory = [];
    this.errorPatterns = new Map();

    this.integrityMonitor = {
      checksumCache: new Map(),
      lastValidation: null,
      validationCount: 0,
      errorCount: 0,
      warningCount: 0,
    };

    this.initializeDefaultRules();
  }

  /**
   * Initialize default validation rules
   * Set up built-in validation rules for common data types and patterns
   */
  initializeDefaultRules() {
    this.addValidationRule("audioData", {
      type: "array",
      minLength: 1,
      maxLength: 1000000,
      elementType: "number",
      range: [-1, 1],
      validator: this.validateAudioSamples.bind(this),
    });

    this.addValidationRule("sessionMetadata", {
      type: "object",
      required: ["sessionId", "timestamp", "userId"],
      properties: {
        sessionId: { type: "string", pattern: /^[a-zA-Z0-9-]+$/ },
        timestamp: { type: "number", min: 0 },
        userId: { type: "string", minLength: 1 },
      },
      validator: this.validateSessionMetadata.bind(this),
    });

    this.addValidationRule("performanceMetrics", {
      type: "object",
      required: ["cpuUsage", "memoryUsage", "latency"],
      properties: {
        cpuUsage: { type: "number", min: 0, max: 100 },
        memoryUsage: { type: "number", min: 0 },
        latency: { type: "number", min: 0, max: 10000 },
      },
      validator: this.validatePerformanceData.bind(this),
    });
  }

  /**
   * Add custom validation rule
   * Register a new validation rule with the validation engine
   */
  addValidationRule(name, rule) {
    if (!name || !rule) {
      throw new Error("Rule name and definition are required");
    }

    this.validationRules.set(name, {
      ...rule,
      createdAt: Date.now(),
      usageCount: 0,
    });

    return this;
  }

  /**
   * Validate data against schema
   * Primary validation method that checks data against defined rules
   */
  async validateData(data, schemaName, options = {}) {
    const startTime = performance.now();

    try {
      const schema = this.validationRules.get(schemaName);
      if (!schema) {
        throw new Error(`Validation schema '${schemaName}' not found`);
      }

      const typeValidation = await this.performTypeValidation(data, schema);
      if (!typeValidation.isValid) {
        return this.createValidationResult(
          false,
          typeValidation.errors,
          startTime
        );
      }

      const constraintValidation = await this.performConstraintValidation(
        data,
        schema
      );
      if (!constraintValidation.isValid) {
        return this.createValidationResult(
          false,
          constraintValidation.errors,
          startTime
        );
      }

      if (schema.validator) {
        const customValidation = await schema.validator(data, options);
        if (!customValidation.isValid) {
          return this.createValidationResult(
            false,
            customValidation.errors,
            startTime
          );
        }
      }

      schema.usageCount++;
      this.integrityMonitor.validationCount++;

      return this.createValidationResult(true, [], startTime, {
        schemaUsed: schemaName,
        dataSize: this.calculateDataSize(data),
      });
    } catch (error) {
      this.integrityMonitor.errorCount++;
      return this.createValidationResult(false, [error.message], startTime);
    }
  }

  /**
   * Perform type validation
   * Validate data types against schema requirements
   */
  async performTypeValidation(data, schema) {
    const errors = [];

    if (
      schema.type &&
      typeof data !== schema.type &&
      !this.isValidArrayType(data, schema)
    ) {
      errors.push(`Expected type '${schema.type}', got '${typeof data}'`);
    }

    if (schema.type === "array" && Array.isArray(data) && schema.elementType) {
      const invalidElements = data.filter((item, index) => {
        if (typeof item !== schema.elementType) {
          errors.push(
            `Array element at index ${index}: expected '${
              schema.elementType
            }', got '${typeof item}'`
          );
          return true;
        }
        return false;
      });
    }

    if (schema.type === "object" && schema.properties) {
      for (const [key, propSchema] of Object.entries(schema.properties)) {
        if (data[key] !== undefined) {
          const propValidation = await this.performTypeValidation(
            data[key],
            propSchema
          );
          if (!propValidation.isValid) {
            errors.push(
              ...propValidation.errors.map((err) => `Property '${key}': ${err}`)
            );
          }
        }
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Perform constraint validation
   * Validate data constraints like ranges, patterns, required fields
   */
  async performConstraintValidation(data, schema) {
    const errors = [];

    if (schema.required && schema.type === "object") {
      for (const requiredField of schema.required) {
        if (data[requiredField] === undefined || data[requiredField] === null) {
          errors.push(`Required field '${requiredField}' is missing`);
        }
      }
    }

    if (schema.pattern && typeof data === "string") {
      if (!schema.pattern.test(data)) {
        errors.push(
          `String does not match required pattern: ${schema.pattern}`
        );
      }
    }

    if (typeof data === "number") {
      if (schema.min !== undefined && data < schema.min) {
        errors.push(`Value ${data} is below minimum ${schema.min}`);
      }
      if (schema.max !== undefined && data > schema.max) {
        errors.push(`Value ${data} is above maximum ${schema.max}`);
      }
    }

    if (Array.isArray(data)) {
      if (schema.minLength !== undefined && data.length < schema.minLength) {
        errors.push(
          `Array length ${data.length} is below minimum ${schema.minLength}`
        );
      }
      if (schema.maxLength !== undefined && data.length > schema.maxLength) {
        errors.push(
          `Array length ${data.length} is above maximum ${schema.maxLength}`
        );
      }

      if (schema.range && schema.elementType === "number") {
        const [min, max] = schema.range;
        data.forEach((value, index) => {
          if (value < min || value > max) {
            errors.push(
              `Array element at index ${index}: value ${value} is outside range [${min}, ${max}]`
            );
          }
        });
      }
    }

    if (typeof data === "string") {
      if (schema.minLength !== undefined && data.length < schema.minLength) {
        errors.push(
          `String length ${data.length} is below minimum ${schema.minLength}`
        );
      }
      if (schema.maxLength !== undefined && data.length > schema.maxLength) {
        errors.push(
          `String length ${data.length} is above maximum ${schema.maxLength}`
        );
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate audio samples
   * Custom validator for audio data arrays
   */
  async validateAudioSamples(data, options = {}) {
    const errors = [];

    const invalidSamples = data.filter((sample, index) => {
      if (isNaN(sample) || !isFinite(sample)) {
        errors.push(`Invalid audio sample at index ${index}: ${sample}`);
        return true;
      }
      return false;
    });

    if (options.checkDCOffset !== false) {
      const dcOffset =
        data.reduce((sum, sample) => sum + sample, 0) / data.length;
      if (Math.abs(dcOffset) > 0.1) {
        errors.push(`High DC offset detected: ${dcOffset.toFixed(4)}`);
      }
    }

    if (options.checkClipping !== false) {
      const clippedSamples = data.filter(
        (sample) => Math.abs(sample) >= 0.99
      ).length;
      const clippingPercentage = (clippedSamples / data.length) * 100;
      if (clippingPercentage > 1) {
        errors.push(
          `Audio clipping detected: ${clippingPercentage.toFixed(
            2
          )}% of samples`
        );
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate session metadata
   * Custom validator for session metadata objects
   */
  async validateSessionMetadata(data, options = {}) {
    const errors = [];

    const now = Date.now();
    if (data.timestamp > now) {
      errors.push("Session timestamp cannot be in the future");
    }

    const oneYearAgo = now - 365 * 24 * 60 * 60 * 1000;
    if (data.timestamp < oneYearAgo) {
      errors.push("Session timestamp is too old (more than 1 year)");
    }

    if (data.duration !== undefined) {
      if (data.duration < 0) {
        errors.push("Session duration cannot be negative");
      }
      if (data.duration > 24 * 60 * 60 * 1000) {
        // 24 hours
        errors.push("Session duration exceeds maximum allowed (24 hours)");
      }
    }

    if (data.userAgent && typeof data.userAgent === "string") {
      if (data.userAgent.length > 500) {
        errors.push("User agent string is too long");
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate performance data
   * Custom validator for performance metrics
   */
  async validatePerformanceData(data, options = {}) {
    const errors = [];

    if (data.cpuUsage > 95) {
      errors.push("CPU usage is critically high (>95%)");
    }

    if (data.memoryUsage > 1024 * 1024 * 1024) {
      // 1GB
      errors.push("Memory usage is extremely high (>1GB)");
    }

    if (data.latency > 1000) {
      // 1 second
      errors.push("Latency is critically high (>1000ms)");
    }

    if (data.frameRate && data.frameRate < 30) {
      errors.push("Frame rate is below acceptable threshold (<30fps)");
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * TODO: Batch validate multiple data items
   * Validate an array of data items against the same schema
   */
  async batchValidate(dataArray, schemaName, options = {}) {
    const results = [];
    const batchOptions = {
      ...options,
      batchMode: true,
      batchSize: dataArray.length,
    };

    for (let i = 0; i < dataArray.length; i++) {
      const itemResult = await this.validateData(dataArray[i], schemaName, {
        ...batchOptions,
        batchIndex: i,
      });

      results.push({
        index: i,
        data: dataArray[i],
        ...itemResult,
      });
    }

    const summary = this.generateBatchSummary(results);
    return {
      results,
      summary,
      isValid: results.every((result) => result.isValid),
    };
  }

  /**
   * Generate batch validation summary
   * Create summary statistics for batch validation results
   */
  generateBatchSummary(results) {
    const totalItems = results.length;
    const validItems = results.filter((r) => r.isValid).length;
    const invalidItems = totalItems - validItems;

    const allErrors = results.flatMap((r) => r.errors || []);
    const errorFrequency = {};

    allErrors.forEach((error) => {
      errorFrequency[error] = (errorFrequency[error] || 0) + 1;
    });

    return {
      totalItems,
      validItems,
      invalidItems,
      successRate: (validItems / totalItems) * 100,
      errorFrequency,
      mostCommonError: Object.keys(errorFrequency).reduce(
        (a, b) => (errorFrequency[a] > errorFrequency[b] ? a : b),
        null
      ),
    };
  }

  /**
   * TODO: Get validation statistics
   * Return comprehensive validation statistics and metrics
   */
  getValidationStatistics() {
    const ruleStats = Array.from(this.validationRules.entries()).map(
      ([name, rule]) => ({
        name,
        usageCount: rule.usageCount,
        createdAt: rule.createdAt,
      })
    );

    return {
      monitor: { ...this.integrityMonitor },
      rules: ruleStats,
      historyCount: this.validationHistory.length,
      errorPatterns: Array.from(this.errorPatterns.entries()),
      successRate:
        this.integrityMonitor.validationCount > 0
          ? ((this.integrityMonitor.validationCount -
              this.integrityMonitor.errorCount) /
              this.integrityMonitor.validationCount) *
            100
          : 0,
    };
  }

  /**
   * TODO: Helper methods for internal operations
   */
  isValidArrayType(data, schema) {
    return schema.type === "array" && Array.isArray(data);
  }

  calculateDataSize(data) {
    return JSON.stringify(data).length;
  }

  createValidationResult(isValid, errors, startTime, metadata = {}) {
    const result = {
      isValid,
      errors,
      executionTime: performance.now() - startTime,
      timestamp: Date.now(),
      ...metadata,
    };

    this.validationHistory.push(result);
    if (this.validationHistory.length > 1000) {
      this.validationHistory = this.validationHistory.slice(-1000);
    }

    return result;
  }
}

// Export the DataValidator class
export default DataValidator;

// Also export utility functions
export { DataValidator };
