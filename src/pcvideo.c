/* SPDX-License-Identifier: GPL-2.0-or-later
 * Read GEMCAP-compatible monochrome plane 2 without modifying video RAM.
 * VGA GC registers are readable. Original EGA registers are write-only:
 * that backend must be given the exact current index/read-map/mode state.
 */
#include "pcvideo.h"
#include <errno.h>
#if defined(__ia16__) || defined(PC_TEST)
#ifndef PC_TEST
static unsigned pc_lock(void) {
    unsigned f;
    __asm__ volatile("pushf; pop %0; cli":"=r"(f)::"memory");
    return f;
}
static void pc_unlock(unsigned f) {
    __asm__ volatile("push %0; popf"::"r"(f):"memory","cc");
}
static unsigned char pc_in(unsigned port) {
    unsigned b; /* IA16 byte constraint a also admits AH: reserve the full AX. */
    __asm__ volatile("inb %%dx,%%al":"=a"(b):"d"(port):"memory");
    return b;
}
static void pc_out(unsigned port,unsigned char b) {
    __asm__ volatile("outb %%al,%%dx"::"a"((unsigned)b),"d"(port):"memory");
}
static unsigned char pc_byte(unsigned off) {
    volatile unsigned char __far *p=(volatile unsigned char __far *)(0xa0000000UL+off);
    return *p;
}
#endif
int pc_plane_row(unsigned offset,unsigned char *dst,unsigned n,int ega,
                 const unsigned char *state) {
    unsigned f,i;
    unsigned char index,map,mode;
    if(!n || n>80 || (unsigned long)offset+n>65536UL || (ega && !state)) { errno=EINVAL; return -1; }
    f=pc_lock();
    if(ega) { index=state[0];map=state[1];mode=state[2]; }
    else {
        index=pc_in(0x3ce);
        pc_out(0x3ce,4);map=pc_in(0x3cf);
        pc_out(0x3ce,5);mode=pc_in(0x3cf);
    }
    pc_out(0x3ce,5);pc_out(0x3cf,mode&~8U); /* read mode 0 */
    pc_out(0x3ce,4);pc_out(0x3cf,2);        /* red plane */
    for(i=0;i<n;i++) dst[i]=pc_byte(offset+i)^255;
    pc_out(0x3ce,4);pc_out(0x3cf,map);
    pc_out(0x3ce,5);pc_out(0x3cf,mode);
    pc_out(0x3ce,index);
    pc_unlock(f);
    return 0;
}
#else
int pc_plane_row(unsigned offset,unsigned char *dst,unsigned n,int ega,
                 const unsigned char *state) {
    (void)offset;(void)dst;(void)n;(void)ega;(void)state;
    errno=ENODEV;return -1;
}
#endif
