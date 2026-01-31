/*
 * linker64.ld - Linker Script for GegOS 64-bit
 * Places kernel at high address (0xffffffff80000000) for 64-bit
 * But for simplicity, we'll load at 1MB like 32-bit and let boot code handle it
 */

/* Entry point */
ENTRY(start64)

/* Define sections */
SECTIONS
{
    /* Start at 1 MB - will be remapped by paging */
    . = 1M;

    /* Multiboot header */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot_header)
    }

    /* Text section (code) */
    .text ALIGN(4K) :
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Discard unwanted sections */
    /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu_debugdata) }
}
