cmake_minimum_required(VERSION 4.1)
project(untitled)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled main.cpp
        include/miniaudio.h
        include/id3.h
        include/tools.h
        include/music.h
        src/id3.cpp
        src/music.cpp
        src/tools.cpp
        resources.rc
)

target_include_directories(untitled PRIVATE include)