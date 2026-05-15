/// Phase 7: AI Service Integration unit tests
/// Uses a local mock OpenAI server (httplib) — no network dependency
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include "server/httplib.h"
#include "ai/AiService.hpp"
#include "ai/OpenAiClient.hpp"
#include "storage/StorageManager.hpp"
#include "server/ApiServer.hpp"
#include "types/AiResult.hpp"
#include "types/Email.hpp"
#include "types/AccountConfig.hpp"

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
// Mock OpenAI Server
// ============================================================================

/// Starts a mock OpenAI-compatible HTTP server on the given port in a detached thread.
/// Returns the server pointer; caller is responsible for calling stop() and joining.
class MockOpenAiServer {
public:
    MockOpenAiServer(int port) : port_(port) {
        server_.Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            std::string body = req.body;
            bool isReply = (body.find("原始邮件") != std::string::npos ||
                            body.find("回复") != std::string::npos);

            // Simulate error modes via special markers in content
            if (body.find("__SIMULATE_500__") != std::string::npos) {
                res.status = 500;
                res.set_content(R"({"error":{"message":"Internal server error"}})", "application/json");
                return;
            }
            if (body.find("__SIMULATE_429__") != std::string::npos) {
                if (rateLimitCount_ == 0) {
                    rateLimitCount_++;
                    res.status = 429;
                    res.set_content(R"({"error":{"message":"Rate limit exceeded"}})", "application/json");
                    return;
                }
            }

            nlohmann::json resp;
            nlohmann::json choice;
            nlohmann::json message;
            if (isReply) {
                message["content"] = "这是AI生成的回复内容。";
            } else {
                message["content"] = "工作";
            }
            choice["message"] = message;
            resp["choices"] = nlohmann::json::array({choice});
            res.set_content(resp.dump(), "application/json");
        });

        thread_ = std::thread([this]() {
            server_.listen("127.0.0.1", port_);
        });

        // Wait for server to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ~MockOpenAiServer() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string url() const {
        return "http://127.0.0.1:" + std::to_string(port_) + "/v1/chat/completions";
    }

private:
    int port_;
    httplib::Server server_;
    std::thread thread_;
    int rateLimitCount_ = 0;
};

// ============================================================================
// Test: Classify — success
// ============================================================================
static void test_classify_success() {
    TEST("OpenAiClient classify returns valid tag");
    MockOpenAiServer mock(19090);

    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = mock.url();
    OpenAiClient client(config);

    ClassificationResult result = client.classify("这是一封工作邮件，关于项目进度汇报。");
    CHECK(result.success, "Classify should succeed");
    CHECK(result.tag == "工作", "Expected tag '工作', got '" + result.tag + "'");
    PASS();
}

// ============================================================================
// Test: Reply — success
// ============================================================================
static void test_reply_success() {
    TEST("OpenAiClient generateReply returns content");
    MockOpenAiServer mock(19091);

    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = mock.url();
    OpenAiClient client(config);

    ReplyResult result = client.generateReply("原始邮件：\n你好，请确认会议时间。", "");
    CHECK(result.success, "Reply should succeed");
    CHECK(!result.replyContent.empty(), "Reply content should not be empty");
    PASS();
}

// ============================================================================
// Test: Empty API key
// ============================================================================
static void test_classify_empty_apikey() {
    TEST("OpenAiClient with empty apiKey returns failure (no crash)");
    MockOpenAiServer mock(19092);

    OpenAiConfig config;
    config.apiKey = "";  // Empty key
    config.baseUrl = mock.url();
    OpenAiClient client(config);

    ClassificationResult result = client.classify("测试邮件内容");
    CHECK(!result.success, "Should fail with empty API key");
    PASS();
}

// ============================================================================
// Test: Bad URL (network error handling)
// ============================================================================
static void test_classify_bad_url() {
    TEST("OpenAiClient handles bad URL gracefully");
    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = "http://127.0.0.1:19999/nonexistent/endpoint";
    config.maxRetries = 0;  // Don't wait for retries
    OpenAiClient client(config);

    ClassificationResult result = client.classify("测试邮件内容");
    CHECK(!result.success, "Should fail with bad URL");
    PASS();
}

// ============================================================================
// Test: Retry on failure (429 then success)
// ============================================================================
static void test_retry_on_failure() {
    TEST("OpenAiClient retries on 429 then succeeds");
    MockOpenAiServer mock(19093);

    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = mock.url();
    config.maxRetries = 2;
    OpenAiClient client(config);

    // __SIMULATE_429__ triggers rate limit on first attempt, then succeeds
    ClassificationResult result = client.classify("__SIMULATE_429__ 测试邮件内容");
    CHECK(result.success, "Should succeed after retry");
    CHECK(result.tag == "工作", "Expected tag '工作', got '" + result.tag + "'");
    PASS();
}

// ============================================================================
// Test: End-to-end classify via ApiServer saves tag
// ============================================================================
static void test_classify_endpoint_saves_tag() {
    TEST("POST /api/emails/{id}/classify — tag saved after async completion");
    MockOpenAiServer mock(19094);

    StorageManager storage;
    CHECK(storage.open("test_phase7_temp1.db"), "Failed to open temp DB");

    // Insert dummy account + email (FK constraints: emails→accounts, ai_tags→emails)
    AccountConfig dummyAcc;
    dummyAcc.id = "test-account";
    dummyAcc.email = "test-account@example.com";
    dummyAcc.displayName = "Test Account";
    dummyAcc.encryptedPassword = "encrypted-test-pass";
    dummyAcc.smtpServer = "smtp.example.com";
    dummyAcc.imapServer = "imap.example.com";
    storage.saveAccount(dummyAcc);

    Email dummyEmail;
    dummyEmail.id = "test-email-1";
    dummyEmail.accountId = "test-account";
    dummyEmail.sender = "sender@example.com";
    dummyEmail.subject = "Test Subject";
    dummyEmail.folder = "INBOX";
    storage.saveEmail(dummyEmail);

    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = mock.url();
    config.maxRetries = 0;
    AiService aiService(config);
    aiService.setStorageManager(&storage);

    ApiServer server;
    server.setStorageManager(&storage);
    server.setAiService(&aiService);
    CHECK(server.start("127.0.0.1", 19095), "Failed to start ApiServer");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Trigger async classify
    httplib::Client cli("http://127.0.0.1:19095");
    auto res = cli.Post("/api/emails/test-email-1/classify",
                        R"({"content":"项目进度汇报邮件"})",
                        "application/json");
    CHECK(res != nullptr, "No response from classify POST");
    CHECK(res->status == 202, "Expected 202 Accepted, got " + std::to_string(res->status));

    // Poll for the tag to be saved (async completion)
    std::string tag;
    bool gotTag = false;
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        tag = storage.getAiTag("test-email-1");
        if (!tag.empty()) {
            gotTag = true;
            break;
        }
    }
    CHECK(gotTag, "Tag was not saved after 5 seconds (async classify may have failed)");
    CHECK(tag == "工作", "Expected tag '工作', got '" + tag + "'");

    server.stop();
    storage.close();
    std::remove("test_phase7_temp1.db");
    PASS();
}

// ============================================================================
// Test: Reply endpoint broadcasts via WebSocket
// ============================================================================
static void test_reply_endpoint_broadcasts() {
    TEST("POST /api/emails/{id}/reply — WebSocket broadcast received");
    MockOpenAiServer mock(19096);

    OpenAiConfig config;
    config.apiKey = "sk-test-key-1234567890";
    config.baseUrl = mock.url();
    config.maxRetries = 0;
    AiService aiService(config);

    ApiServer server;
    server.setAiService(&aiService);
    CHECK(server.start("127.0.0.1", 19097), "Failed to start ApiServer");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect WebSocket client in a separate thread (matching test_phase6 pattern)
    std::atomic<bool> wsConnected{false};
    std::string wsMessage;
    std::atomic<bool> gotReply{false};

    std::thread wsThread([&]() {
        httplib::ws::WebSocketClient ws("ws://127.0.0.1:19097/ws/notifications");
        if (ws.connect()) {
            wsConnected = true;
            for (int i = 0; i < 50; ++i) {
                std::string msg;
                auto result = ws.read(msg);
                if (result != httplib::ws::ReadResult::Fail && !msg.empty()) {
                    auto j = nlohmann::json::parse(msg);
                    if (j["event"] == "reply_ready") {
                        wsMessage = msg;
                        gotReply = true;
                        break;
                    }
                }
            }
            ws.close();
        }
    });

    // Wait for WebSocket to connect
    int waitCount = 0;
    while (!wsConnected && waitCount < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }
    CHECK(wsConnected, "WebSocket failed to connect");

    // Trigger async reply
    httplib::Client cli("http://127.0.0.1:19097");
    auto res = cli.Post("/api/emails/test-reply-1/reply",
                        R"({"originalEmail":"原始邮件：\n你好，请确认会议时间。","userPrompt":"请用中文回复"})",
                        "application/json");
    CHECK(res != nullptr, "No response from reply POST");
    CHECK(res->status == 202, "Expected 202 Accepted, got " + std::to_string(res->status));

    wsThread.join();

    CHECK(gotReply, "Did not receive reply_ready WebSocket broadcast within 5s");
    if (!wsMessage.empty()) {
        auto j = nlohmann::json::parse(wsMessage);
        CHECK(j["event"] == "reply_ready", "Expected event 'reply_ready'");
        CHECK(j.contains("data"), "Message should contain data field");
    }

    server.stop();
    PASS();
}

// ============================================================================
// Test: Classify without AiService returns 503
// ============================================================================
static void test_classify_endpoint_no_service() {
    TEST("POST /api/emails/{id}/classify without AiService returns 503");
    ApiServer server;
    CHECK(server.start("127.0.0.1", 19098), "Failed to start ApiServer");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:19098");
    auto res = cli.Post("/api/emails/test-id/classify",
                        R"({"content":"test"})",
                        "application/json");
    CHECK(res != nullptr, "No response");
    CHECK(res->status == 503, "Expected 503, got " + std::to_string(res->status));

    server.stop();
    PASS();
}

// ============================================================================
// Test: reportCorrection updates tag in database
// ============================================================================
static void test_report_correction_updates_tag() {
    TEST("AiService::reportCorrection updates tag via StorageManager");
    StorageManager storage;
    CHECK(storage.open("test_phase7_temp2.db"), "Failed to open temp DB");

    // Insert dummy account + email (FK constraints)
    AccountConfig dummyAcc;
    dummyAcc.id = "corr-account";
    dummyAcc.email = "corr-account@example.com";
    dummyAcc.displayName = "Corr Account";
    dummyAcc.encryptedPassword = "encrypted-pass";
    dummyAcc.smtpServer = "smtp.example.com";
    dummyAcc.imapServer = "imap.example.com";
    storage.saveAccount(dummyAcc);

    Email dummyEmail;
    dummyEmail.id = "correction-test-1";
    dummyEmail.accountId = "corr-account";
    dummyEmail.sender = "sender@example.com";
    dummyEmail.subject = "Correction Test";
    dummyEmail.folder = "INBOX";
    storage.saveEmail(dummyEmail);

    // Save initial AI tag
    storage.saveAiTag("correction-test-1", "通知");

    OpenAiConfig config;
    config.apiKey = "sk-test-key";
    AiService aiService(config);
    aiService.setStorageManager(&storage);

    // User corrects the tag
    aiService.reportCorrection("correction-test-1", "工作");

    // Verify the tag was updated
    std::string updatedTag = storage.getAiTag("correction-test-1");
    CHECK(updatedTag == "工作", "Expected tag '工作' after correction, got '" + updatedTag + "'");

    storage.close();
    std::remove("test_phase7_temp2.db");
    PASS();
}

// ============================================================================
// Test: GET classify tag endpoint
// ============================================================================
static void test_get_classify_tag() {
    TEST("GET /api/emails/{id}/classify returns saved tag");
    StorageManager storage;
    CHECK(storage.open("test_phase7_temp3.db"), "Failed to open temp DB");

    // Insert dummy account + email (FK constraints)
    AccountConfig dummyAcc;
    dummyAcc.id = "get-tag-account";
    dummyAcc.email = "get-tag-account@example.com";
    dummyAcc.displayName = "GetTag Account";
    dummyAcc.encryptedPassword = "encrypted-pass";
    dummyAcc.smtpServer = "smtp.example.com";
    dummyAcc.imapServer = "imap.example.com";
    storage.saveAccount(dummyAcc);

    Email dummyEmail;
    dummyEmail.id = "get-tag-test-1";
    dummyEmail.accountId = "get-tag-account";
    dummyEmail.sender = "sender@example.com";
    dummyEmail.subject = "Get Tag Test";
    dummyEmail.folder = "INBOX";
    storage.saveEmail(dummyEmail);

    // Save a tag
    storage.saveAiTag("get-tag-test-1", "重要");

    ApiServer server;
    server.setStorageManager(&storage);
    CHECK(server.start("127.0.0.1", 19099), "Failed to start ApiServer");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:19099");
    auto res = cli.Get("/api/emails/get-tag-test-1/classify");
    CHECK(res != nullptr, "No response from GET classify");
    CHECK(res->status == 200, "Expected 200, got " + std::to_string(res->status));

    auto j = nlohmann::json::parse(res->body);
    CHECK(j["tag"] == "重要", "Expected tag '重要', got '" + j["tag"].get<std::string>() + "'");

    server.stop();
    storage.close();
    std::remove("test_phase7_temp3.db");
    PASS();
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "=== Phase 7: AI Service Integration Tests ===\n\n";

    std::cout << "[OpenAiClient — Mock Server]\n";
    test_classify_success();
    test_reply_success();
    test_classify_empty_apikey();
    test_classify_bad_url();
    test_retry_on_failure();

    std::cout << "\n[ApiServer AI Endpoints]\n";
    test_classify_endpoint_saves_tag();
    test_reply_endpoint_broadcasts();
    test_classify_endpoint_no_service();
    test_get_classify_tag();

    std::cout << "\n[Feedback Loop]\n";
    test_report_correction_updates_tag();

    std::cout << "\n=== Results: " << g_passed << " passed, "
              << g_failed << " failed ===\n";
    return g_failed > 0 ? 1 : 0;
}
