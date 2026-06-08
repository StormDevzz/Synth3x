# Gentoo Linux: Gentoo Hardened: SELinux Integration

SELinux (Security-Enhanced Linux) enforces mandatory access control (MAC) over all files and processes.

---

## Detailed Explanation

Gentoo provides SELinux profiles which compile packages with SELinux support automatically:
```bash
eselect profile set default/linux/amd64/17.1/hardened/selinux
```
Verify labels using `ls -Z`.

---

## Best Practices & Tips

> [!TIP]
> Configuring correct SELinux policies prevents unauthorized service escalations.
