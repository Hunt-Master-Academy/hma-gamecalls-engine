/**
 * @file access-controller.h
 * @brief Access Control Security Header - Phase 3.4 Security Framework
 *
 * This header defines the AccessController class and related structures
 * for comprehensive authentication, authorization, and session management.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

#ifndef HUNTMASTER_SECURITY_ACCESS_CONTROLLER_H
#define HUNTMASTER_SECURITY_ACCESS_CONTROLLER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Access Types
 */
enum class AccessType { Read, Write, Execute, Delete, Admin };

/**
 * Session Information
 */
struct SessionInfo {
    std::string sessionId;
    std::string userId;
    uint64_t createdTime;
    uint64_t lastAccessTime;
    uint64_t expirationTime;
    bool isActive;
    std::string clientIP;
    std::string userAgent;
};

/**
 * Access Policy
 */
struct AccessPolicy {
    std::string resource;
    std::vector<AccessType> allowedAccess;
    std::vector<std::string> allowedRoles;
    bool requiresAuthentication;
    uint32_t maxConcurrentSessions;
    uint64_t sessionTimeout;
};

/**
 * Access Controller Class
 *
 * Provides comprehensive access control including authentication,
 * authorization, session management, and role-based access control.
 */
class AccessController {
  public:
    AccessController();
    ~AccessController();

    // Authentication
    bool authenticate(const std::string& username, const std::string& credentials);

    // Authorization
    bool authorize(const std::string& userId, const std::string& resource, AccessType access);

    // Session Management
    void createSession(const std::string& userId, SessionInfo& session);
    void destroySession(const std::string& sessionId);
    bool validateSession(const std::string& sessionId);

    // Role Management
    void addRole(const std::string& userId, const std::string& role);
    void removeRole(const std::string& userId, const std::string& role);
    bool hasPermission(const std::string& userId, const std::string& permission);

    // Policy Management
    void setAccessPolicy(const std::string& resource, const AccessPolicy& policy);
    AccessPolicy getAccessPolicy(const std::string& resource);

    // User Management
    std::vector<std::string> getUserRoles(const std::string& userId);

    // Audit
    bool performAccessAudit();

  private:
    struct AccessControllerImpl;
    std::unique_ptr<AccessControllerImpl> impl_;
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_ACCESS_CONTROLLER_H
