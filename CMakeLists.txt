cmake_minimum_required(VERSION 3.10)
project(ffmpeg_tools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 FFmpeg 依赖
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
message(${FFMPEG_DIR})
include_directories(${FFMPEG_DIR}/include)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${FFMPEG_DIR}/lib/x64/windows)
else()
    link_directories(${FFMPEG_DIR}/lib/x86/windows)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/deps/CLI11/)

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接 FFmpeg 库
target_link_libraries(${PROJECT_NAME}
    avcodec
    avformat
    avutil
    swscale
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)