.section .text
.global _start
.type _start, @function

# Zenith virtual-controller ABI test.
# Framebuffer: 0x800000, 320x240, 32-bit pixels.
# Input ABI: 0xF000.
# buttons = [0xF008], dpad = [0xF00C]

_start:
    mov $0x800000, %r12
    mov $0xF000, %r15

loop:
    # Clear the framebuffer to black. One qword contains two black pixels.
    xor %eax, %eax
    mov $38400, %ecx
    mov %r12, %rdi
clear:
    mov %rax, (%rdi)
    add $8, %rdi
    dec %rcx
    jnz clear

    # Read current button/dpad state.
    mov (%r15), %r14
    mov 8(%r15), %r13

    # A B X Y
    mov $0x004080c0, %eax
    test $1, %r14d
    jz a_off
    mov $0x00ffffff, %eax
a_off:
    mov $0x800100, %rdi
    call box

    mov $0x00c04040, %eax
    test $2, %r14d
    jz b_off
    mov $0x00ffffff, %eax
b_off:
    mov $0x800120, %rdi
    call box

    mov $0x0040c080, %eax
    test $4, %r14d
    jz x_off
    mov $0x00ffffff, %eax
x_off:
    mov $0x800140, %rdi
    call box

    mov $0x00c0a040, %eax
    test $8, %r14d
    jz y_off
    mov $0x00ffffff, %eax
y_off:
    mov $0x800160, %rdi
    call box

    # LB RB LS RS
    mov $0x006060c0, %eax
    test $16, %r14d
    jz lb_off
    mov $0x00ffffff, %eax
lb_off:
    mov $0x804000, %rdi
    call box

    mov $0x00c060c0, %eax
    test $32, %r14d
    jz rb_off
    mov $0x00ffffff, %eax
rb_off:
    mov $0x804020, %rdi
    call box

    mov $0x0060c0c0, %eax
    test $64, %r14d
    jz ls_off
    mov $0x00ffffff, %eax
ls_off:
    mov $0x804040, %rdi
    call box

    mov $0x00c060a0, %eax
    test $128, %r14d
    jz rs_off
    mov $0x00ffffff, %eax
rs_off:
    mov $0x804060, %rdi
    call box

    # View/Menu
    mov $0x006080c0, %eax
    test $256, %r14d
    jz view_off
    mov $0x00ffffff, %eax
view_off:
    mov $0x806000, %rdi
    call box

    mov $0x00c08060, %eax
    test $512, %r14d
    jz menu_off
    mov $0x00ffffff, %eax
menu_off:
    mov $0x806020, %rdi
    call box

    # D-pad: up/down/left/right
    mov $0x00808080, %eax
    test $1, %r13d
    jz du_off
    mov $0x00ffffff, %eax
du_off:
    mov $0x808100, %rdi
    call box

    mov $0x00808080, %eax
    test $2, %r13d
    jz dd_off
    mov $0x00ffffff, %eax
dd_off:
    mov $0x808120, %rdi
    call box

    mov $0x00808080, %eax
    test $4, %r13d
    jz dl_off
    mov $0x00ffffff, %eax
dl_off:
    mov $0x808140, %rdi
    call box

    mov $0x00808080, %eax
    test $8, %r13d
    jz dr_off
    mov $0x00ffffff, %eax
dr_off:
    mov $0x808160, %rdi
    call box

    # Present and keep running.
    mov $200, %eax
    syscall
    mov $200000, %ecx
delay:
    dec %ecx
    jnz delay
    jmp loop

# Draw an 16x8 block using 32-bit pixels so adjacent pixels do not become black.
# rdi = framebuffer address, eax = color.
box:
    mov %rdi, %r8
    mov $8, %edx
box_row:
    mov %eax, (%r8)
    mov %eax, 4(%r8)
    mov %eax, 8(%r8)
    mov %eax, 12(%r8)
    mov %eax, 16(%r8)
    mov %eax, 20(%r8)
    mov %eax, 24(%r8)
    mov %eax, 28(%r8)
    mov %eax, 32(%r8)
    mov %eax, 36(%r8)
    mov %eax, 40(%r8)
    mov %eax, 44(%r8)
    mov %eax, 48(%r8)
    mov %eax, 52(%r8)
    mov %eax, 56(%r8)
    mov %eax, 60(%r8)
    add $1280, %r8
    dec %edx
    jnz box_row
    ret
