#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include "types/AccountConfig.hpp"

namespace SmartMail {

class ServiceClient;

/// 单个账号的添加/编辑对话框
class AccountDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountDialog(QWidget* parent = nullptr);

    void setAccount(const AccountConfig& account);
    AccountConfig getAccount() const;

    void setServiceClient(ServiceClient* client);
    void setEditMode(const std::string& existingAccountId);

private slots:
    void onTestConnection();
    void onSave();
    void onProviderChanged(int index);
    void onEmailTextChanged(const QString& text);

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
    QComboBox* providerCombo_;
    QSpinBox* syncIntervalSpin_;

    ServiceClient* client_ = nullptr;
    bool editMode_ = false;
    std::string editAccountId_;
};

} // namespace SmartMail
