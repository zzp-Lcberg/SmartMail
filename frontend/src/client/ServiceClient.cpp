#include "ServiceClient.hpp"
#include <QUrl>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace SmartMail {

// ============================================================================
// Serialization helpers
// ============================================================================

static QJsonObject accountToJson(const AccountConfig& acc) {
    QJsonObject j;
    j["id"] = QString::fromStdString(acc.id);
    j["displayName"] = QString::fromStdString(acc.displayName);
    j["email"] = QString::fromStdString(acc.email);
    j["encryptedPassword"] = QString::fromStdString(acc.encryptedPassword);
    j["smtpServer"] = QString::fromStdString(acc.smtpServer);
    j["smtpPort"] = acc.smtpPort;
    j["smtpUseSSL"] = acc.smtpUseSSL;
    j["imapServer"] = QString::fromStdString(acc.imapServer);
    j["imapPort"] = acc.imapPort;
    j["imapUseSSL"] = acc.imapUseSSL;
    j["pop3Server"] = QString::fromStdString(acc.pop3Server);
    j["pop3Port"] = acc.pop3Port;
    j["pop3UseSSL"] = acc.pop3UseSSL;
    j["preferredProtocol"] = (acc.preferredProtocol == AccountConfig::Protocol::IMAP)
        ? "IMAP" : "POP3";
    j["syncInterval"] = acc.syncInterval;
    j["maxFetchCount"] = acc.maxFetchCount;
    j["autoClassify"] = acc.autoClassify;
    return j;
}

static QJsonObject emailToJson(const Email& email) {
    QJsonObject j;
    j["id"] = QString::fromStdString(email.id);
    j["accountId"] = QString::fromStdString(email.accountId);
    j["sender"] = QString::fromStdString(email.sender);

    QJsonArray recipients;
    for (const auto& r : email.recipients) {
        recipients.append(QString::fromStdString(r));
    }
    j["recipients"] = recipients;

    j["cc"] = QString::fromStdString(email.cc);
    j["bcc"] = QString::fromStdString(email.bcc);
    j["subject"] = QString::fromStdString(email.subject);
    j["bodyPlain"] = QString::fromStdString(email.bodyPlain);
    j["bodyHtml"] = QString::fromStdString(email.bodyHtml);
    j["receivedAt"] = static_cast<qint64>(email.receivedAt);
    j["sentAt"] = static_cast<qint64>(email.sentAt);
    j["isRead"] = email.isRead;
    j["isStarred"] = email.isStarred;
    j["folder"] = QString::fromStdString(email.folder);

    QJsonArray attachments;
    for (const auto& a : email.attachments) {
        attachments.append(QString::fromStdString(a));
    }
    j["attachments"] = attachments;

    if (email.aiTag.has_value()) {
        j["aiTag"] = QString::fromStdString(email.aiTag.value());
    } else {
        j["aiTag"] = QJsonValue::Null;
    }
    return j;
}

// ============================================================================
// ServiceClient lifecycle
// ============================================================================

ServiceClient::ServiceClient(QObject* parent)
    : QObject(parent), nam_(new QNetworkAccessManager(this)) {
}

ServiceClient::~ServiceClient() = default;

void ServiceClient::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

// ============================================================================
// Account API
// ============================================================================

void ServiceClient::getAccounts(std::function<void(QJsonArray)> callback) {
    auto* reply = get("/api/accounts");
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.array());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

void ServiceClient::addAccount(const AccountConfig& account,
                                std::function<void(bool)> callback) {
    QJsonObject body = accountToJson(account);
    auto* reply = post("/api/accounts", body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

void ServiceClient::updateAccount(const AccountConfig& account,
                                   std::function<void(bool)> callback) {
    QJsonObject body = accountToJson(account);
    QString path = "/api/accounts/" + QString::fromStdString(account.id);
    auto* reply = put(path, body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

void ServiceClient::deleteAccount(const QString& id,
                                   std::function<void(bool)> callback) {
    QString path = "/api/accounts/" + id;
    auto* reply = del(path);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

// ============================================================================
// Email API
// ============================================================================

void ServiceClient::getEmails(const QString& accountId,
                               const QString& folder,
                               int limit, int offset,
                               std::function<void(QJsonArray)> callback) {
    QString path = "/api/accounts/" + accountId + "/emails";
    QUrl url(baseUrl_ + path);
    QUrlQuery query;
    query.addQueryItem("folder", folder);
    query.addQueryItem("limit", QString::number(limit));
    query.addQueryItem("offset", QString::number(offset));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto* reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.array());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

void ServiceClient::getEmailDetail(const QString& id,
                                    std::function<void(QJsonObject)> callback) {
    QString path = "/api/emails/" + id;
    auto* reply = get(path);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.object());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

void ServiceClient::sendEmail(const Email& email,
                               std::function<void(bool)> callback) {
    QJsonObject body = emailToJson(email);
    auto* reply = post("/api/emails", body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

void ServiceClient::markAsRead(const QString& id,
                                std::function<void(bool)> callback) {
    QString path = "/api/emails/" + id + "/read";
    auto* reply = put(path, {});
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

void ServiceClient::deleteEmail(const QString& id,
                                 std::function<void(bool)> callback) {
    QString path = "/api/emails/" + id;
    auto* reply = del(path);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

// ============================================================================
// AI API
// ============================================================================

void ServiceClient::classifyEmail(const QString& id,
                                   const QString& content,
                                   std::function<void(QJsonObject)> callback) {
    QString path = "/api/emails/" + id + "/classify";
    QJsonObject body;
    body["content"] = content;
    auto* reply = post(path, body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.object());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

void ServiceClient::generateReply(const QString& id,
                                   const QString& originalEmail,
                                   const QString& prompt,
                                   std::function<void(QJsonObject)> callback) {
    QString path = "/api/emails/" + id + "/reply";
    QJsonObject body;
    body["originalEmail"] = originalEmail;
    body["userPrompt"] = prompt;
    auto* reply = post(path, body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.object());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

// ============================================================================
// Connection Test & Settings API
// ============================================================================

void ServiceClient::testConnection(const AccountConfig& account,
                                    std::function<void(bool, QString)> callback) {
    QJsonObject body = accountToJson(account);
    auto* reply = post("/api/test-connection", body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            callback(obj["success"].toBool(), obj["message"].toString());
        } else {
            callback(false, reply->errorString());
        }
        reply->deleteLater();
    });
}

void ServiceClient::getSettings(std::function<void(QJsonObject)> callback) {
    auto* reply = get("/api/settings");
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            callback(doc.object());
        } else {
            callback({});
        }
        reply->deleteLater();
    });
}

void ServiceClient::updateSettings(const QJsonObject& settings,
                                    std::function<void(bool)> callback) {
    auto* reply = post("/api/settings", settings);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        callback(reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    });
}

// ============================================================================
// Search API (Phase 11)
// ============================================================================

void ServiceClient::search(const QString& /*query*/, const QString& /*tag*/,
                            int64_t /*fromDate*/, int64_t /*toDate*/,
                            std::function<void(QJsonArray)> callback) {
    // TODO: GET /api/search (Phase 11)
    callback({});
}

// ============================================================================
// Private HTTP helpers
// ============================================================================

QNetworkReply* ServiceClient::get(const QString& path) {
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return nam_->get(request);
}

QNetworkReply* ServiceClient::post(const QString& path, const QJsonObject& body) {
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(body);
    return nam_->post(request, doc.toJson());
}

QNetworkReply* ServiceClient::put(const QString& path, const QJsonObject& body) {
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(body);
    return nam_->put(request, doc.toJson());
}

QNetworkReply* ServiceClient::del(const QString& path) {
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return nam_->deleteResource(request);
}

} // namespace SmartMail
