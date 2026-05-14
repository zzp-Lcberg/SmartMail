#include "SettingsDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

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
    auto* removeAccountBtn = new QPushButton("删除账号");
    accountBtnLayout->addWidget(addAccountBtn);
    accountBtnLayout->addWidget(removeAccountBtn);
    accountBtnLayout->addStretch();
    accountLayout->addLayout(accountBtnLayout);

    connect(addAccountBtn, &QPushButton::clicked, this, &SettingsDialog::onAddAccount);
    connect(removeAccountBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveAccount);

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

void SettingsDialog::onSave() {
    // TODO: 保存所有设置 (Phase 10)
    QMessageBox::information(this, "提示", "设置已保存（功能开发中）");
    accept();
}

void SettingsDialog::onAddAccount() {
    // TODO: 添加账号 (Phase 10)
    QMessageBox::information(this, "提示", "账号添加功能开发中");
}

void SettingsDialog::onRemoveAccount() {
    // TODO: 删除账号 (Phase 10)
    QMessageBox::information(this, "提示", "账号删除功能开发中");
}

} // namespace SmartMail
