#include "AccountDialog.hpp"
#include "client/ServiceClient.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

namespace SmartMail {

AccountDialog::AccountDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("添加/编辑账号");
    resize(450, 400);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    displayNameEdit_ = new QLineEdit();
    emailEdit_ = new QLineEdit();
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("邮箱授权码或密码");

    smtpServerEdit_ = new QLineEdit();
    smtpServerEdit_->setPlaceholderText("smtp.gmail.com");
    smtpPortSpin_ = new QSpinBox();
    smtpPortSpin_->setRange(1, 65535);
    smtpPortSpin_->setValue(465);

    imapServerEdit_ = new QLineEdit();
    imapServerEdit_->setPlaceholderText("imap.gmail.com");
    imapPortSpin_ = new QSpinBox();
    imapPortSpin_->setRange(1, 65535);
    imapPortSpin_->setValue(993);

    sslCheck_ = new QCheckBox("使用 SSL/TLS 加密");
    sslCheck_->setChecked(true);

    protocolCombo_ = new QComboBox();
    protocolCombo_->addItem("IMAP", QVariant::fromValue(0));
    protocolCombo_->addItem("POP3", QVariant::fromValue(1));

    syncIntervalSpin_ = new QSpinBox();
    syncIntervalSpin_->setRange(30, 3600);
    syncIntervalSpin_->setValue(300);
    syncIntervalSpin_->setSuffix(" 秒");

    form->addRow("显示名称:", displayNameEdit_);
    form->addRow("邮箱地址:", emailEdit_);
    form->addRow("密码/授权码:", passwordEdit_);
    form->addRow("SMTP 服务器:", smtpServerEdit_);
    form->addRow("SMTP 端口:", smtpPortSpin_);
    form->addRow("IMAP 服务器:", imapServerEdit_);
    form->addRow("IMAP 端口:", imapPortSpin_);
    form->addRow(sslCheck_);
    form->addRow("接收协议:", protocolCombo_);
    form->addRow("同步间隔:", syncIntervalSpin_);

    layout->addLayout(form);

    auto* btnLayout = new QHBoxLayout();
    auto* testBtn = new QPushButton("测试连接");
    auto* saveBtn = new QPushButton("保存");
    auto* cancelBtn = new QPushButton("取消");
    btnLayout->addWidget(testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(testBtn, &QPushButton::clicked, this, &AccountDialog::onTestConnection);
    connect(saveBtn, &QPushButton::clicked, this, &AccountDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AccountDialog::setServiceClient(ServiceClient* client) {
    client_ = client;
}

void AccountDialog::setEditMode(const std::string& existingAccountId) {
    editMode_ = true;
    editAccountId_ = existingAccountId;
    setWindowTitle("编辑账号");
    passwordEdit_->setPlaceholderText("留空则不修改密码");
}

void AccountDialog::setAccount(const AccountConfig& account) {
    displayNameEdit_->setText(QString::fromStdString(account.displayName));
    emailEdit_->setText(QString::fromStdString(account.email));
    smtpServerEdit_->setText(QString::fromStdString(account.smtpServer));
    smtpPortSpin_->setValue(account.smtpPort);
    imapServerEdit_->setText(QString::fromStdString(account.imapServer));
    imapPortSpin_->setValue(account.imapPort);
    sslCheck_->setChecked(account.smtpUseSSL);
    syncIntervalSpin_->setValue(account.syncInterval);

    int idx = (account.preferredProtocol == AccountConfig::Protocol::POP3) ? 1 : 0;
    protocolCombo_->setCurrentIndex(idx);
}

AccountConfig AccountDialog::getAccount() const {
    AccountConfig account;
    account.displayName = displayNameEdit_->text().toStdString();
    account.email = emailEdit_->text().toStdString();
    account.encryptedPassword = passwordEdit_->text().toStdString();
    account.smtpServer = smtpServerEdit_->text().toStdString();
    account.smtpPort = smtpPortSpin_->value();
    account.imapServer = imapServerEdit_->text().toStdString();
    account.imapPort = imapPortSpin_->value();
    account.smtpUseSSL = sslCheck_->isChecked();
    account.imapUseSSL = sslCheck_->isChecked();
    account.syncInterval = syncIntervalSpin_->value();

    int protocolValue = protocolCombo_->currentData().toInt();
    account.preferredProtocol = (protocolValue == 1)
        ? AccountConfig::Protocol::POP3
        : AccountConfig::Protocol::IMAP;
    return account;
}

void AccountDialog::onTestConnection() {
    if (!client_) {
        QMessageBox::warning(this, "错误", "服务未连接");
        return;
    }

    AccountConfig acc = getAccount();
    if (acc.email.empty()) {
        QMessageBox::warning(this, "信息不完整", "请先填写邮箱地址。");
        return;
    }
    if (acc.smtpServer.empty()) {
        QMessageBox::warning(this, "信息不完整", "请先填写SMTP服务器地址。");
        return;
    }

    auto* testBtn = qobject_cast<QPushButton*>(sender());
    if (testBtn) testBtn->setEnabled(false);

    client_->testConnection(acc, [this, testBtn](bool success, const QString& message) {
        if (testBtn) testBtn->setEnabled(true);
        if (success) {
            QMessageBox::information(this, "连接成功", message);
        } else {
            QMessageBox::warning(this, "连接失败", message);
        }
    });
}

void AccountDialog::onSave() {
    if (emailEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "信息不完整", "请填写邮箱地址。");
        return;
    }
    if (passwordEdit_->text().isEmpty() && !editMode_) {
        QMessageBox::warning(this, "信息不完整", "请填写密码/授权码。");
        return;
    }
    if (smtpServerEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "信息不完整", "请填写SMTP服务器地址。");
        return;
    }
    accept();
}

} // namespace SmartMail
