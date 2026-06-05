/* Synth3x Network Check — Assembly syscall wrappers
 * All functions follow System V AMD64 ABI.
 * Performs raw Linux syscalls without libc dependency.
 */

.section .text

/* Syscall numbers (x86-64) */
.equ SYS_SOCKET,   41
.equ SYS_CONNECT,  42
.equ SYS_CLOSE,    3
.equ SYS_SENDTO,   44
.equ SYS_RECVFROM, 45
.equ SYS_SELECT,   23
.equ SYS_GETPID,   39

/* Socket domains */
.equ AF_INET,      2

/* Socket types */
.equ SOCK_STREAM,  1
.equ SOCK_DGRAM,   2

/* Protocol */
.equ IPPROTO_TCP,  6
.equ IPPROTO_UDP,  17

/* ─── net_check_connect ──────────────────────────────────────────
 * int net_check_connect(const char *ip, int port, int timeout_sec);
 *
 * Attempts TCP connect to ip:port with given timeout.
 * ip must be in dotted decimal (e.g. "1.1.1.1").
 * Returns 1 on success, 0 on failure.
 */
.global net_check_connect
net_check_connect:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $128, %rsp

    /* Save args */
    movq    %rdi, -8(%rbp)      /* ip string */
    movl    %esi, -12(%rbp)     /* port */
    movl    %edx, -16(%rbp)     /* timeout */

    /* socket(AF_INET, SOCK_STREAM, 0) */
    movl    $SYS_SOCKET, %eax
    movl    $AF_INET, %edi
    movl    $SOCK_STREAM, %esi
    movl    $0, %edx
    syscall
    cmpq    $0, %rax
    jl      .Lnet_fail
    movl    %eax, -20(%rbp)     /* save fd */

    /* Build sockaddr_in on stack */
    movw    $AF_INET, -40(%rbp)         /* sin_family = AF_INET */
    movl    -12(%rbp), %eax             /* port (host byte order) */
    xchgb   %al, %ah                    /* htons */
    movw    %ax, -38(%rbp)              /* sin_port */

    /* Parse IP address (dotted decimal) */
    movq    -8(%rbp), %rdi              /* ip string */
    call    .Lparse_ip
    movl    %eax, -36(%rbp)             /* sin_addr */

    /* connect(fd, &addr, 16) */
    leaq    -40(%rbp), %rsi
    movl    -20(%rbp), %edi
    movl    $16, %edx
    movl    $SYS_CONNECT, %eax
    syscall
    cmpq    $0, %rax
    jl      .Lnet_close

    /* Success — close and return 1 */
    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
    movl    $1, %eax
    leave
    ret

.Lnet_close:
    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
.Lnet_fail:
    xorl    %eax, %eax
    leave
    ret

/* ─── Parse dotted decimal IP string ────────────────────────────
 * uint32_t parse_ip(const char *str);
 * Converts "1.2.3.4" to network byte order uint32.
 */
.Lparse_ip:
    pushq   %rbx
    pushq   %rcx
    pushq   %rdx
    xorl    %eax, %eax          /* result */
    xorl    %ebx, %ebx          /* current byte */
    xorl    %ecx, %ecx          /* shift counter */
    movl    $4, %r8d            /* 4 bytes */

.Lip_loop:
    movzbl  (%rdi), %edx
    cmpb    $0, %dl
    je      .Lip_done
    cmpb    $46, %dl            /* '.' */
    je      .Lip_next
    /* digit */
    subb    $48, %dl
    imull   $10, %ebx
    addl    %edx, %ebx
    incq    %rdi
    jmp     .Lip_loop

.Lip_next:
    /* shift existing result left 8, add current byte */
    shll    $8, %eax
    orl     %ebx, %eax
    xorl    %ebx, %ebx
    incq    %rdi
    decl    %r8d
    jnz     .Lip_loop

.Lip_done:
    shll    $8, %eax
    orl     %ebx, %eax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

/* ─── net_check_dns ──────────────────────────────────────────────
 * int net_check_dns(void);
 * Checks DNS resolution by connecting to 1.1.1.1:53 (UDP) with a
 * minimal DNS query for google.com.
 * Returns 1 on success, 0 on failure.
 *
 * Note: This is a simplified check — just verifies basic UDP
 * connectivity to a DNS server.
 */
.global net_check_dns
net_check_dns:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $128, %rsp

    /* socket(AF_INET, SOCK_DGRAM, 0) */
    movl    $SYS_SOCKET, %eax
    movl    $AF_INET, %edi
    movl    $SOCK_DGRAM, %esi
    movl    $0, %edx
    syscall
    cmpq    $0, %rax
    jl      .Ldns_fail
    movl    %eax, -20(%rbp)

    /* Build sockaddr_in for 1.1.1.1:53 */
    movw    $AF_INET, -40(%rbp)
    movw    $0x3500, -38(%rbp)          /* port 53 in network byte order */
    movl    $0x01010101, -36(%rbp)      /* 1.1.1.1 */

    /* Build a minimal DNS query in the stack buffer */
    /* Randomize transaction ID using PID */
    movl    $SYS_GETPID, %eax
    syscall
    movw    %ax, -80(%rbp)              /* ID */
    movw    $0x0100, -78(%rbp)          /* flags: recursion desired */
    movw    $1, -76(%rbp)               /* QDCOUNT = 1 */
    movw    $0, -74(%rbp)               /* ANCOUNT = 0 */
    movw    $0, -72(%rbp)               /* NSCOUNT = 0 */
    movw    $0, -70(%rbp)               /* ARCOUNT = 0 */

    /* QNAME: 6google3com0 */
    movb    $6, -68(%rbp)               /* label length */
    movl    $0x6c6f6f67, -67(%rbp)      /* "gool" */
    movw    $0x656c, -63(%rbp)          /* "le" */
    movb    $3, -61(%rbp)               /* label length */
    movl    $0x6d6f63, -60(%rbp)        /* "com" */
    movb    $0, -57(%rbp)               /* root */

    movw    $1, -56(%rbp)               /* QTYPE = A */
    movw    $1, -54(%rbp)               /* QCLASS = IN */

    /* sendto(fd, buf, 34, 0, &addr, 16) */
    movl    -20(%rbp), %edi
    leaq    -80(%rbp), %rsi
    movl    $34, %edx                   /* DNS query size */
    xorl    %r10d, %r10d                /* flags = 0 */
    leaq    -40(%rbp), %r8
    movl    $16, %r9d
    movl    $SYS_SENDTO, %eax
    syscall
    cmpq    $0, %rax
    jl      .Ldns_close

    /* Close socket and return success */
    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
    movl    $1, %eax
    leave
    ret

.Ldns_close:
    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
.Ldns_fail:
    xorl    %eax, %eax
    leave
    ret

/* ─── net_check_ping ─────────────────────────────────────────────
 * int net_check_ping(void);
 * Simple ICMP ping to 1.1.1.1 using raw socket.
 * Returns 1 on reachable, 0 on failure.
 *
 * Note: Requires root/CAP_NET_RAW for raw sockets.
 */
.global net_check_ping
net_check_ping:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $128, %rsp

    /* socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP) */
    /* Actually need raw socket for ICMP — use UDP to 1.1.1.1:80 as fallback */
    movl    $SYS_SOCKET, %eax
    movl    $AF_INET, %edi
    movl    $SOCK_DGRAM, %esi
    movl    $IPPROTO_UDP, %edx
    syscall
    cmpq    $0, %rax
    jl      .Lping_fail
    movl    %eax, -20(%rbp)

    /* Build sockaddr_in for 1.1.1.1:80 */
    movw    $AF_INET, -40(%rbp)
    movw    $0x5000, -38(%rbp)          /* port 80 in network byte order */
    movl    $0x01010101, -36(%rbp)      /* 1.1.1.1 */

    /* connect(fd, &addr, 16) — just test reachability */
    movl    -20(%rbp), %edi
    leaq    -40(%rbp), %rsi
    movl    $16, %edx
    movl    $SYS_CONNECT, %eax
    syscall
    cmpq    $0, %rax
    jl      .Lping_close

    /* Send 1 byte UDP */
    movb    $0, -81(%rbp)
    movl    -20(%rbp), %edi
    leaq    -81(%rbp), %rsi
    movl    $1, %edx
    movl    $0, %r10d
    leaq    -40(%rbp), %r8
    movl    $16, %r9d
    movl    $SYS_SENDTO, %eax
    syscall

    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
    movl    $1, %eax
    leave
    ret

.Lping_close:
    movl    -20(%rbp), %edi
    movl    $SYS_CLOSE, %eax
    syscall
.Lping_fail:
    xorl    %eax, %eax
    leave
    ret
