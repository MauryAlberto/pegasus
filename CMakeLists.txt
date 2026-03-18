cmake_minimum_required(VERSION 3.16)

project(pegasus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pegasus
    src/main.cpp
)

target_compile_options(pegasus PRIVATE
    -g
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -fsanitize=address
    -fsanitize=undefined
)

target_link_options(pegasus PRIVATE
    -fsanitize=address
    -fsanitize=undefined
)

target_include_directories(pegasus PRIVATE include)