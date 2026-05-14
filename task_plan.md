# 任务计划：SmartMail Desktop 邮件客户端开发

## 目标
完成 Phase 6 ApiServer — HTTP REST + WebSocket 服务器实现

## 当前阶段
阶段 6：ApiServer HTTP/WebSocket

## 各阶段

### 阶段 1：项目初始化 ✅
- **状态：** complete

### 阶段 2：StorageManager ✅
- **状态：** complete

### 阶段 3：SmtpClient + CryptoUtils ✅
- **状态：** complete

### 阶段 4：AccountManager ✅
- **状态：** complete

### 阶段 5：ImapClient + Pop3Client ✅
- **状态：** complete (20/20 测试通过)

### 阶段 6：ApiServer HTTP/WebSocket
- [x] 6.1 获取 httplib.h 依赖
- [x] 6.2 实现 ApiServer::start()/stop() 核心逻辑
- [x] 6.3 实现 13 个 REST API 端点 (setupRoutes)
- [x] 6.4 实现 WebSocket /ws/notifications (setupWebSocket + broadcast)
- [x] 6.5 添加 JSON 序列化支持
- [x] 6.6 更新 main.cpp 集成所有管理器
- [x] 6.7 编写 test_phase6.cpp 和 CMakeLists.txt
- [ ] 6.8 获取缺失依赖 (CURL, nlohmann/json, SQLite3 开发库)
- [ ] 6.9 编译验证、测试通过
- **状态：** in_progress (阻塞: 3个依赖包缺失)

### 阶段 7-12
- **状态：** pending

## 遇到的错误
| 错误 | 尝试次数 | 解决方案 |
|------|---------|---------|
| OpenSSL 找不到 | 2 | 创建本地 openssl-local 目录 |
| httplib.h 缺失 | 1 | 用户手动下载 |
| vcpkg安装依赖失败 | 1 | SSL连接错误，需手动下载 |
| CURL/nlohmann_json/SQLite3 缺失 | 待处理 | 需用户手动下载 |
