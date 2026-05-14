#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QListWidget>

namespace SmartMail {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onAddAccount();
    void onRemoveAccount();

private:
    QTabWidget* tabs_;

    // 账号管理页
    QListWidget* accountList_;
    QLineEdit* displayNameEdit_;
    QLineEdit* emailEdit_;
    QLineEdit* passwordEdit_;
    QLineEdit* smtpServerEdit_;
    QSpinBox* smtpPortSpin_;
    QLineEdit* imapServerEdit_;
    QSpinBox* imapPortSpin_;
    QCheckBox* sslCheck_;

    // AI 设置页
    QLineEdit* apiKeyEdit_;
    QLineEdit* apiUrlEdit_;
    QCheckBox* autoClassifyCheck_;

    // 通用设置页
    QSpinBox* syncIntervalSpin_;
    QCheckBox* minimizeToTrayCheck_;
};

} // namespace SmartMail
