cmake_minimum_required(VERSION 3.16)
project(hevc-engine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(hevc STATIC
    src/compare/compare.c
    src/picture/geometry.c
    src/picture/neighbor_availability.c
)
target_include_directories(hevc PUBLIC include)

add_executable(hevc_check
    tests/main.c
    tests/expect.c
    tests/version0_picture.c
    tests/compare_test.c
    tests/picture_geometry_test.c
    tests/neighbor_availability_test.c
)
target_link_libraries(hevc_check PRIVATE hevc)
target_include_directories(hevc_check PRIVATE tests)

enable_testing()
add_test(NAME hevc_check COMMAND hevc_check)
