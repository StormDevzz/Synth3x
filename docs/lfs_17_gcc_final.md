# Linux From Scratch: Building GCC (Final Native Pass)

The final pass builds GCC as a fully-featured compiler with optimization flags and compiler wrappers.

---

## Detailed Explanation

This pass compiles C++ support, link-time optimizations (LTO), and libstdc++ plugins natively for LFS.

---

## Best Practices & Tips

> [!TIP]
> Ensure GCC links dynamically to the native target libraries without compiler wrapper flags.
