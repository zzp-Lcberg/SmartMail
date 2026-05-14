#include "FolderModel.hpp"
#include <QIcon>

namespace SmartMail {

FolderModel::FolderModel(QObject* parent) : QAbstractListModel(parent) {}

int FolderModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(folders_.size());
}

QVariant FolderModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(folders_.size())) return {};

    const auto& folder = folders_[row];
    switch (role) {
        case NameRole:       return QString::fromStdString(folder.name);
        case PathRole:       return QString::fromStdString(folder.path);
        case UnreadCountRole: return folder.unreadCount;
        case TotalCountRole:  return folder.totalCount;
        case Qt::DisplayRole: return QString::fromStdString(folder.name);
        default: return {};
    }
}

QHash<int, QByteArray> FolderModel::roleNames() const {
    return {
        {NameRole, "folderName"},
        {PathRole, "folderPath"},
        {UnreadCountRole, "unreadCount"},
        {TotalCountRole, "totalCount"}
    };
}

void FolderModel::setFolders(const std::vector<Folder>& folders) {
    beginResetModel();
    folders_ = folders;
    endResetModel();
}

void FolderModel::updateUnreadCount(const std::string& folderPath, int count) {
    for (size_t i = 0; i < folders_.size(); ++i) {
        if (folders_[i].path == folderPath) {
            folders_[i].unreadCount = count;
            QModelIndex idx = index(static_cast<int>(i));
            emit dataChanged(idx, idx, {UnreadCountRole});
            return;
        }
    }
}

} // namespace SmartMail
