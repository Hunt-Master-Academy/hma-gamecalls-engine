/**
 * @file ui/index.js
 * @brief Comprehensive UI Components and Interface Management Modules
 *
 * This index provides access to all user interface modules including:
 * - Reusable UI components (buttons, sliders, etc.)
 * - Responsive layout management and grid systems
 * - WCAG 2.1 AA accessibility compliance systems
 * - Advanced theme and styling management
 * - Mobile optimization and touch interactions
 * - Advanced gesture recognition and handling
 * - Data visualization and visual feedback systems
 * - User interaction and feedback systems
 *
 * @author Huntmaster Engine Team
 * @version 2.0 - Complete UI System
 * @date July 24, 2025
 */

/**
 * UI Module Exports
 *
 * Comprehensive user interface components for the Huntmaster Engine web application.
 * Includes basic controls, advanced components, and interactive elements.
 */

// Core UI Components
export { default as UIManager } from "./UIManager.js";
export { default as AudioPlayer } from "./AudioPlayer.js";
export { default as RecordingControls } from "./RecordingControls.js";
export { default as WaveformDisplay } from "./WaveformDisplay.js";

// Advanced UI Components (Phase 2.2E)
export { default as AdvancedControls } from "./AdvancedControls.js";
export { default as InteractiveWaveform } from "./InteractiveWaveform.js";
export { default as AudioProcessor } from "./AudioProcessor.js";
export { default as EffectsPanel } from "./EffectsPanel.js";
export { default as PresetManager } from "./PresetManager.js";

// Interactive User Experience Components (Phase 2.3B)
export { default as GestureController } from "./GestureController.js";
export { default as KeyboardShortcuts } from "./KeyboardShortcuts.js";
export { default as VoiceCommands } from "./VoiceCommands.js";
