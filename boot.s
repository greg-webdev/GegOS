; boot.s - Multiboot compliant entry point for GegOS
; Assembler: NASM
; Target: i686 (32-bit x86)

; Multiboot 1 constants
MBALIGN     equ  1 << 0                 ; Align loaded modules on page boundaries
MEMINFO     equ  1 << 1                 ; Provide memory map
FLAGS       equ  MBALIGN | MEMINFO      ; Multiboot flag field
MAGIC       equ  0x1BADB002             ; Multiboot magic number
CHECKSUM    equ -(MAGIC + FLAGS)        ; Checksum (magic + flags + checksum = 0)

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
    resb 16384                          ; 16 KB stack
stack_top:

; Text section - kernel entry point
section .text
global _start
extern kernel_main

_start:
    ; Set up the stack pointer
    mov esp, stack_top

    ; Reset EFLAGS
    push 0
    popf

    ; Push multiboot info pointer (ebx) and magic number (eax)
    ; These are passed by the bootloader
    push ebx                            ; Multiboot info structure pointer
    push eax                            ; Multiboot magic number

    ; Call the C kernel entry point
    call kernel_main

    ; If kernel_main returns, hang the CPU
    cli                                 ; Disable interrupts
.hang:
    hlt                                 ; Halt the CPU
    jmp .hang                           ; Loop forever if interrupted
