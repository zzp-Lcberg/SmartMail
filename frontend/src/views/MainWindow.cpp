#include "MainWindow.hpp"
#include "EmailListView.hpp"
#include "EmailDetailView.hpp"
#include "AiPanel.hpp"
#include "client/ServiceClient.hpp"
#include "client/WebSocketHandler.hpp"
#include "models/MailModel.hpp"
#include "models/FolderModel.hpp"
#include "dialogs/SettingsDialog.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>

namespace SmartMail {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , client_(new ServiceClient(this))
    , wsHandler_(new WebSocketHandler(this))
    , mailModel_(new MailModel(this))
    , folderModel_(new FolderModel(this)) {
    setWindowTitle("SmartMail Desktop");
    resize(1200, 750);
    setupUi();
    setupToolbar();
    setupConnections();
    connectToService();
    statusBar()->showMessage("就绪");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    // 中央三栏布局
    auto* centralSplitter = new QSplitter(Qt::Horizontal, this);

    // 左侧: 账号/文件夹列表 (占位)
    auto* folderWidget = new QWidget();
    auto* folderLayout = new QVBoxLayout(folderWidget);
    folderLayout->addWidget(new QLabel("账号列表(开发中)"));
    folderLayout->addStretch();

    // 中间: 邮件列表
    emailListView_ = new EmailListView(this);
    emailListView_->setModel(mailModel_);

    // 右侧: 邮件详情 + AI 面板
    auto* rightSplitter = new QSplitter(Qt::Vertical);
    emailDetailView_ = new EmailDetailView(this);
    aiPanel_ = new AiPanel(this);
    rightSplitter->addWidget(emailDetailView_);
    rightSplitter->addWidget(aiPanel_);

    centralSplitter->addWidget(folderWidget);
    centralSplitter->addWidget(emailListView_);
    centralSplitter->addWidget(rightSplitter);

    centralSplitter->setSizes({200, 400, 600});
    rightSplitter->setSizes({400, 200});

    setCentralWidget(centralSplitter);
}

void MainWindow::setupToolbar() {
    toolbar_ = addToolBar("主工具栏");
    toolbar_->setMovable(false);

    toolbar_->addAction("新邮件", this, &MainWindow::onNewEmail);
    toolbar_->addSeparator();
    toolbar_->addAction("回复", this, [this]() { /* TODO */ });
    toolbar_->addAction("转发", this, [this]() { /* TODO */ });
    toolbar_->addAction("删除", this, [this]() { /* TODO */ });
    toolbar_->addSeparator();
    toolbar_->addAction("刷新", this, &MainWindow::onRefresh);

    // 搜索框
    toolbar_->addSeparator();
    searchBox_ = new QLineEdit();
    searchBox_->setPlaceholderText("搜索邮件...");
    searchBox_->setMaximumWidth(250);
    toolbar_->addWidget(searchBox_);
    toolbar_->addAction("搜索", this, &MainWindow::onSearch);

    toolbar_->addSeparator();
    toolbar_->addAction("设置", this, &MainWindow::onOpenSettings);
}

void MainWindow::setupConnections() {
    // TODO: 连接 WebSocket 信号到对应槽 (Phase 8)
}

void MainWindow::connectToService() {
    // TODO: 启动连接后端服务 (Phase 10)
    client_->setBaseUrl("http://127.0.0.1:8080");
    wsHandler_->connectToServer("ws://127.0.0.1:8080/ws/notifications");
}

void MainWindow::onNewEmail() {
    // TODO: 打开 EmailComposer (Phase 9)
    statusBar()->showMessage("撰写新邮件...");
}

void MainWindow::onRefresh() {
    // TODO: 触发邮件刷新 (Phase 10)
    statusBar()->showMessage("刷新中...");
}

void MainWindow::onSearch() {
    QString query = searchBox_->text().trimmed();
    if (query.isEmpty()) return;
    client_->search(query);
    statusBar()->showMessage("搜索: " + query);
}

void MainWindow::onOpenSettings() {
    // TODO: 打开设置对话框 (Phase 10)
    statusBar()->showMessage("打开设置...");
}

void MainWindow::onEmailSelected(const QString& emailId) {
    // TODO: 加载邮件详情 (Phase 9)
    client_->getEmailDetail(emailId, [this](const QJsonObject& detail) {
        emailDetailView_->showEmail(detail);
    });
}

void MainWindow::onNewEmailNotification(const QJsonObject& /*data*/) {
    // TODO: 处理新邮件通知 (Phase 10)
    statusBar()->showMessage("收到新邮件!", 3000);
}

} // namespace SmartMail
