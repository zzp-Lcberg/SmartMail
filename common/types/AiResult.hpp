#pragma once

#include <string>
#include <optional>

namespace SmartMail {

/// AI 分类结果
struct ClassificationResult {
    std::string emailId;       // 邮件 ID
    std::string tag;           // 分类标签: 工作/个人/账单/通知/社交/重要
    bool success = false;      // 是否成功
    std::string errorMessage;  // 错误信息 (如有)
};

/// AI 回复生成结果
struct ReplyResult {
    std::string emailId;       // 原始邮件 ID
    std::string replyContent;  // 生成的回复草稿
    bool success = false;      // 是否成功
    std::string errorMessage;  // 错误信息 (如有)
};

/// AI 分类标签常量
namespace AiTags {
    constexpr const char* WORK      = "工作";
    constexpr const char* PERSONAL  = "个人";
    constexpr const char* BILL      = "账单";
    constexpr const char* NOTIFICATION = "通知";
    constexpr const char* SOCIAL    = "社交";
    constexpr const char* IMPORTANT = "重要";

    /// 验证标签是否有效
    inline bool isValid(const std::string& tag) {
        return tag == WORK || tag == PERSONAL || tag == BILL
            || tag == NOTIFICATION || tag == SOCIAL || tag == IMPORTANT;
    }
}

/// OpenAI API 配置
struct OpenAiConfig {
    std::string apiKey;
    std::string baseUrl = "https://api.openai.com/v1/chat/completions";
    std::string model = "gpt-4o";
    int timeoutSeconds = 30;
    int maxRetries = 2;
};

} // namespace SmartMail
