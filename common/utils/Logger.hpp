#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace SmartMail {

/// 简易日志工具
class Logger {
public:
    enum class Level { Debug, Info, Warning, Error };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) file_.close();
        file_.open(path, std::ios::out | std::ios::app);
    }

    void setMinLevel(Level level) { minLevel_ = level; }

    void log(Level level, const std::string& message,
             const char* file = nullptr, int line = 0) {
        if (level < minLevel_) return;
        std::lock_guard<std::mutex> lock(mutex_);

        std::string entry = format(level, message, file, line);
        if (file_.is_open()) {
            file_ << entry << std::endl;
        } else {
            // 未配置文件则输出到标准输出
            printf("%s\n", entry.c_str());
        }
    }

    void debug(const std::string& msg, const char* file = nullptr, int line = 0) {
        log(Level::Debug, msg, file, line);
    }
    void info(const std::string& msg, const char* file = nullptr, int line = 0) {
        log(Level::Info, msg, file, line);
    }
    void warn(const std::string& msg, const char* file = nullptr, int line = 0) {
        log(Level::Warning, msg, file, line);
    }
    void error(const std::string& msg, const char* file = nullptr, int line = 0) {
        log(Level::Error, msg, file, line);
    }

private:
    Logger() = default;

    std::string format(Level level, const std::string& message,
                       const char* file, int line) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms.count()
            << " [" << levelStr(level) << "] ";

        if (file) {
            std::string fname = std::filesystem::path(file).filename().string();
            oss << fname << ":" << line << " ";
        }

        oss << message;
        return oss.str();
    }

    static const char* levelStr(Level level) {
        switch (level) {
            case Level::Debug:   return "DEBUG";
            case Level::Info:    return "INFO ";
            case Level::Warning: return "WARN ";
            case Level::Error:   return "ERROR";
        }
        return "????";
    }

    std::ofstream file_;
    std::mutex mutex_;
    Level minLevel_ = Level::Debug;
};

// 便捷宏
#define LOG_DEBUG(msg) SmartMail::Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  SmartMail::Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  SmartMail::Logger::instance().warn(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) SmartMail::Logger::instance().error(msg, __FILE__, __LINE__)

} // namespace SmartMail
