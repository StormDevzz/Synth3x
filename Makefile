# Synth3x OS v0.9 — Makefile (Wayland Compositor + DRM/KMS)
ASM      = as
CC64     = gcc
CFLAGS   = -O2 -Wall -Wextra -I src/kernel -I src/lib -I src/compositor -I/usr/include/libdrm -I/usr/include/drm
LDFLAGS  = -lpthread -lrt -lm -ldrm

BUILD    = build
COMPOSITOR = $(BUILD)/synth3x
INIT     = $(BUILD)/init
SYN_CMD  = $(BUILD)/syn
WHO_BINS = $(BUILD)/ram_analyzer $(BUILD)/disk_analyzer $(BUILD)/device_names $(BUILD)/usb_analyzer $(BUILD)/cable_analyzer
CHECK_BINS = $(BUILD)/checks/check_display $(BUILD)/checks/check_keyboard $(BUILD)/checks/check_mouse $(BUILD)/checks/check_sound
CMD_BINS = $(BUILD)/commands/reboot $(BUILD)/commands/shutdown

.PHONY: all clean iso run

all: $(COMPOSITOR) $(INIT) $(SYN_CMD) $(WHO_BINS) $(CHECK_BINS) $(CMD_BINS)

# ─── Compositor (Wayland + DRM/KMS + AmnesiaDE shell) ───
COMPOSITOR_SRC = \
	src/compositor/main.c \
	src/compositor/drm.c \
	src/compositor/input.c \
	src/compositor/wl_server.c \
	src/compositor/shell.c \
	src/compositor/render.S \
	src/compositor/font.S

$(COMPOSITOR): $(COMPOSITOR_SRC)
	@mkdir -p $(BUILD)
	$(CC64) $(CFLAGS) -o $@ $(COMPOSITOR_SRC) $(LDFLAGS)
	@echo "  ── Compositor (Wayland): $@"

# ─── Init (PID 1, userspace) ───
INIT_SRC = src/init/init.c src/init/splash.S src/compositor/font.S
INIT_HW  = src/hardware/hw_cpuid.S src/hardware/hw_detect.c

$(INIT): $(INIT_SRC) $(INIT_HW)
	@mkdir -p $(BUILD)
	$(CC64) -static -O2 -Wall -o $@ $(INIT_SRC) $(INIT_HW) -lpthread -lrt
	@echo "  ── Init (PID 1): $@"

# ─── Package Manager (syn) ───
$(SYN_CMD): src/commands/syn.c
	@mkdir -p $(BUILD)
	$(CC64) -static -O2 -Wall -o $@ $< -lpthread
	@echo "  ── syn package manager: $@"

# ─── Hardware analyzers ───
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

# ─── Check utilities ───
$(BUILD)/checks/check_display: src/checks/check_display.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/checks/check_keyboard: src/checks/check_keyboard.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/checks/check_mouse: src/checks/check_mouse.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/checks/check_sound: src/checks/check_sound.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -O2 -Wall -o $@ $<

# ─── Commands (reboot, shutdown) ───
$(BUILD)/commands/reboot: src/commands/reboot.c
	@mkdir -p $(BUILD)/commands
	$(CC64) -static -O2 -Wall -o $@ $<
$(BUILD)/commands/shutdown: src/commands/shutdown.c
	@mkdir -p $(BUILD)/commands
	$(CC64) -static -O2 -Wall -o $@ $<

# ─── Initramfs ───
INITRAMFS = $(BUILD)/initrd.img
$(INITRAMFS): $(COMPOSITOR) $(INIT) $(SYN_CMD) $(WHO_BINS) $(CHECK_BINS) $(CMD_BINS)
	@echo "  ── Building initramfs..."
	@rm -rf $(BUILD)/initramfs
	@mkdir -p $(BUILD)/initramfs/bin $(BUILD)/initramfs/sbin
	@mkdir -p $(BUILD)/initramfs/usr/bin $(BUILD)/initramfs/usr/sbin
	@mkdir -p $(BUILD)/initramfs/dev $(BUILD)/initramfs/proc
	@mkdir -p $(BUILD)/initramfs/sys $(BUILD)/initramfs/tmp
	@mkdir -p $(BUILD)/initramfs/etc $(BUILD)/initramfs/var/db/syn
	@mkdir -p $(BUILD)/initramfs/var/cache/syn
	@mkdir -p $(BUILD)/initramfs/usr/local/bin
	@mkdir -p $(BUILD)/initramfs/lib/modules
	@cp $(INIT) $(BUILD)/initramfs/init
	@cp $(COMPOSITOR) $(BUILD)/initramfs/usr/bin/synth3x
	@cp $(SYN_CMD) $(BUILD)/initramfs/usr/bin/syn
	@cp $(WHO_BINS) $(BUILD)/initramfs/usr/bin/
	@cp scripts/synth3x-installer.sh $(BUILD)/initramfs/usr/bin/synth3x-installer
	@cp $(CMD_BINS) $(BUILD)/initramfs/bin/
	@cp $(CHECK_BINS) $(BUILD)/initramfs/usr/bin/
	@cp src/checks/tor-start.sh $(BUILD)/initramfs/usr/bin/tor-start
	@cp src/checks/check_drivers.sh $(BUILD)/initramfs/usr/bin/check-drivers-all
	@cp boot/torrc $(BUILD)/initramfs/etc/torrc 2>/dev/null || true
	@cp boot/nftables.rules $(BUILD)/initramfs/etc/nftables.rules 2>/dev/null || true
	
	# Busybox
	@cp /bin/busybox $(BUILD)/initramfs/bin/busybox 2>/dev/null || true
	@cd $(BUILD)/initramfs && ln -sf busybox bin/sh 2>/dev/null; \
		for app in ls cat mount umount ps kill mkdir cp mv rm dmesg; do \
			ln -sf /bin/busybox bin/$$app 2>/dev/null; done
	
	# Tor + nft + deps
	@cp /usr/bin/tor $(BUILD)/initramfs/usr/bin/tor 2>/dev/null || true
	@cp /usr/sbin/nft $(BUILD)/initramfs/usr/sbin/nft 2>/dev/null || true
	
	# Shared libs for dynamically-linked compositor
	@mkdir -p $(BUILD)/initramfs/lib64 $(BUILD)/initramfs/lib
	@ldd $(COMPOSITOR) 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | sort -u | while read lib; do \
		dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
		cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done
	@ldd $(COMPOSITOR) 2>/dev/null | grep -o '/[^ ]*ld-linux[^ ]*' | sort -u | while read lib; do \
		dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
		cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done
	
	@cd $(BUILD)/initramfs && find . | cpio -H newc -o --quiet > ../initramfs.cpio 2>/dev/null
	@gzip -f $(BUILD)/initramfs.cpio 2>/dev/null; \
		mv $(BUILD)/initramfs.cpio.gz $(INITRAMFS) 2>/dev/null; \
		true
	@rm -rf $(BUILD)/initramfs
	@echo "  ── Initramfs: $(INITRAMFS)"

# ─── ISO ───
iso: $(INITRAMFS)
	@echo "  ── Building ISO..."
	@mkdir -p iso/boot/grub
	@cp /boot/vmlinuz-linux iso/boot/vmlinuz-linux 2>/dev/null || true
	@cp $(INITRAMFS) iso/boot/initrd.img 2>/dev/null || true
	@cp boot/grub.cfg iso/boot/grub/ 2>/dev/null || true
	@if command -v grub-mkrescue >/dev/null 2>&1; then \
		grub-mkrescue -o iso/synth3x-os.iso iso 2>/dev/null; \
		echo "  ── ISO: iso/synth3x-os.iso"; \
	else \
		echo "  ── grub-mkrescue not found"; \
	fi

# ─── Run in QEMU ───
run: $(INITRAMFS)
	qemu-system-x86_64 -kernel /boot/vmlinuz-linux -initrd $(INITRAMFS) \
		-m 1024 -accel kvm -vga virtio -device virtio-tablet \
		-net nic,model=virtio -net user -soundhw hda \
		-append "loglevel=3 console=tty0"

run-iso: iso
	qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm \
		-vga virtio -device virtio-tablet -net nic,model=virtio -net user

# ─── Clean ───
clean:
	@rm -rf $(BUILD) iso
	@echo "  ── Cleaned"
