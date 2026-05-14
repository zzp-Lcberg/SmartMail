#include "SmtpClient.hpp"
#include "account/CryptoUtils.hpp"
#include "utils/Logger.hpp"
#include <curl/curl.h>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace SmartMail {

// --- 构造函数 ---

SmtpClient::SmtpClient() {
    LOG_DEBUG("SmtpClient created");
}

SmtpClient::~SmtpClient() {
    disconnect();
}

// --- 连接与断开 ---

bool SmtpClient::connect(const AccountConfig& config) {
    config_ = config;

    if (config.smtpServer.empty() || config.smtpPort <= 0) {
        LOG_ERROR("Invalid SMTP server configuration");
        return false;
    }

    // 使用 libcurl 做基本连通性测试
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to init libcurl for SMTP connection test");
        return false;
    }

    std::string url = (config.smtpUseSSL ? "smtps://" : "smtp://")
                    + config.smtpServer + ":" + std::to_string(config.smtpPort);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config.smtpUseSSL ? CURLUSESSL_ALL : CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 宽松验证
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    // 设置 MAIL_FROM 以便 libcurl 走完整的 SMTP 握手
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config.email.c_str());

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_cleanup(curl);

    // CURLE_OK 或认证相关错误说明服务器可达
    if (res == CURLE_OK || res == CURLE_LOGIN_DENIED
        || (res == CURLE_SEND_ERROR && responseCode > 0)) {
        connected_ = true;
        LOG_INFO("SMTP connection verified: " + config.smtpServer);
        return true;
    }

    LOG_ERROR("SMTP connect failed: " + std::string(curl_easy_strerror(res)));
    return false;
}

void SmtpClient::disconnect() {
    connected_ = false;
    config_ = AccountConfig{};
    LOG_DEBUG("SMTP client disconnected");
}

bool SmtpClient::isConnected() const {
    return connected_;
}

// --- 发送邮件 ---

bool SmtpClient::sendEmail(const Email& email) {
    if (email.recipients.empty()) {
        LOG_ERROR("sendEmail: no recipients specified");
        return false;
    }
    if (!connected_) {
        LOG_ERROR("sendEmail: not connected, call connect() first");
        return false;
    }

    std::string payload = preparePayload(email);
    if (payload.empty()) {
        LOG_ERROR("sendEmail: failed to build email payload");
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to init libcurl for SMTP send");
        return false;
    }

    std::string url = (config_.smtpUseSSL ? "smtps://" : "smtp://")
                    + config_.smtpServer + ":" + std::to_string(config_.smtpPort);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config_.smtpUseSSL ? CURLUSESSL_ALL : CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config_.email.c_str());

    // Phase 4: 优先使用 authPassword_（由 AccountManager 解密后设置），
    // 否则回退到 config_.encryptedPassword（Phase 3 测试用明文）
    const std::string& pass = !authPassword_.empty() ? authPassword_ : config_.encryptedPassword;
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pass.c_str());

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config_.email.c_str());

    // 收件人列表
    struct curl_slist* recipients = nullptr;
    for (const auto& rcpt : email.recipients) {
        recipients = curl_slist_append(recipients, rcpt.c_str());
    }
    if (!email.cc.empty()) {
        recipients = curl_slist_append(recipients, email.cc.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    // 上传 payload（READ 回调：libcurl 从 payload 读取数据发送）
    UploadCtx uploadCtx{payload, 0};
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, SmtpClient::readCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &uploadCtx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    // 收集服务器响应（供调试）
    std::string serverResponse;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SmtpClient::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &serverResponse);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR(std::string("SMTP send failed: ") + curl_easy_strerror(res));
        return false;
    }

    LOG_INFO("Email sent successfully to " + email.recipients.front());
    return true;
}

// --- RFC 2822 邮件格式构建 ---

std::string SmtpClient::preparePayload(const Email& email) {
    std::ostringstream msg;

    // 日期头
    time_t now = time(nullptr);
    char dateBuf[128];
    std::strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));

    // From（需要包含邮箱地址）
    msg << "Date: " << dateBuf << "\r\n";
    msg << "From: " << email.sender << "\r\n";

    // To
    msg << "To: ";
    for (size_t i = 0; i < email.recipients.size(); ++i) {
        if (i > 0) msg << ", ";
        msg << email.recipients[i];
    }
    msg << "\r\n";

    // Cc
    if (!email.cc.empty()) {
        msg << "Cc: " << email.cc << "\r\n";
    }

    // Subject - RFC 2047 编码（如果包含非ASCII字符则用 Base64）
    std::string subject = email.subject;
    if (subject.empty()) subject = "(No Subject)";
    msg << "Subject: =?UTF-8?B?"
        << CryptoUtils::base64Encode(subject)
        << "?=\r\n";

    // MIME
    msg << "MIME-Version: 1.0\r\n";

    bool hasHtml = !email.bodyHtml.empty();
    std::string contentType;
    if (hasHtml) {
        // multipart/alternative: 同时支持纯文本和HTML
        std::string boundary = "====SmartMail_Boundary_" + std::to_string(now) + "====";
        contentType = "multipart/alternative; boundary=\"" + boundary + "\"";
        msg << "Content-Type: " << contentType << "\r\n";
        msg << "\r\n";

        // 纯文本部分
        msg << "--" << boundary << "\r\n";
        msg << "Content-Type: text/plain; charset=UTF-8\r\n";
        msg << "\r\n";
        msg << (email.bodyPlain.empty() ? email.bodyHtml : email.bodyPlain) << "\r\n";

        // HTML 部分
        msg << "--" << boundary << "\r\n";
        msg << "Content-Type: text/html; charset=UTF-8\r\n";
        msg << "\r\n";
        msg << email.bodyHtml << "\r\n";

        msg << "--" << boundary << "--\r\n";
    } else {
        // 纯文本邮件
        msg << "Content-Type: text/plain; charset=UTF-8\r\n";
        msg << "Content-Transfer-Encoding: 7bit\r\n";
        msg << "\r\n";
        msg << (email.bodyPlain.empty() ? "(Empty body)" : email.bodyPlain) << "\r\n";
    }

    // 以 CRLF.CRLF 结尾（libcurl 的 SMTP 期望仅消息体，curl 会自己加最后的点）
    return msg.str();
}

// --- libcurl 回调 ---

size_t SmtpClient::readCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* ctx = static_cast<UploadCtx*>(userdata);
    size_t bufferSize = size * nitems;
    size_t remaining = ctx->data.size() - ctx->offset;
    size_t toCopy = (bufferSize < remaining) ? bufferSize : remaining;
    if (toCopy > 0) {
        std::memcpy(buffer, ctx->data.data() + ctx->offset, toCopy);
        ctx->offset += toCopy;
    }
    return toCopy;
}

size_t SmtpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// --- 认证密码 ---

void SmtpClient::setAuthPassword(const std::string& password) {
    authPassword_ = password;
}

// --- 获取邮件 (SMTP 不支持) ---

std::vector<Email> SmtpClient::fetchEmails(int /*limit*/) {
    return {};
}

bool SmtpClient::markAsRead(const std::string& /*emailId*/) {
    return false;
}

bool SmtpClient::deleteEmail(const std::string& /*emailId*/) {
    return false;
}

} // namespace SmartMail
