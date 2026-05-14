#include "StorageManager.hpp"
#include "utils/Logger.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>

namespace SmartMail {

// --- RAII prepared statement wrapper ---
class StmtGuard {
public:
    StmtGuard(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            LOG_ERROR(std::string("Prepare failed: ") + sqlite3_errmsg(db_));
            stmt_ = nullptr;
        }
    }
    ~StmtGuard() { if (stmt_) sqlite3_finalize(stmt_); }
    sqlite3_stmt* get() { return stmt_; }
    operator bool() const { return stmt_ != nullptr; }

    bool bindInt(int idx, int64_t val) {
        return sqlite3_bind_int64(stmt_, idx, val) == SQLITE_OK;
    }
    bool bindText(int idx, const std::string& val) {
        return sqlite3_bind_text(stmt_, idx, val.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
    }
    bool bindNull(int idx) {
        return sqlite3_bind_null(stmt_, idx) == SQLITE_OK;
    }

    int64_t lastInsertId() { return sqlite3_last_insert_rowid(db_); }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

// --- Helpers ---
static std::string recipientsJoin(const std::vector<std::string>& recipients) {
    std::string result;
    for (size_t i = 0; i < recipients.size(); ++i) {
        if (i > 0) result += "; ";
        result += recipients[i];
    }
    return result;
}

static std::vector<std::string> recipientsSplit(const std::string& str) {
    std::vector<std::string> result;
    if (str.empty()) return result;
    std::istringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ';')) {
        // trim
        auto start = item.find_first_not_of(" \t");
        auto end = item.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    return result;
}

static Email emailFromRow(sqlite3_stmt* stmt) {
    Email e;
    e.id          = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    e.accountId   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    e.sender      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    e.recipients  = recipientsSplit(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    e.subject     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    const char* plain = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    if (plain) e.bodyPlain = plain;
    const char* html  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    if (html)  e.bodyHtml  = html;
    e.receivedAt  = sqlite3_column_int64(stmt, 7);
    e.sentAt      = sqlite3_column_int64(stmt, 8);
    e.isRead      = sqlite3_column_int(stmt, 9) != 0;
    e.isStarred   = sqlite3_column_int(stmt, 10) != 0;
    e.folder      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
    return e;
}

// ============================================================================

StorageManager::StorageManager() {
    LOG_DEBUG("StorageManager created");
}

StorageManager::~StorageManager() {
    close();
}

bool StorageManager::open(const std::string& dbPath) {
    dbPath_ = dbPath;
    LOG_INFO("Opening database: " + dbPath);

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        LOG_ERROR(std::string("Cannot open database: ") + sqlite3_errmsg(db_));
        return false;
    }

    char* err = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;",  nullptr, nullptr, &err);
    sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, &err);

    return initSchema();
}

void StorageManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        LOG_DEBUG("Database closed");
    }
}

bool StorageManager::isOpen() const {
    return db_ != nullptr;
}

// --- 邮件 CRUD ---

bool StorageManager::saveEmail(const Email& email) {
    if (!db_) return false;

    const char* sql = R"(
        INSERT OR REPLACE INTO emails
            (id, account_id, sender, recipients, subject, body_plain, body_html,
             received_at, sent_at, is_read, is_starred, folder)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    StmtGuard stmt(db_, sql);
    if (!stmt) return false;

    stmt.bindText(1,  email.id);
    stmt.bindText(2,  email.accountId);
    stmt.bindText(3,  email.sender);
    stmt.bindText(4,  recipientsJoin(email.recipients));
    stmt.bindText(5,  email.subject);
    stmt.bindText(6,  email.bodyPlain);
    stmt.bindText(7,  email.bodyHtml);
    stmt.bindInt(8,   email.receivedAt);
    stmt.bindInt(9,   email.sentAt);
    stmt.bindInt(10,  email.isRead ? 1 : 0);
    stmt.bindInt(11,  email.isStarred ? 1 : 0);
    stmt.bindText(12, email.folder);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        LOG_ERROR(std::string("saveEmail failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool StorageManager::saveEmails(const std::vector<Email>& emails) {
    if (!db_ || emails.empty()) return false;

    // 使用事务批量插入以提升性能
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    bool ok = true;
    for (const auto& email : emails) {
        if (!saveEmail(email)) {
            ok = false;
            break;
        }
    }
    if (ok) {
        sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    return ok;
}

std::vector<Email> StorageManager::getEmails(const std::string& accountId,
                                              const std::string& folder,
                                              int limit, int offset) {
    std::vector<Email> result;
    if (!db_) return result;

    const char* sql = R"(
        SELECT e.id, e.account_id, e.sender, e.recipients, e.subject,
               e.body_plain, e.body_html, e.received_at, e.sent_at,
               e.is_read, e.is_starred, e.folder,
               COALESCE(a.tag, '') as ai_tag
        FROM emails e
        LEFT JOIN ai_tags a ON e.id = a.email_id
        WHERE e.account_id = ? AND e.folder = ?
        ORDER BY e.received_at DESC
        LIMIT ? OFFSET ?;
    )";

    StmtGuard stmt(db_, sql);
    if (!stmt) return result;

    stmt.bindText(1, accountId);
    stmt.bindText(2, folder);
    stmt.bindInt(3, limit);
    stmt.bindInt(4, offset);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        Email e = emailFromRow(stmt.get());
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 12));
        if (tag && strlen(tag) > 0) e.aiTag = tag;
        result.push_back(std::move(e));
    }
    return result;
}

std::unique_ptr<Email> StorageManager::getEmailById(const std::string& id) {
    if (!db_) return nullptr;

    const char* sql = R"(
        SELECT e.id, e.account_id, e.sender, e.recipients, e.subject,
               e.body_plain, e.body_html, e.received_at, e.sent_at,
               e.is_read, e.is_starred, e.folder
        FROM emails e
        WHERE e.id = ?;
    )";

    StmtGuard stmt(db_, sql);
    if (!stmt) return nullptr;

    stmt.bindText(1, id);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        return std::make_unique<Email>(emailFromRow(stmt.get()));
    }
    return nullptr;
}

bool StorageManager::markAsRead(const std::string& id) {
    if (!db_) return false;

    const char* sql = "UPDATE emails SET is_read = 1 WHERE id = ?;";
    StmtGuard stmt(db_, sql);
    if (!stmt) return false;
    stmt.bindText(1, id);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        LOG_ERROR(std::string("markAsRead failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    return sqlite3_changes(db_) > 0;
}

bool StorageManager::deleteEmail(const std::string& id) {
    if (!db_) return false;

    // 软删除：移动到回收站文件夹
    const char* sql = "UPDATE emails SET folder = 'Trash' WHERE id = ?;";
    StmtGuard stmt(db_, sql);
    if (!stmt) return false;
    stmt.bindText(1, id);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        LOG_ERROR(std::string("deleteEmail failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    return sqlite3_changes(db_) > 0;
}

int StorageManager::getUnreadCount(const std::string& accountId) {
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM emails WHERE account_id = ? AND is_read = 0 AND folder = 'INBOX';";
    StmtGuard stmt(db_, sql);
    if (!stmt) return 0;
    stmt.bindText(1, accountId);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        return sqlite3_column_int(stmt.get(), 0);
    }
    return 0;
}

// --- AI 标签操作 ---

bool StorageManager::saveAiTag(const std::string& emailId,
                                const std::string& tag, bool isManual) {
    if (!db_) return false;

    const char* sql = R"(
        INSERT OR REPLACE INTO ai_tags (email_id, tag, is_manual, classified_at)
        VALUES (?, ?, ?, strftime('%s','now'));
    )";

    StmtGuard stmt(db_, sql);
    if (!stmt) return false;

    stmt.bindText(1, emailId);
    stmt.bindText(2, tag);
    stmt.bindInt(3, isManual ? 1 : 0);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        LOG_ERROR(std::string("saveAiTag failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

std::string StorageManager::getAiTag(const std::string& emailId) {
    if (!db_) return "";

    const char* sql = "SELECT tag FROM ai_tags WHERE email_id = ?;";
    StmtGuard stmt(db_, sql);
    if (!stmt) return "";
    stmt.bindText(1, emailId);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        return tag ? tag : "";
    }
    return "";
}

bool StorageManager::updateAiTag(const std::string& emailId,
                                  const std::string& newTag) {
    return saveAiTag(emailId, newTag, true);
}

// --- 设置操作 ---

std::string StorageManager::getSetting(const std::string& key,
                                        const std::string& defaultValue) {
    if (!db_) return defaultValue;

    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    StmtGuard stmt(db_, sql);
    if (!stmt) return defaultValue;
    stmt.bindText(1, key);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        return val ? val : defaultValue;
    }
    return defaultValue;
}

bool StorageManager::setSetting(const std::string& key, const std::string& value) {
    if (!db_) return false;

    const char* sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    StmtGuard stmt(db_, sql);
    if (!stmt) return false;

    stmt.bindText(1, key);
    stmt.bindText(2, value);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        LOG_ERROR(std::string("setSetting failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

// --- 搜索 (FTS5) ---

std::vector<Email> StorageManager::search(const std::string& query,
                                           const std::string& tag,
                                           int64_t fromDate, int64_t toDate,
                                           int limit) {
    std::vector<Email> result;
    if (!db_ || query.empty()) return result;

    // FTS5 全文搜索
    std::string sql = R"(
        SELECT e.id, e.account_id, e.sender, e.recipients, e.subject,
               e.body_plain, e.body_html, e.received_at, e.sent_at,
               e.is_read, e.is_starred, e.folder
        FROM emails_fts f
        JOIN emails e ON f.rowid = e.rowid
        WHERE emails_fts MATCH ?
    )";

    if (!tag.empty()) {
        sql += " AND e.id IN (SELECT email_id FROM ai_tags WHERE tag = ?)";
    }
    if (fromDate > 0) {
        sql += " AND e.received_at >= ?";
    }
    if (toDate > 0) {
        sql += " AND e.received_at <= ?";
    }
    sql += " ORDER BY rank LIMIT ?;";

    StmtGuard stmt(db_, sql.c_str());
    if (!stmt) return result;

    int idx = 1;
    stmt.bindText(idx++, query);
    if (!tag.empty()) stmt.bindText(idx++, tag);
    if (fromDate > 0) stmt.bindInt(idx++, fromDate);
    if (toDate > 0)   stmt.bindInt(idx++, toDate);
    stmt.bindInt(idx++, limit);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        result.push_back(emailFromRow(stmt.get()));
    }
    return result;
}

// --- Schema 初始化 ---

bool StorageManager::initSchema() {
    LOG_DEBUG("Initializing database schema...");

    const char* tables[] = {
        // accounts
        R"(CREATE TABLE IF NOT EXISTS accounts (
            id TEXT PRIMARY KEY,
            display_name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            encrypted_password TEXT NOT NULL,
            smtp_server TEXT NOT NULL,
            smtp_port INTEGER NOT NULL DEFAULT 465,
            imap_server TEXT NOT NULL,
            imap_port INTEGER NOT NULL DEFAULT 993,
            pop3_server TEXT DEFAULT '',
            pop3_port INTEGER NOT NULL DEFAULT 995,
            use_ssl INTEGER NOT NULL DEFAULT 1,
            preferred_protocol INTEGER NOT NULL DEFAULT 0,
            sync_interval INTEGER NOT NULL DEFAULT 300,
            max_fetch_count INTEGER NOT NULL DEFAULT 50,
            auto_classify INTEGER NOT NULL DEFAULT 1
        );)",
        // emails
        R"(CREATE TABLE IF NOT EXISTS emails (
            id TEXT PRIMARY KEY,
            account_id TEXT NOT NULL,
            sender TEXT NOT NULL,
            recipients TEXT,
            subject TEXT,
            body_plain TEXT,
            body_html TEXT,
            received_at INTEGER,
            sent_at INTEGER,
            is_read INTEGER DEFAULT 0,
            is_starred INTEGER DEFAULT 0,
            folder TEXT DEFAULT 'INBOX',
            FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
        );)",
        // ai_tags
        R"(CREATE TABLE IF NOT EXISTS ai_tags (
            email_id TEXT PRIMARY KEY,
            tag TEXT NOT NULL CHECK(tag IN ('工作', '个人', '账单', '通知', '社交', '重要')),
            is_manual INTEGER DEFAULT 0,
            classified_at INTEGER DEFAULT (strftime('%s','now')),
            FOREIGN KEY (email_id) REFERENCES emails(id) ON DELETE CASCADE
        );)",
        // settings
        R"(CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );)",
    };

    char* err = nullptr;
    for (const char* ddl : tables) {
        int rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            LOG_ERROR(std::string("Schema init error: ") + (err ? err : "unknown"));
            if (err) { sqlite3_free(err); err = nullptr; }
            return false;
        }
    }

    // 索引
    const char* indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_emails_account  ON emails(account_id);",
        "CREATE INDEX IF NOT EXISTS idx_emails_folder   ON emails(account_id, folder);",
        "CREATE INDEX IF NOT EXISTS idx_emails_received ON emails(received_at DESC);",
        "CREATE INDEX IF NOT EXISTS idx_emails_read     ON emails(account_id, is_read);",
    };
    for (const char* idx : indexes) {
        sqlite3_exec(db_, idx, nullptr, nullptr, nullptr);
    }

    return createFtsIndex();
}

bool StorageManager::createFtsIndex() {
    const char* sql = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS emails_fts
        USING fts5(sender, subject, body_plain, content='emails', content_rowid='rowid');
    )";

    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        LOG_WARN(std::string("FTS5: ") + (err ? err : "ok"));
        if (err) sqlite3_free(err);
    }
    return true;
}

} // namespace SmartMail
