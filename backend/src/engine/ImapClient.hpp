#pragma once

#include "IMailProtocol.hpp"
#include <openssl/ssl.h>
#include <string>

namespace SmartMail {

/// IMAP 客户端 — SSL/TLS 连接 + 完整 IMAP4rev1 协议
class ImapClient : public IMailProtocol {
public:
    ImapClient();
    ~ImapClient() override;

    // IMailProtocol 接口
    bool connect(const AccountConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<Email> fetchEmails(int limit = 50) override;
    bool sendEmail(const Email& email) override;
    bool markAsRead(const std::string& emailId) override;
    bool deleteEmail(const std::string& emailId) override;

    // IMAP 特有操作
    std::vector<Folder> listFolders();
    bool selectFolder(const std::string& folderPath);

    /// 设置解密后的认证密码（由 AccountManager 解密后设置）
    void setAuthPassword(const std::string& password);

private:
    // 网络层
    bool createSocket(const std::string& host, int port);
    void closeSocket();
    bool sslHandshake();
    int sendLine(const std::string& line);
    std::string readLine();
    std::string readUntilTag(const std::string& tag);

    // 协议层
    std::string nextTag();
    std::string sendCommand(const std::string& cmd);
    bool login();
    std::string selectedFolder_;

    // 解析层
    Email parseFetchResponse(const std::string& response);
    Folder parseListLine(const std::string& line);
    std::string decodeUtf7(const std::string& input);

    // 网络状态
    bool connected_ = false;
    AccountConfig config_;
    std::string authPassword_;
    int sockfd_ = -1;
    SSL* ssl_ = nullptr;
    SSL_CTX* sslCtx_ = nullptr;
    int tagCounter_ = 0;
};

} // namespace SmartMail
