# Makefile for GegOS - 32-bit x86

KERNEL_BIN = gegos.bin
ISO_NAME = GegOS.iso
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir

ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
CC = i686-elf-gcc
LD = i686-elf-ld
else
CC = gcc
LD = ld
endif

AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -fno-exceptions -fno-stack-protector -fno-pic -fno-pie -m32
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib -m elf_i386

ASM_SOURCES = boot.s
C_SOURCES = kernel.c vga.c keyboard.c mouse.c gui.c apps.c terminal.c pong.c snake.c game_2048.c

ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

.PHONY: all clean iso run dirs

all: $(ISO_NAME)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub

$(BUILD_DIR)/%.o: %.s | dirs
	@echo "AS    $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c | dirs
	@echo "CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_BIN): $(OBJECTS) linker.ld
	@echo "LD    $(KERNEL_BIN)"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@grub-file --is-x86-multiboot $@ && echo "Multiboot: VALID" || (echo "ERROR: Multiboot invalid!"; exit 1)

$(ISO_NAME): $(BUILD_DIR)/$(KERNEL_BIN) grub.cfg | dirs
	@echo "Creating ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "Build complete: $(ISO_NAME)"

iso: $(ISO_NAME)

run: $(ISO_NAME)
	@qemu-system-i386 -cdrom $(ISO_NAME)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(ISO_NAME)
	@echo "Clean complete."
