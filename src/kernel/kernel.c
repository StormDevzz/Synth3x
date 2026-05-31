typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef u32                uptr;

#define NULL ((void*)0)

#include "kernel.h"

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_ID       0
#define VBE_DISPI_INDEX_XRES     1
#define VBE_DISPI_INDEX_YRES     2
#define VBE_DISPI_INDEX_BPP      3
#define VBE_DISPI_INDEX_ENABLE   4
#define VBE_DISPI_INDEX_VIRT_WIDTH  6
#define VBE_DISPI_ENABLED     0x01
#define VBE_DISPI_LFB_ENABLED 0x40
#define PCI_CONFIG_ADDR   0xCF8
#define PCI_CONFIG_DATA   0xCFC

struct mb_tag {
u32 type;
u32 size;
} __attribute__((packed));

struct mb_info {
u32 total_size;
u32 reserved;
struct mb_tag tags[];
} __attribute__((packed));

struct mb_tag_fb {
u32 type;
u32 size;
u64 fb_addr;
u32 fb_pitch;
u32 fb_width;
u32 fb_height;
u8  fb_bpp;
u8  fb_type;
u16 reserved;
} __attribute__((packed));

static volatile u16 *vga = (u16*)0xB8000;
static int cx = 0, cy = 0;
static u8 color = 0x0F;

#define VGA_W 80
#define VGA_H 25

static void putc(char c) {
if (c == '\n') { cx = 0; cy++; }
else if (c == '\r') { cx = 0; }
else {
vga[cy * VGA_W + cx] = (u16)c | ((u16)color << 8);
cx++;
}
if (cx >= VGA_W) { cx = 0; cy++; }
if (cy >= VGA_H) {
for (int y = 0; y < VGA_H - 1; y++)
for (int x = 0; x < VGA_W; x++)
vga[y * VGA_W + x] = vga[(y + 1) * VGA_W + x];
for (int x = 0; x < VGA_W; x++)
vga[(VGA_H - 1) * VGA_W + x] = 0x0F00;
cy = VGA_H - 1;
}
}

void print(const char *s) { while (*s) putc(*s++); }

void print_hex(u32 n) {
char hex[] = "0123456789ABCDEF";
putc('0'); putc('x');
for (int i = 28; i >= 0; i -= 4) putc(hex[(n >> i) & 0xF]);
}

void print_dec(u32 n, int pad) {
char buf[12]; int i = 0;
if (n == 0) { buf[i++] = '0'; }
while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
while (i < pad) buf[i++] = ' ';
while (i > 0) putc(buf[--i]);
}

void clear_vga(void) {
for (int y = 0; y < VGA_H; y++)
for (int x = 0; x < VGA_W; x++)
vga[y * VGA_W + x] = 0x0F00;
cx = 0; cy = 0;
}

void set_color_vga(u8 fg, u8 bg) { color = (bg << 4) | fg; }

/* ─── Serial ─── */
static void serial_init(void) {
outb(0x3F8 + 1, 0x00);
outb(0x3F8 + 3, 0x80);
outb(0x3F8 + 0, 0x01);
outb(0x3F8 + 1, 0x00);
outb(0x3F8 + 3, 0x03);
outb(0x3F8 + 2, 0xC7);
outb(0x3F8 + 4, 0x0B);
}

static void serial_putc(char c) {
while (!(inb(0x3F8 + 5) & 0x20));
outb(0x3F8, c);
if (c == '\n') serial_putc('\r');
}

static void serial_print(const char *s) {
while (*s) serial_putc(*s++);
}

static void serial_hex(u32 n) {
char hex[] = "0123456789ABCDEF";
serial_putc('0'); serial_putc('x');
for (int i = 28; i >= 0; i -= 4) serial_putc(hex[(n >> i) & 0xF]);
}

/* ─── GDT ─── */
static void init_gdt(void) {
static u8 gdt_data[64] __attribute__((aligned(8)));
struct { u16 limit; u32 base; } __attribute__((packed)) gdtp;
u32 *gdt = (u32*)gdt_data;

gdt[0] = 0x00000000; gdt[1] = 0x00000000;
gdt[2] = 0x0000FFFF; gdt[3] = 0x00CF9A00;
gdt[4] = 0x0000FFFF; gdt[5] = 0x00CF9200;

gdtp.base = (u32)gdt_data;
gdtp.limit = sizeof(gdt_data) - 1;
gdt_load((u32)&gdtp);
gdt_reload_segments();
}

/* ─── IDT ─── */
struct idt_entry {
u16 base_lo;
u16 sel;
u8  always0;
u8  flags;
u16 base_hi;
} __attribute__((packed));

struct idt_ptr {
u16 limit;
u32 base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void isr0(void); extern void isr1(void); extern void isr2(void);
extern void isr3(void); extern void isr4(void); extern void isr5(void);
extern void isr6(void); extern void isr7(void); extern void isr8(void);
extern void isr9(void); extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static void idt_set_entry(int n, void *handler, u8 flags) {
idt[n].base_lo = (u32)handler & 0xFFFF;
idt[n].sel     = 0x08;
idt[n].always0 = 0;
idt[n].flags   = flags;
idt[n].base_hi = ((u32)handler >> 16) & 0xFFFF;
}

static void init_idt(void) {
idtp.limit = sizeof(idt) - 1;
idtp.base  = (u32)&idt;
for (int i = 0; i < 256; i++)
idt_set_entry(i, isr0, 0x8E);
void *isrs[] = {isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31};
for (int i = 0; i < 32; i++) idt_set_entry(i, isrs[i], 0x8E);
void *irqs[] = {irq0,irq1,irq2,irq3,irq4,irq5,irq6,irq7,irq8,irq9,irq10,irq11,irq12,irq13,irq14,irq15};
for (int i = 0; i < 16; i++) idt_set_entry(32 + i, irqs[i], 0x8E);
idt_load((u32)&idtp);
}

/* ─── PIC ─── */
static void init_pic(void) {
outb(0x20, 0x11); outb(0xA0, 0x11);
outb(0x21, 0x20); outb(0xA1, 0x28);
outb(0x21, 0x04); outb(0xA1, 0x02);
outb(0x21, 0x01); outb(0xA1, 0x01);
outb(0x21, 0x00); outb(0xA1, 0x00);
}

/* ─── PCI ─── */
static u32 pci_read(u32 bus, u32 dev, u32 func, u32 reg) {
u32 addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
outl(PCI_CONFIG_ADDR, addr);
return inl(PCI_CONFIG_DATA);
}

static u32 find_lfb_via_pci(void) {
for (int dev = 0; dev < 32; dev++) {
u32 vid = pci_read(0, dev, 0, 0) & 0xFFFF;
u32 cls = pci_read(0, dev, 0, 0x08) >> 24;
if (vid != 0xFFFF && cls == 0x03) {
u32 bar0 = pci_read(0, dev, 0, 0x10);
if (bar0 & 1) bar0 &= ~0xF;
else bar0 &= ~0x3;
return bar0;
}
}
return 0;
}

/* ─── Bochs VBE ─── */
static u16 vbe_read(u16 idx) {
outw(VBE_DISPI_IOPORT_INDEX, idx);
return inw(VBE_DISPI_IOPORT_DATA);
}

static void vbe_write(u16 idx, u16 val) {
outw(VBE_DISPI_IOPORT_INDEX, idx);
outw(VBE_DISPI_IOPORT_DATA, val);
}

/* ─── Framebuffer ─── */
static volatile void *lfb = NULL;
static int fb_w = 0, fb_h = 0, fb_bpp = 0, fb_pitch = 0;

static int vbe_init(u32 fb_addr) {
vbe_write(VBE_DISPI_INDEX_ID, 0xB0C0);
if (vbe_read(VBE_DISPI_INDEX_ID) != 0xB0C0 &&
vbe_read(VBE_DISPI_INDEX_ID) != 0xB0C4)
return 0;
vbe_write(VBE_DISPI_INDEX_ENABLE, 0);
vbe_write(VBE_DISPI_INDEX_XRES, 1024);
vbe_write(VBE_DISPI_INDEX_YRES, 768);
vbe_write(VBE_DISPI_INDEX_BPP, 32);
vbe_write(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
fb_w = 1024; fb_h = 768; fb_bpp = 32; fb_pitch = 1024 * 4;
lfb = (volatile void*)fb_addr;
return 1;
}

static void fb_pixel(int x, int y, u8 r, u8 g, u8 b) {
if (!lfb || x < 0 || y < 0 || x >= fb_w || y >= fb_h) return;
u32 *p = (u32*)(lfb + y * fb_pitch + x * 4);
*p = (r << 16) | (g << 8) | b;
}

static void fb_fill(u8 r, u8 g, u8 b) {
if (!lfb) return;
for (int y = 0; y < fb_h; y++)
for (int x = 0; x < fb_w; x++)
fb_pixel(x, y, r, g, b);
}

static void fb_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b) {
for (int py = y; py < y + h && py < fb_h; py++)
for (int px = x; px < x + w && px < fb_w; px++)
fb_pixel(px, py, r, g, b);
}

static void fb_gradient_v(int x, int y, int w, int h, u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2) {
for (int py = y; py < y + h && py < fb_h; py++) {
float t = (float)(py - y) / h;
u8 r = r1 + (u8)((r2 - r1) * t);
u8 g = g1 + (u8)((g2 - g1) * t);
u8 b = b1 + (u8)((b2 - b1) * t);
for (int px = x; px < x + w && px < fb_w; px++)
fb_pixel(px, py, r, g, b);
}
}

/* ─── 8x8 font bitmap ─── */
static const u8 font8x8[95][8] = {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
{0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
{0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
{0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
{0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
{0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
{0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00,0x00},
{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
{0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
{0x00,0x00,0x00,0x00,0x18,0x18,0x0C,0x00},
{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00},
{0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
{0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
{0x18,0x1C,0x1E,0x18,0x18,0x18,0x7E,0x00},
{0x3E,0x63,0x30,0x18,0x0C,0x63,0x7F,0x00},
{0x3E,0x63,0x30,0x1C,0x30,0x63,0x3E,0x00},
{0x18,0x1C,0x1E,0x1B,0x7F,0x18,0x18,0x00},
{0x7F,0x03,0x03,0x3F,0x60,0x63,0x3E,0x00},
{0x3E,0x63,0x03,0x3F,0x63,0x63,0x3E,0x00},
{0x7F,0x63,0x30,0x18,0x0C,0x06,0x06,0x00},
{0x3E,0x63,0x63,0x3E,0x63,0x63,0x3E,0x00},
{0x3E,0x63,0x63,0x7E,0x60,0x63,0x3E,0x00},
{0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
{0x00,0x18,0x18,0x00,0x18,0x18,0x0C,0x00},
{0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
{0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
{0x3E,0x63,0x30,0x18,0x18,0x00,0x18,0x00},
{0x3E,0x63,0x7B,0x7B,0x03,0x63,0x3E,0x00},
{0x08,0x1C,0x36,0x63,0x7F,0x63,0x63,0x00},
{0x3F,0x63,0x63,0x3F,0x63,0x63,0x3F,0x00},
{0x1E,0x33,0x61,0x01,0x01,0x33,0x1E,0x00},
{0x0F,0x1B,0x33,0x63,0x33,0x1B,0x0F,0x00},
{0x7F,0x31,0x31,0x3D,0x31,0x31,0x7F,0x00},
{0x7F,0x31,0x31,0x3D,0x01,0x01,0x01,0x00},
{0x3E,0x63,0x01,0x79,0x63,0x67,0x7C,0x00},
{0x63,0x63,0x63,0x7F,0x63,0x63,0x63,0x00},
{0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x60,0x60,0x60,0x60,0x63,0x63,0x3E,0x00},
{0x63,0x33,0x1B,0x0F,0x1B,0x33,0x63,0x00},
{0x01,0x01,0x01,0x01,0x31,0x31,0x7F,0x00},
{0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
{0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
{0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
{0x3F,0x63,0x63,0x3F,0x03,0x03,0x03,0x00},
{0x3E,0x63,0x63,0x63,0x3B,0x33,0x6E,0x00},
{0x3F,0x63,0x63,0x3F,0x33,0x63,0x63,0x00},
{0x3E,0x63,0x03,0x1E,0x30,0x63,0x3E,0x00},
{0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x00},
{0x63,0x63,0x63,0x63,0x63,0x63,0x3E,0x00},
{0x63,0x63,0x63,0x36,0x36,0x1C,0x08,0x00},
{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
{0x63,0x36,0x1C,0x08,0x1C,0x36,0x63,0x00},
{0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
{0x7F,0x31,0x18,0x0C,0x06,0x63,0x7F,0x00},
{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
{0x01,0x03,0x06,0x0C,0x18,0x30,0x60,0x00},
{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
{0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
{0x0C,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x3E,0x60,0x7E,0x63,0x7E,0x00},
{0x03,0x03,0x3F,0x63,0x63,0x63,0x3F,0x00},
{0x00,0x00,0x3E,0x63,0x03,0x63,0x3E,0x00},
{0x30,0x30,0x7E,0x33,0x33,0x33,0x7E,0x00},
{0x00,0x00,0x3E,0x63,0x7F,0x03,0x3E,0x00},
{0x1C,0x36,0x06,0x1F,0x06,0x06,0x06,0x00},
{0x00,0x00,0x7E,0x33,0x33,0x7E,0x30,0x3E},
{0x03,0x03,0x3F,0x63,0x63,0x63,0x63,0x00},
{0x18,0x00,0x1E,0x18,0x18,0x18,0x7E,0x00},
{0x18,0x00,0x1E,0x18,0x18,0x18,0x18,0x0E},
{0x03,0x03,0x33,0x1B,0x0F,0x1B,0x33,0x00},
{0x1E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x00,0x00,0x37,0x7F,0x6B,0x63,0x63,0x00},
{0x00,0x00,0x3F,0x63,0x63,0x63,0x63,0x00},
{0x00,0x00,0x3E,0x63,0x63,0x63,0x3E,0x00},
{0x00,0x00,0x3F,0x63,0x63,0x3F,0x03,0x03},
{0x00,0x00,0x7E,0x33,0x33,0x7E,0x30,0x30},
{0x00,0x00,0x3B,0x67,0x03,0x03,0x03,0x00},
{0x00,0x00,0x3E,0x03,0x3E,0x60,0x3E,0x00},
{0x0C,0x0C,0x3F,0x0C,0x0C,0x2C,0x18,0x00},
{0x00,0x00,0x33,0x33,0x33,0x33,0x3E,0x00},
{0x00,0x00,0x63,0x63,0x36,0x1C,0x08,0x00},
{0x00,0x00,0x63,0x63,0x6B,0x7F,0x36,0x00},
{0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
{0x00,0x00,0x63,0x63,0x63,0x3E,0x0C,0x07},
{0x00,0x00,0x7F,0x32,0x1C,0x66,0x7F,0x00},
{0x38,0x0C,0x0C,0x06,0x0C,0x0C,0x38,0x00},
{0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
{0x0E,0x18,0x18,0x30,0x18,0x18,0x0E,0x00},
{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

static void fb_char(int x, int y, char c, u8 r, u8 g, u8 b) {
if (c < 32 || c > 126) return;
const u8 *glyph = font8x8[c - 32];
for (int py = 0; py < 8; py++)
for (int px = 0; px < 8; px++)
if (glyph[py] & (0x80 >> px))
fb_pixel(x + px, y + py, r, g, b);
}

static void fb_text(int x, int y, const char *s, u8 r, u8 g, u8 b) {
int ox = x;
while (*s) {
if (*s == '\n') { x = ox; y += 10; }
else { fb_char(x, y, *s, r, g, b); x += 8; }
s++;
}
}

static void fb_text_large(int x, int y, const char *s, int scale, u8 r, u8 g, u8 b) {
int ox = x;
while (*s) {
if (*s == '\n') { x = ox; y += 8 * scale + 2; s++; continue; }
if (*s < 32 || *s > 126) { s++; continue; }
const u8 *glyph = font8x8[*s - 32];
for (int py = 0; py < 8; py++)
for (int px = 0; px < 8; px++)
if (glyph[py] & (0x80 >> px))
fb_rect(x + px * scale, y + py * scale, scale, scale, r, g, b);
x += 8 * scale + 2;
s++;
}
}

static void draw_logo(int cx, int cy) {
u8 r = 0x00, g = 0xFF, b = 0x88;
int s = 6;
int block[][5] = {
{0,1,1,1,0},{1,0,0,0,1},{1,0,1,0,1},{1,0,0,0,1},{0,1,1,1,0}
};
for (int y = 0; y < 5; y++)
for (int x = 0; x < 5; x++)
if (block[y][x]) fb_rect(cx + x*s, cy + y*s, s, s, r, g, b);
cx += 8*s;
int s3[][7] = {
{0,1,1,1,0,1,1},{1,0,0,0,1,0,0},{0,0,1,1,0,1,1},
{0,0,0,0,1,0,0},{0,1,1,1,0,1,1}
};
for (int y = 0; y < 5; y++)
for (int x = 0; x < 7; x++)
if (s3[y][x]) fb_rect(cx + x*s, cy + y*s, s, s, r, g, b);
}

/* ─── Interrupt handlers ─── */
static volatile u32 kb_scancode = 0;
static volatile int kb_ready = 0;

void isr_handler(u32 *regs) {
(void)regs;
print("EXCEPTION at vector "); print_hex(regs[0]); print("\n");
for (;;) halt();
}

void irq_handler(u32 *regs) {
if (regs[0] >= 40 && regs[0] <= 47) {
if (regs[0] == 33) {
u8 sc = inb(0x60);
kb_scancode = sc;
kb_ready = 1;
}
outb(0xA0, 0x20);
}
outb(0x20, 0x20);
}

static int get_scancode(void) {
while (!kb_ready) halt();
kb_ready = 0;
u32 sc = kb_scancode;
return sc;
}

static int kb_shift = 0;

static int scancode_to_ascii(u8 sc) {
static const u8 normal[128] = {
0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
'b','n','m',',','.','/',0,0,0,' '
};
static const u8 shifted[128] = {
0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
'B','N','M','<','>','?',0,0,0,' '
};
if (sc >= 128) return 0;
u8 c = kb_shift ? shifted[sc] : normal[sc];
if (sc == 0x2A || sc == 0x36) kb_shift = 1;
if (sc == 0xAA || sc == 0xB6) kb_shift = 0;
return c;
}

/* ─── Simple echo shell ─── */
static void shell(void) {
clear_vga();
set_color_vga(0x0A, 0x00);
print("\n  Synth3x OS - Shell v0.1\n");
print("  ────────────────────────\n");
set_color_vga(0x07, 0x00);
char cmd[256]; int ci = 0;
for (;;) {
print("\n  $ ");
ci = 0;
for (;;) {
u8 sc = get_scancode();
char c = scancode_to_ascii(sc);
if (c == '\n') {
cmd[ci] = 0;
print("\n");
break;
}
if (c && ci < 255) {
cmd[ci++] = c;
putc(c);
}
}
if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 's') {
clear_vga();
set_color_vga(0x0A, 0x00);
print("\n  Synth3x OS - Shell v0.1\n");
print("  ────────────────────────\n");
set_color_vga(0x07, 0x00);
} else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') {
print("  Commands: cls, help, info, halt, reboot\n");
} else if (cmd[0] == 'i' && cmd[1] == 'n' && cmd[2] == 'f' && cmd[3] == 'o') {
print("  Synth3x OS Kernel v0.2\n  Custom x86 kernel\n  VGA text mode\n");
} else if (cmd[0] == 'h' && cmd[1] == 'a' && cmd[2] == 'l' && cmd[3] == 't') {
print("  Halting...\n");
for (;;) halt();
} else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o' && cmd[4] == 'o' && cmd[5] == 't') {
print("  Use QEMU exit: Ctrl+A, X\n");
} else if (ci > 0) {
print("  Unknown: "); print(cmd); print("\n");
}
}
}

/* ─── Boot menu (graphics mode) ─── */
static void boot_menu_gfx(void) {
fb_gradient_v(0, 0, fb_w, fb_h, 10, 5, 30, 40, 10, 60);
fb_rect(0, 0, fb_w, 4, 0, 255, 136);
fb_rect(0, fb_h - 4, fb_w, 4, 0, 255, 136);
draw_logo(fb_w / 2 - 100, 60);
fb_text_large(fb_w / 2 - 120, 180, "S Y N T H 3 X   O S", 2, 0, 200, 255);
fb_text(fb_w / 2 - 100, 240, "Custom Kernel v0.2 - x86 Protected Mode", 180, 180, 180);
fb_text(fb_w / 2 - 80, 260, "CPU", 0, 255, 136);
if (cpuid_supported()) {
char brand[49];
cpuid(0x80000002, (u32*)&brand[0],  (u32*)&brand[4],  (u32*)&brand[8],  (u32*)&brand[12]);
cpuid(0x80000003, (u32*)&brand[16], (u32*)&brand[20], (u32*)&brand[24], (u32*)&brand[28]);
cpuid(0x80000004, (u32*)&brand[32], (u32*)&brand[36], (u32*)&brand[40], (u32*)&brand[44]);
brand[48] = 0;
fb_text(fb_w / 2 - 40, 260, brand, 200, 200, 200);
}

int sel = 0;
const char *opts[] = {"SYNTH3X DESKTOP", "XFCE", "SHELL", "HALT"};
const char *desc[] = {"Custom lightweight desktop environment",
"Full-featured desktop environment",
"Command-line shell",
"Stop the system"};
int nopts = 4;

for (;;) {
int bx = fb_w / 2 - 200, by = 310;
for (int i = 0; i < nopts; i++) {
if (i == sel) {
fb_rect(bx, by + i * 40, 400, 34, 0, 180, 100);
fb_text(bx + 10, by + i * 40 + 6, opts[i], 0, 255, 200);
fb_text(bx + 200, by + i * 40 + 6, desc[i], 150, 220, 180);
} else {
fb_rect(bx, by + i * 40, 400, 34, 20, 20, 50);
fb_text(bx + 10, by + i * 40 + 6, opts[i], 150, 150, 150);
}
}

fb_text(fb_w / 2 - 120, by + nopts * 40 + 20,
"ARROWS: select   ENTER: confirm", 100, 100, 100);

u8 sc = get_scancode();
if (sc == 0x48 && sel > 0) sel--;
if (sc == 0x50 && sel < nopts - 1) sel++;
if (sc == 0x1C) {
if (sel == 2) {
fb_text(fb_w / 2 - 80, by + nopts * 40 + 45, "[Starting shell...]", 0, 255, 136);
for (int i = 0; i < 30000000; i++) halt();
return;
}
if (sel == 3) {
fb_text(fb_w / 2 - 80, by + nopts * 40 + 45, "[Halting system...]", 255, 100, 100);
for (int i = 0; i < 10000000; i++) halt();
for (;;) halt();
}
fb_text(fb_w / 2 - 80, by + nopts * 40 + 45, "[Starting...]", 0, 255, 136);
break;
}
}
}

/* ─── Boot menu (text mode) ─── */
static void boot_menu_text(void) {
clear_vga();
set_color_vga(0x0A, 0x00);
print("\n");
print("  ╔═══════════════════════════════════════════════╗\n");
print("  ║           S Y N T H 3 X   O S                ║\n");
print("  ║         Kernel v0.2 - x86 Protected Mode      ║\n");
print("  ╚═══════════════════════════════════════════════╝\n");
print("\n");
set_color_vga(0x07, 0x00);
if (cpuid_supported()) {
char brand[49];
cpuid(0x80000002, (u32*)&brand[0],  (u32*)&brand[4],  (u32*)&brand[8],  (u32*)&brand[12]);
cpuid(0x80000003, (u32*)&brand[16], (u32*)&brand[20], (u32*)&brand[24], (u32*)&brand[28]);
cpuid(0x80000004, (u32*)&brand[32], (u32*)&brand[36], (u32*)&brand[40], (u32*)&brand[44]);
brand[48] = 0;
print("  CPU: "); print(brand); print("\n");
}
set_color_vga(0x0B, 0x00);
print("\n  >> Use arrow keys UP/DOWN, ENTER to select\n\n");
set_color_vga(0x08, 0x00);
print("    [1] Synth3x Desktop   Custom DE\n");
print("    [2] Xfce              Full DE\n");
print("    [3] Shell             Command line\n");
print("    [4] Halt              Stop system\n");
print("\n");

int sel = 0;
for (;;) {
set_color_vga(0x0F, 0x00);
print("  Choice: ");
if (sel == 1) print("2");
else if (sel == 0) print("1");
else if (sel == 2) print("s");
else print("h");

u8 sc = get_scancode();
if (sc == 0x48) { if (sel > 0) sel--; else sel = 0; }
if (sc == 0x50) { if (sel < 3) sel++; else sel = 3; }
if (sc == 0x1C || sc == 0x02 || sc == 0x03) {
u8 c = 0;
if (sc == 0x02 || sel == 0) c = '1';
else if (sc == 0x03 || sel == 1) c = '2';
else if (sel == 2) c = 's';
else if (sel == 3) c = 'h';

print("\n");
if (c == '1') {
print("  [OK] Booting Synth3x Desktop...\n");
break;
} else if (c == '2') {
print("  [OK] Starting Xfce...\n");
break;
} else if (c == 's' || c == 'S') {
print("  [OK] Starting shell...\n");
print("\n");
set_color_vga(0x0F, 0x00);
shell();
break;
} else if (c == 'h') {
print("  [OK] Halting system...\n");
for (;;) halt();
}
}
}
}

/* ─── Main ─── */
__attribute__((used)) void kernel_main(u32 magic, struct mb_info *mbi) {
serial_init();
serial_print("\n=== Synth3x OS Kernel v0.2 ===\n");
serial_print("Booting...\n");

clear_vga();
set_color_vga(0x07, 0x00);
print("Synth3x OS - Booting...\n");

init_gdt();
print("[OK] GDT initialized\n");
serial_print("[OK] GDT initialized\n");

init_idt();
print("[OK] IDT initialized\n");
serial_print("[OK] IDT initialized\n");

init_pic();
print("[OK] PIC initialized\n");
serial_print("[OK] PIC initialized\n");

sti();
print("[OK] Interrupts enabled\n");
serial_print("[OK] Interrupts enabled\n");

u32 fb_addr = 0;
int has_fb = 0;
if (magic == 0x2BADB002 && mbi) {
uptr addr = (uptr)&mbi->tags;
while (addr < (uptr)mbi + mbi->total_size) {
struct mb_tag *tag = (struct mb_tag*)addr;
if (tag->type == 0) break;
if (tag->type == 8) {
struct mb_tag_fb *ft = (struct mb_tag_fb*)tag;
fb_addr = (u32)ft->fb_addr;
}
addr += (tag->size + 7) & ~7;
}
}
if (!fb_addr) fb_addr = find_lfb_via_pci();
if (fb_addr && vbe_init(fb_addr)) {
print("[OK] Bochs VBE: 1024x768x32\n");
has_fb = 1;
} else {
print("[--] VBE not available, using VGA text mode\n");
}

if (has_fb) {
boot_menu_gfx();
} else {
boot_menu_text();
}

print("System ready.\n");
set_color_vga(0x08, 0x00);
print("  Press any key to halt...\n");
get_scancode();
for (;;) halt();
}
