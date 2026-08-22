.section .text
.global _start
.type _start, @function

_start:
    # Intel syntax is used because Clang on Termux assembles .S files
    # using its default integrated assembler syntax here.
    mov eax, 1
    mov edi, 1
    lea rsi, [rip + message]
    mov edx, 19
    syscall

    mov eax, 60
    xor edi, edi
    syscall

.section .rodata
message:
    .ascii "Hello from Zenith!\n"
