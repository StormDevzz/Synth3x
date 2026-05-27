/* Synth3x DE v0.5 — GNOME-like shell
 * C + Assembly rendering pipeline.  Double-buffered.
 * Build: gcc -O2 -Wall synth3x.c fb_asm.S font.S -o synth3x
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>

#include "synth3x.h"

/* ─── CONFIG ─── */
#define MAX_WIN     16
#define MAX_NOTIF   8
#define PANEL_H     28
#define NOTIF_W     280
#define NOTIF_H     72
#define NOTIF_DUR   5
#define WORKSPACES  4

#define RGB565(r,g,b) ((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))

/* ─── COLORS ─── */
typedef struct { unsigned char r, g, b; } Color;
static Color lerp(Color a, Color b, float t) {
    return (Color){ a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t };
}
static uint16_t c565(Color c) { return RGB565(c.r,c.g,c.b); }

#define T(n) Color n
static struct { T(bg); T(panel_bg); T(panel_fg); T(win_bg); T(win_title);
    T(win_border); T(notif_bg); T(notif_fg); T(accent); T(text); T(dim); } theme;

static void theme_update(void) {
    time_t t = time(NULL);
    int h = localtime(&t)->tm_hour;
    Color morning = {200,220,240}, day = {180,200,230}, eve = {210,180,170}, night={15,12,20};
    Color p_morn={220,230,245}, p_day={230,235,245}, p_eve={235,215,200}, p_night={22,20,30};
    Color f_morn={40,50,60}, f_day={30,40,50}, f_eve={50,30,20}, f_night={180,180,200};
    Color w_morn={245,245,250}, w_day={250,250,250}, w_eve={250,240,235}, w_night={25,22,35};
    Color t_morn={210,220,240}, t_day={220,225,235}, t_eve={225,205,190}, t_night={28,25,38};
    Color a_morn={60,120,220}, a_day={50,110,210}, a_eve={200,100,60}, a_night={100,120,200};
    Color x_morn={30,30,40}, x_day={20,20,30}, x_eve={40,25,15}, x_night={200,200,210};
    Color d_morn={120,130,140}, d_day={110,120,130}, d_eve={130,110,100}, d_night={100,100,120};
    
    float f; Color *b1,*b2,*p1,*p2,*f1,*f2,*w1,*w2,*t1,*t2,*a1,*a2,*x1,*x2,*d1,*d2;
    if (h>=6&&h<12){b1=&morning;b2=&day;p1=&p_morn;p2=&p_day;f1=&f_morn;f2=&f_day;w1=&w_morn;w2=&w_day;t1=&t_morn;t2=&t_day;a1=&a_morn;a2=&a_day;x1=&x_morn;x2=&x_day;d1=&d_morn;d2=&d_day;f=(h-6)/6.0f;}
    else if(h>=12&&h<18){b1=&day;b2=&eve;p1=&p_day;p2=&p_eve;f1=&f_day;f2=&f_eve;w1=&w_day;w2=&w_eve;t1=&t_day;t2=&t_eve;a1=&a_day;a2=&a_eve;x1=&x_day;x2=&x_eve;d1=&d_day;d2=&d_eve;f=(h-12)/6.0f;}
    else if(h>=18&&h<22){b1=&eve;b2=&night;p1=&p_eve;p2=&p_night;f1=&f_eve;f2=&f_night;w1=&w_eve;w2=&w_night;t1=&t_eve;t2=&t_night;a1=&a_eve;a2=&a_night;x1=&x_eve;x2=&x_night;d1=&d_eve;d2=&d_night;f=(h-18)/4.0f;}
    else{b1=&night;b2=&morning;p1=&p_night;p2=&p_morn;f1=&f_night;f2=&f_morn;w1=&w_night;w2=&w_morn;t1=&t_night;t2=&t_morn;a1=&a_night;a2=&a_morn;x1=&x_night;x2=&x_morn;d1=&d_night;d2=&d_morn;f=(h>=22)?(h-22)/8.0f:(h+2)/8.0f;}
    #define L(field) theme.field = lerp(*##field##1, *##field##2, f)
    theme.bg=lerp(*b1,*b2,f);theme.panel_bg=lerp(*p1,*p2,f);theme.panel_fg=lerp(*f1,*f2,f);
    theme.win_bg=lerp(*w1,*w2,f);theme.win_title=lerp(*t1,*t2,f);theme.notif_bg=lerp(*w1,*w2,f);
    theme.notif_fg=lerp(*x1,*x2,f);theme.accent=lerp(*a1,*a2,f);theme.text=lerp(*x1,*x2,f);
    theme.dim=lerp(*d1,*d2,f);
}

/* ─── GLOBALS ─── */
static int fb_fd = -1, fb_w = 800, fb_h = 600;
static uint16_t *fb, *backbuf;
static int running = 1;
static int mx = 400, my = 300, mbtn = 0, mclick = 0;

/* ─── WINDOWS ─── */
typedef struct { int x,y,w,h; char title[48]; int hidden,ws,drag,dx,dy; } Win;
static Win wins[MAX_WIN]; static int wc = 0, aw = -1;

static int wnew(const char *t, int w, int h) {
    if (wc>=MAX_WIN) return -1;
    Win *wn = &wins[wc++];
    wn->x = 60+(wc*40)%(fb_w-w-80); wn->y = PANEL_H+40+(wc*30)%(fb_h-PANEL_H-h-80);
    wn->w = w; wn->h = h; wn->hidden=0; wn->ws=0; wn->drag=0;
    strncpy(wn->title, t, 47); aw = wc-1; return aw;
}
static int win_title(Win *w, int x, int y) {
    return x>=w->x && x<=w->x+w->w && y>=w->y-24 && y<=w->y;
}
static int win_close(Win *w, int x, int y) {
    return x>=w->x+w->w-22 && x<=w->x+w->w-4 && y>=w->y-22 && y<=w->y-4;
}

/* ─── NOTIFICATIONS ─── */
typedef struct { char title[48]; char body[128]; time_t t; } Notif;
static Notif notifs[MAX_NOTIF]; static int nc = 0;
static int notif_fd = -1;

static void notif_add(const char *title, const char *body) {
    if(nc>=MAX_NOTIF){memmove(notifs,notifs+1,sizeof(Notif)*(MAX_NOTIF-1));nc--;}
    Notif *n = &notifs[nc++];
    strncpy(n->title,title,47); n->title[47]=0;
    strncpy(n->body,body,127); n->body[127]=0; n->t=time(NULL);
}

static void notif_init(void) {
    unlink("/tmp/synth3x-notif"); mkfifo("/tmp/synth3x-notif", 0666);
    notif_fd = open("/tmp/synth3x-notif", O_RDONLY|O_NONBLOCK);
}
static void notif_read(void) {
    char buf[512]; int n = read(notif_fd, buf, 511); if (n<=0) return; buf[n]=0;
    char *l = buf; while(*l) {
        char *nl = strchr(l,'\n'); if(nl)*nl=0;
        char *sp = strchr(l,'|');
        if (nc<MAX_NOTIF) {
            Notif *no = &notifs[nc++];
            if(sp){*sp=0;strncpy(no->title,l,47);strncpy(no->body,sp+1,127);}
            else{strncpy(no->title,"Notification",47);strncpy(no->body,l,127);}
            no->t = time(NULL);
        }
        if(!nl)break; l=nl+1;
    }
}

/* ─── FRAMEBUFFER (C wrappers for fast asm primitives) ─── */
static void px(int x,int y,uint16_t c) {
    fb_pixel(backbuf, fb_w, fb_h, x, y, c);
}
static void rect(int x,int y,int w,int h,uint16_t c) {
    int x2=x+w,y2=y+h; if(x<0)x=0;if(y<0)y=0;if(x2>fb_w)x2=fb_w;if(y2>fb_h)y2=fb_h;
    fb_fill_rect(backbuf, fb_w, fb_h, x, y, x2-x, y2-y, c);
}

static void fchar(int x,int y,char c,uint16_t fg,uint16_t bg) {
    if(c<32||c>126)c=' ';
    fb_blit_char(backbuf, fb_w, fb_h, x, y, &font8x8[(c-32)*8], fg, bg);
}
static void fstr(int x,int y,const char *s,uint16_t fg,uint16_t bg) {
    while(*s){fchar(x,y,*s++,fg,bg);x+=8;}
}

/* ─── DRAWING ─── */
static void draw_bg(void) {
    uint16_t bg=c565(theme.bg), grid=c565(lerp(theme.bg,(Color){0,0,0},0.04f));
    rect(0,PANEL_H,fb_w,fb_h-PANEL_H,bg);
    for(int x=0;x<fb_w;x+=32) px(x,PANEL_H,grid);
    for(int y=PANEL_H;y<fb_h;y+=32) px(0,y,grid);
}

static void draw_win(Win *w) {
    if(w->hidden) return;
    uint16_t bg=c565(theme.win_bg), bd=c565(theme.win_border);
    uint16_t tl=c565(theme.win_title), tx=c565(theme.text), dm=c565(theme.dim);
    int t = w->y - 24;
    rect(w->x+4, t+4, w->w, w->h+24, c565((Color){0,0,0}));
    rect(w->x-1, t-1, w->w+2, w->h+26, bd);
    rect(w->x, t, w->w, 24, tl);
    fstr(w->x+6, t+8, w->title, tx, tl);
    rect(w->x+w->w-22, t+2, 18, 18, RGB565(200,40,40));
    fstr(w->x+w->w-18, t+6, "X", tx, RGB565(0,0,0));
    rect(w->x, w->y, w->w, w->h, bg);
    char buf[64];
    for(int i=0;i<4;i++) { snprintf(buf,64,"$ Synth3x OS  —  ws %d/%d",w->ws+1,WORKSPACES);
        fstr(w->x+8,w->y+8+i*18,buf,dm,bg); }
}

static void draw_notifs(void) {
    time_t now=time(NULL);
    int y=PANEL_H+10, x=fb_w-NOTIF_W-10;
    for(int i=0;i<nc&&i<3;i++) {
        if(now-notifs[i].t>NOTIF_DUR+2){memmove(notifs+i,notifs+i+1,sizeof(Notif)*(nc-i-1));nc--;i--;continue;}
        uint16_t nb=c565(theme.notif_bg), nf=c565(theme.notif_fg), ac=c565(theme.accent), dm=c565(theme.dim);
        rect(x,y,NOTIF_W,NOTIF_H,nb); rect(x,y,4,NOTIF_H,ac);
        rect(x,y,NOTIF_W,1,ac); rect(x,y+NOTIF_H-1,NOTIF_W,1,dm);
        fstr(x+12,y+8,notifs[i].title,ac,nb); fstr(x+12,y+30,notifs[i].body,nf,nb);
        char s[16]; snprintf(s,16,"%ds",(int)(now-notifs[i].t));
        fstr(x+NOTIF_W-40,y+8,s,dm,nb);
        y+=NOTIF_H+5;
    }
}

static void draw_panel(void) {
    uint16_t bg=c565(theme.panel_bg), fg=c565(theme.panel_fg), ac=c565(theme.accent), dm=c565(theme.dim);
    rect(0,0,fb_w,PANEL_H,bg); rect(0,PANEL_H-1,fb_w,1,ac);
    fstr(8,10,"Synth3x",ac,bg);
    char ws[16]; snprintf(ws,16,"WS %d/%d",0+1,WORKSPACES); fstr(90,10,ws,fg,bg);
    time_t t=time(NULL); char ts[16]; strftime(ts,16," %H:%M ",localtime(&t));
    fstr(fb_w-8*strlen(ts)-8,10,ts,fg,bg);
    if(nc){char ns[8];snprintf(ns,8,"%d!",nc);fstr(fb_w-8*strlen(ts)-8*strlen(ns)-16,10,ns,RGB565(255,80,60),bg);}
}

static void swap(void) { memcpy(fb, backbuf, fb_w*fb_h*2); }

/* ─── INPUT ─── */
static int kbd = -1, mouse = -1;

static void input_init(void) {
    kbd = open("/dev/input/by-path/platform-i8042-serio-0-event-kbd", O_RDONLY|O_NONBLOCK);
    if(kbd<0) kbd = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
    if(kbd<0) kbd = open("/dev/input/event1", O_RDONLY|O_NONBLOCK);
    
    /* Scan for mouse events */
    mouse = -1;
    const char *paths[] = {
        "/dev/input/by-path/platform-i8042-serio-1-event-mouse",
        "/dev/input/by-path/platform-i8042-serio-2-event-mouse",
        "/dev/input/event0","/dev/input/event1","/dev/input/event2",
        "/dev/input/event3","/dev/input/event4","/dev/input/event5",
        "/dev/input/mice",NULL};
    for(int i=0;paths[i];i++) {
        int fd = open(paths[i], O_RDONLY|O_NONBLOCK);
        if(fd>=0) { mouse=fd; break; }
    }
    
    /* If no mouse, try brute force */
    if(mouse<0) {
        DIR *d = opendir("/dev/input");
        if(d) { struct dirent *de;
            while((de=readdir(d))) {
                if(strncmp(de->d_name,"event",5)) continue;
                char p[64]; snprintf(p,64,"/dev/input/%s",de->d_name);
                int fd = open(p, O_RDONLY|O_NONBLOCK);
                if(fd>=0&&mouse<0) { /* Check if it's a mouse */
                    unsigned char keytype[2] = {EV_REL, EV_KEY};
                    struct pollfd pf = {fd, POLLIN, 0};
                    if(poll(&pf,1,0)>=0) { mouse=fd; break; }
                } else if(fd>=0) close(fd);
            } closedir(d);
        }
    }
}

static void handle_key(int code) {
    if(code==1) running=0;                     /* ESC */
    if(code==2) {                               /* 1 */
        wnew("Terminal", fb_w/2-40, fb_h/3);
    }
    if(code==58) {                              /* CapsLock → close active win */
        if(aw>=0 && aw<wc) wins[aw].hidden=1;
    }
    if(code==15) {                              /* Tab */
        for(int i=1;i<=wc;i++) {
            int ni = (aw+i)%wc;
            if(!wins[ni].hidden) { aw=ni; break; }
        }
    }
    if(code==103||code==108) { int d=(code==108)?1:-1; /* Up/Down = prev/next ws */
        int nw = (wins[0].ws+d+WORKSPACES)%WORKSPACES;
        for(int i=0;i<wc;i++) wins[i].ws=nw;
    }
}

/* ─── MAIN LOOP ─── */
int main(int argc, char *argv[]) {
    printf("Synth3x DE v0.4\n");
    
    fb_fd = open("/dev/fb0", O_RDWR);
    if(fb_fd<0) { printf("No /dev/fb0\n"); return 1; }
    struct fb_var_screeninfo vi;
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi);
    fb_w=vi.xres; fb_h=vi.yres;
    uint16_t *fbmap = mmap(NULL, fb_w*fb_h*2, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if(fbmap==MAP_FAILED) { close(fb_fd); return 1; }
    fb = fbmap;
    backbuf = malloc(fb_w*fb_h*2);
    if(!backbuf) { munmap(fb,fb_w*fb_h*2); close(fb_fd); return 1; }
    
    int tty = open("/dev/tty0", O_RDWR);
    if(tty>=0) ioctl(tty, KDSETMODE, KD_GRAPHICS);
    
    input_init(); notif_init();
    
    printf("[OK] %dx%d  kbd:%d mouse:%d notif:%d\n", fb_w, fb_h, kbd, mouse, notif_fd);
    
    wnew("Terminal", fb_w/2-40, fb_h/3);
    wnew("System Info", 360, 200);
    
    notif_add("Welcome","Synth3x DE v0.4: pure C desktop.");
    notif_add("Keyboard","1=new win Caps=close Tab=switch Up/Dn=ws");
    
    while(running) {
        theme_update();
        
        /* Poll input */
        struct pollfd fds[4]; int nf=0;
        if(kbd>=0){fds[nf].fd=kbd;fds[nf].events=POLLIN;nf++;}
        if(mouse>=0){fds[nf].fd=mouse;fds[nf].events=POLLIN;nf++;}
        if(notif_fd>=0){fds[nf].fd=notif_fd;fds[nf].events=POLLIN;nf++;}
        
        if(poll(fds,nf,16)>0) {
            struct input_event ev;
            for(int i=0;i<nf;i++) {
                if(!(fds[i].revents&POLLIN)) continue;
                if(fds[i].fd==kbd) {
                    while(read(kbd,&ev,sizeof(ev))==sizeof(ev))
                        if(ev.type==EV_KEY&&ev.value==1) handle_key(ev.code);
                } else if(fds[i].fd==mouse) {
                    while(read(mouse,&ev,sizeof(ev))==sizeof(ev)) {
                        if(ev.type==EV_REL&&ev.code==REL_X) mx+=ev.value*2;
                        if(ev.type==EV_REL&&ev.code==REL_Y) my+=ev.value*2;
                        if(ev.type==EV_KEY&&ev.code==BTN_LEFT&&ev.value==1) mclick=1;
                        if(ev.type==EV_KEY&&ev.code==BTN_LEFT&&ev.value==0) {
                            for(int j=0;j<wc;j++) wins[j].drag=0; mclick=0;
                        }
                    }
                } else if(fds[i].fd==notif_fd) notif_read();
            }
        }
        
        mx = mx<0?0:(mx>=fb_w?fb_w-1:mx);
        my = my<PANEL_H?PANEL_H:(my>=fb_h?fb_h-1:my);
        
        /* Handle click */
        if(mclick) {
            mclick=0;
            int hit=-1;
            for(int j=wc-1;j>=0;j--) if(!wins[j].hidden&&win_title(&wins[j],mx,my)){hit=j;break;}
            if(hit>=0) {
                if(win_close(&wins[hit],mx,my)) {
                    wins[hit].hidden=1;
                } else {
                    /* Bring to front */
                    Win t = wins[hit];
                    memmove(&wins[hit],&wins[hit+1],sizeof(Win)*(wc-hit-1));
                    wins[wc-1]=t; aw=wc-1;
                    wins[aw].drag=1; wins[aw].dx=mx-wins[aw].x; wins[aw].dy=my-(wins[aw].y-24);
                }
            }
        }
        
        /* Drag */
        for(int i=0;i<wc;i++) if(wins[i].drag) {
            wins[i].x=mx-wins[i].dx; wins[i].y=my-wins[i].dy+24;
            if(wins[i].x<0)wins[i].x=0; if(wins[i].y<PANEL_H+24)wins[i].y=PANEL_H+24;
            if(wins[i].x+wins[i].w>fb_w)wins[i].x=fb_w-wins[i].w;
            if(wins[i].y+wins[i].h>fb_h)wins[i].y=fb_h-wins[i].h;
        }
        
        /* Draw */
        memset(backbuf,0,fb_w*fb_h*2);
        draw_bg();
        for(int i=0;i<wc;i++) draw_win(&wins[i]);
        draw_notifs(); draw_panel();
        
        /* Cursor */
        uint16_t cur = c565(theme.text);
        for(int i=-5;i<=5;i++) px(mx+i,my,cur);
        for(int i=-5;i<=5;i++) px(mx,my+i,cur);
        px(mx,my,c565(theme.accent));
        
        swap();
    }
    
    free(backbuf); munmap(fb,fb_w*fb_h*2); close(fb_fd);
    if(tty>=0) ioctl(tty, KDSETMODE, KD_TEXT);
    printf("Synth3x DE: done.\n");
    for(;;) pause();
}
