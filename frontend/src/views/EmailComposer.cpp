#include "EmailComposer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

namespace SmartMail {

EmailComposer::EmailComposer(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("撰写邮件");
    resize(700, 550);

    auto* layout = new QVBoxLayout(this);

    // 收件人/抄送
    auto* formLayout = new QFormLayout();
    toEdit_ = new QLineEdit();
    toEdit_->setPlaceholderText("收件人邮箱");
    ccEdit_ = new QLineEdit();
    ccEdit_->setPlaceholderText("抄送 (可选)");
    subjectEdit_ = new QLineEdit();
    subjectEdit_->setPlaceholderText("邮件主题");
    formLayout->addRow("收件人:", toEdit_);
    formLayout->addRow("抄送:", ccEdit_);
    formLayout->addRow("主题:", subjectEdit_);
    layout->addLayout(formLayout);

    // 正文
    bodyEdit_ = new QTextEdit();
    bodyEdit_->setPlaceholderText("邮件正文...");
    layout->addWidget(bodyEdit_);

    // 按钮
    auto* btnLayout = new QHBoxLayout();
    sendBtn_ = new QPushButton("发送");
    draftBtn_ = new QPushButton("存草稿");
    cancelBtn_ = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(draftBtn_);
    btnLayout->addWidget(sendBtn_);
    btnLayout->addWidget(cancelBtn_);
    layout->addLayout(btnLayout);

    connect(sendBtn_, &QPushButton::clicked, this, &EmailComposer::onSend);
    connect(draftBtn_, &QPushButton::clicked, this, &EmailComposer::onSaveDraft);
    connect(cancelBtn_, &QPushButton::clicked, this, &EmailComposer::onCancel);
}

void EmailComposer::setReplyMode(const Email& originalEmail) {
    replyMode_ = true;
    originalEmailId_ = originalEmail.id;
    toEdit_->setText(QString::fromStdString(originalEmail.sender));
    subjectEdit_->setText("Re: " + QString::fromStdString(originalEmail.subject));
    setWindowTitle("回复邮件");
}

void EmailComposer::setForwardMode(const Email& originalEmail) {
    replyMode_ = false;
    originalEmailId_ = originalEmail.id;
    toEdit_->clear();
    toEdit_->setPlaceholderText("转发收件人邮箱");
    subjectEdit_->setText("Fwd: " + QString::fromStdString(originalEmail.subject));
    bodyEdit_->setPlainText(QString::fromStdString(
        "\n\n--- 转发邮件 ---\n"
        "发件人: " + originalEmail.sender + "\n"
        "主题: " + originalEmail.subject + "\n"
        "---\n" + originalEmail.bodyPlain));
    setWindowTitle("转发邮件");
}

void EmailComposer::setDraftContent(const std::string& subject,
                                     const std::string& body) {
    subjectEdit_->setText(QString::fromStdString(subject));
    bodyEdit_->setPlainText(QString::fromStdString(body));
}

Email EmailComposer::buildEmail() const {
    Email email;
    email.sender = ""; // 由后端填充
    email.recipients = {toEdit_->text().toStdString()};
    if (!ccEdit_->text().isEmpty()) {
        email.cc = ccEdit_->text().toStdString();
    }
    email.subject = subjectEdit_->text().toStdString();
    email.bodyPlain = bodyEdit_->toPlainText().toStdString();
    return email;
}

void EmailComposer::onSend() {
    if (toEdit_->text().isEmpty() || subjectEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "信息不完整", "请填写收件人和主题。");
        return;
    }
    emit emailReady(buildEmail());
    accept();
}

void EmailComposer::onSaveDraft() {
    // TODO: 保存草稿到后端 (Phase 10)
    Email draft = buildEmail();
    draft.folder = "Drafts";
    emit emailReady(draft);
    accept();
}

void EmailComposer::onCancel() {
    reject();
}

} // namespace SmartMail
