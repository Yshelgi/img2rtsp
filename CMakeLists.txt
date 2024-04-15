cmake_minimum_required(VERSION 3.12)
project(img2rtsp)

set(CMAKE_CXX_STANDARD 17)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)


add_executable(img2rtsp main.cpp
        Media.cpp
        Media.h)
target_link_libraries(img2rtsp
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
)
