cmake_minimum_required(VERSION 3.16)
project(RESTful_API LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RESTful_API
        main.cpp
)

target_link_libraries(RESTful_API
        /home/mariana/buildroot/buildroot-2025.02.8/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libcurl.so
)
