/* Kernel linker script: load at 1MB, Multiboot2 header first */
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot BLOCK(8) : ALIGN(8)
    {
        KEEP(*(.multiboot))
    }

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text .text.*)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data .data.*)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
        __kernel_end = .;
        . = ALIGN(16);
        __stack_bottom = .;
        . += 64K;
        __stack_top = .;
    }

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment)
    }
}
