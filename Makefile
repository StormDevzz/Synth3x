# Synth3x OS — Makefile
ASM      = as
CC32     = gcc
CC64     = gcc
CFLAGS   = -m32 -ffreestanding -fno-stack-protector -fno-pic -nostdlib -O2 -Wall -Wextra -I src/kernel -I src/lib
ASFLAGS  = --32
LDFLAGS  = -m elf_i386 -T src/kernel/linker.ld -nostdlib

BUILD    = build
KERNEL   = $(BUILD)/synth3x-kernel.elf
INIT     = $(BUILD)/init
SYNTH3X  = $(BUILD)/synth3x

KERNEL_ASM_SRC = $(wildcard src/kernel/*.S)
KERNEL_ASM_OBJ = $(patsubst src/kernel/%.S,$(BUILD)/%.o,$(KERNEL_ASM_SRC))
KERNEL_C_SRC   = $(wildcard src/kernel/*.c)
KERNEL_C_OBJ   = $(patsubst src/kernel/%.c,$(BUILD)/%.o,$(KERNEL_C_SRC))
KERNEL_OBJ     = $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

.PHONY: all clean iso run

all: $(KERNEL) $(INIT) $(SYNTH3X)

# ─── Kernel assembly objects ───
$(BUILD)/%.o: src/kernel/%.S
	@mkdir -p $(BUILD)
	$(ASM) $(ASFLAGS) -o $@ $<

# ─── Kernel C objects ───
$(BUILD)/%.o: src/kernel/%.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c -o $@ $<

$(KERNEL): $(KERNEL_OBJ)
	ld $(LDFLAGS) -o $@ $^
	@echo "  ── Kernel: $@"

# ─── Init (userspace, 64-bit host) ───
$(INIT): src/init/init.c
	$(CC64) -O2 -Wall -o $@ $<
	@echo "  ── Init: $@"

# ─── Synth3x DE ───
$(SYNTH3X): src/synth3x/synth3x.c
	$(CC64) -O2 -Wall -o $@ $< -lpthread -lrt
	@echo "  ── Synth3x DE: $@"

# ─── ISO ───
iso: $(KERNEL) $(INIT) $(SYNTH3X)
	@echo "  ── Building ISO..."
	@mkdir -p iso/boot/grub
	@cp $(KERNEL) iso/boot/
	@cp $(INIT) iso/boot/init
	@mkdir -p iso/usr/bin
	@cp $(SYNTH3X) iso/usr/bin/synth3x
	@cp src/boot/grub.cfg iso/boot/grub/
	@if command -v grub-mkrescue >/dev/null 2>&1; then \
		grub-mkrescue -o iso/synth3x-os.iso iso 2>/dev/null; \
		echo "  ── ISO: iso/synth3x-os.iso ($$(du -h iso/synth3x-os.iso | cut -f1))"; \
	else \
		echo "  ── grub-mkrescue not found"; \
	fi

# ─── Run in QEMU ───
run: iso
	@qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 512 -accel kvm

# ─── Clean ───
clean:
	@rm -rf $(BUILD) iso
	@echo "  ── Cleaned"
