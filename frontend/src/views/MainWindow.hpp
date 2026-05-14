#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QToolBar>

namespace SmartMail {

class MailModel;
class FolderModel;
class EmailListView;
class EmailDetailView;
class AiPanel;
class ServiceClient;
class WebSocketHandler;

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
    void onNewEmailNotification(const QJsonObject& data);

private:
    void setupUi();
    void setupToolbar();
    void setupConnections();
    void connectToService();

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
};

} // namespace SmartMail
