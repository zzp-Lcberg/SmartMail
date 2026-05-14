#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include "types/Email.hpp"

namespace SmartMail {

class EmailComposer : public QDialog {
    Q_OBJECT
public:
    explicit EmailComposer(QWidget* parent = nullptr);

    void setReplyMode(const Email& originalEmail);
    void setDraftContent(const std::string& subject, const std::string& body);
    Email buildEmail() const;

signals:
    void emailReady(const Email& email);

private slots:
    void onSend();
    void onSaveDraft();
    void onCancel();

private:
    QLineEdit* toEdit_;
    QLineEdit* ccEdit_;
    QLineEdit* subjectEdit_;
    QTextEdit* bodyEdit_;
    QPushButton* sendBtn_;
    QPushButton* draftBtn_;
    QPushButton* cancelBtn_;

    bool replyMode_ = false;
    std::string originalEmailId_;
};

} // namespace SmartMail
