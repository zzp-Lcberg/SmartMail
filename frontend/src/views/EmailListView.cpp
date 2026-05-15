#include "EmailListView.hpp"
#include "models/MailModel.hpp"
#include <QPainter>

namespace SmartMail {

EmailListView::EmailListView(QWidget* parent) : QListView(parent) {
    setItemDelegate(new EmailItemDelegate(this));
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setUniformItemSizes(false);
    setMouseTracking(true);
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
    painter->setRenderHint(QPainter::Antialiasing);

    QRect r = option.rect;
    bool isRead = index.data(MailModel::IsReadRole).toBool();
    bool isSelected = option.state & QStyle::State_Selected;

    // 背景
    if (isSelected) {
        // 选中态: 靛蓝底色 + 左侧高亮条
        painter->fillRect(r, QColor(42, 48, 80));
        painter->fillRect(QRect(r.left(), r.top(), 3, r.height()), QColor(92, 124, 250));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(r, QColor(40, 44, 52));
    }

    // 底部细线
    painter->setPen(QColor(42, 46, 55));
    painter->drawLine(r.bottomLeft(), r.bottomRight());

    // 未读指示圆点
    QRect contentRect = r.adjusted(14, 10, -12, -10);
    if (!isRead) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(92, 124, 250));
        painter->drawEllipse(contentRect.left() - 10, contentRect.top() + 6, 6, 6);
    }

    // 发件人
    QFont senderFont = painter->font();
    senderFont.setPixelSize(13);
    if (!isRead) {
        senderFont.setBold(true);
    }
    painter->setFont(senderFont);
    painter->setPen(isSelected ? QColor(225, 229, 238) : QColor(220, 224, 233));
    QRect senderRect(contentRect.left(), contentRect.top(), contentRect.width() - 110, 18);
    QString sender = painter->fontMetrics().elidedText(
        index.data(MailModel::SenderRole).toString(), Qt::ElideRight, senderRect.width());
    painter->drawText(senderRect, Qt::AlignLeft | Qt::AlignVCenter, sender);

    // 时间
    QFont timeFont = painter->font();
    timeFont.setPixelSize(11);
    timeFont.setBold(false);
    painter->setFont(timeFont);
    painter->setPen(isSelected ? QColor(150, 160, 180) : QColor(110, 118, 130));
    QDateTime time = index.data(MailModel::TimeRole).toDateTime();
    QString timeStr = time.toString("MM-dd");
    if (time.date() == QDate::currentDate()) {
        timeStr = time.toString("HH:mm");
    }
    QRect timeRect(contentRect.right() - 100, contentRect.top(), 100, 18);
    painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, timeStr);

    // 主题
    QFont subjectFont = painter->font();
    subjectFont.setPixelSize(13);
    subjectFont.setBold(false);
    painter->setFont(subjectFont);
    painter->setPen(isSelected ? QColor(180, 190, 205) : QColor(150, 158, 170));
    QString subject = painter->fontMetrics().elidedText(
        index.data(MailModel::SubjectRole).toString(), Qt::ElideRight,
        contentRect.width() - 60);
    QRect subjectRect(contentRect.left(), contentRect.top() + 22,
                      contentRect.width() - 60, 18);
    painter->drawText(subjectRect, Qt::AlignLeft | Qt::AlignVCenter, subject);

    // AI 标签
    QString aiTag = index.data(MailModel::AiTagRole).toString();
    if (!aiTag.isEmpty()) {
        QFontMetrics fm(subjectFont);
        int tagWidth = fm.horizontalAdvance(aiTag) + 16;

        QRect tagRect(contentRect.right() - tagWidth, contentRect.top() + 22,
                      tagWidth, 16);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(245, 230, 200));
        painter->drawRoundedRect(tagRect, 8, 8);

        QFont tagFont = painter->font();
        tagFont.setPixelSize(10);
        tagFont.setBold(true);
        painter->setFont(tagFont);
        painter->setPen(QColor(122, 94, 30));
        painter->drawText(tagRect, Qt::AlignCenter, aiTag);
    }

    painter->restore();
}

QSize EmailItemDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                   const QModelIndex& /*index*/) const {
    return {200, 56};
}

} // namespace SmartMail
