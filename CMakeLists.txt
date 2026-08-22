cmake_minimum_required(VERSION 3.20)
project(Zenith VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(zenith
    src/main.cpp
    src/window.cpp
    src/renderer.cpp
    src/elf_loader.cpp
    src/memory/guest_memory.cpp
    src/cpu/cpu.cpp
)

target_include_directories(zenith PRIVATE src)

find_package(X11 REQUIRED)
target_link_libraries(zenith PRIVATE X11::X11)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zenith PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Build a tiny statically linked x86-64 ELF guest that only uses instructions
# currently implemented by Zenith. It is useful as an end-to-end loader/CPU
# smoke test and as the first real guest application.
find_program(ZENITH_CLANG clang REQUIRED)
set(ZENITH_HELLO_ELF ${CMAKE_CURRENT_BINARY_DIR}/helloworld.elf)
add_custom_command(
    OUTPUT ${ZENITH_HELLO_ELF}
    COMMAND ${ZENITH_CLANG}
            -nostdlib -static -fuse-ld=lld
            -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/examples/helloworld.ld
            -o ${ZENITH_HELLO_ELF}
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/helloworld.S
    DEPENDS examples/helloworld.S examples/helloworld.ld
    COMMENT "Building Zenith guest helloworld.elf"
    VERBATIM
)
add_custom_target(zenith_helloworld ALL DEPENDS ${ZENITH_HELLO_ELF})
add_dependencies(zenith zenith_helloworld)

# Also expose the generated ELF as a named build target for scripts/Termux.
add_custom_target(helloworld DEPENDS ${ZENITH_HELLO_ELF})

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
