#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include "types/Email.hpp"
#include "types/AccountConfig.hpp"
#include "types/AiResult.hpp"

namespace SmartMail {

/// HTTP API 客户端封装
class ServiceClient : public QObject {
    Q_OBJECT
public:
    explicit ServiceClient(QObject* parent = nullptr);
    ~ServiceClient() override;

    void setBaseUrl(const QString& url);

    // 账号 API
    void getAccounts(std::function<void(QJsonArray)> callback);
    void addAccount(const AccountConfig& account, std::function<void(bool)> callback);
    void updateAccount(const AccountConfig& account, std::function<void(bool)> callback);
    void deleteAccount(const QString& id, std::function<void(bool)> callback);

    // 邮件 API
    void getEmails(const QString& accountId, const QString& folder = "INBOX",
                   int limit = 50, int offset = 0,
                   std::function<void(QJsonArray)> callback = {});
    void getEmailDetail(const QString& id,
                        std::function<void(QJsonObject)> callback);
    void sendEmail(const Email& email, std::function<void(bool)> callback);
    void markAsRead(const QString& id, std::function<void(bool)> callback);
    void deleteEmail(const QString& id, std::function<void(bool)> callback);

    // AI API
    void classifyEmail(const QString& id, const QString& content,
                       std::function<void(QJsonObject)> callback);
    void generateReply(const QString& id, const QString& originalEmail,
                       const QString& prompt,
                       std::function<void(QJsonObject)> callback);

    // 连接测试 & 设置
    void testConnection(const AccountConfig& account,
                        std::function<void(bool, QString)> callback);
    void getSettings(std::function<void(QJsonObject)> callback);
    void updateSettings(const QJsonObject& settings,
                        std::function<void(bool)> callback);

    // 搜索
    void search(const QString& query, const QString& tag = "",
                int64_t fromDate = 0, int64_t toDate = 0,
                std::function<void(QJsonArray)> callback = {});

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private:
    QNetworkReply* get(const QString& path);
    QNetworkReply* post(const QString& path, const QJsonObject& body);
    QNetworkReply* put(const QString& path, const QJsonObject& body);
    QNetworkReply* del(const QString& path);

    QNetworkAccessManager* nam_;
    QString baseUrl_;
};

} // namespace SmartMail
