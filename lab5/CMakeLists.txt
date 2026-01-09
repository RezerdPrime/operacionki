cmake_minimum_required(VERSION 3.10)
project(SerialTempMonitor LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
else()
    find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
    find_library(SQLITE3_LIBRARY sqlite3)
    set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
    set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
endif()

add_executable(emulator emulator.cpp)
add_executable(listener listener.cpp)

target_link_libraries(emulator PRIVATE Threads::Threads)
target_link_libraries(listener PRIVATE Threads::Threads ${SQLITE3_LIBRARIES})

target_include_directories(listener PRIVATE ${SQLITE3_INCLUDE_DIRS})

if(WIN32)
    target_link_libraries(emulator PRIVATE kernel32)
    target_link_libraries(listener PRIVATE kernel32)
endif()