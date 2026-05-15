#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QToolBar>
#include "types/Email.hpp"

namespace SmartMail {

class MailModel;
class FolderModel;
class EmailListView;
class EmailDetailView;
class AiPanel;
class ServiceClient;
class WebSocketHandler;
class EmailComposer;
class Config;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewEmail();
    void onRefresh();
    void onSearch();
    void onOpenSettings();
    void onEmailSelected(const QString& emailId);
    void onNewEmailNotification(const QJsonObject& eventData);

private:
    void setupUi();
    void setupToolbar();
    void setupConnections();
    void connectToService();
    void openReplyComposer(const Email& original, bool forward = false);
    void loadAccounts();
    void updateFolderUnreadCounts(const std::vector<Email>& emails);
    void setOfflineMode(bool offline);

    // 核心组件
    ServiceClient* client_;
    WebSocketHandler* wsHandler_;
    MailModel* mailModel_;
    FolderModel* folderModel_;

    // 视图
    EmailListView* emailListView_;
    EmailDetailView* emailDetailView_;
    AiPanel* aiPanel_;

    // 工具栏
    QToolBar* toolbar_;
    QLineEdit* searchBox_;

    // 状态
    QString selectedEmailId_;
    Email currentEmail_;
    QString selectedAccountId_ = "default";
    QString currentFolder_ = "INBOX";
    bool isOffline_ = false;
};

} // namespace SmartMail
