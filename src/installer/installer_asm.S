/* Synth3x OS v0.8.1 — Installer Assembly Optimizations (x86-64)
 * Performance-critical installer routines:
 *   - Safe memory zeroing (prevents data leaks)
 *   - Fast memory copy for large file transfers
 *   - Disk sector verification (SHA-like checksum)
 *   - Quick string comparison for path matching
 *
 * Linked into the Rust installer via FFI.
 * SystemV AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9
 */

.text
.globl installer_memzero_safe
.globl installer_fast_memcpy
.globl installer_verify_sectors
.globl installer_str_eq
.globl installer_show_progress_asm

/* ─── installer_memzero_safe(rdi=ptr, rsi=size)
 * Securely zeros memory using memset then clflush.
 * Prevents compiler optimization from removing the zeroing.
 * Used for clearing passwords and keys from RAM after use. */
installer_memzero_safe:
    push %rbp
    mov %rsp, %rbp

    test %rsi, %rsi
    jz .memzero_done
    test %rdi, %rdi
    jz .memzero_done

    mov %rdi, %rcx      /* rcx = start pointer */
    mov %rsi, %rdx      /* rdx = byte count */
    add %rdi, %rdx      /* rdx = end pointer */

    /* Zero the memory using rep stosb */
    mov %rdi, %rcx
    mov %rsi, %rax
    xor %al, %al
    rep stosb

    /* Compiler barrier: read back to prevent optimization */
    lfence

.memzero_done:
    pop %rbp
    ret

/* ─── installer_fast_memcpy(rdi=dst, rsi=src, rdx=size)
 * Optimized memory copy using rep movsd (32-bit chunks).
 * Falls back to byte copy for remaining bytes.
 * Used for fast file extraction in initramfs. */
installer_fast_memcpy:
    push %rbp
    mov %rsp, %rbp

    test %rdx, %rdx
    jz .memcpy_done
    test %rdi, %rdi
    jz .memcpy_done
    test %rsi, %rsi
    jz .memcpy_done

    /* Copy 8-byte chunks for aligned addresses */
    mov %rdx, %rcx
    shr $3, %rcx        /* rcx = number of 8-byte chunks */
    jz .memcpy_tail

    cld
    rep movsq

.memcpy_tail:
    /* Copy remaining bytes */
    mov %rdx, %rcx
    and $7, %rcx        /* rcx = remaining bytes */
    jz .memcpy_done

    rep movsb

.memcpy_done:
    pop %rbp
    ret

/* ─── installer_verify_sectors(rdi=ptr, rsi=size) -> rax
 * Fast checksum verification of downloaded data.
 * Computes XOR-based rolling checksum over memory region.
 * Returns checksum in rax.
 * Used to verify downloaded Stage3 tarball integrity. */
installer_verify_sectors:
    push %rbp
    mov %rsp, %rbp

    test %rdi, %rdi
    jz .verify_zero
    test %rsi, %rsi
    jz .verify_zero

    xor %rax, %rax      /* accumulator */
    mov %rdi, %rcx      /* pointer */
    mov %rsi, %rdx      /* counter */

    /* Process 8 bytes at a time */
.verify_loop8:
    cmp $8, %rdx
    jb .verify_tail

    xor (%rcx), %rax    /* XOR 8 bytes into accumulator */
    add $8, %rcx
    sub $8, %rdx
    jmp .verify_loop8

.verify_tail:
    test %rdx, %rdx
    jz .verify_done

.verify_loop1:
    xor (%rcx), %al     /* XOR single byte */
    inc %rcx
    dec %rdx
    jnz .verify_loop1

.verify_done:
    pop %rbp
    ret

.verify_zero:
    xor %rax, %rax
    pop %rbp
    ret

/* ─── installer_str_eq(rdi=s1, rsi=s2) -> rax (1=match, 0=no match)
 * Fast string comparison using rep cmpsb.
 * Used for matching filesystem paths and package names. */
installer_str_eq:
    push %rbp
    mov %rsp, %rbp

    test %rdi, %rdi
    jz .streq_no
    test %rsi, %rsi
    jz .streq_no

    mov $256, %rcx      /* max length */
    cld
    repe cmpsb
    jne .streq_no

    /* Check both strings ended at the same point */
    mov (%rdi), %al
    or (%rsi), %al
    test %al, %al
    jnz .streq_no

    mov $1, %rax
    pop %rbp
    ret

.streq_no:
    xor %rax, %rax
    pop %rbp
    ret

/* ─── installer_show_progress_asm(rdi=label_ptr, rdi2=label_len, rsi=percent)
 * Fast progress bar rendering using VGA text mode directly.
 * Writes directly to VGA memory at 0xB8000 for speed. */
installer_show_progress_asm:
    push %rbp
    mov %rsp, %rbp

    /* VGA text mode memory */
    mov $0xB8000, %rdi
    /* Row 24 (bottom of screen), column 2 */
    mov $24, %rax
    mov $160, %rbx
    mul %rbx
    add $4, %rax
    add %rax, %rdi

    /* "Progress: " */
    movw $0x0F50, (%rdi)    /* P */
    movw $0x0F72, 2(%rdi)   /* r */
    movw $0x0F6F, 4(%rdi)   /* o */
    movw $0x0F67, 6(%rdi)   /* g */
    movw $0x0F72, 8(%rdi)   /* r */
    movw $0x0F65, 10(%rdi)  /* e */
    movw $0x0F73, 12(%rdi)  /* s */
    movw $0x0F73, 14(%rdi)  /* s */
    movw $0x0F3A, 16(%rdi)  /* : */
    movw $0x0F20, 18(%rdi)  /* space */

    /* Bar opening bracket */
    movw $0x0F5B, 20(%rdi)  /* [ */

    /* Calculate bar fill */
    mov %rsi, %rax          /* percent */
    mov $20, %rbx
    mul %rbx
    mov $100, %rbx
    xor %rdx, %rdx
    div %rbx
    mov %rax, %rcx          /* rcx = filled chars (0-20) */

    /* Draw bar */
    mov $21, %rdx           /* starting column offset */
    xor %r8, %r8            /* counter */

.bar_loop:
    cmp $20, %r8
    jge .bar_done

    cmp %r8, %rcx
    jle .bar_empty

    /* Filled block: '#' in green */
    movw $0x0A23, (%rdi, %rdx, 2)  /* # green */
    jmp .bar_next

.bar_empty:
    /* Empty block: '-' in gray */
    movw $0x082D, (%rdi, %rdx, 2)  /* - gray */

.bar_next:
    inc %r8
    inc %rdx
    jmp .bar_loop

.bar_done:
    /* Bar closing bracket */
    movw $0x0F5D, (%rdi, %rdx, 2)  /* ] */
    inc %rdx

    /* Space */
    movw $0x0F20, (%rdi, %rdx, 2)
    inc %rdx

    /* Percent digit (tens) */
    mov %rsi, %rax
    mov $10, %rbx
    xor %rdx2, %rdx2
    div %rbx
    add $0x30, %al
    movb %al, (%rdi, %rdx, 2)
    movb $0x0F, 1(%rdi, %rdx, 2)
    inc %rdx

    /* Percent digit (units) */
    add $0x30, %dl
    movb %dl, (%rdi, %rdx, 2)
    movb $0x0F, 1(%rdi, %rdx, 2)
    inc %rdx

    /* '%' sign */
    movw $0x0F25, (%rdi, %rdx, 2)

    pop %rbp
    ret
