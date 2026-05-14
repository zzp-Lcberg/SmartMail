#pragma once

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>

namespace SmartMail {

/// WebSocket 通知处理
class WebSocketHandler : public QObject {
    Q_OBJECT
public:
    explicit WebSocketHandler(QObject* parent = nullptr);
    ~WebSocketHandler() override;

    void connectToServer(const QString& url);
    void disconnectFromServer();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void newEmail(const QJsonObject& data);
    void emailRead(const QString& emailId);
    void aiClassifyDone(const QJsonObject& data);
    void aiReplyReady(const QJsonObject& data);
    void connectionStatusChanged(const QJsonObject& data);
    void syncProgress(const QJsonObject& data);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    void handleEvent(const QString& event, const QJsonObject& data);

    QWebSocket* socket_;
    QTimer* reconnectTimer_;
    QString url_;
    int reconnectAttempts_ = 0;
    static constexpr int MAX_RECONNECT_DELAY = 30000; // 30s
};

} // namespace SmartMail
