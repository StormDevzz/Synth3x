/* Synth3x OS — Hardware Detection: CPUID (x86-64)
 *
 * Provides low-level CPUID wrappers to detect CPU vendor,
 * features (SSE, AVX, VMX, etc.), and topology.
 *
 * All functions follow System V AMD64 ABI.
 */

.section .text

/* ─── hw_cpuid_vendor ────────────────────────────────────────────
 * char *hw_cpuid_vendor(char buf[13]);
 *
 * Returns CPU vendor string (12 chars + NUL) via CPUID leaf 0.
 * Returns pointer to buf, or 0 on failure.
 */
.global hw_cpuid_vendor
hw_cpuid_vendor:
    pushq   %rbx
    pushq   %rcx
    pushq   %rdx

    xorl    %eax, %eax
    cpuid
    /* eax = max leaf, ebx:edx:ecx = "GenuineIntel" / "AuthenticAMD" etc */
    movl    %ebx, (%rdi)          /* first  4 bytes */
    movl    %edx, 4(%rdi)         /* next   4 bytes */
    movl    %ecx, 8(%rdi)         /* next   4 bytes */
    movb    $0,    12(%rdi)       /* NUL terminator */

    movq    %rdi, %rax

    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

/* ─── hw_cpuid_features ───────────────────────────────────────────
 * uint64_t hw_cpuid_features(void);
 *
 * Returns feature bitmask from CPUID leaf 1 (low 32 = ECX, high 32 = EDX).
 */
.global hw_cpuid_features
hw_cpuid_features:
    pushq   %rcx
    pushq   %rdx

    movl    $1, %eax
    cpuid
    /* ecx = feature bits (SSE3, SSSE3, SSE4.1, SSE4.2, AVX, etc) */
    /* edx = feature bits (MMX, SSE, SSE2, HTT, etc) */
    shlq    $32, %rdx
    movl    %ecx, %edx
    movq    %rdx, %rax

    popq    %rdx
    popq    %rcx
    ret

/* ─── hw_cpuid_has_avx ────────────────────────────────────────────
 * int hw_cpuid_has_avx(void);
 * Returns 1 if AVX is supported, 0 otherwise.
 */
.global hw_cpuid_has_avx
hw_cpuid_has_avx:
    pushq   %rcx
    movl    $1, %eax
    cpuid
    testl   $0x10000000, %ecx     /* bit 28 = AVX */
    setnz   %al
    movzbl  %al, %eax
    popq    %rcx
    ret

/* ─── hw_cpuid_has_sse2 ───────────────────────────────────────────
 * int hw_cpuid_has_sse2(void);
 */
.global hw_cpuid_has_sse2
hw_cpuid_has_sse2:
    pushq   %rdx
    movl    $1, %eax
    cpuid
    testl   $0x04000000, %edx     /* bit 26 = SSE2 */
    setnz   %al
    movzbl  %al, %eax
    popq    %rdx
    ret

/* ─── hw_cpuid_brand_string ──────────────────────────────────────
 * char *hw_cpuid_brand_string(char buf[49]);
 *
 * Reads CPU brand string using leaf 0x80000002–0x80000004.
 * Returns pointer to buf (48 chars + NUL).
 */
.global hw_cpuid_brand_string
hw_cpuid_brand_string:
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
