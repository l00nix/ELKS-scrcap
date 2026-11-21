/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PCVIDEO_H
#define PCVIDEO_H
/* n <= 80. EGA requires caller-supplied GC index/read-map/mode state. */
int pc_plane_row(unsigned offset,unsigned char *dst,unsigned n,int ega,
                 const unsigned char *state);
#endif
