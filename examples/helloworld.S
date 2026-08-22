.section .text
.global _start
.type _start, @function

_start:
    # Keep the guest instruction set intentionally tiny: the current Zenith
    # CPU implements B8+rd, RIP-relative LEA, SYSCALL, and the 64-bit logical
    # operations needed by this program.
    mov $1, %eax              # syscall: write
    mov $1, %edi              # stdout
    lea message(%rip), %rsi
    mov $19, %edx             # strlen("Hello from Zenith!\n")
    syscall

    mov $60, %eax             # syscall: exit
    mov $0, %edi              # status = 0
    syscall

.section .rodata
message:
    .ascii "Hello from Zenith!\n"
