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
    src/storage/sqlite3.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${HTTPLIB_INCLUDE_DIR}
    ${NLOHMANN_JSON_DIR}
    ${CURL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

target_compile_definitions(MailCoreService PRIVATE CURL_STATICLIB SQLITE_ENABLE_FTS5)

target_link_libraries(MailCoreService PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ws2_32
    crypt32
    wldap32
    bcrypt
    normaliz
    iphlpapi
    Secur32
)

# --- Unit Tests (Phase 2) ---
add_executable(TestStorage
    tests/test_storage.cpp
    src/storage/StorageManager.cpp
    src/storage/sqlite3.c
)

target_include_directories(TestStorage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${SQLITE3_INCLUDE_DIR}
)

target_compile_definitions(TestStorage PRIVATE SQLITE_ENABLE_FTS5)

target_link_libraries(TestStorage PRIVATE
    SmartMailCommon
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
    ${CURL_INCLUDE_DIR}
)

target_compile_definitions(TestPhase3 PRIVATE CURL_STATICLIB)

target_link_libraries(TestPhase3 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ws2_32
    crypt32
    wldap32
    bcrypt
    normaliz
    iphlpapi
    Secur32
)

# --- Unit Tests (Phase 4) ---
add_executable(TestPhase4
    tests/test_phase4.cpp
    src/account/AccountManager.cpp
    src/account/CryptoUtils.cpp
)

target_include_directories(TestPhase4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${NLOHMANN_JSON_DIR}
    ${CURL_INCLUDE_DIR}
)

target_compile_definitions(TestPhase4 PRIVATE CURL_STATICLIB)

target_link_libraries(TestPhase4 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ws2_32
    crypt32
    wldap32
    bcrypt
    normaliz
    iphlpapi
    Secur32
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

# --- Unit Tests (Phase 6) ---
add_executable(TestPhase6
    tests/test_phase6.cpp
    src/server/ApiServer.cpp
    src/account/AccountManager.cpp
    src/account/CryptoUtils.cpp
    src/storage/StorageManager.cpp
    src/storage/sqlite3.c
    src/ai/AiService.cpp
    src/ai/OpenAiClient.cpp
)

target_include_directories(TestPhase6 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${HTTPLIB_INCLUDE_DIR}
    ${NLOHMANN_JSON_DIR}
    ${CURL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

target_compile_definitions(TestPhase6 PRIVATE CURL_STATICLIB SQLITE_ENABLE_FTS5)

target_link_libraries(TestPhase6 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ws2_32
    crypt32
    wldap32
    bcrypt
    normaliz
    iphlpapi
    Secur32
)

# --- Unit Tests (Phase 7) ---
add_executable(TestPhase7
    tests/test_phase7.cpp
    src/server/ApiServer.cpp
    src/ai/AiService.cpp
    src/ai/OpenAiClient.cpp
    src/storage/StorageManager.cpp
    src/storage/sqlite3.c
    src/account/AccountManager.cpp
    src/account/CryptoUtils.cpp
)

target_include_directories(TestPhase7 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${HTTPLIB_INCLUDE_DIR}
    ${NLOHMANN_JSON_DIR}
    ${CURL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

target_compile_definitions(TestPhase7 PRIVATE CURL_STATICLIB SQLITE_ENABLE_FTS5)

target_link_libraries(TestPhase7 PRIVATE
    SmartMailCommon
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ws2_32
    crypt32
    wldap32
    bcrypt
    normaliz
    iphlpapi
    Secur32
)
