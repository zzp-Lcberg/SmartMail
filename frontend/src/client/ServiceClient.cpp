#include "ServiceClient.hpp"
#include <QUrl>
#include <QNetworkRequest>

namespace SmartMail {

ServiceClient::ServiceClient(QObject* parent)
    : QObject(parent), nam_(new QNetworkAccessManager(this)) {
}

ServiceClient::~ServiceClient() = default;

void ServiceClient::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

void ServiceClient::getAccounts(std::function<void(QJsonArray)> callback) {
    // TODO: GET /api/accounts (Phase 8)
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

void ServiceClient::addAccount(const AccountConfig& /*account*/,
                                std::function<void(bool)> callback) {
    // TODO: POST /api/accounts (Phase 8)
    callback(false);
}

void ServiceClient::updateAccount(const AccountConfig& /*account*/,
                                   std::function<void(bool)> callback) {
    // TODO: PUT /api/accounts/{id} (Phase 8)
    callback(false);
}

void ServiceClient::deleteAccount(const QString& /*id*/,
                                   std::function<void(bool)> callback) {
    // TODO: DELETE /api/accounts/{id} (Phase 8)
    callback(false);
}

void ServiceClient::getEmails(const QString& /*accountId*/,
                               const QString& /*folder*/,
                               int /*limit*/, int /*offset*/,
                               std::function<void(QJsonArray)> callback) {
    // TODO: GET /api/accounts/{id}/emails (Phase 8)
    callback({});
}

void ServiceClient::getEmailDetail(const QString& /*id*/,
                                    std::function<void(QJsonObject)> callback) {
    // TODO: GET /api/emails/{id} (Phase 8)
    callback({});
}

void ServiceClient::sendEmail(const Email& /*email*/,
                               std::function<void(bool)> callback) {
    // TODO: POST /api/emails (Phase 8)
    callback(false);
}

void ServiceClient::markAsRead(const QString& /*id*/,
                                std::function<void(bool)> callback) {
    // TODO: PUT /api/emails/{id}/read (Phase 8)
    callback(false);
}

void ServiceClient::deleteEmail(const QString& /*id*/,
                                 std::function<void(bool)> callback) {
    // TODO: DELETE /api/emails/{id} (Phase 8)
    callback(false);
}

void ServiceClient::classifyEmail(const QString& /*id*/,
                                   std::function<void(QJsonObject)> callback) {
    // TODO: POST /api/emails/{id}/classify (Phase 8)
    callback({});
}

void ServiceClient::generateReply(const QString& /*id*/,
                                   const QString& /*prompt*/,
                                   std::function<void(QJsonObject)> callback) {
    // TODO: POST /api/emails/{id}/reply (Phase 8)
    callback({});
}

void ServiceClient::search(const QString& /*query*/, const QString& /*tag*/,
                            int64_t /*fromDate*/, int64_t /*toDate*/,
                            std::function<void(QJsonArray)> callback) {
    // TODO: GET /api/search (Phase 11)
    callback({});
}

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
