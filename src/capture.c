/* SPDX-License-Identifier: GPL-2.0-or-later
 * Original ELKS implementation; no GEMCAP code or ROM bundled.
 * Monochrome pixels: top-down, MSB first, 1 = ink, 0 = paper.
 */
#include "capture.h"
#include "pcvideo.h"
#include "font8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
/* IA16 limits a single object to 32767 bytes. Split at a scanline boundary. */
static unsigned char pixels_lo[19200],pixels_hi[19200];
unsigned cap_width=640,cap_height=200,cap_stride=80;
const char *cap_error="capture failed",*cap_font="none (graphics)";
static unsigned char *pixel_row(unsigned y) {
    return y<240?pixels_lo+y*cap_stride:pixels_hi+(y-240)*cap_stride;
}
static unsigned char font[4096],cells[160],ega_state[3];
static int backend,hp_display,offline,ega_known;
static unsigned rows_override,height_override;
static const char *font_file;
void cap_backend(int value) { backend=value; }
void cap_text(unsigned rows,unsigned height,const char *path) {
    rows_override=rows;height_override=height;font_file=path;
}
int cap_is_hp(void) { return hp_display; }
int cap_ega_state(const char *s) {
    unsigned i;char *end;unsigned long v;
    for(i=0;i<3;i++) {
        if(!*s || *s=='-' || *s=='+') return -1;
        v=strtoul(s,&end,0);
        if(end==s || v>(i==0?8UL:i==1?3UL:255UL)) return -1;
        ega_state[i]=(unsigned char)v;
        if(i<2) { if(*end!=',') return -1;s=end+1; }
        else if(*end) return -1;
    }
    ega_known=1;return 0;
}
static unsigned word(const unsigned char *p) { return p[0]|((unsigned)p[1]<<8); }
static int at(int fd,unsigned long addr,void *buf,unsigned n) {
    unsigned char *p=buf;int got;
    if(lseek(fd,(off_t)addr,SEEK_SET)==(off_t)-1) return -1;
    while(n) {
        got=read(fd,p,n);
        if(got<0 && errno==EINTR) continue;
        if(got<=0) { if(!got) errno=EIO;return -1; }
        p+=got;n-=got;
    }
    return 0;
}
static int geometry(unsigned w,unsigned h) {
    unsigned stride=(w+7)/8;
    if(!w || w>640 || !h || h>480 || (unsigned long)stride*h>CAP_BYTES) {
        cap_error="unsupported capture dimensions (maximum 640x480)";errno=EINVAL;return -1;
    }
    cap_width=w;cap_height=h;cap_stride=stride;return 0;
}
int cap_open(const char *image) {
    int fd,found=0;unsigned off,i;unsigned char b[256];
    static const unsigned char sig[]={0xbb,0x50,0x48,0xb9,0x02,0x01,0xba};
    offline=image!=0;
#ifndef __ia16__
    if(!image) { cap_error="host builds require --image (offline fixture)";errno=ENODEV;return -1; }
#endif
    cap_error="cannot open physical screen memory";
    fd=open(image?image:"/dev/kmem",O_RDONLY);if(fd<0)return -1;
    if(backend==CAP_AUTO || backend==CAP_HP) {
        for(off=0xd000;off<0xe000 && !found;off+=240) {
            if(at(fd,0xf0000UL+off,b,sizeof(b))<0) {
                if(backend==CAP_HP)goto fail;
                break; /* generic PC capture need not have readable HP ROM */
            }
            for(i=0;i+10<=sizeof(b);i++)
                if(!memcmp(b+i,sig,sizeof(sig)) && b[i+8]==2 && b[i+9]==0xcf) { found=1;break; }
        }
    }
    if(backend==CAP_HP && !found) { cap_error="HP 200LX ROM identity not found";errno=ENODEV;goto fail; }
    hp_display=found;return fd;
fail:
    { int e=errno;close(fd);errno=e;return -1; }
}
static int nonempty(const unsigned char *p,unsigned n) {
    unsigned i;int zero=1,ones=1;
    for(i=0;i<n;i++) { if(p[i])zero=0;if(p[i]!=255)ones=0; }
    return !zero && !ones;
}
static int loadfont(int fd,unsigned height) {
    int f;unsigned char pointer[4],extra;unsigned long addr;
    if(font_file) {
        cap_error="font file must contain exactly 256 glyphs of the selected height";
        f=open(font_file,O_RDONLY);if(f<0)return -1;
        if(at(f,0,font,256*height)<0 || read(f,&extra,1)!=0) { close(f);errno=EINVAL;return -1; }
        close(f);cap_font="supplied font";return (int)height;
    }
    if(hp_display) {
        cap_error="could not read HP text font";
        if(at(fd,0xffa6eUL,font,1024)<0 || at(fd,0x9fc00UL,font+1024,1024)<0)return -1;
        cap_font="HP firmware 8x8";return 8;
    }
    memcpy(font,fallback8,2048);cap_font="fallback 8x8 (scaled when required)";
    /* CGA/MDA ROM font only. EGA/VGA RAM fonts may differ: use --font for
     * exact loaded glyphs. Never switch adapter font mapping during text capture. */
    if(height==8 && at(fd,0xffa6eUL,font,1024)==0 && nonempty(font,1024)) {
        cap_font="PC ROM lower glyphs; fallback upper glyphs";
        if(at(fd,0x7cUL,pointer,4)==0) {
            addr=(unsigned long)word(pointer+2)*16+word(pointer);
            if(addr>=0x500 && addr<=0xffc00UL && at(fd,addr,font+1024,1024)==0 && nonempty(font+1024,1024))
                cap_font="PC ROM/INT 1F 8x8";
            else memcpy(font+1024,fallback8+1024,1024);
        }
    } else memcpy(font,fallback8,2048);
    return 8;
}
static unsigned luminance(unsigned c) {
    return ((c&4)?30:0)+((c&2)?59:0)+((c&1)?11:0)+((c&8)?40:0);
}
static int text_read(int fd,const unsigned char *b,unsigned mode,unsigned page) {
    unsigned cols=word(b+0x4a),rows,height,row,col,scan;int fh;
    unsigned long base;
    rows=rows_override?rows_override:(b[0x84] && b[0x84]!=255?(unsigned)b[0x84]+1:25);
    height=height_override?height_override:word(b+0x85);
    if(height!=8 && height!=14 && height!=16)height=8;
    if(hp_display) { rows=25;height=8; }
    if((cols!=40 && cols!=80) || (mode==7 && cols!=80) || rows>60 || !rows ||
       (page&1) || (unsigned long)page+(unsigned long)cols*rows*2>32768UL ||
       (hp_display && ((mode!=2 && mode!=3) || cols!=80 || b[0x84]!=24 || page>12384))) {
        cap_error="unsupported text geometry or page";errno=EINVAL;return -1;
    }
    if(geometry(cols*8,rows*height)<0)return -1;
    fh=loadfont(fd,height);if(fh<0)return -1;
    base=(mode==7?0xb0000UL:0xb8000UL)+page;
    cap_error="could not read text cells";
    for(row=0;row<rows;row++) {
        if(at(fd,base+(unsigned long)row*cols*2,cells,cols*2)<0)return -1;
        for(scan=0;scan<height;scan++)for(col=0;col<cols;col++) {
            unsigned ch=cells[col*2],attr=cells[col*2+1];
            unsigned char bits=font[ch*(unsigned)fh+scan*(unsigned)fh/height];

            if(hp_display) {
                unsigned ink=(attr&7)?255:0,paper=(attr&0x70)?255:0;
                pixel_row(row*height+scan)[col]=(bits&ink)|((unsigned char)~bits&paper);
            } else {
                unsigned fg=luminance(attr&15),bg=luminance((attr>>4)&7);
                if(mode==7 && (attr&7)==1 && scan==height-2)bits=255;
                pixel_row(row*height+scan)[col]=fg==bg?0:fg>bg?bits:(unsigned char)~bits;
            }
        }
    }
    return 0;
}
int cap_read(int fd) {
    unsigned char b[256],after[256];unsigned mode,page,y,x;unsigned long addr;
    cap_font="none (graphics)";cap_error="could not read screen memory";
    if(at(fd,0x400UL,b,256)<0)return -1;
    mode=b[0x49]&0x7f;page=word(b+0x4e);
    if(mode==6) {
        if(page || b[0x62] || word(b+0x4a)!=80) { cap_error="unsupported graphics page";errno=EINVAL;return -1; }
        if(geometry(640,200)<0)return -1;
        for(y=0;y<200;y++) {
            addr=0xb8000UL+(y&1)*8192UL+(y/2)*80UL;
            if(at(fd,addr,pixel_row(y),80)<0)return -1;
            if(!hp_display)for(x=0;x<80;x++)pixel_row(y)[x]^=255;
        }
    } else if(mode<=3 || mode==7) {
        if(text_read(fd,b,mode,page)<0)return -1;
    } else if(mode==0x10 || mode==0x12) {
        if(hp_display || backend==CAP_CGA || (mode==0x12 && backend==CAP_EGA))goto unsupported;
        /* Mode 10h does not distinguish original EGA from VGA hardware. */
        if(mode==0x10 && backend!=CAP_EGA && backend!=CAP_VGA) {
            cap_error="mode 10h requires --backend ega or vga";errno=EINVAL;return -1;
        }
        if(backend==CAP_EGA && !ega_known && !offline) {
            cap_error="original EGA requires exact --ega-state index,map,mode (write-only registers)";errno=EINVAL;return -1;
        }
        if(word(b+0x4a)!=80 || geometry(640,mode==0x10?350:480)<0 ||
           (unsigned long)page+(unsigned long)cap_stride*cap_height>65536UL) {
            cap_error="unsupported planar geometry/page";errno=EINVAL;return -1;
        }
        for(y=0;y<cap_height;y++) {
            if(offline) {
                /* Fixture layout: 1 MiB physical image, then four 64 KiB planes. */
                if(at(fd,0x120000UL+page+(unsigned long)y*80,pixel_row(y),80)<0)return -1;
                for(x=0;x<80;x++)pixel_row(y)[x]^=255;
            } else if(pc_plane_row(page+y*80,pixel_row(y),80,backend==CAP_EGA,ega_known?ega_state:0)<0) {
                cap_error="planar adapter read failed";return -1;
            }
        }
    } else {
unsupported:
        cap_error="unsupported video mode/backend";errno=EINVAL;return -1;
    }
    if(at(fd,0x400UL,after,256)<0)return -1;
    if(b[0x49]!=after[0x49] || word(b+0x4a)!=word(after+0x4a) || page!=word(after+0x4e) ||
       b[0x62]!=after[0x62] || b[0x84]!=after[0x84] || word(b+0x85)!=word(after+0x85)) {
        cap_error="video mode/page changed during capture; retry";errno=EAGAIN;return -1;
    }
    return 0;
}
static int putall(int fd, const unsigned char *p, unsigned n) {
    int done;
    while(n) {
        done=write(fd,p,n);
        if(done<0 && errno==EINTR) continue;
        if(done<=0) { if(!done) errno=EIO; return -1; }
        n-=done; p+=done;
    }
    return 0;
}
static void le16(unsigned char *p,unsigned n) { p[0]=n; p[1]=n>>8; }
static void le32(unsigned char *p,unsigned long n) { le16(p,(unsigned)n); le16(p+2,(unsigned)(n>>16)); }
int cap_save(const char *path,int pbm,int invert) {
    unsigned char header[62],line[80];
    unsigned padded=(cap_stride+3)&~3U;
    int fd,result=0,saved;
    unsigned y,x,sy;
    cap_error="cannot create screenshot (existing files are never overwritten)";
    fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0644);
    if(fd<0) return -1;
    cap_error="screenshot write failed";
    if(pbm) {
        int len=snprintf((char *)header,sizeof(header),"P4\n%u %u\n",cap_width,cap_height);
        result=putall(fd,header,(unsigned)len);
    }
    else {
        memset(header,0,sizeof(header)); header[0]='B'; header[1]='M';
        le32(header+2,62UL+(unsigned long)padded*cap_height); le32(header+10,62); le32(header+14,40);
        le32(header+18,cap_width); le32(header+22,cap_height); le16(header+26,1); le16(header+28,1);
        le32(header+34,(unsigned long)padded*cap_height); le32(header+46,2);
        header[54]=header[55]=header[56]=255; /* white then black */
        result=putall(fd,header,sizeof(header));
    }
    for(y=0;y<cap_height && !result;y++) {
        sy=pbm?y:cap_height-1-y;
        memset(line,0,sizeof(line));
        for(x=0;x<cap_stride;x++) line[x]=pixel_row(sy)[x]^(invert?255:0);
        if(cap_width&7)line[cap_stride-1]&=(unsigned char)(255U<<(8-(cap_width&7)));
        result=putall(fd,line,pbm?cap_stride:padded);
    }
    saved=errno;
    if(close(fd)<0 && !result) { result=-1; saved=errno; }
    if(result) unlink(path); /* Only the file successfully created by us. */
    errno=saved; return result;
}
int cap_numbered(const char *dir,int pbm,int invert,char *path) {
    unsigned n;
    cap_error="output directory path too long";
    if(strlen(dir)>220) { errno=ENAMETOOLONG; return -1; }
    for(n=0;n<10000;n++) {
        snprintf(path,240,"%s%sCAP%04u.%s",dir,dir[0] && dir[strlen(dir)-1]=='/'?"":"/",n,pbm?"PBM":"BMP");
        if(cap_save(path,pbm,invert)==0) return 0;
        if(errno!=EEXIST) return -1;
    }
    cap_error="all CAP0000 through CAP9999 filenames already exist";
    errno=EEXIST; return -1;
}
