#ifndef SYNTH3X_KERNEL_H
#define SYNTH3X_KERNEL_H

typedef unsigned long size_t;

/* cpu.S — CPU identification and control */
int cpuid_supported(void);
void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
           uint32_t *ecx, uint32_t *edx);
void halt(void);
void cli(void);
void sti(void);

/* io.S — Port I/O */
uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void outb(uint16_t port, uint8_t  val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);

/* mem.S — Memory operations */
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

/* gdt.S — GDT loading */
void gdt_load(uint32_t gdt_ptr);
void gdt_reload_segments(void);

/* idt.S — IDT loading and interrupt stubs */
void idt_load(uint32_t idt_ptr);
void isr_install_handler(int irq, void (*handler)(void));

#endif
