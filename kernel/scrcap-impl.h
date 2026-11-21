/* SPDX-License-Identifier: GPL-2.0-or-later
 * Included by mem.c under CONFIG_HP200LX_FW. No allocation, screen access,
 * filesystem work or BIOS calls in the keyboard interrupt.
 */
#include "scrcap-state.h"
int scrcap_key(unsigned code,unsigned modifiers,unsigned extended) {
    return scrcap_scan(code,modifiers,extended,jiffies,HZ);
}
static int scrcap_ioctl(int cmd,char *arg) {
    flag_t flags;
    unsigned count;
    int result=0;
    save_flags(flags); clr_irq();
    if(cmd==0x4501) {
        if(scrcap_lease(current->pid,jiffies,HZ)<0) result=-EBUSY;
    } else {
        if(scrcap.owner==current->pid) { scrcap.owner=0; scrcap.down=0; }
        else result=-EPERM;
    }
    count=scrcap.count;
    restore_flags(flags);
    if(!result) put_user(count,arg);
    return result;
}
