/// StorageManager 单元测试
#include <cassert>
#include <cstdio>
#include <iostream>
#include "storage/StorageManager.hpp"
#include "types/AccountConfig.hpp"

using namespace SmartMail;

static int g_passed = 0;
static int g_failed = 0;

// Helper: create a minimal account record to satisfy foreign key constraint
static AccountConfig makeTestAccount(const std::string& id) {
    AccountConfig acc;
    acc.id = id;
    acc.displayName = "Test " + id;
    acc.email = id + "@test.com";
    acc.encryptedPassword = "pw";
    acc.smtpServer = "smtp.test.com";
    acc.imapServer = "imap.test.com";
    return acc;
}

#define TEST(name) \
    do { std::cout << "  TEST: " << name << " ... "; } while(0)

#define PASS() \
    do { std::cout << "PASSED\n"; g_passed++; } while(0)

#define FAIL(msg) \
    do { std::cout << "FAILED - " << msg << "\n"; g_failed++; } while(0)

#define CHECK(cond, msg) \
    do { if (!(cond)) { FAIL(msg); return; } } while(0)

// --- Test fixtures ---

static Email makeTestEmail(const std::string& id, const std::string& accountId,
                            const std::string& folder = "INBOX") {
    Email e;
    e.id = id;
    e.accountId = accountId;
    e.sender = "test@example.com";
    e.recipients = {"user@example.com"};
    e.subject = "Test Subject " + id;
    e.bodyPlain = "This is a test email body for " + id;
    e.bodyHtml = "<p>This is a test email body for " + id + "</p>";
    e.receivedAt = 1700000000 + static_cast<int64_t>(id.size()) * 1000 + static_cast<int64_t>(id[0]);
    e.folder = folder;
    e.isRead = false;
    return e;
}

// --- Tests ---

static void test_open_and_close() {
    TEST("open database");
    StorageManager sm;
    CHECK(!sm.isOpen(), "should be closed initially");

    bool ok = sm.open("test_storage.db");
    CHECK(ok, "open failed");
    CHECK(sm.isOpen(), "should be open after open()");

    sm.close();
    CHECK(!sm.isOpen(), "should be closed after close()");
    std::remove("test_storage.db");
    PASS();
}

static void test_save_and_get_email() {
    TEST("save and get email");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));

    Email email = makeTestEmail("e001", "acc1");
    CHECK(sm.saveEmail(email), "saveEmail failed");

    auto retrieved = sm.getEmailById("e001");
    CHECK(retrieved != nullptr, "getEmailById returned null");
    CHECK(retrieved->id == "e001", "wrong id");
    CHECK(retrieved->accountId == "acc1", "wrong accountId");
    CHECK(retrieved->sender == "test@example.com", "wrong sender");
    CHECK(retrieved->subject == "Test Subject e001", "wrong subject");
    CHECK(retrieved->folder == "INBOX", "wrong folder");
    CHECK(!retrieved->isRead, "should be unread");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_save_and_get_emails_by_folder() {
    TEST("get emails by folder");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));
    sm.saveAccount(makeTestAccount("acc2"));

    sm.saveEmail(makeTestEmail("e01", "acc1", "INBOX"));
    sm.saveEmail(makeTestEmail("e02", "acc1", "INBOX"));
    sm.saveEmail(makeTestEmail("e03", "acc1", "Sent"));
    sm.saveEmail(makeTestEmail("e04", "acc2", "INBOX"));

    auto inbox1 = sm.getEmails("acc1", "INBOX", 50, 0);
    CHECK(inbox1.size() == 2, "acc1 INBOX should have 2 emails");

    auto sent = sm.getEmails("acc1", "Sent", 50, 0);
    CHECK(sent.size() == 1, "acc1 Sent should have 1 email");

    auto inbox2 = sm.getEmails("acc2", "INBOX", 50, 0);
    CHECK(inbox2.size() == 1, "acc2 INBOX should have 1 email");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_mark_as_read() {
    TEST("mark as read");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));
    sm.saveEmail(makeTestEmail("e01", "acc1"));
    CHECK(sm.markAsRead("e01"), "markAsRead failed");

    auto email = sm.getEmailById("e01");
    CHECK(email != nullptr, "email not found");
    CHECK(email->isRead, "email should be marked as read");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_soft_delete() {
    TEST("soft delete (move to trash)");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));
    sm.saveEmail(makeTestEmail("e01", "acc1", "INBOX"));
    CHECK(sm.deleteEmail("e01"), "deleteEmail failed");

    auto email = sm.getEmailById("e01");
    CHECK(email != nullptr, "email should still exist");
    CHECK(email->folder == "Trash", "folder should be Trash");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_unread_count() {
    TEST("unread count");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));

    Email e1 = makeTestEmail("e01", "acc1");
    e1.isRead = false;
    Email e2 = makeTestEmail("e02", "acc1");
    e2.isRead = true;
    Email e3 = makeTestEmail("e03", "acc1");
    e3.isRead = false;

    sm.saveEmail(e1);
    sm.saveEmail(e2);
    sm.saveEmail(e3);

    int count = sm.getUnreadCount("acc1");
    CHECK(count == 2, "should have 2 unread emails in INBOX");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_save_emails_batch() {
    TEST("batch save emails");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));

    std::vector<Email> batch;
    for (int i = 0; i < 10; ++i) {
        batch.push_back(makeTestEmail("batch" + std::to_string(i), "acc1"));
    }
    CHECK(sm.saveEmails(batch), "batch saveEmails failed");

    auto all = sm.getEmails("acc1", "INBOX", 50, 0);
    CHECK(all.size() == 10, "should have 10 emails after batch insert");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_ai_tags() {
    TEST("AI tag operations");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));
    sm.saveEmail(makeTestEmail("e01", "acc1"));

    CHECK(sm.saveAiTag("e01", "工作", false), "saveAiTag failed");

    std::string tag = sm.getAiTag("e01");
    CHECK(tag == "工作", "getAiTag should return 工作");

    CHECK(sm.updateAiTag("e01", "个人"), "updateAiTag failed");
    tag = sm.getAiTag("e01");
    CHECK(tag == "个人", "getAiTag should return 个人 after update");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_settings() {
    TEST("settings CRUD");
    StorageManager sm;
    sm.open("test_storage.db");

    CHECK(sm.setSetting("theme", "dark"), "setSetting failed");
    CHECK(sm.setSetting("language", "zh-CN"), "setSetting failed");

    std::string theme = sm.getSetting("theme");
    CHECK(theme == "dark", "theme should be dark");

    std::string lang = sm.getSetting("language");
    CHECK(lang == "zh-CN", "language should be zh-CN");

    // default value for missing key
    std::string missing = sm.getSetting("nonexistent", "default_val");
    CHECK(missing == "default_val", "should return default for missing key");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

static void test_pagination() {
    TEST("pagination with limit/offset");
    StorageManager sm;
    sm.open("test_storage.db");

    sm.saveAccount(makeTestAccount("acc1"));

    for (int i = 0; i < 25; ++i) {
        sm.saveEmail(makeTestEmail("page" + std::to_string(i), "acc1"));
    }

    auto page1 = sm.getEmails("acc1", "INBOX", 10, 0);
    CHECK(page1.size() == 10, "page1 should have 10 emails");

    auto page2 = sm.getEmails("acc1", "INBOX", 10, 10);
    CHECK(page2.size() == 10, "page2 should have 10 emails");

    auto page3 = sm.getEmails("acc1", "INBOX", 10, 20);
    CHECK(page3.size() == 5, "page3 should have 5 emails");

    sm.close();
    std::remove("test_storage.db");
    PASS();
}

// --- Main ---

int main() {
    std::cout << "=== StorageManager Unit Tests ===\n\n";

    test_open_and_close();
    test_save_and_get_email();
    test_save_and_get_emails_by_folder();
    test_mark_as_read();
    test_soft_delete();
    test_unread_count();
    test_save_emails_batch();
    test_ai_tags();
    test_settings();
    test_pagination();

    std::cout << "\n=== Results: " << g_passed << " passed, "
              << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
