#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include "types/AccountConfig.hpp"

namespace SmartMail {

/// 单个账号的添加/编辑对话框
class AccountDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountDialog(QWidget* parent = nullptr);

    void setAccount(const AccountConfig& account);
    AccountConfig getAccount() const;

private slots:
    void onTestConnection();
    void onSave();

private:
    QLineEdit* displayNameEdit_;
    QLineEdit* emailEdit_;
    QLineEdit* passwordEdit_;
    QLineEdit* smtpServerEdit_;
    QSpinBox* smtpPortSpin_;
    QLineEdit* imapServerEdit_;
    QSpinBox* imapPortSpin_;
    QCheckBox* sslCheck_;
    QComboBox* protocolCombo_;
    QSpinBox* syncIntervalSpin_;
};

} // namespace SmartMail
