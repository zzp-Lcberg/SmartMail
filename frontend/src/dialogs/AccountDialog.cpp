#include "AccountDialog.hpp"
#include "client/ServiceClient.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QGroupBox>
#include <QPalette>
#include <QAbstractItemView>

namespace SmartMail {

// 常见邮箱服务商预设
struct Preset {
    const char* name;
    const char* domain;
    const char* smtp;
    int smtpPort;
    const char* imap;
    int imapPort;
};

static const Preset PRESETS[] = {
    {"Gmail",       "gmail.com",   "smtp.gmail.com",       465, "imap.gmail.com",       993},
    {"QQ邮箱",      "qq.com",      "smtp.qq.com",          465, "imap.qq.com",          993},
    {"163邮箱",     "163.com",     "smtp.163.com",         465, "imap.163.com",         993},
    {"Outlook",     "outlook.com", "smtp.office365.com",   587, "outlook.office365.com",993},
    {"126邮箱",     "126.com",     "smtp.126.com",         465, "imap.126.com",         993},
    {"Yahoo",       "yahoo.com",   "smtp.mail.yahoo.com",  465, "imap.mail.yahoo.com",  993},
    {"自定义",      "",            "",                     0,   "",                      0},
};
static const int PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);

AccountDialog::AccountDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("添加账号");
    resize(520, 560);
    setMinimumSize(480, 500);

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
        "AccountDialog, AccountDialog QWidget {"
        "  color: #e1e5ee;"
        "  background-color: #1a1d23;"
        "}"
        "AccountDialog QGroupBox {"
        "  background-color: #21252b;"
        "  border: 1px solid #333842;"
        "  border-radius: 4px;"
        "  margin-top: 14px;"
        "  padding: 18px 12px 12px 12px;"
        "  font-weight: bold;"
        "  color: #e1e5ee;"
        "}"
        "AccountDialog QGroupBox::title {"
        "  background-color: #21252b;"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 6px;"
        "  color: #5c7cfa;"
        "}"
        "AccountDialog QLabel {"
        "  background-color: #21252b;"
        "  color: #e1e5ee;"
        "  font-size: 12px;"
        "}"
        "AccountDialog QLineEdit {"
        "  background-color: #1a1d23;"
        "  border: 1px solid #333842;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "  font-size: 12px;"
        "}"
        "AccountDialog QLineEdit:focus { border: 1px solid #5c7cfa; }"
        "AccountDialog QComboBox {"
        "  background-color: #1a1d23;"
        "  color: #e1e5ee;"
        "  border: 1px solid #5c7cfa;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  min-height: 28px;"
        "}"
        "AccountDialog QSpinBox {"
        "  background-color: #1a1d23;"
        "  color: #e1e5ee;"
        "  border: 1px solid #333842;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "}"
        "AccountDialog QCheckBox {"
        "  background-color: #21252b;"
        "  color: #e1e5ee;"
        "}"
        "AccountDialog QPushButton {"
        "  background-color: #333842;"
        "  color: #e1e5ee;"
        "  border: 1px solid #3a3f4b;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 12px;"
        "}"
        "AccountDialog QPushButton:hover { background-color: #3a404e; }"
        "AccountDialog QPushButton#testBtn {"
        "  background-color: #5c7cfa;"
        "  color: #ffffff;"
        "  border: none;"
        "  font-weight: bold;"
        "}"
        "AccountDialog QPushButton#testBtn:hover { background-color: #6b8afb; }"
        "AccountDialog QPushButton#saveBtn {"
        "  background-color: #5c7cfa;"
        "  color: #ffffff;"
        "  border: none;"
        "  font-weight: bold;"
        "}"
        "AccountDialog QPushButton#saveBtn:hover { background-color: #6b8afb; }"
    );

    // QSS 无法设置 ::placeholder 颜色，用 QPalette 统一控制文字色和占位符色
    auto setLineEditPalette = [](QLineEdit* edit) {
        QPalette p = edit->palette();
        p.setColor(QPalette::Text, QColor("#e1e5ee"));
        p.setColor(QPalette::PlaceholderText, QColor("#555b68"));
        p.setColor(QPalette::Base, QColor("#1a1d23"));
        edit->setPalette(p);
    };
    // 手动创建表单标签，确保文字颜色可靠渲染
    auto makeLabel = [](const QString& text) {
        auto* label = new QLabel(text);
        label->setStyleSheet("color: #e1e5ee; font-size: 12px; background-color: #21252b;");
        return label;
    };

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    // ====== 第1步：选择邮箱类型 ======
    auto* step1Group = new QGroupBox("第1步：选择邮箱类型");
    auto* step1Layout = new QVBoxLayout(step1Group);

    auto* step1Hint = new QLabel("选择你的邮箱服务商，服务器地址将自动填写");
    step1Hint->setStyleSheet("color: #6a7180; font-size: 11px; margin-bottom: 4px; background-color: #21252b;");
    step1Layout->addWidget(step1Hint);

    auto* providerRow = new QHBoxLayout();
    providerCombo_ = new QComboBox();
    providerCombo_->setMinimumHeight(32);
    for (int i = 0; i < PRESET_COUNT; ++i) {
        providerCombo_->addItem(PRESETS[i].name);
    }
    providerCombo_->setCurrentIndex(PRESET_COUNT - 1);
    providerCombo_->setToolTip("选择邮箱类型后将自动填充下方的服务器地址和端口");
    providerCombo_->view()->setStyleSheet(
        "background-color: #21252b; color: #e1e5ee;"
        "selection-background-color: #2a3050; selection-color: #ffffff;"
        "border: 1px solid #5c7cfa; outline: none; font-size: 13px; padding: 4px;"
    );

    providerRow->addWidget(providerCombo_, 1);
    step1Layout->addLayout(providerRow);

    layout->addWidget(step1Group);

    // ====== 第2步：填写账号信息 ======
    auto* step2Group = new QGroupBox("第2步：填写账号信息");
    auto* step2Form = new QFormLayout(step2Group);
    step2Form->setSpacing(8);

    displayNameEdit_ = new QLineEdit();
    displayNameEdit_->setPlaceholderText("如：张三 或 Zhang San（收件人看到的名字）");
    setLineEditPalette(displayNameEdit_);

    emailEdit_ = new QLineEdit();
    emailEdit_->setPlaceholderText("如：zhangsan@gmail.com");
    setLineEditPalette(emailEdit_);

    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("邮箱服务商生成的授权码，不是登录密码！");
    setLineEditPalette(passwordEdit_);

    step2Form->addRow(makeLabel("显示名称："), displayNameEdit_);
    step2Form->addRow(makeLabel("邮箱地址："), emailEdit_);
    step2Form->addRow(makeLabel("授权码："), passwordEdit_);

    auto* pwdHint = new QLabel("如何获取授权码：Gmail→App Passwords ｜ QQ邮箱→设置→账户→生成授权码 ｜ 163→设置→新增授权码");
    pwdHint->setStyleSheet("color: #f0a050; font-size: 10px; padding-left: 4px; background-color: #21252b;");
    pwdHint->setWordWrap(true);
    step2Form->addRow("", pwdHint);

    layout->addWidget(step2Group);

    // ====== 第3步：服务器设置 ======
    auto* step3Group = new QGroupBox("第3步：服务器设置");
    auto* step3Form = new QFormLayout(step3Group);
    step3Form->setSpacing(8);

    smtpServerEdit_ = new QLineEdit();
    smtpServerEdit_->setPlaceholderText("SMTP 发件服务器地址");
    setLineEditPalette(smtpServerEdit_);
    smtpPortSpin_ = new QSpinBox();
    smtpPortSpin_->setRange(1, 65535);
    smtpPortSpin_->setValue(465);
    smtpPortSpin_->setToolTip("465=SSL加密  587=STARTTLS");

    imapServerEdit_ = new QLineEdit();
    imapServerEdit_->setPlaceholderText("IMAP 收件服务器地址");
    setLineEditPalette(imapServerEdit_);
    imapPortSpin_ = new QSpinBox();
    imapPortSpin_->setRange(1, 65535);
    imapPortSpin_->setValue(993);

    step3Form->addRow(makeLabel("SMTP 服务器："), smtpServerEdit_);
    step3Form->addRow(makeLabel("SMTP 端口："), smtpPortSpin_);
    step3Form->addRow(makeLabel("IMAP 服务器："), imapServerEdit_);
    step3Form->addRow(makeLabel("IMAP 端口："), imapPortSpin_);

    layout->addWidget(step3Group);

    // ====== 高级选项 ======
    auto* advGroup = new QGroupBox("高级选项");
    auto* advForm = new QFormLayout(advGroup);
    advForm->setSpacing(8);

    sslCheck_ = new QCheckBox("使用 SSL/TLS 加密传输（强烈建议开启）");
    sslCheck_->setChecked(true);

    protocolCombo_ = new QComboBox();
    protocolCombo_->addItem("IMAP（推荐 — 邮件保留在服务器，多设备同步）", QVariant::fromValue(0));
    protocolCombo_->addItem("POP3（邮件下载到本地，服务器删除）", QVariant::fromValue(1));
    protocolCombo_->view()->setStyleSheet(
        "background-color: #21252b; color: #e1e5ee;"
        "selection-background-color: #2a3050; selection-color: #ffffff;"
        "border: 1px solid #5c7cfa; outline: none; font-size: 13px; padding: 4px;"
    );

    syncIntervalSpin_ = new QSpinBox();
    syncIntervalSpin_->setRange(30, 3600);
    syncIntervalSpin_->setValue(300);
    syncIntervalSpin_->setSuffix(" 秒");
    syncIntervalSpin_->setToolTip("每隔多少秒自动检查新邮件");

    advForm->addRow(sslCheck_);
    advForm->addRow(makeLabel("接收协议："), protocolCombo_);
    advForm->addRow(makeLabel("同步间隔："), syncIntervalSpin_);

    layout->addWidget(advGroup);

    // ====== 底部按钮 ======
    auto* btnLayout = new QHBoxLayout();
    auto* testBtn = new QPushButton("测试连接");
    testBtn->setObjectName("testBtn");
    testBtn->setMinimumHeight(36);
    testBtn->setMinimumWidth(100);

    auto* saveBtn = new QPushButton("保存");
    saveBtn->setObjectName("saveBtn");
    saveBtn->setMinimumHeight(36);
    saveBtn->setMinimumWidth(80);

    auto* cancelBtn = new QPushButton("取消");
    cancelBtn->setMinimumHeight(36);
    cancelBtn->setMinimumWidth(80);

    btnLayout->addWidget(testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    // ====== 信号连接 ======
    connect(providerCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AccountDialog::onProviderChanged);
    connect(emailEdit_, &QLineEdit::textChanged, this, &AccountDialog::onEmailTextChanged);
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

void AccountDialog::onProviderChanged(int index) {
    if (index < 0 || index >= PRESET_COUNT) return;

    const auto& p = PRESETS[index];
    if (index == PRESET_COUNT - 1) return;

    smtpServerEdit_->setText(p.smtp);
    smtpPortSpin_->setValue(p.smtpPort);
    imapServerEdit_->setText(p.imap);
    imapPortSpin_->setValue(p.imapPort);
    sslCheck_->setChecked(true);
}

void AccountDialog::onEmailTextChanged(const QString& text) {
    QString email = text.trimmed().toLower();
    int atPos = email.indexOf('@');
    if (atPos < 0) return;

    QString domain = email.mid(atPos + 1);

    for (int i = 0; i < PRESET_COUNT - 1; ++i) {
        if (domain == PRESETS[i].domain) {
            if (providerCombo_->currentIndex() != i) {
                providerCombo_->setCurrentIndex(i);
            }
            return;
        }
    }
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
        QMessageBox::warning(this, "信息不完整", "请选择邮箱类型或手动填写SMTP服务器地址。");
        return;
    }

    auto* testBtn = qobject_cast<QPushButton*>(sender());
    if (testBtn) testBtn->setEnabled(false);

    client_->testConnection(acc, [this, testBtn](bool success, const QString& message) {
        if (testBtn) testBtn->setEnabled(true);
        if (success) {
            QMessageBox::information(this, "连接成功", "SMTP服务器连接正常！\n" + message);
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
        QMessageBox::warning(this, "信息不完整",
            "请填写授权码。\n\n这不是你的邮箱登录密码！\n"
            "• Gmail：设置 → 账户 → App Passwords\n"
            "• QQ邮箱：设置 → 账户 → POP3/IMAP → 生成授权码\n"
            "• 163邮箱：设置 → POP3/SMTP/IMAP → 新增授权码");
        return;
    }
    if (smtpServerEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "信息不完整", "请选择邮箱类型自动填充服务器地址，或手动输入。");
        return;
    }
    accept();
}

} // namespace SmartMail
