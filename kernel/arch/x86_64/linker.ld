ENTRY(start32)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .multiboot2 ALIGN(8) : {
        KEEP(*(.multiboot2))
    }

    .text ALIGN(4K) : {
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data .data.*)
    }

    .bss ALIGN(4K) (NOLOAD) : {
        *(COMMON)
        *(.bss .bss.*)
    }

    __kernel_end = .;
}
