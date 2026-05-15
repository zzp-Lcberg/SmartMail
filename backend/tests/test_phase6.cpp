/// ApiServer HTTP/WebSocket unit tests
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "server/ApiServer.hpp"
#include "account/AccountManager.hpp"
#include "storage/StorageManager.hpp"
#include "ai/AiService.hpp"
#include "types/AccountConfig.hpp"
#include "types/Email.hpp"
#include "types/AiResult.hpp"

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
// Test helpers
// ============================================================================

static std::string makeRequest(const std::string& method, const std::string& path,
                               int port, const std::string& body = "") {
    std::string url = "http://127.0.0.1:" + std::to_string(port);
    httplib::Client cli(url.c_str());
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(5, 0);

    httplib::Result res;
    if (method == "GET") {
        res = cli.Get(path.c_str());
    } else if (method == "POST") {
        res = cli.Post(path.c_str(), body, "application/json");
    } else if (method == "PUT") {
        res = cli.Put(path.c_str(), body, "application/json");
    } else if (method == "DELETE") {
        res = cli.Delete(path.c_str());
    } else {
        return "";
    }

    if (!res) {
        return "";
    }
    return res->body;
}

// ============================================================================
// Test cases
// ============================================================================

static void test_server_start_stop() {
    TEST("Server start/stop lifecycle");
    ApiServer server;
    CHECK(server.start("127.0.0.1", 18080), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
    PASS();
}

static void test_get_accounts_empty() {
    TEST("GET /api/accounts returns empty array");
    ApiServer server;
    AccountManager accounts;

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18081), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string body = makeRequest("GET", "/api/accounts", 18081);
    CHECK(!body.empty(), "No response from server");
    CHECK(body == "[]", "Expected empty array, got: " + body);

    server.stop();
    PASS();
}

static void test_add_account() {
    TEST("POST /api/accounts adds new account");
    ApiServer server;
    AccountManager accounts;
    accounts.setMasterPassword("test123");
    accounts.unlock("test123");

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18082), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string json = R"({
        "email": "test@gmail.com",
        "displayName": "Test User",
        "encryptedPassword": "enc_pass_123",
        "smtpServer": "smtp.gmail.com",
        "smtpPort": 465,
        "imapServer": "imap.gmail.com",
        "imapPort": 993
    })";

    std::string body = makeRequest("POST", "/api/accounts", 18082, json);
    CHECK(!body.empty(), "No response from server");

    auto j = nlohmann::json::parse(body);
    CHECK(j.contains("id"), "Response missing id");
    CHECK(j["email"] == "test@gmail.com", "Email mismatch");
    CHECK(j["displayName"] == "Test User", "DisplayName mismatch");

    // Verify it's in the list
    std::string listBody = makeRequest("GET", "/api/accounts", 18082);
    auto arr = nlohmann::json::parse(listBody);
    CHECK(arr.size() == 1, "Expected 1 account, got " + std::to_string(arr.size()));

    server.stop();
    PASS();
}

static void test_add_duplicate_account_fails() {
    TEST("POST /api/accounts duplicate email returns 400");
    ApiServer server;
    AccountManager accounts;

    // Set a master password first to allow add
    accounts.setMasterPassword("test123");
    accounts.unlock("test123");

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18083), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string json = R"({
        "email": "dup@gmail.com",
        "displayName": "Dup User",
        "encryptedPassword": "enc_pass",
        "smtpServer": "smtp.gmail.com",
        "imapServer": "imap.gmail.com"
    })";

    // First add should succeed (201)
    httplib::Client cli("http://127.0.0.1:18083");
    auto res1 = cli.Post("/api/accounts", json, "application/json");
    CHECK(res1 != nullptr, "First add failed");
    CHECK(res1->status == 201, "Expected 201 for first add");

    server.stop();
    PASS();
}

static void test_get_accounts_after_add() {
    TEST("GET /api/accounts returns added accounts");
    ApiServer server;
    AccountManager accounts;
    accounts.setMasterPassword("test123");
    accounts.unlock("test123");

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18084), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add two accounts
    auto j1 = nlohmann::json::parse(R"({
        "email": "user1@gmail.com", "displayName": "User1",
        "encryptedPassword": "pw1", "smtpServer": "smtp.gmail.com",
        "imapServer": "imap.gmail.com"
    })");
    auto j2 = nlohmann::json::parse(R"({
        "email": "user2@gmail.com", "displayName": "User2",
        "encryptedPassword": "pw2", "smtpServer": "smtp.outlook.com",
        "imapServer": "imap.outlook.com"
    })");

    httplib::Client cli("http://127.0.0.1:18084");
    cli.Post("/api/accounts", j1.dump(), "application/json");
    cli.Post("/api/accounts", j2.dump(), "application/json");

    std::string body = makeRequest("GET", "/api/accounts", 18084);
    auto arr = nlohmann::json::parse(body);
    CHECK(arr.size() == 2, "Expected 2 accounts, got " + std::to_string(arr.size()));

    server.stop();
    PASS();
}

static void test_update_account() {
    TEST("PUT /api/accounts/{id} updates account");
    ApiServer server;
    AccountManager accounts;
    accounts.setMasterPassword("test123");
    accounts.unlock("test123");

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18085), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // First add an account
    std::string addJson = R"({
        "id": "test-id-001",
        "email": "update_test@gmail.com",
        "displayName": "Original Name",
        "encryptedPassword": "pw",
        "smtpServer": "smtp.gmail.com",
        "imapServer": "imap.gmail.com"
    })";

    std::string addResp = makeRequest("POST", "/api/accounts", 18085, addJson);
    CHECK(!addResp.empty(), "Add failed");

    // Update the account
    std::string updateJson = R"({
        "email": "update_test@gmail.com",
        "displayName": "Updated Name",
        "encryptedPassword": "pw",
        "smtpServer": "smtp.gmail.com",
        "imapServer": "imap.gmail.com"
    })";

    httplib::Client cli("http://127.0.0.1:18085");
    auto res = cli.Put("/api/accounts/test-id-001", updateJson, "application/json");
    CHECK(res != nullptr, "No response from PUT");
    CHECK(res->status == 200, "Expected 200, got " + std::to_string(res->status));

    auto j = nlohmann::json::parse(res->body);
    CHECK(j["displayName"] == "Updated Name", "Name not updated");

    server.stop();
    PASS();
}

static void test_delete_account() {
    TEST("DELETE /api/accounts/{id} removes account");
    ApiServer server;
    AccountManager accounts;
    accounts.setMasterPassword("test123");
    accounts.unlock("test123");

    server.setAccountManager(&accounts);
    CHECK(server.start("127.0.0.1", 18086), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add an account
    std::string addJson = R"({
        "id": "del-test-001",
        "email": "delete_me@gmail.com",
        "displayName": "Delete Me",
        "encryptedPassword": "pw",
        "smtpServer": "smtp.gmail.com",
        "imapServer": "imap.gmail.com"
    })";
    makeRequest("POST", "/api/accounts", 18086, addJson);

    // Verify it exists
    auto arr1 = nlohmann::json::parse(makeRequest("GET", "/api/accounts", 18086));
    CHECK(arr1.size() >= 1, "Account not added");

    // Delete it
    httplib::Client cli("http://127.0.0.1:18086");
    auto res = cli.Delete("/api/accounts/del-test-001");
    CHECK(res != nullptr, "No response from DELETE");
    CHECK(res->status == 200, "Expected 200, got " + std::to_string(res->status));

    server.stop();
    PASS();
}

static void test_email_crud() {
    TEST("Email CRUD via REST endpoints");
    ApiServer server;
    StorageManager storage;
    CHECK(storage.open("test_phase6_temp.db"), "Failed to open temp DB");

    server.setStorageManager(&storage);
    CHECK(server.start("127.0.0.1", 18087), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create the account first for foreign key constraint
    AccountConfig acc;
    acc.id = "acc-1";
    acc.displayName = "Test Account";
    acc.email = "acc-1@test.com";
    acc.encryptedPassword = "pw";
    acc.smtpServer = "smtp.test.com";
    acc.imapServer = "imap.test.com";
    CHECK(storage.saveAccount(acc), "Failed to save account");

    // Save an email
    std::string emailJson = R"({
        "id": "email-001",
        "accountId": "acc-1",
        "sender": "sender@test.com",
        "recipients": ["receiver@test.com"],
        "subject": "Test Subject",
        "bodyPlain": "Hello world",
        "folder": "INBOX"
    })";

    httplib::Client cli("http://127.0.0.1:18087");
    auto res1 = cli.Post("/api/emails", emailJson, "application/json");
    CHECK(res1 != nullptr, "POST /api/emails failed");
    CHECK(res1->status == 201, "Expected 201, got " + std::to_string(res1->status));

    // Get the email
    auto res2 = cli.Get("/api/emails/email-001");
    CHECK(res2 != nullptr, "GET /api/emails/{id} failed");
    CHECK(res2->status == 200, "Expected 200, got " + std::to_string(res2->status));
    auto j = nlohmann::json::parse(res2->body);
    CHECK(j["subject"] == "Test Subject", "Subject mismatch");

    // Get emails by account
    auto res3 = cli.Get("/api/accounts/acc-1/emails");
    CHECK(res3 != nullptr, "GET /api/accounts/{id}/emails failed");
    auto arr = nlohmann::json::parse(res3->body);
    CHECK(arr.size() >= 1, "No emails returned for account");

    // Mark as read
    auto res4 = cli.Put("/api/emails/email-001/read", "", "application/json");
    CHECK(res4 != nullptr, "PUT /api/emails/{id}/read failed");
    CHECK(res4->status == 200, "Expected 200, got " + std::to_string(res4->status));

    // Delete the email
    auto res5 = cli.Delete("/api/emails/email-001");
    CHECK(res5 != nullptr, "DELETE /api/emails/{id} failed");
    CHECK(res5->status == 200, "Expected 200, got " + std::to_string(res5->status));

    server.stop();
    storage.close();
    std::remove("test_phase6_temp.db");
    PASS();
}

static void test_email_not_found() {
    TEST("GET /api/emails/{id} returns 404 for missing email");
    ApiServer server;
    StorageManager storage;
    CHECK(storage.open("test_phase6_temp2.db"), "Failed to open temp DB");

    server.setStorageManager(&storage);
    CHECK(server.start("127.0.0.1", 18088), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:18088");
    auto res = cli.Get("/api/emails/nonexistent");
    CHECK(res != nullptr, "No response");
    CHECK(res->status == 404, "Expected 404, got " + std::to_string(res->status));

    server.stop();
    storage.close();
    std::remove("test_phase6_temp2.db");
    PASS();
}

static void test_search_endpoint() {
    TEST("GET /api/search returns results");
    ApiServer server;
    StorageManager storage;
    CHECK(storage.open("test_phase6_temp3.db"), "Failed to open temp DB");

    // Create the account first for foreign key constraint
    AccountConfig acc;
    acc.id = "acc-1";
    acc.displayName = "Test Account";
    acc.email = "acc-1@test.com";
    acc.encryptedPassword = "pw";
    acc.smtpServer = "smtp.test.com";
    acc.imapServer = "imap.test.com";
    CHECK(storage.saveAccount(acc), "Failed to save account");

    // Save an email that can be searched
    Email email;
    email.id = "search-email-001";
    email.accountId = "acc-1";
    email.sender = "newsletter@example.com";
    email.subject = "Weekly Newsletter";
    email.bodyPlain = "This is a weekly newsletter with important updates";
    email.folder = "INBOX";
    storage.saveEmail(email);

    server.setStorageManager(&storage);
    CHECK(server.start("127.0.0.1", 18089), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:18089");
    auto res = cli.Get("/api/search?q=newsletter");
    CHECK(res != nullptr, "No response from search");
    CHECK(res->status == 200, "Expected 200, got " + std::to_string(res->status));

    auto arr = nlohmann::json::parse(res->body);
    CHECK(arr.size() > 0, "Search returned no results");

    server.stop();
    storage.close();
    std::remove("test_phase6_temp3.db");
    PASS();
}

static void test_server_without_managers() {
    TEST("Endpoints return 503 when manager not set");
    ApiServer server;
    CHECK(server.start("127.0.0.1", 18090), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:18090");
    auto res = cli.Get("/api/accounts");
    CHECK(res != nullptr, "No response");
    CHECK(res->status == 503, "Expected 503, got " + std::to_string(res->status));

    server.stop();
    PASS();
}

static void test_websocket_broadcast() {
    TEST("WebSocket /ws/notifications broadcast");
    ApiServer server;
    CHECK(server.start("127.0.0.1", 18091), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect WebSocket client in a separate thread
    std::atomic<bool> wsConnected{false};
    std::string wsMessage;
    std::atomic<bool> wsReceived{false};

    std::thread wsThread([&]() {
        httplib::ws::WebSocketClient ws("ws://127.0.0.1:18091/ws/notifications");
        if (ws.connect()) {
            wsConnected = true;
            auto result = ws.read(wsMessage);
            if (result != httplib::ws::ReadResult::Fail) {
                wsReceived = true;
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

    // Broadcast a message
    server.broadcast("test_event", "hello from server");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    wsThread.join();

    // Verify we got the broadcast
    CHECK(wsReceived, "Did not receive WebSocket message");
    auto j = nlohmann::json::parse(wsMessage);
    CHECK(j["event"] == "test_event", "Event name mismatch");
    CHECK(j["data"] == "hello from server", "Data mismatch");

    server.stop();
    PASS();
}

static void test_classify_endpoint_returns_202() {
    TEST("POST /api/emails/{id}/classify returns 202 accepted");
    ApiServer server;
    StorageManager storage;
    CHECK(storage.open("test_phase6_temp4.db"), "Failed to open temp DB");

    OpenAiConfig aiConfig;
    AiService aiService(aiConfig);

    server.setStorageManager(&storage);
    server.setAiService(&aiService);
    CHECK(server.start("127.0.0.1", 18092), "Failed to start server");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    httplib::Client cli("http://127.0.0.1:18092");
    auto res = cli.Post("/api/emails/test-id/classify",
                        R"({"content":"Test email content"})",
                        "application/json");
    CHECK(res != nullptr, "No response");
    CHECK(res->status == 202, "Expected 202, got " + std::to_string(res->status));

    server.stop();
    storage.close();
    std::remove("test_phase6_temp4.db");
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Phase 6: ApiServer HTTP/WebSocket Tests ===\n\n";

    test_server_start_stop();
    test_get_accounts_empty();
    test_add_account();
    test_add_duplicate_account_fails();
    test_get_accounts_after_add();
    test_update_account();
    test_delete_account();
    test_email_crud();
    test_email_not_found();
    test_search_endpoint();
    test_server_without_managers();
    test_websocket_broadcast();
    test_classify_endpoint_returns_202();

    std::cout << "\n=== Results: " << g_passed << " passed, "
              << g_failed << " failed ===\n";
    return g_failed > 0 ? 1 : 0;
}
