# Gentoo Hardened Security & Isolation

Gentoo Hardened is a project designed to bring advanced security features to Gentoo Linux. Synth3x runs on the Hardened profile to ensure system integrity, process isolation, and network privacy.

---

## 1. Toolchain Hardening

The hardened compiler profile enforces security flags during build and compilation:
*   **Position Independent Executables (PIE):** Forces binaries to load at random memory addresses, preventing buffer overflow redirect attacks.
*   **Stack Smashing Protector (SSP):** Inserts guard variables (canaries) on the stack to detect and abort executions when stack overflows occur.
*   **Fortify Source:** Replaces unsafe string and memory functions with bounds-checked alternatives at compile time.

---

## 2. Address Space Protection

Gentoo Hardened utilizes kernel-level protections to prevent memory execution exploits:
*   **Address Space Layout Randomization (ASLR):** Randomizes the layout of the address space of active processes (stack, heap, libraries).
*   **Non-Executable Memory (NX/W^X):** Ensures memory pages are either writable or executable, but never both, preventing execution of injected shellcodes.

---

## 3. Network Privacy & Firewalls

Synth3x routes system-wide network traffic through the Tor onion network.
*   **Transparent Proxying:** The custom firewall rules (`/etc/nftables.rules`) redirect all outgoing TCP traffic to the local Tor transparent proxy port (9040).
*   **DNS Leaks Prevention:** All DNS queries are intercepted and forwarded to Tor's secure resolver, preventing tracking by ISPs.
