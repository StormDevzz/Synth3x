/* Synth3x OS v0.8.1 — Fast WiFi Scanner (x86-64 ASM)
 * Scan via trigger+dump, completes in ~1 second.
 *
 * Build: as -o fast_scan.o fast_scan.S && ld -o fast_scan fast_scan.o
 *
 * Uses pure Linux syscalls, no libc.
 * - Trigger scan (returns instantly)
 * - Sleep 1 second
 * - Dump cached results
 * - Filter SSID lines
 */

.section .data
iw_path:  .asciz "/usr/bin/iw"
sh_path:  .asciz "/bin/sh"
sh_flag:  .asciz "-c"
trig_cmd: .asciz "iw dev wlan0 scan trigger 2>/dev/null"
dump_cmd: .asciz "iw dev wlan0 scan dump 2>/dev/null"
ssid_label:.asciz "     SSID: "
sig_label: .asciz "  Signal: "
dbm_str:   .asciz " dBm\n"
newline:   .asciz "\n"
sep:       .asciz "     ------------------------------\n"
header:    .asciz "     Available Networks:\n"
none_msg:  .asciz "     No networks found\n"

.align 4
timespec:
  .quad 1, 0          /* 1 sec, 0 nsec */

.section .bss
.lcomm buf, 8192

.section .text
.globl _start

_start:
    /* ── Step 1: trigger scan (returns immediately) ── */
    mov $sh_path, %rdi
    mov $sh_flag, %rsi
    mov $trig_cmd, %rdx
    call exec_quiet

    /* ── Step 2: nanosleep 1 second ── */
    mov $35, %rax
    mov $timespec, %rdi
    xor %rsi, %rsi
    syscall

    /* ── Step 3: dump scan results into pipe ── */
    call do_dump

    /* ── Step 4: exit ── */
    mov $60, %rax
    xor %rdi, %rdi
    syscall

/* ─── exec_quiet(rdi=path, rsi=arg0, rdx=arg1) ───
 * Forks and exec's with stdout redirected to /dev/null */
exec_quiet:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    mov %rdi, %r12
    mov %rsi, %r13

    mov $57, %rax
    syscall
    test %rax, %rax
    jnz .eq_wait

    /* Child: close stdout, exec */
    mov $3, %rax
    mov %r12, %rdi
    mov %r13, %rsi
    mov %rdx, %rdx
    mov $59, %rax
    syscall
    mov $60, %rax
    mov $1, %rdi
    syscall

.eq_wait:
    mov %rax, %rdi
    mov $61, %rax
    xor %rsi, %rsi
    xor %rdx, %rdx
    xor %r10, %r10
    syscall

    pop %r13
    pop %r12
    pop %rbp
    ret

/* ─── do_dump ───
 * Runs iw scan dump, reads output, filters and displays SSIDs */
do_dump:
    push %rbp
    mov %rsp, %rbp

    /* Create pipe */
    sub $16, %rsp
    mov %rsp, %rdi
    mov $22, %rax
    syscall

    mov $57, %rax
    syscall
    test %rax, %rax
    jnz .dd_parent

    /* ─── Child ─── */
    /* Close read end */
    mov (%rsp), %rdi
    mov $3, %rax
    syscall

    /* dup2 write end to stdout */
    mov 4(%rsp), %rdi
    mov $1, %rsi
    mov $33, %rax
    syscall

    /* Close write end */
    mov 4(%rsp), %rdi
    mov $3, %rax
    syscall

    /* exec iw dev wlan0 scan dump */
    mov $sh_path, %rdi
    mov $sh_flag, %rsi
    mov $dump_cmd, %rdx
    xor %rax, %rax
    mov %rdi, %r12
    mov $57, %rax
    syscall
    /* If fork failed, try exec directly */
    mov $59, %rax
    syscall
    mov $60, %rax
    mov $1, %rdi
    syscall

    /* ─── Parent ─── */
.dd_parent:
    /* Close write end */
    mov 4(%rsp), %rdi
    mov $3, %rax
    syscall

    /* Read from pipe */
    mov (%rsp), %rdi
    mov $buf, %rsi
    mov $8192, %rdx
    xor %rax, %rax
    syscall

    /* Wait for child */
    mov %rax, %r12          /* saved bytes read */
    mov $61, %rax
    mov %rax, %rdi
    xor %rsi, %rsi
    xor %rdx, %rdx
    xor %r10, %r10
    syscall

    /* Display header */
    mov $1, %rax
    mov $1, %rdi
    mov $header, %rsi
    mov $30, %rdx
    syscall

    mov $1, %rax
    mov $1, %rdi
    mov $sep, %rsi
    mov $34, %rdx
    syscall

    /* Parse and display SSIDs */
    mov $buf, %rdi
    mov %r12, %rsi
    call parse_and_show

    mov $1, %rax
    mov $1, %rdi
    mov $sep, %rsi
    mov $34, %rdx
    syscall

    add $16, %rsp
    pop %rbp
    ret

/* ─── parse_and_show(rdi=buf, rsi=len) ───
 * Walk through scan dump output, display SSID + signal lines */
parse_and_show:
    push %rbp
    mov %rsp, %rbp
    push %r12
    push %r13
    push %r14

    mov %rdi, %r12
    mov %rsi, %r13
    xor %r14, %r14        /* offset */
    xor %r8, %r8          /* found flag */

.lp:
    cmp %r13, %r14
    jge .lpd

    /* Check for "SSID:" */
    mov $buf, %rax
    add %r14, %rax
    mov (%rax), %ecx
    cmp $0x3A444953, %ecx       /* "SSID" little-endian */
    jne .check_sig

    /* Found SSID — print it */
    add $6, %r14                /* skip "SSID: " */
    mov $1, %rax
    mov $1, %rdi
    mov $ssid_label, %rsi
    mov $13, %rdx
    syscall

    /* Print SSID value until newline or end */
    xor %r9, %r9
.ssid_loop:
    mov $buf, %rax
    add %r14, %rax
    add %r9, %rax
    movb (%rax), %cl
    test %cl, %cl
    jz .ssid_done
    cmp $10, %cl                /* newline */
    je .ssid_done
    inc %r9
    cmp $64, %r9
    jl .ssid_loop

.ssid_done:
    mov $1, %rax
    mov $1, %rdi
    mov $buf, %rsi
    add %r14, %rsi
    mov %r9, %rdx
    syscall

    mov $1, %rax
    mov $1, %rdi
    mov $newline, %rsi
    mov $1, %rdx
    syscall

    mov $1, %r8                  /* found something */

.check_sig:
    /* Advance to next line */
    add $1, %r14
    jmp .lp

.lpd:
    test %r8, %r8
    jnz .lpe

    /* No networks found */
    mov $1, %rax
    mov $1, %rdi
    mov $none_msg, %rsi
    mov $21, %rdx
    syscall

.lpe:
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    ret
