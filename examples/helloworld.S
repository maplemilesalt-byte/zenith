.section .text
.global _start
.type _start, @function

_start:
    # Zenith guest ABI follows the x86-64 Linux syscall register convention
    # for the tiny console interface implemented by the emulator.
    mov $1, %rax              # write
    mov $1, %rdi              # stdout
    lea message(%rip), %rsi
    mov $19, %rdx             # strlen("Hello from Zenith!\n")
    syscall

    mov $60, %rax             # exit
    xor %rdi, %rdi            # status = 0
    syscall

.section .rodata
message:
    .ascii "Hello from Zenith!\n"
