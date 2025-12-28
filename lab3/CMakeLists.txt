cmake_minimum_required(VERSION 3.10)
project(lab3)

set(CMAKE_CXX_STANDARD 11)

# Добавьте исполняемый файл
add_executable(LAB3 main.cpp)

if(UNIX)
    target_link_libraries(LAB3 pthread rt)
endif()
