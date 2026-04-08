cmake_minimum_required(VERSION 3.16)

project(pegasus VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pegasus
    src/main.cpp
    src/debug.cpp
    src/chunk.cpp
    src/compiler.cpp
    src/scanner.cpp
    src/vm.cpp
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

# Testing with Catch2
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)

FetchContent_MakeAvailable(Catch2)

add_library(pegasus_lib STATIC
    src/debug.cpp
    src/chunk.cpp
    src/compiler.cpp
    src/scanner.cpp
    src/vm.cpp
)

target_include_directories(pegasus_lib PUBLIC include)
target_compile_options(pegasus_lib PRIVATE
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

target_link_options(pegasus_lib PUBLIC
    -fsanitize=address
    -fsanitize=undefined
)

add_executable(pegasus_tests
    tests/test_chunk.cpp
    tests/test_scanner.cpp
    tests/test_compiler.cpp
    tests/test_vm.cpp
)

target_link_libraries(pegasus_tests PRIVATE pegasus_lib Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(pegasus_tests)