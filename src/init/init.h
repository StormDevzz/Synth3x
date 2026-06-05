#ifndef SYNINIT_H
#define SYNINIT_H

#include <stdint.h>

void syn_cpuid_vendor(char buf[13]);
int  syn_cpuid_has_avx(void);
int  syn_cpuid_has_sse2(void);
void syn_boot_splash(void);

#endif
