#pragma once

#include <vector>
#include <string>
#include <memory>
#include "types/AccountConfig.hpp"

namespace SmartMail {

/// 账户管理器：CRUD、主密码、加密存储
class AccountManager {
public:
    AccountManager();
    ~AccountManager();

    // --- CRUD ---
    std::vector<AccountConfig> getAll() const;
    std::unique_ptr<AccountConfig> getById(const std::string& id) const;
    bool add(const AccountConfig& account);
    bool update(const AccountConfig& account);
    bool remove(const std::string& id);

    // --- 主密码管理 ---
    void setMasterPassword(const std::string& password);
    bool unlock(const std::string& password);        // 加载文件后解锁
    bool verifyMasterPassword(const std::string& password) const;
    bool isUnlocked() const;

    // --- 获取解密凭据 ---
    std::string getDecryptedPassword(const std::string& accountId) const;

    // --- 连接测试 ---
    bool testConnection(const AccountConfig& config) const;

    // --- 文件持久化 ---
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

private:
    std::string encryptPassword(const std::string& plainPassword) const;
    std::string decryptPassword(const std::string& encrypted) const;

    std::vector<AccountConfig> accounts_;
    std::string masterPasswordHash_;   // SHA-256(derivedKey) — 持久化到文件
    std::string salt_;                 // PBKDF2 salt            — 持久化到文件
    std::string derivedKey_;           // 会话密钥缓存 (32 bytes) — 仅内存
};

} // namespace SmartMail
