/**
 * @file ui-components.js
 * @brief UI Component System
 *
 * This module provides reusable UI components with lifecycle management,
 * state handling, event binding, and consistent styling for the
 * Huntmaster web application interface.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

/**
 * @class BaseComponent
 * @brief Base class for all UI components
 *
 * Provides common functionality for component lifecycle,
 * event handling, and state management.
 */
export class BaseComponent {
  constructor(element, options = {}) {
    this.element = element;
    this.options = {
      autoInit: true,
      enableEvents: true,
      enableState: true,
      ...options,
    };

    // Component state
    this.state = {};
    this.isInitialized = false;
    this.isDestroyed = false;

    // Event listeners
    this.eventListeners = new Map();
    this.boundMethods = new Map();

    // Performance tracking
    this.performanceMetrics = {
      renderCount: 0,
      lastRenderTime: 0,
      totalRenderTime: 0,
      eventCount: 0,
    };

    if (this.options.autoInit) {
      this.initialize();
    }
  }

  /**
   * Initialize component
   */
  initialize() {
    if (this.isInitialized || this.isDestroyed) {
      return false;
    }

    try {
      this.setupDOM();
      this.bindEvents();
      this.initializeState();
      this.render();

      this.isInitialized = true;
      this.emit("initialized");
      return true;
    } catch (error) {
      console.error("Component initialization failed:", error);
      this.emit("error", { type: "initialization", error });
      return false;
    }
  }

  /**
   * Setup DOM structure (override in subclasses)
   */
  setupDOM() {
    // Override in subclasses
  }

  /**
   * Bind event listeners (override in subclasses)
   */
  bindEvents() {
    // Override in subclasses
  }

  /**
   * Initialize component state (override in subclasses)
   */
  initializeState() {
    // Override in subclasses
  }

  /**
   * Render component (override in subclasses)
   */
  render() {
    const startTime = performance.now();

    try {
      this.performRender();

      // Update performance metrics
      const renderTime = performance.now() - startTime;
      this.performanceMetrics.renderCount++;
      this.performanceMetrics.lastRenderTime = renderTime;
      this.performanceMetrics.totalRenderTime += renderTime;

      this.emit("rendered");
    } catch (error) {
      console.error("Component render failed:", error);
      this.emit("error", { type: "render", error });
    }
  }

  /**
   * Actual render implementation (override in subclasses)
   */
  performRender() {
    // Override in subclasses
  }

  /**
   * Update component state
   */
  setState(updates, options = {}) {
    if (!this.options.enableState) {
      return false;
    }

    const previousState = { ...this.state };
    this.state = { ...this.state, ...updates };

    this.emit("stateChanged", {
      previousState,
      newState: { ...this.state },
      updates,
    });

    if (options.rerender !== false) {
      this.render();
    }

    return true;
  }

  /**
   * Get component state
   */
  getState() {
    return { ...this.state };
  }

  /**
   * Add event listener with automatic cleanup
   */
  addEventListener(element, event, handler, options = {}) {
    if (!element || typeof handler !== "function") {
      return false;
    }

    // Create bound method for proper cleanup
    const boundHandler = handler.bind(this);
    const listenerKey = `${element.id || "element"}_${event}_${Date.now()}`;

    this.eventListeners.set(listenerKey, {
      element,
      event,
      handler: boundHandler,
      options,
    });

    element.addEventListener(event, boundHandler, options);
    this.performanceMetrics.eventCount++;

    return listenerKey;
  }

  /**
   * Remove specific event listener
   */
  removeEventListener(listenerKey) {
    const listener = this.eventListeners.get(listenerKey);
    if (listener) {
      listener.element.removeEventListener(
        listener.event,
        listener.handler,
        listener.options
      );
      this.eventListeners.delete(listenerKey);
      return true;
    }
    return false;
  }

  /**
   * Emit custom events
   */
  emit(eventType, data = {}) {
    if (!this.element || !this.options.enableEvents) {
      return false;
    }

    const event = new CustomEvent(`component:${eventType}`, {
      detail: {
        component: this,
        data,
        timestamp: Date.now(),
      },
      bubbles: true,
      cancelable: true,
    });

    this.element.dispatchEvent(event);
    return true;
  }

  /**
   * Show component
   */
  show() {
    if (this.element) {
      this.element.style.display = "";
      this.element.classList.remove("hidden");
      this.emit("shown");
    }
  }

  /**
   * Hide component
   */
  hide() {
    if (this.element) {
      this.element.style.display = "none";
      this.element.classList.add("hidden");
      this.emit("hidden");
    }
  }

  /**
   * Enable component
   */
  enable() {
    if (this.element) {
      this.element.disabled = false;
      this.element.classList.remove("disabled");
      this.setState({ disabled: false });
      this.emit("enabled");
    }
  }

  /**
   * Disable component
   */
  disable() {
    if (this.element) {
      this.element.disabled = true;
      this.element.classList.add("disabled");
      this.setState({ disabled: true });
      this.emit("disabled");
    }
  }

  /**
   * Destroy component and cleanup resources
   */
  destroy() {
    if (this.isDestroyed) {
      return false;
    }

    // Remove all event listeners
    for (const [key, listener] of this.eventListeners) {
      listener.element.removeEventListener(
        listener.event,
        listener.handler,
        listener.options
      );
    }
    this.eventListeners.clear();

    // Clear bound methods
    this.boundMethods.clear();

    // Clear state
    this.state = {};

    // Mark as destroyed
    this.isDestroyed = true;
    this.isInitialized = false;

    this.emit("destroyed");
    return true;
  }

  /**
   * Get performance metrics
   */
  getPerformanceMetrics() {
    return {
      ...this.performanceMetrics,
      averageRenderTime:
        this.performanceMetrics.renderCount > 0
          ? this.performanceMetrics.totalRenderTime /
            this.performanceMetrics.renderCount
          : 0,
    };
  }
}

/**
 * @class Button
 * @brief Enhanced button component with loading states and click handling
 */
export class Button extends BaseComponent {
  constructor(element, options = {}) {
    super(element, {
      clickHandler: null,
      loadingText: "Loading...",
      loadingClass: "loading",
      disabledClass: "disabled",
      ...options,
    });
  }

  setupDOM() {
    if (!this.element) return;

    this.originalText = this.element.textContent;
    this.element.classList.add("huntmaster-button");
  }

  bindEvents() {
    if (this.element) {
      this.addEventListener(this.element, "click", this.handleClick);
    }
  }

  initializeState() {
    this.setState(
      {
        loading: false,
        disabled: false,
        text: this.originalText,
      },
      { rerender: false }
    );
  }

  handleClick(event) {
    if (this.state.loading || this.state.disabled) {
      event.preventDefault();
      return false;
    }

    this.emit("click", { originalEvent: event });

    if (this.options.clickHandler) {
      this.options.clickHandler.call(this, event);
    }
  }

  setLoading(loading = true) {
    this.setState({
      loading,
      disabled: loading,
      text: loading ? this.options.loadingText : this.originalText,
    });
  }

  performRender() {
    if (!this.element) return;

    this.element.textContent = this.state.text;
    this.element.disabled = this.state.disabled;

    // Update classes
    this.element.classList.toggle(
      this.options.loadingClass,
      this.state.loading
    );
    this.element.classList.toggle(
      this.options.disabledClass,
      this.state.disabled
    );
  }
}

/**
 * @class ProgressBar
 * @brief Progress bar component with animations and customizable styling
 */
export class ProgressBar extends BaseComponent {
  constructor(element, options = {}) {
    super(element, {
      min: 0,
      max: 100,
      value: 0,
      showText: true,
      textFormat: "{value}%",
      animationDuration: 300,
      ...options,
    });
  }

  setupDOM() {
    if (!this.element) return;

    this.element.classList.add("huntmaster-progress");

    // Create progress bar structure
    this.element.innerHTML = `
      <div class="progress-bar">
        <div class="progress-fill"></div>
        <div class="progress-text"></div>
      </div>
    `;

    this.fillElement = this.element.querySelector(".progress-fill");
    this.textElement = this.element.querySelector(".progress-text");
  }

  initializeState() {
    this.setState(
      {
        value: this.options.value,
        min: this.options.min,
        max: this.options.max,
      },
      { rerender: false }
    );
  }

  setValue(value, animate = true) {
    const clampedValue = Math.max(
      this.state.min,
      Math.min(this.state.max, value)
    );

    this.setState({ value: clampedValue });

    if (animate && this.fillElement) {
      this.fillElement.style.transition = `width ${this.options.animationDuration}ms ease-out`;
    }

    this.emit("valueChanged", { value: clampedValue });
  }

  setRange(min, max) {
    this.setState({ min, max });
  }

  performRender() {
    if (!this.fillElement || !this.textElement) return;

    const percentage =
      ((this.state.value - this.state.min) /
        (this.state.max - this.state.min)) *
      100;

    this.fillElement.style.width = `${percentage}%`;

    if (this.options.showText) {
      const text = this.options.textFormat
        .replace("{value}", this.state.value)
        .replace("{min}", this.state.min)
        .replace("{max}", this.state.max)
        .replace("{percentage}", Math.round(percentage));

      this.textElement.textContent = text;
    }
  }
}

/**
 * @class Toggle
 * @brief Toggle switch component with customizable styling and states
 */
export class Toggle extends BaseComponent {
  constructor(element, options = {}) {
    super(element, {
      checked: false,
      disabled: false,
      onText: "ON",
      offText: "OFF",
      changeHandler: null,
      ...options,
    });
  }

  setupDOM() {
    if (!this.element) return;

    this.element.classList.add("huntmaster-toggle");

    // Create toggle structure
    this.element.innerHTML = `
      <div class="toggle-track">
        <div class="toggle-thumb"></div>
        <span class="toggle-text"></span>
      </div>
    `;

    this.trackElement = this.element.querySelector(".toggle-track");
    this.thumbElement = this.element.querySelector(".toggle-thumb");
    this.textElement = this.element.querySelector(".toggle-text");
  }

  bindEvents() {
    if (this.element) {
      this.addEventListener(this.element, "click", this.handleClick);
      this.addEventListener(this.element, "keydown", this.handleKeyDown);
    }

    // Make focusable
    if (this.element && !this.element.hasAttribute("tabindex")) {
      this.element.setAttribute("tabindex", "0");
    }
  }

  initializeState() {
    this.setState(
      {
        checked: this.options.checked,
        disabled: this.options.disabled,
      },
      { rerender: false }
    );
  }

  handleClick(event) {
    if (this.state.disabled) {
      event.preventDefault();
      return false;
    }

    this.toggle();
  }

  handleKeyDown(event) {
    if (this.state.disabled) return;

    if (event.key === " " || event.key === "Enter") {
      event.preventDefault();
      this.toggle();
    }
  }

  toggle() {
    const newChecked = !this.state.checked;
    this.setChecked(newChecked);
  }

  setChecked(checked) {
    this.setState({ checked });

    this.emit("change", {
      checked,
      value: checked,
    });

    if (this.options.changeHandler) {
      this.options.changeHandler.call(this, checked);
    }
  }

  performRender() {
    if (!this.element) return;

    this.element.classList.toggle("checked", this.state.checked);
    this.element.classList.toggle("disabled", this.state.disabled);

    if (this.textElement) {
      this.textElement.textContent = this.state.checked
        ? this.options.onText
        : this.options.offText;
    }

    // Update ARIA attributes
    this.element.setAttribute("aria-checked", this.state.checked);
    this.element.setAttribute("aria-disabled", this.state.disabled);
  }
}

/**
 * @class Slider
 * @brief Range slider component with precise value control and visual feedback
 */
export class Slider extends BaseComponent {
  constructor(element, options = {}) {
    super(element, {
      min: 0,
      max: 100,
      value: 50,
      step: 1,
      showValue: true,
      valueFormat: "{value}",
      changeHandler: null,
      inputHandler: null,
      ...options,
    });
  }

  setupDOM() {
    if (!this.element) return;

    this.element.classList.add("huntmaster-slider");

    // Create slider structure
    this.element.innerHTML = `
      <div class="slider-track">
        <div class="slider-fill"></div>
        <div class="slider-thumb"></div>
      </div>
      <div class="slider-value"></div>
    `;

    this.trackElement = this.element.querySelector(".slider-track");
    this.fillElement = this.element.querySelector(".slider-fill");
    this.thumbElement = this.element.querySelector(".slider-thumb");
    this.valueElement = this.element.querySelector(".slider-value");

    // Make interactive
    this.element.setAttribute("tabindex", "0");
  }

  bindEvents() {
    if (!this.trackElement) return;

    this.addEventListener(this.trackElement, "mousedown", this.handleMouseDown);
    this.addEventListener(this.element, "keydown", this.handleKeyDown);

    // Touch events for mobile
    this.addEventListener(
      this.trackElement,
      "touchstart",
      this.handleTouchStart,
      { passive: false }
    );
  }

  initializeState() {
    this.setState(
      {
        value: this.options.value,
        min: this.options.min,
        max: this.options.max,
        step: this.options.step,
        dragging: false,
      },
      { rerender: false }
    );
  }

  handleMouseDown(event) {
    if (this.state.disabled) return;

    event.preventDefault();
    this.startDrag(event);

    // Add document listeners for drag
    this.documentMouseMoveHandler = this.handleMouseMove.bind(this);
    this.documentMouseUpHandler = this.handleMouseUp.bind(this);

    document.addEventListener("mousemove", this.documentMouseMoveHandler);
    document.addEventListener("mouseup", this.documentMouseUpHandler);
  }

  handleMouseMove(event) {
    if (!this.state.dragging) return;

    this.updateValueFromEvent(event);
  }

  handleMouseUp(event) {
    this.stopDrag();

    // Remove document listeners
    document.removeEventListener("mousemove", this.documentMouseMoveHandler);
    document.removeEventListener("mouseup", this.documentMouseUpHandler);
  }

  handleTouchStart(event) {
    if (this.state.disabled) return;

    event.preventDefault();
    this.startDrag(event.touches[0]);

    // Add document listeners for touch
    this.documentTouchMoveHandler = this.handleTouchMove.bind(this);
    this.documentTouchEndHandler = this.handleTouchEnd.bind(this);

    document.addEventListener("touchmove", this.documentTouchMoveHandler, {
      passive: false,
    });
    document.addEventListener("touchend", this.documentTouchEndHandler);
  }

  handleTouchMove(event) {
    if (!this.state.dragging) return;

    event.preventDefault();
    this.updateValueFromEvent(event.touches[0]);
  }

  handleTouchEnd(event) {
    this.stopDrag();

    // Remove document listeners
    document.removeEventListener("touchmove", this.documentTouchMoveHandler);
    document.removeEventListener("touchend", this.documentTouchEndHandler);
  }

  handleKeyDown(event) {
    if (this.state.disabled) return;

    let newValue = this.state.value;
    const step = this.state.step;

    switch (event.key) {
      case "ArrowLeft":
      case "ArrowDown":
        newValue = Math.max(this.state.min, this.state.value - step);
        break;
      case "ArrowRight":
      case "ArrowUp":
        newValue = Math.min(this.state.max, this.state.value + step);
        break;
      case "Home":
        newValue = this.state.min;
        break;
      case "End":
        newValue = this.state.max;
        break;
      default:
        return;
    }

    event.preventDefault();
    this.setValue(newValue);
  }

  startDrag(event) {
    this.setState({ dragging: true });
    this.updateValueFromEvent(event);
    this.emit("dragStart", { value: this.state.value });
  }

  stopDrag() {
    this.setState({ dragging: false });
    this.emit("dragEnd", { value: this.state.value });

    if (this.options.changeHandler) {
      this.options.changeHandler.call(this, this.state.value);
    }
  }

  updateValueFromEvent(event) {
    if (!this.trackElement) return;

    const rect = this.trackElement.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const percentage = Math.max(0, Math.min(1, x / rect.width));

    const rawValue =
      this.state.min + percentage * (this.state.max - this.state.min);
    const steppedValue =
      Math.round(rawValue / this.state.step) * this.state.step;
    const clampedValue = Math.max(
      this.state.min,
      Math.min(this.state.max, steppedValue)
    );

    this.setValue(clampedValue, false);

    if (this.options.inputHandler) {
      this.options.inputHandler.call(this, clampedValue);
    }
  }

  setValue(value, triggerChange = true) {
    const clampedValue = Math.max(
      this.state.min,
      Math.min(this.state.max, value)
    );

    this.setState({ value: clampedValue });

    this.emit("input", { value: clampedValue });

    if (triggerChange) {
      this.emit("change", { value: clampedValue });

      if (this.options.changeHandler) {
        this.options.changeHandler.call(this, clampedValue);
      }
    }
  }

  performRender() {
    if (!this.fillElement || !this.thumbElement) return;

    const percentage =
      ((this.state.value - this.state.min) /
        (this.state.max - this.state.min)) *
      100;

    this.fillElement.style.width = `${percentage}%`;
    this.thumbElement.style.left = `${percentage}%`;

    if (this.valueElement && this.options.showValue) {
      const text = this.options.valueFormat.replace(
        "{value}",
        this.state.value
      );
      this.valueElement.textContent = text;
    }

    // Update ARIA attributes
    this.element.setAttribute("aria-valuemin", this.state.min);
    this.element.setAttribute("aria-valuemax", this.state.max);
    this.element.setAttribute("aria-valuenow", this.state.value);
  }
}

/**
 * Component factory for creating components from DOM elements
 */
export class ComponentFactory {
  static components = new Map([
    ["button", Button],
    ["progress", ProgressBar],
    ["toggle", Toggle],
    ["slider", Slider],
  ]);

  /**
   * Create component from element based on data attributes
   */
  static createFromElement(element, options = {}) {
    const componentType = element.dataset.component;

    if (!componentType) {
      return new BaseComponent(element, options);
    }

    const ComponentClass = this.components.get(componentType);

    if (!ComponentClass) {
      console.warn(`Unknown component type: ${componentType}`);
      return new BaseComponent(element, options);
    }

    return new ComponentClass(element, options);
  }

  /**
   * Register new component type
   */
  static register(name, ComponentClass) {
    this.components.set(name, ComponentClass);
  }

  /**
   * Initialize all components in a container
   */
  static initializeAll(container = document, options = {}) {
    const elements = container.querySelectorAll("[data-component]");
    const components = [];

    elements.forEach((element) => {
      try {
        const component = this.createFromElement(element, options);
        components.push(component);
      } catch (error) {
        console.error("Failed to initialize component:", error);
      }
    });

    return components;
  }
}

export default {
  BaseComponent,
  Button,
  ProgressBar,
  Toggle,
  Slider,
  ComponentFactory,
};
