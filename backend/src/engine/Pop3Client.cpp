#include "Pop3Client.hpp"
#include "utils/Logger.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(s) closesocket(s)
    static bool g_wsaInitPop3 = false;
    static void ensureWsa() {
        if (!g_wsaInitPop3) {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2,2), &wsa);
            g_wsaInitPop3 = true;
        }
    }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <cstring>
    #define CLOSE_SOCKET(s) close(s)
#endif

#include <cstring>
#include <sstream>
#include <algorithm>

namespace SmartMail {

// ============================================================================
// 构造 / 析构
// ============================================================================

Pop3Client::Pop3Client() {
#ifdef _WIN32
    ensureWsa();
#endif
    LOG_DEBUG("Pop3Client created");
}

Pop3Client::~Pop3Client() {
    disconnect();
}

// ============================================================================
// 网络层
// ============================================================================

bool Pop3Client::createSocket(const std::string& host, int port) {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        LOG_ERROR("getaddrinfo failed for " + host);
        return false;
    }

    sockfd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd_ < 0) {
        LOG_ERROR("socket() failed");
        freeaddrinfo(res);
        return false;
    }

    struct timeval tv{30, 0};
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    if (::connect(sockfd_, res->ai_addr, static_cast<int>(res->ai_addrlen)) < 0) {
        LOG_ERROR("connect() failed to " + host + ":" + portStr);
        CLOSE_SOCKET(sockfd_);
        sockfd_ = -1;
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    LOG_DEBUG("TCP connected to " + host + ":" + portStr);
    return true;
}

void Pop3Client::closeSocket() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    if (sslCtx_) {
        SSL_CTX_free(sslCtx_);
        sslCtx_ = nullptr;
    }
    if (sockfd_ >= 0) {
        CLOSE_SOCKET(sockfd_);
        sockfd_ = -1;
    }
}

bool Pop3Client::sslHandshake() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD* method = TLS_client_method();
    sslCtx_ = SSL_CTX_new(method);
    if (!sslCtx_) {
        LOG_ERROR("SSL_CTX_new failed");
        return false;
    }

    SSL_CTX_set_verify(sslCtx_, SSL_VERIFY_NONE, nullptr);

    ssl_ = SSL_new(sslCtx_);
    if (!ssl_) {
        LOG_ERROR("SSL_new failed");
        return false;
    }

    SSL_set_fd(ssl_, sockfd_);
    if (SSL_connect(ssl_) != 1) {
        LOG_ERROR("SSL_connect failed");
        return false;
    }

    LOG_DEBUG("SSL handshake completed");
    return true;
}

int Pop3Client::sendLine(const std::string& line) {
    std::string data = line + "\r\n";
    int total = 0;
    while (total < static_cast<int>(data.size())) {
        int n = SSL_write(ssl_, data.data() + total,
                          static_cast<int>(data.size()) - total);
        if (n <= 0) {
            int err = SSL_get_error(ssl_, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            LOG_ERROR("SSL_write failed");
            return -1;
        }
        total += n;
    }
    return total;
}

std::string Pop3Client::readLine() {
    std::string line;
    char c;
    while (true) {
        int n = SSL_read(ssl_, &c, 1);
        if (n <= 0) {
            int err = SSL_get_error(ssl_, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            break;
        }
        line += c;
        if (line.size() >= 2 &&
            line[line.size()-2] == '\r' && line[line.size()-1] == '\n') {
            line.resize(line.size() - 2);
            return line;
        }
    }
    return line;
}

// 读取多行响应直到以 "." 开头的行
std::string Pop3Client::readMultiLine() {
    std::string result;
    while (true) {
        std::string line = readLine();
        if (line == ".") break;          // POP3 终止符
        if (line.substr(0, 2) == "..")   // 字节填充转义
            line = line.substr(1);
        result += line + "\r\n";
    }
    return result;
}

// ============================================================================
// 协议层
// ============================================================================

std::string Pop3Client::sendCommand(const std::string& cmd) {
    LOG_DEBUG("POP3 CMD: " + (cmd.size() > 20 ? cmd.substr(0, 20) + "..." : cmd));
    if (sendLine(cmd) < 0) return "";
    return readLine();
}

void Pop3Client::setAuthPassword(const std::string& password) {
    authPassword_ = password;
}

bool Pop3Client::login() {
    std::string resp = sendCommand("USER " + config_.email);
    if (resp.empty() || resp.substr(0, 3) != "+OK") {
        LOG_ERROR("POP3 USER failed: " + resp);
        return false;
    }

    const std::string& pass = !authPassword_.empty() ? authPassword_ : config_.encryptedPassword;
    resp = sendCommand("PASS " + pass);
    if (resp.empty() || resp.substr(0, 3) != "+OK") {
        LOG_ERROR("POP3 PASS failed");
        return false;
    }

    LOG_INFO("POP3 login ok");
    return true;
}

// ============================================================================
// 公共接口
// ============================================================================

bool Pop3Client::connect(const AccountConfig& config) {
    config_ = config;

    if (config.pop3Server.empty()) {
        LOG_ERROR("POP3 server not configured");
        return false;
    }

    int port = config.pop3Port > 0 ? config.pop3Port : (config.pop3UseSSL ? 995 : 110);

    if (!createSocket(config.pop3Server, port)) return false;

    if (config.pop3UseSSL) {
        if (!sslHandshake()) {
            closeSocket();
            return false;
        }
    }

    // 读取 greeting
    std::string greeting = readLine();
    if (greeting.empty() || greeting.substr(0, 3) != "+OK") {
        LOG_ERROR("Bad POP3 greeting: " + greeting);
        closeSocket();
        return false;
    }
    LOG_DEBUG("POP3 greeting: " + greeting);

    // 登录
    if (!login()) {
        closeSocket();
        return false;
    }

    connected_ = true;
    LOG_INFO("POP3 connected to " + config.pop3Server);
    return true;
}

void Pop3Client::disconnect() {
    if (connected_ && ssl_) {
        sendLine("QUIT");
    }
    closeSocket();
    connected_ = false;
    LOG_DEBUG("POP3 disconnected");
}

bool Pop3Client::isConnected() const {
    return connected_;
}

std::vector<Email> Pop3Client::fetchEmails(int limit) {
    std::vector<Email> result;
    if (!connected_) return result;

    // STAT: 获取邮件统计
    std::string statResp = sendCommand("STAT");
    if (statResp.empty() || statResp.substr(0, 3) != "+OK") {
        LOG_ERROR("POP3 STAT failed");
        return result;
    }

    // 解析邮件总数
    // 格式: +OK <count> <totalSize>
    std::istringstream statSs(statResp.substr(4));
    int totalCount = 0;
    statSs >> totalCount;
    if (totalCount <= 0) return result;

    // LIST: 获取邮件大小列表
    std::string listResp = sendCommand("LIST");
    if (listResp.empty() || listResp.substr(0, 3) != "+OK") {
        LOG_ERROR("POP3 LIST failed");
        return result;
    }
    std::string listBody = readMultiLine();

    // 解析 LIST 响应获取所有邮件编号
    std::vector<int> msgNums;
    std::istringstream listSs(listBody);
    std::string listLine;
    while (std::getline(listSs, listLine)) {
        if (!listLine.empty() && listLine.back() == '\r') listLine.pop_back();
        if (!listLine.empty() && std::isdigit(static_cast<unsigned char>(listLine[0]))) {
            std::istringstream numSs(listLine);
            int num = 0;
            numSs >> num;
            if (num > 0) msgNums.push_back(num);
        }
    }

    // RETR: 逐封获取邮件
    int fetched = 0;
    // 从最新的开始获取
    for (auto it = msgNums.rbegin(); it != msgNums.rend() && fetched < limit; ++it) {
        std::string retrCmd = "RETR " + std::to_string(*it);
        std::string retrResp = sendCommand(retrCmd);
        if (retrResp.empty() || retrResp.substr(0, 3) != "+OK") {
            LOG_ERROR("POP3 RETR " + std::to_string(*it) + " failed");
            continue;
        }

        std::string message = readMultiLine();
        if (!message.empty()) {
            Email email = parseRetrResponse(message);
            email.id = std::to_string(*it);
            email.folder = "INBOX";
            result.push_back(std::move(email));
            fetched++;
        }
    }

    LOG_INFO("POP3 fetched " + std::to_string(fetched) + " emails");
    return result;
}

bool Pop3Client::sendEmail(const Email& /*email*/) {
    return false; // POP3 不负责发送
}

bool Pop3Client::markAsRead(const std::string& /*emailId*/) {
    // POP3 不支持标记已读
    return false;
}

bool Pop3Client::deleteEmail(const std::string& emailId) {
    if (!connected_) return false;

    std::string cmd = "DELE " + emailId;
    std::string resp = sendCommand(cmd);
    if (resp.empty() || resp.substr(0, 3) != "+OK") {
        LOG_ERROR("POP3 DELE failed for " + emailId);
        return false;
    }

    LOG_DEBUG("POP3 deleted: " + emailId);
    return true;
}

// ============================================================================
// 解析层
// ============================================================================

Email Pop3Client::parseRetrResponse(const std::string& response) {
    Email email;

    // 解析 RFC 2822 邮件头
    std::istringstream ss(response);
    std::string line;
    bool inHeaders = true;
    std::string body;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (inHeaders) {
            if (line.empty()) {
                inHeaders = false; // 空行分隔头和正文
                continue;
            }

            // 解析头字段
            if (line.find("From:") == 0 || line.find("From ") == 0) {
                std::string header = line.substr(line.find(':'));
                size_t colonPos = header.find(':');
                if (colonPos != std::string::npos) {
                    email.sender = header.substr(colonPos + 1);
                    // 去除前导空格
                    auto start = email.sender.find_first_not_of(" \t");
                    if (start != std::string::npos) email.sender = email.sender.substr(start);
                    else email.sender.clear();
                }
            } else if (line.find("Subject:") == 0 || line.find("Subject ") == 0) {
                std::string header = line.substr(line.find(':'));
                size_t colonPos = header.find(':');
                if (colonPos != std::string::npos) {
                    email.subject = header.substr(colonPos + 1);
                    auto start = email.subject.find_first_not_of(" \t");
                    if (start != std::string::npos) email.subject = email.subject.substr(start);
                    else email.subject.clear();
                }
            } else if (line.find("Date:") == 0 || line.find("Date ") == 0) {
                std::string header = line.substr(line.find(':'));
                size_t colonPos = header.find(':');
                if (colonPos != std::string::npos) {
                    std::string dateStr = header.substr(colonPos + 1);
                    auto start = dateStr.find_first_not_of(" \t");
                    if (start != std::string::npos) dateStr = dateStr.substr(start);
                    // 简化时间戳 — Phase 12 完善
                    email.receivedAt = 0;
                }
            } else if (line.find("To:") == 0 || line.find("To ") == 0) {
                std::string header = line.substr(line.find(':'));
                size_t colonPos = header.find(':');
                if (colonPos != std::string::npos) {
                    std::string to = header.substr(colonPos + 1);
                    auto start = to.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        to = to.substr(start);
                        email.recipients.push_back(to);
                    }
                }
            }
        } else {
            // 邮件正文
            if (!body.empty()) body += "\n";
            body += line;
        }
    }

    email.bodyPlain = body;
    return email;
}

} // namespace SmartMail
