#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextBrowser>
#include <QJsonObject>

namespace SmartMail {

class EmailDetailView : public QWidget {
    Q_OBJECT
public:
    explicit EmailDetailView(QWidget* parent = nullptr);

    void showEmail(const QJsonObject& emailData);
    void clear();

private:
    QLabel* subjectLabel_;
    QLabel* senderLabel_;
    QLabel* timeLabel_;
    QLabel* tagLabel_;
    QTextBrowser* bodyView_;
};

} // namespace SmartMail
