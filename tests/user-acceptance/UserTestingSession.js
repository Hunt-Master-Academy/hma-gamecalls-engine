/**
 * @file UserTestingSession.js
 * @brief User Acceptance Testing Session Management System
 *
 * This system manages user testing sessions, collects feedback,
 * and provides analytics for improving user experience and
 * application usability.
 *
 * @author Huntmaster Engine Team
 * @version 2.0
 * @date July 24, 2025
 */

// TODO: Phase 3.2 - User Acceptance Testing Framework - COMPREHENSIVE FILE TODO
// =============================================================================

// TODO 3.2.1: User Testing Session Management
// -------------------------------------------
/**
 * TODO: Implement comprehensive user testing session management with:
 * [ ] Anonymous user session tracking and identification
 * [ ] Test scenario assignment and randomization
 * [ ] User action logging and behavior analytics
 * [ ] Session state management and persistence
 * [ ] Real-time feedback collection and analysis
 * [ ] User consent management and privacy compliance
 * [ ] Cross-device session synchronization
 * [ ] Test completion tracking and validation
 * [ ] Automated report generation and insights
 * [ ] Integration with analytics platforms
 */

class UserTestingSession {
  constructor(config = {}) {
    this.sessionId = this.generateSessionId();
    this.config = {
      enableAnalytics: config.enableAnalytics ?? true,
      enableFeedback: config.enableFeedback ?? true,
      enableRecording: config.enableRecording ?? false,
      privacyMode: config.privacyMode ?? "anonymous",
      testScenarios: config.testScenarios ?? [],
      maxSessionDuration: config.maxSessionDuration ?? 30 * 60 * 1000, // 30 minutes
      ...config,
    };

    this.startTime = Date.now();
    this.actions = [];
    this.feedback = [];
    this.metrics = {};
    this.currentTask = null;
    this.isActive = false;
    this.consentGiven = false;

    // TODO: Initialize analytics tracking
    this.analytics = new UserAnalytics(this.sessionId, this.config);

    // TODO: Initialize feedback system
    this.feedbackSystem = new FeedbackSystem(this.sessionId, this.config);
  }

  // TODO 3.2.2: Session Initialization and Consent Management
  // ---------------------------------------------------------
  async initialize() {
    // TODO: Request user consent for testing participation
    if (!this.consentGiven) {
      const consent = await this.requestUserConsent();
      if (!consent) {
        throw new Error("User consent required for testing session");
      }
      this.consentGiven = true;
    }

    // TODO: Initialize session tracking
    this.isActive = true;
    this.analytics.startTracking();

    // TODO: Log session initialization
    this.logAction("session_initialized", {
      sessionId: this.sessionId,
      timestamp: this.startTime,
      userAgent:
        typeof navigator !== "undefined" ? navigator.userAgent : "unknown",
      screenResolution: this.getScreenResolution(),
      viewport: this.getViewportSize(),
      deviceType: this.detectDeviceType(),
    });

    // TODO: Assign test scenario if configured
    if (this.config.testScenarios.length > 0) {
      await this.assignTestScenario();
    }

    console.log(`User testing session initialized: ${this.sessionId}`);
    return this.sessionId;
  }

  // TODO 3.2.3: Test Scenario Assignment and Management
  // ---------------------------------------------------
  async assignTestScenario() {
    // TODO: Select scenario based on configuration
    const scenario = this.selectScenario();
    this.currentTask = {
      scenarioId: scenario.id,
      name: scenario.name,
      description: scenario.description,
      steps: scenario.steps,
      currentStep: 0,
      startTime: Date.now(),
      completed: false,
      timeLimit: scenario.timeLimit || null,
    };

    // TODO: Log scenario assignment
    this.logAction("scenario_assigned", {
      scenarioId: scenario.id,
      scenarioName: scenario.name,
      totalSteps: scenario.steps.length,
    });

    // TODO: Display scenario instructions to user
    if (this.config.enableFeedback) {
      await this.feedbackSystem.showScenarioInstructions(scenario);
    }

    return scenario;
  }

  selectScenario() {
    // TODO: Implement scenario selection logic
    const availableScenarios = this.config.testScenarios;

    // TODO: Random assignment for A/B testing
    const randomIndex = Math.floor(Math.random() * availableScenarios.length);
    return availableScenarios[randomIndex];
  }

  // TODO 3.2.4: User Action Logging and Behavior Analytics
  // ------------------------------------------------------
  logAction(actionType, actionData = {}) {
    const actionEntry = {
      id: this.generateActionId(),
      sessionId: this.sessionId,
      type: actionType,
      timestamp: Date.now(),
      relativeTime: Date.now() - this.startTime,
      data: actionData,
      currentTask: this.currentTask
        ? {
            scenarioId: this.currentTask.scenarioId,
            currentStep: this.currentTask.currentStep,
          }
        : null,
    };

    this.actions.push(actionEntry);

    // TODO: Send to analytics if enabled
    if (this.config.enableAnalytics) {
      this.analytics.trackAction(actionEntry);
    }

    // TODO: Check for task completion
    if (this.currentTask && actionType === "task_step_completed") {
      this.checkTaskProgress(actionData);
    }

    // TODO: Auto-save session data periodically
    if (this.actions.length % 10 === 0) {
      this.saveSessionData();
    }
  }

  // TODO: Specific action logging methods
  logButtonClick(buttonId, context = {}) {
    this.logAction("button_click", {
      buttonId,
      context,
      coordinates: context.coordinates || null,
      elementText: context.elementText || null,
    });
  }

  logPageNavigation(fromPage, toPage, method = "click") {
    this.logAction("page_navigation", {
      fromPage,
      toPage,
      method,
      loadTime: context.loadTime || null,
    });
  }

  logAudioInteraction(interactionType, details = {}) {
    this.logAction("audio_interaction", {
      interactionType, // 'play', 'pause', 'record', 'stop', 'upload'
      audioDetails: details,
      processingTime: details.processingTime || null,
      audioLength: details.audioLength || null,
    });
  }

  logErrorEncountered(errorType, errorDetails = {}) {
    this.logAction("error_encountered", {
      errorType,
      errorMessage: errorDetails.message || null,
      errorStack: errorDetails.stack || null,
      context: errorDetails.context || null,
      severity: errorDetails.severity || "medium",
    });
  }

  logUserFeedback(feedbackType, feedbackData = {}) {
    this.logAction("user_feedback", {
      feedbackType, // 'rating', 'comment', 'survey_response'
      feedbackData,
      promptId: feedbackData.promptId || null,
    });
  }

  // TODO 3.2.5: Task Progress Tracking
  // ----------------------------------
  checkTaskProgress(actionData) {
    if (!this.currentTask) return;

    const currentStep = this.currentTask.steps[this.currentTask.currentStep];

    // TODO: Check if current step is completed
    if (this.isStepCompleted(currentStep, actionData)) {
      this.currentTask.currentStep++;

      // TODO: Log step completion
      this.logAction("task_step_completed", {
        stepIndex: this.currentTask.currentStep - 1,
        stepName: currentStep.name,
        timeSpent:
          Date.now() - (currentStep.startTime || this.currentTask.startTime),
      });

      // TODO: Check if entire task is completed
      if (this.currentTask.currentStep >= this.currentTask.steps.length) {
        this.completeCurrentTask();
      } else {
        // TODO: Move to next step
        const nextStep = this.currentTask.steps[this.currentTask.currentStep];
        nextStep.startTime = Date.now();

        if (this.config.enableFeedback) {
          this.feedbackSystem.showStepInstructions(nextStep);
        }
      }
    }
  }

  isStepCompleted(step, actionData) {
    // TODO: Implement step completion logic based on step criteria
    if (!step.completionCriteria) return false;

    const criteria = step.completionCriteria;

    switch (criteria.type) {
      case "button_click":
        return actionData.buttonId === criteria.targetButton;
      case "page_reach":
        return actionData.toPage === criteria.targetPage;
      case "audio_processed":
        return actionData.interactionType === "process_complete";
      case "time_spent":
        return Date.now() - step.startTime >= criteria.minimumTime;
      default:
        return false;
    }
  }

  completeCurrentTask() {
    if (!this.currentTask) return;

    this.currentTask.completed = true;
    this.currentTask.completionTime = Date.now();
    this.currentTask.totalTime =
      this.currentTask.completionTime - this.currentTask.startTime;

    // TODO: Log task completion
    this.logAction("task_completed", {
      scenarioId: this.currentTask.scenarioId,
      totalTime: this.currentTask.totalTime,
      stepsCompleted: this.currentTask.steps.length,
      success: true,
    });

    // TODO: Collect post-task feedback
    if (this.config.enableFeedback) {
      this.feedbackSystem.showPostTaskSurvey(this.currentTask);
    }

    // TODO: Check if there are more scenarios to assign
    this.assignNextScenarioIfAvailable();
  }

  async assignNextScenarioIfAvailable() {
    // TODO: Check if more scenarios should be assigned
    const completedScenarios = this.actions
      .filter((action) => action.type === "task_completed")
      .map((action) => action.data.scenarioId);

    const remainingScenarios = this.config.testScenarios.filter(
      (scenario) => !completedScenarios.includes(scenario.id)
    );

    if (remainingScenarios.length > 0 && this.shouldContinueTesting()) {
      // TODO: Brief pause before next scenario
      await new Promise((resolve) => setTimeout(resolve, 2000));

      this.config.testScenarios = remainingScenarios;
      await this.assignTestScenario();
    } else {
      // TODO: All scenarios completed or session should end
      await this.endSession();
    }
  }

  shouldContinueTesting() {
    const sessionDuration = Date.now() - this.startTime;
    const maxDuration = this.config.maxSessionDuration;

    return sessionDuration < maxDuration && this.isActive;
  }

  // TODO 3.2.6: Real-time Feedback Collection
  // -----------------------------------------
  async collectFeedback(feedbackType, promptData = {}) {
    const feedbackId = this.generateFeedbackId();

    const feedbackRequest = {
      id: feedbackId,
      sessionId: this.sessionId,
      type: feedbackType,
      timestamp: Date.now(),
      promptData,
      currentTask: this.currentTask
        ? {
            scenarioId: this.currentTask.scenarioId,
            currentStep: this.currentTask.currentStep,
          }
        : null,
    };

    // TODO: Display feedback prompt to user
    const response = await this.feedbackSystem.displayPrompt(feedbackRequest);

    if (response) {
      const feedbackEntry = {
        ...feedbackRequest,
        response,
        responseTime: Date.now() - feedbackRequest.timestamp,
      };

      this.feedback.push(feedbackEntry);
      this.logUserFeedback(feedbackType, feedbackEntry);

      return feedbackEntry;
    }

    return null;
  }

  // TODO: Specific feedback collection methods
  async collectRating(prompt, scale = 5) {
    return await this.collectFeedback("rating", {
      prompt,
      scale,
      responseType: "numeric",
    });
  }

  async collectComment(prompt) {
    return await this.collectFeedback("comment", {
      prompt,
      responseType: "text",
      maxLength: 500,
    });
  }

  async collectSurveyResponse(surveyQuestions) {
    return await this.collectFeedback("survey", {
      questions: surveyQuestions,
      responseType: "structured",
    });
  }

  // TODO 3.2.7: Session Analytics and Metrics
  // -----------------------------------------
  calculateSessionMetrics() {
    const sessionDuration = Date.now() - this.startTime;
    const actionCount = this.actions.length;

    // TODO: Calculate interaction metrics
    const interactions = {
      buttonClicks: this.actions.filter((a) => a.type === "button_click")
        .length,
      pageNavigations: this.actions.filter((a) => a.type === "page_navigation")
        .length,
      audioInteractions: this.actions.filter(
        (a) => a.type === "audio_interaction"
      ).length,
      errorsEncountered: this.actions.filter(
        (a) => a.type === "error_encountered"
      ).length,
    };

    // TODO: Calculate task metrics
    const taskMetrics = this.currentTask
      ? {
          scenarioId: this.currentTask.scenarioId,
          stepsCompleted: this.currentTask.currentStep,
          totalSteps: this.currentTask.steps.length,
          completionPercentage:
            (this.currentTask.currentStep / this.currentTask.steps.length) *
            100,
          timeSpent: Date.now() - this.currentTask.startTime,
          completed: this.currentTask.completed,
        }
      : null;

    // TODO: Calculate performance metrics
    const performanceMetrics = {
      averageActionInterval:
        actionCount > 1 ? sessionDuration / (actionCount - 1) : 0,
      errorRate: interactions.errorsEncountered / actionCount,
      engagementScore: this.calculateEngagementScore(),
    };

    return {
      sessionId: this.sessionId,
      duration: sessionDuration,
      actionCount,
      interactions,
      taskMetrics,
      performanceMetrics,
      feedbackCount: this.feedback.length,
      timestamp: Date.now(),
    };
  }

  calculateEngagementScore() {
    // TODO: Implement engagement scoring algorithm
    const factors = {
      actionFrequency: Math.min(1, this.actions.length / 50), // Normalize to 0-1
      taskProgress: this.currentTask
        ? this.currentTask.currentStep / this.currentTask.steps.length
        : 0,
      feedbackProvided: Math.min(1, this.feedback.length / 3), // Normalize to 0-1
      errorRate:
        1 -
        Math.min(
          1,
          this.actions.filter((a) => a.type === "error_encountered").length / 5
        ),
    };

    const weights = {
      actionFrequency: 0.3,
      taskProgress: 0.4,
      feedbackProvided: 0.2,
      errorRate: 0.1,
    };

    return Object.keys(factors).reduce((score, factor) => {
      return score + factors[factor] * weights[factor];
    }, 0);
  }

  // TODO 3.2.8: Session Data Management
  // -----------------------------------
  saveSessionData() {
    const sessionData = {
      sessionId: this.sessionId,
      startTime: this.startTime,
      actions: this.actions,
      feedback: this.feedback,
      currentTask: this.currentTask,
      metrics: this.calculateSessionMetrics(),
      config: {
        privacyMode: this.config.privacyMode,
        testScenarios: this.config.testScenarios.map((s) => s.id),
      },
    };

    // TODO: Save to local storage (anonymized)
    if (typeof localStorage !== "undefined") {
      try {
        localStorage.setItem(
          `usertest_${this.sessionId}`,
          JSON.stringify(sessionData)
        );
      } catch (error) {
        console.warn("Could not save session data to localStorage:", error);
      }
    }

    // TODO: Send to analytics service (if consent given)
    if (this.config.enableAnalytics && this.consentGiven) {
      this.analytics.saveSessionData(sessionData);
    }
  }

  loadSessionData(sessionId) {
    // TODO: Load session data from storage
    if (typeof localStorage !== "undefined") {
      try {
        const data = localStorage.getItem(`usertest_${sessionId}`);
        if (data) {
          const sessionData = JSON.parse(data);
          this.actions = sessionData.actions || [];
          this.feedback = sessionData.feedback || [];
          this.currentTask = sessionData.currentTask || null;
          return sessionData;
        }
      } catch (error) {
        console.warn("Could not load session data from localStorage:", error);
      }
    }
    return null;
  }

  // TODO 3.2.9: Session Termination and Cleanup
  // --------------------------------------------
  async endSession(reason = "user_completed") {
    if (!this.isActive) return;

    this.isActive = false;
    const endTime = Date.now();
    const totalDuration = endTime - this.startTime;

    // TODO: Log session end
    this.logAction("session_ended", {
      reason,
      totalDuration,
      totalActions: this.actions.length,
      totalFeedback: this.feedback.length,
      taskCompleted: this.currentTask?.completed || false,
    });

    // TODO: Final metrics calculation
    const finalMetrics = this.calculateSessionMetrics();
    this.metrics = finalMetrics;

    // TODO: Show final feedback survey
    if (this.config.enableFeedback) {
      await this.feedbackSystem.showFinalSurvey({
        sessionDuration: totalDuration,
        taskCompleted: this.currentTask?.completed || false,
      });
    }

    // TODO: Save final session data
    this.saveSessionData();

    // TODO: Generate session report
    const report = await this.generateSessionReport();

    // TODO: Cleanup analytics tracking
    this.analytics.stopTracking();

    console.log(
      `User testing session ended: ${this.sessionId}, Duration: ${totalDuration}ms`
    );
    return report;
  }

  async generateSessionReport() {
    // TODO: Generate comprehensive session report
    const report = {
      sessionSummary: {
        sessionId: this.sessionId,
        duration: Date.now() - this.startTime,
        startTime: new Date(this.startTime).toISOString(),
        endTime: new Date().toISOString(),
        userAgent:
          typeof navigator !== "undefined" ? navigator.userAgent : "unknown",
      },
      taskPerformance: this.currentTask
        ? {
            scenarioId: this.currentTask.scenarioId,
            scenarioName: this.currentTask.name,
            completed: this.currentTask.completed,
            stepsCompleted: this.currentTask.currentStep,
            totalSteps: this.currentTask.steps.length,
            completionRate:
              (this.currentTask.currentStep / this.currentTask.steps.length) *
              100,
            timeSpent: this.currentTask.completionTime
              ? this.currentTask.completionTime - this.currentTask.startTime
              : Date.now() - this.currentTask.startTime,
          }
        : null,
      actionAnalysis: {
        totalActions: this.actions.length,
        actionBreakdown: this.getActionBreakdown(),
        errorCount: this.actions.filter((a) => a.type === "error_encountered")
          .length,
        mostCommonActions: this.getMostCommonActions(),
      },
      feedbackSummary: {
        totalFeedback: this.feedback.length,
        ratings: this.feedback.filter((f) => f.type === "rating"),
        comments: this.feedback.filter((f) => f.type === "comment"),
        surveyResponses: this.feedback.filter((f) => f.type === "survey"),
      },
      performanceMetrics: this.metrics,
      recommendations: this.generateRecommendations(),
    };

    return report;
  }

  getActionBreakdown() {
    const breakdown = {};
    for (const action of this.actions) {
      breakdown[action.type] = (breakdown[action.type] || 0) + 1;
    }
    return breakdown;
  }

  getMostCommonActions() {
    const breakdown = this.getActionBreakdown();
    return Object.entries(breakdown)
      .sort(([, a], [, b]) => b - a)
      .slice(0, 5)
      .map(([type, count]) => ({ type, count }));
  }

  generateRecommendations() {
    // TODO: AI-powered recommendations based on session data
    const recommendations = [];

    // TODO: Task completion recommendations
    if (this.currentTask && !this.currentTask.completed) {
      recommendations.push({
        type: "task_completion",
        priority: "high",
        message:
          "User did not complete the assigned task. Consider simplifying the workflow or providing better guidance.",
      });
    }

    // TODO: Error rate recommendations
    const errorRate =
      this.actions.filter((a) => a.type === "error_encountered").length /
      this.actions.length;
    if (errorRate > 0.1) {
      recommendations.push({
        type: "error_reduction",
        priority: "medium",
        message: `High error rate detected (${(errorRate * 100).toFixed(
          1
        )}%). Review error sources and improve error handling.`,
      });
    }

    // TODO: Engagement recommendations
    const engagementScore = this.calculateEngagementScore();
    if (engagementScore < 0.6) {
      recommendations.push({
        type: "engagement",
        priority: "medium",
        message:
          "Low engagement detected. Consider improving UI/UX or task design to increase user engagement.",
      });
    }

    return recommendations;
  }

  // TODO 3.2.10: Utility Methods
  // ----------------------------
  generateSessionId() {
    return (
      "session_" + Date.now() + "_" + Math.random().toString(36).substr(2, 9)
    );
  }

  generateActionId() {
    return (
      "action_" + Date.now() + "_" + Math.random().toString(36).substr(2, 6)
    );
  }

  generateFeedbackId() {
    return (
      "feedback_" + Date.now() + "_" + Math.random().toString(36).substr(2, 6)
    );
  }

  async requestUserConsent() {
    // TODO: Display consent dialog to user
    if (typeof window !== "undefined" && this.config.enableFeedback) {
      return new Promise((resolve) => {
        const consentDialog = document.createElement("div");
        consentDialog.innerHTML = `
                    <div style="position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); z-index: 10000; display: flex; align-items: center; justify-content: center;">
                        <div style="background: white; padding: 20px; border-radius: 8px; max-width: 500px; margin: 20px;">
                            <h3>User Testing Participation</h3>
                            <p>We would like to collect anonymous usage data to improve our application. Your participation will help us create a better user experience.</p>
                            <p><strong>What we collect:</strong></p>
                            <ul>
                                <li>Anonymous interaction data (button clicks, page navigation)</li>
                                <li>Task completion times and success rates</li>
                                <li>Optional feedback and ratings</li>
                                <li>Technical performance metrics</li>
                            </ul>
                            <p><strong>What we don't collect:</strong></p>
                            <ul>
                                <li>Personal information or account details</li>
                                <li>Audio recordings or uploaded content</li>
                                <li>Sensitive data or private information</li>
                            </ul>
                            <div style="margin-top: 20px; text-align: right;">
                                <button id="consent-decline" style="margin-right: 10px; padding: 8px 16px; border: 1px solid #ccc; background: white; border-radius: 4px;">Decline</button>
                                <button id="consent-accept" style="padding: 8px 16px; background: #007bff; color: white; border: none; border-radius: 4px;">Accept</button>
                            </div>
                        </div>
                    </div>
                `;

        document.body.appendChild(consentDialog);

        document.getElementById("consent-accept").onclick = () => {
          document.body.removeChild(consentDialog);
          resolve(true);
        };

        document.getElementById("consent-decline").onclick = () => {
          document.body.removeChild(consentDialog);
          resolve(false);
        };
      });
    }

    // TODO: Default to false if no UI available
    return false;
  }

  getScreenResolution() {
    if (typeof screen !== "undefined") {
      return {
        width: screen.width,
        height: screen.height,
        colorDepth: screen.colorDepth,
        pixelRatio: window.devicePixelRatio || 1,
      };
    }
    return null;
  }

  getViewportSize() {
    if (typeof window !== "undefined") {
      return {
        width: window.innerWidth,
        height: window.innerHeight,
      };
    }
    return null;
  }

  detectDeviceType() {
    if (typeof navigator === "undefined") return "unknown";

    const userAgent = navigator.userAgent.toLowerCase();

    if (/mobile|android|iphone|ipad|phone/i.test(userAgent)) {
      return "mobile";
    } else if (/tablet|ipad/i.test(userAgent)) {
      return "tablet";
    } else {
      return "desktop";
    }
  }
}

// TODO 3.2.11: User Analytics System
// ----------------------------------
class UserAnalytics {
  constructor(sessionId, config) {
    this.sessionId = sessionId;
    this.config = config;
    this.isTracking = false;
    this.eventQueue = [];
    this.batchSize = 10;
    this.flushInterval = 30000; // 30 seconds
    this.flushTimer = null;
  }

  startTracking() {
    this.isTracking = true;

    // TODO: Start periodic flush of events
    this.flushTimer = setInterval(() => {
      this.flushEvents();
    }, this.flushInterval);
  }

  stopTracking() {
    this.isTracking = false;

    if (this.flushTimer) {
      clearInterval(this.flushTimer);
      this.flushTimer = null;
    }

    // TODO: Final flush of remaining events
    this.flushEvents();
  }

  trackAction(actionEntry) {
    if (!this.isTracking) return;

    // TODO: Add to event queue
    this.eventQueue.push({
      type: "user_action",
      sessionId: this.sessionId,
      data: actionEntry,
      clientTimestamp: Date.now(),
    });

    // TODO: Flush if batch size reached
    if (this.eventQueue.length >= this.batchSize) {
      this.flushEvents();
    }
  }

  async flushEvents() {
    if (this.eventQueue.length === 0) return;

    const events = [...this.eventQueue];
    this.eventQueue = [];

    try {
      // TODO: Send events to analytics service
      await this.sendEventsToService(events);
    } catch (error) {
      console.warn("Failed to send analytics events:", error);
      // TODO: Re-queue events for retry
      this.eventQueue = [...events, ...this.eventQueue];
    }
  }

  async sendEventsToService(events) {
    // TODO: Implement actual analytics service integration
    console.log("Sending analytics events:", events.length);

    // TODO: Example implementation with fetch
    if (typeof fetch !== "undefined") {
      const response = await fetch("/api/user-analytics", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          events,
          sessionId: this.sessionId,
          timestamp: Date.now(),
        }),
      });

      if (!response.ok) {
        throw new Error(`Analytics service error: ${response.status}`);
      }
    }
  }

  async saveSessionData(sessionData) {
    // TODO: Save complete session data to analytics service
    try {
      if (typeof fetch !== "undefined") {
        await fetch("/api/user-sessions", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify(sessionData),
        });
      }
    } catch (error) {
      console.warn("Failed to save session data:", error);
    }
  }
}

// TODO 3.2.12: Feedback System
// ----------------------------
class FeedbackSystem {
  constructor(sessionId, config) {
    this.sessionId = sessionId;
    this.config = config;
    this.activePrompts = new Map();
  }

  async showScenarioInstructions(scenario) {
    if (!this.config.enableFeedback) return;

    // TODO: Display scenario instructions to user
    return new Promise((resolve) => {
      const instructionDialog = this.createInstructionDialog(scenario);
      document.body.appendChild(instructionDialog);

      // TODO: Auto-dismiss after reading time
      setTimeout(() => {
        if (document.body.contains(instructionDialog)) {
          document.body.removeChild(instructionDialog);
        }
        resolve();
      }, 10000); // 10 seconds
    });
  }

  createInstructionDialog(scenario) {
    const dialog = document.createElement("div");
    dialog.innerHTML = `
            <div style="position: fixed; top: 20px; right: 20px; background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 8px; padding: 16px; max-width: 300px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); z-index: 1000;">
                <h4 style="margin-top: 0; color: #495057;">Test Scenario</h4>
                <h5 style="color: #007bff;">${scenario.name}</h5>
                <p style="font-size: 14px; color: #6c757d; margin-bottom: 12px;">${
                  scenario.description
                }</p>
                <div style="font-size: 12px; color: #868e96;">
                    <strong>Steps:</strong>
                    <ol style="padding-left: 16px; margin: 4px 0;">
                        ${scenario.steps
                          .map((step) => `<li>${step.name}</li>`)
                          .join("")}
                    </ol>
                </div>
                <button onclick="this.parentElement.parentElement.remove()" style="float: right; margin-top: 8px; padding: 4px 8px; background: #007bff; color: white; border: none; border-radius: 4px; font-size: 12px;">Got it</button>
            </div>
        `;
    return dialog;
  }

  async displayPrompt(feedbackRequest) {
    if (!this.config.enableFeedback) return null;

    const promptId = feedbackRequest.id;

    return new Promise((resolve) => {
      const prompt = this.createFeedbackPrompt(feedbackRequest, resolve);
      this.activePrompts.set(promptId, prompt);
      document.body.appendChild(prompt);

      // TODO: Auto-timeout after 60 seconds
      setTimeout(() => {
        if (this.activePrompts.has(promptId)) {
          this.closePrompt(promptId);
          resolve(null);
        }
      }, 60000);
    });
  }

  createFeedbackPrompt(feedbackRequest, resolve) {
    const { type, promptData } = feedbackRequest;
    let promptHTML = "";

    switch (type) {
      case "rating":
        promptHTML = this.createRatingPrompt(promptData, resolve);
        break;
      case "comment":
        promptHTML = this.createCommentPrompt(promptData, resolve);
        break;
      case "survey":
        promptHTML = this.createSurveyPrompt(promptData, resolve);
        break;
      default:
        promptHTML = "<p>Unknown feedback type</p>";
    }

    const prompt = document.createElement("div");
    prompt.innerHTML = `
            <div style="position: fixed; bottom: 20px; right: 20px; background: white; border: 1px solid #dee2e6; border-radius: 8px; padding: 16px; max-width: 350px; box-shadow: 0 4px 8px rgba(0,0,0,0.15); z-index: 1001;">
                ${promptHTML}
                <button onclick="this.closest('[style*=\"position: fixed\"]').remove()" style="position: absolute; top: 8px; right: 8px; background: none; border: none; font-size: 18px; color: #6c757d; cursor: pointer;">×</button>
            </div>
        `;

    return prompt;
  }

  createRatingPrompt(promptData, resolve) {
    const { prompt, scale = 5 } = promptData;
    const stars = Array.from({ length: scale }, (_, i) => i + 1);

    return `
            <h4 style="margin-top: 0; font-size: 16px;">Quick Feedback</h4>
            <p style="font-size: 14px; color: #495057; margin-bottom: 12px;">${prompt}</p>
            <div style="text-align: center; margin-bottom: 16px;">
                ${stars
                  .map(
                    (rating) => `
                    <button onclick="window.submitRating(${rating})"
                            style="background: none; border: none; font-size: 24px; color: #ddd; cursor: pointer; margin: 0 2px;"
                            onmouseover="this.style.color='#ffc107'"
                            onmouseout="this.style.color='#ddd'">★</button>
                `
                  )
                  .join("")}
            </div>
            <script>
                window.submitRating = function(rating) {
                    ${resolve.toString()}({ rating: rating, timestamp: Date.now() });
                    document.querySelector('[style*="position: fixed"]').remove();
                };
            </script>
        `;
  }

  createCommentPrompt(promptData, resolve) {
    const { prompt } = promptData;

    return `
            <h4 style="margin-top: 0; font-size: 16px;">Your Feedback</h4>
            <p style="font-size: 14px; color: #495057; margin-bottom: 12px;">${prompt}</p>
            <textarea id="feedback-comment" placeholder="Share your thoughts..."
                      style="width: 100%; height: 80px; padding: 8px; border: 1px solid #ced4da; border-radius: 4px; font-size: 14px; resize: vertical;"></textarea>
            <div style="text-align: right; margin-top: 12px;">
                <button onclick="window.submitComment()"
                        style="padding: 6px 12px; background: #007bff; color: white; border: none; border-radius: 4px; font-size: 14px;">Submit</button>
            </div>
            <script>
                window.submitComment = function() {
                    const comment = document.getElementById('feedback-comment').value;
                    if (comment.trim()) {
                        ${resolve.toString()}({ comment: comment, timestamp: Date.now() });
                    } else {
                        ${resolve.toString()}(null);
                    }
                    document.querySelector('[style*="position: fixed"]').remove();
                };
            </script>
        `;
  }

  createSurveyPrompt(promptData, resolve) {
    const { questions } = promptData;

    const questionsHTML = questions
      .map((q, index) => {
        switch (q.type) {
          case "multiple-choice":
            return `
                        <div style="margin-bottom: 16px;">
                            <p style="font-size: 14px; font-weight: 500; margin-bottom: 8px;">${
                              q.question
                            }</p>
                            ${q.options
                              .map(
                                (option, optIndex) => `
                                <label style="display: block; margin-bottom: 4px; font-size: 13px;">
                                    <input type="radio" name="q${index}" value="${option}" style="margin-right: 6px;">
                                    ${option}
                                </label>
                            `
                              )
                              .join("")}
                        </div>
                    `;
          case "yes-no":
            return `
                        <div style="margin-bottom: 16px;">
                            <p style="font-size: 14px; font-weight: 500; margin-bottom: 8px;">${q.question}</p>
                            <label style="margin-right: 16px; font-size: 13px;">
                                <input type="radio" name="q${index}" value="yes" style="margin-right: 6px;">
                                Yes
                            </label>
                            <label style="font-size: 13px;">
                                <input type="radio" name="q${index}" value="no" style="margin-right: 6px;">
                                No
                            </label>
                        </div>
                    `;
          default:
            return `<p style="font-size: 14px;">${q.question}</p>`;
        }
      })
      .join("");

    return `
            <h4 style="margin-top: 0; font-size: 16px;">Quick Survey</h4>
            <form id="survey-form">
                ${questionsHTML}
                <div style="text-align: right; margin-top: 16px;">
                    <button type="button" onclick="window.submitSurvey()"
                            style="padding: 6px 12px; background: #007bff; color: white; border: none; border-radius: 4px; font-size: 14px;">Submit</button>
                </div>
            </form>
            <script>
                window.submitSurvey = function() {
                    const form = document.getElementById('survey-form');
                    const formData = new FormData(form);
                    const responses = {};
                    for (let [key, value] of formData.entries()) {
                        responses[key] = value;
                    }
                    ${resolve.toString()}({ responses: responses, timestamp: Date.now() });
                    document.querySelector('[style*="position: fixed"]').remove();
                };
            </script>
        `;
  }

  closePrompt(promptId) {
    if (this.activePrompts.has(promptId)) {
      const prompt = this.activePrompts.get(promptId);
      if (document.body.contains(prompt)) {
        document.body.removeChild(prompt);
      }
      this.activePrompts.delete(promptId);
    }
  }

  async showPostTaskSurvey(task) {
    const surveyQuestions = [
      {
        type: "multiple-choice",
        question: "How easy was it to complete this task?",
        options: [
          "Very Easy",
          "Easy",
          "Neutral",
          "Difficult",
          "Very Difficult",
        ],
      },
      {
        type: "yes-no",
        question: "Did you encounter any confusing elements?",
      },
      {
        type: "multiple-choice",
        question: "How would you rate the overall experience?",
        options: ["Excellent", "Good", "Fair", "Poor"],
      },
    ];

    return await this.displayPrompt({
      id: this.generateId(),
      type: "survey",
      promptData: { questions: surveyQuestions },
    });
  }

  async showFinalSurvey(sessionData) {
    const finalQuestions = [
      {
        type: "multiple-choice",
        question: "Overall, how would you rate this application?",
        options: ["Excellent", "Very Good", "Good", "Fair", "Poor"],
      },
      {
        type: "yes-no",
        question: "Would you recommend this application to others?",
      },
      {
        type: "multiple-choice",
        question: "What is the most important area for improvement?",
        options: [
          "User Interface",
          "Performance",
          "Features",
          "Audio Quality",
          "Ease of Use",
        ],
      },
    ];

    return await this.displayPrompt({
      id: this.generateId(),
      type: "survey",
      promptData: { questions: finalQuestions },
    });
  }

  generateId() {
    return (
      "feedback_" + Date.now() + "_" + Math.random().toString(36).substr(2, 6)
    );
  }
}

export { UserTestingSession, UserAnalytics, FeedbackSystem };
