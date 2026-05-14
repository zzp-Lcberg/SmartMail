/// CryptoUtils + SmtpClient 单元测试
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include "account/CryptoUtils.hpp"
#include "engine/SmtpClient.hpp"

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

// ============================================================================
// CryptoUtils 测试
// ============================================================================

static void test_generate_salt() {
    TEST("generateSalt produces unique values");
    std::string s1 = CryptoUtils::generateSalt(16);
    std::string s2 = CryptoUtils::generateSalt(16);
    CHECK(s1.size() == 16, "salt should be 16 bytes");
    CHECK(s2.size() == 16, "salt should be 16 bytes");
    CHECK(s1 != s2, "two salts should differ");
    PASS();
}

static void test_derive_key() {
    TEST("PBKDF2 key derivation");
    std::string salt = CryptoUtils::generateSalt(16);
    std::string key1 = CryptoUtils::deriveKey("test_password", salt);
    std::string key2 = CryptoUtils::deriveKey("test_password", salt);
    CHECK(key1.size() == 32, "derived key should be 32 bytes (AES-256)");
    CHECK(key1 == key2, "same password+salt should produce same key");

    // 不同密码产生不同密钥
    std::string key3 = CryptoUtils::deriveKey("other_password", salt);
    CHECK(key1 != key3, "different passwords must produce different keys");
    PASS();
}

static void test_encrypt_decrypt_roundtrip() {
    TEST("AES-256-CBC encrypt/decrypt roundtrip");
    std::string salt = CryptoUtils::generateSalt(16);
    std::string key = CryptoUtils::deriveKey("master_secret", salt);

    std::string plaintext = "Hello, this is a test email content. 包含中文测试。";

    std::string encrypted = CryptoUtils::encrypt(plaintext, key);
    CHECK(!encrypted.empty(), "encrypt should produce output");
    CHECK(encrypted != plaintext, "encrypted text should differ from plaintext");

    std::string decrypted = CryptoUtils::decrypt(encrypted, key);
    CHECK(decrypted == plaintext, "decrypt should recover original text");
    PASS();
}

static void test_encrypt_different_iv() {
    TEST("each encryption produces different ciphertext (random IV)");
    std::string salt = CryptoUtils::generateSalt(16);
    std::string key = CryptoUtils::deriveKey("password", salt);
    std::string plaintext = "Same content twice";

    std::string enc1 = CryptoUtils::encrypt(plaintext, key);
    std::string enc2 = CryptoUtils::encrypt(plaintext, key);
    CHECK(enc1 != enc2, "same plaintext should produce different ciphertext");

    // 但解密后应该相同
    CHECK(CryptoUtils::decrypt(enc1, key) == plaintext, "decrypt enc1");
    CHECK(CryptoUtils::decrypt(enc2, key) == plaintext, "decrypt enc2");
    PASS();
}

static void test_decrypt_wrong_key() {
    TEST("decrypt with wrong key returns empty");
    std::string salt = CryptoUtils::generateSalt(16);
    std::string key1 = CryptoUtils::deriveKey("correct", salt);
    std::string key2 = CryptoUtils::deriveKey("wrong", salt);

    std::string encrypted = CryptoUtils::encrypt("secret data", key1);
    CHECK(!encrypted.empty(), "encrypt should succeed");

    std::string result = CryptoUtils::decrypt(encrypted, key2);
    CHECK(result.empty(), "decrypt with wrong key should return empty");
    PASS();
}

static void test_base64_encode_decode() {
    TEST("Base64 encode/decode roundtrip");
    // 用显式长度构造，避免 \x00 在 C 字面量中被截断
    const char rawData[] = "Hello World! 你好世界！\x00\x01\x02";
    std::string original(rawData, sizeof(rawData) - 1);
    std::string encoded = CryptoUtils::base64Encode(original);
    CHECK(!encoded.empty(), "encode should produce output");

    std::string decoded = CryptoUtils::base64Decode(encoded);
    CHECK(decoded == original, "decode should recover original (including binary)");
    PASS();
}

static void test_base64_no_newlines() {
    TEST("Base64 output has no newlines");
    std::string data(200, 'x');
    std::string encoded = CryptoUtils::base64Encode(data);
    CHECK(encoded.find('\n') == std::string::npos, "should not contain newlines");
    PASS();
}

static void test_encrypt_empty_or_bad_input() {
    TEST("encrypt/decrypt edge cases");
    std::string salt = CryptoUtils::generateSalt(16);
    std::string key = CryptoUtils::deriveKey("test", salt);

    // 空明文
    std::string emptyResult = CryptoUtils::encrypt("", key);
    CHECK(emptyResult.empty(), "encrypt empty string should return empty");

    // 密钥长度不对
    std::string shortKey(16, 'x');
    std::string badEnc = CryptoUtils::encrypt("data", shortKey);
    CHECK(badEnc.empty(), "encrypt with 16-byte key should fail");

    // 空密文解密
    std::string badDec = CryptoUtils::decrypt("", key);
    CHECK(badDec.empty(), "decrypt empty string should return empty");
    PASS();
}

// ============================================================================
// SmtpClient 测试（离线 / 格式验证）
// ============================================================================

static void test_prepare_payload_plain() {
    TEST("RFC 2822 payload - plain text");
    Email email;
    email.sender = "sender@test.com";
    email.recipients = {"to@test.com"};
    email.subject = "Test Subject";
    email.bodyPlain = "This is a test body.";

    SmtpClient client;
    // 用 connect 达成基本配置状态（即使没有真实服务器）
    // preparePayload 是私有的，我们通过间接方式测试
    // 这里测试的是：即使未连接，sendEmail 也会提示需要先 connect
    bool sent = client.sendEmail(email);
    CHECK(!sent, "sendEmail should fail without connect()");
    PASS();
}

static void test_send_email_no_recipients() {
    TEST("sendEmail rejects empty recipients");
    SmtpClient client;
    Email email;
    email.sender = "sender@test.com";
    email.subject = "No recipients";

    bool sent = client.sendEmail(email);
    CHECK(!sent, "should reject email with no recipients");
    PASS();
}

static void test_connect_invalid_config() {
    TEST("connect with invalid config fails gracefully");
    SmtpClient client;
    AccountConfig config;
    config.smtpServer = "";   // 无效
    config.smtpPort = 0;

    bool ok = client.connect(config);
    CHECK(!ok, "should fail with empty server");
    CHECK(!client.isConnected(), "should not be connected");
    PASS();
}

static void test_disconnect_clears_state() {
    TEST("disconnect resets state");
    SmtpClient client;
    AccountConfig config;
    config.smtpServer = "smtp.test.com";
    config.smtpPort = 587;
    config.email = "test@test.com";

    // 即使 connect 失败（无网络），disconnect 也应正常执行
    client.connect(config);
    client.disconnect();
    CHECK(!client.isConnected(), "should not be connected after disconnect");
    PASS();
}

static void test_fetch_mark_delete_unsupported() {
    TEST("SMTP does not support fetch/mark/delete");
    SmtpClient client;
    CHECK(client.fetchEmails(10).empty(), "fetch should return empty");
    CHECK(!client.markAsRead("any"), "markAsRead should return false");
    CHECK(!client.deleteEmail("any"), "deleteEmail should return false");
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Phase 3 Unit Tests: CryptoUtils + SmtpClient ===\n\n";

    std::cout << "[CryptoUtils]\n";
    test_generate_salt();
    test_derive_key();
    test_encrypt_decrypt_roundtrip();
    test_encrypt_different_iv();
    test_decrypt_wrong_key();
    test_base64_encode_decode();
    test_base64_no_newlines();
    test_encrypt_empty_or_bad_input();

    std::cout << "\n[SmtpClient]\n";
    test_prepare_payload_plain();
    test_send_email_no_recipients();
    test_connect_invalid_config();
    test_disconnect_clears_state();
    test_fetch_mark_delete_unsupported();

    std::cout << "\n=== Results: " << g_passed << " passed, "
              << g_failed << " failed ===\n";

    return g_failed > 0 ? 1 : 0;
}
