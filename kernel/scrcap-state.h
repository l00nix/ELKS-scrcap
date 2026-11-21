/* SPDX-License-Identifier: GPL-2.0-or-later
 * Pure state machine shared with tests. Call under saved interrupt flags.
 * A three-second renewable lease prevents a dead listener owning the key.
 */
struct scrcap_state {
    int owner;
    unsigned long renewed;
    unsigned count;
    unsigned char down;
};
static struct scrcap_state scrcap;
static int scrcap_live(unsigned long now, unsigned hz) {
    return scrcap.owner && now-scrcap.renewed < 3UL*hz;
}
static int scrcap_lease(int pid,unsigned long now,unsigned hz) {
    if(scrcap_live(now,hz) && scrcap.owner!=pid) return -1;
    if(!scrcap_live(now,hz)) scrcap.down=0;
    scrcap.owner=pid; scrcap.renewed=now;
    return 0;
}
static int scrcap_scan(unsigned code,unsigned modifiers,unsigned extended,
                       unsigned long now,unsigned hz) {
    if(extended || (code&0x7f)!=0x34) return 0;
    if(code&0x80) {
        int consumed=scrcap.down;
        scrcap.down=0; return consumed;
    }
    if(!scrcap_live(now,hz)) { scrcap.down=0; return 0; }
    if(scrcap.down) return 1; /* Suppress typematic repeats until release. */
    if((modifiers&0x0f)!=0x08) return 0; /* Alt only, no Ctrl/Shift. */
    scrcap.down=1; ++scrcap.count; return 1;
}
