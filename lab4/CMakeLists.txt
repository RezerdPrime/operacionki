cmake_minimum_required(VERSION 3.10)
project(SerialTempMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
find_package(Threads REQUIRED)

add_executable(emulator emulator.cpp)
add_executable(listener listener.cpp)

if(WIN32)
    target_link_libraries(emulator PRIVATE kernel32)
    target_link_libraries(listener PRIVATE kernel32)
endif()