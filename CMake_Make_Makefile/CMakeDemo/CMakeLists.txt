cmake_minimum_required(VERSION 3.10)

project(APP LANGUAGES CXX)

add_compile_options(-Wall -Wextra -Wpedantic -g -O2)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES demo.cpp message.cpp)

add_executable(app ${SOURCES})