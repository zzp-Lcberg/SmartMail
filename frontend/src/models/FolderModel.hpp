#pragma once

#include <QAbstractListModel>
#include <vector>
#include "types/Email.hpp"

namespace SmartMail {

/// 文件夹树数据模型
class FolderModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        UnreadCountRole,
        TotalCountRole
    };

    explicit FolderModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFolders(const std::vector<Folder>& folders);
    void updateUnreadCount(const std::string& folderPath, int count);

private:
    std::vector<Folder> folders_;
};

} // namespace SmartMail
