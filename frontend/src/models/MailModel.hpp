#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QDateTime>
#include <vector>
#include "types/Email.hpp"

namespace SmartMail {

/// 邮件列表数据模型 (Qt Model/View)
class MailModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SenderRole,
        SubjectRole,
        TimeRole,
        IsReadRole,
        IsStarredRole,
        AiTagRole,
        FolderRole
    };

    explicit MailModel(QObject* parent = nullptr);

    // QAbstractListModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 数据操作
    void setEmails(const std::vector<Email>& emails);
    void appendEmails(const std::vector<Email>& emails);
    void updateEmail(const std::string& id, const Email& updated);
    void removeEmail(const std::string& id);
    void clear();

    Email getEmail(int row) const;

private:
    std::vector<Email> emails_;
};

} // namespace SmartMail
