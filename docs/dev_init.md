# Developer Guide: Init System (syninit)

Synth3x boots directly into a custom PID 1 init manager written in C and assembly, designed for amnesic RAM-only operations and system staging.

---

## 1. Initial boot stage (`syninit`)

The source code is located in `src/init/init.c`.
When the kernel boots, it hands control to `syninit`:
1.  **Mount Virtual Filesystems:** Mounts `/proc`, `/sys`, `/dev` (devtmpfs), and `/dev/pts`.
2.  **Hardware Probing:** Calls hardware detection procedures (`src/hardware/hw_detect.c`) using low-level Assembly code (`src/hardware/hw_cpuid.S`) to verify CPU flags.
3.  **Start Services:** Spawns the Rust service manager (`synit-svc`).
4.  **Tty Configuration:** Sets up standard loop input/output and spawns terminal environments.

---

## 2. Service Management (`synit-svc`)

Service descriptors are compiled into or managed by the service daemon (`src/init/svc/`). To define a service:
1.  Identify the command and privileges required.
2.  Configure security profiles (e.g. dropping root privileges to target user or sandbox).
3.  Add it to the autostart group in `src/init/svc/src/main.rs`.

---

## 3. Rebuilding the Boot Sequence

If you edit the init structure, rebuild it via:
```bash
make build/init
```
It is statically linked, ensuring it does not fail due to shared library mismatches in the early initramfs boot environment.
