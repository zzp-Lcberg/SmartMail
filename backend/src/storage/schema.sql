-- SmartMail 数据库初始化脚本
-- Phase 2: Storage Layer

PRAGMA journal_mode=WAL;
PRAGMA foreign_keys=ON;

-- 账号配置表
CREATE TABLE IF NOT EXISTS accounts (
    id              TEXT PRIMARY KEY,
    display_name    TEXT NOT NULL,
    email           TEXT NOT NULL UNIQUE,
    encrypted_password TEXT NOT NULL,
    smtp_server     TEXT NOT NULL,
    smtp_port       INTEGER NOT NULL DEFAULT 465,
    imap_server     TEXT NOT NULL,
    imap_port       INTEGER NOT NULL DEFAULT 993,
    pop3_server     TEXT DEFAULT '',
    pop3_port       INTEGER NOT NULL DEFAULT 995,
    use_ssl         INTEGER NOT NULL DEFAULT 1,
    preferred_protocol INTEGER NOT NULL DEFAULT 0,
    sync_interval   INTEGER NOT NULL DEFAULT 300,
    max_fetch_count INTEGER NOT NULL DEFAULT 50,
    auto_classify   INTEGER NOT NULL DEFAULT 1
);

-- 邮件元数据缓存表
CREATE TABLE IF NOT EXISTS emails (
    id              TEXT PRIMARY KEY,
    account_id      TEXT NOT NULL,
    sender          TEXT NOT NULL,
    recipients      TEXT,
    subject         TEXT,
    body_plain      TEXT,
    body_html       TEXT,
    received_at     INTEGER,
    sent_at         INTEGER,
    is_read         INTEGER DEFAULT 0,
    is_starred      INTEGER DEFAULT 0,
    folder          TEXT DEFAULT 'INBOX',
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- AI 分类标签表
CREATE TABLE IF NOT EXISTS ai_tags (
    email_id    TEXT PRIMARY KEY,
    tag         TEXT NOT NULL CHECK(tag IN ('工作', '个人', '账单', '通知', '社交', '重要')),
    is_manual   INTEGER DEFAULT 0,
    classified_at INTEGER DEFAULT (strftime('%s','now')),
    FOREIGN KEY (email_id) REFERENCES emails(id) ON DELETE CASCADE
);

-- 用户设置表
CREATE TABLE IF NOT EXISTS settings (
    key     TEXT PRIMARY KEY,
    value   TEXT NOT NULL
);

-- 全文索引 (FTS5)
CREATE VIRTUAL TABLE IF NOT EXISTS emails_fts
USING fts5(sender, subject, body_plain, content='emails', content_rowid='rowid');

-- 索引
CREATE INDEX IF NOT EXISTS idx_emails_account  ON emails(account_id);
CREATE INDEX IF NOT EXISTS idx_emails_folder   ON emails(account_id, folder);
CREATE INDEX IF NOT EXISTS idx_emails_received ON emails(received_at DESC);
CREATE INDEX IF NOT EXISTS idx_emails_read     ON emails(account_id, is_read);
