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
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace SmartMail {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("设置");
    resize(600, 500);

    auto* mainLayout = new QVBoxLayout(this);
    tabs_ = new QTabWidget();

    // === 账号管理页 ===
    auto* accountPage = new QWidget();
    auto* accountLayout = new QVBoxLayout(accountPage);

    accountList_ = new QListWidget();
    accountLayout->addWidget(new QLabel("已配置账号:"));
    accountLayout->addWidget(accountList_);

    auto* formLayout = new QFormLayout();
    displayNameEdit_ = new QLineEdit();
    emailEdit_ = new QLineEdit();
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    smtpServerEdit_ = new QLineEdit();
    smtpPortSpin_ = new QSpinBox();
    smtpPortSpin_->setRange(1, 65535);
    smtpPortSpin_->setValue(465);
    imapServerEdit_ = new QLineEdit();
    imapPortSpin_ = new QSpinBox();
    imapPortSpin_->setRange(1, 65535);
    imapPortSpin_->setValue(993);
    sslCheck_ = new QCheckBox("使用 SSL/TLS");
    sslCheck_->setChecked(true);

    formLayout->addRow("显示名称:", displayNameEdit_);
    formLayout->addRow("邮箱地址:", emailEdit_);
    formLayout->addRow("密码/授权码:", passwordEdit_);
    formLayout->addRow("SMTP 服务器:", smtpServerEdit_);
    formLayout->addRow("SMTP 端口:", smtpPortSpin_);
    formLayout->addRow("IMAP 服务器:", imapServerEdit_);
    formLayout->addRow("IMAP 端口:", imapPortSpin_);
    formLayout->addRow(sslCheck_);
    accountLayout->addLayout(formLayout);

    auto* accountBtnLayout = new QHBoxLayout();
    auto* addAccountBtn = new QPushButton("添加账号");
    auto* editAccountBtn = new QPushButton("编辑账号");
    auto* removeAccountBtn = new QPushButton("删除账号");
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
    aiForm->addRow("API Key:", apiKeyEdit_);
    aiForm->addRow("API URL:", apiUrlEdit_);
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
    syncForm->addRow("同步间隔:", syncIntervalSpin_);
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
    auto* cancelBtn = new QPushButton("取消");
    bottomLayout->addStretch();
    bottomLayout->addWidget(saveBtn);
    bottomLayout->addWidget(cancelBtn);
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
