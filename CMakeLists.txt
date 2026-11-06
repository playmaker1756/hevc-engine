cmake_minimum_required(VERSION 3.16)
project(hevc-engine C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hevc STATIC
    src/check.c
    src/geometry.c
)
target_include_directories(hevc PUBLIC include)

add_executable(hevc_check tests/check_main.cpp)
target_link_libraries(hevc_check PRIVATE hevc)

enable_testing()
add_test(NAME hevc_check COMMAND hevc_check)
