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

# Build the guest explicitly for x86-64. The host may be ARM64 (for example
# Termux on a phone), so using the host compiler's default target would emit
# ARM64 assembly instead of the x86-64 instructions Zenith emulates.
find_program(ZENITH_CLANG clang REQUIRED)
find_program(ZENITH_LLD ld.lld REQUIRED)
set(ZENITH_HELLO_OBJ ${CMAKE_CURRENT_BINARY_DIR}/helloworld.o)
set(ZENITH_HELLO_ELF ${CMAKE_CURRENT_BINARY_DIR}/helloworld.elf)

add_custom_command(
    OUTPUT ${ZENITH_HELLO_OBJ}
    COMMAND ${ZENITH_CLANG}
            --target=x86_64-unknown-linux-gnu
            -c ${CMAKE_CURRENT_SOURCE_DIR}/examples/helloworld.S
            -o ${ZENITH_HELLO_OBJ}
    DEPENDS examples/helloworld.S
    COMMENT "Assembling x86-64 Zenith guest helloworld"
    VERBATIM
)

add_custom_command(
    OUTPUT ${ZENITH_HELLO_ELF}
    COMMAND ${ZENITH_LLD}
            -m elf_x86_64
            -T ${CMAKE_CURRENT_SOURCE_DIR}/examples/helloworld.ld
            -o ${ZENITH_HELLO_ELF}
            ${ZENITH_HELLO_OBJ}
    DEPENDS ${ZENITH_HELLO_OBJ} examples/helloworld.ld
    COMMENT "Linking x86-64 Zenith guest helloworld.elf"
    VERBATIM
)

add_custom_target(zenith_helloworld ALL DEPENDS ${ZENITH_HELLO_ELF})
add_dependencies(zenith zenith_helloworld)
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
