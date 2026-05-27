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
SYNTH3X_ASM = src/synth3x/fb_asm.S src/synth3x/font.S
$(SYNTH3X): src/synth3x/synth3x.c $(SYNTH3X_ASM)
	$(CC64) -O2 -Wall -o $@ src/synth3x/synth3x.c $(SYNTH3X_ASM) -lpthread -lrt
	@echo "  ── Synth3x DE: $@"

# ─── Initramfs ───
INITRAMFS = $(BUILD)/initrd.img
$(INITRAMFS): $(INIT) $(SYNTH3X)
	@echo "  ── Building initramfs..."
	@mkdir -p $(BUILD)/initramfs/bin $(BUILD)/initramfs/usr/bin
	@cp $(INIT) $(BUILD)/initramfs/init
	@cp $(SYNTH3X) $(BUILD)/initramfs/usr/bin/synth3x
	@cp /usr/bin/busybox $(BUILD)/initramfs/bin/busybox
	@cd $(BUILD)/initramfs && ln -sf busybox bin/sh 2>/dev/null; \
		for app in ls cat mount umount ps kill mkdir cp mv rm dmesg; do \
			ln -sf /bin/busybox bin/$$app 2>/dev/null; done
	@cd $(BUILD)/initramfs && find . | cpio -H newc -o --quiet > ../initramfs.cpio
	@gzip -f $(BUILD)/initramfs.cpio
	@mv $(BUILD)/initramfs.cpio.gz $(INITRAMFS)
	@rm -rf $(BUILD)/initramfs
	@echo "  ── Initramfs: $(INITRAMFS) ($$(du -h $(INITRAMFS) | cut -f1))"

# ─── ISO ───
iso: $(KERNEL) $(INIT) $(SYNTH3X) $(INITRAMFS)
	@echo "  ── Building ISO..."
	@mkdir -p iso/boot/grub
	@cp /boot/vmlinuz-linux iso/boot/vmlinuz-linux
	@cp $(INITRAMFS) iso/boot/initrd.img
	@cp boot/grub.cfg iso/boot/grub/
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
