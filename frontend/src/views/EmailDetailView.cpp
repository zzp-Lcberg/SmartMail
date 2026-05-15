#include "EmailDetailView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace SmartMail {

EmailDetailView::EmailDetailView(QWidget* parent) : QWidget(parent) {
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
    layout->addWidget(bodyView_);
}

void EmailDetailView::showEmail(const QJsonObject& data) {
    subjectLabel_->setText(data["subject"].toString());
    senderLabel_->setText("发件人: " + data["sender"].toString());
    timeLabel_->setText(data["time"].toString());

    QString tag = data["ai_tag"].toString();
    if (!tag.isEmpty()) {
        tagLabel_->setText(tag);
        tagLabel_->show();
    } else {
        tagLabel_->hide();
    }

    QString body = data["body_html"].toString();
    if (body.isEmpty()) {
        body = data["body_plain"].toString();
    }
    bodyView_->setHtml(body);
}

void EmailDetailView::clear() {
    subjectLabel_->setText("选择一封邮件");
    senderLabel_->clear();
    timeLabel_->clear();
    tagLabel_->hide();
    bodyView_->clear();
}

} // namespace SmartMail
