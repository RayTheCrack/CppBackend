cmake_minimum_required(VERSION 3.10)

project(SMARTPOINTER VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/test/*.cpp
)

add_compile_options(-Wall -Wextra -Wpedantic -g -O2)

add_executable(smartpointer ${SOURCES})
