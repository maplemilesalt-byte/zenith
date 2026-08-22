.section .text
.global _start
.type _start, @function

# Zenith APPX platformer test.
# 320x240 framebuffer at 0x800000, 32-bit pixels.
# syscall 200 = present framebuffer.
# This first test is intentionally self-contained and uses no libc.
# It draws a simple platformer scene and animates the player horizontally.

_start:
    mov $0x800000, %r12
    mov $0, %r13                 # animation frame

loop:
    # Clear screen: dark blue background.
    mov $0x00001830, %eax
    mov $9600, %ecx
    mov %r12, %rdi
clear:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz clear

    # Ground: 320x24 at y=216.
    mov $0x00305020, %eax
    mov $960, %ecx
    mov %r12, %rdi
    add $69120, %rdi
    mov %rdi, %r14
    call fill_ground

    # Platforms.
    mov $0x00c08030, %eax
    mov $0x800000, %rdi
    add $51200, %rdi             # y=160
    mov $32, %ecx
    call fill_platform

    mov $0x00c08030, %eax
    mov $0x800000, %rdi
    add $38400, %rdi             # y=120
    mov $24, %ecx
    call fill_platform

    mov $0x00c08030, %eax
    mov $0x800000, %rdi
    add $25600, %rdi             # y=80
    mov $20, %ecx
    call fill_platform

    # Player: an 16x16 yellow square. Its x position loops across the screen.
    mov $0x00f0d040, %eax
    mov %r13, %r15
    and $0xff, %r15
    add $16, %r15
    mov $0x800000, %rdi
    add $69120, %rdi
    # move up to y=200
    sub $5120, %rdi
    mov $16, %ecx
    call fill_player

    # Advance animation and wrap at 256.
    add $1, %r13

    mov $200, %eax
    syscall

    # Simple delay loop.
    mov $200000, %ecx
delay:
    dec %ecx
    jnz delay
    jmp loop

fill_ground:
    # 320 pixels wide = 160 qwords.
    mov $160, %ecx
fg:
    mov %rax, (%r14)
    add $8, %r14
    dec %ecx
    jnz fg
    ret

fill_platform:
    # 8 qwords per row, 8 rows. This gives a visible block.
    mov %rdi, %r14
    mov $8, %edx
fp_row:
    mov $8, %ebx
fp_col:
    mov %rax, (%r14)
    add $8, %r14
    dec %ebx
    jnz fp_col
    add $2560, %r14
    dec %edx
    jnz fp_row
    ret

fill_player:
    mov %rdi, %r14
    # Horizontal offset, 8 bytes per pair of pixels.
    add %r15, %r14
    mov $8, %edx
pl_row:
    mov $2, %ebx
pl_col:
    mov %rax, (%r14)
    add $8, %r14
    dec %ebx
    jnz pl_col
    add $2560, %r14
    sub $16, %r14
    dec %edx
    jnz pl_row
    ret

