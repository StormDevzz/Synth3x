# Synth3x-Anon — Makefile
ASM      = as
CC64     = gcc
VERSION  := $(shell cat VERSION 2>/dev/null || echo "0.8.1")
CFLAGS   = -march=x86-64 -mno-avx -O2 -Wall -Wextra -DVERSION=\"$(VERSION)\" -I src/kernel -I src/lib -I src/compositor -I src/installer -I/usr/include/libdrm -I/usr/include/drm
LDFLAGS  = -lpthread -lrt -lm -ldrm

BUILD    = build
COMPOSITOR = $(BUILD)/synth3x
INIT     = $(BUILD)/init
SYN_CMD  = $(BUILD)/syn
WHO_BINS = $(BUILD)/ram_analyzer $(BUILD)/disk_analyzer $(BUILD)/device_names $(BUILD)/usb_analyzer $(BUILD)/cable_analyzer
CHECK_BINS = $(BUILD)/checks/check_display $(BUILD)/checks/check_keyboard $(BUILD)/checks/check_mouse $(BUILD)/checks/check_sound
CMD_BINS = $(BUILD)/commands/reboot $(BUILD)/commands/shutdown

# Installer C components
INSTALLER_DOWNLOADER = $(BUILD)/synth3x-downloader
INSTALLER_WIFI       = $(BUILD)/synth3x-wifi

# Rust build
CARGO    = cargo
RUST_DIR = src/lib
RUST_INSTALLER = $(BUILD)/synth3x-installer

# ASM fast scanner (no libc, static)
INSTALLER_FASTSCAN = $(BUILD)/synth3x-fastscan

.PHONY: all clean iso run rust

all: $(COMPOSITOR) $(INIT) $(SYN_CMD) $(WHO_BINS) $(CHECK_BINS) $(CMD_BINS) $(INSTALLER_DOWNLOADER) $(INSTALLER_WIFI) rust $(INSTALLER_FASTSCAN) boot/grub.cfg

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
	$(CC64) -static -O2 -Wall -mno-avx -mno-avx2 -DVERSION=\"$(VERSION)\" -o $@ $(INIT_SRC) $(INIT_HW) -lpthread -lrt
	@echo "  ── Init (PID 1): $@"

# ─── Package Manager (syn) ───
$(SYN_CMD): src/commands/syn.c
	@mkdir -p $(BUILD)
	$(CC64) -static -O2 -Wall -mno-avx -mno-avx2 -o $@ $< -lpthread
	@echo "  ── syn package manager: $@"

# ─── Hardware analyzers ───
$(BUILD)/ram_analyzer: src/who/ram_analyzer.c
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/disk_analyzer: src/who/disk_analyzer.c
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/device_names: src/who/device_names.c
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/usb_analyzer: src/who/usb_analyzer.c
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/cable_analyzer: src/who/cable_analyzer.c
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<

# ─── Check utilities ───
$(BUILD)/checks/check_display: src/checks/check_display.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/checks/check_keyboard: src/checks/check_keyboard.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/checks/check_mouse: src/checks/check_mouse.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/checks/check_sound: src/checks/check_sound.c
	@mkdir -p $(BUILD)/checks
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<

# ─── Commands (reboot, shutdown) ───
$(BUILD)/commands/reboot: src/commands/reboot.c
	@mkdir -p $(BUILD)/commands
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<
$(BUILD)/commands/shutdown: src/commands/shutdown.c
	@mkdir -p $(BUILD)/commands
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -o $@ $<

# ─── Installer C components ───
$(INSTALLER_DOWNLOADER): src/installer/downloader.c
	@mkdir -p $(BUILD)
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -DVERSION=\"$(VERSION)\" -DSTANDALONE -o $@ $< -lpthread
	@echo "  ── Installer downloader: $@"

$(INSTALLER_WIFI): src/installer/wifi_manager.c
	@mkdir -p $(BUILD)
	$(CC64) -static -march=x86-64 -mno-avx -O2 -Wall -DVERSION=\"$(VERSION)\" -DSTANDALONE -o $@ $< -lpthread
	@echo "  ── Installer WiFi manager: $@"

# ─── ASM fast scanner (statically linked, no libc) ───
$(INSTALLER_FASTSCAN): src/installer/fast_scan.S
	@mkdir -p $(BUILD)
	as -o $(BUILD)/fast_scan.o $<
	ld -o $@ $(BUILD)/fast_scan.o
	@rm -f $(BUILD)/fast_scan.o
	@echo "  ── ASM WiFi fast scanner: $@"

# ─── Rust safe components ───
$(RUST_INSTALLER): $(wildcard $(RUST_DIR)/synth3x-installer/src/*.rs) $(wildcard $(RUST_DIR)/synth3x-safe/src/*.rs)
	@echo "  ── Building Rust components..."
	$(CARGO) build --release --manifest-path $(RUST_DIR)/Cargo.toml 2>&1
	@cp $(RUST_DIR)/target/release/synth3x-installer $(RUST_INSTALLER) 2>/dev/null || true
	@echo "  ── Rust installer: $@"

rust: $(RUST_INSTALLER)

# ─── Initramfs ───
INITRAMFS = $(BUILD)/initrd.img
$(INITRAMFS): $(COMPOSITOR) $(INIT) $(SYN_CMD) $(WHO_BINS) $(CHECK_BINS) $(CMD_BINS) $(INSTALLER_DOWNLOADER) $(INSTALLER_WIFI) $(RUST_INSTALLER)
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
	@cp $(RUST_INSTALLER) $(BUILD)/initramfs/usr/bin/synth3x-installer
	@cp $(INSTALLER_DOWNLOADER) $(BUILD)/initramfs/usr/bin/synth3x-downloader
	@cp $(INSTALLER_WIFI) $(BUILD)/initramfs/usr/bin/synth3x-wifi
	@cp $(INSTALLER_FASTSCAN) $(BUILD)/initramfs/usr/bin/synth3x-fastscan
	@cp $(CMD_BINS) $(BUILD)/initramfs/bin/
	@cp $(CHECK_BINS) $(BUILD)/initramfs/usr/bin/
	@cp src/checks/tor-start.sh $(BUILD)/initramfs/usr/bin/tor-start
	@cp src/checks/check_drivers.sh $(BUILD)/initramfs/usr/bin/check-drivers-all
	@cp boot/torrc $(BUILD)/initramfs/etc/torrc 2>/dev/null || true
	@cp boot/nftables.rules $(BUILD)/initramfs/etc/nftables.rules 2>/dev/null || true
	
	# Busybox
	@cp /bin/busybox $(BUILD)/initramfs/bin/busybox 2>/dev/null || true
	@cd $(BUILD)/initramfs && ln -sf busybox bin/sh 2>/dev/null; \
		for app in ls cat mount umount ps kill mkdir cp mv rm dmesg grep find chmod chown df du echo env export hostname id killall less login more sed sleep sort tail tee test touch uname watch which whoami yes yes clear reset stty; do \
			ln -sf /bin/busybox bin/$$app 2>/dev/null; done
	@cd $(BUILD)/initramfs && mkdir -p sbin; \
		for app in ifconfig route reboot halt poweroff; do \
			ln -sf ../bin/busybox sbin/$$app 2>/dev/null; done
	
	# Bash (needed for interactive shell)
	@cp /usr/bin/bash $(BUILD)/initramfs/bin/bash 2>/dev/null || true
	@for lib in $$(ldd /usr/bin/bash 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | sort -u); do \
		dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
		cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done
	@ln -sf bash $(BUILD)/initramfs/bin/bash.bak 2>/dev/null || true
	
	# Tor + nft + deps
	@cp /usr/bin/tor $(BUILD)/initramfs/usr/bin/tor 2>/dev/null || true
	@cp /usr/sbin/nft $(BUILD)/initramfs/usr/sbin/nft 2>/dev/null || true
	
	# Installer tools: network, disk, boot
	@for bin in iw iwctl wpa_supplicant dhcpcd udhcpc parted mkfs.vfat mkfs.ext4 \
		grub-install grub-mkrescue udevadm ping wget curl tar gzip xz lsblk fdisk; do \
		path=$$(which $$bin 2>/dev/null); \
		if [ -n "$$path" ]; then \
			cp "$$path" "$(BUILD)/initramfs/usr/bin/$$bin" 2>/dev/null || \
			cp "$$path" "$(BUILD)/initramfs/bin/$$bin" 2>/dev/null || true; \
			ldd "$$path" 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | sort -u | while read lib; do \
				dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
				cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done; \
		fi; done
	@for bin in chpasswd useradd usermod groupadd; do \
		path=$$(which $$bin 2>/dev/null); \
		if [ -n "$$path" ]; then \
			cp "$$path" "$(BUILD)/initramfs/usr/sbin/$$bin" 2>/dev/null || true; \
			ldd "$$path" 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | sort -u | while read lib; do \
				dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
				cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done; \
		fi; done
	
	# Synth3x scripts (emerge wrapper, help, etc.)
	@cp src/scripts/emerge-wrapper.sh $(BUILD)/initramfs/usr/bin/emerge 2>/dev/null || true
	@sed 's/@VERSION@/$(VERSION)/g' src/scripts/synth3x-help.sh > $(BUILD)/initramfs/usr/bin/synth3x-help 2>/dev/null || true
	@chmod +x $(BUILD)/initramfs/usr/bin/synth3x-help 2>/dev/null || true
	
	# Shared libs for dynamically-linked binaries
	@mkdir -p $(BUILD)/initramfs/lib64 $(BUILD)/initramfs/lib
	@for bin in $(COMPOSITOR) $(RUST_INSTALLER); do \
		if [ -f "$$bin" ]; then \
			ldd "$$bin" 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | sort -u | while read lib; do \
				dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
				cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done; \
			ldd "$$bin" 2>/dev/null | grep -o '/[^ ]*ld-linux[^ ]*' | sort -u | while read lib; do \
				dir=$$(dirname "$$lib"); mkdir -p "$(BUILD)/initramfs$$dir"; \
				cp -n "$$lib" "$(BUILD)/initramfs$$lib" 2>/dev/null || true; done; \
		fi; done
	
	@cd $(BUILD)/initramfs && find . | cpio -H newc -o --quiet > ../initramfs.cpio 2>/dev/null
	@gzip -f $(BUILD)/initramfs.cpio 2>/dev/null; \
		mv $(BUILD)/initramfs.cpio.gz $(INITRAMFS) 2>/dev/null; \
		true
	@rm -rf $(BUILD)/initramfs
	@echo "  ── Initramfs: $(INITRAMFS)"

# ─── Generated files from VERSION ───
boot/grub.cfg: boot/grub.cfg.in VERSION
	@sed 's/@VERSION@/$(VERSION)/g' $< > $@
	@echo "  ── Generated: $@"

.PHONY: gen-version
gen-version: boot/grub.cfg

# ─── ISO ───
iso: $(INITRAMFS) boot/grub.cfg
	@echo "  ── Building ISO..."
	@mkdir -p iso/boot/grub
	@cp /boot/vmlinuz-linux iso/boot/vmlinuz-linux 2>/dev/null || \
		cp $(BUILD)/vmlinuz-linux iso/boot/vmlinuz-linux 2>/dev/null || \
		{ echo "  ── ERROR: no kernel found (vmlinuz-linux). Copy one to build/vmlinuz-linux"; exit 1; }
	@cp $(INITRAMFS) iso/boot/initrd.img 2>/dev/null || true
	@cp boot/grub.cfg iso/boot/grub/ 2>/dev/null || true
	@if command -v grub-mkrescue >/dev/null 2>&1; then \
		grub-mkrescue -o iso/synth3x.iso iso 2>/dev/null; \
		echo "  ── ISO: iso/synth3x.iso"; \
	else \
		echo "  ── grub-mkrescue not found"; \
	fi

# ─── Run in QEMU ───
KERNEL ?= $(shell test -f /boot/vmlinuz-linux && echo /boot/vmlinuz-linux || echo $(BUILD)/vmlinuz-linux)

run: $(INITRAMFS)
	qemu-system-x86_64 -kernel $(KERNEL) -initrd $(INITRAMFS) \
		-m 1024 -accel kvm -vga virtio -device virtio-tablet \
		-net nic,model=virtio -net user -soundhw hda \
		-append "loglevel=3 console=tty0"

run-iso: iso
	qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm \
		-vga virtio -device virtio-tablet -net nic,model=virtio -net user

run-installer: iso
	qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm \
		-vga virtio -device virtio-tablet -net nic,model=virtio -net user \
		-append "loglevel=3 console=tty0 installer"

# ─── Clean ───
clean:
	@rm -rf $(BUILD) iso
	@$(CARGO) clean --manifest-path $(RUST_DIR)/Cargo.toml 2>/dev/null || true
	@echo "  ── Cleaned"
