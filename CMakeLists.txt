cmake_minimum_required(VERSION 3.16)
project(SmartMail VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find_package
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

# cpp-httplib (header-only, 通过 vcpkg 或手动引入)
find_path(HTTPLIB_INCLUDE_DIR httplib.h)
if(NOT HTTPLIB_INCLUDE_DIR)
    message(FATAL_ERROR "cpp-httplib not found. Please install via vcpkg or provide the include path.")
endif()

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
