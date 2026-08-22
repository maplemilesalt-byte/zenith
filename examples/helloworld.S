.section .text
.global _start
.type _start, @function

_start:
    # Tiny x86-64 guest program using only instructions currently implemented
    # by the Zenith CPU interpreter.
    mov $1, %eax
    mov $1, %edi
    lea message(%rip), %rsi
    mov $19, %edx
    syscall

    mov $60, %eax
    mov $0, %edi
    syscall

.section .rodata
message:
    .ascii "Hello from Zenith!\n"
