#include "OpenAiClient.hpp"
#include "utils/Logger.hpp"

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

std::string OpenAiClient::callApi(const std::string& systemPrompt,
                                   const std::string& userMessage) {
    // TODO: 实现 HTTP POST 到 OpenAI API (Phase 7)
    LOG_DEBUG("Calling OpenAI API...");
    return "";
}

} // namespace SmartMail
