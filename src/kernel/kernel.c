/* Synth3x OS — Multiboot Kernel
 * Architecture: x86-32 (protected mode)
 * Language: C (no libc, freestanding)
 * Entry point: _start
 */

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t           uintptr_t;

#define NULL ((void*)0)
#define MULTIBOOT_MAGIC  0xE85250D6
#define MULTIBOOT_ARCH   0

struct __attribute__((packed)) multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct __attribute__((packed)) multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
    struct multiboot_tag tags[];
};

/* VGA text mode buffer */
static volatile uint16_t *vga = (uint16_t *)0xB8000;
static int cursor_x = 0, cursor_y = 0;
static uint8_t color = 0x0F; /* white on black */

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | ((uint16_t)color << 8);
        cursor_x++;
    }
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        /* scroll */
        for (int y = 0; y < VGA_HEIGHT - 1; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
        for (int x = 0; x < VGA_WIDTH; x++)
            vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0x0F00;
        cursor_y = VGA_HEIGHT - 1;
    }
}

void print(const char *s) {
    while (*s) putchar(*s++);
}

void print_hex(uint32_t n) {
    char hex[] = "0123456789ABCDEF";
    putchar('0'); putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        putchar(hex[(n >> i) & 0xF]);
}

void clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga[y * VGA_WIDTH + x] = 0x0F00;
    cursor_x = 0;
    cursor_y = 0;
}

/* Set font color */
void set_color(uint8_t fg, uint8_t bg) {
    color = (bg << 4) | fg;
}

/* Check CPUID */
static int cpuid_supported(void) {
    uint32_t flags;
    __asm__ volatile("pushf; pop %0; mov %0, %%ecx; xor $0x200000, %0; push %0; popf; pushf; pop %0; xor %%ecx, %0"
        : "=r"(flags) :: "ecx");
    return (flags & 0x200000) != 0;
}

/* Read CPU name */
static void print_cpu(void) {
    if (!cpuid_supported()) {
        print("CPU: Unknown (no CPUID)\n");
        return;
    }
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000002));
    char brand[49];
    __asm__ volatile("cpuid" : "=a"(((uint32_t*)brand)[0]), "=b"(((uint32_t*)brand)[1]), "=c"(((uint32_t*)brand)[2]), "=d"(((uint32_t*)brand)[3]) : "a"(0x80000002));
    __asm__ volatile("cpuid" : "=a"(((uint32_t*)brand)[4]), "=b"(((uint32_t*)brand)[5]), "=c"(((uint32_t*)brand)[6]), "=d"(((uint32_t*)brand)[7]) : "a"(0x80000003));
    __asm__ volatile("cpuid" : "=a"(((uint32_t*)brand)[8]), "=b"(((uint32_t*)brand)[9]), "=c"(((uint32_t*)brand)[10]), "=d"(((uint32_t*)brand)[11]) : "a"(0x80000004));
    brand[48] = 0;
    print("CPU: ");
    print(brand);
    print("\n");
}

/* Read memory map from multiboot */
static void print_memory(struct multiboot_info *mbi) {
    uintptr_t addr = (uintptr_t)&mbi->tags;
    while (addr < (uintptr_t)mbi + mbi->total_size) {
        struct multiboot_tag *tag = (struct multiboot_tag *)addr;
        if (tag->type == 0) break; /* end tag */

        if (tag->type == 6) { /* memory map */
            struct __attribute__((packed)) {
                uint32_t type;
                uint32_t size;
                uint32_t entry_size;
                uint32_t entry_version;
            } *mmap = (void *)tag;
            uintptr_t entries = addr + sizeof(*mmap);
            uint32_t count = (tag->size - sizeof(*mmap)) / mmap->entry_size;
            print("Memory regions: ");
            print_hex(count);
            print("\n");
            for (uint32_t i = 0; i < count && i < 8; i++) {
                struct __attribute__((packed)) {
                    uint64_t base;
                    uint64_t length;
                    uint32_t type;
                    uint32_t reserved;
                } *entry = (void *)(entries + (uintptr_t)i * mmap->entry_size);
                if (entry->type == 1) {
                    print("  RAM: ");
                    print_hex((uint32_t)(entry->length / 1024 / 1024));
                    print(" MB\n");
                }
            }
        }
        addr += (tag->size + 7) & ~7; /* align to 8 */
    }
}

__attribute__((used)) void kernel_main(uint32_t magic, struct multiboot_info *mbi) {
    clear_screen();
    set_color(0x0A, 0x00); /* green on black */

    print("\n");
    print("  ╔═══════════════════════════════════════════════════╗\n");
    print("  ║              S Y N T H 3 X   O S                 ║\n");
    print("  ║         Kernel v0.1 — Written in C + ASM         ║\n");
    print("  ╚═══════════════════════════════════════════════════╝\n");
    print("\n");

    set_color(0x07, 0x00);

    if (magic != 0x36D76289) {
        print("ERROR: Not booted by Multiboot-compliant bootloader\n");
        print("Magic: ");
        print_hex(magic);
        print("\n");
        goto halt;
    }

    print("[OK] Bootloader: GRUB (Multiboot2)\n");
    print_cpu();
    print("[OK] Kernel loaded at 0x100000\n");
    print_memory(mbi);
    print("[OK] VGA text mode initialized\n");
    print("\n");

    set_color(0x0B, 0x00); /* bright cyan */
    print("  >> Press enter to select desktop environment...\n");
    print("\n");

    set_color(0x08, 0x00); /* dark gray */
    print("  Available environments:\n");
    print("    [1] Synth3x DE — Custom lightweight desktop\n");
    print("    [2] Xfce      — Full-featured desktop\n");
    print("    [s] Shell     — Command line only\n");
    print("\n");

    set_color(0x07, 0x00);
    print("  Choice: ");

    /* Wait for input */
    for (;;) {
        /* Check keyboard status port */
        if (__builtin_expect(1, 1)) {
            /* Simple polling */
            uint8_t status;
            __asm__ volatile("inb $0x64, %0" : "=a"(status));
            if (status & 0x01) {
                uint8_t scancode;
                __asm__ volatile("inb $0x60, %0" : "=a"(scancode));
                
                /* Convert scancode to ASCII (US layout) */
                static const char ascii[] = {
                    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                    0, 0, 0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
                    'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j',
                    'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b',
                    'n', 'm', ',', '.', '/', 0, '*', 0, ' '
                };
                
                if (scancode < sizeof(ascii) && ascii[scancode]) {
                    char c = ascii[scancode];
                    putchar(c);
                    
                    if (c == '1') {
                        print("\n\n  Loading Synth3x DE...\n");
                        break;
                    } else if (c == '2') {
                        print("\n\n  Starting Xfce...\n");
                        break;
                    } else if (c == 's' || c == 'S') {
                        print("\n\n  Starting shell...\n");
                        break;
                    }
                }
            }
        }
    }

    print("\n[OK] System ready.\n");

halt:
    for (;;) __asm__ volatile("hlt");
}

/* Entry point: start.S sets up stack and calls kernel_main */
