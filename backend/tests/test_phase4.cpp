/// AccountManager 单元测试
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "account/AccountManager.hpp"
#include "account/CryptoUtils.hpp"

using namespace SmartMail;

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) \
    do { std::cout << "  TEST: " << name << " ... "; } while(0)

#define PASS() \
    do { std::cout << "PASSED\n"; g_passed++; } while(0)

#define FAIL(msg) \
    do { std::cout << "FAILED - " << msg << "\n"; g_failed++; } while(0)

#define CHECK(cond, msg) \
    do { if (!(cond)) { FAIL(msg); return; } } while(0)

// --- Helpers ---

static AccountConfig makeTestAccount(const std::string& email) {
    AccountConfig acc;
    acc.displayName = "Test User";
    acc.email = email;
    acc.encryptedPassword = "test_password_123";
    acc.smtpServer = "smtp." + email.substr(email.find('@') + 1);
    acc.smtpPort = 465;
    acc.smtpUseSSL = true;
    acc.imapServer = "imap." + email.substr(email.find('@') + 1);
    acc.imapPort = 993;
    acc.imapUseSSL = true;
    acc.syncInterval = 300;
    acc.maxFetchCount = 50;
    acc.autoClassify = true;
    return acc;
}

// --- Master Password Tests ---

static void test_set_master_password() {
    TEST("set master password unlocks manager");
    AccountManager mgr;
    CHECK(!mgr.isUnlocked(), "should be locked initially");
    mgr.setMasterPassword("my_secure_password");
    CHECK(mgr.isUnlocked(), "should be unlocked after set");
    PASS();
}

static void test_verify_master_password() {
    TEST("verify correct master password");
    AccountManager mgr;
    mgr.setMasterPassword("correct_horse_battery_staple");
    CHECK(mgr.verifyMasterPassword("correct_horse_battery_staple"), "correct password should verify");
    CHECK(!mgr.verifyMasterPassword("wrong_password"), "wrong password should not verify");
    PASS();
}

static void test_unlock_after_load() {
    TEST("unlock with password after simulated load");
    AccountManager mgr;

    // Step 1: 设置主密码（模拟首次使用）
    mgr.setMasterPassword("my_secret");
    CHECK(mgr.isUnlocked(), "should be unlocked");

    // Step 2: 保存到文件
    bool saved = mgr.saveToFile("test_phase4_accounts.json");
    CHECK(saved, "save should succeed");

    // Step 3: 新建一个 Manager 加载文件（模拟重启）
    AccountManager mgr2;
    bool loaded = mgr2.loadFromFile("test_phase4_accounts.json");
    CHECK(loaded, "load should succeed");
    CHECK(!mgr2.isUnlocked(), "should be locked after load (key not in file)");

    // Step 4: 用正确密码解锁
    bool unlocked = mgr2.unlock("my_secret");
    CHECK(unlocked, "unlock with correct password should succeed");
    CHECK(mgr2.isUnlocked(), "should be unlocked");

    // Step 5: 错误密码解锁失败
    AccountManager mgr3;
    mgr3.loadFromFile("test_phase4_accounts.json");
    CHECK(!mgr3.unlock("wrong_password"), "unlock with wrong password should fail");
    CHECK(!mgr3.isUnlocked(), "should remain locked");

    std::remove("test_phase4_accounts.json");
    PASS();
}

// --- CRUD Tests ---

static void test_add_account_encrypts_password() {
    TEST("add account encrypts password");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    AccountConfig acc = makeTestAccount("user@example.com");
    acc.encryptedPassword = "plaintext_app_password";

    bool ok = mgr.add(acc);
    CHECK(ok, "add should succeed");

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "should have 1 account");
    CHECK(all[0].encryptedPassword != "plaintext_app_password",
          "stored password should be encrypted (not plaintext)");
    PASS();
}

static void test_get_decrypted_password() {
    TEST("getDecryptedPassword returns original plaintext");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    AccountConfig acc = makeTestAccount("test@example.com");
    const std::string originalPassword = "my_app_password";
    acc.encryptedPassword = originalPassword;

    CHECK(mgr.add(acc), "add should succeed");

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "should have 1 account");
    std::string decrypted = mgr.getDecryptedPassword(all[0].id);
    CHECK(decrypted == originalPassword, "decrypted password should match original");
    PASS();
}

static void test_add_requires_unlock() {
    TEST("add fails when locked");
    AccountManager mgr;
    AccountConfig acc = makeTestAccount("user@example.com");
    bool ok = mgr.add(acc);
    CHECK(!ok, "add should fail without master password");
    PASS();
}

static void test_add_duplicate_email() {
    TEST("add duplicate email is rejected");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    mgr.add(makeTestAccount("dup@example.com"));
    bool ok = mgr.add(makeTestAccount("dup@example.com"));
    CHECK(!ok, "duplicate email should be rejected");
    CHECK(mgr.getAll().size() == 1, "should still have exactly 1 account");
    PASS();
}

static void test_get_all_and_get_by_id() {
    TEST("getAll and getById");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    auto acc1 = makeTestAccount("alice@example.com");
    auto acc2 = makeTestAccount("bob@example.com");
    mgr.add(acc1);
    mgr.add(acc2);

    auto all = mgr.getAll();
    CHECK(all.size() == 2, "should have 2 accounts");

    auto found = mgr.getById(all[0].id);
    CHECK(found != nullptr, "getById should find account");
    CHECK(found->email == all[0].email, "email should match");

    auto missing = mgr.getById("nonexistent_id");
    CHECK(missing == nullptr, "nonexistent id should return null");
    PASS();
}

static void test_update_account() {
    TEST("update account");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    auto acc = makeTestAccount("update@example.com");
    CHECK(mgr.add(acc), "add should succeed");

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "should have 1 account");

    // 更新显示名称
    auto existing = mgr.getById(all[0].id);
    existing->displayName = "Updated Name";
    bool ok = mgr.update(*existing);
    CHECK(ok, "update should succeed");

    auto updated = mgr.getById(all[0].id);
    CHECK(updated->displayName == "Updated Name", "displayName should be updated");
    PASS();
}

static void test_update_password() {
    TEST("update account password");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    auto acc = makeTestAccount("pass@example.com");
    acc.encryptedPassword = "old_password";
    CHECK(mgr.add(acc), "add should succeed");

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "should have 1 account");

    // 更新密码
    auto existing = mgr.getById(all[0].id);
    existing->encryptedPassword = "new_password";
    mgr.update(*existing);

    std::string decrypted = mgr.getDecryptedPassword(all[0].id);
    CHECK(decrypted == "new_password", "password should be updated");
    PASS();
}

static void test_remove_account() {
    TEST("remove account");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    auto acc = makeTestAccount("remove@example.com");
    CHECK(mgr.add(acc), "add should succeed");

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "should have 1 account");

    bool ok = mgr.remove(all[0].id);
    CHECK(ok, "remove should succeed");
    CHECK(mgr.getAll().size() == 0, "should have 0 accounts after remove");

    CHECK(!mgr.remove("nonexistent"), "removing nonexistent should fail");
    PASS();
}

static void test_auto_generate_id() {
    TEST("auto-generate ID when empty");
    AccountManager mgr;
    mgr.setMasterPassword("master123");

    AccountConfig acc = makeTestAccount("idtest@example.com");
    acc.id = "";  // 空 ID
    mgr.add(acc);

    auto all = mgr.getAll();
    CHECK(all.size() == 1, "account should be added");
    CHECK(!all[0].id.empty(), "ID should be auto-generated");
    CHECK(all[0].id.length() >= 8, "ID should be at least 8 chars");
    PASS();
}

// --- File Persistence Tests ---

static void test_save_and_load_full_roundtrip() {
    TEST("saveToFile + loadFromFile + unlock full roundtrip");
    AccountManager mgr1;
    mgr1.setMasterPassword("my_master_key");

    // 添加两个账号
    auto acc1 = makeTestAccount("alice@example.com");
    acc1.encryptedPassword = "alice_password";
    mgr1.add(acc1);

    auto acc2 = makeTestAccount("bob@example.com");
    acc2.encryptedPassword = "bob_password";
    mgr1.add(acc2);

    CHECK(mgr1.saveToFile("test_phase4_roundtrip.json"), "save should succeed");

    // 新建 Manager 加载
    AccountManager mgr2;
    CHECK(mgr2.loadFromFile("test_phase4_roundtrip.json"), "load should succeed");
    CHECK(!mgr2.isUnlocked(), "should be locked after load");
    CHECK(mgr2.unlock("my_master_key"), "unlock should succeed");

    // 验证数据
    auto all = mgr2.getAll();
    CHECK(all.size() == 2, "should have 2 accounts");

    // 验证密码可解密
    std::string dec1 = mgr2.getDecryptedPassword(all[0].id);
    CHECK(dec1 == "alice_password" || dec1 == "bob_password",
          "password should decrypt correctly");

    std::remove("test_phase4_roundtrip.json");
    PASS();
}

static void test_load_nonexistent_file() {
    TEST("load nonexistent file returns false");
    AccountManager mgr;
    bool ok = mgr.loadFromFile("this_file_does_not_exist.json");
    CHECK(!ok, "should return false for nonexistent file");
    PASS();
}

static void test_save_empty_accounts() {
    TEST("save and load with zero accounts");
    AccountManager mgr;
    mgr.setMasterPassword("key123");

    CHECK(mgr.saveToFile("test_phase4_empty.json"), "save empty should succeed");

    AccountManager mgr2;
    CHECK(mgr2.loadFromFile("test_phase4_empty.json"), "load should succeed");
    CHECK(mgr2.unlock("key123"), "unlock should succeed");
    CHECK(mgr2.getAll().empty(), "should have 0 accounts");

    std::remove("test_phase4_empty.json");
    PASS();
}

static void test_file_format_contains_required_fields() {
    TEST("file format structure");
    AccountManager mgr;
    mgr.setMasterPassword("format_test");
    mgr.add(makeTestAccount("format@example.com"));
    mgr.saveToFile("test_phase4_format.json");

    // 手动读取文件检查结构
    std::ifstream file("test_phase4_format.json");
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    CHECK(content.find("\"salt\"") != std::string::npos, "file should contain salt");
    CHECK(content.find("\"password_hash\"") != std::string::npos, "file should contain password_hash");
    CHECK(content.find("\"accounts\"") != std::string::npos, "file should contain accounts");
    CHECK(content.find("\"encrypted_password\"") != std::string::npos,
          "file should contain encrypted_password");

    std::remove("test_phase4_format.json");
    PASS();
}

// --- Main ---

int main() {
    std::cout << "=== Phase 4 Unit Tests: AccountManager ===\n\n";

    std::cout << "[Master Password]\n";
    test_set_master_password();
    test_verify_master_password();
    test_unlock_after_load();

    std::cout << "\n[CRUD]\n";
    test_add_account_encrypts_password();
    test_get_decrypted_password();
    test_add_requires_unlock();
    test_add_duplicate_email();
    test_get_all_and_get_by_id();
    test_update_account();
    test_update_password();
    test_remove_account();
    test_auto_generate_id();

    std::cout << "\n[File Persistence]\n";
    test_save_and_load_full_roundtrip();
    test_load_nonexistent_file();
    test_save_empty_accounts();
    test_file_format_contains_required_fields();

    std::cout << "\n=== Results: " << g_passed << " passed, "
              << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
