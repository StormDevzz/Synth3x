#ifndef CHECKS_CPUCHECK_H
#define CHECKS_CPUCHECK_H

#include <stddef.h>

/* CPU capability levels */
typedef enum {
    CPU_LEVEL_LEGACY,   /* No SSE3, Penryn-class or older */
    CPU_LEVEL_CORE2,    /* Core 2 / Penryn — SSSE3, no SSE4 */
    CPU_LEVEL_NEHALEM,  /* Nehalem / Westmere — SSE4.1/4.2 */
    CPU_LEVEL_SANDY,    /* Sandy Bridge / Ivy Bridge — AVX */
    CPU_LEVEL_HASWELL,  /* Haswell+ — AVX2 */
    CPU_LEVEL_MODERN,   /* Skylake+ — all features */
} CpuLevel;

/* Returns the detected CPU capability level */
CpuLevel cpu_detect_level(void);

/* Writes recommended GCC flags to buf based on CPU level */
void cpu_recommend_flags(char *buf, size_t size);

/* Returns human-readable CPU description */
const char *cpu_description(void);

/* Returns number of logical CPU cores */
int cpu_logical_cores(void);

#endif
