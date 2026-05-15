#include "WebSocketHandler.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace SmartMail {

WebSocketHandler::WebSocketHandler(QObject* parent)
    : QObject(parent)
    , socket_(new QWebSocket(QString(), QWebSocketProtocol::Version13, this))
    , reconnectTimer_(new QTimer(this)) {
    reconnectTimer_->setSingleShot(true);
    connect(socket_, &QWebSocket::connected, this, &WebSocketHandler::onConnected);
    connect(socket_, &QWebSocket::disconnected, this, &WebSocketHandler::onDisconnected);
    connect(socket_, &QWebSocket::textMessageReceived,
            this, &WebSocketHandler::onTextMessageReceived);
    connect(socket_, &QWebSocket::errorOccurred,
            this, &WebSocketHandler::onError);
    connect(reconnectTimer_, &QTimer::timeout, this, &WebSocketHandler::onReconnectTimer);
}

WebSocketHandler::~WebSocketHandler() {
    disconnectFromServer();
}

void WebSocketHandler::connectToServer(const QString& url) {
    url_ = url;
    reconnectAttempts_ = 0;
    qDebug() << "WebSocket connecting to:" << url;
    socket_->open(QUrl(url));
}

void WebSocketHandler::disconnectFromServer() {
    reconnectTimer_->stop();
    socket_->close();
}

bool WebSocketHandler::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void WebSocketHandler::onConnected() {
    reconnectAttempts_ = 0;
    qDebug() << "WebSocket connected";
    emit connected();
}

void WebSocketHandler::onDisconnected() {
    qDebug() << "WebSocket disconnected";
    emit disconnected();
    // 自动重连
    int delay = std::min(1000 * (1 << reconnectAttempts_), MAX_RECONNECT_DELAY);
    reconnectTimer_->start(delay);
    reconnectAttempts_++;
}

void WebSocketHandler::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    QString event = obj["event"].toString();
    QJsonObject data = obj["data"].toObject();
    handleEvent(event, data);
}

void WebSocketHandler::onError(QAbstractSocket::SocketError /*error*/) {
    qWarning() << "WebSocket error:" << socket_->errorString();
}

void WebSocketHandler::onReconnectTimer() {
    if (!isConnected()) {
        qDebug() << "WebSocket reconnecting...";
        socket_->open(QUrl(url_));
    }
}

void WebSocketHandler::handleEvent(const QString& event, const QJsonObject& data) {
    if (event == "new_email") emit newEmail(data);
    else if (event == "email_read") emit emailRead(data["email_id"].toString());
    else if (event == "ai_classify_done") emit aiClassifyDone(data);
    else if (event == "ai_reply_ready") emit aiReplyReady(data);
    else if (event == "connection_status") emit connectionStatusChanged(data);
    else if (event == "sync_progress") emit syncProgress(data);
}

} // namespace SmartMail
