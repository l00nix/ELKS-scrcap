/* SPDX-License-Identifier: GPL-2.0-or-later
 * Exercise writer handling of short writes, EINTR, disk-full and close failure.
 */
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
static int failure,calls;
static ssize_t test_write(int fd,const void *p,size_t n) {
    ++calls;
    if(calls==1) { errno=EINTR; return -1; }
    if(failure==1 && calls==8) { errno=ENOSPC; return -1; }
    if(failure==2 && calls==8) return 0;
    return write(fd,p,n>7?7:n);
}
static int test_close(int fd) {
    int result=close(fd);
    if(failure==3) { errno=EIO; return -1; }
    return result;
}
#define write test_write
#define close test_close
#include "../src/capture.c"
#undef write
#undef close
int main(void) {
    char dir[]="/tmp/scrcap-faults-XXXXXX",path[240];
    struct stat st;
    assert(mkdtemp(dir));snprintf(path,sizeof(path),"%s/test.bmp",dir);
    assert(!cap_save(path,0,0));assert(!stat(path,&st) && st.st_size==16062);
    assert(!unlink(path));
    for(failure=1;failure<=3;failure++) {
        calls=0;assert(cap_save(path,0,0)==-1);assert(stat(path,&st)==-1 && errno==ENOENT);
    }
    failure=0;calls=0;
    assert(!geometry(13,3));
    pixel_row(0)[0]=0x11;pixel_row(0)[1]=0xff;
    pixel_row(1)[0]=0x22;pixel_row(1)[1]=0xff;
    pixel_row(2)[0]=0x33;pixel_row(2)[1]=0xff;
    assert(!cap_save(path,0,0));
    { unsigned char b[74];int f=open(path,O_RDONLY);
      assert(f>=0 && read(f,b,sizeof(b))==74);close(f);
      assert(b[62]==0x33 && b[63]==0xf8 && !b[64] && !b[65]);
      assert(b[66]==0x22 && b[70]==0x11); }
    assert(!unlink(path));
    assert(!rmdir(dir));puts("PASS short writes, EINTR, ENOSPC, zero writes, close errors, partial-file cleanup and non-byte-aligned BMP padding");
    return 0;
}
