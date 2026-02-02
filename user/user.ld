ENTRY(_start)
SECTIONS
{
    . = 0x10000000;  /* Must match USER_ENTRY in kernel/task/pcb.h */
    .text : {
        *(.text._start)    /* Put _start first */
        *(.text .text.*)
    }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }
}
