# Synth3x

[Leer en Español (Spanish)](README_ES.md) | [阅读中文 (Chinese)](README_ZH.md)

<!-- git configuration update -->

Synth3x is an amnesic, hardened, source-based operating system powered by Gentoo Linux, a custom C/Assembly Wayland compositor (`AmnesiaDE`), and a Rust-based safe process installer.

---

## Quick Start

### Build the Live ISO
```bash
./scripts/build_iso.sh
```

### Run in QEMU
```bash
make run-iso          # Boot the Live environment
make run-installer    # Boot directly into the terminal installer
```

---

## Read the Docs (Mandatory)

Synth3x is a source-based OS that expects you to understand what is happening under the hood. Before you build, configure, or install the OS, you **must** read the relevant documentation:

*   **How do I install the operating system?**
    Follow the step-by-step pipeline in the [Gentoo Installation Guide](docs/gentoo_installation.md).
*   **How do I download Stage3 or fetch packages?**
    Learn how to fetch clean environments in the [Stage3 Download Guide](docs/gentoo_download_stage3.md) and connect to [Global Download Mirrors](docs/gentoo_download_mirrors.md).
*   **How do I configure USE flags and manage packages?**
    Read the [Portage Package Manager Guide](docs/gentoo_portage.md) to control how your packages compile.
*   **How do I optimize system flags and make compilation faster?**
    Configure `/etc/portage/make.conf` using the [make.conf Configuration Guide](docs/gentoo_makeconfig.md).
*   **My compile times are slow. How do I speed them up?**
    Implement these [10 Ways to Speed Up Gentoo](docs/gentoo_download_speedup.md) to compile in RAM, enable parallel downloading, and configure cache.
*   **How do I configure services or build a custom kernel?**
    Read the [Kernel Configuration Guide](docs/gentoo_kernel.md) and the [OpenRC Service Management Guide](docs/gentoo_openrc.md).
*   **Something failed to build. How do I debug it?**
    Consult the [Portage Troubleshooting Guide](docs/gentoo_troubleshooting.md) for OOM issues, masked packages, and dependency conflicts.
*   **Want an overview of the architecture?**
    Read the [Gentoo Intro & Philosophy Guide](docs/gentoo_intro.md).
*   **How does Gentoo Hardened security work?**
    Read the [Gentoo Hardened Security Guide](docs/gentoo_security.md).

---

## Developer Guides (Mandatory for Developers)

If you are developing applications, packages, or modifications for Synth3x, read these specialized guides:

*   **How do I develop for the Wayland compositor?**
    Read the [Compositor Developer Guide](docs/dev_compositor.md).
*   **How do I write and package custom applications?**
    Read the [Applications Developer Guide](docs/dev_applications.md).
*   **How does early boot and init sequence work?**
    Read the [Init System Developer Guide](docs/dev_init.md).
*   **How do I modify the system installer sequence?**
    Read the [System Installer Developer Guide](docs/dev_installer.md).
*   **How do I create custom packaging for the syn package manager?**
    Read the [Custom Packaging Developer Guide](docs/dev_packaging.md).

---

## Friends

See [FRIENDS.md](FRIENDS.md) for a list of projects and people we stand with.
