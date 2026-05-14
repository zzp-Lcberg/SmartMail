set(BACKEND_SOURCES
    src/main.cpp
    src/engine/SmtpClient.cpp
    src/engine/ImapClient.cpp
    src/engine/Pop3Client.cpp
    src/account/AccountManager.cpp
    src/account/CryptoUtils.cpp
    src/ai/AiService.cpp
    src/ai/OpenAiClient.cpp
    src/storage/StorageManager.cpp
    src/server/ApiServer.cpp
)

set(BACKEND_HEADERS
    src/engine/IMailProtocol.hpp
    src/engine/SmtpClient.hpp
    src/engine/ImapClient.hpp
    src/engine/Pop3Client.hpp
    src/account/AccountManager.hpp
    src/account/CryptoUtils.hpp
    src/ai/AiService.hpp
    src/ai/OpenAiClient.hpp
    src/storage/StorageManager.hpp
    src/server/ApiServer.hpp
)

add_executable(MailCoreService ${BACKEND_SOURCES} ${BACKEND_HEADERS})

target_include_directories(MailCoreService PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${HTTPLIB_INCLUDE_DIR}
)

target_link_libraries(MailCoreService PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

# --- Unit Tests (Phase 2+) ---
add_executable(TestStorage
    tests/test_storage.cpp
    src/storage/StorageManager.cpp
)

target_include_directories(TestStorage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(TestStorage PRIVATE
    SmartMailCommon
    SQLite::SQLite3
)

# --- Unit Tests (Phase 3) ---
add_executable(TestPhase3
    tests/test_phase3.cpp
    src/account/CryptoUtils.cpp
    src/engine/SmtpClient.cpp
)

target_include_directories(TestPhase3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(TestPhase3 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
)

# --- Unit Tests (Phase 4) ---
add_executable(TestPhase4
    tests/test_phase4.cpp
    src/account/AccountManager.cpp
    src/account/CryptoUtils.cpp
)

target_include_directories(TestPhase4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(TestPhase4 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# --- Unit Tests (Phase 5) ---
add_executable(TestPhase5
    tests/test_phase5.cpp
    src/engine/ImapClient.cpp
    src/engine/Pop3Client.cpp
)

target_include_directories(TestPhase5 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(TestPhase5 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
)
