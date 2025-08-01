#include "huntmaster/security/crypto-manager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_map>

namespace huntmaster {
namespace security {

// Pimpl implementation forward declaration
struct CryptoManager::CryptoManagerImpl {
    CryptoConfig config;
    std::unordered_map<std::string, CryptoKey> keyStore_;
    std::random_device rd_;
    std::mt19937 gen_;

    CryptoManagerImpl(const CryptoConfig& cfg) : config(cfg), gen_(rd_()) {}
    ~CryptoManagerImpl() = default;

    std::string generateKeyId() {
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 16; i++) {
            ss << dis(gen_);
        }
        return "key_" + ss.str();
    }

    uint64_t getCurrentTime() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    // Simple XOR encryption for testing (not cryptographically secure)
    void simpleEncrypt(const std::vector<uint8_t>& input,
                       const std::vector<uint8_t>& key,
                       std::vector<uint8_t>& output) {
        output.resize(input.size() + 4);  // Add 4 bytes for magic marker

        // Add magic marker at the beginning to identify valid encrypted data
        output[0] = 0xDE;
        output[1] = 0xAD;
        output[2] = 0xBE;
        output[3] = 0xEF;

        // Encrypt the actual data
        for (size_t i = 0; i < input.size(); ++i) {
            output[i + 4] = input[i] ^ key[i % key.size()];
        }
    }

    // Simple XOR decryption with validation
    bool simpleDecrypt(const std::vector<uint8_t>& input,
                       const std::vector<uint8_t>& key,
                       std::vector<uint8_t>& output) {
        if (input.size() < 4) {
            return false;  // Too small to contain magic marker
        }

        // Check magic marker
        if (input[0] != 0xDE || input[1] != 0xAD || input[2] != 0xBE || input[3] != 0xEF) {
            return false;  // Invalid encrypted data
        }

        output.resize(input.size() - 4);

        // Decrypt the actual data
        for (size_t i = 4; i < input.size(); ++i) {
            output[i - 4] = input[i] ^ key[(i - 4) % key.size()];
        }

        return true;
    }

    // Simple hash function for testing (not cryptographically secure)
    void simpleHash(const std::vector<uint8_t>& data,
                    HashAlgorithm algorithm,
                    std::vector<uint8_t>& hashOutput) {
        size_t hashSize = (algorithm == HashAlgorithm::SHA256) ? 32 : 64;
        hashOutput.resize(hashSize);

        uint64_t hash = 0x9e3779b9;
        for (uint8_t byte : data) {
            hash ^= byte;
            hash *= 0x9e3779b9;
        }

        for (size_t i = 0; i < hashSize; ++i) {
            hashOutput[i] = (hash >> (i * 8)) & 0xFF;
        }
    }
};

CryptoManager::CryptoManager(const CryptoConfig& config)
    : impl_(std::make_unique<CryptoManagerImpl>(config)) {
    // Validate configuration parameters
    if (impl_->config.keyDerivationIterations < 10000) {
        impl_->config.keyDerivationIterations = 100000;  // Secure default
    }
}

CryptoManager::~CryptoManager() = default;

bool CryptoManager::encrypt(const std::vector<uint8_t>& plaintext,
                            const std::string& keyId,
                            std::vector<uint8_t>& ciphertext) {
    if (keyId.empty()) {
        return false;
    }

    auto it = impl_->keyStore_.find(keyId);
    if (it == impl_->keyStore_.end()) {
        return false;
    }

    // Allow empty plaintext encryption
    impl_->simpleEncrypt(plaintext, it->second.keyData, ciphertext);
    return true;
}

bool CryptoManager::decrypt(const std::vector<uint8_t>& ciphertext,
                            const std::string& keyId,
                            std::vector<uint8_t>& plaintext) {
    if (keyId.empty()) {
        return false;
    }

    auto it = impl_->keyStore_.find(keyId);
    if (it == impl_->keyStore_.end()) {
        return false;
    }

    // Use the new validation-enabled decryption
    return impl_->simpleDecrypt(ciphertext, it->second.keyData, plaintext);
}

bool CryptoManager::hash(const std::vector<uint8_t>& data,
                         HashAlgorithm algorithm,
                         std::vector<uint8_t>& hashOutput) {
    // Allow empty data hashing - produce hash of empty data
    impl_->simpleHash(data, algorithm, hashOutput);
    return true;
}

bool CryptoManager::verifyHash(const std::vector<uint8_t>& data,
                               const std::vector<uint8_t>& expectedHash,
                               HashAlgorithm algorithm) {
    if (data.empty() || expectedHash.empty()) {
        return false;
    }

    std::vector<uint8_t> computedHash;
    if (!hash(data, algorithm, computedHash)) {
        return false;
    }

    return computedHash == expectedHash;
}

std::string CryptoManager::generateKey(KeyType type, size_t keySize) {
    std::string keyId = impl_->generateKeyId();

    CryptoKey key;
    key.keyId = keyId;
    key.type = type;
    key.keyData.resize(keySize);
    key.creationTime = impl_->getCurrentTime();
    key.expirationTime = key.creationTime + 86400;  // 24 hours
    key.isActive = true;

    // Generate random key data
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < keySize; ++i) {
        key.keyData[i] = dis(impl_->gen_);
    }

    impl_->keyStore_[keyId] = key;
    return keyId;
}

bool CryptoManager::storeKey(const std::string& keyId, const CryptoKey& key) {
    if (keyId.empty()) {
        return false;
    }

    impl_->keyStore_[keyId] = key;
    return true;
}

bool CryptoManager::retrieveKey(const std::string& keyId, CryptoKey& key) {
    if (keyId.empty()) {
        return false;
    }

    auto it = impl_->keyStore_.find(keyId);
    if (it != impl_->keyStore_.end()) {
        key = it->second;
        return true;
    }

    return false;
}

bool CryptoManager::deleteKey(const std::string& keyId) {
    if (keyId.empty()) {
        return false;
    }

    auto it = impl_->keyStore_.find(keyId);
    if (it != impl_->keyStore_.end()) {
        impl_->keyStore_.erase(it);
        return true;
    }

    return false;
}

bool CryptoManager::rotateKey(const std::string& keyId) {
    if (keyId.empty()) {
        return false;
    }

    auto it = impl_->keyStore_.find(keyId);
    if (it != impl_->keyStore_.end()) {
        // Generate new key data
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        for (auto& byte : it->second.keyData) {
            byte = dis(impl_->gen_);
        }
        it->second.creationTime = impl_->getCurrentTime();
        it->second.expirationTime = it->second.creationTime + 86400;
        return true;
    }

    return false;
}

bool CryptoManager::generateSecureRandom(std::vector<uint8_t>& randomData, size_t size) {
    if (size == 0) {
        return true;  // Allow zero-size random generation
    }

    randomData.resize(size);

    // Generate different random data each time by using random generator
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < size; ++i) {
        randomData[i] = dis(impl_->gen_);
    }

    return true;
}

bool CryptoManager::deriveKey(const std::string& password,
                              const std::vector<uint8_t>& salt,
                              uint32_t iterations,
                              size_t keyLength,
                              std::vector<uint8_t>& derivedKey) {
    if (password.empty() || salt.empty() || keyLength == 0) {
        return false;
    }

    derivedKey.resize(keyLength);

    // Simple key derivation for testing with length-dependent output
    uint64_t hash = 0x9e3779b9;
    for (char c : password) {
        hash ^= c;
        hash *= 0x9e3779b9;
    }
    for (uint8_t byte : salt) {
        hash ^= byte;
        hash *= 0x9e3779b9;
    }
    hash ^= iterations;
    hash ^= keyLength;  // Include key length in hash to make output length-dependent

    // Generate different patterns for different key lengths
    for (size_t i = 0; i < keyLength; ++i) {
        uint64_t elementHash = hash;
        elementHash ^= (i * 0xdeadbeef);
        elementHash *= 0x9e3779b9;
        derivedKey[i] = static_cast<uint8_t>((elementHash >> (i % 8 * 8)) & 0xFF);
    }

    return true;
}

bool CryptoManager::sign(const std::vector<uint8_t>& data,
                         const std::string& privateKeyId,
                         std::vector<uint8_t>& signature) {
    if (data.empty() || privateKeyId.empty()) {
        return false;
    }

    // Placeholder signature - simple hash
    impl_->simpleHash(data, HashAlgorithm::SHA256, signature);
    return true;
}

bool CryptoManager::verify(const std::vector<uint8_t>& data,
                           const std::vector<uint8_t>& signature,
                           const std::string& publicKeyId) {
    if (data.empty() || signature.empty() || publicKeyId.empty()) {
        return false;
    }

    std::vector<uint8_t> expectedSignature;
    impl_->simpleHash(data, HashAlgorithm::SHA256, expectedSignature);
    return expectedSignature == signature;
}

bool CryptoManager::performCryptoAudit() {
    return true;  // Placeholder implementation
}

}  // namespace security
}  // namespace huntmaster
