cmake_minimum_required(VERSION 3.20)
project(Zenith VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(zenith
    src/main.cpp
    src/window.cpp
    src/renderer.cpp
    src/memory/guest_memory.cpp
    src/cpu/cpu.cpp
)

target_include_directories(zenith PRIVATE src)

find_package(X11 REQUIRED)
target_link_libraries(zenith PRIVATE X11::X11)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zenith PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(zenith_memory_tests
    tests/guest_memory_tests.cpp
    src/memory/guest_memory.cpp
)
target_include_directories(zenith_memory_tests PRIVATE src)
add_test(NAME zenith_memory_tests COMMAND zenith_memory_tests)

add_executable(zenith_cpu_tests
    tests/cpu_tests.cpp
    src/cpu/cpu.cpp
    src/memory/guest_memory.cpp
)
target_include_directories(zenith_cpu_tests PRIVATE src)
add_test(NAME zenith_cpu_tests COMMAND zenith_cpu_tests)
