#pragma once

#include "IMailProtocol.hpp"
#include <curl/curl.h>
#include <string>

namespace SmartMail {

class SmtpClient : public IMailProtocol {
public:
    SmtpClient();
    ~SmtpClient() override;

    // IMailProtocol 接口
    bool connect(const AccountConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<Email> fetchEmails(int limit = 50) override;
    bool sendEmail(const Email& email) override;
    bool markAsRead(const std::string& emailId) override;
    bool deleteEmail(const std::string& emailId) override;

    /// 设置解密后的 SMTP 认证密码（Phase 4+ 使用）
    void setAuthPassword(const std::string& password);

private:
    bool connected_ = false;
    AccountConfig config_;
    std::string authPassword_;
    std::string preparePayload(const Email& email);
    static size_t readCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    struct UploadCtx {
        const std::string& data;
        size_t offset = 0;
    };
};

} // namespace SmartMail
