cmake_minimum_required(VERSION 3.16)
project(SmartMail VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# =============================================
# Dependencies — manual paths (vcpkg unavailable)
# =============================================
set(PROJECT_DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# OpenSSL (pre-installed locally)
set(OPENSSL_ROOT_DIR "C:/Users/pengzhanzhu/openssl-local")
find_package(OpenSSL REQUIRED)

# cpp-httplib (header-only, bundled)
set(HTTPLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/backend/src/server)

# nlohmann/json (header-only, bundled)
set(NLOHMANN_JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/backend/src/server)

# CURL (manual import library)
set(CURL_INCLUDE_DIR ${PROJECT_DEPS_DIR}/curl/include)
set(CURL_LIBRARY_DIR ${PROJECT_DEPS_DIR}/curl/lib)
set(CURL_LIBRARY ${CURL_LIBRARY_DIR}/libcurl.lib)

# SQLite3 (amalgamation, compiled from source)
set(SQLITE3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/backend/src/storage)
set(SQLITE3_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/backend/src/storage/sqlite3.c)

# common 库 (仅头文件)
add_library(SmartMailCommon INTERFACE)
target_include_directories(SmartMailCommon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_compile_features(SmartMailCommon INTERFACE cxx_std_17)

# backend
add_subdirectory(backend)

# frontend (需要 Qt6)
find_package(Qt6 COMPONENTS Widgets Network WebSockets QUIET)
if(Qt6_FOUND)
    add_subdirectory(frontend)
else()
    message(WARNING "Qt6 not found, frontend will not be built.")
endif()
