#include "OpenAiClient.hpp"
#include "utils/Logger.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

namespace SmartMail {

OpenAiClient::OpenAiClient(const OpenAiConfig& config)
    : config_(config) {
    LOG_DEBUG("OpenAiClient created");
}

ClassificationResult OpenAiClient::classify(const std::string& emailContent) {
    // TODO: 调用 OpenAI API 进行邮件分类 (Phase 7)
    LOG_DEBUG("AI classifying email...");

    ClassificationResult result;
    result.emailId = "";
    result.success = false;

    std::string prompt = buildClassificationPrompt(emailContent);
    std::string response = callApi(
        "你是一个邮件分类助手。分析邮件内容，将其归类为：工作、个人、账单、通知、社交、重要。仅返回标签名称。",
        prompt);

    // 验证返回结果
    if (!response.empty() && AiTags::isValid(response)) {
        result.success = true;
        result.tag = response;
    }

    return result;
}

ReplyResult OpenAiClient::generateReply(const std::string& originalEmail,
                                         const std::string& userPrompt) {
    // TODO: 调用 OpenAI API 生成回复 (Phase 7)
    LOG_DEBUG("AI generating reply...");

    ReplyResult result;
    result.emailId = "";
    result.success = false;

    std::string prompt = buildReplyPrompt(originalEmail, userPrompt);
    std::string response = callApi(
        "你是一个专业的邮件助手。请生成专业、礼貌、简洁的邮件回复。",
        prompt);

    if (!response.empty()) {
        result.success = true;
        result.replyContent = response;
    }

    return result;
}

std::string OpenAiClient::buildClassificationPrompt(const std::string& content) {
    return "请将以下邮件内容分类：\n\n" + content;
}

std::string OpenAiClient::buildReplyPrompt(const std::string& originalEmail,
                                            const std::string& userPrompt) {
    std::string prompt = "原始邮件：\n" + originalEmail + "\n\n";
    if (!userPrompt.empty()) {
        prompt += "用户要求：" + userPrompt + "\n";
    }
    prompt += "请生成合适的回复内容。";
    return prompt;
}

static std::string maskKey(const std::string& key) {
    if (key.length() <= 8) return "***";
    return key.substr(0, 3) + "..." + key.substr(key.length() - 4);
}

size_t OpenAiClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string OpenAiClient::buildChatJson(const std::string& systemPrompt,
                                         const std::string& userMessage) {
    nlohmann::json body;
    body["model"] = config_.model;
    nlohmann::json msgSystem;
    msgSystem["role"] = "system";
    msgSystem["content"] = systemPrompt;
    nlohmann::json msgUser;
    msgUser["role"] = "user";
    msgUser["content"] = userMessage;
    body["messages"] = nlohmann::json::array({msgSystem, msgUser});
    return body.dump();
}

std::string OpenAiClient::parseApiResponse(const std::string& responseBody) {
    try {
        auto j = nlohmann::json::parse(responseBody);
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            auto& choice = j["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                std::string content = choice["message"]["content"];
                size_t start = content.find_first_not_of(" \t\n\r");
                size_t end = content.find_last_not_of(" \t\n\r");
                if (start == std::string::npos) return "";
                return content.substr(start, end - start + 1);
            }
        }
        if (j.contains("error") && j["error"].contains("message")) {
            LOG_ERROR("OpenAI API error: " + j["error"]["message"].get<std::string>());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse OpenAI API response: " + std::string(e.what()));
    }
    return "";
}

std::string OpenAiClient::callApi(const std::string& systemPrompt,
                                   const std::string& userMessage) {
    if (config_.apiKey.empty()) {
        LOG_WARN("OpenAI API key is empty, cannot make API call");
        return "";
    }

    std::string jsonBody = buildChatJson(systemPrompt, userMessage);
    LOG_DEBUG("Calling OpenAI API...");

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl handle");
        return "";
    }

    std::string responseBody;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + config_.apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, config_.baseUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(config_.timeoutSeconds));

    std::string result;
    CURLcode res = CURLE_OK;
    long httpCode = 0;

    for (int attempt = 0; attempt <= config_.maxRetries; ++attempt) {
        responseBody.clear();
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode == 200) {
                result = parseApiResponse(responseBody);
                LOG_DEBUG("OpenAI API call succeeded");
                break;
            } else if (httpCode == 401) {
                LOG_ERROR("OpenAI API authentication failed (401) - check API key: " + maskKey(config_.apiKey));
                break;  // Don't retry auth failures
            } else if (httpCode == 429) {
                LOG_WARN("OpenAI API rate limited (429), attempt " + std::to_string(attempt + 1));
                if (attempt < config_.maxRetries) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            } else {
                LOG_ERROR("OpenAI API returned HTTP " + std::to_string(httpCode));
                if (!responseBody.empty()) {
                    parseApiResponse(responseBody);  // Log error details
                }
                break;
            }
        } else {
            LOG_ERROR(std::string("curl error: ") + curl_easy_strerror(res));
            if (attempt < config_.maxRetries) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

} // namespace SmartMail
