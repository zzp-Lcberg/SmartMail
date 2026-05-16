#include "ApiServer.hpp"
#include "account/AccountManager.hpp"
#include "storage/StorageManager.hpp"
#include "ai/AiService.hpp"
#include "types/AccountConfig.hpp"
#include "types/Email.hpp"
#include "types/AiResult.hpp"
#include "utils/Logger.hpp"
#include <random>
#include <sstream>

namespace SmartMail {

// ============================================================================
// JSON helpers
// ============================================================================

static std::string generateId() {
    static std::mt19937_64 rng(std::random_device{}());
    static thread_local char buf[32];
    uint64_t a = rng();
    uint64_t b = rng();
    std::snprintf(buf, sizeof(buf), "%016llx%016llx",
                  static_cast<unsigned long long>(a),
                  static_cast<unsigned long long>(b));
    return buf;
}

nlohmann::json ApiServer::accountToJson(const AccountConfig& acc) {
    nlohmann::json j;
    j["id"] = acc.id;
    j["displayName"] = acc.displayName;
    j["email"] = acc.email;
    j["smtpServer"] = acc.smtpServer;
    j["smtpPort"] = acc.smtpPort;
    j["smtpUseSSL"] = acc.smtpUseSSL;
    j["imapServer"] = acc.imapServer;
    j["imapPort"] = acc.imapPort;
    j["imapUseSSL"] = acc.imapUseSSL;
    j["pop3Server"] = acc.pop3Server;
    j["pop3Port"] = acc.pop3Port;
    j["pop3UseSSL"] = acc.pop3UseSSL;
    j["preferredProtocol"] = (acc.preferredProtocol == AccountConfig::Protocol::IMAP) ? "IMAP" : "POP3";
    j["syncInterval"] = acc.syncInterval;
    j["maxFetchCount"] = acc.maxFetchCount;
    j["autoClassify"] = acc.autoClassify;
    return j;
}

nlohmann::json ApiServer::emailToJson(const Email& email) {
    nlohmann::json j;
    j["id"] = email.id;
    j["accountId"] = email.accountId;
    j["sender"] = email.sender;
    j["recipients"] = email.recipients;
    j["cc"] = email.cc;
    j["bcc"] = email.bcc;
    j["subject"] = email.subject;
    j["bodyPlain"] = email.bodyPlain;
    j["bodyHtml"] = email.bodyHtml;
    j["receivedAt"] = email.receivedAt;
    j["sentAt"] = email.sentAt;
    j["isRead"] = email.isRead;
    j["isStarred"] = email.isStarred;
    j["folder"] = email.folder;
    j["attachments"] = email.attachments;
    if (email.aiTag.has_value()) {
        j["aiTag"] = email.aiTag.value();
    } else {
        j["aiTag"] = nullptr;
    }
    return j;
}

static AccountConfig parseAccountFromJson(const nlohmann::json& j) {
    AccountConfig acc;
    acc.id = j.value("id", "");
    acc.displayName = j.value("displayName", "");
    acc.email = j.value("email", "");
    acc.encryptedPassword = j.value("encryptedPassword", "");
    acc.smtpServer = j.value("smtpServer", "");
    acc.smtpPort = j.value("smtpPort", 465);
    acc.smtpUseSSL = j.value("smtpUseSSL", true);
    acc.imapServer = j.value("imapServer", "");
    acc.imapPort = j.value("imapPort", 993);
    acc.imapUseSSL = j.value("imapUseSSL", true);
    acc.pop3Server = j.value("pop3Server", "");
    acc.pop3Port = j.value("pop3Port", 995);
    acc.pop3UseSSL = j.value("pop3UseSSL", true);
    if (j.contains("preferredProtocol")) {
        acc.preferredProtocol = (j["preferredProtocol"] == "POP3")
            ? AccountConfig::Protocol::POP3
            : AccountConfig::Protocol::IMAP;
    }
    acc.syncInterval = j.value("syncInterval", 300);
    acc.maxFetchCount = j.value("maxFetchCount", 50);
    acc.autoClassify = j.value("autoClassify", true);
    return acc;
}

static Email parseEmailFromJson(const nlohmann::json& j) {
    Email email;
    email.id = j.value("id", generateId());
    email.accountId = j.value("accountId", "");
    email.sender = j.value("sender", "");
    email.recipients = j.value("recipients", std::vector<std::string>{});
    email.cc = j.value("cc", "");
    email.bcc = j.value("bcc", "");
    email.subject = j.value("subject", "");
    email.bodyPlain = j.value("bodyPlain", "");
    email.bodyHtml = j.value("bodyHtml", "");
    email.receivedAt = j.value("receivedAt", static_cast<int64_t>(0));
    email.sentAt = j.value("sentAt", static_cast<int64_t>(0));
    email.isRead = j.value("isRead", false);
    email.isStarred = j.value("isStarred", false);
    email.folder = j.value("folder", "INBOX");
    email.attachments = j.value("attachments", std::vector<std::string>{});
    if (j.contains("aiTag") && !j["aiTag"].is_null()) {
        email.aiTag = j["aiTag"].get<std::string>();
    }
    return email;
}

// ============================================================================
// ApiServer lifecycle
// ============================================================================

ApiServer::ApiServer() {
    LOG_DEBUG("ApiServer created");
}

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::start(const std::string& host, int port) {
    if (running_) {
        LOG_WARN("ApiServer already running");
        return false;
    }
    host_ = host;
    port_ = port;

    httpServer_ = std::make_unique<httplib::Server>();
    setupRoutes();
    setupWebSocket();

    running_ = true;
    serverThread_ = std::make_unique<std::thread>([this]() {
        LOG_INFO("ApiServer listening on " + host_ + ":" + std::to_string(port_));
        httpServer_->listen(host_.c_str(), port_);
    });

    // Brief wait for the server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    LOG_INFO("ApiServer started successfully");
    return true;
}

void ApiServer::stop() {
    if (!running_) return;
    running_ = false;

    // Close all WebSocket connections
    {
        std::lock_guard<std::mutex> lock(wsMutex_);
        for (auto* ws : wsClients_) {
            if (ws->is_open()) {
                ws->close();
            }
        }
        wsClients_.clear();
    }

    if (httpServer_) {
        httpServer_->stop();
    }
    if (serverThread_ && serverThread_->joinable()) {
        serverThread_->join();
    }
    httpServer_.reset();
    LOG_INFO("ApiServer stopped");
}

// ============================================================================
// WebSocket
// ============================================================================

void ApiServer::setupWebSocket() {
    httpServer_->WebSocket("/ws/notifications", [this](const httplib::Request& /*req*/, httplib::ws::WebSocket& ws) {
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsClients_.insert(&ws);
        }
        LOG_DEBUG("WebSocket client connected");

        std::string msg;
        while (ws.is_open()) {
            auto result = ws.read(msg);
            if (result == httplib::ws::ReadResult::Fail) {
                break;
            }
        }

        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsClients_.erase(&ws);
        }
        LOG_DEBUG("WebSocket client disconnected");
    });
}

void ApiServer::broadcast(const std::string& event, const std::string& data) {
    nlohmann::json msg;
    msg["event"] = event;
    msg["data"] = data;
    std::string payload = msg.dump();

    std::lock_guard<std::mutex> lock(wsMutex_);
    for (auto* ws : wsClients_) {
        if (ws->is_open()) {
            ws->send(payload);
        }
    }
}

// ============================================================================
// REST Routes
// ============================================================================

void ApiServer::setupRoutes() {
    // --- Master password ---

    // POST /api/unlock
    httpServer_->Post("/api/unlock", [this](const httplib::Request& req, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string password = j.value("password", "");
            if (password.empty()) {
                res.status = 400;
                res.set_content(R"({"error":"Password is required"})", "application/json");
                return;
            }
            bool ok;
            if (accountManager_->isUnlocked()) {
                ok = accountManager_->verifyMasterPassword(password);
            } else {
                accountManager_->setMasterPassword(password);
                ok = true;
            }
            nlohmann::json result;
            result["success"] = ok;
            result["message"] = ok ? "Unlocked" : "Invalid password";
            res.set_content(result.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json err;
            err["error"] = std::string("Invalid JSON: ") + e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    // --- Account endpoints ---

    // GET /api/accounts
    httpServer_->Get("/api/accounts", [this](const httplib::Request& /*req*/, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        auto accounts = accountManager_->getAll();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& acc : accounts) {
            arr.push_back(accountToJson(acc));
        }
        res.set_content(arr.dump(), "application/json");
    });

    // POST /api/accounts
    httpServer_->Post("/api/accounts", [this](const httplib::Request& req, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        try {
            auto j = nlohmann::json::parse(req.body);
            AccountConfig acc = parseAccountFromJson(j);
            if (acc.id.empty()) {
                acc.id = generateId();
            }
            if (!accountManager_->add(acc)) {
                res.status = 400;
                res.set_content(R"({"error":"Failed to add account"})", "application/json");
                return;
            }
            // 同步到 StorageManager 的 SQLite accounts 表
            if (storageManager_) {
                storageManager_->saveAccount(acc);
            }
            res.status = 201;
            res.set_content(accountToJson(acc).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json err;
            err["error"] = std::string("Invalid JSON: ") + e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    // PUT /api/accounts/{id}
    httpServer_->Put(R"(/api/accounts/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        try {
            auto j = nlohmann::json::parse(req.body);
            AccountConfig acc = parseAccountFromJson(j);
            acc.id = id;
            if (!accountManager_->update(acc)) {
                res.status = 404;
                res.set_content(R"({"error":"Account not found"})", "application/json");
                return;
            }
            if (storageManager_) {
                storageManager_->saveAccount(acc);
            }
            res.set_content(accountToJson(acc).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json err;
            err["error"] = std::string("Invalid JSON: ") + e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    // DELETE /api/accounts/{id}
    httpServer_->Delete(R"(/api/accounts/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        if (!accountManager_->remove(id)) {
            res.status = 404;
            res.set_content(R"({"error":"Account not found"})", "application/json");
            return;
        }
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // POST /api/test-connection
    httpServer_->Post("/api/test-connection", [this](const httplib::Request& req, httplib::Response& res) {
        if (!accountManager_) {
            res.status = 503;
            res.set_content(R"({"error":"AccountManager not available"})", "application/json");
            return;
        }
        try {
            auto j = nlohmann::json::parse(req.body);
            AccountConfig acc = parseAccountFromJson(j);
            bool ok = accountManager_->testConnection(acc);
            nlohmann::json result;
            result["success"] = ok;
            result["message"] = ok ? "SMTP server reachable" : "Connection failed";
            res.set_content(result.dump(), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json result;
            result["success"] = false;
            result["message"] = std::string("Invalid request: ") + e.what();
            res.set_content(result.dump(), "application/json");
        }
    });

    // GET /api/settings
    httpServer_->Get("/api/settings", [this](const httplib::Request& /*req*/, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        nlohmann::json j;
        j["api_key"] = storageManager_->getSetting("api_key", "");
        j["base_url"] = storageManager_->getSetting("base_url", "https://api.openai.com/v1/chat/completions");
        j["model"] = storageManager_->getSetting("model", "gpt-4o");
        j["auto_classify"] = storageManager_->getSetting("auto_classify", "true");
        j["sync_interval"] = storageManager_->getSetting("sync_interval", "300");
        res.set_content(j.dump(), "application/json");
    });

    // POST /api/settings
    httpServer_->Post("/api/settings", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        try {
            auto j = nlohmann::json::parse(req.body);
            for (auto& [key, value] : j.items()) {
                if (value.is_string()) {
                    storageManager_->setSetting(key, value.get<std::string>());
                }
            }
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json err;
            err["error"] = std::string("Invalid JSON: ") + e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    // GET /api/accounts/{id}/emails
    httpServer_->Get(R"(/api/accounts/([^/]+)/emails)", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string accountId = req.matches[1];
        std::string folder = "INBOX";
        int limit = 50;
        int offset = 0;
        if (req.has_param("folder")) folder = req.get_param_value("folder");
        if (req.has_param("limit")) limit = std::stoi(req.get_param_value("limit"));
        if (req.has_param("offset")) offset = std::stoi(req.get_param_value("offset"));

        auto emails = storageManager_->getEmails(accountId, folder, limit, offset);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& email : emails) {
            arr.push_back(emailToJson(email));
        }
        res.set_content(arr.dump(), "application/json");
    });

    // --- Email endpoints ---

    // GET /api/emails/{id}
    httpServer_->Get(R"(/api/emails/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        auto email = storageManager_->getEmailById(id);
        if (!email) {
            res.status = 404;
            res.set_content(R"({"error":"Email not found"})", "application/json");
            return;
        }
        res.set_content(emailToJson(*email).dump(), "application/json");
    });

    // POST /api/emails
    httpServer_->Post("/api/emails", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        try {
            auto j = nlohmann::json::parse(req.body);
            Email email = parseEmailFromJson(j);
            if (email.id.empty()) {
                email.id = generateId();
            }
            if (!storageManager_->saveEmail(email)) {
                res.status = 500;
                res.set_content(R"({"error":"Failed to save email"})", "application/json");
                return;
            }
            res.status = 201;
            res.set_content(emailToJson(email).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json err;
            err["error"] = std::string("Invalid JSON: ") + e.what();
            res.set_content(err.dump(), "application/json");
        }
    });

    // PUT /api/emails/{id}/read
    httpServer_->Put(R"(/api/emails/([^/]+)/read)", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        if (!storageManager_->markAsRead(id)) {
            res.status = 404;
            res.set_content(R"({"error":"Email not found"})", "application/json");
            return;
        }
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // DELETE /api/emails/{id}
    httpServer_->Delete(R"(/api/emails/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        if (!storageManager_->deleteEmail(id)) {
            res.status = 404;
            res.set_content(R"({"error":"Email not found"})", "application/json");
            return;
        }
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // POST /api/emails/{id}/classify
    httpServer_->Post(R"(/api/emails/([^/]+)/classify)", [this](const httplib::Request& req, httplib::Response& res) {
        if (!aiService_) {
            res.status = 503;
            res.set_content(R"({"error":"AiService not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        std::string content;
        try {
            auto j = nlohmann::json::parse(req.body);
            content = j.value("content", "");
        } catch (...) {
            content = req.body;
        }
        aiService_->classifyAsync(id, content, [this, id](const ClassificationResult& result) {
            if (result.success && storageManager_) {
                storageManager_->saveAiTag(id, result.tag);
                LOG_INFO("AI classification saved: email=" + id + " tag=" + result.tag);
            } else if (!result.success) {
                LOG_WARN("AI classification failed for email " + id + ": " + result.errorMessage);
            }
        });
        res.status = 202;
        res.set_content(R"({"status":"accepted"})", "application/json");
    });

    // GET /api/emails/{id}/classify
    httpServer_->Get(R"(/api/emails/([^/]+)/classify)", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        std::string tag = storageManager_->getAiTag(id);
        nlohmann::json j;
        j["emailId"] = id;
        j["tag"] = tag.empty() ? nullptr : nlohmann::json(tag);
        res.set_content(j.dump(), "application/json");
    });

    // POST /api/emails/{id}/reply
    httpServer_->Post(R"(/api/emails/([^/]+)/reply)", [this](const httplib::Request& req, httplib::Response& res) {
        if (!aiService_) {
            res.status = 503;
            res.set_content(R"({"error":"AiService not available"})", "application/json");
            return;
        }
        std::string id = req.matches[1];
        std::string originalEmail;
        std::string userPrompt;
        try {
            auto j = nlohmann::json::parse(req.body);
            originalEmail = j.value("originalEmail", "");
            userPrompt = j.value("userPrompt", "");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid JSON body"})", "application/json");
            return;
        }
        aiService_->generateReplyAsync(id, originalEmail, userPrompt, [this, id](const ReplyResult& result) {
            nlohmann::json j;
            j["emailId"] = id;
            if (result.success) {
                j["replyContent"] = result.replyContent;
                broadcast("reply_ready", j.dump());
                LOG_INFO("AI reply generated and broadcast for email " + id);
            } else {
                j["error"] = result.errorMessage;
                broadcast("reply_error", j.dump());
                LOG_WARN("AI reply failed for email " + id + ": " + result.errorMessage);
            }
        });
        res.status = 202;
        res.set_content(R"({"status":"accepted"})", "application/json");
    });

    // --- Search ---

    // GET /api/search
    httpServer_->Get("/api/search", [this](const httplib::Request& req, httplib::Response& res) {
        if (!storageManager_) {
            res.status = 503;
            res.set_content(R"({"error":"StorageManager not available"})", "application/json");
            return;
        }
        std::string query = req.has_param("q") ? req.get_param_value("q") : "";
        std::string tag = req.has_param("tag") ? req.get_param_value("tag") : "";
        int64_t fromDate = req.has_param("from") ? std::stoll(req.get_param_value("from")) : 0;
        int64_t toDate = req.has_param("to") ? std::stoll(req.get_param_value("to")) : 0;
        int limit = req.has_param("limit") ? std::stoi(req.get_param_value("limit")) : 50;

        auto emails = storageManager_->search(query, tag, fromDate, toDate, limit);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& email : emails) {
            arr.push_back(emailToJson(email));
        }
        res.set_content(arr.dump(), "application/json");
    });

    LOG_DEBUG("API routes registered (17 endpoints)");
}

} // namespace SmartMail
