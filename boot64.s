; boot64.s - 64-bit Multiboot bootloader for GegOS
; Assembler: NASM
; Target: x86-64 (64-bit)
; Enters long mode and boots 64-bit kernel

; Multiboot 1 constants
MBALIGN     equ  1 << 0
MEMINFO     equ  1 << 1
FLAGS       equ  MBALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

; Multiboot header section
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

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
    mov ss, ax
    
    mov rsp, stack_top
    
    mov rdi, rbx                ; Multiboot info
    call kernel64_main
    
    cli
    hlt

no_64bit:
    cli
    hlt
