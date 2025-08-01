#include "huntmaster/security/access-controller.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace huntmaster {
namespace security {

// Pimpl implementation forward declaration
struct AccessController::AccessControllerImpl {
    // User credentials (username -> password)
    std::unordered_map<std::string, std::string> userCredentials_;

    // User roles (username -> set of roles)
    std::unordered_map<std::string, std::unordered_set<std::string>> userRoles_;

    // Active sessions (sessionId -> SessionInfo)
    std::unordered_map<std::string, SessionInfo> activeSessions_;

    // Access policies (resource -> AccessPolicy)
    std::unordered_map<std::string, AccessPolicy> accessPolicies_;

    // Random number generator for session IDs
    std::random_device rd_;
    std::mt19937 gen_;

    AccessControllerImpl() : gen_(rd_()) {
        initializeDefaultUsers();
        initializeDefaultPolicies();
    }

    ~AccessControllerImpl() = default;

    void initializeDefaultUsers() {
        // Add some default users for testing
        userCredentials_["admin_user"] = "admin123!";
        userCredentials_["regular_user"] = "user123!";
        userCredentials_["guest_user"] = "guest123!";
        userCredentials_["service_account"] = "service123!";

        // Set default roles
        userRoles_["admin_user"].insert("admin");
        userRoles_["regular_user"].insert("user");
        userRoles_["guest_user"].insert("guest");
        userRoles_["service_account"].insert("service");
    }

    void initializeDefaultPolicies() {
        // Admin policy
        AccessPolicy adminPolicy;
        adminPolicy.resource = "/api/admin/config";
        adminPolicy.allowedAccess = {AccessType::Read,
                                     AccessType::Write,
                                     AccessType::Execute,
                                     AccessType::Delete,
                                     AccessType::Admin};
        adminPolicy.allowedRoles = {"admin"};
        adminPolicy.requiresAuthentication = true;
        adminPolicy.maxConcurrentSessions = 10;
        adminPolicy.sessionTimeout = 3600;
        accessPolicies_["/api/admin/config"] = adminPolicy;

        // User policy
        AccessPolicy userPolicy;
        userPolicy.resource = "/api/audio/process";
        userPolicy.allowedAccess = {AccessType::Read, AccessType::Write, AccessType::Execute};
        userPolicy.allowedRoles = {"user", "admin"};
        userPolicy.requiresAuthentication = true;
        userPolicy.maxConcurrentSessions = 5;
        userPolicy.sessionTimeout = 1800;
        accessPolicies_["/api/audio/process"] = userPolicy;

        // Guest policy
        AccessPolicy guestPolicy;
        guestPolicy.resource = "/api/user/profile";
        guestPolicy.allowedAccess = {AccessType::Read};
        guestPolicy.allowedRoles = {"guest", "user", "admin"};
        guestPolicy.requiresAuthentication = true;
        guestPolicy.maxConcurrentSessions = 3;
        guestPolicy.sessionTimeout = 900;
        accessPolicies_["/api/user/profile"] = guestPolicy;

        // Audio hierarchy policy
        AccessPolicy audioPolicy;
        audioPolicy.resource = "/api/audio";
        audioPolicy.allowedAccess = {AccessType::Read};
        audioPolicy.allowedRoles = {"user", "admin"};
        audioPolicy.requiresAuthentication = true;
        audioPolicy.maxConcurrentSessions = 5;
        audioPolicy.sessionTimeout = 1800;
        accessPolicies_["/api/audio"] = audioPolicy;
    }

    std::string generateSessionId() {
        std::uniform_int_distribution<> dis(0, 15);
        std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen_);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen_);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen_);
        }
        ss << "-";
        ss << dis2(gen_);
        for (i = 0; i < 3; i++) {
            ss << dis(gen_);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen_);
        }
        return ss.str();
    }

    uint64_t getCurrentTime() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
};

AccessController::AccessController() : impl_(std::make_unique<AccessControllerImpl>()) {
    // Access controller is now fully initialized with default users and policies
}

AccessController::~AccessController() = default;

bool AccessController::authenticate(const std::string& username, const std::string& credentials) {
    if (username.empty() || credentials.empty()) {
        return false;
    }

    auto it = impl_->userCredentials_.find(username);
    if (it != impl_->userCredentials_.end()) {
        return it->second == credentials;
    }

    return false;
}

bool AccessController::authorize(const std::string& userId,
                                 const std::string& resource,
                                 AccessType access) {
    if (userId.empty() || resource.empty()) {
        return false;
    }

    // Check if user has required roles for this resource
    auto policyIt = impl_->accessPolicies_.find(resource);
    if (policyIt == impl_->accessPolicies_.end()) {
        // Check for parent resource policies (hierarchical access)
        for (const auto& [policyResource, policy] : impl_->accessPolicies_) {
            if (resource.find(policyResource) == 0 && resource != policyResource) {
                policyIt = impl_->accessPolicies_.find(policyResource);
                break;
            }
        }
        if (policyIt == impl_->accessPolicies_.end()) {
            return false;
        }
    }

    const AccessPolicy& policy = policyIt->second;

    // Check if access type is allowed
    bool accessTypeAllowed = false;
    for (const auto& allowedAccess : policy.allowedAccess) {
        if (allowedAccess == access) {
            accessTypeAllowed = true;
            break;
        }
    }
    if (!accessTypeAllowed) {
        return false;
    }

    // Check if user has required role
    auto userRolesIt = impl_->userRoles_.find(userId);
    if (userRolesIt == impl_->userRoles_.end()) {
        return false;
    }

    for (const auto& requiredRole : policy.allowedRoles) {
        if (userRolesIt->second.find(requiredRole) != userRolesIt->second.end()) {
            return true;
        }
    }

    return false;
}

void AccessController::createSession(const std::string& userId, SessionInfo& session) {
    if (userId.empty()) {
        return;
    }

    session.sessionId = impl_->generateSessionId();
    session.userId = userId;
    session.createdTime = impl_->getCurrentTime();
    session.lastAccessTime = session.createdTime;
    session.expirationTime = session.createdTime + 3600;  // Default 1 hour
    session.isActive = true;
    session.clientIP = "127.0.0.1";  // Default for testing
    session.userAgent = "Test Agent";

    impl_->activeSessions_[session.sessionId] = session;
}

void AccessController::destroySession(const std::string& sessionId) {
    if (sessionId.empty()) {
        return;
    }

    auto it = impl_->activeSessions_.find(sessionId);
    if (it != impl_->activeSessions_.end()) {
        it->second.isActive = false;
        impl_->activeSessions_.erase(it);
    }
}

bool AccessController::validateSession(const std::string& sessionId) {
    if (sessionId.empty()) {
        return false;
    }

    auto it = impl_->activeSessions_.find(sessionId);
    if (it == impl_->activeSessions_.end()) {
        return false;
    }

    SessionInfo& session = it->second;
    uint64_t currentTime = impl_->getCurrentTime();

    // Check if session has expired
    if (currentTime > session.expirationTime || !session.isActive) {
        impl_->activeSessions_.erase(it);
        return false;
    }

    // Update last access time
    session.lastAccessTime = currentTime;
    return true;
}

void AccessController::addRole(const std::string& userId, const std::string& role) {
    if (userId.empty() || role.empty()) {
        return;
    }

    // Only add roles to existing users
    if (impl_->userCredentials_.find(userId) != impl_->userCredentials_.end()) {
        impl_->userRoles_[userId].insert(role);
    }
}

void AccessController::removeRole(const std::string& userId, const std::string& role) {
    if (userId.empty() || role.empty()) {
        return;
    }

    auto it = impl_->userRoles_.find(userId);
    if (it != impl_->userRoles_.end()) {
        it->second.erase(role);
        if (it->second.empty()) {
            impl_->userRoles_.erase(it);
        }
    }
}

bool AccessController::hasPermission(const std::string& userId, const std::string& permission) {
    if (userId.empty() || permission.empty()) {
        return false;
    }

    // For now, return false as this is a placeholder implementation
    return false;
}

void AccessController::setAccessPolicy(const std::string& resource, const AccessPolicy& policy) {
    if (resource.empty()) {
        return;
    }

    impl_->accessPolicies_[resource] = policy;
}

AccessPolicy AccessController::getAccessPolicy(const std::string& resource) {
    if (resource.empty()) {
        return AccessPolicy{};
    }

    auto it = impl_->accessPolicies_.find(resource);
    if (it != impl_->accessPolicies_.end()) {
        return it->second;
    }

    return AccessPolicy{};
}

std::vector<std::string> AccessController::getUserRoles(const std::string& userId) {
    std::vector<std::string> roles;

    if (userId.empty()) {
        return roles;
    }

    auto it = impl_->userRoles_.find(userId);
    if (it != impl_->userRoles_.end()) {
        for (const auto& role : it->second) {
            roles.push_back(role);
        }
    }

    return roles;
}

bool AccessController::performAccessAudit() {
    // Placeholder audit implementation - always returns true for now
    return true;
}

}  // namespace security
}  // namespace huntmaster
