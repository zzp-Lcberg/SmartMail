#include "EmailListView.hpp"
#include "models/MailModel.hpp"
#include <QPainter>

namespace SmartMail {

EmailListView::EmailListView(QWidget* parent) : QListView(parent) {
    setItemDelegate(new EmailItemDelegate(this));
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(false);
}

void EmailListView::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous) {
    QListView::currentChanged(current, previous);
    if (current.isValid()) {
        QString emailId = current.data(MailModel::IdRole).toString();
        emit emailClicked(emailId);
    }
}

// --- EmailItemDelegate ---

void EmailItemDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    painter->save();

    // 背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else if (!index.data(MailModel::IsReadRole).toBool()) {
        painter->fillRect(option.rect, QColor(240, 248, 255)); // 未读淡蓝
    }

    QRect r = option.rect.adjusted(8, 4, -8, -4);

    // 发件人 - 加粗
    QFont senderFont = option.font;
    senderFont.setBold(!index.data(MailModel::IsReadRole).toBool());
    painter->setFont(senderFont);
    painter->drawText(r.left(), r.top(), r.width() - 80, 18,
                      Qt::AlignLeft, index.data(MailModel::SenderRole).toString());

    // 时间
    painter->setFont(option.font);
    painter->setPen(Qt::gray);
    QDateTime time = index.data(MailModel::TimeRole).toDateTime();
    painter->drawText(r.right() - 80, r.top(), 80, 18,
                      Qt::AlignRight, time.toString("MM-dd hh:mm"));

    // 主题
    painter->setPen(Qt::black);
    QString subject = index.data(MailModel::SubjectRole).toString();
    painter->drawText(r.left(), r.top() + 20, r.width(), 18,
                      Qt::AlignLeft, subject);

    // AI 标签
    QString aiTag = index.data(MailModel::AiTagRole).toString();
    if (!aiTag.isEmpty()) {
        QRect tagRect(r.right() - 50, r.top() + 20, 50, 16);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(100, 149, 237));
        painter->drawRoundedRect(tagRect, 4, 4);
        painter->setPen(Qt::white);
        QFont tagFont = option.font;
        tagFont.setPointSize(8);
        painter->setFont(tagFont);
        painter->drawText(tagRect, Qt::AlignCenter, aiTag);
    }

    painter->restore();
}

QSize EmailItemDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                   const QModelIndex& /*index*/) const {
    return {200, 48};
}

} // namespace SmartMail
