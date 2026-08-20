cmake_minimum_required(VERSION 3.20)
project(Xenith VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(xenith
    src/main.cpp
    src/window.cpp
    src/renderer.cpp
    src/memory/guest_memory.cpp
)

target_include_directories(xenith PRIVATE src)

find_package(X11 REQUIRED)
target_link_libraries(xenith PRIVATE X11::X11)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xenith PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(xenith_memory_tests
    tests/guest_memory_tests.cpp
    src/memory/guest_memory.cpp
)
target_include_directories(xenith_memory_tests PRIVATE src)
add_test(NAME xenith_memory_tests COMMAND xenith_memory_tests)
