### Paused for a indefinite amount of time
# Zenith

Experimental Xbox Series X|S emulator project.

## Current status

- C++20
- CMake
- X11 window
- Software framebuffer renderer
- Rotating 3D cube
- Guest virtual-memory system with 4 KiB pages
- Initial x86-64 interpreter
- CPU instruction and memory regression tests
- GitHub Actions CI

### Initial x86-64 instruction set

The interpreter currently implements a deliberately small, testable subset:

- `NOP`
- `MOV r32, imm32`
- `REX.W MOV r64, imm64`
- `REX.W ADD r64, r64` (register-direct ModRM)
- `REX.W SUB r64, r64` (register-direct ModRM)
- `REX.W CMP r64, r64` (register-direct ModRM)
- `JMP rel8`
- `JZ rel8`
- `JNZ rel8`

This is an emulator foundation, not yet a complete x86-64 implementation.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/zenith
```

The graphical prototype is intentionally simple. No game assets or generated boot screens are included.

## Roadmap

1. Expand x86-64 register/state model
2. Expand instruction decoder and execution coverage
3. Add stack operations and calls/returns
4. Add executable loader
5. Add system/kernel abstractions
6. Add block cache/JIT for performance
7. Build graphics abstraction and GPU work
8. Compatibility testing
