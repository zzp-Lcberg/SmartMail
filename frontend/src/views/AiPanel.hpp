#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

namespace SmartMail {

class AiPanel : public QWidget {
    Q_OBJECT
public:
    explicit AiPanel(QWidget* parent = nullptr);

    void showClassification(const QString& tag);
    void showReplyDraft(const QString& replyContent);
    void setLoading(bool loading);

signals:
    void acceptReply(const QString& content);
    void regenerateReply();
    void editReply(const QString& content);

private slots:
    void onAccept();
    void onRegenerate();
    void onEdit();

private:
    QLabel* titleLabel_;
    QLabel* tagLabel_;
    QTextEdit* replyEdit_;
    QPushButton* acceptBtn_;
    QPushButton* regenerateBtn_;
    QPushButton* editBtn_;
};

} // namespace SmartMail
