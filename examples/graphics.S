.section .text
.global _start
.type _start, @function

# Zenith guest framebuffer: 320x240, 32-bit pixels at 0x800000.
# syscall 200 = present framebuffer.
_start:
    mov $0x800000, %edi
    mov $12800, %ecx

    mov $0x003060c0, %eax
blue:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz blue

    mov $12800, %ecx
    mov $0x0030c060, %eax
green:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz green

    mov $12800, %ecx
    mov $0x00c06030, %eax
red:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz red

    mov $200, %eax
    syscall

    mov $60, %eax
    mov $0, %edi
    syscall
