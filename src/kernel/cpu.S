/* Synth3x OS — CPU identification and control
 * Calling convention: cdecl (i386)
 */

.section .text

/* int cpuid_supported(void)
 * Returns 1 if CPUID instruction is available, 0 otherwise.
 * Uses the ID flag (bit 21) in EFLAGS.
 */
.global cpuid_supported
cpuid_supported:
    pushf
    popl    %eax
    movl    %eax, %ecx
    xorl    $0x200000, %eax
    pushl   %eax
    popf
    pushf
    popl    %eax
    xorl    %ecx, %eax
    shrl    $21, %eax
    andl    $1, %eax
    ret

/* void cpuid(uint32_t leaf,
 *            uint32_t *eax_out, uint32_t *ebx_out,
 *            uint32_t *ecx_out, uint32_t *edx_out)
 */
.global cpuid
cpuid:
    pushl   %ebp
    movl    %esp, %ebp
    pushl   %ebx
    pushl   %edi

    movl    8(%ebp),  %eax
    cpuid

    movl    12(%ebp), %edi
    testl   %edi, %edi
    jz      1f
    movl    %eax, (%edi)
1:
    movl    16(%ebp), %edi
    testl   %edi, %edi
    jz      2f
    movl    %ebx, (%edi)
2:
    movl    20(%ebp), %edi
    testl   %edi, %edi
    jz      3f
    movl    %ecx, (%edi)
3:
    movl    24(%ebp), %edi
    testl   %edi, %edi
    jz      4f
    movl    %edx, (%edi)
4:
    popl    %edi
    popl    %ebx
    popl    %ebp
    ret

/* void halt(void) */
.global halt
halt:
    hlt
    ret

/* void cli(void) */
.global cli
cli:
    cli
    ret

/* void sti(void) */
.global sti
sti:
    sti
    ret
