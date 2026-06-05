/* syninit — PID 1 init system
 * Assembly: boot.S — CPUID + splash
 * C:       init.c — mount, fork, shell
 * Rust:    svc/    — DHCP, firewall, health
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>

#include "init.h"

static void vga(const char *s) {
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd >= 0) { write(fd, s, strlen(s)); close(fd); }
    int sf = open("/dev/ttyS0", O_WRONLY);
    if (sf >= 0) { write(sf, s, strlen(s)); close(sf); }
}

static void syn_mount(void) {
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys",  0755); mount("sysfs","/sys", "sysfs", 0, NULL);
    mkdir("/dev",  0755); mount("devtmpfs","/dev","devtmpfs",0,NULL);
    mkdir("/tmp",  0755); mount("tmpfs","/tmp","tmpfs",0,NULL);
    mkdir("/run",  0755); mount("tmpfs","/run","tmpfs",0,NULL);
}

static void syn_hw(void) {
    char cv[16] = ""; syn_cpuid_vendor(cv);
    char b[128];

    char v[64]="", p[64]="";
    int fd = open("/sys/class/dmi/id/sys_vendor", O_RDONLY);
    if(fd>=0){int n=read(fd,v,sizeof(v)-1);if(n>0){v[n]=0;char*q=strchr(v,'\n');if(q)*q=0;}close(fd);}
    fd = open("/sys/class/dmi/id/product_name", O_RDONLY);
    if(fd>=0){int n=read(fd,p,sizeof(p)-1);if(n>0){p[n]=0;char*q=strchr(p,'\n');if(q)*q=0;}close(fd);}

    snprintf(b,sizeof(b),"   CPU: %s\n",cv); vga(b);
    if(v[0]){snprintf(b,sizeof(b),"   Vendor: %s\n",v);vga(b);}
    if(p[0]){snprintf(b,sizeof(b),"   Model: %s\n",p);vga(b);}

    int vm=strstr(v,"QEMU")||strstr(v,"VirtualBox")||strstr(v,"VMware")||strstr(v,"KVM")?1:0;
    vga(vm?"   Environment: Virtual Machine\n":"   Environment: Physical Hardware\n");

    fd=open("/proc/bus/input/devices",O_RDONLY);
    int tp=0;
    if(fd>=0){char tb[4096];int n=read(fd,tb,sizeof(tb)-1);close(fd);
        if(n>0){tb[n]=0;tp=strstr(tb,"Touchpad")||strstr(tb,"Synaptics")||strstr(tb,"ALPS")?1:0;}}
    vga(tp?"   Touchpad: detected\n":"   Touchpad: not detected\n");

    int lap=0; DIR*d=opendir("/sys/class/power_supply");
    if(d){struct dirent*e;while((e=readdir(d))){if(strstr(e->d_name,"BAT")){lap=1;break;}}closedir(d);}
    vga(lap?"   Chassis: Laptop\n":"   Chassis: Desktop\n");

    syn_boot_splash();
}

static void rand_mac(const char *ifname) {
    int s=socket(AF_INET,SOCK_DGRAM,0); if(s<0)return;
    struct ifreq ifr; memset(&ifr,0,sizeof(ifr));
    snprintf(ifr.ifr_name,sizeof(ifr.ifr_name),"%s",ifname);
    srand(time(NULL)^getpid());
    ifr.ifr_hwaddr.sa_family=1; ifr.ifr_hwaddr.sa_data[0]=0x02;
    for(int i=1;i<6;i++) ifr.ifr_hwaddr.sa_data[i]=rand()%256;
    ioctl(s,SIOCSIFHWADDR,&ifr); close(s);
}

static void syn_net(void) {
    vga(" * Randomizing network identity ...\n");
    srand(time(NULL)^getpid());
    char hn[32]; snprintf(hn,sizeof(hn),"synth-%04x",rand()%0xffff);
    sethostname(hn,strlen(hn));
    DIR*d=opendir("/sys/class/net");
    if(d){struct dirent*e;while((e=readdir(d))){
        if(strcmp(e->d_name,".")==0||strcmp(e->d_name,"..")==0||strcmp(e->d_name,"lo")==0)continue;
        rand_mac(e->d_name);}closedir(d);}
    int s=socket(AF_INET,SOCK_DGRAM,0);
    if(s>=0){struct ifreq ifr;memset(&ifr,0,sizeof(ifr));strcpy(ifr.ifr_name,"lo");
        if(ioctl(s,SIOCGIFFLAGS,&ifr)>=0){ifr.ifr_flags|=IFF_UP|IFF_RUNNING;ioctl(s,SIOCSIFFLAGS,&ifr);}close(s);}
    int rf=open("/etc/resolv.conf",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(rf>=0){write(rf,"nameserver 1.1.1.1\nnameserver 8.8.8.8\n",38);close(rf);}
    vga("   [ OK ]  network identity randomized\n");
}

static void syn_services(void) {
    pid_t hw=fork();
    if(hw==0){alarm(15);
        system("for d in /lib/modules/*/;do [ -f \"${d}cfg80211.ko\" ]||continue;"
               "insmod \"${d}cfg80211.ko\" 2>/dev/null;insmod \"${d}e1000.ko\" 2>/dev/null;"
               "insmod \"${d}psmouse.ko\" 2>/dev/null;insmod \"${d}bochs.ko\" 2>/dev/null;"
               "insmod \"${d}virtio-gpu.ko\" 2>/dev/null;break;done 2>/dev/null||true");
        system("modprobe snd_hda_intel 2>/dev/null||true");
        _exit(0);}
    int st;waitpid(hw,&st,0);
    vga(WIFSIGNALED(st)&&WTERMSIG(st)==SIGALRM?"   [TIMEOUT] hardware modules\n":"   [ OK ]  hardware modules\n");

    if(fork()==0){execl("/usr/sbin/nft","nft","-f","/etc/nftables.rules",NULL);_exit(1);}
    vga("   [ OK ]  firewall (nftables)\n");

    /* Launch Rust service manager */
    pid_t svc=fork();
    if(svc==0){execl("/usr/bin/synit-svc","synit-svc","daemon",NULL);_exit(1);}
}

static void sigchld(int sig){(void)sig;while(waitpid(-1,NULL,WNOHANG)>0);}

static void syn_shell(void) {
    for(;;){
        pid_t sh=fork();
        if(sh<0){sleep(1);continue;}
        if(sh==0){setsid();
            int fd=open("/dev/tty1",O_RDWR|O_NOCTTY);
            if(fd>=0){ioctl(fd,TIOCSCTTY,0);}
            if(fd>=0){dup2(fd,0);dup2(fd,1);dup2(fd,2);if(fd>2)close(fd);}
            setenv("HOME","/root",1);setenv("SHELL","/bin/bash",1);
            setenv("USER","root",1);setenv("LOGNAME","root",1);
            setenv("PATH","/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin",1);
            setenv("TERM","linux",1);
            setenv("PS1","\\[\\033[1;36m\\]synth3x-root\\[\\033[0m\\]:\\[\\033[1;33m\\]\\w\\[\\033[0m\\]\\$ ",1);
            execl("/bin/bash","bash","-l",NULL);
            execl("/bin/sh","sh",NULL);_exit(1);}
        int st;waitpid(sh,&st,0);
        if(WIFEXITED(st)&&WEXITSTATUS(st)==0)continue;
        vga("\n[shell restarted...]\n");sleep(1);
    }
}

static void syn_welcome(void) {
    vga("\033[2J\033[H");
    vga("\n");
    vga("\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n");
    vga("\xe2\x95\x91                                                          \xe2\x95\x91\n");
    vga("\xe2\x95\x91                      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88                     \xe2\x95\x91\n");
    vga("\xe2\x95\x91                      \xe2\x96\x88      \xe2\x96\x88      \xe2\x96\x88                     \xe2\x95\x91\n");
    vga("\xe2\x95\x91                     \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88                    \xe2\x95\x91\n");
    vga("\xe2\x95\x91                         \xe2\x96\x88      \xe2\x96\x88  \xe2\x96\x88  \xe2\x96\x88                   \xe2\x95\x91\n");
    vga("\xe2\x95\x91                     \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88                    \xe2\x95\x91\n");
    vga("\xe2\x95\x91                                                          \xe2\x95\x91\n");
    vga("\xe2\x95\x91                     syninit v" VERSION "                  \xe2\x95\x91\n");
    vga("\xe2\x95\x91             a lightweight Gentoo-based distro            \xe2\x95\x91\n");
    vga("\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n");
    vga("\n");
    vga("                                                          .--,-``-.                \n");
    vga("  .--.--.                             ___      ,---,     /   /     '.              \n");
    vga(" /  /    '.                         ,--.'|_  ,--.' |    / ../        ;             \n");
    vga("|  :  /`. /                 ,---,   |  | :,' |  |  :    \\\\ ``\\\\  .`-    '            \n");
    vga(";  |  |--`              ,-+-. /  |  :  : ' : :  :  :     \\\\___\\\\/   \\\\   :,--,  ,--,  \n");
    vga("|  :  ;_         .--,  ,--.'|'   |.;__,'  /  :  |  |,--.      \\\\   :   ||'. \\\\/ .`|  \n");
    vga(" \\\\  \\\\    `.    /_ ./| |   |  ,\\\"' ||  |   |   |  :  '   |      /  /   / '  \\\\/  / ;  \n");
    vga("  `----.   \\\\, ' , ' : |   | /  | |:__,'| :   |  |   /' :      \\\\  \\\\   \\\\  \\\\  \\\\.' /   \n");
    vga("  __ \\\\  \\\\  /___/ \\\\: | |   | |  | |  '  : |__ '  :  | | |  ___ /   :   |  \\\\  ;  ;   \n");
    vga(" /  /`--'  /.  \\\\  ' | |   | |  |/   |  | '.||  |  ' | : /   /\\\\   /   : / \\\\  \\\\  \\\\  \n");
    vga("'--'.     /  \\\\  ;   : |   | |--'    ;  :    ;|  :  :_,:'/ ,,/  ',-    ./__;   ;  \\\\ \n");
    vga("  `--'---'    \\\\  \\\\  ; |   |/        |  ,   / |  | ,'    \\\\ ''\\\\        ;|   :/\\\\  \\\\ ; \n");
    vga("               :  \\\\  \\\\'---'          ---`-'  `--''       \\\\   \\\\     .' `---'  `--`  \n");
    vga("                \\\\  ' ;                                    `--`-,,-'                \n");
    vga("                 `--`                                                               \n");
    vga("\n");
    vga(" * Start Installation:\n");
    vga("   # synth3x-installer\n\n");
    vga(" * WiFi Setup:\n");
    vga("   # synth3x-wifi\n");
    vga("   # iwctl\n\n");
    vga(" * Package Manager:\n");
    vga("   # emerge --ask <package>\n\n");
    vga(" * Download Files:\n");
    vga("   # synth3x-downloader\n\n");
    vga(" * AmnesiaDE Desktop:\n");
    vga("   # synth3x\n\n");
    vga(" * Help:\n");
    vga("   # synth3x-help\n\n");
}

int main(int argc, char *argv[]) {
    setenv("PATH","/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin",1);

    vga("\n syninit v" VERSION "\n");
    vga("══════════════════════════════════════════════════════════\n");

    vga(" * Mounting filesystems ...\n"); syn_mount();
    vga("   [ OK ]  proc, sysfs, devtmpfs, tmpfs\n");

    vga(" * Detecting hardware ...\n"); syn_hw();

    mkdir("/etc",0755);
    int fd=open("/etc/passwd",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(fd>=0){write(fd,"root:x:0:0:root:/root:/bin/bash\n",33);close(fd);}
    fd=open("/etc/group",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(fd>=0){write(fd,"root:x:0:\n",10);close(fd);}

    syn_net();
    vga(" * Loading services ...\n"); syn_services();

    mkdir("/root",0700);
    int rc=open("/etc/bashrc",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(rc>=0){
        const char *br=
            "PS1='\\[\\033[1;36m\\]synth3x-root\\[\\033[0m\\]:\\[\\033[1;33m\\]\\w\\[\\033[0m\\]\\$ '\n"
            "export PS1\nalias ls='ls --color=auto'\nalias ll='ls -la'\n"
            "alias install='synth3x-installer'\nalias wifi='synth3x-wifi'\n"
            "alias dl='synth3x-downloader'\nalias help='synth3x-help'\n"
            "export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin\n";
        write(rc,br,strlen(br));close(rc);}

    struct sigaction sa={.sa_handler=sigchld,.sa_flags=SA_RESTART|SA_NOCLDSTOP};
    sigemptyset(&sa.sa_mask); sigaction(SIGCHLD,&sa,NULL);

    syn_welcome();
    syn_shell();
    return 0;
}
