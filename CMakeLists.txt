cmake_minimum_required(VERSION 3.11)
project(TinyCompiler)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf-3")

add_executable(tiny_compiler token.cpp IRNode.cpp main.cpp)
target_include_directories(tiny_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tiny_compiler PUBLIC stdc++)

# target_link_libraries...

#target_include_directories()