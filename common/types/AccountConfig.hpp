#pragma once

#include <string>
#include <cstdint>

namespace SmartMail {

/// 邮箱账号配置
struct AccountConfig {
    std::string id;                // 账号唯一标识 (UUID)
    std::string displayName;       // 显示名称
    std::string email;             // 邮箱地址
    std::string encryptedPassword; // AES-256 加密后的密码/授权码

    // SMTP 配置
    std::string smtpServer;
    int smtpPort = 465;
    bool smtpUseSSL = true;

    // IMAP 配置
    std::string imapServer;
    int imapPort = 993;
    bool imapUseSSL = true;

    // POP3 配置 (可选, 备用)
    std::string pop3Server;
    int pop3Port = 995;
    bool pop3UseSSL = true;

    // 协议偏好
    enum class Protocol { IMAP, POP3 };
    Protocol preferredProtocol = Protocol::IMAP;

    // 同步设置
    int syncInterval = 300;        // 同步间隔 (秒), 默认 5 分钟
    int maxFetchCount = 50;        // 每次最大获取数量
    bool autoClassify = true;      // 是否自动 AI 分类

    bool isValid() const {
        return !email.empty() && !smtpServer.empty()
            && !imapServer.empty() && !encryptedPassword.empty();
    }
};

} // namespace SmartMail
