.section .text
.global _start
.type _start, @function

_start:
    # x86-64 AT&T syntax. CMake invokes Clang with an explicit x86-64 target,
    # even when the host (such as Termux) is ARM64.
    mov $1, %eax
    mov $1, %edi
    lea message(%rip), %rsi
    mov $19, %edx
    syscall

    mov $60, %eax
    xor %edi, %edi
    syscall

.section .rodata
message:
    .ascii "Hello from Zenith!\n"
