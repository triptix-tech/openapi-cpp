cmake_minimum_required(VERSION 3.10)
project(openapi)

include(cmake/pkg.cmake)

file(GLOB_RECURSE openapi-src src/*.cc)
add_library(openapi ${openapi-src})
target_include_directories(openapi PUBLIC include)
target_compile_features(openapi PUBLIC cxx_std_23)
target_link_libraries(openapi PUBLIC utl boost-url boost-json yaml-cpp::yaml-cpp boost cista)

add_executable(openapi-generate exe/generate.cc)
target_link_libraries(openapi-generate openapi)
target_compile_features(openapi-generate PRIVATE cxx_std_23)

function(openapi_generate openapi-file lib ns)
    add_custom_command(
            COMMAND
                openapi-generate
                    ${CMAKE_CURRENT_SOURCE_DIR}/${openapi-file}
                    ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}.h
                    ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}.cc
                    ${ns}
            DEPENDS
                openapi-generate
                ${CMAKE_CURRENT_SOURCE_DIR}/${openapi-file}
            OUTPUT
                ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}.h
                ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}.cc
    )
    add_library(${lib} ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}.cc)
    target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${lib} openapi)
    target_compile_features(${lib} PUBLIC cxx_std_23)
    set_target_properties(${lib} PROPERTIES CXX_CLANG_TIDY "")
endfunction()

openapi_generate(test/pet.yml pet-api pet)

add_library(openapi-generated INTERFACE)
file(GLOB_RECURSE openapi-test-files test/*.cc)
add_executable(openapi-test ${openapi-test-files})
target_link_libraries(openapi-test pet-api gtest gtest_main)
target_compile_options(openapi-test PRIVATE ${openapi-compile-options})
