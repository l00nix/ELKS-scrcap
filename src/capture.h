/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SCRCAP_CAPTURE_H
#define SCRCAP_CAPTURE_H
#define CAP_BYTES 38400U
#define CAP_LEASE 0x4501
#define CAP_RELEASE 0x4502
#define CAP_AUTO 0
#define CAP_HP 1
#define CAP_CGA 2
#define CAP_PC 3
#define CAP_EGA 4
#define CAP_VGA 5

extern unsigned cap_width,cap_height,cap_stride;
extern const char *cap_error,*cap_font;
void cap_backend(int value);
void cap_text(unsigned rows,unsigned height,const char *fontpath);
int cap_ega_state(const char *value);
int cap_is_hp(void);
int cap_open(const char *image);
int cap_read(int fd);
int cap_save(const char *path, int pbm, int invert);
int cap_numbered(const char *dir, int pbm, int invert, char *path);
#endif
