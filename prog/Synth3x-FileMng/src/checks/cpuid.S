/* Synth3x CPU Feature Detection — Assembly CPUID Wrappers
 * All functions follow System V AMD64 ABI (x86-64).
 * Returns feature flags readable from C.
 */

.section .text

/* ─── cpu_get_vendor ────────────────────────────────────────────
 * char *cpu_get_vendor(char buf[13]);
 * Returns CPU vendor string via CPUID leaf 0.
 */
.global cpu_get_vendor
cpu_get_vendor:
    pushq   %rbx
    pushq   %rcx
    pushq   %rdx
    xorl    %eax, %eax
    cpuid
    movl    %ebx, (%rdi)
    movl    %edx, 4(%rdi)
    movl    %ecx, 8(%rdi)
    movb    $0,    12(%rdi)
    movq    %rdi, %rax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

/* ─── cpu_features_ecx ──────────────────────────────────────────
 * uint32_t cpu_features_ecx(void);
 * Returns ECX from CPUID leaf 1 (feature bits).
 */
.global cpu_features_ecx
cpu_features_ecx:
    pushq   %rcx
    pushq   %rdx
    movl    $1, %eax
    cpuid
    movl    %ecx, %eax
    popq    %rdx
    popq    %rcx
    ret

/* ─── cpu_features_edx ──────────────────────────────────────────
 * uint32_t cpu_features_edx(void);
 * Returns EDX from CPUID leaf 1 (feature bits).
 */
.global cpu_features_edx
cpu_features_edx:
    pushq   %rcx
    pushq   %rdx
    movl    $1, %eax
    cpuid
    movl    %edx, %eax
    popq    %rdx
    popq    %rcx
    ret

/* ─── cpu_features_ext ──────────────────────────────────────────
 * uint32_t cpu_features_ext(void);
 * Returns ECX from CPUID leaf 0x80000001 (extended features).
 */
.global cpu_features_ext
cpu_features_ext:
    pushq   %rcx
    pushq   %rdx
    movl    $0x80000001, %eax
    cpuid
    movl    %ecx, %eax
    popq    %rdx
    popq    %rcx
    ret

/* ─── cpu_brand_string ──────────────────────────────────────────
 * char *cpu_brand_string(char buf[49]);
 * Reads CPU brand via leaves 0x80000002-0x80000004.
 */
.global cpu_brand_string
cpu_brand_string:
    pushq   %rbx
    pushq   %rcx
    pushq   %rdx
    movq    %rdi, %rsi
    movl    $0x80000002, %eax
    cpuid
    movl    %eax,    (%rsi)
    movl    %ebx,   4(%rsi)
    movl    %ecx,   8(%rsi)
    movl    %edx,  12(%rsi)
    movl    $0x80000003, %eax
    cpuid
    movl    %eax,  16(%rsi)
    movl    %ebx,  20(%rsi)
    movl    %ecx,  24(%rsi)
    movl    %edx,  28(%rsi)
    movl    $0x80000004, %eax
    cpuid
    movl    %eax,  32(%rsi)
    movl    %ebx,  36(%rsi)
    movl    %ecx,  40(%rsi)
    movl    %edx,  44(%rsi)
    movb    $0,    48(%rsi)
    movq    %rdi, %rax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

/* ─── cpu_cores ─────────────────────────────────────────────────
 * uint32_t cpu_cores(void);
 * Returns number of CPU cores from CPUID leaf 0x80000008 (ECX bits 0-7).
 */
.global cpu_cores
cpu_cores:
    pushq   %rcx
    pushq   %rdx
    movl    $0x80000008, %eax
    cpuid
    andl    $0xff, %ecx
    movl    %ecx, %eax
    popq    %rdx
    popq    %rcx
    ret

/* ─── cpu_family ────────────────────────────────────────────────
 * uint32_t cpu_family(void);
 * Returns CPU family from CPUID leaf 1 (EAX bits 8-11, with extended bits).
 */
.global cpu_family
cpu_family:
    pushq   %rcx
    pushq   %rdx
    pushq   %rbx
    movl    $1, %eax
    cpuid
    movl    %eax, %ecx
    shrl    $8, %eax
    andl    $0xf, %eax
    movl    %ecx, %edx
    shrl    $20, %edx
    andl    $0xff, %edx
    cmpl    $0xf, %eax
    cmove   %edx, %eax
    popq    %rbx
    popq    %rdx
    popq    %rcx
    ret

/* ─── cpu_model ─────────────────────────────────────────────────
 * uint32_t cpu_model(void);
 * Returns CPU model from CPUID leaf 1 (EAX bits 4-7, with extended bits).
 */
.global cpu_model
cpu_model:
    pushq   %rcx
    pushq   %rdx
    movl    $1, %eax
    cpuid
    movl    %eax, %ecx
    shrl    $4, %eax
    andl    $0xf, %eax
    movl    %ecx, %edx
    shrl    $16, %edx
    andl    $0xf, %edx
    shll    $4, %edx
    orl     %edx, %eax
    popq    %rdx
    popq    %rcx
    ret
