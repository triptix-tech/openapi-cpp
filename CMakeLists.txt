cmake_minimum_required(VERSION 3.10)
project(openapi)

include(cmake/pkg.cmake)

file(GLOB_RECURSE openapi-src src/*.cc)
add_library(openapi ${openapi-src})
target_include_directories(openapi PUBLIC include)
target_compile_features(openapi PUBLIC cxx_std_23)
target_link_libraries(openapi PUBLIC utl boost-url boost-json yaml-cpp::yaml-cpp boost cista)

add_library(openapi-generated INTERFACE)
file(GLOB_RECURSE openapi-test-files test/*.cc)
add_executable(openapi-test ${openapi-test-files})
target_link_libraries(openapi-test openapi gtest gtest_main)
target_compile_options(openapi-test PRIVATE ${openapi-compile-options})

