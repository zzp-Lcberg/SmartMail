#pragma once

#include <QListView>
#include <QStyledItemDelegate>

namespace SmartMail {

class EmailListView : public QListView {
    Q_OBJECT
public:
    explicit EmailListView(QWidget* parent = nullptr);

signals:
    void emailClicked(const QString& emailId);

protected:
    void currentChanged(const QModelIndex& current,
                        const QModelIndex& previous) override;
};

/// 邮件列表项代理渲染
class EmailItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

} // namespace SmartMail
