# Linux From Scratch: Packages and Patches Downloader

The LFS build process relies entirely on compiling source archives. We must fetch the correct versions of all required source packages and patches.

---

## Detailed Explanation

Create a dedicated sources directory and download the LFS packages list:
```bash
mkdir -v $LFS/sources
chmod -v a+wt $LFS/sources
wget https://www.linuxfromscratch.org/lfs/downloads/stable/wget-list-sysv
wget --input-file=wget-list-sysv --continue --directory-prefix=$LFS/sources
```

Verify package integrity using md5sums:
```bash
wget https://www.linuxfromscratch.org/lfs/downloads/stable/md5sums
pushd $LFS/sources
md5sum -c md5sums
popd
```

---

## Best Practices & Tips

> [!TIP]
> If any checksum verification fails, delete the corrupted package and re-download it immediately.
