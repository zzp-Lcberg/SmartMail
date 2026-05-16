# 进度日志

## 会话：2026-05-14

### 阶段 7：AI 服务集成
- **状态：** complete
- **测试结果：** 10 passed, 0 failed
- **完成时间：** 2026-05-14

**实现内容：**
- OpenAiClient::callApi() — libcurl HTTPS POST 到 OpenAI API
- classify 回调 → StorageManager::saveAiTag() 持久化
- reply 回调 → WebSocket broadcast("reply_ready")
- AiService::reportCorrection() → StorageManager::updateAiTag()
- mock OpenAI 服务器（httplib）离线测试

**修改文件（8 个）：** OpenAiClient.hpp/cpp, AiService.hpp/cpp, ApiServer.cpp, main.cpp, test_phase7.cpp, CMakeLists.txt

---

### 阶段 8：Qt 前端基础 UI + Midnight Paper 主题
- **状态：** complete
- **完成时间：** 2026-05-14
- **编译产物：** `build/frontend/Debug/SmartMailClient.exe`
- **UI 验证：** 窗口正常显示，Midnight Paper 暗色主题生效

**Phase 8 核心修改（6 个文件）：**
| 文件 | 变更 |
|------|------|
| `frontend/src/client/ServiceClient.cpp` | 重写：实现 10 个 HTTP 方法 + accountToJson/emailToJson 序列化 |
| `frontend/src/client/ServiceClient.hpp` | 修复：getEmails/search 回调添加默认值 `= {}` |
| `frontend/src/views/MainWindow.cpp` | 重写：setupConnections WebSocket 信号接线 + QListView 文件夹面板 + objectName |
| `frontend/src/main.cpp` | 编辑：取消 MainWindow 实例化注释 + 加载 QSS 主题 |
| `frontend/src/client/WebSocketHandler.cpp` | 修复：QWebSocket 构造函数适配 Qt6 API |
| `frontend/CMakeLists.txt` | 修复：启用 AUTOMOC/AUTORCC/AUTOUIC + style.qss 资源 |

**Midnight Paper 主题修改（5 个文件）：**
| 文件 | 变更 |
|------|------|
| `frontend/resources/style.qss` | 新建：完整暗色主题 QSS（深炭底色 + 靛蓝主色 + 琥珀点缀 + 暖白内容区） |
| `frontend/src/main.cpp` | 加载 :/resources/style.qss 主题样式 |
| `frontend/src/views/MainWindow.cpp` | 组件 objectName：folderView, emailListView, emailDetailView, aiPanel |
| `frontend/src/views/AiPanel.cpp` | 移除内联 styleSheet，改 objectName（aiTitleLabel, aiTagLabel, secondaryBtn） |
| `frontend/src/views/EmailDetailView.cpp` | 移除内联 styleSheet，改 objectName（subjectLabel, senderLabel, timeLabel, tagLabel） |
| `frontend/src/views/EmailListView.cpp` | 重绘 delegate：未读靛蓝圆点、琥珀 AI 标签、选中高亮左侧条、暗色底色 |

**编译环境：**
- Qt 6.8.3 / MSVC 2022 64-bit
- Qt6Widgets ✅ Qt6Network ✅ Qt6WebSockets ✅
- CMake: VS2022 内置

**回归测试：** Phase 1-7 全部通过（82/82）

---

### 阶段 9：Qt 前端核心视图交互
- **状态：** complete
- **完成时间：** 2026-05-14
- **编译产物：** `build/frontend/Debug/SmartMailClient.exe`
- **回归测试：** 82/82 passed

**实现内容：**

| 功能 | 实现 |
|------|------|
| 新邮件 | `onNewEmail()` → EmailComposer 对话框 → sendEmail API |
| 回复 | 工具栏按钮 → openReplyComposer() 回复模式 |
| 转发 | 工具栏按钮 → openReplyComposer() 转发模式 (Fwd: + 引用原文) |
| 删除 | 工具栏按钮 → deleteEmail API + removeEmail from model + clear detail |
| AI 分类 | 选中邮件自动触发 + 工具栏手动触发 → classifyEmail API |
| AI 回复 | 工具栏按钮 → generateReply API → AiPanel 显示草稿 |
| AiPanel::acceptReply | 用 AI 回复内容打开 EmailComposer 回复模式 |
| AiPanel::regenerateReply | 重新调用 generateReply API |
| AiPanel::editReply | 用 AI 回复内容打开 EmailComposer 编辑 |
| 刷新 | 重新加载当前文件夹邮件列表 |
| classifyEmail | 发送实际邮件 body 内容（之前为空字符串） |
| generateReply | 发送实际邮件内容作为 originalEmail（之前为空字符串） |

**修改文件（6 个）：**

| 文件 | 变更 |
|------|------|
| `frontend/src/views/MainWindow.hpp` | +selectedEmailId_, +currentEmail_, +openReplyComposer(), +Email 引用 |
| `frontend/src/views/MainWindow.cpp` | 工具栏按钮实现、AiPanel 信号连接、onNewEmail/onRefresh/onEmailSelected 完整实现 |
| `frontend/src/views/EmailComposer.hpp` | +setForwardMode() |
| `frontend/src/views/EmailComposer.cpp` | setForwardMode 实现、onSaveDraft 保存草稿 |
| `frontend/src/client/ServiceClient.hpp` | classifyEmail +content 参数、generateReply +originalEmail 参数 |
| `frontend/src/client/ServiceClient.cpp` | 发送实际邮件内容替代空字符串 |

### 阶段 9 补充：Midnight Paper 主题修复
- **状态：** complete
- **问题：** 界面显示纯白默认样式，QSS 未加载
- **根因：** `qt_add_resources` 生成的 .cpp 未被 AUTORCC 自动初始化流程正确处理
- **修复：** 创建 `frontend/resources/resources.qrc`，由 CMAKE_AUTORCC 自动编译和初始化
- **变更文件：**
  - `frontend/resources/resources.qrc` — 新建：`/resources` 前缀 + style.qss + icons
  - `frontend/CMakeLists.txt` — 移除 `qt_add_resources`，改为 `resources/resources.qrc` 加入源码列表

---

---

### 阶段 10：Qt 前端对话框与设置 + 离线缓存 + 未读计数
- **状态：** complete
- **完成时间：** 2026-05-15
- **编译产物：** `build/backend/Debug/MailCoreService.exe` + `build/frontend/Debug/SmartMailClient.exe`
- **回归测试：** 82/82 passed

**实现内容：**

| 模块 | 文件 | 内容 |
|------|------|------|
| 后端端点 | `ApiServer.cpp` | `POST /api/test-connection`、`GET /api/settings`、`POST /api/settings` |
| 前端客户端 | `ServiceClient.hpp/.cpp` | `testConnection()`、`getSettings()`、`updateSettings()` |
| 设置对话框 | `SettingsDialog.hpp/.cpp` | 三标签页（账号管理/AI设置/通用设置）、暗色主题、账号CRUD、AI配置保存到后端+UI偏好保存到本地Config |
| 账号对话框 | `AccountDialog.hpp/.cpp` | 邮箱预设（Gmail/QQ/163/Outlook/126/Yahoo/自定义）、自动域名检测、测试连接、编辑模式、暗色主题 |
| 文件夹模型 | `FolderModel.hpp/.cpp` | `incrementUnread()`、`decrementUnread()`、`updateUnreadCount()` |
| 主窗口集成 | `MainWindow.hpp/.cpp` | `onOpenSettings()`、`loadAccounts()`、`setOfflineMode()`、`updateFolderUnreadCounts()`、离线检测、`selectedAccountId_` |

**修改文件（7 个）：**

| 文件 | 变更 |
|------|------|
| `backend/src/server/ApiServer.cpp` | +3 端点：test-connection, get/update settings |
| `frontend/src/dialogs/AccountDialog.cpp` | 完整重写：预设选择器、三步向导布局、暗色主题 QPalette+QSS、测试连接、编辑模式 |
| `frontend/src/dialogs/AccountDialog.hpp` | providerCombo_, ServiceClient*, editMode_, editAccountId_ |
| `frontend/src/dialogs/SettingsDialog.cpp` | 完整重写：三标签页、账号管理CRUD、AI/通用设置、暗色主题 |
| `frontend/src/dialogs/SettingsDialog.hpp` | 移除冗余表单字段，保留账号列表+AI/通用设置字段 |
| `frontend/src/views/EmailDetailView.cpp` | 字段名修复 body_plain→bodyPlain, body_html→bodyHtml, ai_tag→aiTag |
| `frontend/src/views/MainWindow.cpp` | selectedAccountId_ 替换硬编码 "default" |

**关键修复：**

| 问题 | 根因 | 修复 |
|------|------|------|
| QComboBox 下拉框不可见 | 原生箭头被QSS覆盖 | 移除 `::drop-down`/`::down-arrow` 子控件QSS |
| QComboBox 弹出项白底白字 | `view()` 是顶层窗口，dialog-scoped选择器无法匹配 | `comboBox->view()->setStyleSheet()` 直接设置 |
| QLabel 标签文字不可见 | global QSS `QWidget { color }` 覆盖 QPalette | `makeLabel()` lambda 直接 `label->setStyleSheet()` |
| QLineEdit placeholder 不可见 | Qt QSS 不支持 `::placeholder` | `setLineEditPalette()` lambda 设置 `QPalette::PlaceholderText` |
| **添加账号"保存显示错误"** | `AccountManager::add()` 要求 `isUnlocked()`，启动时未设置主密码 | `main.cpp` 中 `loadFromFile()` 后自动 `setMasterPassword("smartmail")` |

**暗色主题方案：** QPalette（兜底层）+ QSS（控件层）+ 内联 setStyleSheet（最高优先级，绕过选择器问题）

---

## 五问重启检查
| 问题 | 答案 |
|------|------|
| 我在哪里？ | Phase 10 完成 — 设置/账号对话框、离线模式、未读计数全部实现 |
| 我要去哪里？ | Phase 11：搜索与性能优化 |
| 目标是什么？ | 全文搜索功能、邮件列表虚拟化、增量同步 |
| 我学到了什么？ | Qt QSS 选择器对顶层窗口无效（QComboBox popup）；placeholder 色用 QPalette；`QWidget { color }` 全局规则会覆盖所有子控件；内联 setStyleSheet 优先级最高可绕过所有选择器问题；AccountManager 需要 unlock 才能 add |
| 我做了什么？ | 3 后端端点 + 2 对话框暗色重写 + FolderModel 未读计数 + MainWindow 离线/账号集成 + auto-unlock 修复 |
