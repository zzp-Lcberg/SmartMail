#include "MainWindow.hpp"
#include "EmailListView.hpp"
#include "EmailDetailView.hpp"
#include "EmailComposer.hpp"
#include "AiPanel.hpp"
#include "client/ServiceClient.hpp"
#include "client/WebSocketHandler.hpp"
#include "models/MailModel.hpp"
#include "models/FolderModel.hpp"
#include "dialogs/SettingsDialog.hpp"
#include "utils/Config.hpp"

#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

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

    // 左侧: 文件夹列表
    auto* folderView = new QListView();
    folderView->setObjectName("folderView");
    folderView->setModel(folderModel_);
    folderView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 预设默认文件夹
    std::vector<Folder> defaultFolders = {
        {"收件箱", "INBOX", 0, 0},
        {"已发送", "Sent", 0, 0},
        {"草稿箱", "Drafts", 0, 0},
        {"已删除", "Trash", 0, 0},
    };
    folderModel_->setFolders(defaultFolders);

    // 中间: 邮件列表
    emailListView_ = new EmailListView(this);
    emailListView_->setObjectName("emailListView");
    emailListView_->setModel(mailModel_);

    // 右侧: 邮件详情 + AI 面板
    auto* rightSplitter = new QSplitter(Qt::Vertical);
    emailDetailView_ = new EmailDetailView(this);
    emailDetailView_->setObjectName("emailDetailView");
    aiPanel_ = new AiPanel(this);
    aiPanel_->setObjectName("aiPanel");
    rightSplitter->addWidget(emailDetailView_);
    rightSplitter->addWidget(aiPanel_);

    centralSplitter->addWidget(folderView);
    centralSplitter->addWidget(emailListView_);
    centralSplitter->addWidget(rightSplitter);

    centralSplitter->setSizes({200, 400, 600});
    rightSplitter->setSizes({400, 200});

    setCentralWidget(centralSplitter);

    // 文件夹切换 → 加载邮件
    connect(folderView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex& current, const QModelIndex& /*previous*/) {
        if (!current.isValid()) return;
        QString folder = current.data(FolderModel::PathRole).toString();
        currentFolder_ = folder;
        client_->getEmails(selectedAccountId_, folder, 50, 0, [this](const QJsonArray& emails) {
            std::vector<Email> emailList;
            for (const auto& val : emails) {
                QJsonObject obj = val.toObject();
                Email e;
                e.id = obj["id"].toString().toStdString();
                e.accountId = obj["accountId"].toString().toStdString();
                e.sender = obj["sender"].toString().toStdString();
                e.subject = obj["subject"].toString().toStdString();
                e.receivedAt = static_cast<int64_t>(obj["receivedAt"].toDouble());
                e.isRead = obj["isRead"].toBool();
                e.isStarred = obj["isStarred"].toBool();
                e.folder = obj["folder"].toString().toStdString();
                if (!obj["aiTag"].isNull()) {
                    e.aiTag = obj["aiTag"].toString().toStdString();
                }
                emailList.push_back(e);
            }
            mailModel_->setEmails(emailList);
            updateFolderUnreadCounts(emailList);
        });
    });
}

void MainWindow::setupToolbar() {
    toolbar_ = addToolBar("主工具栏");
    toolbar_->setMovable(false);

    toolbar_->addAction("新邮件", this, &MainWindow::onNewEmail);
    toolbar_->addSeparator();
    toolbar_->addAction("回复", this, [this]() {
        if (selectedEmailId_.isEmpty()) {
            statusBar()->showMessage("请先选择一封邮件", 3000);
            return;
        }
        openReplyComposer(currentEmail_);
    });
    toolbar_->addAction("转发", this, [this]() {
        if (selectedEmailId_.isEmpty()) {
            statusBar()->showMessage("请先选择一封邮件", 3000);
            return;
        }
        openReplyComposer(currentEmail_, true);
    });
    toolbar_->addAction("删除", this, [this]() {
        if (selectedEmailId_.isEmpty()) return;
        client_->deleteEmail(selectedEmailId_, [this](bool ok) {
            if (ok) {
                mailModel_->removeEmail(selectedEmailId_.toStdString());
                emailDetailView_->clear();
                aiPanel_->showClassification("");
                statusBar()->showMessage("邮件已删除", 3000);
                selectedEmailId_.clear();
            } else {
                statusBar()->showMessage("删除失败", 3000);
            }
        });
    });
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
    toolbar_->addAction("AI 分类", this, [this]() {
        if (selectedEmailId_.isEmpty()) {
            statusBar()->showMessage("请先选择一封邮件", 3000);
            return;
        }
        QString content = QString::fromStdString(currentEmail_.bodyPlain);
        if (content.isEmpty()) content = QString::fromStdString(currentEmail_.bodyHtml);
        if (content.isEmpty()) return;
        aiPanel_->setLoading(true);
        client_->classifyEmail(selectedEmailId_, content,
            [this](const QJsonObject& result) {
            aiPanel_->setLoading(false);
            QString tag = result["tag"].toString();
            if (!tag.isEmpty()) {
                aiPanel_->showClassification(tag);
            }
        });
    });
    toolbar_->addAction("AI 回复", this, [this]() {
        if (selectedEmailId_.isEmpty()) {
            statusBar()->showMessage("请先选择一封邮件", 3000);
            return;
        }
        QString content = QString::fromStdString(currentEmail_.bodyPlain);
        if (content.isEmpty()) content = QString::fromStdString(currentEmail_.bodyHtml);
        if (content.isEmpty()) return;
        aiPanel_->setLoading(true);
        client_->generateReply(selectedEmailId_, content, "",
            [this](const QJsonObject& result) {
            aiPanel_->setLoading(false);
            QString replyContent = result["replyContent"].toString();
            if (!replyContent.isEmpty()) {
                aiPanel_->showReplyDraft(replyContent);
            }
        });
    });
    toolbar_->addSeparator();
    toolbar_->addAction("设置", this, &MainWindow::onOpenSettings);
}

void MainWindow::setupConnections() {
    // WebSocket 连接状态
    connect(wsHandler_, &WebSocketHandler::connected, this, [this]() {
        statusBar()->showMessage("WebSocket 已连接", 3000);
        if (isOffline_) {
            setOfflineMode(false);
        }
    });
    connect(wsHandler_, &WebSocketHandler::disconnected, this, [this]() {
        statusBar()->showMessage("WebSocket 连接断开");
        if (!isOffline_) {
            setOfflineMode(true);
        }
    });

    // 新邮件通知
    connect(wsHandler_, &WebSocketHandler::newEmail, this,
            &MainWindow::onNewEmailNotification);

    // 邮件已读通知
    connect(wsHandler_, &WebSocketHandler::emailRead, this,
            [this](const QString& /*emailId*/) {
        folderModel_->decrementUnread("INBOX");
    });

    // AI 分类完成 → 刷新列表
    connect(wsHandler_, &WebSocketHandler::aiClassifyDone, this,
            [this](const QJsonObject& eventData) {
        QString emailId = eventData["emailId"].toString();
        QString tag = eventData["tag"].toString();
        statusBar()->showMessage("AI 分类完成: " + emailId + " → " + tag, 3000);
    });

    // AI 回复就绪 → 显示在 AI 面板
    connect(wsHandler_, &WebSocketHandler::aiReplyReady, this,
            [this](const QJsonObject& eventData) {
        QString replyContent = eventData["replyContent"].toString();
        aiPanel_->showReplyDraft(replyContent);
    });

    // 同步进度
    connect(wsHandler_, &WebSocketHandler::syncProgress, this,
            [this](const QJsonObject& eventData) {
        QString msg = "同步中...";
        if (eventData.contains("progress")) {
            msg += " " + eventData["progress"].toString();
        }
        statusBar()->showMessage(msg);
    });

    // 连接状态变化
    connect(wsHandler_, &WebSocketHandler::connectionStatusChanged, this,
            [this](const QJsonObject& eventData) {
        QString status = eventData["status"].toString();
        statusBar()->showMessage("连接状态: " + status);
    });

    // 邮件列表选中 → 加载详情
    connect(emailListView_, &EmailListView::emailClicked, this,
            &MainWindow::onEmailSelected);

    // AiPanel 信号
    connect(aiPanel_, &AiPanel::acceptReply, this, [this](const QString& content) {
        if (selectedEmailId_.isEmpty()) return;
        auto* composer = new EmailComposer(this);
        composer->setReplyMode(currentEmail_);
        composer->setDraftContent(currentEmail_.subject, content.toStdString());
        connect(composer, &EmailComposer::emailReady, this, [this](const Email& email) {
            client_->sendEmail(email, [this](bool ok) {
                statusBar()->showMessage(ok ? "回复已发送" : "发送失败", 3000);
            });
        });
        composer->setAttribute(Qt::WA_DeleteOnClose);
        composer->show();
    });

    connect(aiPanel_, &AiPanel::regenerateReply, this, [this]() {
        if (selectedEmailId_.isEmpty()) return;
        QString content = QString::fromStdString(currentEmail_.bodyPlain);
        if (content.isEmpty()) {
            content = QString::fromStdString(currentEmail_.bodyHtml);
        }
        aiPanel_->setLoading(true);
        client_->generateReply(selectedEmailId_, content, "",
            [this](const QJsonObject& result) {
            aiPanel_->setLoading(false);
            QString replyContent = result["replyContent"].toString();
            if (!replyContent.isEmpty()) {
                aiPanel_->showReplyDraft(replyContent);
            }
        });
    });

    connect(aiPanel_, &AiPanel::editReply, this, [this](const QString& content) {
        if (selectedEmailId_.isEmpty()) return;
        auto* composer = new EmailComposer(this);
        composer->setReplyMode(currentEmail_);
        composer->setDraftContent(currentEmail_.subject, content.toStdString());
        connect(composer, &EmailComposer::emailReady, this, [this](const Email& email) {
            client_->sendEmail(email, [this](bool ok) {
                statusBar()->showMessage(ok ? "回复已发送" : "发送失败", 3000);
            });
        });
        composer->setAttribute(Qt::WA_DeleteOnClose);
        composer->show();
    });
}

void MainWindow::connectToService() {
    client_->setBaseUrl("http://127.0.0.1:8080");
    wsHandler_->connectToServer("ws://127.0.0.1:8080/ws/notifications");
    // 连接后加载账号列表
    loadAccounts();
}

void MainWindow::onNewEmail() {
    auto* composer = new EmailComposer(this);
    connect(composer, &EmailComposer::emailReady, this, [this](const Email& email) {
        client_->sendEmail(email, [this](bool ok) {
            if (ok) {
                statusBar()->showMessage("邮件已发送", 3000);
            } else {
                statusBar()->showMessage("发送失败", 3000);
            }
        });
    });
    composer->setAttribute(Qt::WA_DeleteOnClose);
    composer->show();
}

void MainWindow::onRefresh() {
    client_->getEmails(selectedAccountId_, currentFolder_, 50, 0, [this](const QJsonArray& emails) {
        std::vector<Email> emailList;
        for (const auto& val : emails) {
            QJsonObject obj = val.toObject();
            Email e;
            e.id = obj["id"].toString().toStdString();
            e.sender = obj["sender"].toString().toStdString();
            e.subject = obj["subject"].toString().toStdString();
            e.receivedAt = static_cast<int64_t>(obj["receivedAt"].toDouble());
            e.isRead = obj["isRead"].toBool();
            e.folder = obj["folder"].toString().toStdString();
            if (!obj["aiTag"].isNull()) {
                e.aiTag = obj["aiTag"].toString().toStdString();
            }
            emailList.push_back(e);
        }
        mailModel_->setEmails(emailList);
        updateFolderUnreadCounts(emailList);
        statusBar()->showMessage("刷新完成", 3000);
    });
}

void MainWindow::onSearch() {
    QString query = searchBox_->text().trimmed();
    if (query.isEmpty()) return;
    client_->search(query);
    statusBar()->showMessage("搜索: " + query);
}

void MainWindow::onOpenSettings() {
    auto* dlg = new SettingsDialog(this);
    dlg->setServiceClient(client_);

    static Config config("smartmail.ini");
    dlg->setConfig(&config);

    // 加载账号列表和设置
    dlg->loadAccounts();
    dlg->loadSettings();

    if (dlg->exec() == QDialog::Accepted) {
        statusBar()->showMessage("设置已保存", 3000);
        // 刷新账号选择
        loadAccounts();
    }
    dlg->deleteLater();
}

void MainWindow::onEmailSelected(const QString& emailId) {
    selectedEmailId_ = emailId;
    client_->getEmailDetail(emailId, [this, emailId](const QJsonObject& detail) {
        currentEmail_.id = emailId.toStdString();
        currentEmail_.sender = detail["sender"].toString().toStdString();
        currentEmail_.subject = detail["subject"].toString().toStdString();
        currentEmail_.bodyPlain = detail["body_plain"].toString().toStdString();
        currentEmail_.bodyHtml = detail["body_html"].toString().toStdString();
        currentEmail_.folder = detail["folder"].toString().toStdString();
        if (!detail["ai_tag"].isNull()) {
            currentEmail_.aiTag = detail["ai_tag"].toString().toStdString();
        }

        emailDetailView_->showEmail(detail);

        // 触发 AI 分类
        QString content = QString::fromStdString(currentEmail_.bodyPlain);
        if (content.isEmpty()) {
            content = QString::fromStdString(currentEmail_.bodyHtml);
        }
        if (!content.isEmpty()) {
            aiPanel_->setLoading(true);
            client_->classifyEmail(emailId, content,
                [this](const QJsonObject& result) {
                aiPanel_->setLoading(false);
                QString tag = result["tag"].toString();
                if (!tag.isEmpty()) {
                    aiPanel_->showClassification(tag);
                }
            });
        }
    });
}

void MainWindow::onNewEmailNotification(const QJsonObject& eventData) {
    QString subject = eventData["subject"].toString();
    statusBar()->showMessage("新邮件: " + subject, 5000);
    // 实时更新未读计数
    folderModel_->incrementUnread("INBOX");
}

void MainWindow::openReplyComposer(const Email& original, bool forward) {
    auto* composer = new EmailComposer(this);
    if (forward) {
        composer->setForwardMode(original);
    } else {
        composer->setReplyMode(original);
    }
    connect(composer, &EmailComposer::emailReady, this, [this](const Email& email) {
        client_->sendEmail(email, [this](bool ok) {
            statusBar()->showMessage(ok ? "邮件已发送" : "发送失败", 3000);
        });
    });
    composer->setAttribute(Qt::WA_DeleteOnClose);
    composer->show();
}

void MainWindow::loadAccounts() {
    client_->getAccounts([this](const QJsonArray& arr) {
        if (!arr.isEmpty()) {
            QJsonObject first = arr[0].toObject();
            selectedAccountId_ = first["id"].toString();
            // 加载选中账号的收件箱
            onRefresh();
        }
    });
}

void MainWindow::updateFolderUnreadCounts(const std::vector<Email>& emails) {
    std::map<std::string, int> unreadByFolder;
    for (const auto& e : emails) {
        if (!e.isRead) {
            unreadByFolder[e.folder]++;
        }
    }
    for (const auto& [folder, count] : unreadByFolder) {
        folderModel_->updateUnreadCount(folder, count);
    }
}

void MainWindow::setOfflineMode(bool offline) {
    isOffline_ = offline;
    if (offline) {
        statusBar()->showMessage("离线模式 - 显示缓存数据");
        toolbar_->setEnabled(false);
    } else {
        statusBar()->showMessage("已重新连接", 3000);
        toolbar_->setEnabled(true);
        onRefresh();
    }
}

} // namespace SmartMail
