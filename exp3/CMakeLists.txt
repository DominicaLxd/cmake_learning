cmake_minimum_required(VERSION 3.5)

project(static_library)  # 项目名

# 加载Gist
#file(GLOB_RECURSE <GIST_DIR> "${PROJECT_SOURCE_DIR}/include/gist/.cpp" "${PROJECT_SOURCE_DIR}/include/gist/.h")


add_library(GIST_lib 
STATIC  # 从gist_dir 创建一个static库，叫GIST_lib
    include/gist/gist.cpp)



# 从某些源文件中创建一个库
add_library(static_lib STATIC  # 从Hello.cpp 创建一个static库，叫static_library
    src/Hello.cpp)

target_include_directories(static_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)  # static_library 库链接到/include目录


# create an executable

add_executable(static_library_exe  # 可执行文件static_library_exe， 从源文件main.cpp
    src/main.cpp)

# 
target_link_libraries(static_library_exe # 可执行文件，链接到库static_library
PRIVATE 
GIST_lib)

target_link_libraries(static_library_exe # 可执行文件，链接到库static_library
    PRIVATE 
    static_lib)