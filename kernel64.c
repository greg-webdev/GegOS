/*
 * kernel64.c - GegOS Kernel v0.7 (64-bit x86-64 version)
 * Extended to 64-bit architecture
 * 
 * NOTE: This is a minimal 64-bit kernel stub.
 * It demonstrates 64-bit boot capability but doesn't include
 * the full GUI and game features of the 32-bit version yet.
 */

#include <stdint.h>
#include <stddef.h>
#include "vga.h"

/* Kernel main entry point - 64-bit version */
void kernel64_main(uint32_t* multiboot_info, uint32_t magic) {
    (void)magic;
    (void)multiboot_info;
    
    /* Initialize VGA graphics */
    vga_init();
    
    /* Fill screen with blue to indicate 64-bit kernel */
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLUE);
    
    /* Draw status message */
    vga_putstring(100, 50, "GegOS 64-bit Kernel", COLOR_WHITE, COLOR_BLUE);
    vga_putstring(80, 100, "Long Mode Activated!", COLOR_YELLOW, COLOR_BLUE);
    vga_putstring(100, 150, "64-bit x86-64 Architecture", COLOR_CYAN, COLOR_BLUE);
    vga_putstring(60, 200, "Full 32-bit version: Use 32-bit option in GRUB menu", COLOR_WHITE, COLOR_BLUE);
    
    /* Halt - keep running the kernel */
    asm("hlt");
    
    /* Infinite loop just in case HLT returns */
    while (1) {
        asm("hlt");
    }
}
