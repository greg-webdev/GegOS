# Makefile for GegOS
# A 32-bit x86 Hobby Operating System

# ============================================================================
# Configuration
# ============================================================================

# Output names
KERNEL_BIN = gegos.bin
ISO_NAME = GegOS.iso

# Directories
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir

# ============================================================================
# Toolchain Configuration
# ============================================================================

# Try to use i686-elf cross-compiler if available, otherwise use system gcc
# with appropriate flags for freestanding compilation
ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC = i686-elf-gcc
    LD = i686-elf-ld
    CROSS_COMPILE = 1
else
    CC = gcc
    LD = ld
    CROSS_COMPILE = 0
endif

# Assembler
AS = nasm

# ============================================================================
# Compiler Flags
# ============================================================================

# Common C flags for freestanding kernel development
CFLAGS = -std=gnu99 \
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

# Additional flags for non-cross-compiler
ifeq ($(CROSS_COMPILE), 0)
    CFLAGS += -fno-builtin \
              -nostdlib \
              -nodefaultlibs
endif

# Assembler flags
ASFLAGS = -f elf32

# Linker flags
LDFLAGS = -T linker.ld \
          -nostdlib \
          -m elf_i386

# ============================================================================
# Source Files
# ============================================================================

# Assembly sources
ASM_SOURCES = boot.s

# C sources
C_SOURCES = kernel.c vga.c keyboard.c mouse.c gui.c apps.c terminal.c

# Object files
ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean iso run run-debug dirs help check-deps

# Default target: build the ISO
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

# Create the bootable ISO
$(ISO_NAME): $(BUILD_DIR)/$(KERNEL_BIN) grub.cfg | dirs
	@echo "Creating bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "================================================"
	@echo "Build complete: $(ISO_NAME)"
	@echo "================================================"

# Build the ISO (explicit target)
iso: $(ISO_NAME)

# Run in QEMU (quick testing)
run: $(ISO_NAME)
	@echo "Starting QEMU..."
	@qemu-system-i386 -cdrom $(ISO_NAME)

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
	@echo "GegOS Build System"
	@echo "=================="
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the bootable ISO (default)"
	@echo "  iso        - Build the bootable ISO"
	@echo "  clean      - Remove build artifacts"
	@echo "  run        - Run in QEMU"
	@echo "  run-debug  - Run in QEMU with debug output"
	@echo "  run-vbox   - Run in VirtualBox"
	@echo "  check-deps - Check for required dependencies"
	@echo "  help       - Display this help message"
	@echo ""
	@echo "Toolchain:"
ifeq ($(CROSS_COMPILE), 1)
	@echo "  Using cross-compiler: i686-elf-gcc"
else
	@echo "  Using system compiler: gcc (with freestanding flags)"
endif
	@echo ""
	@echo "Required packages:"
	@echo "  - nasm (assembler)"
	@echo "  - gcc or i686-elf-gcc (compiler)"
	@echo "  - grub-pc-bin, grub-common (bootloader)"
	@echo "  - xorriso, mtools (ISO creation)"
	@echo "  - qemu-system-x86 (optional, for testing)"
