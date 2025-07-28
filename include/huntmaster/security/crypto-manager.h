/**
 * @file crypto-manager.h
 * @brief Cryptographic Manager Header - Phase 3.4 Security Framework
 *
 * This header defines the CryptoManager class and related structures
 * for comprehensive cryptographic operations and key management.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 26, 2025
 */

#ifndef HUNTMASTER_SECURITY_CRYPTO_MANAGER_H
#define HUNTMASTER_SECURITY_CRYPTO_MANAGER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huntmaster {
namespace security {

/**
 * Encryption Algorithms
 */
enum class EncryptionAlgorithm { AES_256_GCM, ChaCha20_Poly1305, AES_128_GCM };

/**
 * Hash Algorithms
 */
enum class HashAlgorithm { SHA256, SHA512, Blake2b };

/**
 * Key Types
 */
enum class KeyType { Symmetric, PublicKey, PrivateKey, SessionKey };

/**
 * Crypto Configuration
 */
struct CryptoConfig {
    EncryptionAlgorithm defaultEncryption = EncryptionAlgorithm::AES_256_GCM;
    HashAlgorithm defaultHash = HashAlgorithm::SHA256;
    bool enableHardwareAcceleration = true;
    bool requireSecureRandom = true;
    uint32_t keyDerivationIterations = 100000;
    bool enableKeyRotation = true;
    uint64_t keyRotationInterval = 86400;  // 24 hours
};

/**
 * Cryptographic Key
 */
struct CryptoKey {
    std::string keyId;
    KeyType type;
    std::vector<uint8_t> keyData;
    uint64_t creationTime;
    uint64_t expirationTime;
    bool isActive;
};

/**
 * Crypto Manager Class
 *
 * Provides comprehensive cryptographic services including encryption,
 * decryption, hashing, key management, and secure random generation.
 */
class CryptoManager {
  public:
    explicit CryptoManager(const CryptoConfig& config = CryptoConfig{});
    ~CryptoManager();

    // Encryption/Decryption
    bool encrypt(const std::vector<uint8_t>& plaintext,
                 const std::string& keyId,
                 std::vector<uint8_t>& ciphertext);

    bool decrypt(const std::vector<uint8_t>& ciphertext,
                 const std::string& keyId,
                 std::vector<uint8_t>& plaintext);

    // Hashing
    bool hash(const std::vector<uint8_t>& data,
              HashAlgorithm algorithm,
              std::vector<uint8_t>& hashOutput);

    bool verifyHash(const std::vector<uint8_t>& data,
                    const std::vector<uint8_t>& expectedHash,
                    HashAlgorithm algorithm);

    // Key Management
    std::string generateKey(KeyType type, size_t keySize = 32);
    bool storeKey(const std::string& keyId, const CryptoKey& key);
    bool retrieveKey(const std::string& keyId, CryptoKey& key);
    bool deleteKey(const std::string& keyId);
    bool rotateKey(const std::string& keyId);

    // Random Generation
    bool generateSecureRandom(std::vector<uint8_t>& randomData, size_t size);

    // Key Derivation
    bool deriveKey(const std::string& password,
                   const std::vector<uint8_t>& salt,
                   uint32_t iterations,
                   size_t keyLength,
                   std::vector<uint8_t>& derivedKey);

    // Digital Signatures (placeholder for future expansion)
    bool sign(const std::vector<uint8_t>& data,
              const std::string& privateKeyId,
              std::vector<uint8_t>& signature);

    bool verify(const std::vector<uint8_t>& data,
                const std::vector<uint8_t>& signature,
                const std::string& publicKeyId);

    // Crypto Audit
    bool performCryptoAudit();

  private:
    struct CryptoManagerImpl;
    std::unique_ptr<CryptoManagerImpl> impl_;
};

}  // namespace security
}  // namespace huntmaster

#endif  // HUNTMASTER_SECURITY_CRYPTO_MANAGER_H
