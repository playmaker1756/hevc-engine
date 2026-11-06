cmake_minimum_required(VERSION 3.16)
project(hevc-engine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(hevc STATIC
    src/compare/compare.c
    src/common/integer_spec.c
    src/picture/geometry.c
    src/picture/neighbor_availability.c
    src/intra/reference_gather.c
    src/intra/reference_substitute.c
    src/intra/reference_smooth.c
    src/intra/predict_dc.c
    src/intra/predict_planar.c
    src/intra/predict_angular.c
    src/intra/mode.c
    src/residual/inverse_4x4.c
    src/residual/inverse_transform.c
    src/residual/trans_matrix.c
    src/loop/deblock.c
    src/loop/sao.c
    src/bits/exp_golomb.c
    src/bits/annex_b.c
    src/bits/cabac.c
    src/bits/rbsp.c
    src/picture/coded_structure.c
)
target_include_directories(hevc PUBLIC include)

add_executable(hevc_check
    tests/main.c
    tests/expect.c
    tests/version0_picture.c
    tests/compare_test.c
    tests/picture_geometry_test.c
    tests/neighbor_availability_test.c
    tests/intra_predict_test.c
    tests/derived_rest_test.c
    tests/cabac_transform_test.c
)
target_link_libraries(hevc_check PRIVATE hevc)
target_include_directories(hevc_check PRIVATE tests)

enable_testing()
add_test(NAME hevc_check COMMAND hevc_check)
