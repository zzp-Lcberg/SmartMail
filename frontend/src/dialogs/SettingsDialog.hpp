#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QListWidget>
#include <vector>
#include "types/AccountConfig.hpp"

namespace SmartMail {

class ServiceClient;
class Config;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setServiceClient(ServiceClient* client);
    void setConfig(Config* config);

    void loadAccounts();
    void loadSettings();

private slots:
    void onSave();
    void onAddAccount();
    void onRemoveAccount();
    void onEditAccount();

private:
    QTabWidget* tabs_;

    // 账号管理页
    QListWidget* accountList_;

    // AI 设置页
    QLineEdit* apiKeyEdit_;
    QLineEdit* apiUrlEdit_;
    QCheckBox* autoClassifyCheck_;

    // 通用设置页
    QSpinBox* syncIntervalSpin_;
    QCheckBox* minimizeToTrayCheck_;

    ServiceClient* client_ = nullptr;
    Config* config_ = nullptr;
    std::vector<AccountConfig> accounts_;
};

} // namespace SmartMail
