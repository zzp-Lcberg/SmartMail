#include "AiPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace SmartMail {

AiPanel::AiPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    titleLabel_ = new QLabel("AI 助手");
    titleLabel_->setObjectName("aiTitleLabel");
    QFont titleFont = titleLabel_->font();
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);

    tagLabel_ = new QLabel();
    tagLabel_->setObjectName("aiTagLabel");
    tagLabel_->setAlignment(Qt::AlignCenter);
    tagLabel_->hide();

    replyEdit_ = new QTextEdit();
    replyEdit_->setPlaceholderText("AI 生成的回复草稿将显示在这里...");
    replyEdit_->setMinimumHeight(120);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    acceptBtn_ = new QPushButton("采纳");
    regenerateBtn_ = new QPushButton("重新生成");
    regenerateBtn_->setObjectName("secondaryBtn");
    editBtn_ = new QPushButton("编辑");
    editBtn_->setObjectName("secondaryBtn");
    btnLayout->addWidget(acceptBtn_);
    btnLayout->addWidget(regenerateBtn_);
    btnLayout->addWidget(editBtn_);

    layout->addWidget(titleLabel_);
    layout->addWidget(tagLabel_);
    layout->addWidget(replyEdit_);
    layout->addLayout(btnLayout);

    connect(acceptBtn_, &QPushButton::clicked, this, &AiPanel::onAccept);
    connect(regenerateBtn_, &QPushButton::clicked, this, &AiPanel::onRegenerate);
    connect(editBtn_, &QPushButton::clicked, this, &AiPanel::onEdit);

    setEnabled(false);
}

void AiPanel::showClassification(const QString& tag) {
    tagLabel_->setText(tag);
    tagLabel_->show();
    titleLabel_->setText("AI 分类结果");
    replyEdit_->clear();
}

void AiPanel::showReplyDraft(const QString& replyContent) {
    titleLabel_->setText("AI 回复建议");
    replyEdit_->setPlainText(replyContent);
    setEnabled(true);
}

void AiPanel::setLoading(bool loading) {
    if (loading) {
        titleLabel_->setText("AI 思考中...");
    }
    setEnabled(!loading);
}

void AiPanel::onAccept() {
    emit acceptReply(replyEdit_->toPlainText());
}

void AiPanel::onRegenerate() {
    emit regenerateReply();
}

void AiPanel::onEdit() {
    emit editReply(replyEdit_->toPlainText());
}

} // namespace SmartMail
