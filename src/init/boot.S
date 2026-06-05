/* syninit — Assembly boot: CPUID, splash */

.section .text

.global syn_cpuid_vendor
syn_cpuid_vendor:
    push %rbx
    xor  %eax, %eax
    cpuid
    mov  %ebx, (%rdi)
    mov  %edx, 4(%rdi)
    mov  %ecx, 8(%rdi)
    movb $0,   12(%rdi)
    mov  %rdi, %rax
    pop  %rbx
    ret

.global syn_cpuid_has_avx
syn_cpuid_has_avx:
    mov  $1, %eax
    cpuid
    test $1<<28, %ecx
    jnz  1f
    xor  %eax, %eax
    ret
1:  mov  $1, %eax
    ret

.global syn_cpuid_has_sse2
syn_cpuid_has_sse2:
    mov  $1, %eax
    cpuid
    test $1<<26, %edx
    jnz  1f
    xor  %eax, %eax
    ret
1:  mov  $1, %eax
    ret

.global syn_boot_splash
syn_boot_splash:
    push %rbp
    mov  %rsp, %rbp
    mov  $2, %rax
    lea  fb_path(%rip), %rdi
    xor  %rsi, %rsi
    xor  %rdx, %rdx
    syscall
    cmp  $0, %rax
    jl   splash_done
    mov  %rax, %r12
    xor  %rdi, %rdi
    mov  $0x300000, %rsi
    mov  $3, %rdx
    mov  $1, %r10
    xor  %r8, %r8
    mov  %r12, %r9
    mov  $9, %rax
    syscall
    cmp  $0xfffffffffffff001, %rax
    ja   splash_close
    mov  %rax, %rdi
    mov  $0x0001, %ecx
    mov  $0x001f, %r8w
    mov  $800, %esi
    mov  $600, %edx
    call splash_fast_fill
    mov  %r12, %rdi
    mov  $6, %rax
    syscall
splash_close:
    mov  %r12, %rdi
    mov  $6, %rax
    syscall
splash_done:
    pop  %rbp
    ret

.global splash_fast_fill
splash_fast_fill:
    movzwl %cx,   %r8d
    movl   %esi,  %eax
    mull   %edx
    movl   %eax,  %ecx
    movzwl %r8w,  %eax
    rep    stosw
    ret

.global splash_pixel
splash_pixel:
    movl   %ecx,  %eax
    imull  %esi,  %eax
    addl   %edx,  %eax
    movzwl %r8w,  %r9d
    movw   %r9w,  (%rdi,%rax,2)
    ret

.data
fb_path: .asciz "/dev/fb0"
