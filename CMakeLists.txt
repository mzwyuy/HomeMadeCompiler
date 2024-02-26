cmake_minimum_required(VERSION 3.11)
project(TinyCompiler)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

cmake_policy(SET CMP0074 NEW)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf-3")

#find_package(GTest REQUIRED)  # need to use vcpkg to install pkgs
add_subdirectory(${CMAKE_SOURCE_DIR}/3rd_lib/googletest-main)
include_directories(${CMAKE_SOURCE_DIR}/3rd_lib/googletest-main/googletest/include)

add_subdirectory(lex_and_parse)
add_subdirectory(test_cases)

add_executable(tiny_compiler main.cpp)
target_include_directories(tiny_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ./lex_and_parse ./test_cases)
target_link_libraries(tiny_compiler PUBLIC stdc++)
target_link_libraries(tiny_compiler PUBLIC gtest gtest_main lex_and_parse)
