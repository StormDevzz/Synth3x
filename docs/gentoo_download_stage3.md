# Downloading and Selecting a Stage3 Archive

A Stage3 archive is a tarball containing a minimal working Gentoo environment. It is used to bootstrap the system installation.

---

## 1. Choosing the Right Stage3 Variant

Gentoo builds multiple Stage3 flavors. Select the one matching your architecture and profile goals:

*   **stage3-amd64-openrc-latest.tar.xz:** Standard amd64 system with OpenRC init system. Best for general use and default installations.
*   **stage3-amd64-hardened+openrc-latest.tar.xz:** Hardened build with security features enabled in compiler (PIE, SSP) and system libraries.
*   **stage3-amd64-desktop-systemd-latest.tar.xz:** Optimized for desktop setups using the systemd init system.

---

## 2. Downloading via Terminal

Use `curl` with redirected output to download the latest Stage3 OpenRC archive:
```bash
curl -L -O https://bouncer.gentoo.org/fetch/root/all/releases/amd64/autobuilds/current-stage3-amd64-openrc/stage3-amd64-openrc-latest.tar.xz
```
Verify the SHA512 hash file before extraction:
```bash
sha512sum -c stage3-amd64-openrc-latest.tar.xz.sha512
```
