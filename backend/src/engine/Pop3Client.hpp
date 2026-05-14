#pragma once

#include "IMailProtocol.hpp"
#include <openssl/ssl.h>
#include <string>

namespace SmartMail {

/// POP3 客户端 — SSL/TLS 连接 + 完整 POP3 协议
class Pop3Client : public IMailProtocol {
public:
    Pop3Client();
    ~Pop3Client() override;

    // IMailProtocol 接口
    bool connect(const AccountConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<Email> fetchEmails(int limit = 50) override;
    bool sendEmail(const Email& email) override;
    bool markAsRead(const std::string& emailId) override;
    bool deleteEmail(const std::string& emailId) override;

    /// 设置解密后的认证密码（由 AccountManager 解密后设置）
    void setAuthPassword(const std::string& password);

private:
    // 网络层（与 ImapClient 同模式）
    bool createSocket(const std::string& host, int port);
    void closeSocket();
    bool sslHandshake();
    int sendLine(const std::string& line);
    std::string readLine();
    std::string readMultiLine();

    // 协议层
    std::string sendCommand(const std::string& cmd);
    bool login();

    // 解析层
    Email parseRetrResponse(const std::string& response);

    bool connected_ = false;
    AccountConfig config_;
    std::string authPassword_;
    int sockfd_ = -1;
    SSL* ssl_ = nullptr;
    SSL_CTX* sslCtx_ = nullptr;
};

} // namespace SmartMail
