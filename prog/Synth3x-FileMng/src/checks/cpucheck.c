#include "cpuid.h"
#include "cpucheck.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

CpuLevel cpu_detect_level(void) {
    uint32_t ecx = cpu_features_ecx();
    uint32_t edx = cpu_features_edx();
    uint32_t ext = cpu_features_ext();

    if (!(edx & CPU_EDX_SSE)) return CPU_LEVEL_LEGACY;
    if (!(edx & CPU_EDX_SSE2)) return CPU_LEVEL_LEGACY;

    if (!(ecx & CPU_ECX_SSE3)) return CPU_LEVEL_LEGACY;

    if (ecx & CPU_ECX_AVX) {
        if (ext & CPU_EXT_AVX2) return CPU_LEVEL_HASWELL;
        return CPU_LEVEL_SANDY;
    }

    if (ecx & CPU_ECX_SSE4_2) return CPU_LEVEL_NEHALEM;

    if (ecx & CPU_ECX_SSSE3) return CPU_LEVEL_CORE2;

    return CPU_LEVEL_LEGACY;
}

void cpu_recommend_flags(char *buf, size_t size) {
    CpuLevel level = cpu_detect_level();

    char arch[32];
    char noflags[256] = "";

    switch (level) {
        case CPU_LEVEL_LEGACY:
            snprintf(arch, sizeof(arch), "x86-64");
            snprintf(noflags, sizeof(noflags),
                     " -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2"
                     " -mno-avx -mno-avx2");
            break;
        case CPU_LEVEL_CORE2:
            snprintf(arch, sizeof(arch), "x86-64");
            snprintf(noflags, sizeof(noflags),
                     " -mno-sse4.1 -mno-sse4.2"
                     " -mno-avx -mno-avx2");
            break;
        case CPU_LEVEL_NEHALEM:
            snprintf(arch, sizeof(arch), "x86-64");
            snprintf(noflags, sizeof(noflags),
                     " -mno-avx -mno-avx2");
            break;
        default:
            snprintf(arch, sizeof(arch), "x86-64");
            noflags[0] = '\0';
            break;
    }

    snprintf(buf, size, "-march=%s -mtune=generic%s", arch, noflags);
}

const char *cpu_description(void) {
    static char desc[128];
    char vendor[13] = "";
    char brand[49] = "";
    cpu_get_vendor(vendor);
    cpu_brand_string(brand);

    CpuLevel level = cpu_detect_level();
    const char *level_str;
    switch (level) {
        case CPU_LEVEL_LEGACY:  level_str = "Legacy (pre-Pentium 4)"; break;
        case CPU_LEVEL_CORE2:   level_str = "Core 2 / Penryn"; break;
        case CPU_LEVEL_NEHALEM: level_str = "Nehalem / Westmere"; break;
        case CPU_LEVEL_SANDY:   level_str = "Sandy / Ivy Bridge"; break;
        case CPU_LEVEL_HASWELL: level_str = "Haswell / Broadwell"; break;
        case CPU_LEVEL_MODERN:  level_str = "Skylake+"; break;
        default:                level_str = "Unknown"; break;
    }

    snprintf(desc, sizeof(desc), "%s (%s) — %s — %d cores",
             brand, vendor, level_str, cpu_logical_cores());
    return desc;
}

int cpu_logical_cores(void) {
    uint32_t cores = cpu_cores();
    if (cores == 0) {
        long n = sysconf(_SC_NPROCESSORS_CONF);
        if (n > 0) return (int)n;
        return 1;
    }
    return (int)cores;
}

#ifdef STANDALONE
int main(void) {
    char flags[256];
    cpu_recommend_flags(flags, sizeof(flags));
    printf("%s\n", flags);
    return 0;
}
#endif
