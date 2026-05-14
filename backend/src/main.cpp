/// SmartMail MailCore Service - backend entry point
#include <iostream>
#include <csignal>
#include <chrono>
#include <curl/curl.h>
#include "utils/Logger.hpp"
#include "server/ApiServer.hpp"
#include "account/AccountManager.hpp"
#include "storage/StorageManager.hpp"
#include "ai/AiService.hpp"

using namespace SmartMail;

static volatile bool g_running = true;

void signalHandler(int signal) {
    LOG_INFO(std::string("Received signal ") + std::to_string(signal) + ", shutting down...");
    g_running = false;
}

int main(int argc, char* argv[]) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    Logger::instance().setLogFile("mailcore.log");
    Logger::instance().setMinLevel(Logger::Level::Debug);
    LOG_INFO("MailCore Service starting...");

    // --- Storage ---
    StorageManager storage;
    if (!storage.open("smartmail.db")) {
        LOG_ERROR("Failed to open database");
        curl_global_cleanup();
        return 1;
    }
    LOG_INFO("StorageManager initialized");

    // --- Accounts ---
    AccountManager accounts;
    // Try to load existing accounts, ignore if file doesn't exist
    accounts.loadFromFile("accounts.dat");
    LOG_INFO("AccountManager initialized");

    // --- AI Service ---
    OpenAiConfig aiConfig;
    aiConfig.apiKey = "";       // To be configured
    aiConfig.baseUrl = "https://api.openai.com/v1/chat/completions";
    aiConfig.model = "gpt-4o";
    AiService aiService(aiConfig);
    LOG_INFO("AiService initialized");

    // --- API Server ---
    ApiServer server;
    server.setAccountManager(&accounts);
    server.setStorageManager(&storage);
    server.setAiService(&aiService);

    int port = 8080;
    std::string host = "127.0.0.1";
    if (!server.start(host, port)) {
        LOG_ERROR("Failed to start API server");
        curl_global_cleanup();
        return 1;
    }

    LOG_INFO("MailCore Service initialized successfully.");

    // Main loop
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Shutdown
    LOG_INFO("Shutting down...");
    server.stop();
    accounts.saveToFile("accounts.dat");
    storage.close();
    curl_global_cleanup();
    LOG_INFO("MailCore Service stopped.");
    return 0;
}
