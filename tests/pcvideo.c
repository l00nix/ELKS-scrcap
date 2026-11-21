/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
static unsigned locked,flags=0x202,ins,index=7;
static unsigned char regs[9],ram[65536];
static unsigned pc_lock(void) { unsigned f=flags;assert(!locked);locked=1;flags&=~512;return f; }
static void pc_unlock(unsigned f) { assert(locked);locked=0;flags=f; }
static unsigned char pc_in(unsigned p) { assert(locked);++ins;return p==0x3ce?index:regs[index]; }
static void pc_out(unsigned p,unsigned char b) { assert(locked);if(p==0x3ce)index=b;else {assert(index<9);regs[index]=b;} }
static unsigned char pc_byte(unsigned off) { assert(locked && regs[4]==2 && !(regs[5]&8));return ram[off]; }
#define PC_TEST
#include "../src/pcvideo.c"
int main(void) {
    unsigned i,ega;unsigned char row[80],before[9],state[]={7,3,0x18};
    for(ega=0;ega<2;ega++) {
        for(i=0;i<9;i++)regs[i]=(unsigned char)(i*17);
        regs[4]=3;regs[5]=0x18;index=7;ins=0;
        for(i=0;i<80;i++)ram[640+i]=(unsigned char)i;
        for(i=0;i<9;i++)before[i]=regs[i];
        assert(!pc_plane_row(640,row,80,ega,state));
        for(i=0;i<80;i++)assert(row[i]==(unsigned char)(i^255));
        for(i=0;i<9;i++)assert(regs[i]==before[i]);
        assert(index==7 && flags==0x202 && !locked);
        assert(ins==(ega?0:3));
        flags=2;assert(!pc_plane_row(640,row,80,ega,state));assert(flags==2);flags=0x202;
    }
    assert(!pc_plane_row(65535,row,1,0,0));
    assert(pc_plane_row(0,row,0,0,0)<0);
    assert(pc_plane_row(0,row,81,0,0)<0);
    assert(pc_plane_row(65535,row,2,0,0)<0);
    assert(pc_plane_row(0,row,80,1,0)<0);
    assert(!locked && flags==0x202);
    puts("PASS VGA/EGA read-map/mode/index/CPU flags restored; no EGA register reads; bounded critical section");
    return 0;
}
