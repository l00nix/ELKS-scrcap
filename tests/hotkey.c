/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../kernel/scrcap-state.h"
int main(void) {
    assert(!scrcap_scan(0x34,8,0,0,100));
    assert(!scrcap_lease(1,10,100));
    assert(scrcap_lease(2,20,100)==-1);
    assert(!scrcap_scan(0x34,0,0,20,100));
    assert(!scrcap_scan(0x34,9,0,20,100));
    assert(!scrcap_scan(0x34,12,0,20,100));
    assert(!scrcap_scan(0x34,8,1,20,100));
    assert(scrcap_scan(0x34,8,0,20,100)); assert(scrcap.count==1);
    assert(scrcap_scan(0x34,8,0,21,100)); assert(scrcap.count==1);
    assert(scrcap_scan(0xb4,0,0,22,100));
    assert(scrcap_scan(0x34,8,0,23,100)); assert(scrcap.count==2);
    assert(!scrcap_scan(0x34,8,0,311,100));
    assert(!scrcap_lease(2,311,100));
    scrcap.count=65535; assert(scrcap_scan(0x34,8,0,312,100));
    assert((scrcap.count&65535)==0);
    scrcap.owner=0; assert(!scrcap_lease(3,ULONG_MAX-10,100));
    assert(scrcap_live(20,100)); assert(!scrcap_live(400,100));
    puts("PASS hotkey lease, modifiers, extended keys, repeat, expiry, counter/tick wrap");
    return 0;
}
