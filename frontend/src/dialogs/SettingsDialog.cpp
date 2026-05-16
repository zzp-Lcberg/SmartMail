#include "SettingsDialog.hpp"
#include "AccountDialog.hpp"
#include "client/ServiceClient.hpp"
#include "utils/Config.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QPalette>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace SmartMail {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("设置");
    resize(600, 500);

    // QPalette 兜底暗色背景
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor("#1a1d23"));
    darkPalette.setColor(QPalette::WindowText, QColor("#e1e5ee"));
    darkPalette.setColor(QPalette::Base, QColor("#1a1d23"));
    darkPalette.setColor(QPalette::AlternateBase, QColor("#21252b"));
    darkPalette.setColor(QPalette::Text, QColor("#e1e5ee"));
    darkPalette.setColor(QPalette::Button, QColor("#333842"));
    darkPalette.setColor(QPalette::ButtonText, QColor("#e1e5ee"));
    darkPalette.setColor(QPalette::Highlight, QColor("#5c7cfa"));
    darkPalette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    darkPalette.setColor(QPalette::PlaceholderText, QColor("#555b68"));
    setPalette(darkPalette);
    setAutoFillBackground(true);

    // QSS 微调
    setStyleSheet(
        "SettingsDialog, SettingsDialog QWidget {"
        "  color: #e1e5ee;"
        "  background-color: #1a1d23;"
        "}"
        "SettingsDialog QTabWidget::pane {"
        "  border: 1px solid #333842;"
        "}"
        "SettingsDialog QTabBar::tab {"
        "  background-color: #21252b; color: #a0a8b8;"
        "  padding: 8px 20px; border: 1px solid #333842;"
        "  border-bottom: none; margin-right: 2px;"
        "}"
        "SettingsDialog QTabBar::tab:selected {"
        "  background-color: #1a1d23; color: #5c7cfa;"
        "  border-bottom: 2px solid #5c7cfa;"
        "}"
        "SettingsDialog QGroupBox {"
        "  background-color: #21252b; color: #e1e5ee;"
        "  border: 1px solid #333842; border-radius: 4px;"
        "  margin-top: 12px; padding: 16px 12px 12px 12px;"
        "  font-weight: bold;"
        "}"
        "SettingsDialog QGroupBox::title {"
        "  background-color: #21252b;"
        "  subcontrol-origin: margin; left: 12px;"
        "  padding: 0 6px; color: #5c7cfa;"
        "}"
        "SettingsDialog QLabel {"
        "  background-color: transparent; color: #e1e5ee; font-size: 12px;"
        "}"
        "SettingsDialog QLineEdit {"
        "  background-color: #1a1d23; color: #e1e5ee;"
        "  border: 1px solid #333842; border-radius: 4px;"
        "  padding: 6px 8px; font-size: 12px;"
        "}"
        "SettingsDialog QLineEdit:focus { border: 1px solid #5c7cfa; }"
        "SettingsDialog QSpinBox {"
        "  background-color: #1a1d23; color: #e1e5ee;"
        "  border: 1px solid #333842; border-radius: 4px;"
        "  padding: 6px 8px;"
        "}"
        "SettingsDialog QCheckBox {"
        "  background-color: transparent; color: #e1e5ee;"
        "}"
        "SettingsDialog QListWidget {"
        "  background-color: #1a1d23; color: #e1e5ee;"
        "  border: 1px solid #333842; border-radius: 4px;"
        "}"
        "SettingsDialog QListWidget::item {"
        "  padding: 6px 8px;"
        "}"
        "SettingsDialog QListWidget::item:selected {"
        "  background-color: #2a3050; color: #ffffff;"
        "}"
        "SettingsDialog QPushButton {"
        "  background-color: #333842; color: #e1e5ee;"
        "  border: 1px solid #3a3f4b; border-radius: 4px;"
        "  padding: 6px 16px; font-size: 12px;"
        "}"
        "SettingsDialog QPushButton:hover { background-color: #3a404e; }"
    );

    auto* mainLayout = new QVBoxLayout(this);
    tabs_ = new QTabWidget();

    // Helper: 手动创建标签确保暗色渲染
    auto makeLabel = [](const QString& text) {
        auto* label = new QLabel(text);
        label->setStyleSheet("color: #e1e5ee; font-size: 12px; background-color: transparent;");
        return label;
    };

    // === 账号管理页 ===
    auto* accountPage = new QWidget();
    auto* accountLayout = new QVBoxLayout(accountPage);

    auto* accountHint = new QLabel("点击「添加账号」选择邮箱类型并填写账号信息");
    accountHint->setStyleSheet("color: #6a7180; font-size: 11px; background-color: transparent;");
    accountLayout->addWidget(accountHint);

    accountList_ = new QListWidget();
    accountList_->setMinimumHeight(120);
    accountLayout->addWidget(makeLabel("已配置账号:"));
    accountLayout->addWidget(accountList_);

    auto* accountBtnLayout = new QHBoxLayout();
    auto* addAccountBtn = new QPushButton("添加账号");
    auto* editAccountBtn = new QPushButton("编辑账号");
    auto* removeAccountBtn = new QPushButton("删除账号");
    addAccountBtn->setStyleSheet(
        "QPushButton { background-color: #5c7cfa; color: #ffffff; border: none;"
        "  border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #6b8afb; }"
    );
    accountBtnLayout->addWidget(addAccountBtn);
    accountBtnLayout->addWidget(editAccountBtn);
    accountBtnLayout->addWidget(removeAccountBtn);
    accountBtnLayout->addStretch();
    accountLayout->addLayout(accountBtnLayout);

    connect(addAccountBtn, &QPushButton::clicked, this, &SettingsDialog::onAddAccount);
    connect(editAccountBtn, &QPushButton::clicked, this, &SettingsDialog::onEditAccount);
    connect(removeAccountBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveAccount);
    connect(accountList_, &QListWidget::itemDoubleClicked, this, &SettingsDialog::onEditAccount);

    // === AI 设置页 ===
    auto* aiPage = new QWidget();
    auto* aiLayout = new QVBoxLayout(aiPage);
    auto* aiGroup = new QGroupBox("OpenAI 配置");
    auto* aiForm = new QFormLayout(aiGroup);
    apiKeyEdit_ = new QLineEdit();
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    apiUrlEdit_ = new QLineEdit("https://api.openai.com/v1/chat/completions");
    autoClassifyCheck_ = new QCheckBox("新邮件自动 AI 分类");
    autoClassifyCheck_->setChecked(true);
    aiForm->addRow(makeLabel("API Key:"), apiKeyEdit_);
    aiForm->addRow(makeLabel("API URL:"), apiUrlEdit_);
    aiForm->addRow(autoClassifyCheck_);
    aiLayout->addWidget(aiGroup);
    aiLayout->addStretch();

    // === 通用设置页 ===
    auto* generalPage = new QWidget();
    auto* generalLayout = new QVBoxLayout(generalPage);
    auto* syncGroup = new QGroupBox("同步设置");
    auto* syncForm = new QFormLayout(syncGroup);
    syncIntervalSpin_ = new QSpinBox();
    syncIntervalSpin_->setRange(30, 3600);
    syncIntervalSpin_->setValue(300);
    syncIntervalSpin_->setSuffix(" 秒");
    syncForm->addRow(makeLabel("同步间隔:"), syncIntervalSpin_);
    generalLayout->addWidget(syncGroup);

    minimizeToTrayCheck_ = new QCheckBox("关闭时最小化到托盘");
    generalLayout->addWidget(minimizeToTrayCheck_);
    generalLayout->addStretch();

    tabs_->addTab(accountPage, "账号管理");
    tabs_->addTab(aiPage, "AI 设置");
    tabs_->addTab(generalPage, "通用设置");

    mainLayout->addWidget(tabs_);

    // 底部按钮
    auto* bottomLayout = new QHBoxLayout();
    auto* saveBtn = new QPushButton("保存");
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #5c7cfa; color: #ffffff; border: none;"
        "  border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #6b8afb; }"
    );
    auto* cancelBtn = new QPushButton("取消");
    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(saveBtn);
    mainLayout->addLayout(bottomLayout);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::setServiceClient(ServiceClient* client) {
    client_ = client;
}

void SettingsDialog::setConfig(Config* config) {
    config_ = config;
    // 加载本地配置
    if (config_) {
        syncIntervalSpin_->setValue(config_->getInt("sync_interval", 300));
        minimizeToTrayCheck_->setChecked(config_->getBool("minimize_to_tray", false));
    }
}

void SettingsDialog::loadAccounts() {
    if (!client_) return;
    client_->getAccounts([this](const QJsonArray& arr) {
        accounts_.clear();
        accountList_->clear();
        for (const auto& val : arr) {
            QJsonObject obj = val.toObject();
            AccountConfig acc;
            acc.id = obj["id"].toString().toStdString();
            acc.displayName = obj["displayName"].toString().toStdString();
            acc.email = obj["email"].toString().toStdString();
            acc.smtpServer = obj["smtpServer"].toString().toStdString();
            acc.smtpPort = obj["smtpPort"].toInt(465);
            acc.smtpUseSSL = obj["smtpUseSSL"].toBool(true);
            acc.imapServer = obj["imapServer"].toString().toStdString();
            acc.imapPort = obj["imapPort"].toInt(993);
            acc.imapUseSSL = obj["imapUseSSL"].toBool(true);
            acc.syncInterval = obj["syncInterval"].toInt(300);
            acc.autoClassify = obj["autoClassify"].toBool(true);

            QString protocol = obj["preferredProtocol"].toString();
            acc.preferredProtocol = (protocol == "POP3")
                ? AccountConfig::Protocol::POP3
                : AccountConfig::Protocol::IMAP;

            accounts_.push_back(acc);

            QString label = QString::fromStdString(acc.displayName)
                          + " <" + QString::fromStdString(acc.email) + ">";
            accountList_->addItem(label);
        }
    });
}

void SettingsDialog::loadSettings() {
    if (!client_) return;
    client_->getSettings([this](const QJsonObject& settings) {
        if (settings.contains("api_key") && !settings["api_key"].toString().isEmpty())
            apiKeyEdit_->setText(settings["api_key"].toString());
        if (settings.contains("base_url") && !settings["base_url"].toString().isEmpty())
            apiUrlEdit_->setText(settings["base_url"].toString());
        if (settings.contains("auto_classify"))
            autoClassifyCheck_->setChecked(settings["auto_classify"].toString() == "true");
    });
}

void SettingsDialog::onSave() {
    // 1. Save AI settings to backend
    if (client_) {
        QJsonObject aiSettings;
        aiSettings["api_key"] = apiKeyEdit_->text();
        aiSettings["base_url"] = apiUrlEdit_->text();
        aiSettings["auto_classify"] = autoClassifyCheck_->isChecked() ? "true" : "false";
        client_->updateSettings(aiSettings, [](bool) {});

        // 2. Save sync interval to backend
        QJsonObject generalSettings;
        generalSettings["sync_interval"] = QString::number(syncIntervalSpin_->value());
        client_->updateSettings(generalSettings, [](bool) {});
    }

    // 3. Save local UI preferences
    if (config_) {
        config_->setInt("sync_interval", syncIntervalSpin_->value());
        config_->setBool("minimize_to_tray", minimizeToTrayCheck_->isChecked());
        config_->save();
    }

    accept();
}

void SettingsDialog::onAddAccount() {
    auto* dlg = new AccountDialog(this);
    dlg->setServiceClient(client_);
    if (dlg->exec() == QDialog::Accepted) {
        AccountConfig acc = dlg->getAccount();
        if (client_) {
            client_->addAccount(acc, [this](bool ok) {
                if (ok) {
                    loadAccounts();
                } else {
                    QMessageBox::warning(this, "错误", "添加账号失败");
                }
            });
        }
    }
    dlg->deleteLater();
}

void SettingsDialog::onEditAccount() {
    int row = accountList_->currentRow();
    if (row < 0 || row >= static_cast<int>(accounts_.size())) return;

    auto* dlg = new AccountDialog(this);
    dlg->setServiceClient(client_);
    dlg->setEditMode(accounts_[row].id);
    dlg->setAccount(accounts_[row]);
    if (dlg->exec() == QDialog::Accepted) {
        AccountConfig acc = dlg->getAccount();
        acc.id = accounts_[row].id;
        if (client_) {
            client_->updateAccount(acc, [this](bool ok) {
                if (ok) {
                    loadAccounts();
                } else {
                    QMessageBox::warning(this, "错误", "更新账号失败");
                }
            });
        }
    }
    dlg->deleteLater();
}

void SettingsDialog::onRemoveAccount() {
    int row = accountList_->currentRow();
    if (row < 0 || row >= static_cast<int>(accounts_.size())) return;

    auto answer = QMessageBox::question(this, "确认删除",
        QString::fromStdString("确定要删除账号 " + accounts_[row].email + " 吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        QString id = QString::fromStdString(accounts_[row].id);
        if (client_) {
            client_->deleteAccount(id, [this](bool ok) {
                if (ok) {
                    loadAccounts();
                } else {
                    QMessageBox::warning(this, "错误", "删除账号失败");
                }
            });
        }
    }
}

} // namespace SmartMail
