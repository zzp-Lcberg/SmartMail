#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>

namespace SmartMail {

/// 简易配置文件读写工具 (INI 格式)
class Config {
public:
    explicit Config(const std::string& filepath) : filepath_(filepath) {
        load();
    }

    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data_.find(key);
        return it != data_.end() ? it->second : defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try { return std::stoi(it->second); }
            catch (...) { return defaultValue; }
        }
        return defaultValue;
    }

    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            std::string v = it->second;
            return v == "true" || v == "1" || v == "yes";
        }
        return defaultValue;
    }

    void set(const std::string& key, const std::string& value) {
        data_[key] = value;
    }

    void setInt(const std::string& key, int value) {
        data_[key] = std::to_string(value);
    }

    void setBool(const std::string& key, bool value) {
        data_[key] = value ? "true" : "false";
    }

    bool save() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream file(filepath_);
        if (!file.is_open()) return false;
        for (const auto& [key, value] : data_) {
            file << key << "=" << value << "\n";
        }
        return true;
    }

    void reload() { load(); }

private:
    void load() {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
        std::ifstream file(filepath_);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = trim(line.substr(0, pos));
                std::string value = trim(line.substr(pos + 1));
                data_[key] = value;
            }
        }
    }

    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::string filepath_;
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mutex_;
};

} // namespace SmartMail
