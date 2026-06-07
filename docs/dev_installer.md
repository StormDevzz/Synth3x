# Developer Guide: System Installer

The Synth3x system installer is located under `src/lib/synth3x-installer/` and written in Rust, leveraging type safety and safe abstractions.

---

## 1. Step Pipeline Sequence

The installer executes a sequential pipeline defined inside the `main()` function of `src/lib/synth3x-installer/src/main.rs`.

Each step is defined by:
1.  Banners and prompts (`show_banner()`, `show_step(step_num, total, step_name)`).
2.  Input validation and safety verification (e.g. disk warnings, password match).
3.  Calling the installation functions.

### How steps are structured:
```rust
show_banner();
show_step(8, 11, "Custom Applications Selection");
// ... prompt input ...
// ... parse results ...
```

---

## 2. Base Staging Logic (`install_base`)

The actual copying, directory setups, chrooting, environment profiles (like hostname, timezone, locale), and configurations are performed inside `install_base()`.
*   It mounts the partition.
*   Extracts or downloads Stage3.
*   Configures system variables and copies libraries.
*   Spawns helper commands in the chroot.

To add new post-installation hooks (e.g., config changes), insert commands at the end of the `install_base()` function before partitions are unmounted.
