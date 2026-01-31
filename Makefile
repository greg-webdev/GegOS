# Makefile for GegOS
# A dual 32-bit x86 and 64-bit x86-64 Hobby Operating System

# ============================================================================
# Configuration
# ============================================================================

# Output names
KERNEL_BIN_32 = gegos32.bin
KERNEL_BIN_64 = gegos64.bin
ISO_NAME = GegOS.iso

# Directories
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir

# ============================================================================
# Toolchain Configuration
# ============================================================================

# 32-bit toolchain
ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC32 = i686-elf-gcc
    LD32 = i686-elf-ld
else
    CC32 = gcc
    LD32 = ld
endif

# 64-bit toolchain
ifneq ($(shell which x86_64-elf-gcc 2>/dev/null),)
    CC64 = x86_64-elf-gcc
    LD64 = x86_64-elf-ld
else
    CC64 = gcc
    LD64 = ld
endif

# Assembler
AS = nasm

# ============================================================================
# Compiler Flags
# ============================================================================

# 32-bit C flags
CFLAGS32 = -std=gnu99 \
          -ffreestanding \
          -O2 \
          -Wall \
          -Wextra \
          -Werror \
          -fno-exceptions \
          -fno-stack-protector \
          -fno-pic \
          -fno-pie \
          -m32

# 64-bit C flags
CFLAGS64 = -std=gnu99 \
          -ffreestanding \
          -O2 \
          -Wall \
          -Wextra \
          -Werror \
          -fno-exceptions \
          -fno-stack-protector \
          -fno-pic \
          -fno-pie \
          -m64

# 32-bit assembler flags
ASFLAGS32 = -f elf32

# 64-bit assembler flags
ASFLAGS64 = -f elf64

# 32-bit linker flags
LDFLAGS32 = -T linker.ld \
           -nostdlib \
           -m elf_i386

# 64-bit linker flags
LDFLAGS64 = -T linker64.ld \
           -nostdlib \
           -m elf_x86_64

# ============================================================================
# Source Files
# ============================================================================

# 32-bit assembly and C sources
ASM_SOURCES32 = boot.s
C_SOURCES_COMMON = vga.c keyboard.c mouse.c gui.c apps.c terminal.c pong.c snake.c game_2048.c
C_SOURCES32 = kernel.c $(C_SOURCES_COMMON)

# 64-bit assembly and C sources
ASM_SOURCES64 = boot64.s
C_SOURCES64 = kernel64.c $(C_SOURCES_COMMON)

# Object files
ASM_OBJECTS32 = $(patsubst %.s,$(BUILD_DIR)/%_32.o,$(ASM_SOURCES32))
C_OBJECTS32 = $(patsubst %.c,$(BUILD_DIR)/%_32.o,$(C_SOURCES32))
OBJECTS32 = $(ASM_OBJECTS32) $(C_OBJECTS32)

ASM_OBJECTS64 = $(patsubst %.s,$(BUILD_DIR)/%_64.o,$(ASM_SOURCES64))
C_OBJECTS64 = $(patsubst %.c,$(BUILD_DIR)/%_64.o,$(C_SOURCES64))
OBJECTS64 = $(ASM_OBJECTS64) $(C_OBJECTS64)

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean iso run run-debug dirs help check-deps

# Default target: build the ISO with both kernels
all: $(ISO_NAME)

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub

# Check for required dependencies
check-deps:
	@echo "Checking dependencies..."
	@which nasm > /dev/null 2>&1 || (echo "ERROR: nasm not found. Install with: sudo apt install nasm" && exit 1)
	@which grub-mkrescue > /dev/null 2>&1 || (echo "ERROR: grub-mkrescue not found. Install with: sudo apt install grub-pc-bin grub-common" && exit 1)
	@which xorriso > /dev/null 2>&1 || (echo "ERROR: xorriso not found. Install with: sudo apt install xorriso" && exit 1)
	@echo "All dependencies found!"

# 32-bit compilation
$(BUILD_DIR)/%_32.o: %.s | dirs
	@echo "AS32  $<"
	@$(AS) $(ASFLAGS32) $< -o $@

$(BUILD_DIR)/%_32.o: %.c | dirs
	@echo "CC32  $<"
	@$(CC32) $(CFLAGS32) -c $< -o $@

# 64-bit compilation
$(BUILD_DIR)/%_64.o: %.s | dirs
	@echo "AS64  $<"
	@$(AS) $(ASFLAGS64) $< -o $@

$(BUILD_DIR)/%_64.o: %.c | dirs
	@echo "CC64  $<"
	@$(CC64) $(CFLAGS64) -c $< -o $@

# Link 32-bit kernel
$(BUILD_DIR)/$(KERNEL_BIN_32): $(OBJECTS32) linker.ld
	@echo "LD32  $(KERNEL_BIN_32)"
	@$(LD32) $(LDFLAGS32) $(OBJECTS32) -o $@
	@echo "Verifying Multiboot header (32-bit)..."
	@if grub-file --is-x86-multiboot $@; then \
		echo "Multiboot header (32-bit): VALID"; \
	else \
		echo "WARNING: 32-bit Multiboot header verification failed"; \
	fi

# Link 64-bit kernel
$(BUILD_DIR)/$(KERNEL_BIN_64): $(OBJECTS64) linker64.ld
	@echo "LD64  $(KERNEL_BIN_64)"
	@$(LD64) $(LDFLAGS64) $(OBJECTS64) -o $@
	@echo "Note: 64-bit kernels use custom boot method, not Multiboot"

# Create the bootable ISO with both kernels
$(ISO_NAME): $(BUILD_DIR)/$(KERNEL_BIN_32) $(BUILD_DIR)/$(KERNEL_BIN_64) grub.cfg | dirs
	@echo "Creating dual-kernel ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(KERNEL_BIN_32) $(ISO_DIR)/boot/
	@cp $(BUILD_DIR)/$(KERNEL_BIN_64) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "================================================"
	@echo "Build complete: $(ISO_NAME) (32-bit + 64-bit)"
	@echo "================================================"
	@which xorriso > /dev/null 2>&1 || (echo "ERROR: xorriso not found. Install with: sudo apt install xorriso" && exit 1)
	@echo "All dependencies found!"

# Compile assembly files
$(BUILD_DIR)/%.o: %.s | dirs
	@echo "AS    $<"
	@$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c | dirs
	@echo "CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Link the kernel binary
$(BUILD_DIR)/$(KERNEL_BIN): $(OBJECTS) linker.ld
	@echo "LD    $(KERNEL_BIN)"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Verifying Multiboot header..."
	@if grub-file --is-x86-multiboot $@; then \
		echo "Multiboot header: VALID"; \
	else \
		echo "ERROR: Multiboot header is INVALID!"; \
		exit 1; \
	fi

# Build the ISO (explicit target)
iso: $(ISO_NAME)

# Run 32-bit version in QEMU
run: $(ISO_NAME)
	@echo "Starting QEMU (32-bit)..."
	@qemu-system-i386 -cdrom $(ISO_NAME)

# Run 64-bit version in QEMU
run64: $(ISO_NAME)
	@echo "Starting QEMU (64-bit)..."
	@qemu-system-x86_64 -cdrom $(ISO_NAME)

# Run in QEMU with debug options
run-debug: $(ISO_NAME)
	@echo "Starting QEMU in debug mode..."
	@qemu-system-i386 -cdrom $(ISO_NAME) -d int -no-reboot -no-shutdown

# Run in VirtualBox
run-vbox: $(ISO_NAME)
	@echo "Starting VirtualBox..."
	@VBoxManage createvm --name "GegOS" --ostype "Other" --register 2>/dev/null || true
	@VBoxManage modifyvm "GegOS" --memory 128 --vram 16 --cpus 1 2>/dev/null || true
	@VBoxManage storagectl "GegOS" --name "IDE" --add ide 2>/dev/null || true
	@VBoxManage storageattach "GegOS" --storagectl "IDE" --port 0 --device 0 --type dvddrive --medium "$(PWD)/$(ISO_NAME)" 2>/dev/null || \
		VBoxManage storageattach "GegOS" --storagectl "IDE" --port 0 --device 0 --type dvddrive --medium "$(PWD)/$(ISO_NAME)" --forceunmount 2>/dev/null || true
	@VBoxManage startvm "GegOS"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(ISO_NAME)
	@echo "Clean complete."

# Display help
help:
	@echo "GegOS Dual-Kernel Build System (32-bit + 64-bit)"
	@echo "=================================================="
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the dual-kernel ISO (default)"
	@echo "  iso        - Build the dual-kernel ISO"
	@echo "  clean      - Remove build artifacts"
	@echo "  run        - Run 32-bit in QEMU"
	@echo "  run64      - Run 64-bit in QEMU"
	@echo "  run-debug  - Run in QEMU with debug output"
	@echo "  run-vbox   - Run in VirtualBox"
	@echo "  check-deps - Check for required dependencies"
	@echo "  help       - Display this help message"
	@echo ""
	@echo "The ISO includes both 32-bit and 64-bit kernels."
	@echo "Select from GRUB menu at boot."
	@echo ""
	@echo "Required packages:"
	@echo "  - nasm (assembler)"
	@echo "  - gcc or i686-elf-gcc (32-bit compiler)"
	@echo "  - x86_64-elf-gcc (64-bit compiler, optional)"
	@echo "  - grub-pc-bin, grub-common (bootloader)"
	@echo "  - xorriso, mtools (ISO creation)"
