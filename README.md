# Xenith

Experimental Xbox Series X|S emulator project.

## Current status

- C++20
- CMake
- X11 window
- Software framebuffer renderer
- Rotating 3D cube
- Initial guest virtual-memory system
- Guest memory tests

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/xenith
```

The current graphical prototype is intentionally simple. No game assets or generated boot screens are included.

## Roadmap

1. Guest memory
2. x86-64 CPU core
3. Instruction decoding/execution
4. Executable loader
5. System/kernel abstractions
6. Graphics abstraction and GPU work
7. Compatibility testing
