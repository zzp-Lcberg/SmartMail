#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "types/Email.hpp"
#include "types/AccountConfig.hpp"
#include "types/AiResult.hpp"

namespace SmartMail {

/// SQLite 数据库管理
class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool open(const std::string& dbPath);
    void close();
    bool isOpen() const;

    // 邮件操作
    bool saveEmail(const Email& email);
    bool saveEmails(const std::vector<Email>& emails);
    std::vector<Email> getEmails(const std::string& accountId,
                                  const std::string& folder = "INBOX",
                                  int limit = 50, int offset = 0);
    std::unique_ptr<Email> getEmailById(const std::string& id);
    bool markAsRead(const std::string& id);
    bool deleteEmail(const std::string& id);
    int getUnreadCount(const std::string& accountId);

    // AI 标签操作
    bool saveAiTag(const std::string& emailId, const std::string& tag,
                    bool isManual = false);
    std::string getAiTag(const std::string& emailId);
    bool updateAiTag(const std::string& emailId, const std::string& newTag);

    // 设置操作
    std::string getSetting(const std::string& key, const std::string& defaultValue = "");
    bool setSetting(const std::string& key, const std::string& value);

    // 搜索
    std::vector<Email> search(const std::string& query,
                               const std::string& tag = "",
                               int64_t fromDate = 0, int64_t toDate = 0,
                               int limit = 50);

private:
    bool initSchema();
    bool createFtsIndex();

    sqlite3* db_ = nullptr;
    std::string dbPath_;
};

} // namespace SmartMail
