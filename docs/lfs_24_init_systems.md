# Linux From Scratch: Bootscripts & Init Systems (SysVinit vs Systemd)

The init system is PID 1, the first process executed by the kernel. It controls service configuration.

---

## Detailed Explanation

SysVinit uses a serial boot sequence controlled by script folders `/etc/rc.d/`. Systemd uses parallel socket-activated units in `/etc/systemd/system/`.

Installing SysVinit bootscripts:
```bash
tar -xf lfs-bootscripts-20230101.tar.xz
cd lfs-bootscripts-20230101
make install
```

---

## Best Practices & Tips

> [!TIP]
> Ensure default runlevels and interactive shells are configured correctly in `/etc/inittab`.
