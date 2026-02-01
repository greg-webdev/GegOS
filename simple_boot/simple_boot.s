; simple_boot.s - Simple bootloader for GegOS (no GRUB)
; Loads the kernel directly from disk

BITS 16
ORG 0x7C00

start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Save boot drive
    mov [boot_drive], dl

    ; Load kernel from disk
    mov ah, 0x02        ; BIOS read sectors
    mov al, 64          ; Read 64 sectors (32KB kernel)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2 (after boot sector)
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]; Drive number
    mov bx, 0x1000      ; Load to 0x1000:0 (0x10000 linear)
    mov es, bx
    xor bx, bx
    int 0x13
    jc load_error

    ; Jump to kernel (0x10000 linear = 0x1000:0)
    jmp 0x1000:0x0000

load_error:
    ; Print error message
    mov si, error_msg
    call print_string
    hlt

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret

error_msg db 'Error loading kernel!', 0
boot_drive db 0

; Pad to 512 bytes and add boot signature
times 510 - ($ - $$) db 0
dw 0xAA55