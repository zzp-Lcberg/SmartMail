#include "EmailDetailView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace SmartMail {

EmailDetailView::EmailDetailView(QWidget* parent) : QWidget(parent) {
    setStyleSheet("QWidget#emailDetailView { background-color: #faf8f5; }");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    // 邮件头信息
    subjectLabel_ = new QLabel("选择一封邮件");
    subjectLabel_->setObjectName("subjectLabel");
    subjectLabel_->setWordWrap(true);
    QFont subjectFont = subjectLabel_->font();
    subjectFont.setPointSize(14);
    subjectFont.setBold(true);
    subjectLabel_->setFont(subjectFont);

    auto* headerLayout = new QHBoxLayout();
    senderLabel_ = new QLabel();
    senderLabel_->setObjectName("senderLabel");
    timeLabel_ = new QLabel();
    timeLabel_->setObjectName("timeLabel");
    tagLabel_ = new QLabel();
    tagLabel_->setObjectName("tagLabel");
    tagLabel_->hide();
    headerLayout->addWidget(senderLabel_);
    headerLayout->addStretch();
    headerLayout->addWidget(tagLabel_);
    headerLayout->addWidget(timeLabel_);

    layout->addWidget(subjectLabel_);
    layout->addLayout(headerLayout);

    // 分隔线
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("QFrame { color: #e0dbd0; }");
    layout->addWidget(line);

    // 邮件正文
    bodyView_ = new QTextBrowser();
    bodyView_->setOpenExternalLinks(true);
    bodyView_->setStyleSheet(
        "QTextBrowser {"
        "  background-color: #faf8f5;"
        "  color: #2c3038;"
        "  font-size: 14px;"
        "  border: 1px solid #e0dbd0;"
        "  border-radius: 4px;"
        "  padding: 12px;"
        "  selection-background-color: #c5d0f8;"
        "}"
    );
    layout->addWidget(bodyView_);
}

void EmailDetailView::showEmail(const QJsonObject& json) {
    subjectLabel_->setText(json["subject"].toString());
    senderLabel_->setText("发件人: " + json["sender"].toString());

    // 时间戳转换
    qint64 receivedAt = static_cast<qint64>(json["receivedAt"].toDouble());
    if (receivedAt > 0) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(receivedAt);
        timeLabel_->setText(dt.toString("yyyy-MM-dd hh:mm"));
    } else {
        timeLabel_->setText("");
    }

    // AI 标签
    QString tag = json["aiTag"].toString();
    if (!tag.isEmpty()) {
        tagLabel_->setText(tag);
        tagLabel_->show();
    } else {
        tagLabel_->hide();
    }

    // 邮件正文
    QString body = json["bodyHtml"].toString();
    if (body.isEmpty()) {
        body = json["bodyPlain"].toString();
    }
    if (!body.isEmpty()) {
        bodyView_->setHtml(body);
    } else {
        bodyView_->setPlainText(json["bodyPlain"].toString());
    }
}

void EmailDetailView::clear() {
    subjectLabel_->setText("选择一封邮件");
    senderLabel_->clear();
    timeLabel_->clear();
    tagLabel_->hide();
    bodyView_->clear();
}

} // namespace SmartMail
