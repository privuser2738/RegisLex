# SQLite Amalgamation Build
# Download sqlite-amalgamation and place sqlite3.c and sqlite3.h here

cmake_minimum_required(VERSION 3.16)

# Check if SQLite source exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c")
    add_library(sqlite3 STATIC sqlite3.c)
    target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # SQLite compile options for better performance and security
    target_compile_definitions(sqlite3 PRIVATE
        SQLITE_ENABLE_FTS5
        SQLITE_ENABLE_JSON1
        SQLITE_ENABLE_RTREE
        SQLITE_ENABLE_COLUMN_METADATA
        SQLITE_ENABLE_STAT4
        SQLITE_ENABLE_UPDATE_DELETE_LIMIT
        SQLITE_THREADSAFE=2
        SQLITE_DQS=0
        SQLITE_DEFAULT_MEMSTATUS=0
        SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
        SQLITE_LIKE_DOESNT_MATCH_BLOBS
        SQLITE_MAX_EXPR_DEPTH=0
        SQLITE_OMIT_DECLTYPE
        SQLITE_OMIT_DEPRECATED
        SQLITE_OMIT_PROGRESS_CALLBACK
        SQLITE_OMIT_SHARED_CACHE
        SQLITE_USE_ALLOCA
    )

    if(WIN32)
        target_compile_definitions(sqlite3 PRIVATE SQLITE_OS_WIN)
    else()
        target_compile_definitions(sqlite3 PRIVATE SQLITE_OS_UNIX)
    endif()
else()
    message(WARNING "SQLite source not found. Please download the SQLite amalgamation from https://www.sqlite.org/download.html and place sqlite3.c and sqlite3.h in ${CMAKE_CURRENT_SOURCE_DIR}")

    # Create a stub interface library
    add_library(sqlite3 INTERFACE)
    target_include_directories(sqlite3 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
