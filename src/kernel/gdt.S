/* Synth3x OS — GDT (Global Descriptor Table) operations
 * Calling convention: cdecl (i386)
 *
 * Note: GRUB sets up a flat GDT for us.  These routines allow
 * the kernel to reload its own GDT later (e.g. after enabling
 * paging or switching to user mode).
 */

.section .text

/* void gdt_load(uint32_t gdt_ptr)
 *
 * gdt_ptr points to a 6-byte pseudo-descriptor:
 *   bytes 0-1: GDT size - 1
 *   bytes 2-5: linear address of GDT
 */
.global gdt_load
gdt_load:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax
    lgdt    (%eax)
    popl    %ebp
    ret

/* void gdt_reload_segments(void)
 *
 * Reloads all data segment registers and performs a far jump
 * to reload CS.  Assumes:
 *   CS index = 0x08 (ring 0, code)
 *   DS/ES/FS/GS/SS index = 0x10 (ring 0, data)
 *
 * Use after gdt_load() to make the new GDT take effect.
 */
.global gdt_reload_segments
gdt_reload_segments:
    /* far jump to reload CS */
    pushl   $0x08
    pushl   $.Lcs_reload
    lret

.Lcs_reload:
    movw    $0x10, %ax
    movw    %ax,   %ds
    movw    %ax,   %es
    movw    %ax,   %fs
    movw    %ax,   %gs
    movw    %ax,   %ss
    ret
