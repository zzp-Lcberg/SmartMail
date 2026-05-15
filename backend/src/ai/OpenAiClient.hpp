#pragma once

#include <string>
#include "types/AiResult.hpp"

namespace SmartMail {

/// OpenAI API 调用客户端
class OpenAiClient {
public:
    explicit OpenAiClient(const OpenAiConfig& config);

    /// AI 邮件分类
    ClassificationResult classify(const std::string& emailContent);

    /// AI 生成回复
    ReplyResult generateReply(const std::string& originalEmail,
                              const std::string& userPrompt = "");

private:
    std::string buildClassificationPrompt(const std::string& content);
    std::string buildReplyPrompt(const std::string& originalEmail,
                                 const std::string& userPrompt);
    std::string callApi(const std::string& systemPrompt,
                        const std::string& userMessage);
    std::string buildChatJson(const std::string& systemPrompt,
                              const std::string& userMessage);
    std::string parseApiResponse(const std::string& responseBody);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

    OpenAiConfig config_;
};

} // namespace SmartMail
