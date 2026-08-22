.section .text
.global _start
.type _start, @function

# Zenith APPX platformer test.
# 320x240 framebuffer at 0x800000, 32-bit pixels.
# syscall 200 = present framebuffer.
# The player automatically walks across the scene for now.

_start:
    mov $0x800000, %r12
    xor %r13, %r13
    mov $2560, %r10
    mov $288, %r11
loop:
    mov $0x00001830, %eax
    mov $9600, %ecx
    mov %r12, %rdi
clear:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz clear

    mov $0x00305020, %eax
    mov %r12, %rdi
    mov $552960, %r14
    add %r14, %rdi
    mov %r10, %r14
    call fill_ground

    mov $0x00c08030, %eax
    mov %r12, %rdi
    mov $409600, %r14
    add %r14, %rdi
    call fill_platform

    mov $0x00c08030, %eax
    mov %r12, %rdi
    mov $307200, %r14
    add %r14, %rdi
    call fill_platform

    mov $0x00c08030, %eax
    mov %r12, %rdi
    mov $204800, %r14
    add %r14, %rdi
    call fill_platform

    mov $0x00f0d040, %eax
    mov %r12, %rdi
    mov $512000, %r14
    add %r14, %rdi
    add %r13, %rdi
    call fill_player

    mov %r13, %r14
    cmp %r11, %r14
    jl move_on
    xor %r13, %r13
move_on:
    add $1, %r13
    mov $200, %eax
    syscall
    mov $200000, %ecx
delay:
    dec %rcx
    jnz delay
    jmp loop

# rdi = start, r10 = row stride, rax = color
fill_ground:
    mov %rdi, %r14
    mov $160, %ecx
fg:
    mov %rax, (%r14)
    add $8, %r14
    dec %rcx
    jnz fg
    ret

fill_platform:
    mov %rdi, %r14
    mov $8, %edx
fp_row:
    mov $8, %ebx
fp_col:
    mov %rax, (%r14)
    add $8, %r14
    dec %rbx
    jnz fp_col
    add %r10, %r14
    dec %rdx
    jnz fp_row
    ret

fill_player:
    mov %rdi, %r14
    mov $8, %edx
pl_row:
    mov $2, %ebx
pl_col:
    mov %rax, (%r14)
    add $8, %r14
    dec %rbx
    jnz pl_col
    add %r10, %r14
    dec %rdx
    jnz pl_row
    ret
