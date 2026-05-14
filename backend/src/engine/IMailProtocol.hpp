#pragma once

#include <vector>
#include <string>
#include "types/Email.hpp"
#include "types/AccountConfig.hpp"

namespace SmartMail {

/// 统一邮件协议接口
class IMailProtocol {
public:
    virtual ~IMailProtocol() = default;

    virtual bool connect(const AccountConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual std::vector<Email> fetchEmails(int limit = 50) = 0;
    virtual bool sendEmail(const Email& email) = 0;
    virtual bool markAsRead(const std::string& emailId) = 0;
    virtual bool deleteEmail(const std::string& emailId) = 0;
};

} // namespace SmartMail
