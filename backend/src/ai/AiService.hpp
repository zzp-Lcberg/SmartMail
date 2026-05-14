#pragma once

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "types/AiResult.hpp"
#include "types/Email.hpp"

namespace SmartMail {

class OpenAiClient;

/// AI 服务管理层
class AiService {
public:
    explicit AiService(const OpenAiConfig& config);
    ~AiService();

    /// 异步分类
    void classifyAsync(const std::string& emailId, const std::string& content,
                       std::function<void(ClassificationResult)> callback);
    /// 异步生成回复
    void generateReplyAsync(const std::string& emailId,
                            const std::string& originalEmail,
                            const std::string& userPrompt,
                            std::function<void(ReplyResult)> callback);

    /// 反馈修正
    void reportCorrection(const std::string& emailId, const std::string& correctTag);

    bool isAvailable() const;

private:
    void workerLoop();
    struct Task {
        enum class Type { Classify, GenerateReply };
        Type type;
        std::string emailId;
        std::string content;
        std::string userPrompt;
        std::function<void(ClassificationResult)> classifyCallback;
        std::function<void(ReplyResult)> replyCallback;
    };

    std::unique_ptr<OpenAiClient> client_;
    std::queue<Task> taskQueue_;
    std::thread workerThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = true;
};

} // namespace SmartMail
