ENTRY(start)

SECTIONS
{
    . = 1M;

    .multiboot2 ALIGN(8) :
    {
        KEEP(*(.multiboot2))
    }

    . = ALIGN(4K);
    .text :
    {
        *(.text*)
    }

    . = ALIGN(4K);
    .rodata :
    {
        *(.rodata*)
    }

    . = ALIGN(4K);
    .data :
    {
        *(.data*)
    }

    . = ALIGN(4K);
    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
}
