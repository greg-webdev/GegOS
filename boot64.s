; boot64.s - 64-bit Multiboot2 bootloader for GegOS
; Assembler: NASM
; Target: x86-64 (64-bit)
; Enters long mode and boots 64-bit kernel

; Multiboot 2 constants
MB2_MAGIC       equ 0xe85250d6
MB2_ARCH        equ 0         ; i386 protected mode
MB2_HEADER_LEN  equ header_end - header_start
MB2_CHECKSUM    equ 0x100000000 - (MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN)

; Multiboot2 header section
section .multiboot
header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM

    ; Framebuffer tag
    align 8
    dw 5                    ; Type: framebuffer
    dw 0                    ; Flags
    dd 20                   ; Size
    dd 1024                 ; Width
    dd 768                  ; Height
    dd 32                   ; Depth

    ; End tag
    align 8
    dw 0                    ; Type: end
    dw 0                    ; Flags
    dd 8                    ; Size
header_end:

; Stack section - 16KB stack
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; GDT and page tables in data section
section .data
align 8

; 64-bit GDT
gdt64:
    dq 0                        ; Null descriptor
    dq 0x00af9a000000ffff       ; Code 64-bit
    dq 0x00af92000000ffff       ; Data 64-bit
gdt64_end:

gdtr:
    dw gdt64_end - gdt64 - 1
    dq gdt64

; Page tables for 1GB identity mapping
align 4096
pml4_table:
    dq pdp_table + 3
    times 511 dq 0

align 4096
pdp_table:
    dq 0x00000083               ; 1GB page
    times 511 dq 0

; Text section - kernel entry
section .text
bits 32
global start64
extern kernel64_main

start64:
    cli
    
    ; Check 64-bit CPU support
    mov eax, 0x80000001
    cpuid
    test edx, 0x20000000        ; Check LM bit
    jz no_64bit
    
    ; Disable paging
    mov eax, cr0
    and eax, 0x7fffffff
    mov cr0, eax
    
    ; Load GDT
    lgdt [rel gdtr]
    
    ; Enable PAE (CR4.PAE = 1)
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    
    ; Load page tables
    mov eax, pml4_table
    mov cr3, eax
    
    ; Enable long mode (EFER.LME = 1)
    mov ecx, 0xc0000080
    rdmsr
    or eax, 0x100
    wrmsr
    
    ; Enable paging (CR0.PG = 1)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ; Jump to 64-bit mode
    jmp 0x08:start64_long

bits 64
start64_long:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rsp, stack_top
    
    mov rdi, rbx                ; Multiboot info in RDI (first arg)
    mov rsi, rdx                ; Magic number in RSI (second arg)
    
    ; Call kernel64_main(multiboot_info, magic)
    call kernel64_main
    
    cli
    hlt

no_64bit:
    ; Display error message - simplified for 64-bit
    mov rax, 0x4f204f204f204f20  ; Print spaces for delay
    mov rbx, 0x4f204f204f204f20
    mov rcx, 0x4f204f204f204f20
    cli
    hlt
