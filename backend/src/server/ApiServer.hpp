#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <set>

#include "httplib.h"
#include <nlohmann/json.hpp>

namespace SmartMail {

class AccountManager;
class StorageManager;
class AiService;

/// HTTP + WebSocket server wrapping cpp-httplib
class ApiServer {
public:
    ApiServer();
    ~ApiServer();

    bool start(const std::string& host = "127.0.0.1", int port = 8080);
    void stop();

    void setAccountManager(AccountManager* mgr) { accountManager_ = mgr; }
    void setStorageManager(StorageManager* mgr) { storageManager_ = mgr; }
    void setAiService(AiService* svc) { aiService_ = svc; }

    // WebSocket broadcast to all connected clients
    void broadcast(const std::string& event, const std::string& data);

private:
    void setupRoutes();
    void setupWebSocket();

    // Helpers: serialize common types to JSON
    static nlohmann::json accountToJson(const class AccountConfig& acc);
    static nlohmann::json emailToJson(const class Email& email);

    AccountManager* accountManager_ = nullptr;
    StorageManager* storageManager_ = nullptr;
    AiService* aiService_ = nullptr;

    std::unique_ptr<httplib::Server> httpServer_;
    std::unique_ptr<std::thread> serverThread_;
    std::atomic<bool> running_{false};
    int port_ = 8080;
    std::string host_ = "127.0.0.1";

    // WebSocket connection tracking for broadcast
    std::mutex wsMutex_;
    std::set<httplib::ws::WebSocket*> wsClients_;
};

} // namespace SmartMail
