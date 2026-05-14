#pragma once

#include <string>

namespace SmartMail {

/// AES-256-CBC 加密工具
class CryptoUtils {
public:
    /// 使用 PBKDF2 从主密码派生 AES 密钥
    static std::string deriveKey(const std::string& masterPassword,
                                  const std::string& salt);

    /// AES-256-CBC 加密
    static std::string encrypt(const std::string& plaintext,
                                const std::string& key);
    /// AES-256-CBC 解密
    static std::string decrypt(const std::string& ciphertext,
                                const std::string& key);

    /// 生成随机 salt
    static std::string generateSalt(size_t length = 16);

    /// Base64 编解码
    static std::string base64Encode(const std::string& data);
    static std::string base64Decode(const std::string& encoded);
};

} // namespace SmartMail
