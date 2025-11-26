# RegisLex - Enterprise Legal Software Suite
# Cross-platform build configuration
cmake_minimum_required(VERSION 3.16)
project(RegisLex VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(REGISLEX_BUILD_TESTS "Build unit tests" ON)
option(REGISLEX_BUILD_SHARED "Build shared library" ON)
option(REGISLEX_ENABLE_SSL "Enable SSL/TLS support" ON)
option(REGISLEX_ENABLE_JSON "Enable JSON support" ON)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(REGISLEX_PLATFORM_WINDOWS ON)
    add_definitions(-DREGISLEX_PLATFORM_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(REGISLEX_PLATFORM_LINUX ON)
    add_definitions(-DREGISLEX_PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(REGISLEX_PLATFORM_MACOS ON)
    add_definitions(-DREGISLEX_PLATFORM_MACOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(REGISLEX_PLATFORM_ANDROID ON)
    add_definitions(-DREGISLEX_PLATFORM_ANDROID)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Find required packages
find_package(Threads REQUIRED)

# SQLite (bundled or system)
option(REGISLEX_USE_SYSTEM_SQLITE "Use system SQLite instead of bundled" OFF)
if(REGISLEX_USE_SYSTEM_SQLITE)
    find_package(SQLite3 REQUIRED)
else()
    add_subdirectory(third_party/sqlite)
endif()

# OpenSSL for encryption (optional)
if(REGISLEX_ENABLE_SSL)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        add_definitions(-DREGISLEX_HAS_OPENSSL)
    else()
        message(WARNING "OpenSSL not found, encryption features will be limited")
    endif()
endif()

# Source files - Core
set(CORE_SOURCES
    src/core/regislex.c
    src/core/error.c
    src/core/memory.c
    src/core/string_utils.c
    src/core/time_utils.c
    src/core/uuid.c
    src/core/config.c
    src/core/logger.c
)

# Source files - Platform abstraction
set(PLATFORM_SOURCES
    src/platform/platform.c
    src/platform/filesystem.c
    src/platform/threading.c
    src/platform/network.c
)

# Source files - Database
set(DATABASE_SOURCES
    src/database/database.c
    src/database/sqlite_driver.c
    src/database/migration.c
    src/database/query_builder.c
)

# Source files - Modules
set(MODULE_SOURCES
    # Case Management
    src/modules/case_management/case.c
    src/modules/case_management/party.c
    src/modules/case_management/matter.c
    src/modules/case_management/case_repository.c

    # Deadline Management
    src/modules/deadline_management/deadline.c
    src/modules/deadline_management/calendar.c
    src/modules/deadline_management/reminder.c
    src/modules/deadline_management/statute_limitations.c

    # Workflow Automation
    src/modules/workflow/workflow_engine.c
    src/modules/workflow/task.c
    src/modules/workflow/trigger.c
    src/modules/workflow/action.c
    src/modules/workflow/template.c

    # Reporting
    src/modules/reporting/report_engine.c
    src/modules/reporting/report_template.c
    src/modules/reporting/data_source.c
    src/modules/reporting/export.c

    # Document Management
    src/modules/document_management/document.c
    src/modules/document_management/storage.c
    src/modules/document_management/version_control.c
    src/modules/document_management/template_engine.c
    src/modules/document_management/search.c

    # Legislative Tracking
    src/modules/legislative_tracking/legislation.c
    src/modules/legislative_tracking/tracker.c
    src/modules/legislative_tracking/alert.c
    src/modules/legislative_tracking/stakeholder.c

    # Enterprise Legal Management
    src/modules/elm/elm_core.c
    src/modules/elm/billing.c
    src/modules/elm/contract.c
    src/modules/elm/risk.c
    src/modules/elm/vendor.c
)

# Source files - API
set(API_SOURCES
    src/api/server.c
    src/api/router.c
    src/api/handler.c
    src/api/auth.c
    src/api/json_utils.c
)

# Source files - Utils
set(UTILS_SOURCES
    src/utils/json.c
    src/utils/xml.c
    src/utils/csv.c
    src/utils/pdf.c
    src/utils/crypto.c
    src/utils/validation.c
)

# Main library
add_library(regislex_core STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${DATABASE_SOURCES}
    ${MODULE_SOURCES}
    ${API_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(regislex_core
    Threads::Threads
)

# Platform-specific libraries
if(REGISLEX_PLATFORM_WINDOWS)
    target_link_libraries(regislex_core ws2_32 shlwapi)
elseif(REGISLEX_PLATFORM_LINUX)
    target_link_libraries(regislex_core dl m)
elseif(REGISLEX_PLATFORM_MACOS)
    target_link_libraries(regislex_core dl m)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(regislex_core ${COREFOUNDATION_LIBRARY})
endif()

# SQLite linking
if(REGISLEX_USE_SYSTEM_SQLITE)
    target_link_libraries(regislex_core SQLite::SQLite3)
else()
    target_link_libraries(regislex_core sqlite3)
endif()

# OpenSSL linking
if(OpenSSL_FOUND)
    target_link_libraries(regislex_core OpenSSL::SSL OpenSSL::Crypto)
endif()

# Main executable
add_executable(regislex src/main.c)
target_link_libraries(regislex regislex_core)

# CLI tool
add_executable(regislex-cli src/cli/cli_main.c src/cli/commands.c)
target_link_libraries(regislex-cli regislex_core)

# Tests
if(REGISLEX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS regislex regislex-cli regislex_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "RegisLex")
set(CPACK_PACKAGE_VENDOR "RegisLex Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Enterprise Legal Software Suite")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
include(CPack)

message(STATUS "RegisLex Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SSL support: ${REGISLEX_ENABLE_SSL}")
message(STATUS "  Build tests: ${REGISLEX_BUILD_TESTS}")
