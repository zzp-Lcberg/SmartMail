#include "AiService.hpp"
#include "OpenAiClient.hpp"
#include "utils/Logger.hpp"

namespace SmartMail {

AiService::AiService(const OpenAiConfig& config) {
    client_ = std::make_unique<OpenAiClient>(config);
    workerThread_ = std::thread(&AiService::workerLoop, this);
    LOG_INFO("AiService started");
}

AiService::~AiService() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    LOG_INFO("AiService stopped");
}

void AiService::classifyAsync(const std::string& emailId,
                               const std::string& content,
                               std::function<void(ClassificationResult)> callback) {
    Task task;
    task.type = Task::Type::Classify;
    task.emailId = emailId;
    task.content = content;
    task.classifyCallback = std::move(callback);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(std::move(task));
    }
    cv_.notify_one();
}

void AiService::generateReplyAsync(const std::string& emailId,
                                    const std::string& originalEmail,
                                    const std::string& userPrompt,
                                    std::function<void(ReplyResult)> callback) {
    Task task;
    task.type = Task::Type::GenerateReply;
    task.emailId = emailId;
    task.content = originalEmail;
    task.userPrompt = userPrompt;
    task.replyCallback = std::move(callback);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(std::move(task));
    }
    cv_.notify_one();
}

void AiService::reportCorrection(const std::string& emailId,
                                  const std::string& correctTag) {
    // TODO: 记录用户修正用于优化分类 (Phase 7)
    LOG_INFO("User corrected tag for email " + emailId + " to " + correctTag);
}

bool AiService::isAvailable() const {
    return client_ != nullptr;
}

void AiService::workerLoop() {
    while (running_) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !taskQueue_.empty() || !running_; });
            if (!running_) break;
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }

        if (task.type == Task::Type::Classify) {
            ClassificationResult result = client_->classify(task.content);
            result.emailId = task.emailId;
            if (task.classifyCallback) {
                task.classifyCallback(result);
            }
        } else if (task.type == Task::Type::GenerateReply) {
            ReplyResult result = client_->generateReply(task.content, task.userPrompt);
            result.emailId = task.emailId;
            if (task.replyCallback) {
                task.replyCallback(result);
            }
        }
    }
}

} // namespace SmartMail
