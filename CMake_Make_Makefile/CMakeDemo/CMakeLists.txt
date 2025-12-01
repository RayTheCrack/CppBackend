cmake_minimum_required(VERSION 3.10)

project(APP LANGUAGES CXX)

# 编译选项：开启警告、生成调试信息、二级优化
add_compile_options(-Wall -Wextra -Wpedantic -g -O2)

# 指定 C++17 标准，强制要求支持（不支持则报错）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可执行文件 app 的源文件列表
set(SOURCES demo.cpp message.cpp)

# 编译动态库 libpri（如果要改静态库，把 SHARED 改成 STATIC 即可）
add_library(libpri SHARED primessage.cpp)

# PUBLIC 表示：libpri 自身编译时能用，链接它的 app 也能用
target_include_directories(libpri PUBLIC 
    ${CMAKE_SOURCE_DIR}/HeadFile  # 明确指定头文件路径：项目根目录下的 HeadFile 文件夹
)

# 编译可执行文件 app
add_executable(app ${SOURCES})

# 让 app 链接 libpri 库（动态库或静态库，取决于上面的 SHARED/STATIC）
target_link_libraries(app libpri)