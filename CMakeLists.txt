cmake_minimum_required(VERSION 3.2)

project(H264Analyer)

set(CMAKE_CXX_STANDARD 14)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH $ENV{QTDIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)

add_executable(h264analyer main.cpp
        mainwindow.cpp
        h264_avcc.cpp
        h264_sei.cpp
        h264_stream.cpp
        h265_avcc.cpp
        h265_sei.cpp
        h265_stream.cpp
        nal_parse.cpp
        NalDebugTree.cpp
        NalDebugTxt.cpp)

target_link_libraries(h264analyer Qt5::Widgets)

