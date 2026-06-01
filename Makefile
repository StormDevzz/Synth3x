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

all: $(KERNEL) $(INIT) $(SYNTH3X) $(WHO_BINS) $(SYN_CMD)

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

# ─── Hardware detection (assembly + C) ───
HW_ASM = src/hardware/hw_cpuid.S
HW_SRC = src/hardware/hw_detect.c
$(BUILD)/hw_detect.o: $(HW_ASM) $(HW_SRC)
	$(CC64) -static -O2 -Wall -c -o $(BUILD)/hw_cpuid.o src/hardware/hw_cpuid.S
	$(CC64) -static -O2 -Wall -c -o $@ src/hardware/hw_detect.c

# ─── Init (userspace, 64-bit, static for initramfs) ───
INIT_ASM = src/init/splash.S src/synth3x/font.S
$(INIT): src/init/init.c $(INIT_ASM) $(HW_ASM) $(HW_SRC)
	$(CC64) -static -march=x86-64 -mno-avx -mno-avx2 -mno-sse4 -O2 -Wall -o $@ src/init/init.c src/init/splash.S src/synth3x/font.S src/hardware/hw_cpuid.S src/hardware/hw_detect.c -lpthread -lrt
	@echo "  ── Init: $@"

# ─── Synth3x DE (static for initramfs) ───
SYNTH3X_ASM = src/synth3x/font.S
$(SYNTH3X): src/synth3x/synth3x.c $(SYNTH3X_ASM)
	$(CC64) -static -O2 -Wall -o $@ src/synth3x/synth3x.c $(SYNTH3X_ASM) -lpthread -lrt -lm
	@echo "  ── Synth3x DE: $@"

# ─── System Commands ───
SYN_CMD = $(BUILD)/syn
WHO_BINS = $(BUILD)/ram_analyzer $(BUILD)/disk_analyzer $(BUILD)/device_names $(BUILD)/usb_analyzer $(BUILD)/cable_analyzer

$(SYN_CMD): src/commands/syn.c
	$(CC64) -static -O2 -Wall -o $@ $<
	@echo "  ── syn command: $@"

$(BUILD)/ram_analyzer: src/who/ram_analyzer.c
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/disk_analyzer: src/who/disk_analyzer.c
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/device_names: src/who/device_names.c
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/usb_analyzer: src/who/usb_analyzer.c
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/cable_analyzer: src/who/cable_analyzer.c
	$(CC64) -static -O2 -Wall -o $@ $<
# ─── Initramfs ───
INITRAMFS = $(BUILD)/initrd.img
$(INITRAMFS): $(INIT) $(SYNTH3X) $(WHO_BINS) $(SYN_CMD)
	@echo "  ── Building initramfs..."
	@mkdir -p $(BUILD)/initramfs/bin $(BUILD)/initramfs/usr/bin
	@mkdir -p $(BUILD)/initramfs/dev $(BUILD)/initramfs/proc
	@mkdir -p $(BUILD)/initramfs/sys $(BUILD)/initramfs/tmp
	@cp $(INIT) $(BUILD)/initramfs/init
	@cp $(SYNTH3X) $(BUILD)/initramfs/usr/bin/synth3x
	@cp $(SYN_CMD) $(BUILD)/initramfs/usr/bin/syn
	@cp $(WHO_BINS) $(BUILD)/initramfs/usr/bin/
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
	@qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm -vga virtio -device virtio-tablet -device usb-ehci -net nic,model=virtio -net user -soundhw hda

run-hdd:
	@qemu-system-x86_64 -drive file=build/synth3x-anon.qcow2,if=virtio -m 1024 -accel kvm -vga virtio -device virtio-tablet -net nic,model=virtio -net user -soundhw hda

# ─── Clean ───
clean:
	@rm -rf $(BUILD) iso
	@echo "  ── Cleaned"
