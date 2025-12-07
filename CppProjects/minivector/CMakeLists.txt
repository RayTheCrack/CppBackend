cmake_minimum_required(VERSION 3.10)

project(MyProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(MyExecutable main.cpp)

target_include_directories(MyExecutable PRIVATE include)



