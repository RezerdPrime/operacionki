cmake_minimum_required(VERSION 3.10)
project(SerialTempMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти потоки
find_package(Threads REQUIRED)

add_executable(emulator emulator.cpp)
add_executable(listener listener.cpp)

# Линкуем с потоками (это добавит -pthread)
# target_link_libraries(emulator PRIVATE Threads::Threads)
# target_link_libraries(listener PRIVATE Threads::Threads)

# На Windows потоки встроены, но CMake сам поймёт
if(WIN32)
    target_link_libraries(emulator PRIVATE kernel32)
    target_link_libraries(listener PRIVATE kernel32)
endif()