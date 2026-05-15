# SmartMail Desktop — 12 阶段开发计划

## 总览

| 阶段 | 内容 | 状态 | 测试结果 |
|------|------|------|---------|
| Phase 1 | 项目初始化：CMake 构建、目录结构、common 类型定义 | ✅ 完成 | — |
| Phase 2 | StorageManager：SQLite 存储、FTS5 全文索引、CRUD | ✅ 完成 | 10/10 passed |
| Phase 3 | SmtpClient + CryptoUtils：SMTP 发送、AES-256 加密 | ✅ 完成 | 13/13 passed |
| Phase 4 | AccountManager：多账号管理、密码解密注入 | ✅ 完成 | 16/16 passed |
| Phase 5 | ImapClient + Pop3Client：IMAP4rev1/POP3 协议、SSL/TLS | ✅ 完成 | 20/20 passed |
| Phase 6 | ApiServer：HTTP REST + WebSocket 服务器 | ✅ 完成 | 13/13 passed |
| Phase 7 | AI 服务集成：OpenAiClient、Classifier、ReplyGenerator | ✅ 完成 | 10/10 passed |
| Phase 8 | Qt 前端基础 UI：MainWindow、ServiceClient、WebSocketHandler、MailModel + Midnight Paper 暗色主题 | ✅ 完成 | 编译+UI 验证通过 |
| Phase 9 | Qt 前端核心视图：EmailListView、EmailDetailView、EmailComposer、AiPanel | ✅ 完成 | 82/82 passed |
| Phase 10 | Qt 前端对话框：SettingsDialog、AccountDialog、离线缓存、未读计数 | ✅ 完成 | 82/82 passed |
| Phase 11 | 搜索与性能优化：全文检索、分页加载、虚拟列表、错误处理 | ⏳ 待开发 | — |
| Phase 12 | 集成测试与打包：E2E 测试、Bug 修复、安装包、文档 | ⏳ 待开发 | — |

**总测试：82 passed, 0 failed（Phase 10 编译+回归验证通过）**

---

## Phase 1：项目初始化 ✅

- **目标**：建立 CMake 构建系统、目录结构、公共类型定义
- **产出文件**：
  - `CMakeLists.txt` — 根构建文件
  - `backend/CMakeLists.txt` — 后端构建配置
  - `common/types/Email.hpp` — 邮件数据结构
  - `common/types/AccountConfig.hpp` — 账号配置结构
  - `common/types/AiResult.hpp` — AI 结果结构
  - `common/utils/Logger.hpp` — 日志工具
  - `common/utils/Config.hpp` — 配置读写
  - `.gitignore` / `CLAUDE.md` / `docs/`

---

## Phase 2：StorageManager ✅

- **目标**：SQLite 数据库存储层，FTS5 全文索引，邮件 CRUD
- **产出文件**：
  - `backend/src/storage/StorageManager.hpp` — 存储管理器接口
  - `backend/src/storage/StorageManager.cpp` — 完整 CRUD 实现
  - `backend/src/storage/schema.sql` — 数据库建表脚本（可选）
  - `backend/tests/test_storage.cpp` — 10 个测试用例
- **数据库表**：accounts, emails, ai_tags, settings + FTS5 虚拟表
- **测试可执行文件**：`build/backend/Debug/TestStorage.exe`
- **测试结果**：10 passed, 0 failed

---

## Phase 3：SmtpClient + CryptoUtils ✅

- **目标**：SMTP 邮件发送协议，AES-256-CBC 加密/解密，PBKDF2 密钥派生
- **产出文件**：
  - `backend/src/engine/SmtpClient.hpp` — SMTP 发送实现
  - `backend/src/engine/SmtpClient.cpp` — libcurl SMTP 协议栈
  - `backend/src/account/CryptoUtils.hpp` — AES-256 加密工具
  - `backend/src/account/CryptoUtils.cpp` — 加密/解密/PBKDF2/Base64
  - `backend/tests/test_phase3.cpp` — 13 个测试用例
- **测试可执行文件**：`build/backend/Debug/TestPhase3.exe`
- **测试结果**：13 passed, 0 failed

---

## Phase 4：AccountManager ✅

- **目标**：多账号管理，密码加密存储，文件持久化（JSON）
- **产出文件**：
  - `backend/src/account/AccountManager.hpp` — 账号管理器接口
  - `backend/src/account/AccountManager.cpp` — 增删改查 + 文件读写
  - `backend/tests/test_phase4.cpp` — 16 个测试用例
- **测试可执行文件**：`build/backend/Debug/TestPhase4.exe`
- **测试结果**：16 passed, 0 failed

---

## Phase 5：ImapClient + Pop3Client ✅

- **目标**：IMAP4rev1 邮件接收、POP3 邮件接收、SSL/TLS 加密传输
- **产出文件**：
  - `backend/src/engine/ImapClient.hpp` — IMAP 客户端
  - `backend/src/engine/ImapClient.cpp` — IMAP4rev1 协议实现
  - `backend/src/engine/Pop3Client.hpp` — POP3 客户端
  - `backend/src/engine/Pop3Client.cpp` — POP3 协议实现
  - `backend/tests/test_phase5.cpp` — 20 个测试用例
- **测试可执行文件**：`build/backend/Debug/TestPhase5.exe`
- **测试结果**：20 passed, 0 failed

---

## Phase 6：ApiServer ✅

- **目标**：HTTP REST API + WebSocket 服务器（基于 cpp-httplib）
- **产出文件**：
  - `backend/src/server/ApiServer.hpp` — API 服务器接口
  - `backend/src/server/ApiServer.cpp` — 13 REST 端点 + WebSocket + broadcast
  - `backend/src/server/httplib.h` — cpp-httplib header-only 库
  - `backend/src/server/json.hpp` — nlohmann/json header-only 库
  - `backend/src/main.cpp` — 后端服务入口（集成三大管理器）
  - `backend/tests/test_phase6.cpp` — 13 个测试用例
- **REST 端点**（13 个）：accounts CRUD, emails CRUD, classify, reply, search
- **WebSocket 端点**：`/ws/notifications`（6 种事件广播）
- **测试可执行文件**：`build/backend/Debug/TestPhase6.exe`
- **测试结果**：13 passed, 0 failed

---

## Phase 7：AI 服务集成 ✅

- **目标**：OpenAI API 对接，邮件智能分类，AI 回复生成，用户反馈闭环
- **产出文件**：
  - `backend/src/ai/OpenAiClient.hpp` — +callApi/writeCallback/parseApiResponse/buildChatJson
  - `backend/src/ai/OpenAiClient.cpp` — libcurl HTTPS POST 实现 + JSON 构建/解析 + 重试
  - `backend/src/ai/AiService.hpp` — +StorageManager* 成员和 setter
  - `backend/src/ai/AiService.cpp` — reportCorrection() 调用 saveAiTag()
  - `backend/src/server/ApiServer.cpp` — classify 回调→saveAiTag, reply 回调→WebSocket broadcast
  - `backend/src/main.cpp` — +aiService.setStorageManager(&storage)
  - `backend/tests/test_phase7.cpp` — 10 个测试用例（mock OpenAI 服务器，无网络依赖）
- **测试可执行文件**：`build/backend/Debug/TestPhase7.exe`
- **测试结果**：10 passed, 0 failed

---

## Phase 8：Qt 前端基础 UI ⏳

- **目标**：主窗口布局、前后端通信封装、数据模型
- **计划产出**：
  - `frontend/src/views/MainWindow.hpp/.cpp` — 三栏布局主窗口
  - `frontend/src/client/ServiceClient.hpp/.cpp` — HTTP 请求封装
  - `frontend/src/client/WebSocketHandler.hpp/.cpp` — WebSocket 客户端
  - `frontend/src/models/MailModel.hpp/.cpp` — 邮件列表 Model（QAbstractListModel）
  - `frontend/src/models/FolderModel.hpp/.cpp` — 文件夹树 Model
- **验证**：UI 启动正常，布局正确

---

## Phase 9：Qt 前端核心视图 ⏳

- **目标**：邮件列表、详情、撰写、AI 面板
- **计划产出**：
  - `frontend/src/views/EmailListView.hpp/.cpp` — 邮件列表视图（标签筛选/排序）
  - `frontend/src/views/EmailDetailView.hpp/.cpp` — 邮件详情展示
  - `frontend/src/views/EmailComposer.hpp/.cpp` — 邮件撰写窗口（富文本/附件）
  - `frontend/src/views/AiPanel.hpp/.cpp` — AI 侧边面板（分类标签/回复草稿）
- **验证**：核心视图交互正常

---

## Phase 10：Qt 前端对话框与设置 ✅

- **目标**：设置/账号对话框、离线缓存、未读计数
- **产出文件**（修改 11 个文件）：
  - `backend/src/server/ApiServer.cpp` — 新增 POST /api/test-connection, GET/POST /api/settings
  - `frontend/src/client/ServiceClient.hpp/.cpp` — 新增 testConnection(), getSettings(), updateSettings()
  - `frontend/src/dialogs/SettingsDialog.hpp/.cpp` — 三标签页完整逻辑：账号CRUD、AI设置、通用设置
  - `frontend/src/dialogs/AccountDialog.hpp/.cpp` — 测试连接、增强验证、编辑模式、协议映射
  - `frontend/src/models/FolderModel.hpp/.cpp` — 新增 incrementUnread/decrementUnread
  - `frontend/src/views/MainWindow.hpp/.cpp` — 集成所有功能：离线模式、未读计数、账号选择、修复硬编码 accountId
- **验证**：编译通过（后端+前端），82 回归测试全部通过

---

## Phase 11：搜索与性能优化 ⏳

- **目标**：FTS5 全文检索集成、分页加载、虚拟列表、错误处理完善
- **计划产出**：
  - 前端搜索接口对接
  - 邮件分页加载（limit/offset）
  - QAbstractListModel 虚拟列表渲染
  - 错误处理完善（网络异常、AI 不可用等降级策略）
- **验证**：搜索功能测试通过，性能达标

---

## Phase 12：集成测试与打包 ⏳

- **目标**：端到端测试、Bug 修复、安装包制作、文档完善
- **计划产出**：
  - 端到端集成测试
  - Bug 修复与边界情况处理
  - Windows 安装包制作
  - `docs/` 文档完善
- **验证**：全部功能验收通过

---

## 项目信息

- **项目路径**：`C:\Users\pengzhanzhu\SmartMail\`
- **设计方案**：`C:\Users\pengzhanzhu\SmartMail-Desktop-设计方案.md`
- **GitHub**：https://github.com/zzp-Lcberg/SmartMail
- **构建目录**：`C:\Users\pengzhanzhu\SmartMail\build\`
- **测试可执行文件**：`build/backend/Debug/TestStorage.exe`, `TestPhase3.exe` ~ `TestPhase6.exe`

---

## 可用 Skills

| 阶段 | Skill | 用途 |
|------|-------|------|
| 所有阶段 | `planning-with-files-zh` | 进度跟踪与任务管理 |
| 每个阶段结束后 | `review-changes` | 代码审查与错误检测 |
| 每个阶段结束后 | `review-delta` | 增量代码审查 |
| 需要时 | `debug-issue` | 调试问题 |
| 需要时 | `refactor-safely` | 安全重构 |

---

*最后更新：2026-05-15 (Phase 10 完成)*
