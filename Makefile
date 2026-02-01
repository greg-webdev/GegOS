# Makefile for GegOS - 32-bit x86 and 64-bit x86-64

KERNEL_BIN = gegos.bin
KERNEL64_BIN = gegos64.bin
ISO_NAME = GegOS.iso
ISO64_NAME = GegOS64.iso
BUILD_DIR = build
BUILD64_DIR = build64
ISO_DIR = $(BUILD_DIR)/isodir
ISO64_DIR = $(BUILD64_DIR)/isodir

ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
CC = i686-elf-gcc
LD = i686-elf-ld
else
CC = gcc
LD = ld
endif

# 64-bit compiler
CC64 = x86_64-linux-gnu-gcc
LD64 = x86_64-linux-gnu-ld

AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -fno-exceptions -fno-stack-protector -fno-pic -fno-pie -m32
CFLAGS64 = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -fno-exceptions -fno-stack-protector -fno-pic -fno-pie -m64
ASFLAGS = -f elf32
ASFLAGS64 = -f elf64
LDFLAGS = -T linker.ld -nostdlib -m elf_i386
LDFLAGS64 = -T linker64.ld -nostdlib -m elf_x86_64

ASM_SOURCES = boot.s
ASM64_SOURCES = boot64.s
C_SOURCES = kernel.c vga.c keyboard.c mouse.c gui.c apps.c network.c wifi.c terminal.c pong.c snake.c game_2048.c
C64_SOURCES = kernel64.c vga.c keyboard.c mouse.c gui.c apps.c network.c wifi.c terminal.c pong.c snake.c game_2048.c

ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

ASM64_OBJECTS = $(patsubst %.s,$(BUILD64_DIR)/%.o,$(ASM64_SOURCES))
C64_OBJECTS = $(patsubst %.c,$(BUILD64_DIR)/%.o,$(C64_SOURCES))
OBJECTS64 = $(ASM64_OBJECTS) $(C64_OBJECTS)

.PHONY: all clean iso run dirs all32 all64

all: $(ISO_NAME) $(ISO64_NAME)

all32: $(ISO_NAME)

all64: $(ISO64_NAME)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	@mkdir -p $(BUILD64_DIR)
	@mkdir -p $(ISO64_DIR)/boot/grub

$(BUILD_DIR)/%.o: %.s | dirs
	@echo "AS    $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c | dirs
	@echo "CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD64_DIR)/%.o: %.s | dirs
	@echo "AS64  $<"
	@$(AS) $(ASFLAGS64) $< -o $@

$(BUILD64_DIR)/%.o: %.c | dirs
	@echo "CC64  $<"
	@$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_BIN): $(OBJECTS) linker.ld
	@echo "LD    $(KERNEL_BIN)"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@grub-file --is-x86-multiboot $@ && echo "Multiboot: VALID" || (echo "ERROR: Multiboot invalid!"; exit 1)

$(BUILD64_DIR)/$(KERNEL64_BIN): $(OBJECTS64) linker64.ld
	@echo "LD64  $(KERNEL64_BIN)"
	@$(LD64) $(LDFLAGS64) $(OBJECTS64) -o $@
	@grub-file --is-x86-multiboot $@ && echo "Multiboot64: VALID" || (echo "ERROR: Multiboot64 invalid!"; exit 1)

$(ISO_NAME): $(BUILD_DIR)/$(KERNEL_BIN) grub.cfg | dirs
	@echo "Creating 32-bit ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "Build complete: $(ISO_NAME)"

$(ISO64_NAME): $(BUILD64_DIR)/$(KERNEL64_BIN) grub64.cfg | dirs
	@echo "Creating 64-bit ISO..."
	@mkdir -p $(ISO64_DIR)/boot/grub
	@cp $(BUILD64_DIR)/$(KERNEL64_BIN) $(ISO64_DIR)/boot/
	@cp grub64.cfg $(ISO64_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $@ $(ISO64_DIR) 2>/dev/null
	@echo "Build complete: $(ISO64_NAME)"

iso: $(ISO_NAME)

run: $(ISO_NAME)
	@qemu-system-i386 -cdrom $(ISO_NAME)

run64: $(ISO64_NAME)
	@qemu-system-x86_64 -cdrom $(ISO64_NAME)

clean:
	@rm -rf $(BUILD_DIR) $(BUILD64_DIR)
	@rm -f $(ISO_NAME) $(ISO64_NAME)
	@echo "Clean complete."
