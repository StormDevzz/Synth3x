# 10 Ways to Speed Up Downloads and Builds in Gentoo

Source-based package management can be resource-intensive. Below are ten practical methods to accelerate download and compilation phases.

---

## 1. Segmented Multi-threaded Downloads
Replace standard Portage wget downloads with `aria2c` for segmented downloading:
```bash
FETCHCOMMAND="aria2c -s 4 -x 4 -d \${DISTDIR} -o \${FILE} \${URI}"
```

## 2. Compile in RAM (tmpfs)
Set up Portage to compile inside `/var/tmp/portage` mounted in memory:
```bash
# In /etc/fstab:
tmpfs   /var/tmp/portage   tmpfs   size=8G,uid=portage,gid=portage,mode=775,nosuid,noatime,nodev   0 0
```

## 3. Parallel Compiler Processes
Align your `MAKEOPTS` in `make.conf` with the total number of logical cores:
```bash
MAKEOPTS="-j$(nproc)"
```

## 4. Use Binary Packages
Configure Portage to use pre-compiled binaries where available:
```bash
EMERGE_DEFAULT_OPTS="--binpkg-changed-deps=y --getbinpkg=y"
```

## 5. Setup Nearest Mirrors
Use `mirrorselect` to find and append the fastest local mirrors to `make.conf`.

## 6. Use Compiler Caching (ccache)
Install `dev-util/ccache` and enable `ccache` in features to store compiled object files.

## 7. Fast Compression Formats
Choose fast formats for Portage metadata syncing by using `squashfs` or `git`.

## 8. Exclude Unneeded CPU Architectures
Disable support for unused graphic chipsets, input systems, and device drivers in your `VIDEO_CARDS` and `INPUT_DEVICES` variables.

## 9. Parallel Emerge Jobs
Allow emerging non-dependent packages concurrently:
```bash
EMERGE_DEFAULT_OPTS="--jobs=4 --load-average=8"
```

## 10. Avoid Deep Global Rebuilds
Unless security issues require, avoid full compilation upgrades for minor dependencies by avoiding the `-D` flag on general emerge commands.
