cmake_minimum_required(VERSION 3.10)
project(lab5 C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check for local sqlite3 source
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/sqlite3/sqlite3.c")
    message(STATUS "Using local SQLite3 amalgamation")
    set(SQLITE3_SOURCES lib/sqlite3/sqlite3.c)
    include_directories(lib/sqlite3)
    add_library(sqlite3_lib STATIC ${SQLITE3_SOURCES})
else()
    message(STATUS "Searching for system SQLite3")
    find_package(SQLite3)
    if(SQLite3_FOUND)
        include_directories(${SQLite3_INCLUDE_DIRS})
        # link_libraries(${SQLite3_LIBRARIES})
    else()
        message(WARNING "SQLite3 not found. Please run setup script or install libsqlite3-dev.")
    endif()
endif()

# Windows Sockets
if(WIN32)
    set(EXTRA_LIBS ws2_32)
endif()

add_executable(server 
    src/server.cpp 
    src/database.cpp 
    src/http_server.cpp
)
target_include_directories(server PRIVATE src)

if(TARGET sqlite3_lib)
    target_link_libraries(server sqlite3_lib ${EXTRA_LIBS})
    if(UNIX)
        target_link_libraries(server pthread dl)
    endif()
elseif(SQLite3_FOUND)
    target_link_libraries(server SQLite::SQLite3 ${EXTRA_LIBS})
    if(UNIX AND NOT APPLE)
        target_link_libraries(server pthread)
    endif()
else()
    target_link_libraries(server ${EXTRA_LIBS})
endif()

add_executable(simulator src/simulator.cpp)
