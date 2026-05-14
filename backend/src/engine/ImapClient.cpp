#include "ImapClient.hpp"
#include "utils/Logger.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKERR() WSAGetLastError()
    static bool g_wsaInit = false;
    static void ensureWsa() {
        if (!g_wsaInit) {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2,2), &wsa);
            g_wsaInit = true;
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
    #define SOCKERR() errno
#endif

#include <cstring>
#include <sstream>
#include <algorithm>
#include <vector>

namespace SmartMail {

// ============================================================================
// 构造 / 析构
// ============================================================================

ImapClient::ImapClient() {
#ifdef _WIN32
    ensureWsa();
#endif
    LOG_DEBUG("ImapClient created");
}

ImapClient::~ImapClient() {
    disconnect();
}

// ============================================================================
// 网络层
// ============================================================================

bool ImapClient::createSocket(const std::string& host, int port) {
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

    // 设置超时
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

void ImapClient::closeSocket() {
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

bool ImapClient::sslHandshake() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD* method = TLS_client_method();
    sslCtx_ = SSL_CTX_new(method);
    if (!sslCtx_) {
        LOG_ERROR("SSL_CTX_new failed");
        return false;
    }

    // 宽松验证（生产环境应启用）
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

int ImapClient::sendLine(const std::string& line) {
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

std::string ImapClient::readLine() {
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
            line.resize(line.size() - 2); // 去掉 \r\n
            return line;
        }
    }
    return line;
}

std::string ImapClient::readUntilTag(const std::string& tag) {
    std::string result;
    while (true) {
        std::string line = readLine();
        if (line.empty()) break;
        result += line + "\r\n";
        // 以 tag 开头的行是完成信号
        if (line.compare(0, tag.size(), tag) == 0) {
            // 检查是否是 tag OK/BAD/NO
            if (line.find(tag + " OK") != std::string::npos ||
                line.find(tag + " BAD") != std::string::npos ||
                line.find(tag + " NO") != std::string::npos) {
                break;
            }
        }
    }
    return result;
}

// ============================================================================
// 协议层
// ============================================================================

std::string ImapClient::nextTag() {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "A%04d", ++tagCounter_);
    return buf;
}

std::string ImapClient::sendCommand(const std::string& cmd) {
    std::string tag = nextTag();
    std::string fullCmd = tag + " " + cmd;
    LOG_DEBUG("IMAP CMD: " + fullCmd);
    if (sendLine(fullCmd) < 0) {
        LOG_ERROR("sendCommand: failed to send");
        return "";
    }
    return readUntilTag(tag);
}

// ============================================================================
// 公共接口
// ============================================================================

bool ImapClient::connect(const AccountConfig& config) {
    config_ = config;

    if (config.imapServer.empty()) {
        LOG_ERROR("IMAP server not configured");
        return false;
    }

    int port = config.imapPort > 0 ? config.imapPort : (config.imapUseSSL ? 993 : 143);

    if (!createSocket(config.imapServer, port)) return false;

    if (config.imapUseSSL) {
        if (!sslHandshake()) {
            closeSocket();
            return false;
        }
    }

    // 读取服务器 greeting
    std::string greeting = readLine();
    if (greeting.empty() || greeting.substr(0, 4) != "* OK") {
        LOG_ERROR("Bad IMAP greeting: " + greeting);
        closeSocket();
        return false;
    }
    LOG_DEBUG("IMAP greeting: " + greeting);

    // 登录
    if (!login()) {
        closeSocket();
        return false;
    }

    connected_ = true;
    LOG_INFO("IMAP connected to " + config.imapServer);
    return true;
}

void ImapClient::disconnect() {
    if (connected_ && ssl_) {
        sendLine(nextTag() + " LOGOUT");
    }
    closeSocket();
    connected_ = false;
    tagCounter_ = 0;
    selectedFolder_.clear();
    LOG_DEBUG("IMAP disconnected");
}

bool ImapClient::isConnected() const {
    return connected_;
}

void ImapClient::setAuthPassword(const std::string& password) {
    authPassword_ = password;
}

bool ImapClient::login() {
    const std::string& pass = !authPassword_.empty() ? authPassword_ : config_.encryptedPassword;
    std::string cmd = "LOGIN " + config_.email + " " + pass;
    std::string resp = sendCommand(cmd);
    if (resp.empty() || resp.find("OK LOGIN") == std::string::npos) {
        LOG_ERROR("IMAP LOGIN failed");
        return false;
    }
    LOG_INFO("IMAP LOGIN ok");
    return true;
}

std::vector<Folder> ImapClient::listFolders() {
    std::vector<Folder> result;
    if (!connected_) return result;

    std::string resp = sendCommand("LIST \"\" \"*\"");
    if (resp.empty()) return result;

    std::istringstream ss(resp);
    std::string line;
    while (std::getline(ss, line)) {
        // 删除末尾 \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // 格式: * LIST (...) "/" "FolderName"
        if (line.find("* LIST") != std::string::npos) {
            result.push_back(parseListLine(line));
        }
    }
    return result;
}

bool ImapClient::selectFolder(const std::string& folderPath) {
    if (!connected_) return false;

    std::string cmd = "SELECT \"" + folderPath + "\"";
    std::string resp = sendCommand(cmd);
    if (resp.empty() || resp.find("OK SELECT") == std::string::npos
                     && resp.find("OK [READ-WRITE]") == std::string::npos
                     && resp.find("OK [READ-ONLY]") == std::string::npos) {
        LOG_ERROR("IMAP SELECT failed for " + folderPath);
        return false;
    }

    selectedFolder_ = folderPath;
    LOG_INFO("IMAP selected folder: " + folderPath);
    return true;
}

std::vector<Email> ImapClient::fetchEmails(int limit) {
    std::vector<Email> result;
    if (!connected_) return result;

    // 确保选中文件夹
    if (selectedFolder_.empty()) {
        selectFolder("INBOX");
    }

    // 用 UID FETCH 获取邮件
    std::string cmd = "UID FETCH 1:* (FLAGS INTERNALDATE UID RFC822.SIZE ENVELOPE BODY.PEEK[TEXT])";
    std::string resp = sendCommand(cmd);
    if (resp.empty()) return result;

    // 解析每个 FETCH 响应
    // 格式: * <seq> FETCH (FLAGS (...) UID <uid> ENVELOPE (...) BODY[TEXT] {...}\r\n<body>)
    std::istringstream ss(resp);
    std::string accumulated;
    std::string line;
    int count = 0;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.find("* ") == 0 && line.find(" FETCH ") != std::string::npos) {
            // 保存上一个邮件的累积数据
            if (!accumulated.empty() && count < limit) {
                result.push_back(parseFetchResponse(accumulated));
                count++;
            }
            accumulated = line;
        } else if (!accumulated.empty()) {
            accumulated += "\r\n" + line;
        }
    }

    // 最后一个邮件
    if (!accumulated.empty() && count < limit) {
        result.push_back(parseFetchResponse(accumulated));
        count++;
    }

    LOG_INFO("IMAP fetched " + std::to_string(result.size()) + " emails from " + selectedFolder_);
    return result;
}

bool ImapClient::sendEmail(const Email& /*email*/) {
    return false; // IMAP 不负责发送
}

bool ImapClient::markAsRead(const std::string& emailId) {
    if (!connected_) return false;

    // 用 UID STORE 设置 \Seen 标志
    std::string cmd = "UID STORE " + emailId + " +FLAGS (\\Seen)";
    std::string resp = sendCommand(cmd);

    if (resp.empty()) return false;
    bool ok = resp.find("OK STORE") != std::string::npos ||
              resp.find("OK UID STORE") != std::string::npos;
    if (ok) LOG_DEBUG("IMAP marked as read: " + emailId);
    return ok;
}

bool ImapClient::deleteEmail(const std::string& emailId) {
    if (!connected_) return false;

    // 软删除：标记为 \Deleted 然后 expunge
    std::string storeCmd = "UID STORE " + emailId + " +FLAGS (\\Deleted)";
    std::string resp = sendCommand(storeCmd);
    if (resp.empty()) return false;

    // expunge 清除标记为删除的邮件
    std::string expCmd = "EXPUNGE";
    sendCommand(expCmd);

    LOG_DEBUG("IMAP deleted: " + emailId);
    return true;
}

// ============================================================================
// 解析层
// ============================================================================

Email ImapClient::parseFetchResponse(const std::string& response) {
    Email email;
    email.folder = selectedFolder_.empty() ? "INBOX" : selectedFolder_;

    // 解析 UID: ... UID <uid>
    size_t uidPos = response.find("UID ");
    if (uidPos != std::string::npos) {
        uidPos += 4;
        while (uidPos < response.size() && response[uidPos] == ' ') uidPos++;
        std::string uid;
        while (uidPos < response.size() && std::isdigit(static_cast<unsigned char>(response[uidPos]))) {
            uid += response[uidPos++];
        }
        email.id = uid;
    }

    // 解析 FLAGS: ... FLAGS (\Seen ...)
    size_t flagsPos = response.find("FLAGS (");
    if (flagsPos != std::string::npos) {
        flagsPos += 7;
        std::string flags;
        while (flagsPos < response.size() && response[flagsPos] != ')') {
            flags += response[flagsPos++];
        }
        email.isRead = flags.find("\\Seen") != std::string::npos;
        email.isStarred = flags.find("\\Flagged") != std::string::npos;
    }

    // 解析 INTERNALDATE: ... INTERNALDATE "dd-Mon-yyyy ..."
    size_t datePos = response.find("INTERNALDATE \"");
    if (datePos != std::string::npos) {
        datePos += 14;
        std::string dateStr;
        while (datePos < response.size() && response[datePos] != '\"') {
            dateStr += response[datePos++];
        }
        // 简化：仅取时间戳部分；完整解析留待 Phase 12
        email.receivedAt = 0;
    }

    // 解析 ENVELOPE: 简化处理 — 提取 subject 和 sender
    size_t envPos = response.find("ENVELOPE (");
    if (envPos != std::string::npos) {
        size_t pos = envPos + 10;

        // Skip date
        pos = response.find('(', pos);
        if (pos == std::string::npos) return email;

        // Extract subject (2nd top-level parenthesized group)
        int depth = 0;
        size_t subjectStart = std::string::npos;
        size_t subjectEnd = std::string::npos;
        int groupCount = 0;
        bool inString = false;

        for (size_t i = pos; i < response.size(); ++i) {
            char c = response[i];
            if (c == '\"') inString = !inString;
            if (inString) continue;
            if (c == '(') {
                depth++;
                if (depth == 1) groupCount++;
            } else if (c == ')') {
                depth--;
                if (depth == 0) break;
            }
            if (depth == 1 && groupCount == 2 && subjectStart == std::string::npos) {
                if (c == '\"') subjectStart = i + 1;
            } else if (depth == 1 && groupCount == 2 && subjectStart != std::string::npos && subjectEnd == std::string::npos) {
                if (c == '\"') subjectEnd = i;
            }
        }

        if (subjectStart != std::string::npos && subjectEnd != std::string::npos) {
            email.subject = response.substr(subjectStart, subjectEnd - subjectStart);
        }

        // 提取 sender — 第3个顶层括号组
        size_t senderGroupStart = std::string::npos;
        depth = 0;
        groupCount = 0;
        for (size_t i = pos; i < response.size(); ++i) {
            char c = response[i];
            if (c == '\"') inString = !inString;
            if (inString) continue;
            if (c == '(') {
                depth++;
                if (depth == 1) {
                    groupCount++;
                    if (groupCount == 3) senderGroupStart = i;
                }
            } else if (c == ')') {
                depth--;
                if (groupCount == 3 && depth == 1 && senderGroupStart != std::string::npos) {
                    // 提取 sender 中的邮箱
                    std::string senderGroup = response.substr(senderGroupStart, i - senderGroupStart + 1);
                    // 查找 "..."@"..." 模式
                    size_t atPos = senderGroup.find('@');
                    if (atPos != std::string::npos) {
                        // 向前找邮箱开头
                        size_t addrStart = senderGroup.rfind('\"', atPos);
                        if (addrStart != std::string::npos) addrStart++;
                        else addrStart = 0;
                        // 向后找邮箱结尾
                        size_t addrEnd = senderGroup.find('\"', atPos);
                        if (addrEnd != std::string::npos) {
                            email.sender = senderGroup.substr(addrStart, addrEnd - addrStart);
                        }
                    }
                    break;
                }
                if (depth == 0) break;
            }
        }
    }

    // 解析 BODY[TEXT]: ... BODY[TEXT] {size}\r\n<body content>
    size_t bodyPos = response.find("BODY[TEXT] ");
    if (bodyPos == std::string::npos) {
        bodyPos = response.find("BODY[TEXT]<");
    }
    if (bodyPos != std::string::npos) {
        // 跳过 "BODY[TEXT] " 或 "BODY[TEXT]<...>"
        bodyPos = response.find("{", bodyPos);
        if (bodyPos != std::string::npos) {
            bodyPos++;
            std::string sizeStr;
            while (bodyPos < response.size() && std::isdigit(static_cast<unsigned char>(response[bodyPos]))) {
                sizeStr += response[bodyPos++];
            }
            // 跳过 "}\r\n"
            if (bodyPos < response.size() && response[bodyPos] == '}') bodyPos++;
            if (bodyPos < response.size() && response[bodyPos] == '\r') bodyPos++;
            if (bodyPos < response.size() && response[bodyPos] == '\n') bodyPos++;

            if (bodyPos < response.size() && !sizeStr.empty()) {
                email.bodyPlain = response.substr(bodyPos);
            }
        }
    }

    return email;
}

Folder ImapClient::parseListLine(const std::string& line) {
    Folder folder;
    // 格式: * LIST (\HasNoChildren) "/" "FolderName"
    // 或:    * LIST () "." "FolderName"

    // 提取属性
    size_t attrStart = line.find("(");
    size_t attrEnd = line.find(")");
    if (attrStart != std::string::npos && attrEnd != std::string::npos) {
        std::string attrs = line.substr(attrStart + 1, attrEnd - attrStart - 1);
        // 暂时不用属性
    }

    // 提取文件夹名 (最后一个引号对)
    size_t lastQuote = line.rfind('\"');
    size_t secondLastQuote = std::string::npos;
    if (lastQuote != std::string::npos && lastQuote > 0) {
        secondLastQuote = line.rfind('\"', lastQuote - 1);
    }
    if (secondLastQuote != std::string::npos && secondLastQuote < lastQuote) {
        std::string name = line.substr(secondLastQuote + 1, lastQuote - secondLastQuote - 1);
        folder.name = decodeUtf7(name);
        folder.path = name;
    }

    return folder;
}

std::string ImapClient::decodeUtf7(const std::string& input) {
    // IMAP UTF-7 最小解码：处理 &...- 编码段
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '&') {
            size_t end = input.find('-', i + 1);
            if (end != std::string::npos) {
                // 跳过编码段（简化：直接跳过，Phase 12 完整实现）
                i = end;
                continue;
            }
        }
        result += input[i];
    }
    // 将 "&" (转义的 &) 还原
    size_t pos = 0;
    while ((pos = result.find("&-", pos)) != std::string::npos) {
        result.replace(pos, 2, "&");
    }
    return result;
}

} // namespace SmartMail
