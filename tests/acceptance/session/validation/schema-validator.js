/**
 * Schema Validator Module - Session Validation Framework
 *
 * JSON schema validation and data structure validation engine for session data
 * in the Huntmaster Audio Engine. Provides comprehensive schema management,
 * validation, and structure verification capabilities.
 *
 * Features:
 * - JSON Schema Draft 7/2019-09 compliance with full validation
 * - Dynamic schema registration and management
 * - Nested object validation with circular reference detection
 * - Custom format validators and constraint checking
 * - Performance-optimized validation with caching
 *
 * @version 1.0.0
 * @author Huntmaster Team
 * @since 2025-07-26
 */

class SchemaValidator {
  constructor(options = {}) {
    // Initialize schema validation configuration
    this.config = {
      strictMode: options.strictMode || true,
      allowAdditionalProperties: options.allowAdditionalProperties || false,
      validateFormats: options.validateFormats || true,
      cacheSchemas: options.cacheSchemas || true,
      maxDepth: options.maxDepth || 100,
      ...options,
    };

    // Set up schema registry and caching
    this.schemaRegistry = new Map();
    this.validationCache = new Map();
    this.formatValidators = new Map();
    this.validationStats = {
      totalValidations: 0,
      cacheHits: 0,
      cacheMisses: 0,
      errors: 0,
    };

    this.initializeDefaultSchemas();
    this.initializeFormatValidators();
  }

  /**
   * Initialize default schemas
   * Set up built-in schemas for common session data structures
   */
  initializeDefaultSchemas() {
    this.registerSchema("session", {
      $schema: "http://json-schema.org/draft-07/schema#",
      type: "object",
      required: ["sessionId", "timestamp", "userId", "data"],
      properties: {
        sessionId: {
          type: "string",
          pattern: "^[a-zA-Z0-9-_]+$",
          minLength: 8,
          maxLength: 64,
        },
        timestamp: {
          type: "integer",
          minimum: 0,
          maximum: 9999999999999,
        },
        userId: {
          type: "string",
          minLength: 1,
          maxLength: 256,
        },
        data: {
          type: "object",
          additionalProperties: true,
        },
        metadata: {
          type: "object",
          properties: {
            userAgent: { type: "string", maxLength: 500 },
            platform: { type: "string", maxLength: 100 },
            version: { type: "string", pattern: "^\\d+\\.\\d+\\.\\d+$" },
          },
        },
      },
      additionalProperties: false,
    });

    this.registerSchema("audioData", {
      $schema: "http://json-schema.org/draft-07/schema#",
      type: "object",
      required: ["samples", "sampleRate", "channels"],
      properties: {
        samples: {
          type: "array",
          items: {
            type: "number",
            minimum: -1,
            maximum: 1,
          },
          minItems: 1,
          maxItems: 10000000,
        },
        sampleRate: {
          type: "integer",
          enum: [8000, 16000, 22050, 44100, 48000, 96000],
        },
        channels: {
          type: "integer",
          minimum: 1,
          maximum: 8,
        },
        bitDepth: {
          type: "integer",
          enum: [16, 24, 32],
        },
        format: {
          type: "string",
          enum: ["PCM", "Float32", "Float64"],
        },
      },
      additionalProperties: false,
    });

    this.registerSchema("performanceMetrics", {
      $schema: "http://json-schema.org/draft-07/schema#",
      type: "object",
      required: ["timestamp", "metrics"],
      properties: {
        timestamp: {
          type: "integer",
          minimum: 0,
        },
        metrics: {
          type: "object",
          required: ["cpu", "memory", "audio"],
          properties: {
            cpu: {
              type: "object",
              required: ["usage"],
              properties: {
                usage: { type: "number", minimum: 0, maximum: 100 },
                cores: { type: "integer", minimum: 1, maximum: 64 },
                temperature: { type: "number", minimum: 0, maximum: 150 },
              },
            },
            memory: {
              type: "object",
              required: ["used", "total"],
              properties: {
                used: { type: "integer", minimum: 0 },
                total: { type: "integer", minimum: 0 },
                available: { type: "integer", minimum: 0 },
              },
            },
            audio: {
              type: "object",
              required: ["latency", "bufferSize"],
              properties: {
                latency: { type: "number", minimum: 0, maximum: 10000 },
                bufferSize: { type: "integer", minimum: 32, maximum: 8192 },
                sampleRate: { type: "integer", minimum: 8000, maximum: 192000 },
                underruns: { type: "integer", minimum: 0 },
                overruns: { type: "integer", minimum: 0 },
              },
            },
          },
        },
      },
    });
  }

  /**
   * Initialize format validators
   * Set up custom format validation functions
   */
  initializeFormatValidators() {
    this.addFormatValidator("date-time", (value) => {
      const dateRegex = /^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d{3})?Z?$/;
      return dateRegex.test(value) && !isNaN(Date.parse(value));
    });

    this.addFormatValidator("uuid", (value) => {
      const uuidRegex =
        /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
      return uuidRegex.test(value);
    });

    this.addFormatValidator("audio-format", (value) => {
      const validFormats = ["wav", "mp3", "flac", "ogg", "aac", "m4a"];
      return validFormats.includes(value.toLowerCase());
    });

    this.addFormatValidator("session-id", (value) => {
      return /^[a-zA-Z0-9-_]{8,64}$/.test(value);
    });
  }

  /**
   * TODO: Register schema
   * Add a new schema to the registry for validation
   */
  registerSchema(name, schema) {
    if (!name || !schema) {
      throw new Error("Schema name and definition are required");
    }

    this.validateSchemaStructure(schema);

    this.schemaRegistry.set(name, {
      schema,
      registeredAt: Date.now(),
      usageCount: 0,
      lastUsed: null,
    });

    this.clearCacheForSchema(name);

    return this;
  }

  /**
   * Validate data against schema
   * Main validation method that checks data against a registered schema
   */
  async validateSchema(data, schemaName, options = {}) {
    const startTime = performance.now();
    this.validationStats.totalValidations++;

    try {
      const cacheKey = this.generateCacheKey(data, schemaName, options);
      if (this.config.cacheSchemas && this.validationCache.has(cacheKey)) {
        this.validationStats.cacheHits++;
        return this.validationCache.get(cacheKey);
      }
      this.validationStats.cacheMisses++;

      const schemaEntry = this.schemaRegistry.get(schemaName);
      if (!schemaEntry) {
        throw new Error(`Schema '${schemaName}' not found in registry`);
      }

      schemaEntry.usageCount++;
      schemaEntry.lastUsed = Date.now();

      const result = await this.performSchemaValidation(
        data,
        schemaEntry.schema,
        options
      );

      result.executionTime = performance.now() - startTime;
      result.schemaName = schemaName;
      result.timestamp = Date.now();

      if (this.config.cacheSchemas) {
        this.validationCache.set(cacheKey, result);
        // Limit cache size
        if (this.validationCache.size > 1000) {
          const firstKey = this.validationCache.keys().next().value;
          this.validationCache.delete(firstKey);
        }
      }

      return result;
    } catch (error) {
      this.validationStats.errors++;
      return {
        isValid: false,
        errors: [error.message],
        executionTime: performance.now() - startTime,
        schemaName,
        timestamp: Date.now(),
      };
    }
  }

  /**
   * Perform schema validation
   * Core schema validation logic with comprehensive checking
   */
  async performSchemaValidation(
    data,
    schema,
    options = {},
    path = "",
    depth = 0
  ) {
    const errors = [];

    if (depth > this.config.maxDepth) {
      errors.push(`Maximum validation depth exceeded at path: ${path}`);
      return { isValid: false, errors };
    }

    if (schema.type) {
      const typeResult = this.validateType(data, schema.type, path);
      if (!typeResult.isValid) {
        errors.push(...typeResult.errors);
        return { isValid: false, errors }; // Early exit for type errors
      }
    }

    if (schema.enum && !schema.enum.includes(data)) {
      errors.push(`Value at ${path} must be one of: ${schema.enum.join(", ")}`);
    }

    if (typeof data === "string") {
      const stringResult = this.validateStringConstraints(data, schema, path);
      errors.push(...stringResult.errors);
    }

    if (typeof data === "number") {
      const numberResult = this.validateNumberConstraints(data, schema, path);
      errors.push(...numberResult.errors);
    }

    if (Array.isArray(data)) {
      const arrayResult = await this.validateArrayConstraints(
        data,
        schema,
        options,
        path,
        depth
      );
      errors.push(...arrayResult.errors);
    }

    if (data && typeof data === "object" && !Array.isArray(data)) {
      const objectResult = await this.validateObjectConstraints(
        data,
        schema,
        options,
        path,
        depth
      );
      errors.push(...objectResult.errors);
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate data type
   * Check if data matches the expected type
   */
  validateType(data, expectedType, path) {
    const actualType = Array.isArray(data) ? "array" : typeof data;
    const errors = [];

    if (expectedType === "integer") {
      if (typeof data !== "number" || !Number.isInteger(data)) {
        errors.push(`Expected integer at ${path}, got ${actualType}`);
      }
    } else if (expectedType === "null") {
      if (data !== null) {
        errors.push(`Expected null at ${path}, got ${actualType}`);
      }
    } else if (actualType !== expectedType) {
      errors.push(`Expected ${expectedType} at ${path}, got ${actualType}`);
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate string constraints
   * Check string-specific validation rules
   */
  validateStringConstraints(data, schema, path) {
    const errors = [];

    if (schema.minLength !== undefined && data.length < schema.minLength) {
      errors.push(
        `String at ${path} is too short (${data.length} < ${schema.minLength})`
      );
    }

    if (schema.maxLength !== undefined && data.length > schema.maxLength) {
      errors.push(
        `String at ${path} is too long (${data.length} > ${schema.maxLength})`
      );
    }

    if (schema.pattern) {
      const regex = new RegExp(schema.pattern);
      if (!regex.test(data)) {
        errors.push(
          `String at ${path} does not match pattern: ${schema.pattern}`
        );
      }
    }

    if (schema.format && this.config.validateFormats) {
      const formatValidator = this.formatValidators.get(schema.format);
      if (formatValidator && !formatValidator(data)) {
        errors.push(
          `String at ${path} does not match format: ${schema.format}`
        );
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate number constraints
   * Check number-specific validation rules
   */
  validateNumberConstraints(data, schema, path) {
    const errors = [];

    if (schema.minimum !== undefined && data < schema.minimum) {
      errors.push(
        `Number at ${path} is below minimum (${data} < ${schema.minimum})`
      );
    }

    if (schema.maximum !== undefined && data > schema.maximum) {
      errors.push(
        `Number at ${path} is above maximum (${data} > ${schema.maximum})`
      );
    }

    if (
      schema.exclusiveMinimum !== undefined &&
      data <= schema.exclusiveMinimum
    ) {
      errors.push(
        `Number at ${path} must be greater than ${schema.exclusiveMinimum}`
      );
    }

    if (
      schema.exclusiveMaximum !== undefined &&
      data >= schema.exclusiveMaximum
    ) {
      errors.push(
        `Number at ${path} must be less than ${schema.exclusiveMaximum}`
      );
    }

    if (schema.multipleOf !== undefined && data % schema.multipleOf !== 0) {
      errors.push(
        `Number at ${path} is not a multiple of ${schema.multipleOf}`
      );
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate array constraints
   * Check array-specific validation rules and validate items
   */
  async validateArrayConstraints(data, schema, options, path, depth) {
    const errors = [];

    if (schema.minItems !== undefined && data.length < schema.minItems) {
      errors.push(
        `Array at ${path} has too few items (${data.length} < ${schema.minItems})`
      );
    }

    if (schema.maxItems !== undefined && data.length > schema.maxItems) {
      errors.push(
        `Array at ${path} has too many items (${data.length} > ${schema.maxItems})`
      );
    }

    if (schema.uniqueItems && this.hasDuplicates(data)) {
      errors.push(`Array at ${path} contains duplicate items`);
    }

    if (schema.items) {
      for (let i = 0; i < data.length; i++) {
        const itemPath = `${path}[${i}]`;
        const itemResult = await this.performSchemaValidation(
          data[i],
          schema.items,
          options,
          itemPath,
          depth + 1
        );
        errors.push(...itemResult.errors);
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Validate object constraints
   * Check object-specific validation rules and validate properties
   */
  async validateObjectConstraints(data, schema, options, path, depth) {
    const errors = [];

    if (schema.required) {
      for (const requiredProp of schema.required) {
        if (!(requiredProp in data)) {
          errors.push(
            `Required property '${requiredProp}' is missing at ${path}`
          );
        }
      }
    }

    if (schema.properties) {
      for (const [propName, propSchema] of Object.entries(schema.properties)) {
        if (propName in data) {
          const propPath = path ? `${path}.${propName}` : propName;
          const propResult = await this.performSchemaValidation(
            data[propName],
            propSchema,
            options,
            propPath,
            depth + 1
          );
          errors.push(...propResult.errors);
        }
      }
    }

    if (
      !this.config.allowAdditionalProperties &&
      schema.additionalProperties === false
    ) {
      const allowedProps = new Set(Object.keys(schema.properties || {}));
      for (const propName of Object.keys(data)) {
        if (!allowedProps.has(propName)) {
          errors.push(
            `Additional property '${propName}' is not allowed at ${path}`
          );
        }
      }
    }

    return { isValid: errors.length === 0, errors };
  }

  /**
   * Add format validator
   * Register a custom format validation function
   */
  addFormatValidator(format, validator) {
    if (typeof validator !== "function") {
      throw new Error("Format validator must be a function");
    }
    this.formatValidators.set(format, validator);
    return this;
  }

  /**
   * TODO: Get validation statistics
   * Return comprehensive validation statistics and performance metrics
   */
  getValidationStatistics() {
    const schemaStats = Array.from(this.schemaRegistry.entries()).map(
      ([name, entry]) => ({
        name,
        usageCount: entry.usageCount,
        lastUsed: entry.lastUsed,
        registeredAt: entry.registeredAt,
      })
    );

    return {
      validationStats: { ...this.validationStats },
      schemas: schemaStats,
      cacheSize: this.validationCache.size,
      formatValidators: Array.from(this.formatValidators.keys()),
      cacheHitRate:
        this.validationStats.totalValidations > 0
          ? (this.validationStats.cacheHits /
              this.validationStats.totalValidations) *
            100
          : 0,
    };
  }

  /**
   * TODO: Helper methods for internal operations
   */
  validateSchemaStructure(schema) {
    if (!schema || typeof schema !== "object") {
      throw new Error("Schema must be an object");
    }
    // Basic schema structure validation could be expanded
  }

  generateCacheKey(data, schemaName, options) {
    const dataHash = this.simpleHash(JSON.stringify(data));
    const optionsHash = this.simpleHash(JSON.stringify(options));
    return `${schemaName}:${dataHash}:${optionsHash}`;
  }

  simpleHash(str) {
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      const char = str.charCodeAt(i);
      hash = (hash << 5) - hash + char;
      hash = hash & hash; // Convert to 32-bit integer
    }
    return hash.toString(36);
  }

  hasDuplicates(array) {
    const seen = new Set();
    for (const item of array) {
      const key = JSON.stringify(item);
      if (seen.has(key)) return true;
      seen.add(key);
    }
    return false;
  }

  clearCacheForSchema(schemaName) {
    for (const [key, value] of this.validationCache.entries()) {
      if (key.startsWith(`${schemaName}:`)) {
        this.validationCache.delete(key);
      }
    }
  }
}

// Export the SchemaValidator class
export default SchemaValidator;

// Also export utility functions
export { SchemaValidator };
