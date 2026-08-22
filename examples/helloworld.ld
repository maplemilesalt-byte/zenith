ENTRY(_start)

SECTIONS
{
    . = 0x1000;

    .text : ALIGN(16)
    {
        *(.text*)
    }

    . = ALIGN(0x1000);

    .rodata : ALIGN(16)
    {
        *(.rodata*)
    }
}
