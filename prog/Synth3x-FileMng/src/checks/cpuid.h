#ifndef CHECKS_CPUID_H
#define CHECKS_CPUID_H

#include <stdint.h>

/* CPU vendor string (12 chars + NUL) */
char    *cpu_get_vendor(char buf[13]);

/* Feature bits from CPUID leaf 1 */
uint32_t cpu_features_ecx(void);
uint32_t cpu_features_edx(void);

/* Extended features from CPUID leaf 0x80000001 */
uint32_t cpu_features_ext(void);

/* CPU brand string (48 chars + NUL) */
char    *cpu_brand_string(char buf[49]);

/* Number of CPU cores */
uint32_t cpu_cores(void);

/* CPU family (with extended family for > 0xF) */
uint32_t cpu_family(void);

/* CPU model (with extended model) */
uint32_t cpu_model(void);

/* Feature bit masks for ECX leaf 1 */
#define CPU_ECX_SSE3     0x00000001
#define CPU_ECX_SSSE3    0x00000200
#define CPU_ECX_SSE4_1   0x00080000
#define CPU_ECX_SSE4_2   0x00100000
#define CPU_ECX_AVX      0x10000000

/* Feature bit masks for EDX leaf 1 */
#define CPU_EDX_MMX      0x00800000
#define CPU_EDX_SSE      0x02000000
#define CPU_EDX_SSE2     0x04000000
#define CPU_EDX_HTT      0x10000000

/* Feature bit masks for extended ECX (0x80000001) */
#define CPU_EXT_SSE4A    0x00000040
#define CPU_EXT_AVX2     0x00000020

#endif
