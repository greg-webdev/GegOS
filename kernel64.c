/*
 * kernel64.c - GegOS Kernel v0.7 (64-bit x86-64 version)
 * Extended to 64-bit architecture with framebuffer support
 * 
 * Uses Multiboot 2 framebuffer for graphics output
 */

#include <stdint.h>
#include <stddef.h>
#include "io.h"

/* Multiboot 2 structures for framebuffer support */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} multiboot2_info_header_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot2_tag_header_t;

typedef struct {
    uint32_t type;        // 8
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} multiboot2_framebuffer_tag_t;

/* Global framebuffer information */
static uint64_t fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t fb_bpp = 0;

/* Parse Multiboot 2 information structure */
static void parse_multiboot2_info(uint32_t* mb_info) {
    multiboot2_info_header_t* header = (multiboot2_info_header_t*)mb_info;
    uint32_t total_size = header->total_size;
    
    /* Start parsing tags after the header */
    uint32_t offset = 8; // Skip header
    while (offset < total_size) {
        multiboot2_tag_header_t* tag = (multiboot2_tag_header_t*)((uint8_t*)mb_info + offset);
        
        if (tag->type == 8) { // Framebuffer tag
            multiboot2_framebuffer_tag_t* fb_tag = (multiboot2_framebuffer_tag_t*)tag;
            fb_addr = fb_tag->framebuffer_addr;
            fb_pitch = fb_tag->framebuffer_pitch;
            fb_width = fb_tag->framebuffer_width;
            fb_height = fb_tag->framebuffer_height;
            fb_bpp = fb_tag->framebuffer_bpp;
        } else if (tag->type == 0) { // End tag
            break;
        }
        
        /* Move to next tag (aligned to 8 bytes) */
        offset += (tag->size + 7) & ~7;
    }
}

/* Framebuffer drawing functions - simplified for debug */


/* Kernel main entry point - 64-bit version */
void kernel64_main(uintptr_t multiboot_info, uint32_t magic) {
    (void)magic;
    
    /* Parse Multiboot 2 information to get framebuffer details */
    parse_multiboot2_info((uint32_t*)multiboot_info);
    
    /* If we have framebuffer info, use it */
    if (fb_addr != 0 && fb_width > 0 && fb_height > 0) {
        uint32_t* fb = (uint32_t*)fb_addr;
        
        /* Fill screen with blue to indicate successful framebuffer detection */
        for (uint32_t y = 0; y < fb_height; y++) {
            for (uint32_t x = 0; x < fb_width; x++) {
                uint32_t color = 0xFF0000FF; // Blue background
                fb[y * (fb_pitch / 4) + x] = color;
            }
        }
        
        /* Draw a white border */
        for (uint32_t x = 0; x < fb_width; x++) {
            fb[0 * (fb_pitch / 4) + x] = 0xFFFFFFFF; // Top
            fb[(fb_height-1) * (fb_pitch / 4) + x] = 0xFFFFFFFF; // Bottom
        }
        for (uint32_t y = 0; y < fb_height; y++) {
            fb[y * (fb_pitch / 4) + 0] = 0xFFFFFFFF; // Left
            fb[y * (fb_pitch / 4) + (fb_width-1)] = 0xFFFFFFFF; // Right
        }
        
        /* Draw some text-like pattern */
        for (uint32_t y = 100; y < 200; y += 20) {
            for (uint32_t x = 100; x < 400; x += 10) {
                fb[y * (fb_pitch / 4) + x] = 0xFFFFFFFF;
            }
        }
    } else {
        /* Fallback: try to write to common framebuffer addresses */
        uint32_t* fb_addresses[] = { (uint32_t*)0xFD000000, (uint32_t*)0xE0000000, NULL };
        for (int i = 0; fb_addresses[i] != NULL; i++) {
            uint32_t* fb = fb_addresses[i];
            for (uint32_t j = 0; j < 1024 * 768; j++) {
                fb[j] = 0xFFFF0000; // Red screen = no framebuffer
            }
        }
    }
    
    /* Halt */
    asm("hlt");
    while (1) {
        asm("hlt");
    }
}

/* Stub function for 64-bit build - not implemented */
void redraw_cursor_area_kernel(int x, int y) {
    (void)x;
    (void)y;
    /* Do nothing in 64-bit stub */
}
