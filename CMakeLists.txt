cmake_minimum_required(VERSION 3.12)
project(img2rtsp)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    include_directories(D:/libs/ffmpeg/include)
    link_directories(D:/libs/ffmpeg/lib)
else ()
endif ()


add_executable(img2rtsp main.cpp
        Media.cpp
        Media.h)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    message("Use Linux")
    target_link_libraries(img2rtsp
            avcodec avdevice avfilter
            avformat avutil postproc
            swresample swscale
    )

elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    message("Use Windows")
    target_link_libraries(img2rtsp
            avcodec avdevice
            avfilter avformat avutil
            postproc swresample swscale
    )
else ()
endif ()