#include "MailModel.hpp"
#include <QDateTime>

namespace SmartMail {

MailModel::MailModel(QObject* parent) : QAbstractListModel(parent) {}

int MailModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(emails_.size());
}

QVariant MailModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(emails_.size())) return {};

    const auto& email = emails_[row];
    switch (role) {
        case IdRole:       return QString::fromStdString(email.id);
        case SenderRole:   return QString::fromStdString(email.sender);
        case SubjectRole:  return QString::fromStdString(email.subject);
        case TimeRole:     return QDateTime::fromSecsSinceEpoch(email.receivedAt);
        case IsReadRole:   return email.isRead;
        case IsStarredRole:return email.isStarred;
        case AiTagRole:    return email.aiTag.has_value()
                                 ? QString::fromStdString(email.aiTag.value())
                                 : QString();
        case FolderRole:   return QString::fromStdString(email.folder);
        case Qt::DisplayRole: return QString::fromStdString(email.subject);
        default: return {};
    }
}

QHash<int, QByteArray> MailModel::roleNames() const {
    return {
        {IdRole, "emailId"},
        {SenderRole, "sender"},
        {SubjectRole, "subject"},
        {TimeRole, "time"},
        {IsReadRole, "isRead"},
        {IsStarredRole, "isStarred"},
        {AiTagRole, "aiTag"},
        {FolderRole, "folder"}
    };
}

void MailModel::setEmails(const std::vector<Email>& emails) {
    beginResetModel();
    emails_ = emails;
    endResetModel();
}

void MailModel::appendEmails(const std::vector<Email>& emails) {
    if (emails.empty()) return;
    beginInsertRows(QModelIndex(), static_cast<int>(emails_.size()),
                    static_cast<int>(emails_.size() + emails.size() - 1));
    emails_.insert(emails_.end(), emails.begin(), emails.end());
    endInsertRows();
}

void MailModel::updateEmail(const std::string& id, const Email& updated) {
    for (size_t i = 0; i < emails_.size(); ++i) {
        if (emails_[i].id == id) {
            emails_[i] = updated;
            QModelIndex idx = index(static_cast<int>(i));
            emit dataChanged(idx, idx);
            return;
        }
    }
}

void MailModel::removeEmail(const std::string& id) {
    for (size_t i = 0; i < emails_.size(); ++i) {
        if (emails_[i].id == id) {
            beginRemoveRows(QModelIndex(), static_cast<int>(i), static_cast<int>(i));
            emails_.erase(emails_.begin() + i);
            endRemoveRows();
            return;
        }
    }
}

void MailModel::clear() {
    beginResetModel();
    emails_.clear();
    endResetModel();
}

Email MailModel::getEmail(int row) const {
    if (row >= 0 && row < static_cast<int>(emails_.size())) {
        return emails_[row];
    }
    return {};
}

} // namespace SmartMail
