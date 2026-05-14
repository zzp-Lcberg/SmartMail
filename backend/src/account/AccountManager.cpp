#include "AccountManager.hpp"
#include "CryptoUtils.hpp"
#include "utils/Logger.hpp"
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <ctime>
#include <cstring>

namespace SmartMail {

using json = nlohmann::json;

// --- 构造 / 析构 ---

AccountManager::AccountManager() {
    LOG_DEBUG("AccountManager created");
}

AccountManager::~AccountManager() {
    LOG_DEBUG("AccountManager destroyed");
}

// --- CRUD ---

std::vector<AccountConfig> AccountManager::getAll() const {
    return accounts_;
}

std::unique_ptr<AccountConfig> AccountManager::getById(const std::string& id) const {
    for (const auto& acc : accounts_) {
        if (acc.id == id) {
            return std::make_unique<AccountConfig>(acc);
        }
    }
    return nullptr;
}

bool AccountManager::add(const AccountConfig& account) {
    if (account.email.empty()) {
        LOG_ERROR("add: email is required");
        return false;
    }
    if (!isUnlocked()) {
        LOG_ERROR("add: master password not set, call setMasterPassword or unlock first");
        return false;
    }

    // 检查重复
    for (const auto& existing : accounts_) {
        if (existing.email == account.email) {
            LOG_ERROR("add: account with email " + account.email + " already exists");
            return false;
        }
    }

    AccountConfig stored = account;

    // 自动生成 ID
    if (stored.id.empty()) {
        std::ostringstream idSrc;
        idSrc << account.email << "_" << std::time(nullptr);
        std::string idRaw = idSrc.str();
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(idRaw.data()), idRaw.size(), hash);
        // 取前 8 字节编码为字母数字 ID
        std::string b64 = CryptoUtils::base64Encode(
            std::string(reinterpret_cast<char*>(hash), 8));
        stored.id.clear();
        for (char c : b64) {
            if (std::isalnum(static_cast<unsigned char>(c))) stored.id += c;
        }
    }

    // 加密密码
    if (!stored.encryptedPassword.empty()) {
        std::string encrypted = encryptPassword(stored.encryptedPassword);
        if (encrypted.empty()) {
            LOG_ERROR("add: password encryption failed");
            return false;
        }
        stored.encryptedPassword = encrypted;
    }

    accounts_.push_back(stored);
    LOG_INFO("Account added: " + stored.email);
    return true;
}

bool AccountManager::update(const AccountConfig& account) {
    for (auto& acc : accounts_) {
        if (acc.id == account.id) {
            AccountConfig updated = account;

            // 新明文密码 → 加密；空 → 保留旧密码
            if (!account.encryptedPassword.empty() && isUnlocked()) {
                std::string encrypted = encryptPassword(account.encryptedPassword);
                if (encrypted.empty()) {
                    LOG_ERROR("update: password encryption failed");
                    return false;
                }
                updated.encryptedPassword = encrypted;
            } else if (account.encryptedPassword.empty()) {
                updated.encryptedPassword = acc.encryptedPassword;
            }

            acc = updated;
            LOG_INFO("Account updated: " + acc.email);
            return true;
        }
    }
    LOG_ERROR("update: account not found: " + account.id);
    return false;
}

bool AccountManager::remove(const std::string& id) {
    auto it = std::remove_if(accounts_.begin(), accounts_.end(),
        [&id](const AccountConfig& a) { return a.id == id; });
    if (it == accounts_.end()) return false;
    accounts_.erase(it, accounts_.end());
    LOG_INFO("Account removed: " + id);
    return true;
}

// --- 主密码管理 ---

void AccountManager::setMasterPassword(const std::string& password) {
    salt_ = CryptoUtils::generateSalt(16);
    if (salt_.empty()) {
        LOG_ERROR("Failed to generate salt");
        return;
    }

    std::string key = CryptoUtils::deriveKey(password, salt_);
    if (key.empty()) {
        LOG_ERROR("Failed to derive key");
        salt_.clear();
        return;
    }

    // 缓存派生密钥（仅内存，不持久化）
    derivedKey_ = key;

    // 存储密钥哈希用于后续验证（持久化到文件）
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(key.data()), key.size(), hash);
    masterPasswordHash_ = std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);

    LOG_INFO("Master password set successfully");
}

bool AccountManager::unlock(const std::string& password) {
    if (salt_.empty()) {
        LOG_ERROR("unlock: no salt, call setMasterPassword first or load accounts file");
        return false;
    }

    std::string key = CryptoUtils::deriveKey(password, salt_);
    if (key.empty()) return false;

    // 验证：SHA-256(key) == stored_hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(key.data()), key.size(), hash);

    if (std::memcmp(hash, masterPasswordHash_.data(), SHA256_DIGEST_LENGTH) != 0) {
        LOG_ERROR("unlock: incorrect master password");
        return false;
    }

    derivedKey_ = key;  // 缓存密钥
    LOG_INFO("AccountManager unlocked");
    return true;
}

bool AccountManager::verifyMasterPassword(const std::string& password) const {
    if (salt_.empty()) return false;

    std::string key = CryptoUtils::deriveKey(password, salt_);
    if (key.empty()) return false;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(key.data()), key.size(), hash);

    if (!masterPasswordHash_.empty()) {
        return std::memcmp(hash, masterPasswordHash_.data(), SHA256_DIGEST_LENGTH) == 0;
    }

    // 如果没有 hash（首次设置前），比较派生密钥本身
    return derivedKey_ == key;
}

bool AccountManager::isUnlocked() const {
    return !derivedKey_.empty();
}

// --- 解密凭据 ---

std::string AccountManager::getDecryptedPassword(const std::string& accountId) const {
    auto acc = getById(accountId);
    if (!acc) {
        LOG_ERROR("getDecryptedPassword: account not found: " + accountId);
        return "";
    }
    if (!isUnlocked()) {
        LOG_ERROR("getDecryptedPassword: not unlocked");
        return "";
    }
    return decryptPassword(acc->encryptedPassword);
}

// --- 连接测试 ---

bool AccountManager::testConnection(const AccountConfig& config) const {
    if (config.smtpServer.empty()) {
        LOG_ERROR("testConnection: smtp server not configured");
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    int port = config.smtpPort > 0 ? config.smtpPort : 465;
    std::string url = (config.smtpUseSSL ? "smtps://" : "smtp://")
                    + config.smtpServer + ":" + std::to_string(port);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config.smtpUseSSL ? CURLUSESSL_ALL : CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config.email.c_str());

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    bool reachable = (res == CURLE_OK || res == CURLE_LOGIN_DENIED);
    if (reachable) {
        LOG_INFO("SMTP connection test OK: " + config.smtpServer);
    } else {
        LOG_ERROR("SMTP connection test FAILED: " + std::string(curl_easy_strerror(res)));
    }
    return reachable;
}

// --- 文件持久化 ---

bool AccountManager::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOG_WARN("Cannot open account file: " + filepath);
        return false;
    }

    try {
        json j;
        file >> j;

        salt_ = CryptoUtils::base64Decode(j.value("salt", ""));
        masterPasswordHash_ = CryptoUtils::base64Decode(j.value("password_hash", ""));
        derivedKey_.clear();  // 加载后需重新 unlock

        accounts_.clear();
        for (const auto& accJson : j.value("accounts", json::array())) {
            AccountConfig acc;
            acc.id                = accJson.value("id", "");
            acc.displayName       = accJson.value("display_name", "");
            acc.email             = accJson.value("email", "");
            acc.encryptedPassword = CryptoUtils::base64Decode(accJson.value("encrypted_password", ""));
            acc.smtpServer        = accJson.value("smtp_server", "");
            acc.smtpPort          = accJson.value("smtp_port", 465);
            acc.smtpUseSSL        = accJson.value("smtp_use_ssl", true);
            acc.imapServer        = accJson.value("imap_server", "");
            acc.imapPort          = accJson.value("imap_port", 993);
            acc.imapUseSSL        = accJson.value("imap_use_ssl", true);
            acc.pop3Server        = accJson.value("pop3_server", "");
            acc.pop3Port          = accJson.value("pop3_port", 995);
            acc.pop3UseSSL        = accJson.value("pop3_use_ssl", true);
            acc.preferredProtocol = accJson.value("preferred_protocol", 0) == 0
                                    ? AccountConfig::Protocol::IMAP
                                    : AccountConfig::Protocol::POP3;
            acc.syncInterval      = accJson.value("sync_interval", 300);
            acc.maxFetchCount     = accJson.value("max_fetch_count", 50);
            acc.autoClassify      = accJson.value("auto_classify", true);

            accounts_.push_back(std::move(acc));
        }

        LOG_INFO("Loaded " + std::to_string(accounts_.size()) + " accounts from " + filepath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to parse account file: ") + e.what());
        return false;
    }
}

bool AccountManager::saveToFile(const std::string& filepath) const {
    try {
        json j;
        j["salt"] = CryptoUtils::base64Encode(salt_);
        j["password_hash"] = CryptoUtils::base64Encode(masterPasswordHash_);

        json accountsArr = json::array();
        for (const auto& acc : accounts_) {
            json accJson;
            accJson["id"]                 = acc.id;
            accJson["display_name"]       = acc.displayName;
            accJson["email"]              = acc.email;
            accJson["encrypted_password"] = CryptoUtils::base64Encode(acc.encryptedPassword);
            accJson["smtp_server"]        = acc.smtpServer;
            accJson["smtp_port"]          = acc.smtpPort;
            accJson["smtp_use_ssl"]       = acc.smtpUseSSL;
            accJson["imap_server"]        = acc.imapServer;
            accJson["imap_port"]          = acc.imapPort;
            accJson["imap_use_ssl"]       = acc.imapUseSSL;
            accJson["pop3_server"]        = acc.pop3Server;
            accJson["pop3_port"]          = acc.pop3Port;
            accJson["pop3_use_ssl"]       = acc.pop3UseSSL;
            accJson["preferred_protocol"] = acc.preferredProtocol == AccountConfig::Protocol::IMAP ? 0 : 1;
            accJson["sync_interval"]      = acc.syncInterval;
            accJson["max_fetch_count"]    = acc.maxFetchCount;
            accJson["auto_classify"]      = acc.autoClassify;
            accountsArr.push_back(accJson);
        }
        j["accounts"] = accountsArr;

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Cannot open file for writing: " + filepath);
            return false;
        }
        file << j.dump(2);
        LOG_INFO("Saved " + std::to_string(accounts_.size()) + " accounts to " + filepath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to save account file: ") + e.what());
        return false;
    }
}

// --- 私有辅助 ---

std::string AccountManager::encryptPassword(const std::string& plainPassword) const {
    return CryptoUtils::encrypt(plainPassword, derivedKey_);
}

std::string AccountManager::decryptPassword(const std::string& encrypted) const {
    return CryptoUtils::decrypt(encrypted, derivedKey_);
}

} // namespace SmartMail
