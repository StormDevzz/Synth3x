#ifndef SYNTH3X_KERNEL_H
#define SYNTH3X_KERNEL_H

typedef unsigned long size_t;

int cpuid_supported(void);
void cpuid(unsigned int leaf, unsigned int *eax, unsigned int *ebx,
           unsigned int *ecx, unsigned int *edx);
void halt(void);
void cli(void);
void sti(void);

unsigned char  inb(unsigned short port);
unsigned short inw(unsigned short port);
unsigned int   inl(unsigned short port);
void outb(unsigned short port, unsigned char  val);
void outw(unsigned short port, unsigned short val);
void outl(unsigned short port, unsigned int   val);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

void gdt_load(unsigned int gdt_ptr);
void gdt_reload_segments(void);
void idt_load(unsigned int idt_ptr);

#endif
