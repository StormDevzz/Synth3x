# Linux From Scratch: Creating Essential Files & Symlinks

Certain symlinks and files are required by compile tools and the shell during stage 3 bootstrapping.

---

## Detailed Explanation

Creating symlinks for directories and dynamic linkers:
```bash
ln -sfv /proc/self/mounts /etc/mtab

# Create default users file
cat > /etc/passwd << "EOF"
root:x:0:0:root:/root:/bin/bash
bin:x:1:1:bin:/dev/null:/bin/false
nobody:x:65534:65534:Nobody:/:/bin/false
EOF

cat > /etc/group << "EOF"
root:x:0:
bin:x:1:daemon
sys:x:2:
wheel:x:10:root
nobody:x:65534:
EOF
```

---

## Best Practices & Tips

> [!TIP]
> Initialize log files like `/var/log/lastlog` and `/var/log/wtmp` with correct permissions.
