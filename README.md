# Synth3x

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
