# Simple Boot Makefile (no GRUB)

AS = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy

# Flags
ASFLAGS = -f bin
CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector
LDFLAGS = -m elf_i386 -T linker.ld --build-id=none

# Source files
BOOT_SRC = simple_boot.s
KERNEL_SRC = kernel.c vga.c keyboard.c mouse.c gui.c apps.c network.c wifi.c terminal.c pong.c snake.c game_2048.c

# Object files
BOOT_OBJ = simple_boot.o
KERNEL_OBJS = $(KERNEL_SRC:.c=.o)

# Output files
BOOT_BIN = simple_boot.bin
KERNEL_BIN = gegos.bin
ISO_NAME = GegOS-Simple.iso

# Build targets
all: $(ISO_NAME)

$(BOOT_BIN): $(BOOT_SRC)
	nasm -f bin -o $@ $<

$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(ISO_NAME): $(BOOT_BIN) $(KERNEL_BIN)
	mkdir -p iso/boot
	cp $(BOOT_BIN) iso/boot/
	cp $(KERNEL_BIN) iso/boot/
	xorriso -as mkisofs -b boot/$(BOOT_BIN) -no-emul-boot -boot-load-size 4 -o $@ iso/

clean:
	rm -f *.o *.bin *.iso
	rm -rf iso/

.PHONY: all clean