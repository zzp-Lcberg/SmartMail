# 发现与决策

## 需求
- Phase 6 需要将 AccountManager、StorageManager、AiService 通过 HTTP REST + WebSocket 暴露
- 前端 ServiceClient 期望 13 个 REST 端点 + /ws/notifications WebSocket
- 前端 WebSocket 期望 JSON: `{"event": "...", "data": {...}}`

## 研究发现

### ApiServer 实现完成
- `backend/src/server/ApiServer.hpp`: 已更新，include httplib.h, WebSocket连接集合, mutex
- `backend/src/server/ApiServer.cpp`: 已完整实现 (13端点 + WebSocket + broadcast)
- `backend/src/main.cpp`: 已更新集成三大管理器
- `backend/tests/test_phase6.cpp`: 15个测试用例编写完成
- `build/test_phase6/CMakeLists.txt`: 独立编译配置

### 依赖状态（2026-05-13 最终状态）
| 依赖 | 状态 | 说明 |
|------|------|------|
| OpenSSL | ✅ | C:/Users/pengzhanzhu/openssl-local/ |
| httplib.h | ✅ | backend/src/server/httplib.h (20143行) |
| CURL 开发库 | ❌ | 需要 curl.h + libcurl.lib |
| nlohmann/json | ❌ | 需要 json.hpp (header-only, 单文件) |
| SQLite3 开发库 | ❌ | 需要 sqlite3.h + sqlite3.lib |

### vcpkg 状态
- vcpkg 已安装: C:/Users/pengzhanzhu/vcpkg/vcpkg.exe (2026-04-08版)
- 但用不了：需要CMake 4.3.2，下载时SSL连接失败（国内网络问题）
- 错误信息: "curl operation failed with error code 35 (SSL connect error)"

### 手动下载地址
- nlohmann/json: https://github.com/nlohmann/json/releases/latest (单文件 json.hpp)
- SQLite3: https://www.sqlite.org/download.html (sqlite-amalgamation-*.zip)
- CURL: https://curl.se/windows/ (Windows 64-bit 开发包)

## 技术决策
| 决策 | 理由 |
|------|------|
| httplib.h 放到 backend/src/server/ | 隔离依赖，避免全局路径配置 |
| 直接用 nlohmann/json 手工构造 JSON | 避免修改 common 类型定义（影响面小） |
| WebSocket 连接用 mutex+set 管理 | httplib内置支持，简单 |
| 独立 CMakeLists.txt 测试 | 绕过主项目缺失依赖 |

## 资源
- cpp-httplib: https://github.com/yhirose/cpp-httplib
- nlohmann/json: https://github.com/nlohmann/json
- SQLite3: https://www.sqlite.org
- CURL: https://curl.se

---
*每执行2次查看/浏览器/搜索操作后更新此文件*
