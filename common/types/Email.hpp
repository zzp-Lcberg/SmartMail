#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace SmartMail {

/// 邮件数据结构
struct Email {
    std::string id;              // 邮件唯一标识 (UID)
    std::string accountId;       // 所属账号 ID
    std::string sender;          // 发件人
    std::vector<std::string> recipients; // 收件人列表
    std::string cc;              // 抄送
    std::string bcc;             // 密送
    std::string subject;         // 主题
    std::string bodyPlain;       // 纯文本正文
    std::string bodyHtml;        // HTML 正文
    int64_t receivedAt = 0;      // 接收时间 (Unix timestamp)
    int64_t sentAt = 0;          // 发送时间 (Unix timestamp)
    bool isRead = false;         // 是否已读
    bool isStarred = false;      // 是否星标
    std::string folder;          // 所属文件夹 (INBOX/Sent/Drafts/Trash)
    std::vector<std::string> attachments; // 附件文件名列表
    std::optional<std::string> aiTag;     // AI 分类标签
};

/// 邮件文件夹
struct Folder {
    std::string name;            // 文件夹名称
    std::string path;            // 文件夹路径 (IMAP delimiter)
    int unreadCount = 0;         // 未读数量
    int totalCount = 0;          // 总邮件数
};

} // namespace SmartMail
